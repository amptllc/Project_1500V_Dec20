/*
+------------------------------------------------------------------------------
| Purpose:    Functions for radio and packet handling
+------------------------------------------------------------------------------
| Decription: All functions related to radio configuration and packet
|             handling for the packet error rate test application.
+----------------------------------------------------------------------------*/

/*==== DECLARATION CONTROL ===================================================*/
/*==== INCLUDES ==============================================================*/
//#include <string.h>
#include "hal_main.h"
#include "main.h"

#define FOC_MODERATE_10K        0x16
#define FOC_CONSTUPEDIC_10K     0x05
#define FOC_RELAXED_10K         0x1B
#define FOC_CONFIG_10K          FOC_MODERATE_10K

#define DEVIATN_CONSTUPEDIC_10K 0x1
#define DEVIATN_RELAXED_10K     0x7
#define DEVIATN_MODERATE_10K    0x5
#define DEVIATN_CONFIG_10K      0x4

/*==== GLOBAL VARS ===========================================================*/
INT16 perRssiOffset;             // RSSI offset for receiver, depends on chip model and data rate
/*==== PUBLIC FUNCTIONS ======================================================*/
/******************************************************************************
* @fn  radioConfigure
* @brief
*        Configures the radio, either CC1110 or CC2510, supporting a set
*        of preset data rates and frequency options. Also, the transmitter's
*        transmit power level and receiver's RSSI offset is set.
* Parameters:
* @param  UINT32 dataRate            Data rate in bits per second (bps)
* @param  UINT32 frequency           RX/TX radio frequency to use (kHz)
* @return void
******************************************************************************/
static const BYTE mdm_rate12[3]               = { 0x1D, 0x55, 0x73};
static const BYTE mdm_rate3[3]                = { 0x69, 0x7, 0x02 };   
static const BYTE mdm_rate4[3]                = { 0x6A, 0x48, 0x13 };   
static const BYTE foc_bs_agc_rate4[5]         = { 0x1D, 0x1C, 0xC7, 0x00, 0xB2 };
static const BYTE mcsm_defvals[3]             = { 0x7,  0x30, /*0x32,*/ 0x14 };
static const BYTE agc_frend_fscal_defvals[9]  = { 0x43, 0x40, 0x91, 0xB6, 0x10, 0xA9, 0x0A, 0x00, 0x11 };
static const BYTE foc_bs_agc_rate12[5]        = { 0x1D, 0x1C, 0xC7, 0x40, 0xB2 };
static const BYTE tst210_defvals[3]           = { 0x81, 0x35, 0x09 }; 
static const BYTE pkt_defvals[3]              = { PACKET_LENGTH_ED, 0x85, 0x45 }; 

