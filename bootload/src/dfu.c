//
// Created by Clyde Stubbs on 15/3/17.
//

#include <dfu.h>
#include <io.h>
#include <flash.h>
#include <em_crypto.h>
#include <native_gecko.h>
#include <gatt_db.h>

#define PROG_INCREMENT  25
#define DFU_RESYNC 1
#define DIGEST_FAILED 2

static uint32 dataAddress, digestSize, digestAddress, baseAddress, dataCount;
static uint32 ivLen, digestLen;
static uint8_t iv[IV_LEN];
static uint8_t digest[DIGEST_LEN];
static uint8_t calcDigest[DIGEST_LEN];

bool doReset;

static uint8_t dataBuffer[FLASH_PAGE_SIZE];     // holds data for decryption
static uint32_t bufferBase;                     // address corresponding to base of buffer
static uint32_t bufferStart;                    // start of encrypted data in buffer
static uint32_t bufferEnd;                      // length of encrypted data in buffer
static uint32_t startTime;
static uint32_t bytesRead;
static uint8_t progressBuf[5];
static bool digestFailed;

// get a 16 bit word

static uint32 getWord16(uint8 *ptr) {
    return *ptr + (ptr[1] << 8);
}

// get a 32 bit word

static uint32 getWord32(uint8 *ptr) {
    return *ptr + (ptr[1] << 8) + (ptr[2] << 16) + (ptr[3] << 24);
}

// put a 32 bit word

static uint32 putWord32(uint8 *ptr, uint32_t val) {
    ptr[0] = (uint8) val;
    ptr[1] = (uint8) (val >> 8);
    ptr[2] = (uint8) (val >> 16);
    ptr[3] = (uint8) (val >> 24);
}

static void decode() {
    if (bufferEnd != bufferStart) {
        uint8_t newIv[IV_LEN];
        // save the last block of ciphertext as the new IV
        memcpy(newIv, dataBuffer + bufferEnd - IV_LEN, IV_LEN);
        uint8_t *bp = dataBuffer + bufferStart;
        CRYPTO_AES_CBC256(CRYPTO, bp, bp, bufferEnd - bufferStart, deKey, iv, false);
        memcpy(iv, newIv, IV_LEN);
        printf("Flashing block at %X\n", bufferBase);
        FLASH_eraseOneBlock(bufferBase);
        FLASH_writeBlock((void *) bufferBase, FLASH_PAGE_SIZE, dataBuffer);
    }
}


// get time since boot in ms
static uint32_t getTime() {
    struct gecko_msg_hardware_get_time_rsp_t *tp = gecko_cmd_hardware_get_time();
    return tp->seconds * 1000 + tp->ticks / 328;
}

/**
 * Set the address of the buffer base. Copy existing data if required.
 * @param address   The next address to write to
 */
static void setAddress(uint32_t address) {
    dataAddress = address;
    //printf("Set address to %X\n", address);
    uint32_t base = address & ~(FLASH_PAGE_SIZE - 1);     // get start of block
    if (bufferBase != base) {
        decode();
        bufferBase = base;
        // prefill the buffer with whatever data is already there, in case we want to write a partial block
        memcpy(dataBuffer, (const void *) bufferBase, FLASH_PAGE_SIZE);
        bufferStart = address - base;
        bufferEnd = bufferStart;
    }
}

void dumphex(const uint8_t *buf, unsigned len) {
    while (len-- != 0)
        printf("%02X ", *buf++);
}

bool checkDigest() {
    CRYPTO_SHA_256(CRYPTO, (const uint8_t *) digestAddress, digestSize, calcDigest);
    if (memcmp(digest, calcDigest, DIGEST_LEN) != 0) {
        digestFailed = true;
#if defined(DEBUG)
        printf("Digest failed: expected:\n");
        dumphex(digest, DIGEST_LEN);
        printf("\nActually got:\n");
        dumphex(calcDigest, DIGEST_LEN);
#endif
        progressBuf[0] = DIGEST_FAILED;
        gecko_cmd_gatt_server_send_characteristic_notification(currentConnection, GATTDB_ota_progress,
                                                               1, progressBuf);
        return false;
    }
    digestFailed = false;
    return true;
}

// copy a block of data into the buffer. Side effects include writing it to memory.

static void copydata(uint8_t *packet, uint32_t len) {
    uint32_t offs = dataAddress - bufferBase;
    memcpy(dataBuffer + offs, packet, len);
    dataAddress += len;
    bufferEnd = offs + len;
    //printf("Length remaining %d\n", count);
    if (dataAddress == baseAddress + dataCount) {
        uint32_t duration = getTime() - startTime;
        printf("Transferred %u bytes in %d.%1d seconds at %d/sec\n", bytesRead, duration / 1000, (duration % 1000) / 10,
               bytesRead * 1000 / duration);
        decode();
        dataCount = 0;
    } else
        setAddress(dataAddress);
}

