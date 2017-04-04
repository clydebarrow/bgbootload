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
#include <em_device.h>
#include <io.h>
#include <em_rmu.h>
#include <flash.h>

#define lockBits        ((uint32_t *)LOCKBITS_BASE)

#define DLW             lockBits[127]                   // debug lock word



// expected first 4 words of stack AAT

static const uint32_t stack_aat[] = {0x20007BF8, 0x00020A4B, 0x0000CE87, 0x0000F393};

// random bytes

static const uint8_t dfu_key[] = {0xCE, 0x8D, 0xC1, 0x1F, 0x37, 0x7B, 0xB1, 0x9A,
                            0x79, 0xF5, 0xE1, 0x44, 0x8C, 0xC9, 0xAD, 0x57};
// copy of random bytes
static uint8_t dfu_key_copy[sizeof(dfu_key)] __attribute__ ((section (".dfu_key")));
bool enterDfu;        // set if we should enter dfu mode


static void hang() {
    for (;;);
}

void main(void) {

    EMU_init();
    CMU_init();
    /* Enable writing to the flash */

#if !defined(DEBUG)
    // check debug lock
    if(DLW != 0) {
    /* Unlock the MSC */
        MSC->LOCK = MSC_UNLOCK_CODE;
        MSC->WRITECTRL |= MSC_WRITECTRL_WREN;
        FLASH_writeWord((uint32_t) &DLW, 0);
        MSC->WRITECTRL &= ~MSC_WRITECTRL_WREN;
        MSC->LOCK = 0;
    }
#endif

    // for debug purposes, never run the user program

#if defined(DFU_DEBUG)
    enterDfu = true;
#endif
    uint32_t cause = RMU_ResetCauseGet();
    RMU_ResetCauseClear();
    // enter DFU if we got here from anything other than a power on reset, and our key
    // has been copied to RAM
    if (!(cause & RMU_RSTCAUSE_PORST) && memcmp(dfu_key, dfu_key_copy, sizeof(dfu_key)) == 0)
        enterDfu = true;
    memset(dfu_key_copy, 1, sizeof(dfu_key_copy));
    /* check for valid BT stack loaded */
    if (memcmp(stack_aat, &__stack_AAT, sizeof stack_aat) != 0)
        hang();

    // start up the bluetooth stack.
    //use vector table from aat
    SCB->VTOR = (uint32_t) __stack_AAT.vectorTable;
    //Use stack from aat
    asm("mov sp,%0"::"r" (__stack_AAT.topOfStack));
    __stack_AAT.resetVector();
}

/**
 * This function is called from the application program to flag that we should enter DFU mode
 * after the next non-power on reset. It is accessed through
 * an entry in the vector table, indexed at ENTER_DFU_VECTOR.
 */
void EnterDFU_Handler(void) {
    memcpy(dfu_key_copy, dfu_key, sizeof(dfu_key_copy));
}
