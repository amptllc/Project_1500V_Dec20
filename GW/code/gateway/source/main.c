/*==== DECLARATION CONTROL ===================================================*/
/*==== INCLUDES ==============================================================*/
#include "hal_main.h"
#include "main.h"
#include "usb_cdc.h"

#include "usb_interrupt.h"
#include "rf_usb_library_headers.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>

#define PARKING_CHANNEL          137

#define DONGLE_ADDRESS           0x01
#define NETWORK_ADDRESS0         0x12
#define NETWORK_ADDRESS1         0x13
#define NETWORK_ADDRESS2         0x14

#define USB_RX_TIMEOUT           10
/*
#define BUFFER_SIZE              137
#define MAX_DATA_BYTES           ((BUFFER_SIZE - HEADER_LENGTH) - CRC_LENGTH)
#define MAX_LENGTH_BYTE          (BUFFER_SIZE - 3)
*/
//#define MKS_IN_TICK     40
//#define TICKS_IN_SEC    25000
//#define TICKS_IN_MS     25

#define MKS_IN_TICK     100
#define TICKS_IN_SEC    10000
#define TICKS_IN_MS     10
/*
#define MKS_IN_TICK     250
#define TICKS_IN_SEC    4000
#define TICKS_IN_MS     4
*/
#define NULLOP          0
#define SETREG          0x10
#define RESET           0x20
#define SILENCE         0x30
#define CALIBRATE_R     0x40
#define SET_MAC_CMD     0x50
#define BOOT_CMD        0x60
#define SET_NEXT_KEY    0x70
#define SET_NEXT_IV     0x80
#define SHORT_JOIN      0xF0

#define BROADCAST       0
#define STRINGADDR      1
#define NIDADDR         2
#define MACADDR         3

#define FLOAT_VAL       0
#define BYTE_VAL        (1<<2)
#define SHORT_VAL       (2<<2)
#define LONG_VAL        (3<<2)

// double registers
#define SET_DFK0    1
#define SET_DFK1    2
#define SET_DFK2    3
#define SET_DFK3    4

#define SET_TK0     5
#define SET_TK1     6

#define SET_VIN_TURN_ON     7
#define SET_VIN_SHUT_OFF    8

#define SET_LC00 10
#define SET_LC01 11
#define SET_LC10 12
#define SET_LC11 13 
#define SET_LC20 14
#define SET_LC21 15
#define SET_LC30 16
#define SET_LC31 17
#define SET_LC40 18
#define SET_LC41 19
#define SET_LC50 20
#define SET_LC51 21
#define SET_LC60 22
#define SET_LC61 23

#define SET_VREF0 24
#define SET_VREF1 25
#define SET_VREF2 26

// INT32 registers
#define SET_DF_TOLERANCE    2

// INT16 registers
#define SET_GROUP_ID        1
#define JOIN                2

// BYTE and BOOL registers
// from 0 to 15 - P1 and P2 pins directly - will be used for OPEN, MPP, LED (?)

// MAC bytes
#define MAC0    (16+0)
#define MAC1    (16+1)
#define MAC2    (16+2)
#define MAC3    (16+3)
#define MAC4    (16+4)
#define MAC5    (16+5)

#define CLEAR_THE_PAGE  10
#define FLASH_REFRESH2  11

// Timer channels
#define SET_T3CH0        22
#define SET_T3CH1        23

// Radio Channel
#define SET_CHANNEL      26
// FLASH control
#define FLASH_REFRESH    27
#define FLASH_AVAILABLE  28
#define CW_MODE          29
#define RESTART_MEASUREMENT         30                         
#define DISSOLVE_NETWORK            31
#define REPORT_SCALING              32
#define COEFFICIENTS_AVAILABLE      33

#define RADIO_POWER                 35
#define PRODUCTION                  36
#define ENSURE_WD_RESET             37
#define USE_10_kBod                 38
#define USE_FEC                     39
#define ENABLE_HOPPING              40
#define IS_RELAY                    41
#define IS_500_ALWAYS               42
#define REPEATER_CHANNEL            43
#define REPEATER_POWER              44
#define SEARCH_FOR_COMMUNICATION    45

#define SET_SHOW_STATE              46
#define SetCriticalLevel500         47
#define SetCriticalLevel10          48

#define END_OF_TEST                 49
#define CLEAR_MAX_BUNCH             50
#define SYNCHRONIZE_MPP_CYCLES      51
#define BOOT_PAGE_IMAGE             52
#define SET_START500_CHANNEL        53
#define TRY_OTHER_IMAGE             54
#define CHECK_FLASH                 55
#define SWITCH_SECURITY             56

#define NDEV_MASK   0x3F
#define JOIN_MASK   0x80
#define HOP_MASK    0x40

CDC_LINE_CODING_STRUCTURE lineCoding;

typedef struct {
   // upper nibble - command code / tag, 
   // lower nibble - 
   //      upper 2 bits : value format (byte, int, long, float),
   //      lower 2 bits : address mode (broadcast, string, individual net addr, mac addr)
   BYTE cmd;    
   
   union{
     UINT16 stringAddr;
     UINT16 netId;
     BYTE   mac[6];
     BYTE   arr[6]; 
   } addr;
   
   union{
     BYTE   bval;
     UINT16 ival;
     UINT32 lval;
     double dval;
     BYTE   arr[18]; // 2 for addr and 18 is maximum amount of bytes we can fit in here - used for boot over the air
   } value;
   
   BYTE reg;
   BYTE repCount;
   UINT16 seq, bAddr;
   BOOL useReg, useAddr, useValue;
} CMD;

extern BOOL transfer; 

extern void setup_hopper( BYTE curch );
extern void hop(void);

//void setupTimer3(void);
void setupTimer1(void);
void releaseTheDog(void);

void feedTheDog(void);
void sendRequest(void);
void handleStdUsb(void);
void processEDReport(void);
void start2Receive(void);
void parseCommand(void); 
void calibrate(void);
void usb(char *s);
void addDF(INT32 dFreq);
void computeADCs(BOOL timeout);
void  adjustF2Temperature(INT32 adcT, BOOL timeout);
INT32 abs32(INT32 a);
void armAdcs(void);
void readNextValue(void);
CMD *getCmdPtr(BYTE cmd, BOOL useReg, BOOL useAddr, BOOL useValue);

struct{
    UINT16 barrier;                                                                                         // 2
    UINT16 nDevs;                                                                                           // 2
    char curKey[ AES_SIZE ];                                                                                // 16
    char curIV [ AES_SIZE ];                                                                                // 16
    BYTE channel;                                                                                           // 1
    
    float dFk[4];                                                                                          // 16
    float tk[2];                                                                                           // 8

    INT16  theDelta;                                                                                        // 2
    INT32  iRef;                                                                                            // 4
    INT8   theShift; // 16 in the offset register                                                           // 1
    BYTE   dF_Tolerance;
    BOOL   useADC, use30kbod;
    BYTE   power;
    BOOL   long_packets;
    char   mac[ 6 ];
    BOOL   isRxOnly;
    BOOL   isHopper;
    BYTE   edAddr, gwAddr;
    BOOL   useFEC;
    BYTE   version;
    BOOL   longReport;
    BYTE   maxDevs;
    BOOL   beatSpike;
    BYTE   bootDelay;
    BYTE   parkingChannel, decStep, nSteps;
    UINT32 secondsJoinEnabled;
    BOOL   printSupercycle;
} page = {                                                                                                   // 68
    0xAAAA, 0, 
    { 'A', 'N', 'D', 'S',  0,  'l', 'a', 'r', 'A', 'u', 'g', 'u', 's', 't',  0,   7 },
    { 'A', 'u', 'g', 'u', 's', 't',  0,   7,  'A', 'u', 'g', 'u', 's', 't',  0,   7 },
    
    2,  //should be 0 in production
    
    { -0.000627, 0.0469, 0.253, -123.0 }, 
    { 0.435, 625.0 }, //{ 75500.0, 0.39 }, 
    5,    121000L,    0, 64, 
    TRUE, FALSE, 
    0xED,
    TRUE,
    { 0,0,0,0,0,0 },
    FALSE, FALSE, 
    2,     1,
    FALSE, 0xE2, TRUE,
    16,
    TRUE, 
    2,
    87, 10, 12,
    0, //14L*24L*3600L
    TRUE
};

BYTE dumb0, def_repeat = 4;
BYTE  *pagePtr = (BYTE *)&page;
UINT16 PAGESIZE = ( 2*sizeof( page )+1 ) / 2;

INT16 theTemperature = 0;

UINT16  _sendRequest = 2 * TICKS_IN_SEC, _dogsFeeding =  TICKS_IN_SEC/8, _calibrationDelay = TICKS_IN_MS-1,
        _timeBetweenMeasurements = TICKS_IN_SEC / 100, _adjustFrequency = 5*(UINT16)TICKS_IN_SEC, 
        _adcMeasurement = TICKS_IN_SEC / 100;

UINT16  ticks2SendRequest = 0, ticks2FeedTheDog = 0,   ticks2Start2Receive = 0,  inBuffIdx = 0,     ticks2MsTime = 0, 
        ticks2Send = 0,        runningTicks = 0,       cmdCycle,                 ticks2Measure = 0, ticks2AdjustFrequency = 0, 
        adcCount = 0,          bufferCount = 0,        ticks2AdcMeasurement = 0, cmdCount = 1,      ticks2CheckFlash = 0, 
        ticks2Hop = 0,         ticks2SetUpChannel = 0, lastCycle = 0,            sec2USBWatchDog  = 0;

//UINT32  secondsJoinEnabled = 14L*24L*3600L; // should be 0 in production, 2 weeks
int     lastRssi = 0;
INT32 df;
            
BOOL    time2SendRequest = FALSE, time2FeedTheDog = FALSE, time2Start2Receive = FALSE, needCommandParsing = FALSE, 
        joinEnabled  = FALSE,      echoEnabled = TRUE,      flashDirty = FALSE,         needNewPage = FALSE, 
        computeDelta = FALSE,     time2MeasureAdc = FALSE, adcDataReady = FALSE,       time2AdjustFrequency = FALSE, 
        bufferFilled = FALSE,     dissolveNetwork = FALSE, isCW = FALSE,               secondGone = FALSE,
        setNewChannel = FALSE,    time2CheckFlash = FALSE, time2Hop = FALSE,           time2SetUpChannel = FALSE,
        bunch255      = FALSE,    time2Restart    = FALSE;

BYTE rpbCount = 0, hopCnt = 0, ticks2FullPower = 0, stage = 0;

BYTE parking_channels[4] = {57, 137, 177, 197},
     fscal_parking_1[4], fscal_parking_2[4], fscal_parking_3[4];

//char theKey[ AES_SIZE ] = { 'A', 'N', 'D', 'S',  0,  'l', 'a', 'r', 'A', 'u', 'g', 'u', 's', 't',  0,   7 };
//char theIV [ AES_SIZE ] = { 'A', 'u', 'g', 'u', 's', 't',  0,   7,  'A', 'u', 'g', 'u', 's', 't',  0,   7 };
//UINT32  freqs[5]        = { FREQUENCY_B_CC2511, FREQUENCY_A_CC2511, FREQUENCY_3_CC2511, FREQUENCY_1_CC2511, FREQUENCY_C_CC2511 };

#define N_CHANNELS      3
INT32  adcs[   N_CHANNELS ];
INT16  adcsCounter = 0;
const BYTE  adcChannels[ N_CHANNELS ] = {  
    ADC_REF_AVDD | ADC_12_BIT | ADC_AIN5,    // ext ref         2.5 V
    ADC_REF_AVDD | ADC_12_BIT | ADC_AIN3,    // ext ground
    ADC_REF_AVDD | ADC_12_BIT | ADC_AIN4     // ext Temp
};

#define OUT_BUF_SIZE 512
unsigned char allAdcs[ 160 ]; // 256
unsigned char inBuff [ OUT_BUF_SIZE/8 ]; // 32
unsigned char output_buffer[ OUT_BUF_SIZE ];

INT32   utc = 0;
UINT16   ms  = 0;

//#define DATA_RATE_3A         1               
#define DATA_RATE_4          1

#ifdef DATA_RATE_4
    UINT32  _freq        = FREQUENCY_FCC_CC2511;
    UINT16  _slot        = ( 40 * TICKS_IN_MS );
    UINT16  _loop_delay  = ( 40 * TICKS_IN_MS );
    UINT32  _datarate    = DATA_RATE_4_CC2511;
#endif

#ifdef DATA_RATE_3A
    UINT32  _freq        = FREQUENCY_A_CC2511;
    UINT16  _slot        = ( 40 * TICKS_IN_MS );
    UINT16  _loop_delay  = ( 40 * TICKS_IN_MS );
    UINT32  _datarate    = DATA_RATE_3_CC2511;
#endif