// process a data packet.
bool processDataPacket(uint8 *packet, uint8 len) {
    if (len != 68)
        printf("Data packet len %d\n", len);
    if (ivLen != 0) {
        if (len == ivLen) {
            memcpy(iv, packet, len);
            ivLen = 0;
            return true;
        }
        printf("Bad IV len %d\n", len);
        ivLen = 0;
        return false;
    }

    if (digestLen != 0) {
        if (len == digestLen) {
            memcpy(digest, packet, len);
            digestLen = 0;
            if (checkDigest())
                return true;
            return false;
        }
        printf("Bad digest len %d\n", len);
        digestLen = 0;
        return false;
    }

    uint32_t baddr = getWord32(packet);
    if (baddr != dataAddress) {
        printf("packet address %X != expected %X\n", baddr, dataAddress);
        if (baddr > dataAddress) {
            progressBuf[0] = DFU_RESYNC;
            putWord32(progressBuf + 1, dataAddress);
            gecko_cmd_gatt_server_send_characteristic_notification(currentConnection, GATTDB_ota_progress,
                                                                   sizeof(progressBuf), progressBuf);
            return true;
        }
        setAddress(baddr);
    }

    uint8_t dlen = (uint8_t) (len - 4);
    bytesRead += dlen;
    if (dlen + dataAddress <= baseAddress + dataCount) {
        // does the packet cross a page boundary?
        uint32_t offs = dataAddress - bufferBase;
        if (bufferEnd + dlen > FLASH_PAGE_SIZE) {
            uint32_t tlen = FLASH_PAGE_SIZE - bufferEnd;
            copydata(packet + 4, tlen);
            dlen -= tlen;
            packet += tlen;
        }
        copydata(packet + 4, dlen);
        return true;
    }
    return false;
}

// process a control packet. Return true if accepted
bool processCtrlPacket(uint8 *packet) {
    uint32 cmd = getWord16(packet + DFU_CTRL_PKT_CMD);
    uint32 len = getWord16(packet + DFU_CTRL_PKT_LEN);
    uint32 address = getWord32(packet + DFU_CTRL_PKT_ADR);

    printf("Cmd %X, len %d @ %X\n", cmd, len, address);
    switch (cmd) {
        case DFU_CMD_RESTART:
            dataCount = 0;
            bufferBase = 0;
            bufferEnd = 0;
            bufferStart = 0;
            printf("Restarted DFU\n");
            int i = gecko_cmd_le_gap_set_conn_parameters(MIN_CONN_INTERVAL, MAX_CONN_INTERVAL, LATENCY,
                                                         SUPERV_TIMEOUT)->result;
            if (i != 0)
                printf("set_conn_parameters failed: error %u\n", i);
            return true;

        case DFU_CMD_DATA:
            if (ivLen != 0 || dataCount != 0) {
                printf("DATA command before previous complete\n");
                return false;
            }
            if (address < (uint32) USER_BLAT) {
                printf("Invalid address - %X should be less than %X", address, USER_BLAT);
                return false;
            }
            dataCount = len;
            baseAddress = address;
            setAddress(address);
            startTime = getTime();
            bytesRead = 0;
            printf("DATA command: %d bytes at %X\n", len, address);
            return true;

        case DFU_CMD_IV:
            if (ivLen != 0 || dataCount != 0) {
                printf("IV command before previous complete\n");
                return false;
            }
            ivLen = len;
            printf("IV command: %d bytes\n", ivLen);
            return true;

        case DFU_CMD_RESET:
            doReset = true;
            return true;

        case DFU_CMD_DIGEST:
            if (digestLen != 0 || ivLen != 0 || dataCount != 0) {
                printf("DIGEST command before previous complete\n");
                return false;
            }
            if (address == 0 || len == 0) {
                printf("DIGEST command without data block\n");
                return false;
            }
            digestAddress = address;
            digestLen = DIGEST_LEN;
            digestSize = len;
            printf("Digest command: %d bytes\n", len);
            return true;

        case DFU_CMD_DONE:
            if (digestFailed || digestLen != 0 || ivLen != 0 || dataCount != 0)
                return false;
            if (USER_BLAT->type == APP_BOOT_ADDRESS_TYPE) {
                printf("Updating BLAT:");
                MSC->WRITECTRL |= MSC_WRITECTRL_WREN;
                FLASH_writeWord((uint32_t) &USER_BLAT->type, APP_APP_ADDRESS_TYPE);
                MSC->WRITECTRL &= ~MSC_WRITECTRL_WREN;
                printf(" Blat now %X\n", USER_BLAT->type);
            }
            return true;

        case DFU_CMD_PING:
            // checking if we are up to the same point as the master thinks we should be
            printf("Pinged at %d/%d\n", len, dataAddress - baseAddress);
            uint32_t t = getTime() - startTime;
            printf("time: %d.%02d, rate %d/sec\n", t / 1000, (t % 1000) / 10, (bytesRead * 1000) / t);
            return true;

        default:
            break;
    }
    return false;
}

