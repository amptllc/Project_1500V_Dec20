#include "hal_main.h"
#include "main.h"

#include <string.h>

#define PAGEADDR 0x7C00
#define PAGELEN  85
#define BARRIER  0xAA

static SchedulerInterface __xdata *si = (SchedulerInterface __xdata *)EXCHANGE_BUFFER;

void clearThePage(BYTE page){ 
    while( FCTL & 0x80 ) asm("NOP;");
    INT_GLOBAL_ENABLE(FALSE);
    //asm( "C1: MOV A, FCTL;  JB ACC.7, C1;" );
    FADDRH = page; FWT = 0x21; FCTL |= 1; asm("NOP");
    //asm( "MOV FWT,  #21h; MOV FCTL, #01h; NOP; " );
    INT_GLOBAL_ENABLE(TRUE);
    //asm( "C2: MOV A, FCTL; NOP; JB ACC.7, C2;" );
    while( FCTL & 0x80 ) asm("NOP;");
}

BOOL refreshing = FALSE, firstTime = TRUE;
static UINT16 refreshAddr = 0;

BOOL initiateTransfer(BYTE *ptr, BYTE *addr, UINT16 len){
    if( si->transfer ) return FALSE;

    SET_WORD(dmaConfig[2].SRCADDRH,  dmaConfig[2].SRCADDRL,  ptr);     // Source: ptr
    SET_WORD(dmaConfig[2].LENH,      dmaConfig[2].LENL, len);          // Sets the maximum transfer count allowed (length byte + data)
    SET_WORD(DMA1CFGH, DMA1CFGL,  &dmaConfig[1]);                       
    
    FADDRH = (((UINT16)addr)>>9) & 0xFF;    FADDRL = (((UINT16)addr)>>1) & 0xFF;  // setup flash address
    FWT = 0x21;    // 2A 

    DMAIRQ &= 4;
    DMAIF &= ~IRQ_DONE;
    DMAARM = DMAARM_CHANNEL2;                                                    // arm dma channel
    HAL_INT_ENABLE(INUM_DMA, INT_ON);    
    
    si->transfer = TRUE;
     
    halFlashDmaTrigger(); 

    //while( transfer ) halWait( 1 );
    return TRUE;
}
#define RLEN 0x40
static BYTE buff[ RLEN ];
void refreshTheFlash(void){ 
    refreshAddr = 0; 
    memcpy( buff, (BYTE *)refreshAddr, RLEN );
    refreshing = initiateTransfer( (BYTE *)buff, (BYTE *)refreshAddr, RLEN );
}
void usb(char *ptr);
void _mymemcpy( BYTE *destination, BYTE *source, BYTE counter );
#pragma vector=DMA_VECTOR
__interrupt void dmaFlash_IRQ(void) {
    DMAIF &= ~IRQ_DONE;
    if( DMAIRQ & 0x4 ){
        si->transfer = FALSE;
        DMAIRQ &= ~4;
        
        HAL_INT_ENABLE(INUM_DMA, INT_OFF);  
        DMAARM = DMAARM_CHANNEL2;
        if( refreshing ){
            refreshAddr += RLEN; 
            refreshing  = FALSE; 
            if( refreshAddr < 0x8000 ){
                _mymemcpy( buff, (BYTE *)refreshAddr, RLEN );
                refreshing = initiateTransfer( buff, (BYTE *)refreshAddr, RLEN );
            }
        }
    }
}

