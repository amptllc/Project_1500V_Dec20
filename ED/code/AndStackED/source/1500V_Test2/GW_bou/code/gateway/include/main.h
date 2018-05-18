/*-----------------------------------------------------------------------------
| Purpose:     main header file
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
#define PARTNUM_CC2510               0x81   // Part number for the CC2510

// Define oldest and newest chip revision supported by radio configuration
#define CC2510_MIN_SUPPORTED_VERSION 0x04   // Oldest CC2510 revision supported
#define CC2510_MAX_SUPPORTED_VERSION 0x04   // Newest CC2510 revision supported

#define STROBE_TX                    0x03   // Strobe commands for the RFST
#define STROBE_RX                    0x02   // register
#define STROBE_CAL                   0x01  
#define STROBE_IDLE                  0x04   

#define IRQ_DONE                     0x10   // The IRQ_DONE bit in the RFIF-
                                            // and RFIM-register
#define DMAARM_CHANNEL0              0x01   // The value to arm the DMA channel 0 in the DMAARM register
#define DMAARM_CHANNEL1              0x02   // The value to arm the DMA channel 1 in the DMAARM register
#define DMAARM_CHANNEL2              0x04   // The value to arm the DMA channel 2 in the DMAARM register

#define NUMBER_OF_MODES              4      // Operational mode constants
#define RADIO_MODE_TX                0x10
#define RADIO_MODE_RX                0x20
#define RADIO_MODE_CONFIG            0x40
#define RADIO_MODE_CALIBRATE         0x80
#define RADIO_MODE_UNDEF             0

#define ADC_REF                      0x0D     // Reference

/* Some adjustable settings */
#define PACKET_LENGTH               34 //// Payload length. 

#define AES_SIZE                    16


#define RSSI_AVG_WINDOW_SIZE         2  //32     // Size of ring buffer for RSSI
                                             // values to average over (sliding
                                             // window). Max 256

#define NETWORK_ID_KEY               0x5AA5 // Network ID key that identifies
                                            // transmitter/receiver pair

/* Some NOT SO adjustable settings ** See also LOCALS section if manipulated */
// Preset frequency alternatives
#define NUMBER_OF_FREQUENCIES_CC2510  4
#define FREQUENCY_1_CC2510      2480000     // kHz. NOTE: If you want to alter
#define FREQUENCY_2_CC2510      2460000     // these values you will also have
#define FREQUENCY_3_CC2510      2440000     // to modify the register settings in
#define FREQUENCY_4_CC2510      2420000     // radioConfigure() in
#define FREQUENCY_A_CC2510      2405000

#define NUMBER_OF_FREQUENCIES_CC2511  4
#define FREQUENCY_1_CC2511      2480001     // kHz. NOTE: If you want to alter
#define FREQUENCY_2_CC2511      2460001     // these values you will also have
#define FREQUENCY_3_CC2511      2440001     // to modify the register settings in
#define FREQUENCY_4_CC2511      2420001     // radioConfigure() in
#define FREQUENCY_A_CC2511      2402001
#define FREQUENCY_FCC_CC2511    2410001

#define FREQUENCY_B_CC2511      2301001
#define FREQUENCY_C_CC2511      2580001

// radio.c
// Preset data rate alternatives
#define NUMBER_OF_DATA_RATES_CC2510   3
#define DATA_RATE_1_CC2510       500000     // bps. NOTE: If you alter these
#define DATA_RATE_2_CC2510       250000     // values you will also have to
//#define DATA_RATE_3_CC2510        10000     // modify register settings in
#define DATA_RATE_3_CC2510        12000     // modify register settings in
                                            // radioConfigure() in
                                            // per_test_radio.c
#define NUMBER_OF_DATA_RATES_CC2511   4
#define DATA_RATE_1_CC2511       1     // bps. NOTE: If you alter these
#define DATA_RATE_2_CC2511       2     // values you will also have to
//#define DATA_RATE_3_CC2511        10001     // modify register settings in
#define DATA_RATE_3_CC2511       12     // modify register settings in
                                            // radioConfigure() in
                                            // per_test_radio.c
#define DATA_RATE_4_CC2511       30


#define PACKET_LENGTH_ED    34
#define PACKET_LENGTH_GW_2  18

//#define DATA_RATE_3A                1
//#define DATA_RATE_4                 1

/*==== MACROS ================================================================*/
/*==== TYPES =================================================================*/
/*==== GLOBALS ================================================================*/
/*====  GLOBAL VARS ==========================================================*/

// This is the text displayed on the LCD for each menu option and their mapping
// to the defined values


typedef struct {
   DWORD dteRate;
   BYTE charFormat;
   BYTE parityType;
   BYTE dataBits;
} CDC_LINE_CODING_STRUCTURE;

extern CDC_LINE_CODING_STRUCTURE lineCoding;

extern DMA_DESC dmaConfig[3];                  // Struct for the DMA configuration

//#define MKS_IN_TICK     200
//#define TICKS_IN_SEC    5000
//#define TICKS_IN_MS     5

#define MKS_IN_TICK     500
#define TICKS_IN_SEC    2000
#define TICKS_IN_MS     2

