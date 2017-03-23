/*
 * bg_dfu.h
 *
 *  Created on: 2015-11-23
 *      Author: jaknaapp
 */

#ifndef COMMON_INCLUDE_BG_DFU_H_
#define COMMON_INCLUDE_BG_DFU_H_
#include <stdint.h>

typedef struct 
{ 
    uint32_t code_crc;
    uint32_t header_crc;
    uint32_t length;
    uint32_t type;
}bg_dfu_header_t;

/*GNU linker does not calculate CRC. Use known random numbers when linking with GNU LD*/
#define DEVELOPMENT_CODE_CRC         (0x4fd105c3)
#define DEVELOPMENT_HEADER_CRC         (0xd121cee3)

#define BG_DFU_TYPE_RESET_NEEDED (0x1)
#define BG_DFU_TYPE_DEVELOPMENT  (0x80)
#define BG_DFU_TYPE_STACK        ((0<<1)|BG_DFU_TYPE_RESET_NEEDED)
#define BG_DFU_TYPE_APP          (1<<1)
#define BG_DFU_TYPE_GATT         (3<<1)
#define BG_DFU_TYPE_PS_SET       (4<<1)
#define BG_DFU_TYPE_PS_REMOVE    (5<<1)
#define BG_DFU_TYPE_END          ((6<<1)|BG_DFU_TYPE_RESET_NEEDED)

#endif /* COMMON_INCLUDE_BG_DFU_H_ */
