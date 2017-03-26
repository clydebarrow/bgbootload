#include <stdint.h>
#include "aat_def.h"


//AAT
extern void NMI_Handler(void);
extern void HardFault_Handler(void);
extern void Reset_Handler(void);

extern uint32_t __StackTop;
extern uint32_t __Vectors;
const aat_t app_ATT
__attribute__ ((section (".AAT")))
=
{
    .topOfStack=&__StackTop,    //.topOfStack=(uint32_t*)&CSTACK$$Limit,
    .resetVector=Reset_Handler,           // main program for the app
    .nmiHandler=NMI_Handler,             /*  2 - NMI */
    .hardFaultHandler=HardFault_Handler,       /*  3 - HardFault */
    .type=APP_ADDRESS_TABLE_TYPE,     //uint16_t type;
    .version=AAT_MAJOR_VERSION,          //uint16_t version;
    .vectorTable=(void*)&__Vectors,              //const HalVectorTableType *vectorTable;
    .aatSize=sizeof(AppAddressTable),    
    .simeeBottom=(void*)0xFFFFFFFF,
    .imageCrc=IMAGE_CRC_MAGIC,
    .timeStamp=IMAGE_TIMESTAMP_MAGIC
};
