#include "hal_main.h"
#include "main.h"

#include <string.h>

#define PAGEADDR 0x7C00
#define PAGELEN  85
#define BARRIER  0xAA

static BYTE *lastUsed(void){
BYTE *addr = (BYTE *)PAGEADDR;
    if( addr[0]!=BARRIER || addr[1]!=BARRIER ) return NULL; else return addr;
}

BOOL readTheLatest(BYTE *ptr, UINT16 len ){ 
BYTE *addr = lastUsed(); 
    if( addr == NULL )  return FALSE;
    memcpy( ptr, addr, len ); 
    return TRUE;
}
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

BOOL transfer = FALSE, refreshing = FALSE, firstTime = TRUE;
static UINT16 refreshAddr = 0;
extern BOOL isRxOnly;

static BOOL initiateTransfer(BYTE *ptr, BYTE *addr, UINT16 len){
    if( transfer ) return FALSE;
    /*
    //if( firstTime ){
        firstTime = FALSE;
        dmaConfig[2].PRIORITY       = DMA_PRI_LOW;                     // DMA has priority over CPU - as recommended
        dmaConfig[2].M8             = DMA_M8_USE_8_BITS;               // Use 8 bits for transfer count
        dmaConfig[2].IRQMASK        = DMA_IRQMASK_ENABLE;              // DMA interrupt when done
        dmaConfig[2].TRIG           = DMA_TRIG_FLASH;                  // DMA triggers on flash
        dmaConfig[2].TMODE          = DMA_TMODE_SINGLE;                // Single transfer per trigger.
        dmaConfig[2].WORDSIZE       = DMA_WORDSIZE_BYTE;               // One byte is transferred each time.
        //SET_WORD(dmaConfig[2].SRCADDRH,  dmaConfig[2].SRCADDRL,  ptr);     // Source: ptr
        SET_WORD(dmaConfig[2].DESTADDRH, dmaConfig[2].DESTADDRL, 0xDFAF );   // Destination: FWDATA register
        dmaConfig[2].VLEN           = DMA_VLEN_USE_LEN;
        //SET_WORD(dmaConfig[2].LENH, dmaConfig[2].LENL, len);         // Sets the maximum transfer count allowed (length byte + data)
        dmaConfig[2].SRCINC         = DMA_SRCINC_1;                    // Data source address is incremented by 1 byte
        dmaConfig[2].DESTINC        = DMA_DESTINC_0;                   // Destination address is constant
        SET_WORD(DMA1CFGH, DMA1CFGL,  &dmaConfig[1]);                       
    
        //FADDRH = (((UINT16)addr)>>9) & 0xFF;    FADDRL = (((UINT16)addr)>>1) & 0xFF;  // setup flash address
    //}
    */
    //dmaRadioSetup( mode = RADIO_MODE_TX );  
    //SET_WORD(dmaConfig[2].SRCADDRH,  dmaConfig[2].SRCADDRL,  ptr);     // Source: ptr
    //SET_WORD(dmaConfig[2].LENH,      dmaConfig[2].LENL, len);          // Sets the maximum transfer count allowed (length byte + data)

    FADDRH = (((UINT16)addr)>>9) & 0xFF;    FADDRL = (((UINT16)addr)>>1) & 0xFF;  // setup flash address
    FWT = 0x21;    // 2A 

    DMAIRQ &= 4;
    DMAIF &= ~IRQ_DONE;
    DMAARM = DMAARM_CHANNEL2;                                                    // arm dma channel
    HAL_INT_ENABLE(INUM_DMA, INT_ON);    
    
    transfer = TRUE;
     
    halFlashDmaTrigger(); 

    //while( transfer ) halWait( 1 );
    return TRUE;
}
void writeTheLatest(BYTE *ptr, UINT16 len, BOOL needNewPage){
BYTE *addr = (BYTE *)PAGEADDR;
    if( transfer ) return;
    clearThePage( (PAGEADDR>>9)&0xFF ); 
    //halWait(10);
    initiateTransfer( ptr, addr, len );
}
#define RLEN 0x40
static BYTE buff[ RLEN ];
void refreshTheFlash(void){ 
    refreshAddr = 0; 
    memcpy( buff, (BYTE *)refreshAddr, RLEN );
    refreshing = initiateTransfer( (BYTE *)buff, (BYTE *)refreshAddr, RLEN );
}
extern BYTE *pagePtr;
extern UINT16 PAGESIZE;
void usb(char *ptr);
#pragma vector=DMA_VECTOR
__interrupt void dmaFlash_IRQ(void) {
    DMAIF &= ~IRQ_DONE;
    if( DMAIRQ & 0x4 ){
        transfer = FALSE;
        DMAIRQ &= ~4;
        
        HAL_INT_ENABLE(INUM_DMA, INT_OFF);  
        DMAARM = DMAARM_CHANNEL2;
        /*
        if( refreshing ){
            refreshAddr += RLEN; 
            refreshing  = FALSE; 
            if( refreshAddr < 0x8000 ){
                memcpy( buff, (BYTE *)refreshAddr, RLEN );
                refreshing = initiateTransfer( buff, (BYTE *)refreshAddr, RLEN );
            }
        }
        */
    }
}
