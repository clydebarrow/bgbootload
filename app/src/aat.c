#include <stdint.h>
#include <blat.h>


//AAT
extern void NMI_Handler(void);

extern void HardFault_Handler(void);

extern void Reset_Handler(void);

extern uint32_t __StackTop;
extern uint32_t __Vectors;
const blat_t app_ATT
        __attribute__ ((section (".AAT")))
        =
        {
                .topOfStack=&__StackTop,    //.topOfStack=(uint32_t*)&CSTACK$$Limit,
                .resetVector=Reset_Handler,           // main program for the app
                .nmiHandler=NMI_Handler,             /*  2 - NMI */
                .hardFaultHandler=HardFault_Handler,       /*  3 - HardFault */
                .type=APP_BOOT_ADDRESS_TYPE,                       // type;    The bootloader will overwrite this once it's happy
                .vectorTable=(void *) &__Vectors,              //const HalVectorTableType *vectorTable;
                .aatSize=sizeof(blat_t),
        };