BYTE     maxBunches = 254; // 4096-16 devices
BYTE     bunch    = 0,   nBunches   = 1;

#define CMDBUFMSK 0xF
CMD  cmdBuf[ CMDBUFMSK+1 ];
BYTE cmdIdx0 = 0, cmdIdx1 = 0xFF;

void switchDataRate( BOOL fcc ){
#ifdef RevC
  _freq        = FREQUENCY_A_CC2511;
  _datarate    = DATA_RATE_3_CC2511;
#else
  _freq        = FREQUENCY_FCC_CC2511;
  _datarate    = fcc ? DATA_RATE_4_CC2511 : DATA_RATE_3_CC2511;
#endif  
  //_slot        = fcc ? ( 40 * TICKS_IN_MS ) : ( 45 * TICKS_IN_MS );
  _loop_delay  = ( 40 * TICKS_IN_MS );
}

void _mymemset( BYTE *buff,        BYTE what,    UINT16 counter ){ while( counter-- ) *buff++ = what; }
#define mymemset( a, b, c ) _mymemset( (BYTE *)a, b, c )
void _mymemcpy( BYTE *destination, BYTE *source, BYTE counter ){ while( counter-- ) *destination++ = *source++; }
#define mymemcpy( a, b, c ) _mymemcpy( (BYTE *)a, (BYTE *)b, c )
BOOL _mymemcmp( BYTE *destination, BYTE *source, BYTE counter ){ while( counter-- ) if(*destination++ != *source++) return 1; return 0; }
#define mymemcmp( a, b, c ) _mymemcmp( (BYTE *)a, (BYTE *)b, c )

void configureAdcs( void );
void writePage2Flash(void){
    //page.channel = CHANNR;
    if( page.isRxOnly ) ticks2Start2Receive = TICKS_IN_SEC/8;  else ticks2SendRequest = TICKS_IN_SEC;
    mode = RADIO_MODE_CALIBRATE;
    INT_GLOBAL_ENABLE(FALSE);
    RFST = STROBE_IDLE; 
    RFST = STROBE_CAL; 
    INT_GLOBAL_ENABLE(TRUE);
    writeTheLatest( (BYTE *)&page, PAGESIZE, TRUE /*PAGESIZE, needNewPage*/); 
    ticks2CheckFlash = TICKS_IN_SEC/8;
}

void setupRadio(void){
    RFST = STROBE_IDLE; 
    switchDataRate( page.use30kbod );
    radioConfigure( _datarate, _freq ); 
    if( page.useFEC && page.use30kbod ) MDMCFG1 |= 0x80; else MDMCFG1 &= 0x7F;
    PA_TABLE0 = page.power; CHANNR = page.channel;  
    if( page.isRxOnly ) PKTCTRL1 &= 0xFC; // address check off 
    ADDR = page.gwAddr;
    calibrate();
}
void usb_out(void);

void calibrateParking( void ){
BYTE i, ch = CHANNR;
      INT_GLOBAL_ENABLE(FALSE);
      for( i = 0; i < 4; i++){
           halWait( 1 );
           RFST = STROBE_IDLE; 
           CHANNR = parking_channels[ i ];
           RFST = STROBE_CAL; halWait( 1 );
           fscal_parking_1[i] = FSCAL1;    fscal_parking_2[i] = FSCAL2;     fscal_parking_3[i] = FSCAL3;
      }
      CHANNR = ch; RFST = STROBE_CAL; 
      INT_GLOBAL_ENABLE(TRUE);
}

