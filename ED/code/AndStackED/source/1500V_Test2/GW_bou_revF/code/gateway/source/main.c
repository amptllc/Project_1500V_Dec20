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

#define USB_RX_TIMEOUT           10
/*
#define BUFFER_SIZE              137
#define MAX_DATA_BYTES           ((BUFFER_SIZE - HEADER_LENGTH) - CRC_LENGTH)
#define MAX_LENGTH_BYTE          (BUFFER_SIZE - 3)
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
#define ASVOL_NETCMD    0x90
// redefine NULLOP
#define CHANNEL_CMD     0x0F       

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
     BYTE   arr[16]; // 2 for addr and 16 is maximum amount of bytes we can fit in here - used for boot over the air
   } value;
   
   BYTE reg;
   UINT16 seq, bAddr;
   BOOL useReg      :1, 
        useAddr     :1, 
        useValue    :1;
   BYTE repCount;
} CMD;

extern void setup_hopper( BYTE curch ); //, BOOL f_hopper );
extern void hop(void);

void feedTheDog(void);
void sendRequest(void);
void processEDReport(void);
void start2Receive(void);
void parseCommand(void); 
void preparePackage(void);

void computations();
void addDF(INT32 dFreq);

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
    BYTE curKey[ AES_SIZE ];                                                                                // 16
    BYTE curIV [ AES_SIZE ];                                                                                // 16
    BYTE channel;                                                                                           // 1
    
    float dFk[4];                                                                                          // 16
    float tk[2];                                                                                           // 8

    INT16  theDelta;                                                                                        // 2
    INT32  iRef;                                                                                            // 4
    BYTE   dF_Tolerance;
    BOOL   useADC, use30kbod;
    BYTE   power;
    BOOL   long_packets;
    BYTE   mac[ 6 ];
    BOOL   isRxOnly;
    BOOL   isHopper;
    BYTE   edAddr, gwAddr;
    BOOL   useFEC;
    BYTE   version;
    BOOL   longReport;
    BYTE   maxDevs;
    BYTE   bootDelay;
    UINT32 secondsJoinEnabled;
    BOOL   printSupercycle;
    BOOL   f_hopper;
    UINT16 sw;
    BYTE   repeater_datarate;
} page;/* = {                                                                                                   // 68
    0xAAAA, 0, 
    { 'A', 'N', 'D', 'S',  0,  'l', 'a', 'r', 'A', 'u', 'g', 'u', 's', 't',  0,   7 },
    { 'A', 'u', 'g', 'u', 's', 't',  0,   7,  'A', 'u', 'g', 'u', 's', 't',  0,   7 },
    
    255,  //should be 0 in production
    
    { -0.000627, 0.0469, 0.253, -123.0 }, 
    { 0.435, 625.0 }, //{ 75500.0, 0.39 }, 
    5,    121000L,  64, 
    TRUE, TRUE, 
    0xED,
    TRUE,
    { 0,0,0,0,0,0 },
    FALSE, FALSE, 
    2,     1,
    FALSE, 0xE2, TRUE,
    16,
    TRUE, 
    2,
    0, //14L*24L*3600L
    TRUE, 
    0xF0F0
};
*/

BYTE dumb0, def_repeat = 4;

INT16 theTemperature = 0;

UINT16  _sendRequest = 2 * TICKS_IN_SEC,               _calibrationDelay = TICKS_IN_MS-1,
        _timeBetweenMeasurements = TICKS_IN_SEC / 100, _adjustFrequency  = 5*(UINT16)TICKS_IN_SEC, 
        _adcMeasurement = TICKS_IN_SEC / 100;

UINT16  adcCount = 0,          bufferCount = 0,        cmdCycle,                 cmdCount = 1,      
        lastCycle = 0,         sec2USBWatchDog  = 450;

int     lastRssi = 0;
INT32   df, accumulatedDF = 0;
            
BOOL    computeDelta  = FALSE,     
        bufferFilled  = FALSE,     dissolveNetwork = FALSE, isCW = FALSE,               secondGone = FALSE,
        setNewChannel = FALSE,     bunch255        = FALSE, time2Restart = FALSE,       flashDirty = FALSE,         
        needNewPage   = FALSE,    joinEnabled      = FALSE, use_watch_dog = TRUE;

BYTE rpbCount = 0, hopCnt = 0, stage = 0;

#define N_CHANNELS      3
INT32  adcs[   N_CHANNELS ];
INT16  adcsCounter = 0;
__xdata_rom const BYTE  adcChannels[ N_CHANNELS ] = {  
    ADC_REF_AVDD | ADC_12_BIT | ADC_AIN5,    // ext ref         2.5 V
    ADC_REF_AVDD | ADC_12_BIT | ADC_AIN3,    // ext ground
    ADC_REF_AVDD | ADC_12_BIT | ADC_AIN4     // ext Temp
};

INT32   utc = 0;
UINT16  ms  = 0;

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
static SchedulerInterface *exchange_block = (SchedulerInterface __xdata *)EXCHANGE_BUFFER;;
static INT16 *ticks;
static BOOL  *flags;

void mymemset(void *a, BYTE b,  BYTE c){ (*(exchange_block->mymemset))((BYTE *)a, b, c); }
void mymemcpy(void *a, void *b, BYTE c){ (*(exchange_block->mymemcpy))((BYTE *)a, (BYTE *)b, c); }
BOOL mymemcmp(void *a, void *b, BYTE c){ return (*(exchange_block->mymemcmp))((BYTE *)a, (BYTE *)b, c); }
void usb( BYTE *p )                    { (*(exchange_block->usb))( p ); }

float mult( float v, INT16 k ){ return v * ((float) k ); }

UINT32 atou( BYTE *p )                 { return (*(exchange_block->strtoul))( (char *)p, NULL, 10 ); }
UINT32 atoh( BYTE *p )                 { return (*(exchange_block->strtoul))( (char *)p, NULL, 16 ); }

#define temp_buffer     exchange_block->temp 

#define NHIST 25
BYTE idHistory[ 8*NHIST ];
BYTE nullmac[ 6 ]; //={ 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };
BYTE histCnt = 0;

void main_loop( void );

void switchDataRate( BOOL fcc ){
  _freq        = FREQUENCY_FCC_CC2511;
  _datarate    = fcc ? DATA_RATE_4_CC2511 : DATA_RATE_3_CC2511;
  //_slot        = fcc ? ( 40 * TICKS_IN_MS ) : ( 45 * TICKS_IN_MS );
  _loop_delay  = ( 40 * TICKS_IN_MS );
}
#define BARRIER 0xAA
#define PAGEADDR 0x7C00
BOOL readTheLatest( ){ 
BYTE *addr = (BYTE *)PAGEADDR;
    if( addr[0]!=BARRIER || addr[1]!=BARRIER ) return FALSE; 
    mymemcpy( &page, addr, sizeof( page ) ); 
    return TRUE;
}

void configureAdcs( void );
void writePage2Flash(void){
BYTE *addr = (BYTE *)PAGEADDR;
UINT16 pagesize = sizeof( page );
    if( pagesize & 1 ) pagesize++;
    if( exchange_block->transfer ) return;
    (*(exchange_block->clearThePage))( (PAGEADDR>>9)&0xFF ); 
    (*(exchange_block->halWait))(1);
    (*(exchange_block->initiateTransfer))( (BYTE *)&page, addr, pagesize );
    // ticks[CheckFlash] = TICKS_IN_SEC/8;
}

