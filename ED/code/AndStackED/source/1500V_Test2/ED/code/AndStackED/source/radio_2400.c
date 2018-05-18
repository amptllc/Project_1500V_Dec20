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
#include "parameter_block.h"

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
//                                              0x0E  
// DF0C-DF11                                   MDMCFG4 MDMCFG3 MDMCFG2  MDMCFG1  MDMCFG0  DEVIATN
//static const 
BYTE mdm_rate1[6]                           = { 0x0E,   0x55,   0x73,    0x43,    0x59,    0 };

                                             // 0x69                    0xC3?
// DF0C-DF11                                   MDMCFG4 MDMCFG3 MDMCFG2  MDMCFG1  MDMCFG0  DEVIATN
static const BYTE mdm_rate3[20]             = { 0x79,   0x7,    0x01,   0x23,    0x59,     0x45, //};   
//                                               *        *
// 12000                                        0x69    0x7
// 2400                                         0x76    0xA3            0x33               0x51
// 1200                                         0x75    0xA3
// DF12-DF1F                                   MCSM2 MCSM1  MCSM0  FOCCFG  BSCFG  AGCCTRL2  AGCCTRL1  AGCCTRL0  FREND1  FREND0  
                                                0x7,  0x00, 0x14,  0x16,   0x6C,  0x43,     0x40,     0x91,     0x56,   0x10,  
//                                                    0x20         0x16,   0x6C     *
// 2400                                                                           0x03
//                                             FSCAL3  FSCAL2  FSCAL1  FSCAL0
                                                0xA9,   0x0A,   0x00,   0x11 };
/*
// DF0C-DF11                                   MDMCFG4 MDMCFG3 MDMCFG2  MDMCFG1  MDMCFG0  DEVIATN
static const BYTE mdm_rate3_2400[20]        = { 0x76,   0xA3,   0x01,   0x23,    0x59,     0x51, //};   
//                                               *        *
// 12000                                        0x69    0x7
// 2400                                         0x76    0xA3            0x33               0x51
// 1200                                         0x75    0xA3
// DF12-DF1F                                   MCSM2 MCSM1  MCSM0  FOCCFG  BSCFG  AGCCTRL2  AGCCTRL1  AGCCTRL0  FREND1  FREND0  
                                                0x7,  0x00, 0x14,  0x16,   0x6C,  0x03,     0x40,     0x91,     0x56,   0x10,  
//                                                    0x20         0x16,   0x6C     *
// 2400                                                                           0x03
//                                             FSCAL3  FSCAL2  FSCAL1  FSCAL0
                                                0xA9,   0x0A,   0x00,   0x11 };
*/
//                                             TEST2 TEST1 TEST0
static const BYTE tst20_defvals[3]          = { 0x81, 0x35, 0x09 }; 
static const BYTE tst1_defvals[3]           = { 0x88, 0x31, 0x09 }; 
//                                              0x1D           0xC7                0xB2
//                                              FOCCFG  BSCFG  AGCCTRL2  AGCCTRL1  AGCCTRL0  FREND1 FREND0 FSCAL3
/*static const*/ BYTE foc_bs_agc_rate12[8]  = { 0x1D,   0x1C,  0xC7,     0x40,     0xB2,     0xB6,  0x10,  0xEA };
//                                                                                 0x91      0x56
//  DF02-     DF04                              PKTLEN          PKTCTRL1  PKTCTRL0
static const BYTE pkt_defvals[3]            = { PACKET_LENGTH,  0x85,     0x45 };
//                                                              0x80      
// 2400                                                         0xA0      0x40


extern ParameterBlock page;
// was 200, 5000, 5
#define MKS_IN_TICK     500
#define TICKS_IN_SEC    2000
#define TICKS_IN_MS     2
extern INT16  _slot;
extern INT16  _join_slot;
extern INT16  _gw_delay;
extern INT16  _loop_delay;

void radioConfigure( BYTE dataRate ) {
    // Set modulation and other radio parameters
    //if( page.use12kbod ){   
        mymemcpy( (BYTE *)0xDF0C, (BYTE *)mdm_rate3,      20 );  
        _slot         = ( 40 * TICKS_IN_MS );
        _join_slot    = ( 24 * TICKS_IN_MS );
        _gw_delay     = ( 30 * TICKS_IN_MS );
        _loop_delay   = ( 40 * TICKS_IN_MS );
    /*}else{
        mymemcpy( (BYTE *)0xDF0C, (BYTE *)mdm_rate3_2400, 20 );
        _slot         = ( 5 * 40 * TICKS_IN_MS );
        _join_slot    = ( 5 * 24 * TICKS_IN_MS );
        _gw_delay     = ( 5 * 30 * TICKS_IN_MS );
        _loop_delay   = ( 5 * 40 * TICKS_IN_MS );
    }*/
    perRssiOffset = 74; FSCTRL1  = 0x0A;  // Frequency synthesizer control.
    if(dataRate > DATA_RATE_3_CC2511 ){
        // Settings from SmartRFStudio for CC2511, VERSION == 0x04
        // 500 kBaud, MSK modulation, 812 kHz RX filter bandwidth.
        mymemcpy( (BYTE *) 0xDF0C, (BYTE *) mdm_rate1, 6 );
        FSCTRL1  = 0x10;   // Frequency synthesizer control.
        perRssiOffset = 72;// Set proper RSSI offset for receiver
        mymemcpy( (BYTE *)0xDF23, (BYTE *)tst1_defvals, 3 );
        mymemcpy( (BYTE *)0xDF15, (BYTE *)foc_bs_agc_rate12, 8 );
        if( dataRate == DATA_RATE_2_CC2511 ){
            FSCTRL1  = 0x0A;
            FSCAL3   = 0xA9;
            MDMCFG4  = page.bandwidth250; 
        }
    }else mymemcpy( (BYTE *)0xDF23, (BYTE *)tst20_defvals, 3 );
    // We use P1_6, P1_7 for our timers, we do not have radio amplifiers 
    zerofill( (BYTE *)0xDF2F, 3 );
    mymemcpy( (BYTE *)0xDF02, (BYTE *)pkt_defvals, 3 );
    /*
    if( datarate == DATA_RATE_4_CC2511 ){
        MDMCFG4  = 0x76;
        MDMCFG3  = 0xA3;  
        MDMCFG1  = 0x33;  // 6 bytes preamble
        AGCCTRL2 = 0x03;
        PKTCTRL1 = 0xA0;  //
        PKTCTRL0 = 0x40;  // fixed packed length mode
        PKTLEN   = 32;
        DEVIATN  = 0x53;  // 64.453125 kHz
    }
    */
    //return;
}
/*==== END OF FILE ==========================================================*/