/*==== PUBLIC FUNCTIONS ======================================================*/
/******************************************************************************
* @fn  main
* @brief      Main function. Triggers setup menus and main loops for both receiver
*             and transmitter. This function supports both CC1110 and CC2510.
* Parameters:
* @param  void
* @return void
******************************************************************************/
void main(void){
   //RF_PACKET __xdata * pTempPacketBuffer;
   UINT16 outCounter, i, j;

   ticks2SendRequest = _sendRequest;
   ticks2FeedTheDog  = _dogsFeeding;
   ticks2AdjustFrequency   = _adjustFrequency;
   
   ticks2MsTime      = TICKS_IN_MS;
 
   P1SEL &= 0xFD;  P1DIR |= 0x02; 
   //P1 &= ~0xE2;
   P1 |= 0x02; //P1 ^= 0x40;
   
   //___________________________________________________________________________________________________________________________
   CLKCON = 0x80;                // Enable XOSC
   while ((SLEEP & 0x40) == 0);  // Wait until XOSC/USB clock has stabalized
   
   //set default line coding.
#ifdef windows
   lineCoding.dteRate    = 115200;
   lineCoding.charFormat = CDC_CHAR_FORMAT_1_STOP_BIT;
   lineCoding.parityType = CDC_PARITY_TYPE_NONE;
   lineCoding.dataBits   = 8;
#endif

#ifndef windows
   lineCoding.dteRate    = 230400;
   lineCoding.charFormat = CDC_CHAR_FORMAT_1_STOP_BIT;
   lineCoding.parityType = CDC_PARITY_TYPE_NONE;
   lineCoding.dataBits   = 8;
#endif

   /*
   lineCoding.dteRate    = 1000000;
   lineCoding.charFormat = CDC_CHAR_FORMAT_1_STOP_BIT;
   lineCoding.parityType = CDC_PARITY_TYPE_NONE;
   lineCoding.dataBits   = 8;
   */
   
   setupTimer1();   
   releaseTheDog();

   //___________________________________________________________________________________________________________________________
   // Choose the crystal oscillator as the system clock
   halPowerClkMgmtSetMainClkSrc(CRYSTAL);

   //isCW = TRUE;

   // Configure interrupt for every received packet
   //___________________________________________________________________________________________________________________________
   // Initialize the USB framework
   usbfwInit();
   // Initialize the USB interrupt handler with bit mask containing all processed USBIRQ events
   usbirqInit(0xFFFF);

   USBFW_SELECT_ENDPOINT(5);
   USBMAXO = 0;    USBMAXI = 0xFF;
   USBCSIH |= USBCSIH_IN_DBL_BUF;
   
   if( !readTheLatest((BYTE *)&page, PAGESIZE ) ){ flashDirty = TRUE; needNewPage = TRUE; }

   setup_hopper( page.channel );
   FSCTRL0 = (INT8)page.theShift;
   
   setupRadio();

   calibrateParking();
   nBunches = 1 + page.nDevs / page.maxDevs;

   HAL_INT_ENABLE(INUM_RF,  INT_ON);    // Enable RF general interrupt
   HAL_INT_ENABLE(INUM_ADC, INT_ON);    // enable ADC interrupt
   HAL_INT_ENABLE(INUM_T1,  INT_ON);    // enable Timer1 interrupt 

   RFIM    = IRQ_DONE;
   INT_GLOBAL_ENABLE(TRUE);
   
   loadKey( page.curKey );

   // configugre ADC pins
   configureAdcs( );
   ticks2AdcMeasurement = 50;
   //start2Receive();   
    
   //___________________________________________________________________________________________________________________________
   while (TRUE) {
      if (time2FeedTheDog)   {  feedTheDog();       time2FeedTheDog    = FALSE; }
      if ( time2Start2Receive ){  start2Receive();    time2Start2Receive = FALSE; }
      if( isCW ){ // Just transmit - without changing frequency
        //if( page.beatSpike ) PA_TABLE0 = 0;
        INT_GLOBAL_ENABLE(FALSE);
        PA_TABLE0     = page.power;
        RFST = STROBE_TX;
        while( !RFTXRXIF ) asm("NOP");
        RFD = 0x55;
        for( i = 0; i<0x55; i++){
            while( !RFTXRXIF ) asm("NOP");
            RFD = 0x55;
        }
        feedTheDog();
        /* Continuos sending of preambula
        dmaRadioSetup( mode = RADIO_MODE_TX );    
        // Send the packet
            DMAARM = DMAARM_CHANNEL1;       // Arm DMA channel 1
            RFST = STROBE_TX;               // Switch radio to TX
            while( TRUE ) feedTheDog();
        */
      }else{ // Do the real work
          if( secondGone ) { 
              utc++; 
              if( page.secondsJoinEnabled > 0 ){
                  page.secondsJoinEnabled--;
                  if(page.secondsJoinEnabled > 0) joinEnabled = TRUE; else joinEnabled = FALSE;
              }
              secondGone = FALSE;
          }
          INT_GLOBAL_ENABLE( FALSE );
              handleMyStdUsb();
              USBFW_SELECT_ENDPOINT(5);
              while ((USBCSOL & USBCSOL_OUTPKT_RDY)){
                   outCounter = 256*USBCNTH; outCounter += USBCNTL;
                   if( outCounter == 0 ){ /*USBCSIL |= USBCSIL_INPKT_RDY;*/ break; }
                   usbfwReadFifo(&USBF5, outCounter, allAdcs);
                   USBCSOL &= ~USBCSOL_OUTPKT_RDY;
                   for(i = inBuffIdx, j = 0; j<outCounter; inBuffIdx++, j++ ){
                       inBuff[ inBuffIdx ] = allAdcs[ j ];
                       if(      inBuff[ inBuffIdx ] == '\r' ){ inBuff[ ++inBuffIdx ] = '\n'; inBuff[ ++inBuffIdx ] = 0; needCommandParsing = TRUE; break; }
                       else if( inBuff[ inBuffIdx ] == '\n' ){ inBuff[ ++inBuffIdx ] = '\r'; inBuff[ ++inBuffIdx ] = 0; needCommandParsing = TRUE; break; }
                   }
                   // is it OK to ignore the rest of allAdcs when if( j < outCounter ) ???
                   allAdcs[ outCounter ] = 0;
                   if( echoEnabled  && ( ( USBCSIL & 1 ) == 0 ) ) usb( (char *)allAdcs );
                   /*
                   {
                       usbfwWriteFifo(&USBF5, inBuffIdx-i, inBuff+i );
                       USBCSIL |= USBCSIL_INPKT_RDY;
                   }
                   */
              }
              usb_out();
          INT_GLOBAL_ENABLE( TRUE );
    
          if ( !page.isRxOnly && time2SendRequest )  {  sendRequest();      time2SendRequest   = FALSE; }
          if ( page.isRxOnly || ( ticks2SendRequest > 80 ) ){
              if ( pktRcvdFlag )   {  processEDReport();  pktRcvdFlag        = FALSE; }
          }
          if ( time2SetUpChannel && page.isRxOnly ){ time2SetUpChannel = FALSE; CHANNR = page.channel; calibrate(); }
          if ( needCommandParsing ){  parseCommand();     needCommandParsing = FALSE; }
          if ( time2CheckFlash ){ 
              time2CheckFlash = FALSE; 
              if( !readTheLatest((BYTE *)&page, PAGESIZE ) ) writePage2Flash();
          }
          if( page.useADC && time2MeasureAdc  ) { armAdcs();           time2MeasureAdc  = FALSE; }
          if( page.isRxOnly && time2Hop ){        
                hop();    time2Hop = FALSE; 
                if( hopCnt-- ) ticks2Hop = lastCycle - (2 * TICKS_IN_MS); 
          }
          if( time2Restart ){ INT_GLOBAL_ENABLE( FALSE ); while( TRUE ) ; }
      }  
   }
}
/*==== PRIVATE FUNCTIONS =====================================================*/
/*==== INTERRUPT SERVICE ROUTINES ============================================*/
/******************************************************************************
* @fn  rf_IRQ
* @brief    The only interrupt flag which throws this interrupt is the IRQ_DONE interrupt.
*           So this is the code which runs after a packet has been received or  transmitted.
* Parameters:
* @param  void
* @return void
******************************************************************************/
static BYTE stored_channel = 0, stored_fscal_1, stored_fscal_2, stored_fscal_3;
#pragma vector=RF_VECTOR
__interrupt void rf_IRQ(void) {
    INT_GLOBAL_ENABLE(FALSE);
    if (mode == RADIO_MODE_RX){   
         pktRcvdFlag = TRUE;
         DMAARM |= 0x80 | DMAARM_CHANNEL0;
    }else if( mode == RADIO_MODE_TX ){       
         if( page.beatSpike ){  
         BYTE ch = CHANNR;
            stored_fscal_1 = FSCAL1;   stored_fscal_2 = FSCAL2;     stored_fscal_3 = FSCAL3;
            if(CHANNR>page.parkingChannel) CHANNR = ch - ( 7 + (ch%7) );  else CHANNR = ch + (7-ch%7); 
            stage = page.nSteps; 
         }
         ticks2FullPower = 2; 
         DMAARM |= 0x80 | DMAARM_CHANNEL1;
         ticks2Start2Receive = 4*TICKS_IN_MS;
    }
    mode = RADIO_MODE_UNDEF;
    RFIF &= ~IRQ_DONE;        // Tx/Rx completed, clear interrupt flag
    S1CON &= ~0x03;           // Clear the general RFIF interrupt registers
    INT_GLOBAL_ENABLE(TRUE);
}
/******************************************************************************
* @fn  T3_IRQ
* @brief    Timer interrupt - the base of the scheduler. Counts down all the counters
*           and sets appropriate flags.
* Parameters:
* @param  void
* @return void
******************************************************************************/
//#pragma vector=T3_VECTOR
//__interrupt void t3_IRQ(void) {
#define CHECK(counter, flag)    if( counter && (--counter==0)) flag = TRUE;
#pragma vector=T1_VECTOR
__interrupt void t1_IRQ(void) {
    TIMIF &= ~IRQ_DONE;     // clear interrupt flag
    CHECK( ticks2Hop,               time2Hop )
    CHECK( ticks2FeedTheDog,        time2FeedTheDog )
    CHECK( ticks2SendRequest,       time2SendRequest )
    CHECK( ticks2Start2Receive,     time2Start2Receive )
    CHECK( ticks2AdcMeasurement,    time2MeasureAdc )
    CHECK( ticks2CheckFlash,        time2CheckFlash )
    CHECK( ticks2SetUpChannel,      time2SetUpChannel )
    runningTicks++;
    
    if( ticks2FullPower && (--ticks2FullPower==0) ){
          if( !page.beatSpike ){ 
                RFST = STROBE_IDLE; 
          }else if( stage > 6 ){ 
               stage--;  
               if( CHANNR > parking_channels[ 3 ] ) CHANNR = CHANNR - page.decStep;  else  stage = 5;  
               ticks2FullPower = 1; 
          }else if( stage > 2 ){
              while( CHANNR < parking_channels[ stage - 3 ] && stage > 2 ) stage--;
              if( CHANNR > parking_channels[ stage - 3 ] && stage > 2 ){
                  CHANNR = parking_channels[ stage - 3 ];  
                  FSCAL1 = fscal_parking_1[stage-3];  FSCAL2 = fscal_parking_2[stage-3];  FSCAL3 = fscal_parking_3[stage-3];
                  stage--;
              }
              ticks2FullPower = 1;
          }
          else if( stage == 2 ){ stage--; RFST = STROBE_IDLE;                   ticks2FullPower = 1; }
          else if( stage == 1 ){ 
              stage--;
              PA_TABLE0 = page.power; 
              CHANNR = stored_channel;   FSCAL1 = stored_fscal_1;   FSCAL2 = stored_fscal_2;     FSCAL3 = stored_fscal_3;
          } 
    }

    if( ticks2MsTime && (--ticks2MsTime==0) ){           
        ms ++; ticks2MsTime = TICKS_IN_MS;  
        if( ms == 1000 ){ 
            ms = 0; secondGone = TRUE; 
            CHECK( sec2USBWatchDog,       time2Restart )
        }
    }
}
//#pragma vector=ADC_VECTOR 
//__interrupt void adc_IRQ(void) {
//    ADCIF &= ~IRQ_DONE;     // clear interrupt flag
//    adcDataReady = TRUE;
//}
#pragma vector=ADC_VECTOR 
__interrupt void adc_IRQ(void) {
    INT_GLOBAL_ENABLE(FALSE);
        ADCIF &= ~IRQ_DONE;     // clear interrupt flag
        readNextValue();
    INT_GLOBAL_ENABLE(TRUE);
}
//______________________________________________________________________________________________________________________________
/******************************************************************************
* Set up Timer 1 - here it is 24 MHz, sys tick is 100 mks, prescaler = 8, modulo opertaion with counter = 300, generate interrupt
******************************************************************************/
void setupTimer1(void){
    SET_WORD(T1CC0H, T1CC0L, 300 - 1);
//    SET_WORD(T1CC0H, T1CC0L, 750 - 1);
    //        prescaler = 8      modulo mode
    T1CTL   = 0x04             | 0x02;
    //        ie
    T1CCTL0 = 0x44;
}
/******************************************************************************
* Initialize the Watchdog
******************************************************************************/
void releaseTheDog(void){ WDCTL = 0x8; }
/******************************************************************************
* Computes size of the command
******************************************************************************/
BYTE cmdSize(CMD *ptr){
BYTE size = 3; //1+2, cmd + seq;
    if( ( ptr->cmd & 0xF0 ) == SHORT_JOIN ) return 8;

    if( ptr->useAddr )  switch( ptr->cmd & 0x3 ){
        case BROADCAST:             break; // broadcast
        case STRINGADDR: size += 2; break; // string
        case NIDADDR:    size += 2; break; // netId
        case MACADDR:    size += 6; break; // mac 
    }
    switch( ptr->cmd & 0xF0 ){
        case SET_MAC_CMD:   return size + 6;
        case BOOT_CMD:      return size + 2 + 1 + ptr->reg; // reg + bAddr + bytes
        case SET_NEXT_KEY:  case SET_NEXT_IV: return size + 16;
        default:
            if( ptr->useReg ) size++;
            if( ptr->useValue ) switch( ptr->cmd & 0xC ){
                case FLOAT_VAL:  size += 4; break;
                case BYTE_VAL:   size += 1; break;
                case SHORT_VAL:  size += 2; break;
                case LONG_VAL:   size += 4; break;
            }
        break;
    }
    return size + 1;
}
/*****************************************************************************/
#define FROMHEX(ch) ( (ch<='9')&&(ch>='0') ? (ch - '0') : ( (ch<='F')&&(ch>='A') ? ((ch-'A')+10) : ((ch<='F')&&(ch>='A') ? ((ch-'A')+10) : -1 ) ) ) 
#define HEX(h)      ( h<10 ? (h + '0') : ((h-10)+'A') )
//int len(char *ptr){  int cnt = 0; while( *ptr++ ) cnt++; return cnt; }
//____________________________________________________________________________
static char u2abuf[32] = {0};
char* utoa(UINT16 val, int base, char *ptr){
BYTE   i = 30;
    mymemset(u2abuf, 0, 32);
    if( val == 0 ) { *ptr++ = '0'; return ptr; }
	for(; val && i ; --i, val /= base) u2abuf[i] = "0123456789ABCDEF"[val % base];
    strcpy( ptr, u2abuf+i+1 ); 
	return ptr + (30-i);
}
//____________________________________________________________________________
inline char* itoa(INT16 val, char *ptr){
  if( val < 0 ){ *ptr++ = '-'; val = -val; }
  return utoa( (UINT16)val, 10, ptr );
}
//____________________________________________________________________________
char* ultoa(UINT32 val, int base, char *ptr){
BYTE   i = 30;
    mymemset(u2abuf, 0, 32);
    if( val == 0 ) { *ptr++ = '0'; return ptr; }
	for(; val && i ; --i, val /= base) u2abuf[i] = "0123456789ABCDEF"[val % base];
    strcpy( ptr, u2abuf+i+1 ); 
	return ptr + (30-i);
}
//____________________________________________________________________________
inline char* ltoa(INT32 val, char *ptr){
  if( val < 0 ){ *ptr++ = '-'; val = -val; }
  return ultoa( (UINT32)val, 10, ptr );
}
//____________________________________________________________________________
inline char *ftoa(float c, char *ptr){
UINT32 d;
    if( c < 0 ){ *ptr++ = '-'; c = -c; }
    d = (UINT32) c;  ptr = ultoa( d, 10, ptr );
    c -= floor(c);
    *ptr++ = '.';
    if( c <= FLT_MIN ) *ptr++ = '0'; 
    else{
       c *= 10.0;
       while( c < 1 && c > FLT_MIN ){ *ptr++ = '0'; c *= 10.0; }
       d = (UINT32)(c*10000000);
       ptr = ultoa( d, 10, ptr );
    }
    return ptr;
}
//____________________________________________________________________________
INT16 _pktGetAdc16( BYTE *rpb,  unsigned char idx ){
INT16 val = 0; 
    mymemcpy( &val, rpb + 12 + 2*idx, 2 );
    return val;
}
//____________________________________________________________________________
char *printMac( char *ptr, char *mac ){
register BYTE idx, h;
    for( idx = 0; idx < 6; idx ++ ){
      h = ( (*mac) >> 4 ) & 0xF;     *ptr++ = HEX(h);
      h = (*mac++) & 0xF;            *ptr++ = HEX(h);
    }               
    return ptr;
}
/*****************************************************************************/
void qsort_commands( int l, int r ){
static CMD w;
CMD *x;
int i, j; 
    i = l; j = r; x = &cmdBuf[ (i+j)>>1 ];
    do{
      while( cmdBuf[i].seq < x->seq ) i++;
      while( cmdBuf[j].seq > x->seq ) j--;
      if( i <= j ){
            mymemcpy( &w,             &cmdBuf[ i ], sizeof(CMD) );
            mymemcpy( &cmdBuf[i],     &cmdBuf[ j ], sizeof(CMD) );
            mymemcpy( &cmdBuf[ j ],   &w,           sizeof(CMD) );
            i++; j--;
      }
    }while( i <= j );
    if( l < j ) qsort_commands( l, j );
    if( i < r ) qsort_commands( i, r );
}
void sort_commands( void ){ 
BYTE j, lim = CMDBUFMSK;
    for( j = 0; j <= lim; j++)
        if( cmdBuf[ j ].seq == 0xFFFF ){ lim = j; break; }
        else if( (cmdBuf[ j ].cmd == NULLOP) || (cmdBuf[ j ].repCount == 0) ) cmdBuf[ j ].seq = 0xFFFF;
    qsort_commands( 0, lim ); 
}
/*****************************************************************************/
static BYTE newChannel = 0xFF;  
static UINT16 curBunch = 0;
INT8 histCounter = -1;
/******************************************************************************
* Send the request for data to EDs
******************************************************************************/
void sendRequest(void){  
BYTE i, j;//, tmp;
BYTE *ptr; 
BYTE *rpb;
BYTE devsInBunch = 0;
UINT16 freeBytesCount = page.long_packets ? (PACKET_LENGTH_GW - 8 - 2) : (PACKET_LENGTH_GW_2 - 8 - 2) ; //, lastSend = 0;
static BYTE vvv = SETREG | MACADDR | SHORT_VAL;
UINT32 curUTC;
UINT16 curMS;

//UINT16 t0 = runningTicks;

    if( setNewChannel && CHANNR != newChannel){
        CHANNR = newChannel;  
        page.channel = CHANNR;
        setup_hopper( page.channel );
        flashDirty = TRUE;
        newChannel = 0xFF;  
        setNewChannel = FALSE;
        time2Hop = FALSE;
    }
    
    //if( page.nDevs > (bunch+1)*maxDevs ) devsInBunch = maxDevs; else devsInBunch = page.nDevs % maxDevs;

    rpb = radioPktBuffer[rpbCount]; rpbCount = ( rpbCount + 1 ) & 3;
    mymemset( rpb, 0, PACKET_LENGTH_GW );

    if( bunch255 ){
        devsInBunch = page.bootDelay;
        rpb[0] = curBunch = 0xFF;
    }else{
        devsInBunch = page.maxDevs;
        rpb[0]   = bunch; curBunch = bunch;
    }
    cmdCycle = ticks2SendRequest = _slot * ( devsInBunch + (joinEnabled?4:1) ) + _loop_delay;    
    
    if( page.isHopper ) ticks2Hop = ticks2SendRequest - 3;
    
    if( joinEnabled )   rpb[1] = devsInBunch; else rpb[1] = 0x80 | devsInBunch;
    if( page.isHopper ) rpb[1] |= 0x40;       else rpb[1] &= 0xBF;

    curUTC = utc; curMS = ms & 0x3FF;
    mymemcpy( rpb+2, (BYTE *)&utc, 4 ); mymemcpy( rpb+6, (BYTE *)&ms, 2 );

    sort_commands();

    // package the commands from cmd buffer into radio buffer   -   command tag _ addr _ reg _ value
    ptr = rpb + 8;
    for( j = 0; freeBytesCount  && (j <= CMDBUFMSK) ; j++){
          // if there were errors in parsing of the command - skip it
          if( cmdBuf[ j ].cmd == NULLOP ) break;
          
          if( ( ( cmdBuf[ j ].cmd & 0xF0 ) == SET_MAC_CMD ) && ( ( cmdBuf[ j ].cmd & 0x3 ) != MACADDR  && ( cmdBuf[ j ].cmd & 0x3 ) != NIDADDR ) )
            { cmdBuf[ j ].cmd = NULLOP;  cmdBuf[ j ].repCount = 0; continue; }

          // check if we still have size
          i = cmdSize( &cmdBuf[ j ] );                     // or break ???
          if( i <= freeBytesCount ) freeBytesCount -= i;  else   break; //continue; // command too big to fit in buffer !!!
          
          // pack the command into buffer
          *ptr++ = cmdBuf[ j ].cmd;
          if( (cmdBuf[ j ].cmd & 0xF0) == SHORT_JOIN ){ // short Join network command
              *ptr++ = cmdBuf[ j ].reg; 
              mymemcpy( ptr, cmdBuf[ j ].addr.arr, 6 ); 
          }else{
              mymemcpy( ptr, (BYTE *)&cmdBuf[j].seq, 2 );  ptr+= 2; 
              if( (cmdBuf[ j ].cmd == vvv) && (cmdBuf[ j ].cmd == JOIN) ) cmdBuf[j].seq = cmdCount++;
              
              // pack the address into buffer
              if( cmdBuf[ j ].useAddr ) switch( cmdBuf[ j ].cmd & 0x3 ){
                  case BROADCAST:             break; // broadcast
                  case STRINGADDR: case NIDADDR:     // string or netid
                      mymemcpy( ptr, cmdBuf[ j ].addr.arr, 2 ); ptr+=2; break;
                  case MACADDR:                      // mac 
                      mymemcpy( ptr, cmdBuf[ j ].addr.arr, 6 ); ptr+=6; break; 
              }
              // pack the reg into buffer
              if( cmdBuf[ j ].useReg )   *ptr++ = cmdBuf[ j ].reg; 
              // pack the value into buffer
              switch( cmdBuf[ j ].cmd & 0xF0 ){
                  case SET_MAC_CMD: mymemcpy( ptr, cmdBuf[ j ].value.arr, 6 );  ptr+=6; break;
                  case BOOT_CMD:    
                      mymemcpy( ptr, &cmdBuf[ j ].bAddr, 2 );                    ptr+=2;
                      mymemcpy( ptr, cmdBuf[ j ].value.arr,  cmdBuf[ j ].reg );  ptr+=cmdBuf[ j ].reg;
                  break;
                  case SET_NEXT_KEY:  case SET_NEXT_IV:
                      mymemcpy( ptr, cmdBuf[ j ].value.arr,  16 );  ptr+=16;
                  break;  
                  default: 
                      if( cmdBuf[ j ].useValue ) switch( cmdBuf[ j ].cmd & 0xC ){
                          case BYTE_VAL:  *ptr++ =  cmdBuf[ j ].value.arr[0];                 break;
                          case SHORT_VAL: mymemcpy( ptr, cmdBuf[ j ].value.arr, 2 );  ptr+=2;   break;
                          case LONG_VAL:  mymemcpy( ptr, cmdBuf[ j ].value.arr, 4 );  ptr+=4;   break;
                          case FLOAT_VAL: mymemcpy( ptr, cmdBuf[ j ].value.arr, 4 );  ptr+=4;   break; 
                      }
                  break;
              }
          }
          if( cmdBuf[ j ].repCount > 0 )            cmdBuf[ j ].repCount--;
          if( ( ( cmdBuf[ j ].cmd & 0xF0 )         == SETREG ) && 
                      (   cmdBuf[ j ].reg          == DISSOLVE_NETWORK )   &&
                      ( ( cmdBuf[ j ].cmd & 0xC )  == BYTE_VAL  )
              ){
                    if( cmdBuf[ j ].repCount == 0 ) dissolveNetwork= TRUE; else break;
              }          
          if( cmdBuf[ j ].repCount == 0 ){
              if( ( ( cmdBuf[ j ].cmd & 0xF0 ) == SETREG )      && 
                  ( ( cmdBuf[ j ].cmd & 0x03 ) == BROADCAST )   &&
                  (   cmdBuf[ j ].reg          == SET_CHANNEL ) &&
                  ( ( cmdBuf[ j ].cmd & 0xC )  == BYTE_VAL  )
              ){
                    newChannel = cmdBuf[ j ].value.bval;
                    setNewChannel = TRUE;
              }
              mymemset( &cmdBuf[ j ], 0, sizeof( CMD ) );
              // increment the cmdIdx1
          }
          break; // one command at a time ...
    }

    if( page.isHopper && time2Hop ){
        char *ptr;
        hop();    time2Hop = FALSE;
        if( page.longReport ){
            ptr = (char *)allAdcs;
            strcpy( ptr, "\nhopping to " ); ptr += 12;
            ptr = utoa( CHANNR,  10, (char *)ptr );  *ptr = 0;
            usb( (char *)allAdcs );
        }
    }
    stored_channel = CHANNR;
    RFST          = 4; 
    PA_TABLE0     = page.power;

    dmaRadioSetup( mode = RADIO_MODE_TX );    

    loadIV( page.curIV );  
    if( page.long_packets && ( freeBytesCount < 16 ) )   radioPktBufferTx[0] = PACKET_LENGTH_GW;                 // Length byte
    else                                                 radioPktBufferTx[0] = PACKET_LENGTH_GW_2;               // Length byte
    
    encode( radioPktBufferTx[0]-2, (char *)rpb, (char *)(radioPktBufferTx+2) ); 
    radioPktBufferTx[1] = page.edAddr;                             // ED address

    if( page.printSupercycle && (curBunch == 0) ){
        ptr = allAdcs; 
        strcpy( (char *)ptr, "=> MAC:\t" );           ptr+=8;
        ptr = (BYTE *)printMac( (char *)ptr, page.mac ); 
        strcpy( (char *)ptr, "\tCh:\t" );             ptr+=5;
        ptr = (BYTE *)utoa( CHANNR, 10, (char *)ptr ); 
        strcpy( (char *)ptr, "\tT:\t" );              ptr+=4;
        ptr = (BYTE *)itoa( theTemperature, (char *)ptr );    
        strcpy( (char *)ptr, "\tUTC:\t" );            ptr+=6;
        ptr = (BYTE *)ultoa( curUTC, 10, (char *)ptr );    
        strcpy( (char *)ptr, "\tms:\t" );             ptr+=5;
        ptr = (BYTE *)itoa( curMS, (char *)ptr );
        *ptr++ = '\n'; *ptr++ = 0;
        usb( (char *)allAdcs );
    }
    
    // Send the packet
    INT_GLOBAL_ENABLE(FALSE);
        DMAARM = DMAARM_CHANNEL1;       // Arm DMA channel 1
        RFST = STROBE_TX;               // Switch radio to TX
        time2SendRequest = FALSE;
    INT_GLOBAL_ENABLE(TRUE);
    
    if( dissolveNetwork ){
        dissolveNetwork = FALSE;
        flashDirty = FALSE; 
        writeTheLatest( (BYTE *)&page, PAGESIZE, needNewPage); 
        ticks2CheckFlash = TICKS_IN_SEC/8;
        needNewPage = FALSE; 
        histCounter = -1;
    }
    if( ++bunch >= nBunches ) bunch = 0;
    /*
    t0 = (t0<=runningTicks)?(runningTicks-t0):(65535-t0+runningTicks);
    ptr = allAdcs; 
    ptr = (BYTE *)utoa(  t0, 10, (char *)ptr ); *ptr++ = '';  *ptr++ = '\n'; *ptr++ = 0;
    usb( (char *)allAdcs );
    */
}
/******************************************************************************
* Work with watchdogs and LED's
******************************************************************************/
void feedTheDog(void){  
    WDCTL = 0xA8;       WDCTL = 0x58;   // feed the dog
    ticks2FeedTheDog = _dogsFeeding;    // manage flags
}
/******************************************************************************
* Start to receive data
******************************************************************************/
void start2Receive(void){  
    PKTLEN   = PACKET_LENGTH_ED;            // Packet length.
    dmaRadioSetup( mode = RADIO_MODE_RX );  // Set up the DMA to move packet data from radio to buffer
    DMAARM = DMAARM_CHANNEL0;               // Arm DMA channel 0
    RFST   = STROBE_RX;                     // Switch radio to RX
}
/******************************************************************************
* Receive an Report from End Device, format it into the string and send this string through the USB.
******************************************************************************/
static UINT16 free_start = 0, free_finish = OUT_BUF_SIZE; 
#define OUT_SIZE 64
void usb_out( void ){
UINT16 rest = OUT_SIZE, curl = 0;
    if( ( free_finish != OUT_BUF_SIZE ) && (0 == (USBCSIL&1)) ){
        USBFW_SELECT_ENDPOINT(5);
        if( free_finish < free_start ){
            curl = free_start - free_finish;
            if( curl > OUT_SIZE ) curl = OUT_SIZE;
            usbfwWriteFifo(&USBF5, curl, &output_buffer[ free_finish ] );
            free_finish += curl;
        }else{
            curl = OUT_BUF_SIZE - free_finish;
            if( curl > OUT_SIZE ) curl = OUT_SIZE;
            usbfwWriteFifo(&USBF5, curl, &output_buffer[ free_finish ] );
            free_finish += curl; if( free_finish >= OUT_BUF_SIZE ) free_finish = 0; 
            rest        -= curl;
            if( rest ){
                curl = free_start - free_finish;
                if( curl > rest ) curl = rest;
                usbfwWriteFifo(&USBF5, curl, &output_buffer[ free_finish ] );
                free_finish += curl;
            }
        }
        USBCSIL |= USBCSIL_INPKT_RDY;
        if( free_start == free_finish ){ free_start = 0; free_finish = OUT_BUF_SIZE; }
    }
}
void usb( char *s ){
UINT16 idx;
char *s1 = s;
    while( *s1 ) { if( *s1 == '\n' ) *s1 = '\r'; s1++; }
    INT_ENABLE(INUM_P2INT, INT_OFF);
        idx = free_start;
        while( *s && ( 
                       ( idx < free_finish ) || 
                       ( ( free_finish <= free_start ) && ( free_start <= idx ) && ( idx < OUT_BUF_SIZE ) ) 
                     ) 
        ){ 
            output_buffer[ idx++ ] = *s;           if( *s == '\r' ) *s = '\n'; else s++; 
            if( idx >= OUT_BUF_SIZE ) idx = 0; 
        }
        free_start = idx; if( free_finish == OUT_BUF_SIZE ) free_finish = 0;
    INT_ENABLE(INUM_P2INT, INT_ON);
    usb_out();
}
char * printHex( BYTE hex, char *ptr ) { *ptr++ = HEX( ((hex>>4)&0xF) );  *ptr++ = HEX( (hex&0xF) ); return ptr; }
/******************************************************************************
* Receive an Report from End Device, format it into the string and send this string through the USB.
******************************************************************************/
#define NHIST 24
BYTE idHistory[10*NHIST];
BYTE nullmac[6]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
BYTE histCnt = 0;
void processEDReport(void){
signed char off = 0, rssi ;
UINT16 ticksLeft = 0;
CMD *cmdPtr;
char *ptr;
BYTE *rpb;
BYTE *nPtr = NULL;
BYTE idx, i = 0, j, h, plen;//, i;
static UINT32 pktUtc;
static UINT16 pktMs;
static BYTE vvv = SETREG | MACADDR | SHORT_VAL;
    if( histCounter == -1 ){ histCounter = 0; mymemset( idHistory, 0xFF, 10*NHIST ); }
    ticks2Start2Receive = 1;
    // Check if the received packet is a valid PER test packet
    plen = idx = radioPktBufferRx[0];
    if( (( idx == PACKET_LENGTH_ED) || (idx == PACKET_LENGTH_GW_2) ) && 
        (radioPktBufferRx[idx + 2] & 0x80)  
    ) { 
        if( radioPktBufferRx[1] == page.gwAddr ) {
            output = TRUE;   i = FALSE; 
        }else if( page.isRxOnly && ( radioPktBufferRx[1] == page.edAddr ) ) {
            output = TRUE;            i = TRUE;
        }
        rpb = radioPktBuffer[rpbCount]; rpbCount = ( rpbCount + 1 ) & 3;
        loadIV( page.curIV );  decode( idx-2, (char *)(radioPktBufferRx+2), (char *)rpb );
        rssi = radioPktBufferRx[ idx+1 ];
        lastRssi = convertRssiByte( rssi );
        radioPktBufferRx[ idx + 2 ] = 0;
        radioPktBufferRx[0] = 0;    radioPktBufferRx[1] = 0;
        off = (INT8)FREQEST;
    }else{
        ptr = (char *)allAdcs;                                    *ptr++ = '\n';
        ptr = utoa(  radioPktBufferRx[ 1 ], 10, ptr );            *ptr++ = 0;
        usb( (char *)allAdcs );
    }
    // We don't need our packet buffer anymore, prepare for the next packet
    DMAARM = DMAARM_CHANNEL0;
    RFST   = STROBE_RX;               

    ticksLeft = ticks2SendRequest;
    if (output) {
        P1 ^= 2;       
        output = FALSE;
        if( page.isRxOnly && i ){ // another gateway is talking
        BYTE ndev; 
            curBunch = rpb[0]; ndev = rpb[1]; 
            lastCycle = _slot * ( (ndev & NDEV_MASK) + ((ndev & JOIN_MASK)?1:4) ) + _loop_delay;
            
            if( ndev & HOP_MASK ){ ticks2Hop = lastCycle - 2*_slot - 2 * TICKS_IN_MS; ticks2SetUpChannel = 10 * lastCycle; hopCnt = 10; }
            else                 { ticks2SetUpChannel = 0;   ticks2Hop = 0; }
            if( page.longReport ){
                ptr = (char *)allAdcs; 
                strcpy( ptr, "GW\toffset " );   ptr+=10;
                ptr = itoa( off, ptr );         *ptr++ = '\t';
                strcpy( ptr, "rssi " );         ptr+=5;
                ptr = itoa(  lastRssi, ptr );   *ptr++ = ' ';  *ptr++ = 'd'; *ptr++ = 'B';  *ptr++ = '\t';
                strcpy( ptr, "channel " );      ptr+=8;
                ptr = itoa(  CHANNR, ptr );     *ptr++ = '\n'; *ptr++ = 0;
                usb( (char *)allAdcs );
            }
         }else if( (plen == PACKET_LENGTH_GW_2) && joinEnabled && (rpb[0] == 0xFF) && (rpb[1] == 0xFF) && (rpb[2] == 0xFF) && (rpb[3] == 0xFF)){ // join request
                ///UINT16 t0 = runningTicks;
                UINT16 newBunch, newId, i = 0;
                BOOL found = FALSE;
                BYTE *hPtr = idHistory;
                
                cmdPtr = cmdBuf;
                rpb += 10;                

                for( j = 0; j <= CMDBUFMSK; j++, cmdPtr++)
                  if( mymemcmp( cmdPtr->addr.mac, rpb, 6 ) == 0  &&
                      ( ( cmdPtr->cmd == vvv ) || ((cmdPtr->cmd & 0xF0) == SHORT_JOIN) ) )   return; // already in the cmd buf

                cmdPtr = getCmdPtr(SETREG, TRUE, TRUE, TRUE);
                if( cmdPtr == NULL ) return; // no slots in cmd buf

                for( i = 0; i<NHIST; i++, hPtr+=10 ) 
                  if( mymemcmp( hPtr, rpb, 6 ) == 0 )
                    { found = TRUE; mymemcpy( &newBunch, hPtr+6, 2 );  mymemcpy( &newId, hPtr+8, 2 ); break; }
                  else if( nPtr == NULL && (mymemcmp( hPtr, nullmac, 6 )==0) )
                    nPtr = hPtr;
                if( !found ){
                    if( histCnt == NHIST || nPtr == NULL ) return;
                    newId = page.nDevs % page.maxDevs; newBunch = page.nDevs / page.maxDevs;
                    //while( newBunch >= nBunches ) nBunches++;
                    page.nDevs++; //flashDirty = TRUE; needNewPage = TRUE;
                    nBunches = 1 + page.nDevs / page.maxDevs;
                    
                    mymemcpy( nPtr,   rpb, 6 );
                    mymemcpy( nPtr+6, &newBunch, 2 );
                    mymemcpy( nPtr+8, &newId, 2 );
                    histCnt ++;

                    flashDirty = TRUE; needNewPage = TRUE;
                }
                
                cmdPtr->repCount = 1 + def_repeat;
                mymemcpy( cmdPtr->addr.mac, rpb, 6 );
                if( page.long_packets ){
                    cmdPtr->cmd = SETREG | MACADDR | SHORT_VAL;
                    cmdPtr->reg        = JOIN;
                    cmdPtr->value.ival = ( newBunch << 8 ) | newId;
                }else{
                    cmdPtr->cmd = SHORT_JOIN | newId;
                    cmdPtr->reg = newBunch;
                }

                ptr = (char *)allAdcs;                                         *ptr++ = '\n';
                ptr = utoa(  ticksLeft, 10, ptr ); strcpy( ptr, " ticks\t" );   ptr += 7;
                
                rpb -= 6;
                mymemcpy( (BYTE *)&pktUtc, rpb, 4 );
                ptr = ultoa( pktUtc, 10, ptr );                                 *ptr++ = '\t';
                mymemcpy( (BYTE *)&pktMs, rpb+4, 2 );
                ptr = utoa(  pktMs, 10, ptr );                                  *ptr++ = '\t';

                ptr = itoa(  (INT16)off, ptr );                                 *ptr++ = '\t';
                ptr = itoa(  lastRssi, ptr );    
                *ptr++ = 0;
                
              usb( (char *)allAdcs );
                strcpy((char *)allAdcs, " dB\tJoining ");  ptr = (char *)allAdcs + 12;                 
                ptr = printMac( ptr, (char *)(rpb + 6) );
                strcpy(ptr, "\tid="); ptr += 4;     *ptr++ = HEX( newId ); *ptr++ = '(';
                ptr = utoa( newBunch, 10, ptr );    *ptr++ = ')';   *ptr++ = '\n'; *ptr++ = 0;
              usb( (char *)allAdcs );
        }else{
            mymemcpy( (BYTE *)&pktMs, rpb+4, 2 ); pktMs &= 0x3FF;
            if( pktMs > 1000 ){ // calibration
                  strcpy( (char *)allAdcs, "\nSC" ); ptr = (char *)allAdcs + 3;
                  pktMs = ( pktMs - 1000 );
                  h = pktMs >> 4;   if( h ) *ptr++ = HEX( h ); 
                  *ptr++ = HEX( (pktMs&0xF) );  *ptr++ = ':';       *ptr++ = '\t';  // the scaling coeff #
                  
                  if( h >= 5 ){
                      h = ( curBunch >> 4 ) & 0xF;  if( h ) *ptr++ = HEX(h); 
                      h =  curBunch  & 0xF;         *ptr++ = HEX(h); 
                      *ptr++ = ':';                 *ptr++ = HEX( ( ( rpb[ 5 ] >> 2 ) & 0xF) );    *ptr++ = '\t';  
                  }
                  
                  ptr = printMac( ptr, (char *)(rpb+6) );           *ptr++ = '\t';  
                  *ptr = 0; usb( (char *)allAdcs ); ptr = (char *)allAdcs;

                  if( pktMs > 4 ){
                  //UINT16 u16;
                  //UINT32 u32;
                      ptr = (char *)allAdcs; 
                      // mpp, module, ov, oc
                      for(idx = 0;  idx<4;  idx++) ptr = printHex( rpb[idx], ptr ); //{ *ptr++ = HEX( ((rpb[idx]>>4)&0xF) );  *ptr++ = HEX( (rpb[idx]&0xF) ); }
                      // channel, radioDebug, azimuth, positionInString, netId, myBunch
                      for(idx = 12; idx<18; idx++) ptr = printHex( rpb[idx], ptr ); //{ *ptr++ = HEX( ((rpb[idx]>>4)&0xF) );  *ptr++ = HEX( (rpb[idx]&0xF) ); }
                      /*
                      if( pktMs == 5 ){                          
                          *ptr++ = '\t';
                          mymemcpy( (BYTE *)(&u32), rpb+18, 4 );
                          ptr = ultoa( u32, 16, ptr ); *ptr++ = '\t'; // inst date, UTC sec
                          *ptr= 0; usb( (char *)allAdcs ); ptr = (char *)allAdcs;
                          
                          for( idx = 22; idx<32; idx+=2){ // groupId, elevation, latitude, longitude, altitude
                            mymemcpy( (BYTE *)(&u16), rpb+idx, 2 );
                            ptr = utoa( u16, 16, ptr ); *ptr++ = '\t'; 
                          }
                      }else
                      */  
                      for(idx = 18; idx<32; idx++)  ptr = printHex( rpb[idx], ptr ); //{ *ptr++ = HEX( ((rpb[idx]>>4)&0xF) );  *ptr++ = HEX( (rpb[idx]&0xF) ); }
                      *ptr= 0; usb( (char *)allAdcs ); ptr = (char *)allAdcs;
                  }else{ 
                  float  c;
                      mymemcpy( (BYTE *)&c, rpb, 4 ); rpb += 12;
                      ptr = ftoa( c, ptr );
                      *ptr++ = '\t';
        
                      for( idx = 0; idx<5; idx++){
                          mymemcpy( (BYTE *)&c, rpb, 4 ); rpb += 4;
                          ptr = ftoa( c, ptr );
                          *ptr++ = '\t';
                          if( idx == 1 || idx == 4) { 
                              if( idx == 5 ) *ptr++ = '\n'; 
                              *ptr = 0; usb( (char *)allAdcs ); ptr = (char *)allAdcs; 
                          }
                      }
                  }
            }else{ // data
              if( page.longReport ){
                      strcpy( (char *)allAdcs, "\nL1:\t" ); ptr = (char *)allAdcs + 5;
                      ptr = utoa( ticksLeft, 10, ptr);             *ptr++ = '\t'; 
                      ptr = utoa( curBunch, 10, ptr);              *ptr++ = ':';   // bunch
                      *ptr++ = HEX( ((rpb[5]>>2)&0xF) );           *ptr++ = '\t';  // no group, just netId
                      ptr = printMac( ptr, (char *)(rpb+6) );      *ptr++ = '\t';   *ptr++ = 0;
                    usb( (char *)allAdcs );
                      allAdcs[0] = 0; ptr = (char *)allAdcs; 
                      mymemcpy( (BYTE *)&pktUtc, rpb, 4 );
                      ptr = ultoa( pktUtc,  10, ptr );              *ptr++ = '\t';
                      ptr = utoa( pktMs,    10, ptr );              *ptr++ = '\t';
                      ptr = itoa( (INT16)off,    ptr );             *ptr++ = '\t';
                      ptr = itoa( lastRssi,      ptr );             *ptr++ = ' ';   *ptr++ = 'd';   *ptr++ = 'B';   *ptr++ = '\t';
                      ptr = itoa( (signed char)rpb[28],  ptr );     *ptr++ = '\t';                             // pkt off
                      ptr = itoa( (signed char)rpb[29], ptr );      *ptr++ = ' ';   *ptr++ = 'd';   *ptr++ = 'B';   *ptr = 0;  // pkt rssi
                    usb( (char *)allAdcs );
                      strcpy( (char *)allAdcs, "\nL2:" ); ptr = (char *)allAdcs + 4;
                      for( idx = 0; idx < 8; idx ++ ){ 
                            *ptr++ = '\t'; 
                            switch( idx ){
                                case 0: case 1: case 3:                 ptr = utoa( _pktGetAdc16(rpb, idx), 10, ptr );  break;
                                //case 2: case 4: case 5: case 6: case 7: ptr = itoa( _pktGetAdc16(rpb, idx), ptr );      break;
                                default:                                ptr = itoa( _pktGetAdc16(rpb, idx), ptr );      break;
                            }
                      }
                      *ptr++ = '\t'; *ptr++ = 0;    
                    usb( (char *)allAdcs );
                      strcpy( (char *)allAdcs, "OV: " ); ptr = (char *)allAdcs + 4;
                      //strcpy( ptr, "OV: " ); ptr +=  4;
                      ptr = itoa( rpb[30],  ptr ); *ptr++ = '\t';  // over voltage
                      *ptr++ = 'O'; *ptr++ = 'C'; *ptr++ = ':'; *ptr++ = ' ';
                      ptr = itoa( rpb[31], ptr );  *ptr++ = '\t';  // over current
                      if( rpb[5] & 0x40 ) strcpy( ptr, "mpp on \t" ); else strcpy( ptr, "mpp off\t" );
                      ptr += 8;
                      if( rpb[5] & 0x80 ) strcpy( ptr, "module on\n" ); else strcpy( ptr, "module off\n" );
                    usb( (char *)allAdcs );
              }else{
              BYTE *p = &rpb[0];
                    ptr = (char *)allAdcs;
                    *ptr++='\n';   *ptr++ = '|'; 
                    ptr = printHex( curBunch,   ptr );
                    ptr = printHex( off,        ptr );
                    ptr = printHex( lastRssi,   ptr );
                    *ptr++='|';                          // 10
                    for( idx = 0; idx<32; idx++ ){       // 64
                        h = ( (*p) >> 4 ) & 0xF;     *ptr++ = HEX(h);
                        h = (*p++) & 0xF;            *ptr++ = HEX(h);
                        //if( idx == 14 ) { *ptr++ = 0; usb( (char *)allAdcs ); ptr = (char *)allAdcs; }
                    }                                   
                    *ptr++ = 0;
                    usb( (char *)allAdcs );
              }
              if( histCnt ){
                  BYTE *hPtr = idHistory;
                  rpb  += 6;
                  for( i = 0; i<NHIST; i++, hPtr += 10 ) if( mymemcmp( hPtr, rpb, 6 ) == 0 ){
                      cmdPtr = cmdBuf;
                      for( j = 0; j <= CMDBUFMSK; j++, cmdPtr++)
                          if( mymemcmp( cmdPtr->addr.mac, rpb, 6 ) == 0  &&
                              ( ( cmdPtr->cmd == vvv ) || ((cmdPtr->cmd & 0xF0) == SHORT_JOIN) ) )
                                  { mymemset( cmdPtr, 0, sizeof(CMD) ); break; }
                      mymemset( hPtr, 0xFF, 6 );  histCnt--;
                      break;
                  }
              }
             }
        }
     }
     //ticksLeft -= ticks2SendRequest;
     //ptr = utoa( ticksLeft, 10, (char *)allAdcs ); *ptr++='\n'; *ptr = 0;
     //usb( (char *)allAdcs );
}
/******************************************************************************/
unsigned char* rollNW( unsigned char *ptr ) { 
    while( *ptr ) if( *ptr != ' ' && *ptr != '\t' && *ptr != '\n' && *ptr != '\r' ) ptr++; else break; 
    return ptr;
}
unsigned char* rollW( unsigned char *ptr ) { 
    while( *ptr ) if( *ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r' ) ptr++; else break; 
    return ptr;
}
unsigned char* roll2next(   unsigned char *ptr ) {     return rollW( rollNW( ptr ) );  }
/******************************************************************************/
unsigned char ch2int(   unsigned char ch ) { 
    if(      ch >= '0' && ch <= '9' ) ch-= '0';
    else if( ch >= 'A' && ch <= 'F' ) ch = 10 + (ch - 'A');
    else if( ch >= 'a' && ch <= 'f' ) ch = 10 + (ch - 'a');
    else                              ch = 0;
    return   ch & 0xF; 
}
/******************************************************************************
 * command address parsing
******************************************************************************/
BOOL parseAddr(BYTE *ptr, CMD *cmdPtr){
BYTE i;
    switch( *ptr++ ){
        case '*': cmdPtr->cmd |= BROADCAST;                                                          return TRUE; // broadcast command to all
        case 's': cmdPtr->cmd |= STRINGADDR; cmdPtr->addr.stringAddr = strtoul( (char*)ptr, 0, 16 ); return TRUE;
        case '#': cmdPtr->cmd |= NIDADDR;    cmdPtr->addr.netId      = strtoul( (char*)ptr, 0, 16 ); return TRUE;
        case 'm': 
            cmdPtr->cmd |= MACADDR;    
            for(i = 0; i<6; i++){
                cmdPtr->addr.mac[i] =  ch2int( *ptr++ ) << 4; 
                cmdPtr->addr.mac[i] |= ch2int( *ptr++ ); 
            }
        return TRUE;
        default : cmdPtr->cmd = NULLOP; return FALSE; // error - ignore this command
    }
}
/******************************************************************************
 * allocates command from buffer 
******************************************************************************/
CMD *getCmdPtr(BYTE cmd, BOOL useReg, BOOL useAddr, BOOL useValue){
int j;
    for( j = 0; j <= CMDBUFMSK; j++) if( cmdBuf[j].cmd == NULLOP || cmdBuf[j].repCount == 0 ){
        CMD *cmdPtr = &cmdBuf[j];
        cmdPtr->repCount = def_repeat;
        cmdPtr->cmd = cmd;
        cmdPtr->useReg = useReg;  cmdPtr->useAddr = useAddr;  cmdPtr->useValue = useValue;
        cmdPtr->seq = cmdCount++;
        return cmdPtr;
    }
    return NULL;
}

INT32 accumulatedDF = 0;
BOOL readHexArray(BYTE *arr, BYTE *ptr, BYTE l){
BYTE i, j;
    mymemset( arr, 0, l );
    for( i = 0; i < l; i++, arr++ )
        for( j = 0; j < 2; j++, ptr++ )
             if(      ( *ptr >= '0') && (*ptr <= '9') ) *arr |= (*ptr - '0')      << (j?0:4);
             else if( ( *ptr >= 'A') && (*ptr <= 'F') ) *arr |= (*ptr - 'A' + 10) << (j?0:4);
             else if( ( *ptr >= 'a') && (*ptr <= 'f') ) *arr |= (*ptr - 'a' + 10) << (j?0:4);
             else return FALSE;
    return TRUE;
}
/******************************************************************************
* Parse Commands from main computer and take actions
* Commands are:
*
*   Addr:  all  string  inidividual   MAC         
*           *   |sNNNN | #NNNN      | mNNNNNNNNNN
*
*   Value: logical 0/1  byte hex  short hex  32bit hex    float
*           lN         | bNN     | sNNNN    | LNNNNNNNN | fNNNNNNNN 
*
*   'A 0/1 HHHH KKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKK VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv'                     
*                           send AES vector
*                               0 - send new AES key, 1 - send new AES IV
*                               HHHH => network address in hex, first 2 letters - network id, second 2 letters - bunch
*                               KKK...KKK => 16 bytes of 'real secret' AES key in hex
*                               VVV...VVV => 16 bytes of 'real secret' AES initialization vector in hex
*                               vvv...vvv => 16 bytes of new network key in hex
*   'A 2/3 vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv'                     
*                           set AES vector to GW
*                               0 - send new AES key, 1 - send new AES IV
*                               vvv...vvv => 16 bytes of new network key in hex
*   'a 0/1'                 ADCs on /off
*   'B NNNN'                set edAddr and gwAddr for this gateway, NNNN is in hex
*   'd 0/1'                 turn on/off 'bunch 255' mode - needed for boot over the air
*   'C'                     
*   'c addr bPage'          clear the page, page from 1 to 30 inclusive
*   'D'                     
*   'b addr L Addr HexSeq'  send n bytes for 'boot over the air' - 
*                               addr - address in network, as always,
*                               L    - length, in hex
*                               Addr - address in flash, hex
*                               HexSeq - data to send
*   'E 0|1'                 echo on/off
*   'e'                     start sending requests in One second, used for synchronization of several gateways
*   'F Addr'                allow to write to flash for End Devices + store defaults - overV, overC, module, mpp
*   'f Addr'                allow to write to flash for End Devices
*   'G'                     write to flash
*   'g NN'                  set shift to GW offset register
*   'h NN'                  set channel - 8 bit command, BYTE for new channel number      
*   'H 0/1'                 hopping on/off
*   'I', 'i'                GW info
*   'J (sNNNN | mNNNN | hNNNN | dNNNN)'     JOIN enabled for nnnn seconds/minutes/hours/days
*   'k NN'                  make nDevs=NN
*   'K N FF.F'              set Gateway coefficient 4 - tk0, 5 - tk1, 0 - dFk1, 1 - dFk2, 2 - dFk3, 3 - dFk4
*   'l 0/1'                 long packets (34 bytes) only
*   'L 0/1                  long reports / short reports
*   'm Addr'                restart measurement - clear buffer on specified ED. Needed for test purposes.
*   'M mac'                 set mac for this GW
*   'N', 'n'                dissolve network - all ED forget about their id's, GW network id goes to 0
*   'O'                     get gateway coefficients
*   'o Addr'                get coeeficients
*   'p'                     advance command counter by 1000
*   'P HH'                  set power - PA_TABLE0 to 
*   'Q'                     reset the commands counter - suppose to be done every night (before active day) by middleware
*   'r Addr'                reset processor on end device
*   'R'                     reset gateway
*   's' 'S Addr Reg Value'      set register in specified end device
*   'T UTC MS'              set up time in UTC + milliseconds
*   'u'                     read from flash
*   'v 0/1'                 fcc (30 kbod, 2410 start, ~250 kHz interchannel space ) mode off/on
*   'V 0/1'                 use FEC
*   'w Addr'                switch to CW mode for selected devices
*   'W'                     switch this gateway to CW mode
*   'X'                     clear the id distribution history
*   'x'                     watchdog start
*   'Y 0|1' 'y 0/1'         0 means Rx Only, 1 means Normal
*   'Z Addr'                allow for flash rejuvenation on end devices
*   'z N'                   set default amount of repetition
******************************************************************************/
void parseCommand(void){
unsigned char *ptr = inBuff; 
CMD *cmdPtr;
BOOL parseError = FALSE;
BYTE reg, cmd;
static BYTE w, y, l;
static UINT32 n;
//    do{
        ptr = rollW( ptr );  // leading spaces
        reg = 0;
        switch( cmd = *ptr++ ){
            
            case 'z':
                ptr = rollW( ptr );
                if( *ptr >= '0' && *ptr <= '9') l = (BYTE) atoi( (char *)ptr ); 
                if( l > 0 ) def_repeat = l;
            break;
            case 'c':
                ptr = rollW( ptr );
                cmdPtr = getCmdPtr( SETREG, TRUE, TRUE, TRUE ); 
                if( cmdPtr == NULL ) { parseError = TRUE;  *ptr = 0; break; }
                ptr = rollW( ptr );
                // command address parsing
                if( !parseAddr( ptr, cmdPtr ) ) { parseError = TRUE; *ptr = 0; break; }
                ptr = roll2next( ptr );
                // parse register
                cmdPtr->reg = 10;
                cmdPtr->cmd |= BYTE_VAL;     cmdPtr->value.bval = (BYTE) atoi( (char *)ptr ); 
                cmdPtr->repCount = def_repeat;
            break;
            case 'd':
                ptr = rollW( ptr );
                switch( *ptr++ ){
                    case '0': bunch255 = FALSE; break;
                    case '1': bunch255 = TRUE;  break;
                    default: 
                        usb("no action was taken\n"); 
                        mymemset( inBuff, 0,  OUT_BUF_SIZE/16 );   inBuffIdx = 0;
                    return ;
                }
            break;
            case 'b':
                ptr = rollW( ptr );
                cmdPtr = getCmdPtr( BOOT_CMD, TRUE, TRUE, FALSE ); 
                if( cmdPtr == NULL ) { parseError = TRUE;  *ptr = 0; break; }
                ptr = rollW( ptr );
                // command address parsing
                if( !parseAddr( ptr, cmdPtr ) ) { parseError = TRUE; *ptr = 0; break; }
                ptr = roll2next( ptr );
                cmdPtr->reg    = (BYTE)   (strtoul( (char*)ptr, 0, 16)&0xFF);
                ptr = roll2next( ptr );
                cmdPtr->bAddr  = (UINT16) (strtoul( (char*)ptr, 0, 16)&0xFFFF);
                ptr = roll2next( ptr );
                //usb( ptr );
                if( !readHexArray( cmdPtr->value.arr, ptr, cmdPtr->reg ) ){ 
                    cmdPtr->cmd = NULLOP;    parseError = TRUE;  
                    usb( "array\n" );
                }else cmdPtr->repCount = def_repeat;
            break;
            case 'v': 
                ptr = rollW( ptr );
                switch( *ptr++ ){
                    case '0': page.use30kbod = FALSE; break;
                    case '1': page.use30kbod = TRUE;  break;
                    default: 
                        usb("no action was taken\n"); 
                        mymemset( inBuff, 0,  OUT_BUF_SIZE/16 );   inBuffIdx = 0;
                    return ;
                }

                INT_GLOBAL_ENABLE( FALSE );
                setupRadio();
                RFIM    = IRQ_DONE;
                INT_GLOBAL_ENABLE( TRUE );
            break;
            case 'V':
                ptr = rollW( ptr );
                switch( *ptr++ ){
                    case '0': page.useFEC = FALSE; break;
                    case '1': page.useFEC = TRUE;  break;
                    default: 
                        usb("no action was taken\n"); 
                        mymemset( inBuff, 0,  OUT_BUF_SIZE/16 );   inBuffIdx = 0;
                    return ;
                }

                INT_GLOBAL_ENABLE( FALSE );
                if( page.useFEC /*&& page.use30kbod*/ ) MDMCFG1 |= 0x80; else MDMCFG1 &= 0x7F;
                INT_GLOBAL_ENABLE( TRUE );
            break;
            
            case 'P': {
            INT8 dbm = 1;
                ptr = rollW( ptr );
                dbm = (BYTE)atoi( (char*)ptr );
                switch( dbm ){
                    case 1:   page.power = 0xFF; break;
                    case 0:   page.power = 0xFE; break;
                    case -1:  page.power = 0xED; break;
                    case -2:  page.power = 0xBF; break;
                    case -4:  page.power = 0xAA; break;
                    case -6:  page.power = 0x7F; break;
                    case -8:  page.power = 0x99; break;
                    case -10: page.power = 0xCB; break;
                    case -15: page.power = 0x8E; break;
                    case -20: page.power = 0x4D; break;
                    case -25: page.power = 0x51; break;
                    case -35: page.power = 0x40; break;
                    case -65: page.power = 0x0;  break;
                    default: 
                        usb("no action was taken\n"); 
                        mymemset( inBuff, 0,  OUT_BUF_SIZE/16 );   inBuffIdx = 0;
                    return ;
                }
                PA_TABLE0 = page.power;
            }break;

            case 'B': {
            UINT16 addrs = 1;
                ptr = rollW( ptr );
                addrs = (UINT16) (strtoul( (char*)ptr, 0, 16) & 0xFFFF );
                /*{ char *p = (char *)allAdcs;  *p++ = '\n';
                        p = utoa( addrs,  16, p );  *p++ = 0;
                        usb( (char *)allAdcs ); }*/
                /*if( addrs & 0xFF   ) */page.edAddr = addrs & 0xFF;
                /*if( addrs & 0xFF00 ) */page.gwAddr = (BYTE)((addrs>>8)&0xFF);
                ADDR = page.gwAddr;
            }break;

            case 'u':
                if( !readTheLatest((BYTE *)&page, PAGESIZE ) ){ usb("flash empty\n"); flashDirty = TRUE; needNewPage = TRUE; }
                //if( transfer ) usb( "tr not finished\n" );
            break;
            case 'K':{
            float *fptr = page.dFk;
                ptr = rollW( ptr );
                // parse register
                reg = atoi( (char *)ptr ); 
                ptr = roll2next( ptr );
                // parse value
                switch( reg ){
                    case 0: case 1: case 2: case 3: case 4:  case 5:     fptr[ reg ] = atof( (char*)ptr );          break;
                    case 6:{ BYTE md = atoi( (char *)ptr );  if( md >=1 && md <=16) page.maxDevs = md; else usb( "maxDevs should be from 1 to 16");  } break;
                    case 7: page.beatSpike = atoi( (char *)ptr ); break;
                    case 8:{ BYTE md = atoi( (char *)ptr );  if( md <=16) page.bootDelay = md; else usb( "bootDelay should be from 0 to 16");  } break;
                    case 9: page.printSupercycle = atoi( (char *)ptr ); break;
                    case 10:{
                      UINT16 sync = (UINT16) (strtoul( (char*)ptr, 0, 16)&0xFFFF);  
                      SYNC0 = sync & 0xFF; SYNC1 = (sync>>8) & 0xFF;
                    }break;                      
                    default: usb( "there are only 12 registers in the gateway\n");                       break;
                }
            }break;
            
            // k - set the amount of devices
            case 'k':{
                ptr = rollW( ptr );
                page.nDevs = atoi( (char*)ptr );
                nBunches = 1 + page.nDevs / page.maxDevs;
                flashDirty = TRUE; needNewPage = TRUE;
            }break;
            
            // 'T UTC MS' set up time in UTC + milliseconds
            case 'T':   case 't': 
                ptr = rollW( ptr );
                utc = (unsigned long) strtol((char*)ptr, 0, 10);  
                ptr = roll2next( ptr );
                if( *ptr ){
                    ms  = (unsigned int ) (strtol((char*)ptr, 0, 10)&0xFFFF);
                    ptr = roll2next( ptr );
                    ticks2MsTime = TICKS_IN_MS;
                }
            break;
            case 'M':  
                ptr = rollW( ptr );
                for(reg = 0; reg<6; reg++){
                    page.mac[reg] =  ch2int( *ptr++ ) << 4; 
                    page.mac[reg] |= ch2int( *ptr++ ); 
                }                        
            break;
            //                          all  string  inidividual   MAC         reg#  logical  byte  hex byte    short   32bit       float
            case 'S': case 's':  // 'S (*   |sNNNN | #NNNN      | mNNNNNNNNNN) NN   (lN     | bNN  |  hNN      | sNNNN | LNNNNNNNN | fNNNNNNNN )
                cmdPtr = getCmdPtr( SETREG, TRUE, TRUE, TRUE ); 
                if( cmdPtr == NULL ) { parseError = TRUE;  *ptr = 0; break; }
                ptr = rollW( ptr );
                // command address parsing
                if( !parseAddr( ptr, cmdPtr ) ) { parseError = TRUE; *ptr = 0; break; }
                ptr = roll2next( ptr );
                // parse register
                cmdPtr->reg = atoi( (char *)ptr ); 
                ptr = roll2next( ptr );
                // parse value
                switch( *ptr++ ){
                    case 'n': cmdPtr->cmd |= BYTE_VAL;     cmdPtr->value.bval = (BYTE)   atoi( (char*)ptr );          break;
                    case 'b': cmdPtr->cmd |= BYTE_VAL;     cmdPtr->value.bval = (BYTE)   strtoul( (char*)ptr, 0, 10); break;
                    case 'h': cmdPtr->cmd |= BYTE_VAL;     cmdPtr->value.bval = (BYTE)   strtoul( (char*)ptr, 0, 16); break;
                    case 'i': case 's': 
                              cmdPtr->cmd |= SHORT_VAL;    cmdPtr->value.ival = (UINT16) (strtoul( (char*)ptr, 0, 10 )&0xFFFF); break;
                    case 'H': cmdPtr->cmd |= SHORT_VAL;    cmdPtr->value.ival = (UINT16) (strtoul( (char*)ptr, 0, 16)&0xFFFF);  
                        /*{ char *p = (char *)allAdcs; *p++ = '\n';
                                p = utoa( cmdPtr->value.ival,  16, p );  *p++ = 0;
                                usb( (char *)allAdcs ); }*/
                    break;
                    case 'l': cmdPtr->cmd |= LONG_VAL;     cmdPtr->value.lval = (UINT32) strtoul( (char*)ptr, 0, 10 ); break;
                    case 'L': cmdPtr->cmd |= LONG_VAL;     cmdPtr->value.lval = (UINT32) strtoul( (char*)ptr, 0, 16 ); break;                    
                    case 'f': case 'd': 
                              cmdPtr->cmd |= FLOAT_VAL;    cmdPtr->value.dval = atof( (char*)ptr );                    break;
                    case 'm':
                        cmdPtr->useReg = FALSE; cmdPtr->cmd |= SET_MAC_CMD;
                        for(reg = 0; reg<6; reg++){
                            cmdPtr->value.arr[reg] =  ch2int( *ptr++ ) << 4; 
                            cmdPtr->value.arr[reg] |= ch2int( *ptr++ ); 
                        }                        
                    break;
                    case 'S':
                        cmdPtr->useReg = FALSE; cmdPtr->cmd |= SET_MAC_CMD;
                        w  = ch2int( *ptr++ ) * 10; w += ch2int( *ptr++ );  w &= 0x3F;
                        y  = ch2int( *ptr++ ) * 10; y += ch2int( *ptr++ );  y &= 0x7F;
                        l  = *ptr++; if( l > 'a' ) l -= 'a'; else l -= 'A';
                        n  = strtoul( (char *)ptr, 0, 10 );
                        cmdPtr->value.arr[ 5 ] = n       & 0xFF;
                        cmdPtr->value.arr[ 4 ] = (n>>8)  & 0xFF;
                        cmdPtr->value.arr[ 3 ] = (n>>16) & 0xFF;
                        n = ( ((UINT32)w) << 18 ) | ( ((UINT32)y) << 11) | ( l << 6 );
                        cmdPtr->value.arr[ 2 ] = n       & 0xFF;
                        cmdPtr->value.arr[ 1 ] = (n>>8)  & 0xFF;
                        cmdPtr->value.arr[ 0 ] = (n>>16) & 0xFF;
                    break;
                    default : 
                        cmdPtr->cmd = NULLOP;   parseError = TRUE;
                    break; // error - ignore this command
                }
                //usb("'s' command parsed\n");
                if( ( reg == FLASH_REFRESH ) && ( ( cmdPtr->cmd & BYTE_VAL ) == BYTE_VAL ) ){
                    cmdPtr->cmd = NULLOP;   parseError = TRUE;
                }
            break;

            // Reset End Device processors
            case 'R': 
                HAL_INT_ENABLE(INUM_T1,  INT_OFF);     
            break;
            case 'r':
                cmdPtr = getCmdPtr( RESET, FALSE, TRUE, FALSE );
                if( cmdPtr == NULL ) { parseError = TRUE;  *ptr = 0; break; }
                ptr = rollW( ptr );
                // command address parsing
                parseAddr( ptr, cmdPtr ); if( cmdPtr->cmd==NULLOP ){ parseError = TRUE;  *ptr = 0; break; }
            break;

            // SILENCE
            case 'C': 
                cmdPtr = getCmdPtr( SILENCE, FALSE, FALSE, FALSE ); cmdPtr->repCount = 1;
                if( cmdPtr == NULL ) { parseError = TRUE;  *ptr = 0; break; }
            break;

#ifndef RevC
            // Set / send AES key / vector 
            case 'A': {
            static BYTE temp_arr[16];
            BYTE what;
            UINT16 addr;
                ptr = rollW( ptr );
                switch( what = *ptr++ ){
                    case 3: 
                        if( !readHexArray( temp_arr, ptr, 16 ) ) usb( "no action was taken, error" );
                        else{ mymemcpy( page.curKey, temp_arr, 16 ); loadKey( page.curKey ); }
                    break;
                    case 4: 
                        if( !readHexArray( temp_arr, ptr, 16 ) ) usb( "no action was taken, error" );
                        else  mymemcpy( page.curIV, temp_arr, 16 ); 
                    break;
                    case 0:  case 1:
                        addr = (UINT16) strtoul( (char*)ptr, 0, 16 );   
                        ptr = roll2next( ptr );
                        if( !readHexArray( temp_arr, ptr, 16 ) ){ usb( "no action was taken, error" ); break; }
                        ptr = roll2next( ptr );
                        loadKey( (char *)temp_arr );
                        if( readHexArray( temp_arr, ptr, 16 ) ){ 
                            ptr = roll2next( ptr );
                            loadIV( (char *)temp_arr );
                            if( readHexArray( temp_arr, ptr, 16 ) ){ 
                              cmdPtr = getCmdPtr( what ? SET_NEXT_IV : SET_NEXT_KEY, FALSE, TRUE, FALSE ); 
                              encode( 16, (char *)temp_arr, (char *)cmdPtr->value.arr );
                              cmdPtr->cmd |= NIDADDR;    cmdPtr->addr.netId = addr;
                            }else usb( "no action was taken, error" ); 
                        }else usb( "no action was taken, error" ); 
                        loadKey( page.curKey );
                    break;
                }
            }break;
#endif            
            case 'a':
                ptr = rollW( ptr );
                switch( *ptr++ ){
                    case '0': page.useADC = FALSE;                            break;
                    case '1': page.useADC = TRUE;  ticks2AdcMeasurement = 50; break;
                    default: usb("no action was taken\n"); break;
                }
            break;
            
            case 'H':  
                ptr = rollW( ptr );
                switch( *ptr++ ){
                    case '0': page.isHopper = FALSE; CHANNR = page.channel; break;
                    case '1': page.isHopper = TRUE;  time2Hop = FALSE;      break;
                    default: usb("no action was taken\n"); break;
                }
            break;
              
            // JOIN enabled for nnnn seconds/minutes/hours/days
            // J (sNNNN | mNNNN | hNNNN | dNNNN)
            case 'J':  case 'j':{
                UINT32 multiplier = 1; 
                ptr = rollW( ptr );
                switch( *ptr++ ){
                    case 's': case 'S': multiplier = 1L;        break;
                    case 'm': case 'M': multiplier = 60L;       break;
                    case 'h': case 'H': multiplier = 3600L;     break;
                    case 'd': case 'D': multiplier = 24*3600L;  break;
                }
                page.secondsJoinEnabled = multiplier * atol( (char *)ptr );
                joinEnabled = TRUE;
            }break;

            // echo on / off
            case 'E': {
                ptr = rollW( ptr );
                switch( *ptr++ ){
                    case '0': echoEnabled = FALSE;  break;
                    default : echoEnabled = TRUE;   break;
                }
            }break;

            // long packets on / off
            case 'l': {
                ptr = rollW( ptr );
                switch( *ptr++ ){
                    case '0': page.long_packets = FALSE;  break;
                    default : page.long_packets = TRUE;   break;
                }
            }break;

            // long report on / off
            case 'L': {
                ptr = rollW( ptr );
                switch( *ptr++ ){
                    case '0': page.longReport = FALSE;  break;
                    default : page.longReport = TRUE;   break;
                }
            }break;

            // decrease the nDevs - tool for filling slots by daemon - it SET netId for last ED and decrease nDevs after that
            case 'D':
              if( page.nDevs > 0 ) page.nDevs--;   
              nBunches = 1 + page.nDevs / page.maxDevs;
              flashDirty = TRUE; needNewPage = TRUE; 
            break;

            // set channel - 16 bit command, 8 bit for new channel number, 8 bit for delay in ms ( - slot for GW )
            case 'h':{
                ptr = rollW( ptr );
                if( (*ptr>='0' && *ptr<='9') || *ptr=='-' ){
                    INT16 newch = atoi( (char *)ptr );
                    if( newch >= 0 ){
                        cmdPtr = getCmdPtr( SETREG | BYTE_VAL | BROADCAST, TRUE, TRUE, TRUE );
                        if( cmdPtr == NULL ) { parseError = TRUE;  *ptr = 0; break; }
                        cmdPtr->reg = SET_CHANNEL;
                        cmdPtr->value.bval = (BYTE)(newch&0xFF);
                        cmdPtr->repCount = def_repeat + 1;
                        if( page.isRxOnly ){
                            CHANNR = cmdPtr->value.bval; calibrate();
                            cmdPtr->reg = 0; cmdPtr->repCount = 0; cmdPtr->cmd = NULLOP; 
                            setup_hopper( page.channel = cmdPtr->value.bval );
                        }
                    }else{
                        newch = -newch;
                        setup_hopper( newch & 0xFF );
                        CHANNR = page.channel = newch & 0xFF; 
                    }
                }
            }break;

            // reset command count
            case 'Q':  case 'q':    cmdCount = 1;                                   break;
            case 'p':    cmdCount >>= 10; cmdCount++; cmdCount <<=10;    break;
            case 'G': // write the gateway information to flash
                writePage2Flash();
            break;
            case 'X':   mymemset( idHistory, 0xFF, NHIST*10 ); histCnt = 0;    break;
            case 'x':   sec2USBWatchDog = 120;                               break;
            case 'W':
                isCW = TRUE; 
                HAL_INT_ENABLE(INUM_RF,  INT_OFF);    // Enable RF general interrupt
            break;
            case 'Y': 
                ptr = rollW( ptr );
                switch( *ptr++ ){
                    case '0': page.isRxOnly = TRUE;  PKTCTRL1 &= 0xFC; break; // address check off 
                    default : page.isRxOnly = FALSE; PKTCTRL1 |= 0x01; break;
                }
            break;
            
            // allow for flash rejuvenation on end devices
            case 'Z':
                reg = FLASH_REFRESH2;                      //refreshTheFlash();
            // switch to continues transmission - for test by FCC and Anatoli
            case 'w':    if( reg == 0  )  reg = CW_MODE;                            
            // restart measurement on ED's
            case 'm':    if( reg == 0  )  reg = RESTART_MEASUREMENT;                            
            // dissolve network
            case 'N':  case 'n':    if( reg == 0  ){  
                reg = DISSOLVE_NETWORK;  
                page.nDevs = 0; nBunches = 0;  
                mymemset( idHistory, 0xFF, 10*NHIST );
            }
            // ocifrovka - get scaling coefficients
            case 'o':    if( reg == 0 )   reg = REPORT_SCALING; 
            // allow for flash writing on end devices
            case 'F':    if( reg == 0 )   reg = FLASH_AVAILABLE; 
            
            case 'f':
                if( reg == 0 )    reg = COEFFICIENTS_AVAILABLE; 
                cmdPtr = getCmdPtr( SETREG, TRUE, TRUE, TRUE ); 
                if( cmdPtr == NULL ) { parseError = TRUE;  *ptr = 0; break; }
                if( reg != DISSOLVE_NETWORK ){
                    ptr = rollW( ptr ); 
                    parseAddr( ptr, cmdPtr ); // command address parsing
                }
                cmdPtr->cmd |= BYTE_VAL; cmdPtr->value.bval = TRUE; cmdPtr->reg = reg;
            break;

            case 'g':
                ptr = rollW( ptr );
                page.theShift = atoi( (char *) ptr );
                FSCTRL0 = page.theShift;
                flashDirty = TRUE;
            break;
            
            case 'e': ticks2SendRequest = TICKS_IN_SEC; break;
            
            case 'O':{
                char *ptr;
                BYTE i;
                ptr = (char *)allAdcs;  
                for( i = 0; i < 6; i++) { ptr = ftoa( page.dFk[i], ptr );  *ptr ++ = '\t';  }
                *ptr++ = '\n'; *ptr = 0;
                usb( (char *) allAdcs );
            }break;
            
            // info about gateway
            case 'I': case 'i':{ 
            char *s0 = " Freq ";
            char *s1 = " Tick (mks) ";
            char *s2 = " Chan ";
            char *s21 = " Shift ";
            char *s3 = " # SPTs ";
            char *s4 = " bunch size ";
            char *s5 = "\nUTC:";
            char *s6 = " temp ";
            char *s7 = " shift ";
            char *s8 = " ADCs ";
            char *s9 = " Power ";
            char *s10 = "MAC ";
            char *ptr;
                usb("AND Solar Radio Gateway V9, No Lobotomy enabled\nDatarate:"); 
                ptr = ultoa( _datarate,  10, (char *)allAdcs );  strcpy( ptr, s0); ptr += strlen( s0 );
                ptr = ultoa( _freq,      10, (char *)ptr );      strcpy( ptr, s1); ptr += strlen( s1 );
                ptr = utoa( MKS_IN_TICK, 10, ptr);               strcpy( ptr, s2); //ptr += strlen( s2 );
                usb( (char *)allAdcs );
                ptr = utoa( page.channel, 10, (char *)allAdcs ); strcpy( ptr, s21); ptr += strlen( s21 );
                ptr = itoa( (INT8)FSCTRL0,  ptr );               strcpy( ptr, s3);  ptr += strlen( s3 );
                *ptr = 0;
                usb( (char *)allAdcs );
                
                ptr = utoa( page.nDevs, 10, (char *)allAdcs );   strcpy( ptr, s4); ptr += strlen( s4 );
                ptr = utoa( page.maxDevs,    10, ptr );               //*ptr = 0;
                strcpy( ptr, s5 );                                ptr += strlen( s5 );
                ptr = ultoa( utc, 10, ptr );   *ptr++ = ' '; 
                ptr = utoa(  ms,  10, ptr );   *ptr++ = ' '; 
                if( page.isRxOnly ) strcpy( ptr, "Rx Only" ); else strcpy( ptr, "Normal " );
                ptr += 7;  
                if( page.long_packets ) strcpy( ptr, " long packets  ");
                else                    strcpy( ptr, " short packets ");
                ptr +=15;
                *ptr = 0;
                usb( (char *)allAdcs );
                
                ptr = (char *) allAdcs;
                strcpy( ptr, s6 ); ptr+= strlen( s6 );
                ptr = itoa( theTemperature, ptr );              *ptr++ = '\t';
                strcpy( ptr, s7 ); ptr+= strlen( s7 );
                ptr = ltoa( accumulatedDF, ptr );
                strcpy( ptr, s8 ); ptr+= strlen( s8 );         
                strcpy( ptr, page.useADC ? " TRUE " : " FALSE" ); ptr += 6;
                strcpy( ptr, s9 ); ptr+= strlen( s9 );         
                ptr = utoa( PA_TABLE0, 16, ptr );
                *ptr++ = '\n'; 
                strcpy( ptr, s10 ); ptr+= strlen( s10 ); 
                ptr = printMac( ptr, page.mac ); 
                *ptr++ = 0;
                usb( (char *)allAdcs );
                
                ptr = (char *) allAdcs;
                if( page.isHopper ) strcpy( ptr, " hopper " ); else strcpy( ptr, " no hop " );
                ptr+= 8;
                if( MDMCFG1 & 0x80 ) strcpy( ptr, " FEC=1 " ); else strcpy( ptr, " FEC=0 " );
                ptr+= 7;
                strcpy( ptr, "GwAddr " ); ptr+=7;
                ptr = itoa( page.gwAddr, ptr );              *ptr++ = '\t';
                strcpy( ptr, "EdAddr " ); ptr+=7;
                ptr = itoa( page.edAddr, ptr );              //*ptr++ = '\t';
                *ptr++ = '\n'; 
#ifdef RevC
                strcpy( ptr, "Rev C compatible\n" );
#else
                *ptr++ = 0;
#endif
                usb( (char *)allAdcs );
                if( page.beatSpike ) usb("beat Spike mode\n");
                
                ptr = (char *) allAdcs;
                *ptr++ = HEX( ((SYNC1>>4)&0xF) );  *ptr++ = HEX( ((SYNC1)&0xF) );
                *ptr++ = HEX( ((SYNC0>>4)&0xF) );  *ptr++ = HEX( ((SYNC0)&0xF) );
                *ptr++ = '\n'; *ptr = 0;
                usb( allAdcs );
            }break;
            case '\n': case '\r': case ' ': case '\t': case 0: break;

            default: 
              usb("command was not recognized - "); 
              if( cmd != 0 ) allAdcs[0] = cmd; else allAdcs[0] = ' ';
              allAdcs[1] = '(';  allAdcs[2] = HEX(((cmd>>4)&0xF));  allAdcs[3] = HEX((cmd&0xF)); allAdcs[4] = ')';
              allAdcs[5] = '\n'; allAdcs[6] = 0;
              usb( (char *)allAdcs );
            break;
        }
// One line - one command, "V ochered' sukiny deti ..."
//        ptr = roll2next( ptr );
//    }while( *ptr );
    if( parseError ) usb("ERROR\n");
    mymemset( inBuff, 0,  OUT_BUF_SIZE/16 );   inBuffIdx = 0;
}
/******************************************************************************
* @fn  calibrate
* @brief       This function initiates calibration of radio.
* Parameters:
* @return void
******************************************************************************/
void calibrate(void){
    ticks2Start2Receive = _calibrationDelay;
    
    mode = RADIO_MODE_CALIBRATE;
    INT_GLOBAL_ENABLE(FALSE);
    RFST = STROBE_IDLE; 
    RFST = STROBE_CAL; 
    INT_GLOBAL_ENABLE(TRUE);
}
/******************************************************************************
* @fn  abs
* @brief       absolute value
* Parameters: int
* @return     int
******************************************************************************/
INT32 abs32(INT32 a){ return (a<0)?-a:a; }
/******************************************************************************
* @fn  configureAdcs
* @brief                       This function maps all P0 pins to be input ADCs pins
* Parameters:
* @return void
******************************************************************************/
void configureAdcs(void){
BYTE pin;
    P0DIR &= ~0x38; // port 0 pins 3,4,5 are input
    P0INP |= 0x38;  // port 0 pins 3,4,5 are tristate
    ADC_ENABLE_CHANNELS( 0x38 );
    for( pin = 3; pin < 6; pin++ ){ IO_ADC_PORT0_PIN( pin, IO_ADC_EN); }
}
/******************************************************************************
* initiate reading of adc channels
******************************************************************************/
void armAdcs(void){  
    adcCount = 0;
    ticks2AdcMeasurement = 50; //_adcMeasurement;  // 5ms
    ADC_SINGLE_CONVERSION( adcChannels[ 0 ] ); 
    ADCIF = 0; 
}

void addDF(INT32 dFreq);
//INT32 abs32( INT32 a ){ return (a<0) ? -a : a; }

/******************************************************************************
* Read ADC's once into buffer
******************************************************************************/
void readNextValue(void){ 
static INT16 value;
    if( adcCount == 0xFF ){ adcCount = 0; return; }
    ADC_GET_VALUE( value ); //value >>= 4;
    ADCIF = 0; 
    adcs[ adcCount ] += value;
    adcCount++;
    if( adcCount < N_CHANNELS ){
        ADC_SINGLE_CONVERSION( adcChannels[ adcCount ] ); 
    }else{
        adcsCounter ++;
        if( adcsCounter == 128 ){
            static float v, t, rOff;
            float *coeff;
            
            adcs[ 0 ] -= adcs[ 1 ]; adcs[ 2 ] -= adcs[ 1 ]; v = ((float)adcs[ 2 ] * 2500.0 ) / ( (float) adcs[0] );
            t = ( page.tk[1] - v )*page.tk[0];
            theTemperature = (INT16)( 100.0 * t ); 
            
            coeff = page.dFk;
            rOff = ( ( ( coeff[0]*t + coeff[1] )*t + coeff[2] )*t + coeff[3] );
    
            df = (INT32) rOff;
            df -= accumulatedDF;
            if( abs32( df ) > 8 ){ /*time2AddDF = TRUE;*/ addDF( df );   calibrate(); }      
            
            adcs[0] = 0; adcs[1] = 0; adcs[2] = 0; adcsCounter = 0;
        }
    }
}
/******************************************************************************
* @fn  addDF
* @brief       This adds function shift to compensate frequency offset
* Parameters:  frequency shuft
* @return void
******************************************************************************/
void addDF(INT32 dFreq){
static INT32 freq;
    INT_GLOBAL_ENABLE(FALSE);
    freq = 1L*FREQ0;               freq += 256L*FREQ1;                 freq += 65536L*FREQ2;    
    freq += dFreq;
    FREQ0 = (BYTE)(freq&0xFF);     FREQ1 = (BYTE)((freq>>8) &0xFF);    FREQ2 = (BYTE)((freq>>16)&0xFF);
    accumulatedDF += dFreq;
    INT_GLOBAL_ENABLE(TRUE);
}
//__________________________________________________________________________________________________________________________
