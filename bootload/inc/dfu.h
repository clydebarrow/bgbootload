//
// Created by Clyde Stubbs on 15/3/17.
//

#ifndef BGBOOTLOAD_DFU_H
#define BGBOOTLOAD_DFU_H

#include <stdbool.h>
#include <bg_types.h>
#include <aat_def.h>
#include <blat.h>
// definitions for the DFU protocol

// Control packets

#define DFU_CTRL_PKT_CMD    0       // offset of command word
#define DFU_CTRL_PKT_LEN    2       // offset of length word
#define DFU_CTRL_PKT_ADR    4       // offset of address doubleword
#define DFU_CTRL_PKT_SIZE   8       // total length of packet

// commands

#define DFU_CMD_RESTART     0x1     // reset DFU system - resets data counts etc.
#define DFU_CMD_DATA        0x2     // data coming on the data channel
#define DFU_CMD_IV          0x3     // Initialization vector coming on data channel
#define DFU_CMD_DONE        0x4     // Download done, send status
#define DFU_CMD_RESET       0x5     // reset device
#define DFU_CMD_DIGEST      0x6     // Digest coming
#define DFU_CMD_PING        0x7     // Check progress

#define IV_LEN              16      // length of initialization vector
#define KEY_LEN             (256/8) // length of key
#define DIGEST_LEN          (256/8) // length of SHA256 digest


#define MIN_CONN_INTERVAL    9   // 7.5ms
#define MAX_CONN_INTERVAL    9  // 7.5ms
#define LATENCY             40  // max number of connection attempts we can skip. This is set high
#define SUPERV_TIMEOUT      300 // 30s
#define MAX_MTU             72  // max mtu

extern blat_t __UserStart;
#define USER_BLAT    (&__UserStart)
extern bool processCtrlPacket(uint8 * packet);      // process a control packet. Return true if accepted
extern bool processDataPacket(uint8 * packet, uint8 len);    // process a data packet.
extern bool enterDfu;
extern bool doReset;
extern const unsigned char ota_key[KEY_LEN];
extern unsigned char deKey[KEY_LEN];
extern uint8 currentConnection;

#define DFU_ENTRY_VECTOR    7       // index into vector table for EnterDFU_Handler

#endif //BGBOOTLOAD_DFU_H
