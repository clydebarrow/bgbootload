//
// Created by Clyde Stubbs on 15/3/17.
//

#ifndef BGBOOTLOAD_DFU_H
#define BGBOOTLOAD_DFU_H

#include <stdbool.h>
#include "bg_types.h"
// definitions for the DFU protocol

// Control packets

#define DFU_CTRL_PKT_CMD    0       // offset of command word
#define DFU_CTRL_PKT_LEN    2       // offset of length word
#define DFU_CTRL_PKT_ADR    4       // offset of address doubleword
#define DFU_CTRL_PKT_SIZE   8       // total length of packet

// commands

#define DFU_CMD_RESTART     0x1     // reset DFU system - resets data counts etc.
#define DFU_CMD_DATA        0x2     // data coming on the data channel
#define DFU_CMD_CRC         0x3     // total count and CRC of data sent
#define DFU_CMD_RESET       0x4     // reset device

extern bool processCtrlPacket(uint8 * packet);      // process a control packet. Return true if accepted
extern bool processDataPacket(uint8 * packet, uint8 len);    // process a data packet.
extern bool enterDfu;

#endif //BGBOOTLOAD_DFU_H
