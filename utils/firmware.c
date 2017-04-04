#include	<stdio.h>
#include	<ctype.h>
#include	<stdarg.h>
#include	<stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <uuid/uuid.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <sys/time.h>
#include <sys/stat.h>
#include "vector.h"

/**
 * This is the structure of the firmware file. There is a fixed size header followed by one or more block headers,
 * which point to the data in the rest of the file.
 * All data in the headers is little endian
 */

#define    FW_TAG        0x55A322BF        // magic number to identify the file

#define KEY_LEN     (256/8)
#define IV_LEN (128/8)
#define UUID_LEN    16
#define BLOCK_SIZE  16      // round blocks up by this for encryption.

typedef struct {
    unsigned char tag[4];            // magic number goes here
    unsigned char major[2];        // major version number
    unsigned char minor[2];        // minor version number
    unsigned char numblocks[2];        // the number of blocks in the file,
    unsigned char unused[6];
    unsigned char service_uuid[UUID_LEN];    // the service uuid of the bootloader
} firmware;

/** A block header.
 *
 */

typedef struct {
    unsigned char addr[4];        // the address at which to load this block
    unsigned char size[4];        // the size of this block
    unsigned char offset[4];        // offset in the file of the data
    unsigned char padding[1];   // length of padding at end
    unsigned char unused[3];
    unsigned char init_vector[IV_LEN];    // the block 0 initialization vector
    unsigned char sha256[32];       // SHA256 hash of the data
} block_header;

typedef struct {
    unsigned char *data;
    unsigned int length;
    unsigned int fileLength;
    unsigned long addr;
} memblock;

unsigned long base, last;
int lineno, major, minor;
char *filename;
char *outfile;
char verbose;
char lbuf[1024];
unsigned char keybuf[KEY_LEN];
unsigned long offset;
vector_t memblocks;
vector_t assembledBlocks;
uuid_t uuid;
firmware fw;
long baseAddress;   // the expected base address



void add_data(unsigned long i, unsigned int cnt, unsigned char *buffer);

static void
put2(unsigned char *addr, unsigned int val) {
    *addr++ = (unsigned char) (val & 0xFF);
    *addr = (unsigned char) ((val >> 8) & 0xFF);
}

static void
put4(unsigned char *addr, unsigned long val) {
    *addr++ = (unsigned char) (val & 0xFF);
    *addr++ = (unsigned char) ((val >> 8) & 0xFF);
    *addr++ = (unsigned char) ((val >> 16) & 0xFF);
    *addr = (unsigned char) ((val >> 24) & 0xFF);
}

void
hexerror(char *f, ...) {
    va_list ap;

    va_start(ap, f);
    fprintf(stderr, "%s: %d: ", filename, lineno);
    vfprintf(stderr, f, ap);
    putc('\n', stderr);
    fprintf(stderr, "line: \"%s", lbuf);
    exit(1);
}

void
error(char *f, ...) {
    va_list ap;

    va_start(ap, f);
    fprintf(stderr, "%s: %d: ", filename, lineno);
    vfprintf(stderr, f, ap);
    putc('\n', stderr);
    exit(1);
}


unsigned char
getx(char **p) {
    unsigned char hi, lo;

    hi = (unsigned char) *(*p)++;
    lo = (unsigned char) *(*p)++;
    if (!isxdigit(hi) || !isxdigit(lo)) {
        hexerror("Saw 0%o and 0%o:- hex digit expected", hi, lo);
    }
    if (islower(hi))
        hi = (unsigned char) toupper(hi);
    if (islower(lo))
        lo = (unsigned char) toupper(lo);
    if (hi >= 'A')
        hi -= 'A' - ('9' + 1);
    if (lo >= 'A')
        lo -= 'A' - ('9' + 1);
    hi -= '0';
    lo -= '0';
    return (hi << 4) + lo;
}


