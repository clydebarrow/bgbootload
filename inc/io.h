//
// Created by Clyde Stubbs on 8/3/17.
//

#ifndef BGM111_IO_H
#define BGM111_IO_H

#endif //BGM111_IO_H


extern void EMU_init(void);
extern void CMU_init(void);

#ifdef DEBUG
#include <SEGGER_RTT.h>
#define printf(...) SEGGER_RTT_printf(0, __VA_ARGS__)
#else
#define printf(...) (0)
#endif