#define UPPER_END       0x7C00
#define UPPER_BEGIN     0x2400
#define UPPER_PAGE_BEGIN    9
#define UPPER_PAGE_END      31

#define EXCHANGE_BUFFER 0xF600

#define N_Of_Ticks  10

#define MeasureAdc              0
#define FeedTheDog              1
#define Hop                     2
#define SendRequest             3
#define Start2Receive           4
#define SetupChannel            5
#define Ms                      6
#define StartMainImage          7
//#define CheckFlash              7
#define TickWait                8
#define PreparePackage          9

#define N_Of_Flags  8

#define PackageReceived         0
#define ParseCommand            1
#define Hop                     2
#define AdjustFrequency         3
#define DataSent                4

typedef struct {
    INT16  ticks[ N_Of_Ticks ];                                                                 //0xF600
    BYTE   flags[ N_Of_Flags ];                                                                 //0xF60A

    void   (__near_func *armAdcs)(       void );                                                //0xF612
    void   (__near_func *readNextValue)( void );                                                //0xF614
    void   (__near_func *bou_loop)(      void );                                                //0xF616
    void   (__near_func *main_loop)(     void );                                                //0xF618

    BOOL   (__near_func *readHexArray)(BYTE *arr, BYTE *ptr, BYTE l);                           //0xF61A
    //BYTE (__near_func *ch2int)    ( BYTE ch );                                        
    BYTE*  (__near_func *rollNW)   ( BYTE *ptr );                                               //0xF61C
    BYTE*  (__near_func *rollW)    ( BYTE *ptr );                                               //0xF61E
    BYTE*  (__near_func *roll2next)( BYTE *ptr );                                               //0xF620
    void   (__near_func *usb)( BYTE *s );                                                       //0xF622
    //BYTE* (__near_func *printHex )( BYTE hex, BYTE *ptr );
    void   (__near_func *mymemset)( BYTE *buff,        BYTE what,    UINT16 counter );          //0xF624
    void   (__near_func *mymemcpy)( BYTE *destination, BYTE *source, BYTE counter );            //0xF626
    BOOL   (__near_func *mymemcmp)( BYTE *destination, BYTE *source, BYTE counter );            //0xF628
    void   (__near_func *refreshTheFlash)(void);                                                //0xF62A
    void   (__near_func *clearThePage)(BYTE page);                                              //0xF62C
    BOOL   (__near_func *initiateTransfer)(BYTE *ptr, BYTE *addr, UINT16 len);                  //0xF62E
    UINT32 (__near_func *strtoul) ( const char * str, char ** endptr, int base );               //0xF630
    void   (__near_func *halWait)(BYTE ms);                                                     //0xF632
    void   (__near_func *clear_inbuff )( void );                                                //0xF634
    
    BOOL  adcEnabled, transfer;                                                                 //0xF636
    UINT16 runningTick, reserved_UINT16;                                                               //0xF638
    BYTE  radioMode;                                                                            //0xF63E
    BYTE  inBuff[128];
    BYTE  *temp;
    UINT16 timeR;
    //BYTE  parking_channels[2], fscal_parking_1[4], fscal_parking_2[4], fscal_parking_3[4];
    UINT32 utc;
    UINT16 ms;
    BYTE   reserved_BYTES[8];
    DMA_DESC *dma_config;
    BYTE *radioPktBuffer;
    BYTE *radioPktBufferRx;
    BYTE *radioPktBufferTx;
} SchedulerInterface;  // avoided to save space in code sector    
extern SchedulerInterface __xdata *exchange_block;

/*==== FUNCTIONS =============================================================*/

/* See per_test_dma.c for description */
extern void dmaRadioSetup(UINT8 mode);

/* See per_test_radio.c for description */
extern void  radioConfigure(BYTE dataRate );
extern INT16 convertRssiByte(BYTE RSSI_value);

extern void loadKey( BYTE *key );
extern void loadIV(  BYTE *iv );
extern void encode(  BYTE *from, BYTE *to );
extern void decode(  BYTE *from, BYTE *to );

extern void handleMyStdUsb(void);
// **************** Process USB class requests with OUT data phase ************************
extern void usbcrHookProcessOut(void);
// **************** Process USB class requests with IN data phase *************************
extern void usbcrHookProcessIn(void);
// ********************************  Unsupported USB hooks ********************************
extern void usbvrHookProcessOut(void);
extern void usbvrHookProcessIn(void);
// ************************  unsupported/unhandled standard requests **********************
extern void usbsrHookSetDescriptor(void);
extern void usbsrHookSynchFrame(void);
extern void usbsrHookClearFeature(void);
extern void usbsrHookSetFeature(void);
extern void usbsrHookModifyGetStatus(BYTE recipient, BYTE index, WORD __xdata *pStatus);
extern void usbsrHookProcessEvent(BYTE event, UINT8 index);
extern void usbirqHookProcessEvents(void);

extern void halFlashDmaTrigger(void);
extern void refreshTheFlash(void);

#endif  /* PER_TEST_MAIN_H */
/*==== END OF FILE ==========================================================*/