int
readHexLine() {
    int i, j, cksum, type;
    unsigned int cnt;
    unsigned addr;
    unsigned char buffer[256];
    char *p;

    if (fgets(lbuf, sizeof lbuf, stdin) == NULL)
        return 0;
    lineno++;
    p = lbuf;
    while ((i = *p++) != ':')
        if (i == 0)
            return 1;
    cnt = getx(&p);
    cksum = getx(&p);
    addr = getx(&p);
    addr += (cksum << 8);
    /* addr -= 0x100;*/
    cksum += addr + cnt;
    cksum += type = getx(&p);
    for (j = 0; j != cnt; j++) {
        unsigned char c = getx(&p);
        buffer[j] = c;
        cksum += c;
    }
    cksum += getx(&p);
    if ((cksum & 0xFF) != 0) {
        hexerror("Checksum error");
    }
    switch (type) {
        case 1:    /* EOF */
            return 0;

        case 0:    /* data */
            add_data(base + addr, cnt, buffer);
            break;

        case 4:    /* extended linear address */
            base = ((unsigned long) buffer[1] << 16) + ((unsigned long) buffer[0] << 24);
            if (verbose > 1)
                fprintf(stderr, "%d: Linear address %lX\n", lineno, base);
            break;

        case 2:    /* extended segment address */
            base = ((unsigned long) buffer[1] << 4) + ((unsigned long) buffer[0] << 12);
            if (verbose > 1)
                fprintf(stderr, "%d: Segment address %lX\n", lineno, base);
            break;

        case 3:
        case 5:
            break;

        default:
            hexerror("Unknown record type %2.2X - length %d", type, cnt);
            break;

    }
    return 1;
}

void add_data(unsigned long addr, unsigned int cnt, unsigned char *buffer) {
    memblock *mb = calloc(sizeof(memblock), 1);
    mb->data = malloc(cnt);
    memcpy(mb->data, buffer, cnt);
    mb->length = cnt;
    mb->addr = addr;
    vec_add(memblocks, mb);
}

int cmpBlock(const void *a1, const void *a2) {
    const memblock *m1, *m2;
    m1 = *(memblock **) a1;
    m2 = *(memblock **) a2;
    if (m1->addr < m2->addr)
        return -1;
    if (m1->addr > m2->addr)
        return 1;
    return 0;
}

void combine(unsigned i, unsigned j) {
    memblock *m1 = vec_elementAt(memblocks, i);
    memblock *ml = vec_elementAt(memblocks, j - 1);
    unsigned totLength = (unsigned int) (ml->addr + ml->length - m1->addr);
    totLength += BLOCK_SIZE;
    totLength &= ~(BLOCK_SIZE - 1);
    m1->fileLength = totLength;
    unsigned char *buf = calloc(totLength, 1);
    for (unsigned k = i; k != j; k++) {
        memblock *m2 = vec_elementAt(memblocks, k);
        memcpy(buf + m2->addr - m1->addr, m2->data, m2->length);
        free(m2->data);
        if (k != i)
            free(m2);
    }
    m1->data = buf;
    m1->length = (unsigned int) (ml->addr + ml->length - m1->addr);
    memset(buf + m1->length, (unsigned char) (totLength - m1->length), (totLength - m1->length));
    vec_add(assembledBlocks, m1);
}

void assembleBlocks() {

    memblock *m1, *m2;

    if (memblocks->count == 0) {
        error("No data read");
    }
    vec_sort(memblocks, cmpBlock);
    unsigned int i = 0;
    m1 = vec_first(memblocks);
    unsigned int length = m1->length;
    for (unsigned int j = 1; j != vec_size(memblocks); j++) {
        m2 = vec_elementAt(memblocks, j);
        if (m1->addr + length >= m2->addr - BLOCK_SIZE - 1) {
            length += m2->length;
            continue;
        }
        combine(i, j);
        i = j + 1;
    }
    if (i != vec_size(memblocks))
        combine(i, vec_size(memblocks));
}