void radioConfigure(BYTE dataRate/*, UINT32 frequency*/) {

    /* NOTE: The register settings are hard-coded for the predefined set of data
     * rates and frequencies. To enable other data rates or frequencies these
     * register settings should be adjusted accordingly (use SmartRF(R) Studio).
     */
    // Configure the radio frequency to use
    //FREQ2 = 0x64;  FREQ1 = 0x6A; FREQ0 = 0xAA; // always FCC
    /*
    switch (frequency) {
        case FREQUENCY_A_CC2511:    FREQ1 = 0x15; FREQ0 = 0x55;  break; // 2402 MHz 
        case FREQUENCY_FCC_CC2511:  FREQ1 = 0x6A; FREQ0 = 0xAA;  break; // 2410 MHz FCC
    }
    */
    // Set modulation and other radio parameters
    MDMCFG1 = 0x23; //   Channel spacing (exponent) -> CHANSPC_E, 23 - wo FEC, 4 bytes of preamble is transmitted
    MDMCFG0 = 0x59; //   Channel spacing (mantissa) -> CHANSPC_M, 252.6 channel spacing

    //AGCCTRL2 = 0x43; AGCCTRL1 = 0x40;  AGCCTRL0 = 0x91;
    //FREND1 = 0xB6;   FREND0   = 0x10;   
    //FSCAL3   = 0xA9; FSCAL2 = 0x0A;  FSCAL1 = 0x00;  FSCAL0 = 0x11;
    mymemcpy( (BYTE *)0xDF17, (BYTE *)agc_frend_fscal_defvals, 9 );
    //TEST2    = 0x81;   TEST1  = 0x35;     TEST0    = 0x09;
    mymemcpy( (BYTE *)0xDF23, (BYTE *)tst210_defvals, 3 );
    
    DEVIATN  = 0x45; //   Frequency dev. (exponent) -> DEVIATN_E   Frequency dev. (mantissa) -> DEVIATN_M
    FSCTRL1  = 0x0A;
    perRssiOffset = 71;
    
    switch (dataRate) {
        case DATA_RATE_2_CC2511:  case DATA_RATE_1_CC2511:
            //MDMCFG4  = 0x1D; //   Data rate (exponent) -> DRATE_E, Channel bandwidth (exponent) -> CHANBW_E,  Channel bandwidth (mantissa) -> CHANBW_M
            //MDMCFG3  = 0x55; //   Data rate (mantissa) -> DRATE_M
            //MDMCFG2  = 0x73; //   Modulation -> MOD_FORMAT[2:0],  Manchester enable -> MANCHESTER_EN,   Optimization -> -
            mymemcpy( (BYTE *) 0xDF0C, (BYTE *) mdm_rate12, 3 );
            if( dataRate == DATA_RATE_2_CC2511 ){
                // Settings from SmartRFStudio for CC2511, VERSION == 0x04
                // 250 kbod, MSK modulation, 600 kHz filter bandwith
                //FSCTRL1  = 0x0A; //   IF Frequency -> FREQ_IF[4:0] => 234.38 kHz
                //perRssiOffset = 71;// Set proper RSSI offset for receiver
            }else{
                // Settings from SmartRFStudio for CC2511, VERSION == 0x04
                // 500 kBaud, MSK modulation, 812 kHz RX filter bandwidth.
                FSCTRL1  = 0x10;   // Frequency synthesizer control.
                MDMCFG4  = 0x0E;   
                MDMCFG1  = 0x43; // 8 bytes of preamble instead of 4 
                FSCAL3   = 0xEA;   
                perRssiOffset = 72;// Set proper RSSI offset for receiver
            }
            DEVIATN  = 0x00;   // Modem deviation setting (when FSK modulation is enabled).
            //FREND1   = 0x56;
            TEST2    = 0x88;  TEST1 = 0x31;    
            //FOCCFG   = 0x1D;  BSCFG = 0x1C;    AGCCTRL2 = 0xC7;  AGCCTRL0 = 0xB2;
            mymemcpy( (BYTE *)0xDF15, (BYTE *)foc_bs_agc_rate12, 5 );
        break;

        case DATA_RATE_3_CC2511:
            // Settings from SmartRFStudio for CC2511, VERSION == 0x04
            // 10 kBaud, 2-FSK modulation, 232 kHz RX filter bandwidth.
            FSCTRL1  = 0x08;   // Frequency synthesizer control.
            
            // MDMCFG4  = 0x68;   MDMCFG3  = 0xB5;   // 10 kbod Modem configuration. - 2 byte sync word

            //MDMCFG4  = 0x69;   
            //MDMCFG3  = 0x7;       // 12 kbod Modem configuration. - 2 byte sync word
            //MDMCFG2  = 0x02;   
            mymemcpy( (BYTE *)0xDF0C, (BYTE *)mdm_rate3, 3 );

            FREND1   = 0x56;  // Front end RX configuration.
            FOCCFG   = 0x16;   // Frequency Offset Compensation Configuration.
            BSCFG    = 0x6C; //0x61;   // Bit synchronization Configuration.
             // AGC control.
            perRssiOffset = 74;// Set proper RSSI offset for receiver
        break;
        
        case DATA_RATE_4_CC2511:
            // Settings from SmartRFStudio for CC2511, VERSION == 0x04
            // 30 kBaud, 2-FSK modulation, 300 kHz RX filter bandwidth, 252.6 channel spacing
            //FSCTRL1  = 0x0A;//               IF Frequency -> FREQ_IF[4:0] => 234.38 kHz
          
            //MDMCFG4  = 0x6A;    // Data rate (exponent) -> DRATE_E, Channel bandwidth (exponent) -> CHANBW_E, Channel bandwidth (mantissa) -> CHANBW_M
            //MDMCFG3  = 0x48; //   Data rate (mantissa) -> DRATE_M
            //MDMCFG2  = 0x13;//
            mymemcpy( (BYTE *)0xDF0C, (BYTE *)mdm_rate4, 3 );
            //FOCCFG   = 0x1D;            BSCFG    = 0x1C;
            //AGCCTRL2 = 0xC7;            AGCCTRL1 = 0x00;            AGCCTRL0 = 0xB2;
            mymemcpy( (BYTE *)0xDF15, (BYTE *)foc_bs_agc_rate4, 5 );
            FSCAL3 = 0xEA; 
            //perRssiOffset = 71;// Set proper RSSI offset for receiver
        break;            
    }
    //MCSM2 = 0x7; MCSM1 = 0x32; MCSM0 = 0x14;  // Main Radio Control State Machine configuration. 2 means "Stay in TX"
    mymemcpy( (BYTE *)0xDF12, (BYTE *)mcsm_defvals, 3 );
    
    // We use P1_6, P1_7 for our timers, we do not have radio amplifiers 
    //IOCFG2 = 0;    IOCFG1 = 0;     IOCFG0 = 0;
    zerofill( (BYTE *)0xDF2F, 3 );
    
    //PKTLEN   = PACKET_LENGTH_ED;   // Packet length.
    //PKTCTRL1 = 0x85;// 0x04;    // Packet automation control. - add status and check addr, 16 bit of preamble should match ...
    //PKTCTRL0 = 0x45;            // Packet automation control. Data whitening on. Variable packet length
    
    mymemcpy( (BYTE *)0xDF02, (BYTE *)pkt_defvals, 3 );
    
    //ADDR     = 0x02;            // Device address. 
    //PA_TABLE0 = 0xFF; 
    //SYNC0 = 0xF0;    SYNC1 = 0xF0;
    return;
}
/******************************************************************************
* @fn  convertRssiByte
*
* @brief
*      Converts the RSSI (received signal strength indicator) value,
*      given as a 2's complement number, to dBm value. This requires that a
*      proper RSSI offset value is specified in global variable perRssiOffset
*      before use.
* Parameters:
* @param  BYTE rssiComp The RSSI value received from the radio as a 2's complement number
* @return INT16           The RSSI value in dBm
******************************************************************************/
/*
INT16 convertRssiByte(BYTE rssiComp){
    // Convert RSSI value from 2's complement to decimal value.
    //INT16 rssiDec = (INT16) rssiComp;
    // Convert to absolute value (RSSI value from radio has resolution of
    // 0.5 dBm) and subtract the radio's appropriate RSSI offset.
    if(rssiComp < 128)   return (rssiComp >> 1) - perRssiOffset;
    else                 return (((UINT16)rssiComp - 256) >> 1) - perRssiOffset;
    
    //if( rssiComp >= 128 ) rssiDec -= 256; //else rssiDec = (INT16)rssiComp;
    //return rssiDec>>1 - perRssiOffset;
}
*/
/*==== END OF FILE ==========================================================*/
