/*==== DECLARATION CONTROL ===================================================*/
/*==== INCLUDES ==============================================================*/

#include "hal_main.h"
#include "dma.h"
#include "main.h"

/*==== GLOBAL VARS============================================================*/

BYTE radioPktBuffer  [4][PACKET_LENGTH + 3];
BYTE radioPktBufferRx[PACKET_LENGTH + 3]; // to accomodate maximum possible package  PACKET_LENGTH_ED + 3];
BYTE radioPktBufferTx[PACKET_LENGTH + 3];

DMA_DESC dmaConfig[3];                  // Struct for the DMA configuration
#define PAGESIZE 100
#define pagePtr  ((BYTE *)0)
extern BOOL transfer; 
/*==== PUBLIC FUNCTIONS ======================================================*/

/******************************************************************************
* @fn  dmaRadioSetup
*
* @brief
*      This funtion configures the static dmaConfig struct with the correct
*      values for receiving or transmitting data using the DMA.
*
* Parameters:
*
* @param  BYTE mode
*           Either RADIO_MODE_TX or RADIO_MODE_RX to select transmitter or
*           receiver settings
*
* @return void
*
******************************************************************************/
void dmaSetup( void ){
    // Some configuration that are common for both TX and RX:
    // CPU has priority over DMA
    // Use 8 bits for transfer count
    // No DMA interrupt when done
    // DMA triggers on radio
    // Single transfer per trigger.
    // One byte is transferred each time.
    dmaConfig[1].PRIORITY       = DMA_PRI_LOW;
    dmaConfig[1].M8             = DMA_M8_USE_8_BITS;
    dmaConfig[1].IRQMASK        = DMA_IRQMASK_DISABLE;
    dmaConfig[1].TRIG           = DMA_TRIG_RADIO;
    dmaConfig[1].TMODE          = DMA_TMODE_SINGLE;
    dmaConfig[1].WORDSIZE       = DMA_WORDSIZE_BYTE;

    // Transmitter specific DMA settings

    // Source: radioPktBuffer
    // Destination: RFD register
    // Use the first byte read + 1
    // Sets the maximum transfer count allowed (length byte + data)
    // Data source address is incremented by 1 byte
    // Destination address is constant
    SET_WORD(dmaConfig[1].SRCADDRH,  dmaConfig[1].SRCADDRL, radioPktBufferTx);
    SET_WORD(dmaConfig[1].DESTADDRH, dmaConfig[1].DESTADDRL, &X_RFD);
    dmaConfig[1].VLEN           = DMA_VLEN_FIRST_BYTE_P_1; // DMA_VLEN_USE_LEN; 
    SET_WORD(dmaConfig[1].LENH, dmaConfig[1].LENL, PACKET_LENGTH + 5);
    dmaConfig[1].SRCINC         = DMA_SRCINC_1;
    dmaConfig[1].DESTINC        = DMA_DESTINC_0;
    
    dmaConfig[2].PRIORITY       = DMA_PRI_LOW;                     // DMA has priority over CPU - as recommended
    dmaConfig[2].M8             = DMA_M8_USE_8_BITS;               // Use 8 bits for transfer count
    dmaConfig[2].IRQMASK        = DMA_IRQMASK_ENABLE;              // DMA interrupt when done
    dmaConfig[2].TRIG           = DMA_TRIG_FLASH;                  // DMA triggers on flash
    dmaConfig[2].TMODE          = DMA_TMODE_SINGLE;                // Single transfer per trigger.
    dmaConfig[2].WORDSIZE       = DMA_WORDSIZE_BYTE;               // One byte is transferred each time.
    SET_WORD(dmaConfig[2].DESTADDRH, dmaConfig[2].DESTADDRL, 0xDFAF );   // Destination: FWDATA register
    dmaConfig[2].VLEN           = DMA_VLEN_USE_LEN;
    //SET_WORD(dmaConfig[2].LENH, dmaConfig[2].LENL, PAGESIZE);      // Sets the maximum transfer count allowed (length byte + data)
    dmaConfig[2].SRCINC         = DMA_SRCINC_1;                    // Data source address is incremented by 1 byte
    dmaConfig[2].DESTINC        = DMA_DESTINC_0;                   // Destination address is constant

    SET_WORD(dmaConfig[2].SRCADDRH,  dmaConfig[2].SRCADDRL,  pagePtr);     // Source: ptr
    //if( PAGESIZE & 1 ) PAGESIZE++;
    SET_WORD(dmaConfig[2].LENH,      dmaConfig[2].LENL, PAGESIZE);         // Sets the maximum transfer count allowed (length byte + data)
    // Using DMA 1
    SET_WORD(DMA1CFGH, DMA1CFGL, &dmaConfig[1]);

    // Some configuration that are common for both TX and RX:
    // CPU has priority over DMA
    // Use 8 bits for transfer count
    // No DMA interrupt when done
    // DMA triggers on radio
    // Single transfer per trigger.
    // One byte is transferred each time.
    dmaConfig[0].PRIORITY       = DMA_PRI_LOW;
    dmaConfig[0].M8             = DMA_M8_USE_8_BITS;
    dmaConfig[0].IRQMASK        = DMA_IRQMASK_DISABLE;
    dmaConfig[0].TRIG           = DMA_TRIG_RADIO;
    dmaConfig[0].TMODE          = DMA_TMODE_SINGLE;
    dmaConfig[0].WORDSIZE       = DMA_WORDSIZE_BYTE;
    // Receiver specific DMA settings:

    // Source: RFD register
    // Destination: radioPktBuffer
    // Use the first byte read + 3 (incl. 2 status bytes)
    // Sets maximum transfer count allowed (length byte + data + 2 status bytes)
    // Data source address is constant
    // Destination address is incremented by 1 byte for each write
    SET_WORD(dmaConfig[0].SRCADDRH, dmaConfig[0].SRCADDRL, &X_RFD);
    SET_WORD(dmaConfig[0].DESTADDRH, dmaConfig[0].DESTADDRL, radioPktBufferRx);
    dmaConfig[0].VLEN           = DMA_VLEN_FIRST_BYTE_P_3; // DMA_VLEN_USE_LEN; //DMA_VLEN_FIRST_BYTE_P_3;
    SET_WORD(dmaConfig[0].LENH, dmaConfig[0].LENL, PACKET_LENGTH + 5);
    dmaConfig[0].SRCINC         = DMA_SRCINC_0;
    dmaConfig[0].DESTINC        = DMA_DESTINC_1;
    
    // Using DMA 0
    SET_WORD(DMA0CFGH, DMA0CFGL, &dmaConfig);
}
/*==== END OF FILE ==========================================================*/