void writeData() {
    memblock *m1;
    int outlen;
    EVP_CIPHER_CTX *ctx;
    ctx = EVP_CIPHER_CTX_new();
    EVP_MD_CTX * shaCtx = EVP_MD_CTX_create();
    if (ctx == NULL) {
        error("Failed to initialise cipher context");
    }
    block_header header;
    memset(&fw, 0, sizeof(fw));
    for (int i = 0; i != UUID_LEN; i++)
        fw.service_uuid[i] = uuid[i];
    put4(fw.tag, FW_TAG);
    put2(fw.major, (unsigned int) major);
    put2(fw.minor, (unsigned int) minor);
    put2(fw.numblocks, (unsigned int) vec_size(assembledBlocks));
    unsigned long offset = sizeof(firmware) + sizeof(block_header) * vec_size(assembledBlocks);
    fseek(stdout, 0L, SEEK_SET);
    fwrite(&fw, sizeof fw, 1, stdout);
    VEC_ITERATE(assembledBlocks, m1, memblock *) {
        memset(&header, 0, sizeof(header));
        arc4random_buf(header.init_vector, sizeof header.init_vector);
        if(EVP_DigestInit_ex(shaCtx, EVP_sha256(), NULL) != 1)
            error("Sha digest init failed");
        if(EVP_DigestUpdate(shaCtx, m1->data, m1->fileLength) != 1)
            error("Sha digest update failed");
        unsigned int digest_len;
        if(EVP_MD_size(EVP_sha256()) != sizeof(header.sha256))
            error("Sha digest length wrong");
        if(EVP_DigestFinal(shaCtx, header.sha256, &digest_len) != 1|| digest_len != sizeof(header.sha256))
            error("SHA digest final failed");
        if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, keybuf, header.init_vector) != 1) {
            error("EncryptInit_ex failed");
        }
        unsigned char *outbuf = malloc(m1->fileLength);
        if (EVP_EncryptUpdate(ctx, outbuf, &outlen, m1->data, m1->fileLength) != 1) {
            error("EncryptUpdate failed");
        }
        if (outlen != m1->fileLength) {
            error("Mismatched length after encryption - %d should be %d", outlen, m1->fileLength);
        }
        free(m1->data);
        m1->data = outbuf;
        put4(header.addr, m1->addr);
        put4(header.size, m1->length);
        put4(header.offset, offset);
        header.padding[0] = (unsigned char) (m1->fileLength - m1->length);
        offset += m1->fileLength;
        fwrite(&header, sizeof header, 1, stdout);
    }
    EVP_CIPHER_CTX_free(ctx);
    EVP_MD_CTX_destroy(shaCtx);
    VEC_ITERATE(assembledBlocks, m1, memblock *) {
        fwrite(m1->data, 1, m1->fileLength, stdout);
    }
    fclose(stdout);
}

bool file_exists(char *fileName) {
    struct stat buf;
    return stat(fileName, &buf) == 0;
}

char *readKey(char *fileName) {
    FILE *fp = fopen(fileName, "rb");
    size_t len;
    char *buf = calloc((KEY_LEN*2)+1, 1);
    if (!fp) {
        error("Failed to open key file %s", fileName);
    }
    len =  fread(buf, 1, KEY_LEN*2, fp);
    if(len != KEY_LEN*2) {
        error("Short read on key file %s", fileName);
    }
    fclose(fp);
    return buf;

}


