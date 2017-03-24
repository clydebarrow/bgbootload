//
// Created by Clyde Stubbs on 15/3/17.
//

#include <dfu.h>
#include <io.h>

static uint32 address;
static uint32 count;
static uint32 total;
// get a 16 bit word

static uint32 getWord16(uint8 *ptr) {
    return *ptr + (ptr[1] << 8);
}

// get a 32 bit word

static uint32 getWord32(uint8 *ptr) {
    return *ptr + (ptr[1] << 8) + (ptr[2] << 16) + (ptr[3] << 24);
}

// process a control packet. Return true if accepted
bool processCtrlPacket(uint8 *packet) {
    uint32 cmd = getWord16(packet + DFU_CTRL_PKT_CMD);
    uint32 len = getWord16(packet + DFU_CTRL_PKT_LEN);
    address = getWord32(packet + DFU_CTRL_PKT_ADR);

    printf("Cmd %X, len %d @ %X", cmd, len, address);
    switch (cmd) {
        case DFU_CMD_RESTART:
            count = 0;
            total = 0;
            return true;

        case DFU_CMD_DATA:

            if (count != 0)
                return false;
            count = len;
            return true;

        case DFU_CMD_RESET:
            return false;

        default:
            return false;
    }

}

// process a data packet.
bool processDataPacket(uint8 *packet, uint8 len) {
    printf("Data packet len %d\n");
    if(count > len) {
        count -= len;
        return true;
    }
    return false;
}

