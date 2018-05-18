#include "ioCC2510.h"

#include "hal_main.h"
#include "main.h"

//#include <string.h>

#define PAGEADDR 0x400
#define PAGELEN  256

extern void mymemcpy( BYTE *destination, BYTE *source, BYTE counter);

extern DMA_DESC dmaConfig[3];   
BOOL initiateTransfer(BYTE *ptr, BYTE *addr, BYTE len){
    waitForClearThePage();
    FWT = 0x2A;
    SET_WORD(dmaConfig[2].SRCADDRH,  dmaConfig[2].SRCADDRL, (UINT16)ptr);
    SET_WORD(dmaConfig[2].LENH,      dmaConfig[2].LENL,     len );                // Sets the maximum transfer count allowed (length byte + data)
    FADDRH = (((UINT16)addr)>>9) & 0xFF;    FADDRL = (((UINT16)addr)>>1) & 0xFF;  // setup flash address
    DMAIRQ &= ~4;                                                                 // damn important
    DMAARM = DMAARM_CHANNEL2;                                                    // arm dma channel
    halFlashDmaTrigger(); 
    return TRUE;
}