int
main(int argc, char **argv) {
    memblock *m1;
    char *arg;
    bool sawError = false;

    if (argc < 2) {
        fprintf(stderr,
                "Usage: firmware -o <outfile> -b <address_base> -n <major.minor> -k <aeskey> -s <service_uuid> <infile>.hex ...\n");
        exit(1);
    }
    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();
    OPENSSL_config(NULL);

    assembledBlocks = vec_new();
    memblocks = vec_new();
    offset = 0x1000;
    while (argv[1][0] == '-') {
        switch (argv[1][1]) {

            case 'O':
            case 'o':
                arg = argv[1] + 2;
                if (*arg == 0) {
                    if (argc < 1) {
                        fprintf(stderr, "missing arg to -O\n");
                        sawError = true;
                        continue;
                    }
                    argv++;
                    argc--;
                    arg = argv[1];
                }
                outfile = arg;
                break;

            case 'b':
            case 'B':
                arg = argv[1] + 2;
                if (*arg == 0) {
                    if (argc < 1) {
                        fprintf(stderr, "missing address arg to -B\n");
                        sawError = true;
                        continue;
                    }
                    argv++;
                    argc--;
                    arg = argv[1];
                }
                baseAddress = strtol(arg, NULL, 0);
                if (baseAddress == 0) {
                    fprintf(stderr, "Base address evaluates to zero\n");
                    sawError = true;
                }
                break;

            case 'k':
            case 'K':
                arg = argv[1] + 2;
                if (*arg == 0) {
                    if (argc < 1) {
                        fprintf(stderr, "missing arg to -K\n");
                        sawError = true;
                        continue;
                    }
                    argv++;
                    argc--;
                    arg = argv[1];
                }
                if (strlen(arg) != KEY_LEN*2 && file_exists(arg))
                    arg = readKey(arg);
                if (strlen(arg) != KEY_LEN*2) {
                    fprintf(stderr, "The key should be %d bytes in hex\n", KEY_LEN);
                    sawError = true;
                } else
                    for (int i = 0; i != 32; i++)
                        keybuf[i] = getx(&arg);
                break;

            case 's':
            case 'S':
                arg = argv[1] + 2;
                if (*arg == 0) {
                    if (argc < 1) {
                        fprintf(stderr, "missing uuid arg to -S\n");
                        sawError = true;
                        continue;
                    }
                    argv++;
                    argc--;
                    arg = argv[1];
                }
                uuid_parse(arg, uuid);
                break;

            case 'n':
            case 'N':
                arg = argv[1] + 2;
                if (*arg == 0) {
                    if (argc < 1) {
                        fprintf(stderr, "missing version number arg to -N\n");
                        sawError = true;
                        continue;
                    }
                    argv++;
                    argc--;
                    arg = argv[1];
                }
                sscanf(arg, "%d.%d", &major, &minor);
                break;

            case 'v':
            case 'V':
                verbose = 1;
                break;
            default:
                fprintf(stderr, "Unknown arg %s\n", argv[1]);
                sawError = true;
                break;

        }
        argv++;
        argc--;
    }
    argc--;
    argv++;
    if (sawError)
        exit(1);
    while (*argv) {
        if (!freopen(argv[0], "r", stdin)) {
            fprintf(stderr, "Can't open %s\n", *argv);
            exit(1);
        }
        base = 0;
        lineno = 0;
        filename = *argv;
        while (readHexLine())
            continue;
        argv++;
    }
    assembleBlocks();
    if (verbose != 0)
        VEC_ITERATE(assembledBlocks, m1, memblock *)fprintf(stderr, "Block: %lX len %X\n", m1->addr, m1->length);
    memblock *mp = vec_first(assembledBlocks);
    if (baseAddress != 0 && mp->addr != baseAddress) {
        fprintf(stderr, "Lowest address %lX does not match specified base address of %lX\n", mp->addr, baseAddress);
        exit(1);
    }
    if (outfile == NULL) {
        fprintf(stderr, "No output file specified, skipping write\n");
        exit(0);
    }
    // create outfile if not specified as "-"
    if (strcmp(outfile, "-") != 0 && !freopen(outfile, "wb", stdout)) {
        fprintf(stderr, "Can't create output file %s\n", outfile);
        exit(1);
    }
    writeData();
    exit(0);
}


