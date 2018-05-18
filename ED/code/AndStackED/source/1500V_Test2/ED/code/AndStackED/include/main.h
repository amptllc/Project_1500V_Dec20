/*-----------------------------------------------------------------------------
| Purpose:    PER test main header file
+------------------------------------------------------------------------------
| Decription: This file contains settings and defines for the packet error rate
|             test application.
+----------------------------------------------------------------------------*/

/*==== DECLARATION CONTROL ==================================================*/

#ifndef PER_TEST_MAIN_H
#define PER_TEST_MAIN_H

/*==== INCLUDES ==============================================================*/
#include "dma.h"

/*==== CONSTS ================================================================*/

// LBC The CPU uses a set of command strobes to control
// LBC operation of the radio.  Command strobes may be viewed
// LBC as single byte instructions which each start an internal
// LBC sequence in the radio. (Page 185)
// LBC STROBE_TX - In Idle state, Enable TX. (Page 185)
#define STROBE_TX                    0x03   // Strobe commands for the RFST
// LBC STROBE_RX - Enable RX 
#define STROBE_RX                    0x02   // register
// LBC STROBE_CAL - Calibrate Frequency synthesizer and turn it off.
#define STROBE_CAL                   0x01  
// LBC STROBE_IDLE - Enter IDLE state (frequency synthesizer turned off
#define STROBE_IDLE                  0x04

// LBC IRQ_DONE - Packet Received/Transmitted.  (Interrupt pending)
#define IRQ_DONE                     0x10   // The IRQ_DONE bit in the RFIF-
                                            // and RFIM-register
#define DMAARM_CHANNEL0              0x01   // The value to arm the DMA channel 0 in the DMAARM register
#define DMAARM_CHANNEL1              0x02   // The value to arm the DMA channel 1 in the DMAARM register
#define DMAARM_CHANNEL2              0x04   // The value to arm the DMA channel 2 in the DMAARM register

// LBC not referenced(???) #define NUMBER_OF_MODES              2      // Operational mode constants

// LBC - The 5 radio modes we care about
#define RADIO_MODE_TX                0x10
#define RADIO_MODE_UNDEF             0xF0
#define RADIO_MODE_RX                0x20
#define RADIO_MODE_CALIBRATE         0x40
#define RADIO_MODE_IDLE              0x80

// LBC UINT_BARRIER is used to "validate" parameter block
#define UINT_BARRIER                 0xAAAA

/* Some adjustable settings */
//#define PACKET_LENGTH_ED             50      //(17+10*4) // Payload length. Does not include
// LBC PACKET_LENGTH should be 35, (discovered in G+)
#define PACKET_LENGTH                34      //(17+10*4) // Payload length. 
#define PACKET_LENGTH_GW             PACKET_LENGTH
#define PACKET_LENGTH_ED             PACKET_LENGTH
#define PACKET_LENGTH_GW_2           18

#define AES_SIZE                     16

#define RSSI_AVG_WINDOW_SIZE         2  //32     // Size of ring buffer for RSSI
                                             // values to average over (sliding
                                             // window). Max 256
#define DATA_RATE_1_CC2511       50
#define DATA_RATE_2_CC2511       25
#define DATA_RATE_3_CC2511       12
#define DATA_RATE_4_CC2511       2

/*==== MACROS ================================================================*/
/*==== TYPES =================================================================*/
/*==== GLOBALS ================================================================*/
/*====  GLOBAL VARS ==========================================================*/

// This is the text displayed on the LCD for each menu option and their mapping
// to the defined values

// Other global variables
// LBC PACKET_LENGTH + 1 (mistake) + 2 (radio needs)
extern BYTE radioPktBuffer[  PACKET_LENGTH + 3];  // Buffer for packets to send or receive, sized to match the receiver's needs
//extern BYTE radioPktBuff[    PACKET_LENGTH_ED + 3];  // Buffer for packets to send or receive, sized to match the receiver's needs
extern BYTE radioPktBufferRx[PACKET_LENGTH + 3];  //to accomodate maximum possible package
extern BYTE radioPktBufferTx[PACKET_LENGTH + 3];
extern BYTE radioMode;

/*==== FUNCTIONS =============================================================*/

/* See per_test_dma.c for description */
// LBC dmaRadioSetup defined in dma.c
extern void dmaRadioSetup(void);

/* See per_test_radio.c for description */
// LBC - defined in radio.c
extern void radioConfigure(BYTE dataRate/*, UINT32 frequency*/);

extern INT16 convertRssiByte(BYTE RSSI_value);

extern void loadKey( char *key );
extern void loadIV(  char *iv );
//extern void encode( unsigned char size,  char *from, char *to );
//extern void decode( unsigned char size,  char *from, char *to );

