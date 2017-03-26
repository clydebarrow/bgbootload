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
#include <em_rmu.h>

#define lockBits        ((uint32_t *)LOCKBITS_BASE)

#define DLW             lockBits[127]                   // debug lock word



// expected first 4 words of stack AAT

static const uint32_t stack_aat[] = {0x20007BF8, 0x00020A4B, 0x0000CE87, 0x0000F393};

bool enterDfu;        // set if we should enter dfu mode

static void hang() {
    for (;;);
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


    uint32_t cause = RMU_ResetCauseGet();
    RMU_ResetCauseClear();
    if ((cause & (RMU_RSTCAUSE_PORST | RMU_RSTCAUSE_SYSREQRST)) == RMU_RSTCAUSE_SYSREQRST)
        enterDfu = true;
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
