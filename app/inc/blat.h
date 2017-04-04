//
// Created by Clyde Stubbs on 4/4/17.
//

#ifndef BGBOOTLOAD_BLAT_H
#define BGBOOTLOAD_BLAT_H

// this structure is included at the beginning of each user application. It is similar in intent, but different
// in format, to the Silabs AAT.

#define APP_BOOT_ADDRESS_TYPE          (0xFFFFFFA7)     // this value set in the file
#define APP_APP_ADDRESS_TYPE          (0xF765FFA7)      // this value written after successful download

typedef struct {
    uint32_t *topOfStack;           // Stack initialization pointer
    void     (*resetVector)(void);  // Reset vector
    void     (*nmiHandler)(void);   // NMI Handler
    void     (*hardFaultHandler)(void); // Hardfault handler
    uint32_t type;                  // 0x0AA7 for Application Address Table, 0x0BA7 for Bootloader Address Table, 0x0EA7 for RAMEXE
    uint32_t *vectorTable;          // Pointer to the real Cortex-M vector table
    uint32_t timeStamp;             // Unix epoch time for EBL generation
    uint32_t appVersion;            // Customer defined version number.
    uint8_t  aatSize;               // Size of the AAT in bytes.
    uint8_t  unused[3];             // Reserved for bootloader expansion

} blat_t;
#endif //BGBOOTLOAD_BLAT_H
