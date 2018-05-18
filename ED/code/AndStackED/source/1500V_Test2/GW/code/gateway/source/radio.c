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
void radioConfigure(UINT32 dataRate, UINT32 frequency) {

    /* NOTE: The register settings are hard-coded for the predefined set of data
     * rates and frequencies. To enable other data rates or frequencies these
     * register settings should be adjusted accordingly (use SmartRF(R) Studio).
     */

    // Set transmit power: 0 dBm
    PA_TABLE0 = 0xFE;

    // Configure the radio frequency to use
    switch (frequency) {
        /*
        case FREQUENCY_1_CC2510:             // 2480 MHz
        default:
            FREQ2 = 0x5F;
            FREQ1 = 0x62;
            FREQ0 = 0x76;
            break;
        case FREQUENCY_2_CC2510:        // 2460 MHz
            FREQ2 = 0x5E;
            FREQ1 = 0x9D;
            FREQ0 = 0x89;
            break;
        case FREQUENCY_3_CC2510:        // 2440 MHz
            FREQ2 = 0x5D;
            FREQ1 = 0xD8;
            FREQ0 = 0x9D;
            break;
        case FREQUENCY_4_CC2510:        // 2420 MHz
            FREQ2 = 0x5D;
            FREQ1 = 0x13;
            FREQ0 = 0xB1;
            break;
       */            
        //______________________________________________________________________________________________________________________
        case FREQUENCY_1_CC2511:        // 2480 MHz
            FREQ2 = 0x67;
            FREQ1 = 0x55;
            FREQ0 = 0x55;
        break;
        case FREQUENCY_2_CC2511:        // 2460 MHz
            FREQ2 = 0x66;
            FREQ1 = 0x80;
            FREQ0 = 0x00;
        break;
        case FREQUENCY_3_CC2511:        // 2440 MHz
            FREQ2 = 0x65;
            FREQ1 = 0xAA;
            FREQ0 = 0xAA;
        break;
        case FREQUENCY_4_CC2511:        // 2420 MHz
            FREQ2 = 0x64;
            FREQ1 = 0xD5;
            FREQ0 = 0x55;
        break;
        case FREQUENCY_A_CC2511: 
            // FREQ2 = 0x64;            FREQ1 = 0x35;            FREQ0 = 0x55;      // 2405 MHz
             FREQ2 = 0x64;            FREQ1 = 0x15;            FREQ0 = 0x55;      // 2402 MHz
            //   FREQ2 = 0x64;            FREQ1 = 0x6A;            FREQ0 = 0xAA;      //   2410 MHz
        break;
        case FREQUENCY_B_CC2511:        // 2301
            FREQ2 = 0x5F;               //   RF Frequency -> FREQ[23:16]
            FREQ1 = 0xE0;               //   RF Frequency -> FREQ[15:8]
            FREQ0 = 0x00;               //   RF Frequency -> FREQ[7:0]        
        break;
        case FREQUENCY_FCC_CC2511:      // 2410
             FREQ2 = 0x64;            
             FREQ1 = 0x6A;            
             FREQ0 = 0xAA;      //   2410 MHz - FCC frequency
        break;

        case FREQUENCY_C_CC2511:        // 2580
            FREQ2 = 0x6B;   //  RF Frequency -> FREQ[23:16]
            FREQ1 = 0x80;   //  RF Frequency -> FREQ[15:8]
            FREQ0 = 0x00;   //  RF Frequency -> FREQ[7:0]
        break;
    }


    // Set modulation and other radio parameters
    MDMCFG1 = 0x23;//   Channel spacing (exponent) -> CHANSPC_E, 23 - wo FEC, 252.6 channel spacing, 4 bytes of preamble is transmitted
    //MDMCFG1 = 0x23;//   Channel spacing (exponent) -> CHANSPC_E, 23 - wo FEC, 252.6 channel spacing, 4 bytes of preamble is transmitted
    MDMCFG0 = 0x59;//   Channel spacing (mantissa) -> CHANSPC_M
    switch (dataRate) {
        /*
        case DATA_RATE_1_CC2511:
            // Settings from SmartRFStudio for CC2511, VERSION == 0x04
            // 500 kBaud, MSK modulation, 812 kHz RX filter bandwidth.
            FSCTRL1  = 0x10;   // Frequency synthesizer control.
            FSCTRL0  = 0x00;   // Frequency synthesizer control.
            MDMCFG4  = 0x0E;   // Modem configuration.
            MDMCFG3  = 0x55;   // Modem configuration.
            //MDMCFG2  = 0x77;   // Modem configuration.
            MDMCFG2  = 0x73;   // Modem configuration.

            MDMCFG1  = 0x43;   // Modem configuration.
            MDMCFG0  = 0x11;   // Modem configuration.
            CHANNR   = 0x00;
            DEVIATN  = 0x00;   // Modem deviation setting (when FSK modulation is enabled).
            FREND1   = 0xB6;   // Front end RX configuration.
            FREND0   = 0x10;   // Front end RX configuration.
            
            MCSM0    = 0x14;   // Main Radio Control State Machine configuration.
            
            FOCCFG   = 0x1D;   // Frequency Offset Compensation Configuration.
            BSCFG    = 0x1C;   // Bit synchronization Configuration.
            
            //AGCCTRL2 = 0xC7;   // AGC control.
            AGCCTRL2 = 0x07;   // AGC control.
            
            AGCCTRL1 = 0x40;   // AGC control.
            AGCCTRL0 = 0xB2;   // AGC control.
            FSCAL3   = 0xEA;   // Frequency synthesizer calibration.
            FSCAL2   = 0x0A;   // Frequency synthesizer calibration.
            FSCAL1   = 0x00;
            FSCAL0   = 0x11;   // Frequency synthesizer calibration.
            TEST2    = 0x88;   // Various test settings.
            TEST1    = 0x31;   // Various test settings.
            TEST0    = 0x09;   // Various test settings.
    
            perRssiOffset = 72;// Set proper RSSI offset for receiver
            break;
    
        case DATA_RATE_2_CC2511:
            // Settings from SmartRFStudio for CC2511, VERSION == 0x04
            // 250 kBaud, MSK modulation, 540 kHz RX filter bandwidth.
            FSCTRL1  = 0x0A;   // Frequency synthesizer control.
            FSCTRL0  = 0x00;   // Frequency synthesizer control.
            MDMCFG4  = 0x1D;   // Modem configuration.
            MDMCFG3  = 0x55;   // Modem configuration.
            MDMCFG2  = 0x73;   // Modem configuration.
            
            //MDMCFG1  = 0x23;   // Modem configuration.
            MDMCFG1  = 0xA3;   // Modem configuration. FEC enabled
            
            MDMCFG0  = 0x11;   // Modem configuration.
            DEVIATN  = 0x00;   // Modem deviation setting (when FSK modulation is enabled).
            FREND1   = 0xB6;   // Front end RX configuration.
            FREND0   = 0x10;   // Front end RX configuration.
            
            MCSM0    = 0x14;   // Main Radio Control State Machine configuration.
            
            FOCCFG   = 0x1D;   // Frequency Offset Compensation Configuration.
            BSCFG    = 0x1C;   // Bit synchronization Configuration.
            AGCCTRL2 = 0xC7;   // AGC control.
            AGCCTRL1 = 0x00;   // AGC control.
            AGCCTRL0 = 0xB2;   // AGC control.
            FSCAL3   = 0xEA;   // Frequency synthesizer calibration.
            FSCAL2   = 0x0A;   // Frequency synthesizer calibration.
            FSCAL0   = 0x11;   // Frequency synthesizer calibration.
            TEST2    = 0x88;   // Various test settings.
            TEST1    = 0x31;   // Various test settings.
            TEST0    = 0x09;   // Various test settings.
    
            perRssiOffset = 71;// Set proper RSSI offset for receiver
            break;
        */
#ifdef RevC
        case DATA_RATE_3_CC2511:
            // Settings from SmartRFStudio for CC2511, VERSION == 0x04
            // 10 kBaud, 2-FSK modulation, 232 kHz RX filter bandwidth.
            FSCTRL1  = 0x08;   // Frequency synthesizer control.
            FSCTRL0  = 0x00;   // Frequency synthesizer control.
            
            //MDMCFG4  = 0x68;   // Modem configuration. 10 kbod
            //MDMCFG3  = 0xB5;   // Modem configuration.
            MDMCFG4  = 0x69;   // Modem configuration. 12 kbod
            MDMCFG3  = 0x7;    // Modem configuration.


            MDMCFG2  = 0x02;   // Modem configuration. - 2 byte sync word
            MDMCFG1  = 0x23;   // Modem configuration.  310.54 channel spacing
            MDMCFG0  = 0xA8;
            
            DEVIATN  = 0x45;
            FREND1   = 0x56;   // Front end RX configuration.
            FREND0   = 0x10;   // Front end RX configuration.
    
            //MCSM0    = 0x14;   // Main Radio Control State Machine configuration.
            MCSM0    = 0x04;   // Main Radio Control State Machine configuration. - no autocalibration
            
            FOCCFG   = 0x16;   // Frequency Offset Compensation Configuration.
            
            BSCFG    = 0x6C; //0x61;   // Bit synchronization Configuration.

            AGCCTRL2 = 0x43;   // AGC control.
            //AGCCTRL2 = 0x00;
            //AGCCTRL2 = 0x83;
            
            AGCCTRL1 = 0x40;   // AGC control.
            AGCCTRL0 = 0x91;   // AGC control.
            FSCAL3   = 0xA9;   // Frequency synthesizer calibration.
            FSCAL2   = 0x0A;   // Frequency synthesizer calibration.
            FSCAL0   = 0x11;   // Frequency synthesizer calibration.
  
            FSCAL1   = 0x00;
            
            //TEST2    = 0x88;   // Various test settings.
            //TEST1    = 0x31;   // Various test settings.
            TEST2    = 0x81;   // Various test settings.
            TEST1    = 0x35;   // Various test settings.

            TEST0    = 0x09;   // Various test settings.

            perRssiOffset = 74;// Set proper RSSI offset for receiver
    break;
#else
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
#endif
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
    MCSM1    = 0x32; // 0x30;            // Main Radio Control State Machine configuration.
    MCSM0    = 0x14; // 0x14
    
    IOCFG0 = 0x5C;
    IOCFG1 = 0x6F;
    IOCFG2 = 0x5B;    
    
    PKTCTRL1 = 0x86;            // Packet automation control.
//    PKTCTRL1 = 0x84;            // Packet automation control. no address check
    PKTCTRL0 = 0x45;            // Packet automation control. Data whitening on.
    
    ADDR     = 0x01;            // Device address. Not used.

    PA_TABLE0 = 0xFF;
    SYNC0 = 0xF0;    SYNC1 = 0xF0;
    
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
/*==== PRIVATE FUNCTIONS =====================================================*/
/******************************************************************************
* @fn  pktCheckCrc
* @brief      Check the if the CRC is correct for the received packet
* Parameters:
* @param  void
* @return BOOL
*          TRUE: The CRC was correct
*          FALSE: The CRC was wrong
******************************************************************************/
BOOL pktCheckCrc(void){
    // Check if CRC_OK bit (bit 7) in the second status byte received is set
    if(radioPktBufferRx[PACKET_LENGTH_ED + 2] & 0x80){
        radioPktBufferRx[PACKET_LENGTH_ED + 2] = 0x00;   // Clear status byte in buffer
        return TRUE;
    }
    return FALSE;
}
/******************************************************************************
* @fn  pktCheckId
* @brief
*      Check the NETWORK_ID_KEY of the received packet to ensure we are the
*      intended recipient.
* Parameters:
* @param  void
* @return BOOL
*         TRUE: The NETWORK_ID_KEY was correct
*         FALSE: The NETWORK_ID_KEY was wrong
*
******************************************************************************/
//static BOOL pktCheckId(void){
    /*
    // The NETWORK_ID_KEY is sent as the second and third byte in the packet
    if ((radioPktBuffer[0]==(BYTE)(NETWORK_ID_KEY>>8)) &&
        (radioPktBuffer[1]==(BYTE)NETWORK_ID_KEY)) {
        // Reset the NETWORK_ID_KEY from packet buffer to ensure that the next packet will
        // have to update the buffer with it again (to rule out false positives).
        radioPktBuffer[0] = 0x00;        radioPktBuffer[1] = 0x00;
        return TRUE;
    }else if ((radioPktBuffer[0]==0xFF) && (radioPktBuffer[1]==0xFF)) {
        //radioPktBuffer[0] = 0x00;        radioPktBuffer[1] = 0x00;
        return TRUE;
    }
    return FALSE;
    */
//    return ((radioPktBuffer[0]==(BYTE)(NETWORK_ID_KEY>>8)) && (radioPktBuffer[1]==(BYTE)NETWORK_ID_KEY)) ||
//           ((radioPktBuffer[0]==0xFF)                      && (radioPktBuffer[1]==0xFF));
//}
/******************************************************************************
* @fn  pktSetSeqNum
* @brief
*      Sets a 4 byte sequence number into the statically defined packet buffer
*      radioPktBuffer
* Parameters:
* @param  UINT32 seqNum
*         The sequence number to write into the buffer
* @return void
*
******************************************************************************/
/*
void pktSetTS(BYTE *rpb, UINT32 utc, UINT16 ms){
ptr = rpb + 4;
    // Insert the 4 byte sequence number into a static packet buffer
    *ptr++ = (BYTE) (utc>>24);
    *ptr++ = (BYTE) (utc>>16);
    *ptr++ = (BYTE) (utc>>8);
    *ptr++ = (BYTE) utc;
    *ptr++ = (BYTE) (ms>>8);
    *ptr   = (BYTE) ms;
}
*/
/******************************************************************************
* @fn  pktGetSeqNum
* @brief
*      Extracts a 4 byte UTC timestamp from the statically defined  packet buffer radioPktBuffer
* Parameters:
* @param  void
* @return UINT32        The received packet's sequence number as a UINT32
*
******************************************************************************/
UINT32 pktGetUTC(BYTE *rpb){
static UINT32 _utc; memcpy( &_utc, rpb, 4 );
    /*
    ptr = rpb;
    // Grab the 4 byte sequence number from static packet buffer
    _utc =  ((UINT32)*ptr++) << 24;
    _utc |= ((UINT32)*ptr++) << 16;
    _utc |= ((UINT32)*ptr++) << 8;
    _utc |= ((UINT32)*ptr);
    */
    return _utc;
}
/******************************************************************************
* @fn  pktGetSeqNum
* @brief
*      Extracts a 2 byte MS timestamp from the statically defined  packet buffer radioPktBuffer
* Parameters:
* @param  void
* @return UINT32        The received packet's sequence number as a UINT32
*
******************************************************************************/
UINT16 pktGetMS(BYTE *rpb){
static UINT16 _ms; memcpy( &_ms, rpb+4, 2 );
    /*
    ptr = rpb + 4;
    // Grab the 4 byte sequence number from static packet buffer
    _ms  = (((UINT16)*ptr++)&0x3) << 8;
    _ms |= ((UINT16)*ptr);
    */
    return _ms  & 0x3FF;
}
/*==== END OF FILE ==========================================================*/
