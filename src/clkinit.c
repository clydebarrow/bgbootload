//
// Created by Clyde Stubbs on 9/3/17.
//

#include <em_gpio.h>
#include "io.h"
#include "em_emu.h"
#include "em_cmu.h"


/**
 * USART pin definitions.
 *
 */
void EMU_init(void) {
    // $[EMU Initialization]
    EMU_DCDCInit_TypeDef dcdcInit = EMU_DCDCINIT_DEFAULT;

    dcdcInit.powerConfig = emuPowerConfig_DcdcToDvdd;
    dcdcInit.dcdcMode = emuDcdcMode_LowNoise;
    dcdcInit.mVout = 1800;
    dcdcInit.em01LoadCurrent_mA = 5;
    dcdcInit.em234LoadCurrent_uA = 10;
    dcdcInit.maxCurrent_mA = 160;
    dcdcInit.anaPeripheralPower = emuDcdcAnaPeripheralPower_AVDD;

    EMU_DCDCInit(&dcdcInit);
}

void CMU_init(void) {
    // $[High Frequency Clock Setup]
    /* Initializing HFXO */
    CMU_HFXOInit_TypeDef hfxoInit = CMU_HFXOINIT_DEFAULT;

    hfxoInit.autoStartEm01 = 1;
    // hfxoInit.ctuneSteadyState = 0x0;

    CMU_HFXOInit(&hfxoInit);

    /* Skipping HFXO oscillator enable, as it is auto-enabled on EM0/EM1 entry */

    /* Setting system HFXO frequency */
    SystemHFXOClockSet(38400000);

    /* Using HFXO as high frequency clock, HFCLK */
    CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO);

    /* HFRCO not needed when using HFXO */
    CMU_OscillatorEnable(cmuOsc_HFRCO, false, false);
    // [High Frequency Clock Setup]$

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

