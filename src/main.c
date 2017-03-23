//
// Created by Clyde Stubbs on 17/3/17.
//

/*
 * Main program for the initial bootload code. This is run on reset. It notes the type of reset,
 * and sets a flag to determine whether to run the user program or enter dfu mode.
 * If necessary it will patch the Bluetooth stack to ensure that the correct AAT is found.
 * IT then runs the Bluetooth stack startup code,
 */

#include <dfu.h>
#include <aat_def.h>
#include <flash.h>
#include <em_device.h>
#include <em_msc.h>
#include <io.h>

#define SWRESET_MASK    0x3F
#define SWRESET_BITS    0x20

#define BGSTACK_BASE    (uint32_t *)0x4000)                 // AAT for Bluetooth stack
#define BGSTACK_PATCH   (*(uint32_t *)0x63d4)               // address to patch - offset into stack_aat
#define BGSTACK_PREV    0x00020D02                          // expected previous contents

#define lockBits        ((uint32_t *)LOCKBITS_BASE)

#define DLW             lockBits[127]                   // debug lock word



// expected first 4 words of stack AAT

static const uint32_t stack_aat[] = {0x20007BF8, 0x00020A4B, 0x0000CE87, 0x0000F393};

bool enterDfu;        // set if we should enter dfu mode

static void hang() {
    for (;;);
}

static void patch(uint32_t address, uint32_t value) {
    uint8_t buffer[FLASH_PAGE_SIZE];
    uint8_t *ptr = (uint8_t *) (address & ~(FLASH_PAGE_SIZE - 1));
    memcpy(buffer, ptr, FLASH_PAGE_SIZE);
    FLASH_eraseOneBlock((uint32_t) ptr);
    *((uint32_t *) (buffer + (address & (FLASH_PAGE_SIZE - 1)))) = value;
    FLASH_writeBlock(ptr, FLASH_PAGE_SIZE, buffer);
}

void main(void) {

    EMU_init();
    CMU_init();
    /* Unlock the MSC */
    MSC->LOCK = MSC_UNLOCK_CODE;
    /* Enable writing to the flash */

#if DEBUG == 0
    // check debug lock
    if(DLW != 0) {
        MSC->WRITECTRL |= MSC_WRITECTRL_WREN;
        FLASH_writeWord((uint32_t) &DLW, 0);
        MSC->WRITECTRL &= ~MSC_WRITECTRL_WREN;
        MSC->LOCK = 0;
    }
#endif

    if ((RMU->RSTCAUSE & SWRESET_MASK) == SWRESET_BITS)
        enterDfu = true;
    /* check for valid BT stack loaded */
    if (memcmp(stack_aat, &__stack_AAT, sizeof stack_aat) != 0)
        hang();

    // start up the bluetooth stack.
    //use vector table from aat
    SCB->VTOR=(uint32_t)__stack_AAT.vectorTable;
    //Use stack from aat
    asm("mov sp,%0" :: "r" (__stack_AAT.topOfStack));
    __stack_AAT.resetVector();
}