void setupRadio(void){
    if( page.sw == 0xFFFF ) page.sw = 0xF0F0;
    RFST = STROBE_IDLE; 
    switch( page.repeater_datarate ){
        case DATA_RATE_1_CC2511:  case DATA_RATE_2_CC2511:
            radioConfigure( page.repeater_datarate ); MDMCFG1 |= 0x80;
        break;
        default:
            switchDataRate( page.use30kbod );
            radioConfigure( _datarate ); 
            if( page.useFEC && page.use30kbod ) MDMCFG1 |= 0x80; else MDMCFG1 &= 0x7F;
        break;
    }
    PA_TABLE0 = page.power; CHANNR = page.channel;  

    SYNC0 = page.sw & 0xFF; SYNC1 = (page.sw>>8)&0xFF;

    if( page.isRxOnly ) PKTCTRL1 &= 0xFC; // address check off 
    ADDR = page.gwAddr;
}

/*
void calibrateParking( void ){
BYTE i, ch = CHANNR;
    INT_GLOBAL_ENABLE(FALSE);
    for( i = 0; i < 4; i++){
         (*(exchange_block->halWait))( 1 );
         RFST = STROBE_IDLE; 
         CHANNR = exchange_block->parking_channels[ i ];
         RFST = STROBE_CAL; 
         (*(exchange_block->halWait))( 1 );
         exchange_block->fscal_parking_1[i] = FSCAL1;    
         exchange_block->fscal_parking_2[i] = FSCAL2;     
         exchange_block->fscal_parking_3[i] = FSCAL3;
    }
    CHANNR = ch; RFST = STROBE_CAL; 
    INT_GLOBAL_ENABLE(TRUE);
}
*/
/*==== PUBLIC FUNCTIONS ======================================================*/
/******************************************************************************
* @fn  main
* @brief      Main function. Triggers setup menus and main loops for both receiver
*             and transmitter. This function supports both CC1110 and CC2510.
* Parameters:
* @param  void
* @return void
******************************************************************************/
void main_loop( void );
void main(void){
   memset( nullmac, 0xFF, 6 );
   
   usb( "start of main!\n" );
  
   exchange_block->armAdcs          = armAdcs;
   exchange_block->readNextValue    = readNextValue;
   exchange_block->main_loop        = main_loop;
   
   ticks = exchange_block->ticks;
   flags = exchange_block->flags;
     
   ticks[ PreparePackage ]  = _sendRequest - 5*TICKS_IN_MS;
   ticks[ SendRequest ]     = _sendRequest;
   ticks[ FeedTheDog   ]    = TICKS_IN_MS * 125;
   ticks[ Ms ]              = TICKS_IN_MS;
   ticks[ MeasureAdc ]      = 25;

   //exchange_block->parking_channels[0] = 57;
   //exchange_block->parking_channels[1] = 137;
   //exchange_block->parking_channels[2] = 177;
   //exchange_block->parking_channels[3] = 197;
   
   if( !readTheLatest() ){ flashDirty = TRUE; needNewPage = TRUE; }

   setup_hopper( page.channel ); //, page.f_hopper );
   
   setupRadio();

   //calibrateParking();
   nBunches = 1 + page.nDevs / page.maxDevs;

   HAL_INT_ENABLE(INUM_RF,  INT_ON);    // Enable RF general interrupt
   HAL_INT_ENABLE(INUM_ADC, INT_ON);    // enable ADC interrupt

   RFIM    = IRQ_DONE;
   
   loadKey( page.curKey );

   // configugre ADC pins
   configureAdcs( );
   
   while(1) main_loop( );
   
}
BOOL wasWiggling = FALSE, needWiggling = FALSE;
BYTE saved_channel;
void main_loop( void ){
BYTE i;
    exchange_block->bou_loop();
    for( i = 0; i < N_Of_Ticks; i++) if( ticks[i] < 0 ){
        ticks[ i ] = 0;
        switch( i ){
            case FeedTheDog:  WDCTL = 0xA8;       WDCTL = 0x58;   ticks[ FeedTheDog ] = 125*TICKS_IN_MS;              break;
            case PreparePackage: preparePackage();                                                                    break;
            case SendRequest: if ( !page.isRxOnly ){ sendRequest(); needWiggling = ( page.repeater_datarate == 2 ); } break;
            //case CheckFlash:  if( !readTheLatest((BYTE *)&page, PAGESIZE ) ) writePage2Flash();                   break;
            case Hop:         
                if( page.isRxOnly ){ hop(); if( hopCnt-- ) ticks[ Hop ] = lastCycle - (2 * TICKS_IN_MS); }
                else flags[ Hop ] = TRUE; 
            break;
            case Start2Receive:   start2Receive();                                                                    break;
            case MeasureAdc:      if( page.useADC )   armAdcs();                                                      break;    
            case SetupChannel:    if( page.isRxOnly ) CHANNR = page.channel;                                          break;   
            default:                                                                                                  break;
        }
    }
    for( i = 0; i < N_Of_Flags; i++ ) if( flags[i] ){
          flags[ i ] = FALSE;
          switch( i ) {
              case Hop:              flags[ i ] = TRUE;                                          break; 
              case PackageReceived:  processEDReport();                                          break;
              case ParseCommand:     parseCommand();                                             break;
              case AdjustFrequency:  
                if( ticks[ SendRequest ] < 12*TICKS_IN_MS ){ computations();   ticks[ MeasureAdc ] = 50*TICKS_IN_MS;  }
                else                                         flags[ i ] = TRUE;    
              break;
              case DataSent:
                if( needWiggling ){ 
                    needWiggling = FALSE; wasWiggling = TRUE; 
                    INT_GLOBAL_ENABLE(FALSE);
                        RFST = 4;     MCSM0 = 0x04;             // Switch radio to IDLE, calibr off
                        saved_channel = CHANNR; CHANNR ^= 1;
                        exchange_block->radioPktBufferTx[0] = 4;  
                        exchange_block->radioPktBufferTx[1] = 0xFE;
                        exchange_block->radioMode = RADIO_MODE_TX;
                        DMAARM = DMAARM_CHANNEL1;       // Arm DMA channel 1
                        RFST = STROBE_TX;               // Switch radio to TX
                    INT_GLOBAL_ENABLE(TRUE);
                    //ticks[ Start2Receive ] = 0; 
                }else {
                    if( wasWiggling ) { wasWiggling = FALSE; CHANNR = saved_channel; MCSM0 = 0x14; }
                    RFST = 4;
                    ticks[ Start2Receive ] = 10;
                }
              break;
              default:                                                                           break;
          }
    }
    if( exchange_block->ms > 1000 ){
        utc++; exchange_block->ms %= 1000;
        if( page.secondsJoinEnabled > 0 ) page.secondsJoinEnabled--;
        if( page.secondsJoinEnabled > 0 ) joinEnabled = TRUE; else joinEnabled = FALSE;
        if( use_watch_dog && ( sec2USBWatchDog > 0 ) ) {
            sec2USBWatchDog --;
            if( sec2USBWatchDog == 0 ){ HAL_INT_ENABLE(INUM_T1,  INT_OFF); while( 1 ); } // restart
        }
    }    
}
/*==== PRIVATE FUNCTIONS =====================================================*/
//______________________________________________________________________________________________________________________________
/******************************************************************************
* Computes size of the command
******************************************************************************/
#pragma optimize=s 9
BYTE cmdSize(CMD *ptr){
BYTE size = 3; //1+2, cmd + seq;
    if( ( ptr->cmd & 0xF0 ) == SHORT_JOIN ) return 8;

    if( ptr->useAddr )  switch( ptr->cmd & 0x3 ){
        case BROADCAST:             break; // broadcast
        case STRINGADDR: size += 2; break; // string
        case NIDADDR:    size += 2; break; // netId
        case MACADDR:    size += 6; break; // mac 
    }
    if( ( ptr->cmd & 0xF0 ) == ASVOL_NETCMD ) return size+7;
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
//int len(BYTE *ptr){  int cnt = 0; while( *ptr++ ) cnt++; return cnt; }
//____________________________________________________________________________
static BYTE u2abuf[12] = {0};
static BYTE *digits = (BYTE *)"0123456789ABCDEF";
//#pragma optimize=s 9
BYTE* utoa(UINT16 val, int base, BYTE *ptr){
BYTE   i = 10;
    mymemset(u2abuf, 0, 12);
    if( val == 0 ) { *ptr++ = '0'; return ptr; }
	for(; val && i ; --i, val /= base) u2abuf[i] = digits[val % base];
    strcpy( (char *)ptr, (char *)u2abuf+i+1 ); 
	return ptr + (10-i);
}
//____________________________________________________________________________
//#pragma optimize=s 9
inline BYTE* itoa(INT16 val, BYTE *ptr){
  if( val < 0 ){ *ptr++ = '-'; val = -val; }
  return utoa( (UINT16)val, 10, ptr );
}
//____________________________________________________________________________
//#pragma optimize=s 9
BYTE* ultoa(UINT32 val, int base, BYTE *ptr){
BYTE   i = 10;
    mymemset(u2abuf, 0, 12);
    if( val == 0 ) { *ptr++ = '0'; return ptr; }
	for(; val && i ; --i, val /= base) u2abuf[i] = digits[val % base];
    strcpy( (char *)ptr, (char *)u2abuf+i+1 ); 
	return ptr + (10-i);
}
//____________________________________________________________________________
//#pragma optimize=s 9
inline BYTE* ltoa(INT32 val, BYTE *ptr){
  if( val < 0 ){ *ptr++ = '-'; val = -val; }
  return ultoa( (UINT32)val, 10, ptr );
}
//____________________________________________________________________________
//#pragma optimize=s 9
inline BYTE *ftoa(float c, BYTE *ptr){
UINT32 d;
    if( c < 0 ){ *ptr++ = '-'; c = -c; }
    d = (UINT32) c;  ptr = ultoa( d, 10, ptr );
    c -= floor(c);
    *ptr++ = '.';
    if( c <= FLT_MIN ) *ptr++ = '0'; 
    else{
       //c *= 10.0;
       c = mult( c, 10 ); 
       while( c < 1 && c > FLT_MIN ){ *ptr++ = '0'; c = mult( c, 10 ); } //c *= 10.0; }
       d = (UINT32)(c*10000000);
       ptr = ultoa( d, 10, ptr );
    }
    return ptr;
}
//____________________________________________________________________________
//#pragma optimize=s 9
INT16 _pktGetAdc16( BYTE *rpb,  BYTE idx ){
INT16 val = 0; 
    mymemcpy( &val, rpb + 12 + 2*idx, 2 );
    return val;
}
//____________________________________________________________________________
//#pragma optimize=s 9
BYTE *printMac( BYTE *ptr, BYTE *mac ){
register BYTE idx, h;
    for( idx = 0; idx < 6; idx ++ ){
      h = ( (*mac) >> 4 ) & 0xF;     *ptr++ = HEX(h);
      h = (*mac++) & 0xF;            *ptr++ = HEX(h);
    }               
    return ptr;
}
/*****************************************************************************/
#pragma optimize=s 9
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
//#pragma optimize=s 9
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
//#pragma optimize=s 9
static BYTE *rpb;
static BYTE freeBytesCount, devsInBunch;
void preparePackage(void){
BYTE i, j;
BYTE *ptr; 
static BYTE vvv = SETREG | MACADDR | SHORT_VAL;
  
    devsInBunch = 0;
    freeBytesCount = page.long_packets ? (PACKET_LENGTH_GW - 8 - 2) : (PACKET_LENGTH_GW_2 - 8 - 2) ;

    if( setNewChannel && CHANNR != newChannel){
        CHANNR = newChannel;  
        page.channel = CHANNR;
        setup_hopper( page.channel );
        flashDirty = TRUE;
        newChannel = 0xFF;  
        setNewChannel = FALSE;
        flags[ Hop ] = FALSE;
    }
    
    //if( page.nDevs > (bunch+1)*maxDevs ) devsInBunch = maxDevs; else devsInBunch = page.nDevs % maxDevs;

    rpb = exchange_block->radioPktBuffer + rpbCount*PACKET_LENGTH_GW; rpbCount = ( rpbCount + 1 ) & 3;
    mymemset( rpb, 0, PACKET_LENGTH_GW );

    if( bunch255 ){
        devsInBunch = page.bootDelay;
        rpb[0] = curBunch = 0xFF;
    }else{
        devsInBunch = page.maxDevs;
        rpb[0] = bunch; curBunch = bunch;
    }
    
    if( joinEnabled )   rpb[1] = devsInBunch; else rpb[1] = 0x80 | devsInBunch;
    if( page.isHopper ) rpb[1] |= 0x40;       else rpb[1] &= 0xBF;

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
                  case ASVOL_NETCMD:
                      mymemcpy( ptr, cmdBuf[ j ].value.arr,  7 );   ptr+=7;
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
    if( page.isHopper && flags[ Hop ] ){
        BYTE *p;
        hop();    flags[ Hop ] = FALSE;
        if( page.longReport ){
        static __xdata_rom const char long_rep_hopping[] = "\nhopping to ";
            p = (BYTE *)temp_buffer;
            strcpy( (char *)p, long_rep_hopping ); p += 12;
            p = utoa( CHANNR,  10, (BYTE *)p );  *p = 0;
            usb( (BYTE *)temp_buffer );
        }
    }
    if( freeBytesCount > 5 ){ 
        *ptr++ = SETREG | 4; *ptr++ = 0; *ptr++ = 0;  *ptr++ = 0x7F; *ptr++ = CHANNR; *ptr++ = 0;
        freeBytesCount -= 5; 
    }

    loadIV( page.curIV );  
    if( page.long_packets && ( freeBytesCount < 16 ) )   exchange_block->radioPktBufferTx[0] = PACKET_LENGTH_GW;    // Length byte
    else                                                 exchange_block->radioPktBufferTx[0] = PACKET_LENGTH_GW_2;  // Length byte
   
}

INT16 cycle_adjustment = 0;

void sendRequest(void){  
UINT32 curUTC;
UINT16 curMS;
static BYTE round = 0;
BYTE i = 3*TICKS_IN_MS;
    //preparePackage();
    
    cmdCycle = _slot * ( devsInBunch + (joinEnabled?4:1) ) + _loop_delay;    
    if( round++ < 16 ) cmdCycle--;
    if( round ==  25 ) round = 0;
    while( cycle_adjustment && --i ) 
        if( cycle_adjustment > 0 ){ cycle_adjustment--;  cmdCycle--; }else{ cycle_adjustment++;  cmdCycle++; }
    ticks[ SendRequest ]     = cmdCycle;
    ticks[ PreparePackage ]  = cmdCycle - 5*TICKS_IN_MS;
    if( page.isHopper ) ticks[ Hop ] = ticks[ PreparePackage ] - 3;
    
    //__________________________________________________________________________________________________________________________    
    RFST          = 4; 
    PA_TABLE0     = page.power;

    curUTC = utc; curMS = exchange_block->ms % 1000;
    mymemcpy( rpb+2, (BYTE *)&utc, 4 ); 
    mymemcpy( rpb+6, (BYTE *)&curMS, 2 );
    
    encode( exchange_block->radioPktBufferTx[0]-2, rpb, exchange_block->radioPktBufferTx+2 ); 
    exchange_block->radioPktBufferTx[1] = page.edAddr;                                                              // ED address

    // Send the packet
    INT_GLOBAL_ENABLE(FALSE);
        exchange_block->radioMode = RADIO_MODE_TX;
        DMAARM = DMAARM_CHANNEL1;       // Arm DMA channel 1
        RFST = STROBE_TX;               // Switch radio to TX
    INT_GLOBAL_ENABLE(TRUE);

    if( page.printSupercycle && (curBunch == 0) ){
    static __xdata_rom const char print_sc_0[] = "\n=> MAC:\t", print_sc_1[] = "\tCh:\t", print_sc_2[] = "\tT:\t", 
                                  print_sc_3[] = "\tUTC:\t",    print_sc_4[] = "\tms:\t";
        BYTE *ptr;
        ptr = temp_buffer; 
        strcpy( (char *)ptr, print_sc_0 );           ptr+=9;
        ptr = printMac( ptr, page.mac ); 
        strcpy( (char *)ptr, print_sc_1 );           ptr+=5;
        ptr = utoa( CHANNR, 10, (BYTE *)ptr ); 
        strcpy( (char *)ptr, print_sc_2 );           ptr+=4;
        ptr = itoa( theTemperature, (BYTE *)ptr );    
        strcpy( (char *)ptr, print_sc_3 );           ptr+=6;
        ptr = ultoa( curUTC, 10, (BYTE *)ptr );    
        strcpy( (char *)ptr, print_sc_4 );           ptr+=5;
        ptr = itoa( curMS, (BYTE *)ptr );
        *ptr++ = '\n'; *ptr++ = 0;
        usb( temp_buffer );
    }
   
    if( dissolveNetwork ){
        dissolveNetwork = FALSE;
        flashDirty = FALSE; 
        writePage2Flash( ); 
        mymemset( idHistory, 0xFF, sizeof( idHistory ) ); histCnt = 0;
    }
    if( ++bunch >= nBunches ) bunch = 0;
}
/******************************************************************************
* Start to receive data
******************************************************************************/
void start2Receive(void){  
    PKTLEN   = PACKET_LENGTH_ED;              // Packet length.
    exchange_block->radioMode = RADIO_MODE_RX;
    //dmaRadioSetup( mode = RADIO_MODE_RX );  // Set up the DMA to move packet data from radio to buffer
    DMAARM = DMAARM_CHANNEL0;                 // Arm DMA channel 0
    RFST   = STROBE_RX;                       // Switch radio to RX
}
/******************************************************************************
* Receive an Report from End Device, format it into the string and send this string through the USB.
******************************************************************************/
BYTE * printHex( BYTE hex, BYTE *ptr ) { *ptr++ = HEX( ((hex>>4)&0xF) );  *ptr++ = HEX( (hex&0xF) ); return ptr; }
/******************************************************************************
* Receive an Report from End Device, format it into the string and send this string through the USB.
******************************************************************************/
//#pragma optimize=s 9
void processEDReport(void){
signed char off = 0, rssi;
UINT16 ticksLeft = 0;
BOOL output = FALSE;
CMD *cmdPtr;
BYTE *ptr;
BYTE *rpb;
BYTE *nPtr = NULL;
BYTE idx, i = 0, j, h, plen;//, i;
static UINT32 pktUtc;
static UINT16 pktMs;
static BYTE vvv = SETREG | MACADDR | SHORT_VAL;
static __xdata_rom const char ed_report_0[] = "GW\toffset ",
                              ed_report_1[] = "rssi ",
                              ed_report_2[] = "channel ",
                              ed_report_3[] = " ticks\t",
                              ed_report_4[] = " dB\tJoining ",
                              ed_report_5[] = "\tid=",
                              ed_report_6[] = "\nSC",
                              ed_report_7[] = "\nL1:\t",
                              ed_report_8[] = "\nL2:",
                              ed_report_9[] = "OV: ",
                              ed_report_10[] = "mpp on \t",
                              ed_report_11[] = "mpp off\t",
                              ed_report_12[] = "module on\n",
                              ed_report_14[] = "module off\n";

    if( histCounter == -1 ){ histCounter = 0; mymemset( idHistory, 0xFF, sizeof( idHistory ) ); }
    ticks[ Start2Receive ] = 1;
    // Check if the received packet is a valid PER test packet
    plen = idx = exchange_block->radioPktBufferRx[0];
    if( (( idx == PACKET_LENGTH_ED) || (idx == PACKET_LENGTH_GW_2) ) && 
        (exchange_block->radioPktBufferRx[idx + 2] & 0x80)  
    ) { 
        if( exchange_block->radioPktBufferRx[1] == page.gwAddr ) {
            output = TRUE;   i = FALSE; 
        }else if( page.isRxOnly && ( exchange_block->radioPktBufferRx[1] == page.edAddr ) ) {
            output = TRUE;            i = TRUE;
        }
        rpb = exchange_block->radioPktBuffer + PACKET_LENGTH_ED*rpbCount; rpbCount = ( rpbCount + 1 ) & 3;
        loadIV( page.curIV );  decode( idx-2, exchange_block->radioPktBufferRx+2, rpb );
        rssi = exchange_block->radioPktBufferRx[ idx+1 ];
        lastRssi = convertRssiByte( rssi );
        exchange_block->radioPktBufferRx[ idx + 2 ] = 0;
        exchange_block->radioPktBufferRx[0] = 0;    
        exchange_block->radioPktBufferRx[1] = 0;
        off = (INT8)FREQEST;
    }else{
        ptr = exchange_block->temp;                                     *ptr++ = '\n';
        ptr = utoa(  exchange_block->radioPktBufferRx[ 1 ], 10, ptr );  *ptr++ = 0;
        usb(  exchange_block->temp );
    }
    // We don't need our packet buffer anymore, prepare for the next packet
    DMAARM = DMAARM_CHANNEL0;
    RFST   = STROBE_RX;               

    ticksLeft = ticks[ SendRequest ];
    if(output) {
        P1 ^= 2;       
        output = FALSE;
        if( page.isRxOnly && i ){ // another gateway is talking
        BYTE ndev; 
            curBunch = rpb[0]; ndev = rpb[1]; 
            lastCycle = _slot * ( (ndev & NDEV_MASK) + ((ndev & JOIN_MASK)?1:4) ) + _loop_delay;
            
            if( ndev & HOP_MASK ){ ticks[ Hop ]= lastCycle - 2*_slot - 2 * TICKS_IN_MS; ticks[ SetupChannel ] = 10 * lastCycle; hopCnt = 10; }
            else                 { ticks[ SetupChannel ] = 0;   ticks[ Hop ] = 0; }
            if( page.longReport ){
                ptr = (BYTE *)temp_buffer; 
                strcpy( (char *)ptr, ed_report_0 );   ptr+=10;
                ptr = itoa( off, ptr );         *ptr++ = '\t';
                strcpy( (char *)ptr, ed_report_1 );         ptr+=5;
                ptr = itoa(  lastRssi, ptr );   *ptr++ = ' ';  *ptr++ = 'd'; *ptr++ = 'B';  *ptr++ = '\t';
                strcpy( (char *)ptr, ed_report_2 );      ptr+=8;
                ptr = itoa(  CHANNR, ptr );     *ptr++ = '\n'; *ptr++ = 0;
                usb( (BYTE *)temp_buffer );
            }
         }else if( (plen == PACKET_LENGTH_GW_2) && joinEnabled && (rpb[0] == 0xFF) && (rpb[1] == 0xFF) && (rpb[2] == 0xFF) && (rpb[3] == 0xFF)){ // join request
                BYTE newBunch, newId, i = 0;
                BOOL found = FALSE;
                BYTE *hPtr = idHistory;
                
                cmdPtr = cmdBuf;
                rpb += 10;                

                for( j = 0; j <= CMDBUFMSK; j++, cmdPtr++)
                  if( mymemcmp( cmdPtr->addr.mac, rpb, 6 ) == 0  &&
                      ( ( cmdPtr->cmd == vvv ) || ((cmdPtr->cmd & 0xF0) == SHORT_JOIN) ) )   return; // already in the cmd buf

                cmdPtr = getCmdPtr(SETREG, TRUE, TRUE, TRUE);
                if( cmdPtr == NULL ) return; // no slots in cmd buf

                for( i = 0; i < NHIST; i++, hPtr += 8 ) 
                  if( mymemcmp( hPtr, rpb, 6 ) == 0 )                          { found = TRUE; newBunch = hPtr[6];  newId = hPtr[7]; break; }
                  else if( nPtr == NULL && (mymemcmp( hPtr, nullmac, 6 )==0) ) nPtr = hPtr;
                
                if( !found ){
                    if( histCnt == NHIST || nPtr == NULL ) return;
                    newId = page.nDevs % page.maxDevs; newBunch = page.nDevs / page.maxDevs;
                    //while( newBunch >= nBunches ) nBunches++;
                    page.nDevs++; //flashDirty = TRUE; needNewPage = TRUE;
                    nBunches = 1 + page.nDevs / page.maxDevs;
                    
                    mymemcpy( nPtr,   rpb, 6 ); nPtr[6] = newBunch;  nPtr[7] = newId;
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

                ptr = (BYTE *)temp_buffer;                                            *ptr++ = '\n';
                ptr = utoa( ticksLeft, 10, ptr ); strcpy( (char *)ptr, ed_report_3 );   ptr += 7;
                
                rpb -= 6;
                mymemcpy( (BYTE *)&pktUtc, rpb, 4 );
                ptr = ultoa( pktUtc, 10, ptr );                                 *ptr++ = '\t';
                mymemcpy( (BYTE *)&pktMs, rpb+4, 2 );
                ptr = utoa(  pktMs, 10, ptr );                                  *ptr++ = '\t';

                ptr = itoa(  (INT16)off, ptr );                                 *ptr++ = '\t';
                ptr = itoa(  lastRssi, ptr );    
                *ptr++ = 0;
                
              usb( temp_buffer );
                strcpy((char *)temp_buffer, ed_report_4 );  ptr = temp_buffer + 12;                 
                ptr = printMac( ptr, rpb + 6 );
                strcpy((char *)ptr, ed_report_5 ); ptr += 4;     *ptr++ = HEX( newId ); *ptr++ = '(';
                ptr = utoa( newBunch, 10, ptr );    *ptr++ = ')';   *ptr++ = '\n'; *ptr++ = 0;
              usb( temp_buffer );
        }else{
            mymemcpy( (BYTE *)&pktMs, rpb+4, 2 ); pktMs &= 0x3FF;
            if( pktMs >= 1000 ){ // calibration
                  strcpy( (char *)temp_buffer, ed_report_6 ); ptr = (BYTE *)temp_buffer + 3;
                  pktMs = ( pktMs - 1000 );
                  h = pktMs >> 4;   if( h ) *ptr++ = HEX( h ); 
                  *ptr++ = HEX( (pktMs&0xF) );  *ptr++ = ':';       *ptr++ = '\t';  // the scaling coeff #
                  
                  /*if( h >= 5 ){
                      h = ( curBunch >> 4 ) & 0xF;  if( h ) *ptr++ = HEX(h); 
                      h =  curBunch  & 0xF;         *ptr++ = HEX(h); 
                      *ptr++ = ':';                 *ptr++ = HEX( ( ( rpb[ 5 ] >> 2 ) & 0xF) );   *ptr++ = '\t';  
                  }*/
                  
                  ptr = printMac( ptr, (BYTE *)(rpb+6) );     *ptr++ = '\t';  
                  *ptr = 0; usb( (BYTE *)temp_buffer ); 

                  //if( pktMs > 4 ){
                      ptr = temp_buffer; 
                      for(idx = 0;  idx<4;  idx++) ptr = printHex( rpb[idx], ptr ); 
                      for(idx = 12; idx<32; idx++) ptr = printHex( rpb[idx], ptr ); 
                      *ptr= 0; usb( temp_buffer ); 
                  /*}else{ 
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
                              *ptr = 0; usb( temp_buffer ); 
                          }
                      }
                  }*/
            }else{ // data
              if( page.longReport ){
                      strcpy( (char *)temp_buffer, ed_report_7 ); ptr = temp_buffer + 5;
                      ptr = utoa( ticksLeft, 10, ptr);             *ptr++ = '\t'; 
                      ptr = utoa( curBunch, 10, ptr);              *ptr++ = ':';   // bunch
                      *ptr++ = HEX( ((rpb[5]>>2)&0xF) );           *ptr++ = '\t';  // no group, just netId
                      ptr = printMac( ptr, (BYTE *)(rpb+6) );      *ptr++ = '\t';   *ptr++ = 0;
                    usb( temp_buffer );
                      temp_buffer[0] = 0; ptr = (BYTE *)temp_buffer; 
                      mymemcpy( (BYTE *)&pktUtc, rpb, 4 );
                      ptr = ultoa( pktUtc,  10, ptr );              *ptr++ = '\t';
                      ptr = utoa( pktMs,    10, ptr );              *ptr++ = '\t';
                      ptr = itoa( (INT16)off,    ptr );             *ptr++ = '\t';
                      ptr = itoa( lastRssi,      ptr );             *ptr++ = ' ';   *ptr++ = 'd';   *ptr++ = 'B';   *ptr++ = '\t';
                      ptr = itoa( (signed char)rpb[28],  ptr );     *ptr++ = '\t';                             // pkt off
                      ptr = itoa( (signed char)rpb[29], ptr );      *ptr++ = ' ';   *ptr++ = 'd';   *ptr++ = 'B';   *ptr = 0;  // pkt rssi
                    usb( temp_buffer );
                      strcpy( (char *)temp_buffer, ed_report_8); ptr = temp_buffer + 4;
                      for( idx = 0; idx < 8; idx ++ ){ 
                            *ptr++ = '\t'; 
                            switch( idx ){
                                case 0: case 1: case 3:                 ptr = utoa( _pktGetAdc16(rpb, idx), 10, ptr );  break;
                                default:                                ptr = itoa( _pktGetAdc16(rpb, idx), ptr );      break;
                            }
                      }
                      *ptr++ = '\t'; *ptr++ = 0;    
                    usb( temp_buffer );
                      strcpy( (char *)temp_buffer, ed_report_9 ); ptr = (BYTE *)temp_buffer + 4;
                      ptr = itoa( rpb[30],  ptr ); *ptr++ = '\t';  // over voltage
                      *ptr++ = 'O'; *ptr++ = 'C'; *ptr++ = ':'; *ptr++ = ' ';
                      ptr = itoa( rpb[31], ptr );  *ptr++ = '\t';  // over current
                      if( rpb[5] & 0x40 ) strcpy( (char *)ptr, ed_report_10 ); else strcpy( (char *)ptr, ed_report_11 );
                      ptr += 8;
                      if( rpb[5] & 0x80 ) strcpy( (char *)ptr, ed_report_12 ); else strcpy( (char *)ptr, ed_report_14  );
                    usb( temp_buffer );
              }else{
              BYTE *p = &rpb[0];
                    ptr = temp_buffer;
                    *ptr++='\n';   *ptr++ = '|'; 
                    ptr = printHex( curBunch,   ptr );
                    ptr = printHex( off,        ptr );
                    ptr = printHex( lastRssi,   ptr );
                    *ptr++='|';                          // 10
                    for( idx = 0; idx<32; idx++ ){       // 64
                        h = ( (*p) >> 4 ) & 0xF;     *ptr++ = HEX(h);
                        h = (*p++) & 0xF;            *ptr++ = HEX(h);
                    }                                   
                    *ptr++ = 0;
                    usb( temp_buffer );
              }
              if( histCnt ){
                  BYTE *hPtr = idHistory;
                  rpb  += 6;
                  for( i = 0; i < NHIST; i++, hPtr += 8 ) if( mymemcmp( hPtr, rpb, 6 ) == 0 ){
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
}
/******************************************************************************/
BYTE* rollNW( BYTE *ptr ) { 
    while( *ptr ) if( *ptr != ' ' && *ptr != '\t' && *ptr != '\n' && *ptr != '\r' ) ptr++; else break; 
    return ptr;
}
BYTE* rollW( BYTE *ptr ) { 
    while( *ptr ) if( *ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r' ) ptr++; else break; 
    return ptr;
}
BYTE* roll2next(   BYTE *ptr ) {     return rollW( rollNW( ptr ) );  }
/******************************************************************************/
BYTE ch2int(   BYTE ch ) { 
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
        case '*': cmdPtr->cmd |= BROADCAST;                                         return TRUE; // broadcast command to all
        case 's': cmdPtr->cmd |= STRINGADDR; cmdPtr->addr.stringAddr = atoh( ptr ); return TRUE;
        case '#': cmdPtr->cmd |= NIDADDR;    cmdPtr->addr.netId      = atoh( ptr);  return TRUE;
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
*   'c addr bPage'          clear the page, page from 1 to 30 inclusive
*   'b addr L Addr HexSeq'  send n bytes for 'boot over the air' - 
*                               addr - address in network, as always,
*                               L    - length, in hex
*                               Addr - address in flash, hex
*                               HexSeq - data to send
*   'e'                     start sending requests in One second, used for synchronization of several gateways
*   'F Addr'                allow to write to flash for End Devices + store defaults - overV, overC, module, mpp
*   'f Addr'                allow to write to flash for End Devices
*   'G'                     write to flash
*   'h NN'                  set channel - 8 bit command, BYTE for new channel number      
*   'H 0/1'                 hopping on/off
*   'i'                     GW info
*   'J (sNNNN | mNNNN | hNNNN | dNNNN)'     JOIN enabled for nnnn seconds/minutes/hours/days
*   'k NN'                  make nDevs=NN
*   'K N FF.F'              set Gateway coefficient 4 - tk0, 5 - tk1, 0 - dFk1, 1 - dFk2, 2 - dFk3, 3 - dFk4
*   'l 0/1'                 long packets (34 bytes) only
*   'L 0/1                  long reports / short reports
*   'm Addr'                restart measurement - clear buffer on specified ED. Needed for test purposes.
*   'M mac'                 set mac for this GW
*   'n'                     dissolve network - all ED forget about their id's, GW network id goes to 0
*   'N mXXXXXXXXXXXX IIHHEEGGBBRR' ASVOL network command - mac  Id & flag, channel, Ed Addr, Gw Addr, Bunch, group
*   'O'                     get gateway coefficients
*   'o Addr'                get coeeficients
*   'p'                     advance command counter by 1000
*   'P HH'                  set power - PA_TABLE0 to 
*   'Q'                     reset the commands counter - suppose to be done every night (before active day) by middleware
*   'r Addr'                reset processor on end device
*   's Addr Reg Value'      set register in specified end device
*   'T UTC MS'              set up time in UTC + milliseconds
*   'u'                     read from flash
*   'v 0/1'                 fcc (30 kbod, 2410 start, ~250 kHz interchannel space ) mode off/on
*   'V 0/1'                 use FEC
*   'w Addr'                switch to CW mode for selected devices
*   'W'                     switch this gateway to CW mode
*   'X'                     clear the id distribution history
*   'x'                     watchdog start
*   'y'                     disable watchdod
*   'Y 0|1'                 0 means Rx Only, 1 means Normal
*   'Z Addr'                allow for flash rejuvenation on end devices
*   'z N'                   set default amount of repetition


*   'E 0|1'                 echo on/off
*   'R'                     reset gateway
*   'I'
*   'C'
*   'D'
*   'S'
*   'U nnn'                Arm GW for the 'C' command                   
******************************************************************************/
void parseCommand(void){
BYTE *ptr = exchange_block->inBuff; 
CMD *cmdPtr;
BOOL parseError = FALSE;
BYTE reg, cmd;
static BYTE w, y, l;
static UINT32 n;

static __xdata_rom const char 
    s0[] = " Freq",         s1[] = " Tick (mks) ",            s2[] = " Chan ",
    s3[] = " # SPTs ",                s4[] = " bunch size ",
    s5[] = "\nUTC:",        s6[] = " temp ",                  s7[] = " shift ",
    s8[] = " ADCs ",        s9[] = " Power ",                 s10[] = "MAC ",
    s11[]= "Rx Only",       s12[]= "Normal ",                   
    s14[]= "  long",        s15[]=" short",                   s16[]= " packets ",
    s17[]= " TRUE ",        s18[]=" FALSE",                   s19[]= " no",
    s20[]= " hop ",         s21[]=" FEC=",                    s22[]= "GwAddr ",
    s23[]= "EdAddr ",       s24[]="Sync Word: ";
static __xdata_rom const BYTE no_action_was_taken[] = "no action was taken\n",
                              flash_empty[]         = "flash empty\n",
                              array[]               = "array\n",  
                              maxdev_1_16[]         = "maxDevs from 1 to 16",
                              bootDelay_0_16[]      = "bootDelay from 0 to 16",
                              regs_12[]             = "there are only 12 registers in the gateway\n",
                              ampt_bou[]            = "AMPT BOU Radio Gateway ver. F\nDatarate:",
                              command_was[]         = "command was not recognized - ",
                              ERROR[]               = "ERROR\n";

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
                    case '0': bunch255 = FALSE;              break;
                    case '1': bunch255 = TRUE;               break;
                    default:  usb( (BYTE *)no_action_was_taken );  return ;
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
                cmdPtr->reg    = (BYTE)   (atoh( ptr )&0xFF);  
                if( cmdPtr->reg > 16 ){ usb( (BYTE *) "No more the 16 bytes in BOA!" ); break; }
                ptr = roll2next( ptr );
                cmdPtr->bAddr  = (UINT16) (atoh( ptr )&0xFFFF);
                ptr = roll2next( ptr );
                //usb( ptr );
                if( !readHexArray( cmdPtr->value.arr, ptr, cmdPtr->reg ) ){ 
                    cmdPtr->cmd = NULLOP;    parseError = TRUE;  
                    usb( (BYTE *)array );
                }else cmdPtr->repCount = def_repeat;
            break;
            case 'v': 
                ptr = rollW( ptr );
                switch( *ptr++ ){
                    case '0': page.use30kbod = FALSE; break;
                    case '1': page.use30kbod = TRUE;  break;
                    default: usb( (BYTE *)no_action_was_taken );  return ;
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
                    default:  usb( (BYTE *)no_action_was_taken ); return ;
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
                    default:  usb( (BYTE *)no_action_was_taken );  return ;
                }
                PA_TABLE0 = page.power;
            }break;

            case 'B': {
            UINT16 addrs = 1;
                ptr = rollW( ptr );
                addrs = (UINT16) ( atoh( ptr ) & 0xFFFF );
                page.edAddr = addrs & 0xFF;
                page.gwAddr = (BYTE)((addrs>>8)&0xFF);
                ADDR = page.gwAddr;
            }break;

            case 'u':
                if( !readTheLatest( ) ){ usb((BYTE *)flash_empty ); flashDirty = TRUE; needNewPage = TRUE; }
            break;
            case 'K':{
            float *fptr = page.dFk;
                ptr = rollW( ptr );
                // parse register
                reg = atoi( (char *)ptr ); 
                ptr = roll2next( ptr );
                // parse value
                switch( reg ){
                    case 0: case 1: case 2: case 3: case 4:  case 5:     fptr[ reg ] = atof( (char *)ptr );          break;
                    case 6:{ BYTE md = atoi( (char *)ptr );  if( md >=1 && md <=16) page.maxDevs = md; else usb( (BYTE *)maxdev_1_16 );  } break;
                    //case 7: page.f_hopper = atoi( (char *)ptr );  setup_hopper( page.channel, page.f_hopper ); break;
                    case 8:{ BYTE md = atoi( (char *)ptr );  if( md <=16) page.bootDelay = md; else usb( (BYTE *) bootDelay_0_16 );  } break;
                    case 9: page.printSupercycle = atoi( (char *)ptr ); break;
                    case 10:
                        page.sw = (UINT16)atoh( ptr );
                        SYNC0 = page.sw & 0xFF; SYNC1 = ( page.sw >> 8 ) & 0xFF;
                    break;
                    case 11:
                        page.repeater_datarate = (BYTE)atoh( ptr );
                        setupRadio();
                    break;
                    case 12:
                         if( ticks[ SendRequest ] > (cmdCycle/2) ) cycle_adjustment = ticks[ SendRequest ]-cmdCycle;
                         else                                      cycle_adjustment = ticks[ SendRequest ];
                    break;
                    default: usb( (BYTE *) regs_12);                       break;
                }
            }break;
            
            // k - set the amount of devices
            case 'k':{
                ptr = rollW( ptr );
                page.nDevs = atoi( (char *)ptr );
                nBunches = 1 + page.nDevs / page.maxDevs;
                flashDirty = TRUE; needNewPage = TRUE;
            }break;
            
            // 'T UTC MS' set up time in UTC + milliseconds
            case 'T':   case 't': 
                ptr = rollW( ptr );
                utc = (UINT32) strtol((char *)ptr, 0, 10);  
                ptr = roll2next( ptr );
                if( *ptr ){
                    exchange_block->ms  = (UINT16) (strtol((char*)ptr, 0, 10)&0xFFFF);
                    ptr = roll2next( ptr );
                    ticks[ Ms ] = TICKS_IN_MS;
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
            case 's':  // 'S (*   |sNNNN | #NNNN      | mNNNNNNNNNN) NN   (lN     | bNN  |  hNN      | sNNNN | LNNNNNNNN | fNNNNNNNN )
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
                    case 'n': cmdPtr->cmd |= BYTE_VAL;     cmdPtr->value.bval = (BYTE)   atoi( (char *)ptr );          break;
                    case 'b': cmdPtr->cmd |= BYTE_VAL;     cmdPtr->value.bval = (BYTE)   atou( ptr );                  break;
                    case 'h': cmdPtr->cmd |= BYTE_VAL;     cmdPtr->value.bval = (BYTE)   atoh( ptr );                  break;
                    case 'i': case 's': 
                              cmdPtr->cmd |= SHORT_VAL;    cmdPtr->value.ival = (UINT16) (atou( ptr )&0xFFFF);         break;
                    case 'H': cmdPtr->cmd |= SHORT_VAL;    cmdPtr->value.ival = (UINT16) (atoh( ptr )&0xFFFF);         break;
                    case 'l': cmdPtr->cmd |= LONG_VAL;     cmdPtr->value.lval = (UINT32) atou( ptr );                  break;
                    case 'L': cmdPtr->cmd |= LONG_VAL;     cmdPtr->value.lval = (UINT32) atoh( ptr );                  break;                    
                    case 'f': case 'd': cmdPtr->cmd |= FLOAT_VAL;  cmdPtr->value.dval = atof( (char*)ptr );            break;
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
                        n  = atou( ptr );
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
                if( ( reg == FLASH_REFRESH ) && ( ( cmdPtr->cmd & BYTE_VAL ) == BYTE_VAL ) ){
                    cmdPtr->cmd = NULLOP;   parseError = TRUE;
                }
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

            // Set / send AES key / vector 
            case 'A': {
            static BYTE temp_arr[16];
            BYTE what;
            UINT16 addr;
                ptr = rollW( ptr );
                switch( what = *ptr++ ){
                    case 3: case '3': 
                        if( !readHexArray( temp_arr, ptr, 16 ) ) usb( (BYTE *)no_action_was_taken );
                        else{ mymemcpy( page.curKey, temp_arr, 16 ); loadKey( page.curKey ); }
                    break;
                    case 4: case '4':
                        if( !readHexArray( temp_arr, ptr, 16 ) ) usb( (BYTE *)no_action_was_taken );
                        else  mymemcpy( page.curIV, temp_arr, 16 ); 
                    break;
                    case '0':  case '1':
                        ptr = roll2next( ptr );
                        addr = (UINT16) atoh( ptr );   
                        ptr = roll2next( ptr );
                        if( !readHexArray( temp_arr, ptr, 16 ) ){ usb( (BYTE *)no_action_was_taken ); break; }
                        ptr = roll2next( ptr );
                        loadKey( temp_arr );
                        if( readHexArray( temp_arr, ptr, 16 ) ){ 
                            ptr = roll2next( ptr );
                            loadIV( temp_arr );
                            if( readHexArray( temp_arr, ptr, 16 ) ){ 
                              cmdPtr = getCmdPtr( what=='1' ? SET_NEXT_IV : SET_NEXT_KEY, FALSE, TRUE, FALSE ); 
                              encode( 16, temp_arr, cmdPtr->value.arr );
                              cmdPtr->cmd |= NIDADDR;    cmdPtr->addr.netId = addr;
                            }else usb( (BYTE *)no_action_was_taken ); 
                        }else usb( (BYTE *)no_action_was_taken );
                        loadKey( page.curKey );
                    break;
                }
            }break;
            //'N mXXXXXXXXXXXX IIHHEEGGBBRR' ASVOL network command - mac  Id & flag, channel, Ed Addr, Gw Addr, Bunch, group
            case 'N':
                cmdPtr = getCmdPtr( ASVOL_NETCMD, FALSE, TRUE, FALSE );
                ptr = rollW( ptr ); 
                if( !parseAddr( ptr, cmdPtr ) ) { parseError = TRUE; *ptr = 0; break; }
                ptr = roll2next( ptr );
                readHexArray( cmdPtr->value.arr, ptr, 7 );
                printMac( temp_buffer, cmdPtr->value.arr ); 
                temp_buffer[12] = '\n'; temp_buffer[ 13 ] = 0;
                usb( temp_buffer );
            break;

            case 'a':
                ptr = rollW( ptr );
                switch( *ptr++ ){
                    case '0': page.useADC = FALSE;                           break;
                    case '1': page.useADC = TRUE;  ticks[ MeasureAdc ] = 50; break;
                    default: usb( (BYTE *)no_action_was_taken ); break;
                }
            break;
            
            case 'H':  
                ptr = rollW( ptr );
                switch( *ptr++ ){
                    case '0': page.isHopper = FALSE; CHANNR = page.channel;                 break;
                    case '1': page.isHopper = TRUE;  flags[Hop] = FALSE;  ticks[ Hop ] = 0; break;
                    default: usb( (BYTE *)no_action_was_taken ); break;
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
                            CHANNR = cmdPtr->value.bval; 
                            cmdPtr->reg = 0; cmdPtr->repCount = 0; cmdPtr->cmd = NULLOP; 
                            setup_hopper( page.channel  = cmdPtr->value.bval ); //, page.f_hopper );
                        }
                    }else{
                        newch = -newch;
                        setup_hopper( newch & 0xFF ); //, page.f_hopper );
                        CHANNR = page.channel = newch & 0xFF; 
                    }
                }
            }break;

            // reset command count
            case 'Q':  case 'q':    cmdCount = 1;                                   break;
            case 'p':  cmdCount >>= 10; cmdCount++; cmdCount <<=10;                 break;
            case 'G': // write the gateway information to flash
                writePage2Flash();
            break;
            case 'X':   mymemset( idHistory, 0xFF, sizeof( idHistory ) ); histCnt = 0;    break;
            case 'x':   sec2USBWatchDog = 450;                                 break;
            case 'y':   use_watch_dog = FALSE;                                 break;
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
            case 'n':    if( reg == 0  ){  
                reg = DISSOLVE_NETWORK;  
                page.nDevs = 0; nBunches = 0;  
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

            case 'e': ticks[ SendRequest ] = TICKS_IN_SEC; break;
            
            case 'O':{
                BYTE *ptr;
                BYTE i;
                ptr = (BYTE *)temp_buffer;  
                for( i = 0; i < 6; i++) { ptr = ftoa( page.dFk[i], ptr );  *ptr ++ = '\t';  }
                *ptr++ = '\n'; *ptr = 0;
                usb( (BYTE *) temp_buffer );
            }break;
            
            // info about gateway
            case 'i':{ 
            BYTE *ptr;
                usb((BYTE *) ampt_bou ); 
                
                if( page.repeater_datarate == 1 )      ptr = ultoa( 500,  10, (BYTE *)temp_buffer );  
                else if( page.repeater_datarate == 2 ) ptr = ultoa( 250,  10, (BYTE *)temp_buffer );  
                else                                   ptr = ultoa( _datarate,  10, (BYTE *)temp_buffer );  

                strcpy( (char *)ptr, s0); ptr += strlen( s0 );
                ptr = ultoa( _freq,      10, (BYTE *)ptr );          strcpy( (char *)ptr, s1); ptr += strlen( s1 );
                ptr = utoa( MKS_IN_TICK, 10, ptr);                   strcpy( (char *)ptr, s2); 
                usb( (BYTE *)temp_buffer );
                ptr = utoa( page.channel, 10, (BYTE *)temp_buffer ); strcpy( (char *)ptr, s3);  ptr += strlen( s3 );
                *ptr = 0;
                usb( (BYTE *)temp_buffer );
                
                ptr = utoa( page.nDevs, 10, (BYTE *)temp_buffer );   strcpy( (char *)ptr, s4); ptr += strlen( s4 );
                ptr = utoa( page.maxDevs,    10, ptr );      
                strcpy( (char *)ptr, s5 );                           ptr += strlen( s5 );
                ptr = ultoa( utc, 10, ptr );                         *ptr++ = ' '; 
                ptr = utoa(  exchange_block->ms,  10, ptr );         *ptr++ = ' '; 
                if( page.isRxOnly )     strcpy( (char *)ptr, s11 ); else strcpy( (char *)ptr, s12 );
                ptr += 7;  
                if( page.long_packets ) strcpy( (char *)ptr, s14 );
                else                    strcpy( (char *)ptr, s15 );
                ptr += 6; strcpy( (char *)ptr, s16 );
                usb( (BYTE *)temp_buffer );
                
                ptr = (BYTE *) temp_buffer;
                strcpy( (char *)ptr, s6 ); ptr+= strlen( s6 );
                ptr = itoa( theTemperature, ptr );              *ptr++ = '\t';
                strcpy( (char *)ptr, s7 ); ptr+= strlen( s7 );
                ptr = ltoa( accumulatedDF, ptr );
                strcpy( (char *)ptr, s8 ); ptr+= strlen( s8 );         
                strcpy( (char *)ptr, page.useADC ? s17 : s18 ); ptr += 6;
                strcpy( (char *)ptr, s9 ); ptr+= strlen( s9 );         
                ptr = utoa( PA_TABLE0, 16, ptr );
                *ptr++ = '\n'; 
                strcpy( (char *)ptr, s10 ); ptr+= strlen( s10 ); 
                ptr = printMac( ptr, page.mac ); 
                *ptr++ = 0;
                usb( temp_buffer );
                
                ptr = temp_buffer;
                if( !page.isHopper ){ strcpy( (char *)ptr, s19 ); ptr+=3; } 
                else{  *ptr++ = ' ';  if( page.f_hopper ) *ptr++ = 'F'; else *ptr++ = 'E'; }
                strcpy( (char *)ptr, s20 );
                ptr+= 5;
                strcpy( (char *)ptr, s21 ); ptr+=5; 
                *ptr++ = ( MDMCFG1 & 0x80 )?'1':'0'; *ptr+=' ';
                strcpy( (char *)ptr, s22 ); ptr+=7;
                ptr = itoa( page.gwAddr, ptr );              *ptr++ = '\t';
                strcpy( (char *)ptr, s23 ); ptr+=7;
                ptr = itoa( page.edAddr, ptr );              
                *ptr++ = '\n'; 
                *ptr++ = 0;
                usb( temp_buffer );
                usb( (BYTE *)s24 );
                ptr = temp_buffer;
                *ptr++ = HEX(((SYNC1>>4)&0xF)); *ptr++ = HEX(((SYNC1)&0xF)); *ptr++ = HEX(((SYNC0>>4)&0xF)); *ptr++ = HEX(((SYNC0)&0xF));
                *ptr++ = '\n'; 
                *ptr++ = 0;
                usb( temp_buffer );
                if( !use_watch_dog )              usb( "Watchdog disabled\n" );
            }break;
            case '\n': case '\r': case ' ': case '\t': case 0: break;

            default: 
              usb((BYTE *) command_was ); 
              if( cmd != 0 ) temp_buffer[0] = cmd; else temp_buffer[0] = ' ';
              temp_buffer[1] = '(';  temp_buffer[2] = HEX(((cmd>>4)&0xF));  temp_buffer[3] = HEX((cmd&0xF)); temp_buffer[4] = ')';
              temp_buffer[5] = '\n'; temp_buffer[6] = 0;
              usb( (BYTE *)temp_buffer );
            break;
        }
    if( parseError ) usb((BYTE *)ERROR);
    (*(exchange_block->clear_inbuff))();
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
    ticks[ MeasureAdc ] = 50; //_adcMeasurement;  // 5ms
    ADC_SINGLE_CONVERSION( adcChannels[ 0 ] ); 
    ADCIF = 0; 
}
void computations(){
static float v, t, rOff;
float *coeff;
BYTE idx;    
    adcs[ 0 ] -= adcs[ 1 ]; adcs[ 2 ] -= adcs[ 1 ]; v = mult((float)adcs[ 2 ], 2500 ) / ( (float) adcs[0] );
    t = ( page.tk[1] - v )*page.tk[0];
    theTemperature = (INT16)mult( t, 100 ); 
    
    coeff = page.dFk;
    for( idx = 0, rOff = *coeff++; idx < 3; idx++) rOff = rOff*t + (*coeff++);
    //rOff = ( ( ( coeff[0]*t + coeff[1] )*t + coeff[2] )*t + coeff[3] );

    df = (INT32) rOff;
    df -= accumulatedDF;
    if( abs32( df ) > 8 ) addDF( df );   
    
    adcs[0] = 0; adcs[1] = 0; adcs[2] = 0; adcsCounter = 0;
}
/******************************************************************************
* Read ADC's once into buffer
******************************************************************************/
void readNextValue(void){ 
static INT16 value;
    if( adcsCounter > 127 ) return;
    if( adcCount == 0xFF ){ adcCount = 0; return; }
    ADC_GET_VALUE( value ); //value >>= 4;
    ADCIF = 0; 
    adcs[ adcCount ] += value;
    adcCount++;
    if( adcCount < N_CHANNELS ){
        ADC_SINGLE_CONVERSION( adcChannels[ adcCount ] ); 
    }else{
        adcsCounter ++;
        if( adcsCounter == 128 ) flags[ AdjustFrequency ] = TRUE;
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
    freq = FREQ0;               freq += 256L*FREQ1;                 freq += 65536L*FREQ2;    
    freq += dFreq;
    FREQ0 = (BYTE)(freq&0xFF);     FREQ1 = (BYTE)((freq>>8) &0xFF);    FREQ2 = (BYTE)((freq>>16)&0xFF);
    accumulatedDF += dFreq;
    INT_GLOBAL_ENABLE(TRUE);
}
//__________________________________________________________________________________________________________________________