//extern void writeTheLatest(BYTE *ptr, UINT16 len);
extern void halFlashDmaTrigger(void);
extern void refreshTheFlash(void);
extern void flashDmaSetup(void);
//extern BOOL readTheLatest(BYTE *ptr, UINT16 len );
extern void setupTxBuffer( BYTE *ptr );

void receive(void);
void idle(void);
#endif  /* PER_TEST_MAIN_H */

// LBC N_of_Ticks are how many areas of interest related to "ticks"
#define N_Of_Ticks  16
// LBC Indices into SchedulerInterface.ticks
#define MppCycle                0
#define AdcMeasurement          1
#define FeedTheDog              2
#define SendData                3
#define ReceiveData             4
#define BypassMode              5
#define Init                    6
// not used
//#define CheckFlag               7
#define OscEvent                7
#define Hop                     8
#define Cycle                   9
#define TurnOn                  10
#define Ms                      11
// not used now
#define OcRamp                  12
#define DelayedPrepareData      13
#define TickWait                14
#define OvStartup               15

//#define InvMppCycle                (N_Of_Ticks-0-1)
//#define InvAdcMeasurement          (N_Of_Ticks-1-1)

// LBC - N_Of_Flags Max areas of interests related to "flags"
#define N_Of_Flags  16

// LBC - Indices into SchedulerInterface.flags
#define IncreaseSleepCnt        0
#define PackageReceived         1
#define DataSent                2
#define PrepareJoin             3
#define PrepareData             4
#define CheckVin                5
#define Write2Flash             6
#define AdcEnabled              7
#define RefreshFlash            8
// make sure FCycle == Cycle, that is used
#define FCycle                  9
#define AltCheckVin            11

// LBC - Workhorse structure, to be placed at 0xF500, 97 bytes in total
typedef struct {
    INT16  ticks[ N_Of_Ticks ]; // 32 bytes starting at 0xF500 (0x20)
    BOOL   flags[ N_Of_Flags ]; // 16 bytes starting at 0xF520 (0x30)

    void ( __near_func *armAdcs)(void); // 2 bytes starting at 0xF530 (0x32)
    void ( __near_func *readNextValue)(void); // 2 bytes starting at 0xF532 (0x34)
    void ( __near_func *flashCallback)(void); // 2 bytes starting at 0xF534 (0x36)
    
    BYTE  inCurCycle;      // 1 byte starting at 0xF536 (0x37)
    BOOL  adcEnabled;      // 1 byte starting at 0xF537 (0x38)
    UINT16 runningTick;    // 2 bytes starting at 0xF538 (0x3A)
    BYTE  radioMode;       // 1 byte starting at 0xF53A (0x3B)
    BYTE  interImageCommunications[32]; // 32 bytes starting at 0xF53B (0x5B)
    INT16 mppCycle;        // 2 bytes starting at 0xF55B (0x5D)  
    INT16 firstAdcReading; // 2 bytes starting at 0xF55D (0x5F)
    UINT16 packetReceived; // 2 bytes starting at 0xF55F (0x61)
    //void ( __near_func *rfCallback)(void);

} SchedulerInterface;  // avoided to save space in code sector    

#define OtherImageWasTried 0 // LBC track if other image tried (failed)
#define JustLoaded         1

// Possible base values for images
#define MainNetValueStandard 50 // "normal base" for main image netValue
//#define MainNetValueFETFailing 60 // "base" when FET failure conditions being recognized
#define MainNetValueFETFailing 10 // "base" when FET failure conditions being recognized
#define MainNetValueFETFailed 70  // "base" when FET failure conditions met


//extern SchedulerInterface __xdata *si;

// LBC declare function reset, definition in utils.c
void reset(void);
// LBC not referenced(???) void nullDelta( void );
// LBC not referenced(???) INT32 getDelta( void );
/******************************************************************************
* @fn  addDF (LBC defined in utils.c)
* @brief       This adds function shift to compensate frequency offset
* Parameters:  frequency shuft
* @return void
******************************************************************************/
void addDF(INT32 dFreq);

#define EXT_GND 3

void substractGround( INT32 *ptr, BYTE from, BYTE to );
void secDelay( void );
void cw( void );

// LBC - next 6 functions declarations are defined in utils.c
void zerofill( BYTE *ptr, BYTE size);
void mymemset( BYTE *buff,        BYTE what,    BYTE counter );
void mymemcpy( BYTE *destination, BYTE *source, BYTE counter );
BOOL mymemcmp( BYTE *destination, BYTE *source, BYTE counter );

INT16 convS(  float t );
UINT16 convU( float t );

extern void clearThePage(BYTE page);
extern void waitForClearThePage(void); 
/*==== END OF FILE ==========================================================*/
