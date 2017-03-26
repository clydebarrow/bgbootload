//
// Created by Clyde Stubbs on 9/3/17.
//

#include <em_gpio.h>
#include "io.h"
#include "em_emu.h"
#include "em_cmu.h"


void CMU_init(void) {
    // $[High Frequency Clock Setup]
    // skip this, will have been done in the BLE stack startup code
    /* Initializing HFXO */

    /* Setting system HFXO frequency */
    SystemHFXOClockSet(38400000);

    // $[LE clocks enable]
    /* Enable clock to LE modules */
    CMU_ClockEnable(cmuClock_CORELE, true);
    /* Initializing LFXO */
    CMU_LFXOInit_TypeDef lfxoInit = CMU_LFXOINIT_DEFAULT;

    CMU_LFXOInit(&lfxoInit);

    /* Enable LFXO oscillator, and wait for it to be stable */
    CMU_OscillatorEnable(cmuOsc_LFXO, true, true);

    /* Select LFXO as clock source for LFECLK */
    CMU_ClockSelectSet(cmuClock_LFE, cmuSelect_LFXO);
    // [LFECLK Setup]$

    // $[Peripheral Clock enables]
    /* Enable clock for GPCRC */
    CMU_ClockEnable(cmuClock_GPCRC, true);

    /* Enable clock for LDMA */
    CMU_ClockEnable(cmuClock_LDMA, true);

    /* Enable clock for RTCC */
    CMU_ClockEnable(cmuClock_RTCC, true);

    /* Enable clock for HFPER */
    CMU_ClockEnable(cmuClock_HFPER, true);

    /* Enable clock for GPIO by default */
    CMU_ClockEnable(cmuClock_GPIO, true);

    // [Peripheral Clock enables]$

}

