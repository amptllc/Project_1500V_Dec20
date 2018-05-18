/*==== DECLARATION CONTROL ===================================================*/
/*==== INCLUDES ==============================================================*/
#include <string.h>

#include "hal_main.h"
#include "main.h"

#define FOC_MODERATE_10K        0x16
#define FOC_CONSTUPEDIC_10K     0x05
#define FOC_RELAXED_10K         0x1B
#define FOC_CONFIG_10K          FOC_RELAXED_10K

#define DEVIATN_CONSTUPEDIC_10K 0x1
#define DEVIATN_RELAXED_10K     0x7
#define DEVIATN_MODERATE_10K    0x5
#define DEVIATN_CONFIG_10K      DEVIATN_MODERATE_10K

/*==== GLOBAL VARS ===========================================================*/
static INT16 perRssiOffset;             // RSSI offset for receiver, depends on chip model and data rate
/*==== PUBLIC FUNCTIONS ======================================================*/

/******************************************************************************
* @fn  radioConfigure
*
* @brief
*        Configures the radio, either CC1110 or CC2510, supporting a set
*        of preset data rates and frequency options. Also, the transmitter's
*        transmit power level and receiver's RSSI offset is set.
*
* Parameters:
*
* @param  UINT32 dataRate
*           Data rate in bits per second (bps)
* @param  UINT32 frequency
*           RX/TX radio frequency to use (kHz)
*
* @return void
*
******************************************************************************/
void radioConfigure(BYTE dr ) {
UINT32 dataRate = DATA_RATE_3_CC2511;
    /* NOTE: The register settings are hard-coded for the predefined set of data
     * rates and frequencies. To enable other data rates or frequencies these
     * register settings should be adjusted accordingly (use SmartRF(R) Studio).
     */

    // Configure the radio frequency to use
    //switch (frequency) {
    //    case FREQUENCY_FCC_CC2511:      // 2410
             FREQ2 = 0x64;            
             FREQ1 = 0x6A;            
             FREQ0 = 0xAA;      //   2410 MHz - FCC frequency
    //    break;
    //}


    // Set modulation and other radio parameters
    MDMCFG1 = 0x23;//   Channel spacing (exponent) -> CHANSPC_E, 23 - wo FEC, 252.6 channel spacing, 4 bytes of preamble is transmitted
    //MDMCFG1 = 0x23;//   Channel spacing (exponent) -> CHANSPC_E, 23 - wo FEC, 252.6 channel spacing, 4 bytes of preamble is transmitted
    MDMCFG0 = 0x59;//   Channel spacing (mantissa) -> CHANSPC_M
    switch (dataRate) {
        case DATA_RATE_3_CC2511:
            // Settings from SmartRFStudio for CC2511, VERSION == 0x04
            // 10 kBaud, 2-FSK modulation, 232 kHz RX filter bandwidth.
            FSCTRL1  = 0x08;   // Frequency synthesizer control.
            FSCTRL0  = 0x00;   // Frequency synthesizer control.

            //MDMCFG4  = 0x68;   // Modem configuration. 10 kbod
            //MDMCFG3  = 0xB5;   // Modem configuration.
            MDMCFG4  = 0x69;   // Modem configuration. 12 kbod
            MDMCFG3  = 0x7;    // Modem configuration.
            
            MDMCFG2  = 0x02;   // Modem configuration. 2 byte sync
            //MDMCFG2  = 0x02 | 0x08;   // Modem configuration. 2 byte sync Manchester encoding
            
            //MDMCFG1  = 0x23;   // Modem configuration.
            //MDMCFG0  = 0xA8;

            DEVIATN  = 0x45;   // Modem deviation setting (when FSK modulation is enabled).
            
            FREND1   = 0x56;   // Front end RX configuration.
            FREND0   = 0x10;   // Front end RX configuration.

            //MCSM0    = 0x14;   // Main Radio Control State Machine configuration.
            MCSM0    = 0x14;   

            FOCCFG   = 0x16;   // Frequency Offset Compensation Configuration.
            BSCFG    = 0x6C;   // Bit synchronization Configuration.
            
            AGCCTRL2 = 0x43;   // AGC control.            
            AGCCTRL1 = 0x40;   // AGC control.
            AGCCTRL0 = 0x91;   // AGC control.
            FSCAL3   = 0xA9;   // Frequency synthesizer calibration.
            FSCAL2   = 0x0A;   // Frequency synthesizer calibration.
            FSCAL0   = 0x11;   // Frequency synthesizer calibration.
            FSCAL1   = 0x0;
            
            //TEST2    = 0x88;   // Various test settings.
            //TEST1    = 0x31;   // Various test settings.
            TEST2    = 0x81;   // Various test settings.
            TEST1    = 0x35;   // Various test settings.
            TEST0    = 0x09;   // Various test settings.
    
            perRssiOffset = 74;// Set proper RSSI offset for receiver
        break;

        case DATA_RATE_4_CC2511:
            // Settings from SmartRFStudio for CC2511, VERSION == 0x04
            // 30 kBaud, 2-FSK modulation, 300 kHz RX filter bandwidth, 252.6 channel spacing
            FSCTRL1 = 0x0A;//               IF Frequency -> FREQ_IF[4:0] => 234.38 kHz
            MDMCFG4 = 0x6A;
            //Data rate (exponent) -> DRATE_E
            //Channel bandwidth (exponent) -> CHANBW_E
            //Channel bandwidth (mantissa) -> CHANBW_M
            MDMCFG3 = 0x48; //   Data rate (mantissa) -> DRATE_M
            
            MDMCFG2 = 0x13;//
            
            DEVIATN = 0x45;

            FREND1 = 0xB6;  FREND0 = 0x10;//               RF output power -> PA_POWER
            //MCSM0 = 0x14;
            
            FOCCFG = 0x1D;
            BSCFG    = 0x1C;

            AGCCTRL2 = 0xC7;         AGCCTRL1 = 0x00;         AGCCTRL0 = 0xB2;
            FSCAL3 = 0xEA;           FSCAL2 = 0x0A;           FSCAL1 = 0x00;            FSCAL0 = 0x11;
           
            //TEST2 = 0x88;            TEST1 = 0x31;            TEST0 = 0x09;
            TEST2 = 0x81;            TEST1 = 0x35;            TEST0 = 0x09;
            perRssiOffset = 71;// Set proper RSSI offset for receiver
        break;            
    }

    // Common radio settings for CCxx10, any frequency and data rate
    CHANNR   = 0x00;            // Channel number.
    FSCTRL0  = 0x00;
    MCSM1    = 0x30; //0x32;  // Main Radio Control State Machine configuration. 0x32 - not switch on transmission when packet finished
    MCSM0    = 0x14; // 0x14
    
    IOCFG0 = 0x5C;
    IOCFG1 = 0x6F;
    IOCFG2 = 0x5B;    
    
    PKTCTRL1 = 0x85;            // Packet automation control.
//    PKTCTRL1 = 0x84;            // Packet automation control. no address check
    PKTCTRL0 = 0x45;            // Packet automation control. Data whitening on.
    
    ADDR     = 0x01;            // Device address. Not used.

    PA_TABLE0 = 0xFF;
    SYNC0 = 0xF0;    SYNC1 = 0xF0;
    //SYNC0 = 0xFF;    SYNC1 = 0xFF;
    return;
}
/******************************************************************************
* @fn  pktCheckValidity
* @brief
*      Checks the received packet length and network ID to decide if it is a
*      valid PER test packet, hence affecting PER statistics.
*      The packet's CRC and sequence number will be analyzed in the
*      process of updating the appropriate variables needed to keep PER
*      statistics
* Parameters:
* @param  void
* @return BOOL
*         TRUE: Packet was a PER test packet
*         FALSE: Packet was not recognized as a PER test packet
******************************************************************************/
//static BYTE *ptr;
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
