#include <stdint.h>
#include "aat_def.h"


//AAT
extern const void * const __vector_table[];
extern unsigned char CSTACK$$Limit;
extern void dfu_main(void);
extern void NMI_Handler(void);
extern void HardFault_Handler(void);

extern uint32_t __StackTop;
extern uint32_t __dfu_Vectors;
const aat_t __dfu_AAT
__attribute__ ((section (".AAT")))
=
{
    .topOfStack=(uint32_t*)&__StackTop,    //.topOfStack=(uint32_t*)&CSTACK$$Limit,    
    .resetVector=dfu_main,           // main program for the DFU
    .nmiHandler=NMI_Handler,             /*  2 - NMI */
    .hardFaultHandler=HardFault_Handler,       /*  3 - HardFault */
    .type=APP_ADDRESS_TABLE_TYPE,     //uint16_t type;
    .version=AAT_MAJOR_VERSION,          //uint16_t version;
    .vectorTable=(void*)&__dfu_Vectors,              //const HalVectorTableType *vectorTable;
    .aatSize=sizeof(aat_t),
    .simeeBottom=(void*)0xFFFFFFFF,
    .imageCrc=IMAGE_CRC_MAGIC,
    .timeStamp=IMAGE_TIMESTAMP_MAGIC
};
