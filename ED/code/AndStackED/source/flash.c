#include "ioCC2510.h"

#include "hal_main.h"
#include "main.h"

//#include <string.h>

#define PAGEADDR 0x7C00
#define PAGELEN  256
#define BARRIER  0xAA

extern void mymemcpy( BYTE *destination, BYTE *source, BYTE counter);
/*
BOOL readTheLatest(BYTE *ptr, BYTE len ){ 
BYTE *addr = (BYTE *)PAGEADDR; // NO Round Buffer
    if( addr[0]!=BARRIER || addr[1]!=BARRIER ) return FALSE; 
    mymemcpy( ptr, addr, len ); 
    return TRUE;
}
*/
/*
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
*/

BOOL refreshing = FALSE, firstTime = TRUE, transfer = FALSE;
static UINT16 refreshAddr = 0;
extern DMA_DESC dmaConfig[3];   
/*
BOOL initiateTransfer(BYTE *ptr, BYTE *addr, BYTE len){
    if( transfer ) return FALSE;
    
    SET_WORD(dmaConfig[2].SRCADDRH,  dmaConfig[2].SRCADDRL,  ptr);     // Source: ptr
    //SET_WORD(dmaConfig[2].LENH,      dmaConfig[2].LENL, len);          // Sets the maximum transfer count allowed (length byte + data)
    if( len & 1 ) len++;
    dmaConfig[2].LENL = len;
    
    //SET_WORD(DMA1CFGH, DMA1CFGL,     &dmaConfig[1]);                       
    FADDRH = (((UINT16)addr)>>9) & 0xFF;    FADDRL = (((UINT16)addr)>>1) & 0xFF;  // setup flash address

    / *
    FWT = 0x21;    // 2A 
    DMAIRQ &= 4;
    DMAARM = DMAARM_CHANNEL2;                                                    // arm dma channel
    * /
    DMAIRQ &= ~4;
    flashDmaSetup();
    HAL_INT_ENABLE(INUM_DMA, INT_ON);    
    transfer = TRUE;
     
    halFlashDmaTrigger(); 

    //while( transfer ) halWait( 1 );
    return TRUE;
}
*/
BOOL initiateTransfer(BYTE *ptr, BYTE *addr, BYTE len){
    //if( transfer ) return FALSE;
    waitForClearThePage();
    //P1_4 = 0; // debug statement for measuring save PB time
    FWT = 0x2A;
    SET_WORD(dmaConfig[2].SRCADDRH,  dmaConfig[2].SRCADDRL, (UINT16)ptr);
    SET_WORD(dmaConfig[2].LENH,      dmaConfig[2].LENL,     len );                // Sets the maximum transfer count allowed (length byte + data)
    FADDRH = (((UINT16)addr)>>9) & 0xFF;    FADDRL = (((UINT16)addr)>>1) & 0xFF;  // setup flash address
    DMAIRQ &= ~4;                                                                 // damn important
    HAL_INT_ENABLE(INUM_DMA, INT_ON);
    //HAL_INT_ENABLE(INUM_RF,  INT_OFF);
    DMAARM = DMAARM_CHANNEL2;                                                    // arm dma channel
    //transfer = TRUE;
    halFlashDmaTrigger(); 
    return TRUE;
}
/*
void writeTheLatest(BYTE *ptr, BYTE len){
    clearThePage( (PAGEADDR>>9)&0xFF ); 
    //halWait( 40 );
    initiateTransfer( ptr, (BYTE *)PAGEADDR, len );
}
*/
#define RLEN 0x80

BYTE buff[ RLEN ];
/* comment it to avoid flash refresh */
void refreshTheFlash(void){ 
    refreshAddr = 0; 
    mymemcpy( buff, (BYTE *)refreshAddr, RLEN );
    refreshing = initiateTransfer( (BYTE *)buff, (BYTE *)refreshAddr, RLEN );
}
/* till here */
void flashCallback(void){
    HAL_INT_ENABLE(INUM_DMA, INT_OFF);
    //transfer = FALSE;   
    /* comment it to avoid flash refresh */
    if( refreshing ){
        refreshAddr += RLEN; 
        refreshing  = FALSE; 
        if( refreshAddr < 0x8000 ){
            mymemcpy( buff, (BYTE *)refreshAddr, RLEN & 0xFF );
            refreshing = initiateTransfer( buff, (BYTE *)refreshAddr, RLEN & 0xFF );
        }
    }
    /* till here */
    //if( !transfer ) ((BOOL*)0xF520)[Kick] = TRUE;
    //HAL_INT_ENABLE(INUM_RF,  INT_ON);
}
