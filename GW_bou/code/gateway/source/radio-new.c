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
//  0xDF09 - 0xDFB                                FREQ2  FREQ1  FREQ0
static const BYTE freq[3]                     = { 0x64,  0x6A,  0xAA }; 
//  0xDF0C - 0xDF1F                               MDMCFG4  MDMCFG3  MDMCFG2  MDMCFG1  MDMCFG0 DEVIATN MCSM2 MCSM1           MCSM0
static const BYTE regs[20]                    = { 0x69,    0x7,     0x01,    0x23,    0x59,   0x45,    0x7,  0x30, /*0x32,*/ 0x00,
//                                                FOCCFG   BSCFG
                                                  0x16,    0x6C, 
//                                                AGCCTRL2  AGCCTRL1  AGCCTRL0  FREND1  FREND0  FSCAL3  FSCAL2  FSCAL1 FSCAL0
                                                  0x43,    0x40,     0x91,     0x56,    0x10,   0xA9,   0x0A,   0x00,  0x11 };
//  0xDF23-0xDF25                                 TEST2 TEST1 TEST0
static const BYTE tst210_defvals[3]           = { 0x81, 0x35, 0x09 };
//  0xDF02-0xDF05                                 PKTLEN            PKTCTRL1  PKTCTRL0
static const BYTE pkt_defvals[3]              = { PACKET_LENGTH_ED, 0x85,     0x45 }; 
//  0xDF2F-0xDF31                                 IOCFG2  IOCFG1 IOCFG0
static const BYTE iocfg[3]                    = { 0x5B,   0x6F,  0x5C }; 
// 
//void zerofill( BYTE *ptr, BYTE size){  while( size-- ) *ptr++ = 0; } //mymemset( (BYTE *)ptr, 0, size ); }
extern void mymemcpy(void *a, void *b, BYTE c);

void radioConfigure(BYTE dataRate) {
    // Configure the radio frequency to use
    mymemcpy( (BYTE *)0xDF09, (BYTE *)freq, 3 );  // ???
    mymemcpy( (BYTE *)0xDF0C, (BYTE *)regs, 20 );
    mymemcpy( (BYTE *)0xDF23, (BYTE *)tst210_defvals, 3 );
    // Settings from SmartRFStudio for CC2511, VERSION == 0x04
    // 10 kBaud, 2-FSK modulation, 232 kHz RX filter bandwidth.
    FSCTRL1  = 0x08;   // Frequency synthesizer control.
     // AGC control.
    perRssiOffset = 74;// Set proper RSSI offset for receiver
    mymemcpy( (BYTE *)0xDF02, (BYTE *)pkt_defvals, 3 );
    mymemcpy( (BYTE *)0xDF2F, (BYTE *)iocfg,       3 );
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
*
* Parameters:
* @param  BYTE rssiComp                   The RSSI value received from the radio as a 2's complement number
* @return INT16                           The RSSI value in dBm
*
******************************************************************************/
INT16 convertRssiByte(BYTE rssiComp){
    // Convert RSSI value from 2's complement to decimal value.
    INT16 rssiDec = (INT16) rssiComp;
    // Convert to absolute value (RSSI value from radio has resolution of
    // 0.5 dBm) and subtract the radio's appropriate RSSI offset.
    if(rssiDec < 128)   return (rssiDec/2) - perRssiOffset;
    else                return ((rssiDec - 256)/2) - perRssiOffset;
}
/*==== END OF FILE ==========================================================*/
