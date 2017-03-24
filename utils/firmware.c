#include	<stdio.h>
#include	<ctype.h>
#include	<stdarg.h>
#include	<stdlib.h>

/**
 * This is the structure of the firmware file. There is a fixed size header followed by one or more block headers,
 * which point to the data in the rest of the file.
 * All data in the headers is little endian
 */

#define	FW_TAG		0x55A322BF		// magic number to identify the file

typedef struct
{
	unsigned char	tag[4];			// magic number goes here
	unsigned char	major[2];		// major version number
	unsigned char	minor[2];		// minor version number
	unsigned char	numblocks[2];		// the number of blocks in the file,
    unsigned char	service_uuid[16];	// the service uuid of the bootloader
	unsigned char	init_vector[16];	// the block 0 initialization vector
}	firmware;


unsigned long	base, last;
unsigned long	lineno, version ,product;
char *		filename;
char		verbose;
char		lbuf[1024];
unsigned long	offset;


static void
put4(unsigned char * addr, unsigned long val)
{
	*addr++ = val & 0xFF;
	*addr++ = (val >> 8) & 0xFF;
	*addr++ = (val >> 16) & 0xFF;
	*addr++ = (val >> 24) & 0xFF;
}

void
error(char * f, ...)
{
	va_list	ap;

	va_start(ap, f);
	fprintf(stderr, "%s: %ld: ", filename, lineno);
	vfprintf(stderr, f, ap);
	putc('\n', stderr);
	if(verbose)
		fprintf(stderr,  "line: \"%s", lbuf);
}


unsigned int
getx(char ** p)
{
	int	hi, lo;

	hi = *(*p)++;
	lo = *(*p)++;
	if(!isxdigit(hi) || !isxdigit(lo)) {
		error("Saw 0%o and 0%o:- hex digit expected", hi, lo);
		exit(1);
	}
	if(islower(hi))
		hi = toupper(hi);
	if(islower(lo))
		lo = toupper(lo);
	if(hi >= 'A')
		hi -= 'A' - ('9'+1);
	if(lo >= 'A')
		lo -= 'A' - ('9'+1);
	hi -= '0';
	lo -= '0';
	return (hi << 4) + lo;
}

int
getline()
{
	int	i, j, cksum, cnt, type;
	unsigned	addr;
	unsigned char	buffer[256];
	char *		p;

	if(fgets(lbuf, sizeof lbuf, stdin) == NULL)
		return 0;
	lineno++;
	p = lbuf;
	while((i = *p++) != ':')
		if(i == 0)
			return 1;
	cnt = getx(&p);
	cksum = getx(&p);
	addr = getx(&p);
	addr += (cksum << 8);
	/* addr -= 0x100;*/
	cksum += addr + cnt;
	cksum += type = getx(&p);
	for(j = 0 ; j != cnt ; j++) {
		i = getx(&p);
		buffer[j] = i;
		cksum += i;
	}
	cksum += getx(&p);
	if((cksum & 0xFF) != 0) {
		error( "Checksum error");
		exit(1);
	}
	switch(type) {
		case 1:	/* EOF */
			return 0;

		case 0:	/* data */
			if(fseek(stdout, base+addr-offset+sizeof( firmware), 0) == EOF) {
				error( "Seek error on output file - offset %ld", base+addr-offset+sizeof (firmware));
				exit(1);
			}
			if(fwrite(buffer, 1, cnt, stdout) != cnt) {
				error( "Write error on output file - offset %lX", base+addr);
				exit(1);
			}
			if(last < base+addr+cnt)
				last = base+addr+cnt;
			break;

		case 4:	/* extended linear address */
			base = ((unsigned long)buffer[1] << 16) + ((unsigned long)buffer[0] << 24);
			if(verbose)
				fprintf(stderr, "%ld: Linear address %lX\n", lineno, base);
			break;

		case 2:	/* extended segment address */
			base = ((unsigned long)buffer[1] << 4) + ((unsigned long)buffer[0] << 12);
			if(verbose)
				fprintf(stderr, "%ld: Segment address %lX\n", lineno, base);
			break;

		case 3:
		case 5:
			break;

		default:
			error( "Unknown record type %2.2X - length %d", type, cnt);
			return 0;

	}
	return 1;
}
int
main(argc, argv)
char **	argv;
{
	firmware	fw;

	if(argc < 2) {
		fprintf(stderr, "Too few arguments\n");
		exit(1);
	}
	offset = 0x1000;
	while(argv[1][0] == '-') {
		switch(argv[1][1]) {

		case 'O':
		case 'o':
			if(!freopen(&argv[1][2], "wb", stdout)) {
				fprintf(stderr, "Can't create %s\n", &argv[1][2]);
				exit(1);
			}
			break;
		case 'c':
		case 'C':
			offset = strtoul(argv[1]+2, NULL, 16);
			break;

		case 'v':
		case 'V':
			version = strtoul(argv[1]+2, NULL, 16);
			break;

		case 'p':
		case 'P':
			product = strtoul(argv[1]+2, NULL, 16);
			break;

		case 'd':
		case 'D':
			verbose = 1;
			break;
		}
		argv++;
		argc--;
	}
	argc--;
	argv++;
	while(*argv) {
		if(!freopen(argv[0], "r", stdin)) {
			fprintf(stderr, "Can't open %s\n", *argv);
			exit(1);
		}
		base = 0;
		lineno = 0;
		filename = *argv;
		while(getline())
			continue;
		argv++;
	}
	put4(fw.tag, FW_TAG);
	put4(fw.product, product);
	put4(fw.version, version);
	put4(fw.addr, offset);
	put4(fw.size, last-offset);
	fseek(stdout, 0L, SEEK_SET);
	fwrite(&fw, sizeof fw, 1, stdout);
	fclose(stdout);
	exit(0);
}


