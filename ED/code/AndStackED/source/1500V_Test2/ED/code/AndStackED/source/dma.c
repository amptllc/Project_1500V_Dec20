/*-----------------------------------------------------------------------------
| Purpose:    Functions for setup of DMA used with radio.
+------------------------------------------------------------------------------
| Decription: Functions to configure the DMA channel 0 for transport of data
|             either to or from the radio. For use by packet error rate test
|             application.
+----------------------------------------------------------------------------*/

/*==== DECLARATION CONTROL ===================================================*/
/*==== INCLUDES ==============================================================*/

#include "hal_main.h"
#include "dma.h"
#include "main.h"


/*==== GLOBAL VARS============================================================*/

BYTE radioPktBuffer[  PACKET_LENGTH_ED + 3];
BYTE radioPktBufferRx[PACKET_LENGTH_GW + 3];  // to accomodate maximum possible package
BYTE radioPktBufferTx[PACKET_LENGTH_ED + 3];

DMA_DESC dmaConfig[3];                  // Struct for the DMA configuration

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

extern BYTE *pagePtr;

#define PAGESIZE 248

//void setupTxBuffer( BYTE *ptr ){ SET_WORD(dmaConfig[1].SRCADDRH,  dmaConfig[1].SRCADDRL, ptr); }

void dmaRadioSetup(void){
    // Some configuration that are common for both TX and RX:
/*
    // CPU has priority over DMA
    // Use 8 bits for transfer count
    // No DMA interrupt when done
    // DMA triggers on radio
    // Single transfer per trigger.
    // One byte is transferred each time.
    dmaConfig.PRIORITY       = DMA_PRI_LOW;
    dmaConfig.M8             = DMA_M8_USE_8_BITS;
    dmaConfig.IRQMASK        = DMA_IRQMASK_DISABLE;
    dmaConfig.TRIG           = DMA_TRIG_RADIO;
    dmaConfig.TMODE          = DMA_TMODE_SINGLE;
    dmaConfig.WORDSIZE       = DMA_WORDSIZE_BYTE;
*/
    //}else if (mode == RADIO_MODE_RX) {

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
        SET_WORD(dmaConfig[0].SRCADDRH,  dmaConfig[0].SRCADDRL, &X_RFD);
        SET_WORD(dmaConfig[0].DESTADDRH, dmaConfig[0].DESTADDRL, radioPktBufferRx);
        dmaConfig[0].VLEN           = DMA_VLEN_FIRST_BYTE_P_3;
        SET_WORD(dmaConfig[0].LENH, dmaConfig[0].LENL, (PACKET_LENGTH_GW + 3));
        dmaConfig[0].SRCINC         = DMA_SRCINC_0;
        dmaConfig[0].DESTINC        = DMA_DESTINC_1;
        
    //if (mode == RADIO_MODE_TX) {
        
        SET_WORD(DMA0CFGH, DMA0CFGL, &dmaConfig);
        // Transmitter specific DMA settings

        // Source: radioPktBuffer
        // Destination: RFD register
        // Use the first byte read + 1
        // Sets the maximum transfer count allowed (length byte + data)
        // Data source address is incremented by 1 byte
        // Destination address is constant
        dmaConfig[1].PRIORITY       = DMA_PRI_LOW;
        dmaConfig[1].M8             = DMA_M8_USE_8_BITS;
        dmaConfig[1].IRQMASK        = DMA_IRQMASK_DISABLE;
        dmaConfig[1].TRIG           = DMA_TRIG_RADIO;
        dmaConfig[1].TMODE          = DMA_TMODE_SINGLE;
        dmaConfig[1].WORDSIZE       = DMA_WORDSIZE_BYTE;
        
        SET_WORD(dmaConfig[1].SRCADDRH,  dmaConfig[1].SRCADDRL, radioPktBufferTx);
        SET_WORD(dmaConfig[1].DESTADDRH, dmaConfig[1].DESTADDRL, &X_RFD);
        dmaConfig[1].VLEN           = DMA_VLEN_FIRST_BYTE_P_1;
        SET_WORD(dmaConfig[1].LENH, dmaConfig[1].LENL, (PACKET_LENGTH_ED + 1));
        dmaConfig[1].SRCINC         = DMA_SRCINC_1;
        dmaConfig[1].DESTINC        = DMA_DESTINC_0;
        
        dmaConfig[2].PRIORITY       = DMA_PRI_LOW;                    // DMA has priority over CPU - as recommended
        dmaConfig[2].M8             = DMA_M8_USE_8_BITS;               // Use 8 bits for transfer count
        dmaConfig[2].IRQMASK        = DMA_IRQMASK_ENABLE;              // DMA interrupt when done
        dmaConfig[2].TRIG           = DMA_TRIG_FLASH;                  // DMA triggers on flash
        dmaConfig[2].TMODE          = DMA_TMODE_SINGLE;                // Single transfer per trigger.
        dmaConfig[2].WORDSIZE       = DMA_WORDSIZE_BYTE;               // One byte is transferred each time.

        //SET_WORD(dmaConfig[2].SRCADDRH,  dmaConfig[2].SRCADDRL,  pagePtr);     // Source: ptr
        dmaConfig[2].VLEN           = DMA_VLEN_USE_LEN;
        SET_WORD(dmaConfig[2].DESTADDRH, dmaConfig[2].DESTADDRL, 0xDFAF );   // Destination: FWDATA register
        dmaConfig[2].LENH = 0;
        dmaConfig[2].LENL = PAGESIZE;      // Sets the maximum transfer count allowed (length byte + data)
        dmaConfig[2].SRCINC         = DMA_SRCINC_1;                    // Data source address is incremented by 1 byte
        dmaConfig[2].DESTINC        = DMA_DESTINC_0;                   // Destination address is constant
        
    //}
    // Save pointer to the DMA configuration struct into DMA-channel 0
    // configuration registers
        
        SET_WORD(DMA1CFGH, DMA1CFGL, &dmaConfig[1]);
        
    return;
}


/*==== END OF FILE ==========================================================*/
