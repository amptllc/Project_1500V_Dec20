/*==== DECLARATION CONTROL ===================================================*/
/*==== INCLUDES ==============================================================*/

#include "hal_main.h"
#include "main.h"
#include "parameter_block.h"
#include "commands.h"
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>

#define MKS_IN_TICK     500
#define TICKS_IN_SEC    2000
#define TICKS_IN_MS     2
/*
#define MKS_IN_TICK     200
#define TICKS_IN_SEC    5000
#define TICKS_IN_MS     5
*/
//UINT32  _freq         = FREQUENCY_FCC_CC2511;
INT16  _slot         = ( 40 * TICKS_IN_MS );
INT16  _join_slot    = ( 24 * TICKS_IN_MS );
//#define  _slot         ( 40 * TICKS_IN_MS )
//#define  _join_slot    ( 24 * TICKS_IN_MS )
INT16  _gw_delay     = ( 30 * TICKS_IN_MS );
INT16  _loop_delay   = ( 40 * TICKS_IN_MS );
//#define  _gw_delay      ( 30 * TICKS_IN_MS )
//#define  _loop_delay     ( 40 * TICKS_IN_MS )
BYTE    _datarate     = DATA_RATE_4_CC2511;

//#define _mppCycle            ( TICKS_IN_SEC / 16 ) 
#define _mppCycle            ( 60 * TICKS_IN_MS ) 
#define _dogFeeding          ( TICKS_IN_SEC / 16 )
#define _adjustFrequency     ( (UINT16)TICKS_IN_SEC * 2 )
#define _firstAdcMeasurement ( TICKS_IN_SEC / 50 )
//#define _adcMeasurement      ( TICKS_IN_SEC / 100 )
#define _adcMeasurement      ( 4 * TICKS_IN_MS )
#define _calibrationDelay    ( TICKS_IN_MS  )

//void configureAdcs(void);

//void feedTheDog(void);
//BYTE clearAndHop( BYTE p );
void tickWait(BYTE wait);
void rc(void);
void prepareData(void);
void sendData(BYTE *buffer, BOOL is500 );
void armAdcs(void);
void readNextValue(void);
void parseGWPackage(void);
//void calibrate(void);
void computeADCs(BOOL timeout, BOOL accumulateEnergy);
void prepareJoinRequest(void);
void prepareScaling(void);
//void prepareFlashCheck(void);
void setup500( BOOL flag );
BYTE *nextSlot(void);
void switchTo255(void);

void parseCommands(BYTE l);
void setByte( BYTE reg, BYTE val );

extern void flashCallback(void);
extern void setup_hopper(BYTE curch);
extern void hop(void);

extern void wait_aes(void);
extern void load(const unsigned char what, char *ptr);
extern void code(    unsigned char what, unsigned char size,  char *from, char *to );
//extern BOOL initiateTransfer(BYTE *ptr, BYTE *addr, UINT16 len);
extern BOOL initiateTransfer(BYTE *ptr, BYTE *addr, BYTE len);
//extern void writeTheLatest(BYTE *ptr, BYTE len);
extern BOOL readTheLatest( BYTE *ptr, BYTE len);

//#ifdef MidString
//void compute98(void);
//#endif

#define loadKey( key )                  { load( 0x04, key );     wait_aes(); }
#define loadIV(  iv )                   { load( 0x06, iv  );     wait_aes(); }
#define encode( size,  from, to )       code( 0x00, size, from, to );
#define decode( size,  from, to )       code( 0x02, size, from, to );

// must be even
ParameterBlock page;
//float energy       = 0.0;
//float deltaEnergy  = 0.0;
BYTE *base_ptr = NULL;

//#define PAGESIZE 240
#define PAGESIZE 242
#define PAGEADDR 0x5C00

//#define ENERGY_WRITE_LIMIT   ((BYTE *) (PAGEADDR + 0x3F0) )
#ifdef MidString 
    #define KE (1E-3*20.0*1E-3*0.125 / 3600.0)
#else
    #define KE (1E-3*2.0*1E-3*0.125  / 3600.0)
#endif
BYTE  search_stat = 2;

float invVref0 = 0;
INT8 theRealOffset  = 0;

/*==== PUBLIC FUNCTIONS ======================================================*/
#define BUFFER_SIZE 8
#define BUFFER_MASK 0x7    
#define N_CHANNELS  9

UINT16 adcBuf  [ N_CHANNELS ][ BUFFER_SIZE    ];
INT32  adcs[     N_CHANNELS ];
UINT16 adcs16[   N_CHANNELS ];

UINT16 adcsTemp[ N_CHANNELS ];
BYTE tempAdcsCount = 0;

extern BYTE mdm_rate1[6];

UINT16            
       ticks2ProcessCmd = 0,       
       cmdCount = 0,
       lastCycle   = (40 * TICKS_IN_MS * 21),
       cyclesTo500 = 120, cyclesTo10 = 220, cyclesToStep500Channel = 20, cycles2Step10Channel = 30,
       cycles2SoftKick = 8, cycles2HardKick = 80, cyclesFromStart = 0;
UINT16 temperatureProtectionDelay, decodeDelay;

BYTE   curChannel = 0,  curBunch = 0,  /*maxBunch = 0,*/        
       bufferCount = 0, adcCount = 0,  reportScaling = 0,       joinRequestWasSend = 0,
       resetReason,     theOC,         setOC,
       reportFlashCheck = 0,           calibrationRepeat = 4,   cyclesToHop = 0, 
       bunchCount = 0,                 got_gw = 0,              isOnTestStand = 0,
       oscWhatToWaitFor = SetOscLowCurrentOnlyDelay,
       ocShortCircuitLimit = 255; //, page2clear = 255;

// Pointer to memory location tracking whether module can turn on,                                              
// and the three acceptable values. Module turning off because of                                               
// temperature is not tracked.  Pointer to memory location is                                                   
// constant.                                                                                                    
BYTE * const ModuleOffRememberedPtr = ((BYTE *) 0xFFFD);
BYTE ModuleStatusOff = 0xE7;
BYTE ModuleStatusOffFetFailure = 0x7E;
BYTE ModuleStatusCanTurnOn = 0x00;

BOOL   enable2SendData  = FALSE,       txDisabled = FALSE,                    alignMPP = TRUE,     //cleanTxBuffer   = FALSE,
       from_flash = FALSE,             /*zeroPackageWasNotReceived = FALSE,*/ turnOnFlag = FALSE,  isCalibrationPackage = FALSE,
       was_listening = FALSE, rcgen = FALSE,
       ch255 = FALSE,                  packagePrepared = FALSE;//,                  real_cw_mode = FALSE;
INT16 gnd_shift = -600;

//UINT16 Vout98 = 0; 
//INT16  Cur98  = 0;

UINT32 utc = 0;
UINT16 ms  = 0;
//UINT16 ticks2Prepare = 0;

UINT32 utcLast = 0;
UINT16 msLast = 0,  processingTime, pt;

UINT16 mdmcfg4_cycles = 0;

// variables to control turning module on/ "track" FET failures                                             
static BOOL fetFailure = FALSE; // static variable indicating if a FET failure recognized
BOOL checkForFETFailure = TRUE; // once FET failure recognized, no longer check unless restart
// netValue made global so fetFailure condition can access/change reported "status"
static UINT16 netValue = MainNetValueStandard;

static __xdata_rom const UINT16 cyclesDefaults[] = {180, 30, 3, 4, 8, 80};
static __xdata_rom const BYTE default_pb[] = {
/*0xAA, 0xAA, 0x00, 0x00, 0xFF, 0xFF, 0x08, 0x76, 0x40, 0x03, 0x62, 0x7B, 0x71, 0xAA, 0x35, 0xBA, 
0xD3, 0xBC, 0x63, 0x3D, 0x91, 0xED, 0x1C, 0x3F, 0x00, 0x00, 0x34, 0xC3, 0x00, 0x00, 0x1C, 0x45, 
0x95, 0xBF, 0xD6, 0x34, 0x00, 0x00, 0x92, 0x42, 0x85, 0xEB, 0x5D, 0x41, 0x00, 0xC0, 0xD0, 0xC4, 
0x9A, 0x99, 0x49, 0x41, 0x00, 0x00, 0x00, 0x00, 0x8F, 0xC2, 0xFD, 0x40, 0x00, 0x00, 0x12, 0xC4,
0x0A, 0xD7, 0x47, 0x41, 0x00, 0x00, 0x00, 0x00, 0x52, 0xB8, 0xDE, 0x3E, 0x00, 0x40, 0x1C, 0x44, 
0xEC, 0x51, 0xF8, 0x40, 0x00, 0xC0, 0x13, 0xC4, 0xF0, 0xF0, 0x18, 0xFC, 0x00, 0x00, 0x00, 0x64, 
0x41, 0x4E, 0x44, 0x53, 0x00, 0x6C, 0x61, 0x72, 0x41, 0x75, 0x67, 0x75, 0x73, 0x74, 0x00, 0x07, 
0x41, 0x75, 0x67, 0x75, 0x73, 0x74, 0x00, 0x07, 0x41, 0x75, 0x67, 0x75, 0x73, 0x74, 0x00, 0x07, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xAE, 0x4C, 0x09, 0x53, 0xAE, 0x4C, 0x09, 0x53, 0xAE, 0x4C, 0x09, 
0x53, 0xAC, 0xC5, 0xA7, 0x37, 0x17, 0xB7, 0x51, 0x38, 0x01, 0x01, 0xD2, 0x82, 0xFF, 0x02, 0x01, 
0xFE, 0xFF, 0x17, 0x01, 0xA4, 0x9A, 0x38, 0xFF, 0x28, 0x00, 0x1E, 0x00, 0x03, 0x00, 0x04, 0x00, 
0x08, 0x00, 0x50, 0x00, 0x3F, 0x08, 0xF1, 0x00, 0xF1, 0x00, 0xFE, 0x89, 0xFF, 0xFF, 0x19, 0xC8, 
0x6E, 0x64, 0x5A, 0x0A, 0x07, 0x0B, 0x3C, 0x0A, 0x00, 0x00, 0x0A, 0x00, 0x01, 0x00, 0x00, 0x00, 
0x55, 0x55 */
// channel 255, ed/gw 0201, std key, bunch 0, netid 0
/* 0xAA, 0xAA, 0x00, 0x00, 0xFF, 0xFF, 0x2C, 0x76, 0x40, 0x02, 0x27, 0x44, 0x71, 0xAA, 0x35, 0xBA,
0xD3, 0xBC, 0x63, 0x3D, 0x91, 0xED, 0x1C, 0x3F, 0x00, 0x00, 0x48, 0xC3, 0x00, 0x00, 0x1C, 0x45, 
0x95, 0xBF, 0xD6, 0x34, 0x00, 0x00, 0x92, 0x42, 0x85, 0xEB, 0x5D, 0x41, 0x00, 0xC0, 0xD0, 0xC4, 
0x66, 0x66, 0x4A, 0x41, 0x00, 0x00, 0x00, 0x00, 0x8F, 0xC2, 0xFD, 0x40, 0x00, 0x00, 0x12, 0xC4, 
0x0A, 0xD7, 0x47, 0x41, 0x00, 0x00, 0x00, 0x00, 0x52, 0xB8, 0xDE, 0x3E, 0x00, 0x40, 0x1C, 0x44, 
0xEC, 0x51, 0xF8, 0x40, 0x00, 0xC0, 0x13, 0xC4, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 
0x41, 0x4E, 0x44, 0x53, 0x00, 0x6C, 0x61, 0x72, 0x41, 0x75, 0x67, 0x75, 0x73, 0x74, 0x00, 0x07, 
0x41, 0x75, 0x67, 0x75, 0x73, 0x74, 0x00, 0x07, 0x41, 0x75, 0x67, 0x75, 0x73, 0x74, 0x00, 0x07, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0xF0, 0x00, 0x00, 0x00, 0xAA, 
0xFF, 0x78, 0x80, 0x4D, 0xFF, 0xCB, 0x48, 0x66, 0x54, 0xCB, 0x48, 0x66, 0x54, 0xCB, 0x48, 0x66, 
0x54, 0xAC, 0xC5, 0xA7, 0x37, 0x17, 0xB7, 0x51, 0x38, 0x01, 0x01, 0x64, 0x64, 0xFF, 0x02, 0x01, 
0xFF, 0xFF, 0x17, 0x01, 0xA4, 0x9A, 0x38, 0xFF, 0x28, 0x00, 0x1E, 0x00, 0x03, 0x00, 0x04, 0x00, 
0x08, 0x00, 0x50, 0x00, 0x3F, 0x08, 0xF1, 0x00, 0xF1, 0x00, 0xFF, 0x89, 0xFF, 0xFF, 0x19, 0xC8, 
0x6E, 0x64, 0x5A, 0x0A, 0x07, 0x0B, 0x3C, 0x0A, 0x18, 0xFC, 0x0A, 0x00, 0x01, 0x00, 0x00, 0x00, 
0x55, 0x55 */
// New in code parameter block 09/15/15, defaults according to manufacturing
// 1st Line : barrier through dFk[0]
// 2nd Line : dFk[1] through vrefPolynom[0] (dFk[3] changed)
// 3rd Line : vrefPolynom[1] through linearK[0][1] (linearK[0][0], [0][1] changed)
// 4th Line : linearK[1][0] through linearK[2][1] (linearK[1][0], [2][0],[2][1] changed)
// 5th Line : linearK[3][0] through linearK[4][1] (linearK[3][0], [3][1] changed)
// 6th Line : linearK[5][0] through linearK[6][1] (all four values changed)
// 7th Line : curKey[AES_SIZE]  
// 8th Line : curIV[AES_SIZE] 
// 9th Line : fetFailureCount through oscLowCurrentOnlyDelay (only oscLowCurrentLimitInmA stayed same)
// 10th Line: tests[4] through shortCircuitLevel
// 11th Line: reserved through 3rd byte of utcHigh (did not change the 3 times values)
// 12th Line: 4th byte of utcHigh through gwAddr  (should oc change?)
// 13th Line: repeaterChannel through defCyclesToStep10Channel
// 14th Line: defCycles2SoftKick through ov_startup (changed ov_startup)
// 15th Line: t_mod_off through vin_switch_off
// 16th Line: post_barrier
0xAA, 0xAA, 0x00, 0x00, 0xFF, 0xFF, 0x2C, 0x76, 0x40, 0x02, 0x27, 0x44, 0x71, 0xAA, 0x35, 0xBA,
0xD3, 0xBC, 0x63, 0x3D, 0x91, 0xED, 0x1C, 0x3F, 0x00, 0x00, 0x75, 0xC3, 0x00, 0x00, 0x1C, 0x45, 
0x95, 0xBF, 0xD6, 0x34, 0x00, 0x00, 0x92, 0x42, 0xB8, 0x1E, 0xDD, 0x41, 0x00, 0xC0, 0x5A, 0xC5, 
0xE1, 0x7A, 0xC8, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x40, 0x00, 0x80, 0x0E, 0xC4, 
0x8F, 0xC2, 0xC7, 0x41, 0x00, 0x00, 0x6B, 0xC3, 0x52, 0xB8, 0xDE, 0x3E, 0x00, 0x40, 0x1C, 0x44, 
0xCD, 0xCC, 0xFC, 0x40, 0x00, 0x00, 0x05, 0xC4, 0xC3, 0xF5, 0xF8, 0x40, 0x00, 0x80, 0x12, 0xC4, 
0x41, 0x4E, 0x44, 0x53, 0x00, 0x6C, 0x61, 0x72, 0x41, 0x75, 0x67, 0x75, 0x73, 0x74, 0x00, 0x07, 
0x41, 0x75, 0x67, 0x75, 0x73, 0x74, 0x00, 0x07, 0x41, 0x75, 0x67, 0x75, 0x73, 0x74, 0x00, 0x07, 
0x20, 0x4E, 0xBC, 0x02, 0x50, 0x00, 0x00, 0x00, 0x30, 0x75, 0xF0, 0x00, 0xB0, 0x04, 0xF8, 0x7F, 
0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0xF0, 0x00, 0x00, 0x00, 0xAA, 
0xFF, 0x78, 0x80, 0x4D, 0xFF, 0xCB, 0x48, 0x66, 0x54, 0xCB, 0x48, 0x66, 0x54, 0xCB, 0x48, 0x66, 
0x54, 0xAC, 0xC5, 0xA7, 0x37, 0x17, 0xB7, 0x51, 0x38, 0x01, 0x01, 0x64, 0x64, 0xFF, 0x02, 0x01, 
0xFF, 0xFF, 0x17, 0x01, 0xA4, 0x9A, 0x38, 0xFF, 0x28, 0x00, 0x1E, 0x00, 0x03, 0x00, 0x04, 0x00, 
0x08, 0x00, 0x50, 0x00, 0x3F, 0x08, 0xF1, 0x00, 0xF1, 0x00, 0xFF, 0x89, 0xFF, 0xFF, 0x19, 0x46, 
0x6E, 0x64, 0x5A, 0x0A, 0x07, 0x0B, 0x3C, 0x0A, 0x18, 0xFC, 0x0A, 0x14, 0x01, 0x00, 0x00, 0x00, 
0x55, 0x55
};

//static INT32 counts1[5] = {0L,0L,0L,0L,0L};
//static INT32 counts2[5] = {0L,0L,0L,0L,0L};

SchedulerInterface *si = (SchedulerInterface *)0xF500;

static BOOL  *flags;// = si->flags;
static INT16 *ticks;// = si->ticks;

//float safe_vin;
//UINT16 safe_vin_16;
//UINT16 VinTurnOn_16, VinShutOff_16, VinDisableRadio_16;

#define POWER   1
#define CURRENT 2
#define EXT_GND 3
#define REF     4

#define TINT    7
#define TEXT    6

#define Iin2    8

/*
const BYTE  adcChannels[ N_CHANNELS ] = {  
  // Vout  0                                         Pin   1                                        Iout  2      
    ADC_REF_P0_7   | ADC_12_BIT | ADC_AIN1,    ADC_REF_P0_7   | ADC_12_BIT | ADC_AIN2,    ADC_REF_P0_7   | ADC_12_BIT | ADC_AIN3,    
  // Gnd ext 3                                       Ref 4
    ADC_REF_P0_7   | ADC_12_BIT | ADC_AIN4,    ADC_REF_P0_7   | ADC_12_BIT | ADC_AIN6,    
  // Vin  5                                          Text  6                          
    ADC_REF_P0_7   
| ADC_12_BIT | ADC_AIN0,    ADC_REF_P0_7   | ADC_12_BIT | ADC_AIN5,    
  // Tin 7
    ADC_REF_P0_7   | ADC_12_BIT | ADC_TEMP_SENS
    //ADC_REF_1_25_V | ADC_12_BIT | ADC_TEMP_SENS
};
*/

//#define MidString - defined in projects

#ifdef MidString
  #define REFERENCE       ADC_REF_AVDD
  #define REFERENCE_PIN   ADC_AIN7
#else
  #define REFERENCE       ADC_REF_P0_7
  #define REFERENCE_PIN   ADC_AIN6
#endif

const BYTE  adcChannels[ N_CHANNELS ] = {  
  // Vout  0                                         Pin   1                                        Iout  2      
    REFERENCE | ADC_12_BIT | ADC_AIN1,      REFERENCE | ADC_12_BIT | ADC_AIN2,    REFERENCE | ADC_12_BIT | ADC_AIN3,    
  // Gnd ext 3                                       Ref 4
    REFERENCE | ADC_12_BIT | ADC_GND,       REFERENCE | ADC_12_BIT | REFERENCE_PIN,    
  // Vin  5                                          Text  6                          
    REFERENCE | ADC_12_BIT | ADC_AIN0,      REFERENCE | ADC_12_BIT | ADC_AIN5,    
  // Tin 7                                           Iin2 8
    REFERENCE | ADC_12_BIT | ADC_AIN4,       REFERENCE | ADC_12_BIT | ADC_AIN6
};
/*
#else
const BYTE  adcChannels[ N_CHANNELS ] = {  
  // Vout  0                                        Pin   1                                        Iout  2      
    ADC_REF_P0_7 | ADC_12_BIT | ADC_AIN1,      ADC_REF_P0_7 | ADC_12_BIT | ADC_AIN2,    ADC_REF_P0_7   | ADC_12_BIT | ADC_AIN3,    
  // Gnd ext 3                                      Ref 4
    ADC_REF_P0_7 | ADC_12_BIT | ADC_AIN4,      ADC_REF_P0_7 | ADC_12_BIT | ADC_AIN6,    
  // Vin  5                                         Text  6                          
    ADC_REF_P0_7 | ADC_12_BIT | ADC_AIN0,      ADC_REF_P0_7 | ADC_12_BIT | ADC_AIN5,    
  // Tin 7                                          Iin2 8 
    ADC_REF_P0_7 | ADC_12_BIT | ADC_TEMP_SENS, ADC_REF_P0_7 | ADC_12_BIT | ADC_AIN7,
  // int Gnd. 9                                     int Ref 10
    ADC_REF_P0_7 | ADC_12_BIT | ADC_GND,       ADC_REF_P0_7 | ADC_12_BIT | ADC_REF
};
#endif
*/

                                   // Vout Pin Iout           Vin Text  Iout+ Iin2  //  IntGnd  IntRef
const BYTE indices[ N_CHANNELS ] = { 0,     1,  2, 0xFF, 0xFF, 3, 0xFF, 6,    5};   //,    0xFF }; //,   0xFF   };
//const BYTE indices[ N_CHANNELS ] = { 0,     1,  2, 0xFF, 0xFF, 3, 0xFF, 0xFF,  5};   //,    0xFF }; //,   0xFF   };
INT16  lastOff = 0, lastRssi = 0; //, delta = 0;
extern INT32 delta;

const char theKey[ 2*AES_SIZE ] = { 'A', 'N', 'D', 'S',  0,  'l', 'a', 'r', 'A', 'u', 'g', 'u', 's', 't',  0,   7 ,
                                    'A', 'u', 'g', 'u', 's', 't',  0,   7,  'A', 'u', 'g', 'u', 's', 't',  0,   7 };
/******************************************************************************/
/*
void atomicChange( BYTE idx, INT16 val ){
    INT_GLOBAL_ENABLE( INT_OFF ); 
        ticks[ idx ] = val;
    INT_GLOBAL_ENABLE( INT_ON ); 
}
*/
#define atomicChange( idx, val ) ticks[idx]=val

BYTE nextKey[ AES_SIZE ];
BYTE nextIV [ AES_SIZE ];

void savePB();

// 12/27/2016 static const float DEF_CURRENT_MULTYPLIER = 3.93; //7.93; //5.536;

static float k_oc2timer = 0.0;
BYTE oc2timer(BYTE oc){ 
UINT16 val;
#ifdef MidString
  if( oc > ocShortCircuitLimit ) oc = ocShortCircuitLimit;  // Short Circuit protection.
#endif
  // 12/27/2016 if( k_oc2timer < 1.0 ) k_oc2timer = DEF_CURRENT_MULTYPLIER / (page.linearK[2][0]-4.0);
  // 04/06/2017 if( k_oc2timer < 1.0 ) k_oc2timer =  7.6 / (page.linearK[2][0]);
  if( k_oc2timer < 1.0 ) 
    k_oc2timer =  8.33 / (page.linearK[2][0]);

  //  if( test4Zero((BYTE*)&k_oc2timer, 4) ) k_oc2timer = DEF_CURRENT_MULTYPLIER / page.linearK[2][0];

  // 12/27/2016 added line
  if ( oc > 240 ) oc = 240;
  
  // 12/27/2016 val = (UINT16)( oc * k_oc2timer + 0.5 );
  val = (UINT16)( oc * k_oc2timer ); 
  if( val > 255 ) val = 255;
  return (BYTE) val; 
}
INT16 abs( INT16 a ) { return ( a > 0 ) ? a : -a; }
//void bootOtherImage()
//  {void (*f)( void ) = ( void (*)( void ) ) page.imageAddr;  /*si->interImageCommunications[ JustLoaded ] = TRUE;*/ (*f)();}

/******************************************************************************/
inline float sqr( float x ){ return x*x; }
/******************************************************************************/
//inline void setupFrequency(){
    // Configure the radio frequency to use
//    FREQ2 = 0x64;  FREQ1 = 0x6A; FREQ0 = 0xAA; // always FCC
//}
const INT32 defFreq = 0x646AAA;
const BYTE the_frequency[3] = { 0x64, 0x6A, 0xAA };
// def_freq puts the frequency values into the FrequencyControl High/Medium/Low Bytes
void def_freq(void){ mymemcpy( (BYTE *)0xDF09, (BYTE *)the_frequency, 3 ); }
/*
void switchDataRate( BOOL tenKbod ){
  if( page.is500Always )    _datarate    = DATA_RATE_1_CC2511;
  else  if( tenKbod )       _datarate    = DATA_RATE_3_CC2511;
  else                      _datarate    = DATA_RATE_4_CC2511;
}
*/
/*
void setupRadioDR(BYTE dr, BYTE channel, BOOL useFEC ){
    INT_GLOBAL_ENABLE( INT_OFF );           // Enable interrupts globally
        RFST = STROBE_IDLE; si->radioMode = RADIO_MODE_UNDEF;            
        S1CON &= ~0x03; RFIF &= ~IRQ_DONE;  RFIM = IRQ_DONE;                     // Mask IRQ_DONE flag only
        //switchDataRate( page.use12kbod );
        //if( (dr == DATA_RATE_1_CC2511) && page.use250kbod )   radioConfigure( DATA_RATE_2_CC2511 ); 
        //else
        radioConfigure( dr ); 
        CHANNR = channel;      
        if( dr == DATA_RATE_1_CC2511 ) PA_TABLE0 = page.repeaterPower;
        else                           PA_TABLE0 = page.radioPower;
        if( useFEC  ) MDMCFG1 |= 0x80; else MDMCFG1 &= 0x7F;
        ADDR = page.edAddr;
        //SYNC0 = 0xF0;    SYNC1 = 0xF0;
        SYNC0 = page.syncword & 0xFF;    SYNC1 = ( page.syncword >> 8 ) & 0xFF;
        HAL_INT_ENABLE(INUM_RF,  INT_ON);    // Enable RF general interrupt
    INT_GLOBAL_ENABLE( INT_ON );             // Enable interrupts globally
}
extern BOOL transfer;
void setupRadio( BYTE channel ){ 
    //switchDataRate( page.use12kbod ); 
    if( page.is500Always )     _datarate    = DATA_RATE_1_CC2511;
    else  if( page.use12kbod ) _datarate    = DATA_RATE_3_CC2511;
    else                       _datarate    = DATA_RATE_4_CC2511;
    setupRadioDR( _datarate, channel, page.useFEC || page.is500Always ); 
}
void setupRepeater( BOOL flag ){
  DMAARM = 0x83; // stop receive and transmit
  if( flag && (!page.is500Always) ){  
       setupRadioDR( DATA_RATE_1_CC2511, page.repeaterChannel, TRUE );  
       ADDR = page.gwAddr;  
  }else setupRadio( curChannel );                    
  // global interrups are enabled inside setup radio
}
*/
void setupRadioDR(BYTE dr, BYTE channel ){
    //INT_GLOBAL_ENABLE( INT_OFF );           // Enable interrupts globally
        while( *((BYTE *)0xF53A) == RADIO_MODE_TX) tickWait(1);            
        RFST = STROBE_IDLE; si->radioMode = RADIO_MODE_UNDEF;            
        S1CON &= ~0x03; RFIF &= ~IRQ_DONE;  RFIM = IRQ_DONE;                     // Mask IRQ_DONE flag only
        if( (dr == DATA_RATE_1_CC2511) && page.use250kbod )   radioConfigure( DATA_RATE_2_CC2511 ); 
        else                                                  radioConfigure( dr ); 
        CHANNR = channel;      
        if( dr == DATA_RATE_1_CC2511 ) PA_TABLE0 = page.repeaterPower;
        else                           PA_TABLE0 = page.radioPower;
        ADDR = page.edAddr;
        SYNC0 = page.syncword & 0xFF;    SYNC1 = ( page.syncword >> 8 ) & 0xFF;
        //RFST = STROBE_CAL; 
        HAL_INT_ENABLE(INUM_RF,  INT_ON);    // Enable RF general interrupt
    //INT_GLOBAL_ENABLE( INT_ON );             // Enable interrupts globally
}
extern BOOL transfer;
void setupRadio( BYTE channel ){ setupRadioDR( page.is500Always ? DATA_RATE_1_CC2511 : DATA_RATE_3_CC2511, channel ); }
void setupRepeater( BOOL flag ){
  DMAARM = 0x83; // stop receive and transmit
  if( flag && (!page.is500Always) ){  
       //P1_1 = 1;
       setupRadioDR( DATA_RATE_1_CC2511, page.repeaterChannel );  
       ADDR = page.gwAddr;  
  }else{
       //P1_1 = 0;
       setupRadio( curChannel );                    
  }
}
void softKick(void){
    //if( transfer ) return;
    //INT_GLOBAL_ENABLE( INT_OFF );
        //halPowerClkMgmtSetMainClkSrc(CRYSTAL);  // it is never needed to set crystall again
        DMAARM = 0x83; RFST = STROBE_IDLE; si->radioMode = RADIO_MODE_UNDEF;            
        //FREQ2 = 0x64;  FREQ1 = 0x6A; FREQ0 = 0xAA; // always FCC //setupFrequency();       //
        zerofill( (BYTE *)&delta, 4 ); //delta = 0;     
        FSCTRL0 = 0;   def_freq();
        //dmaRadioSetup();
        //MCSM0 = 0x14; // calibraton on
        loadKey( page.curKey );
        setupRadio( curChannel = ( page.is500Always ? curChannel : page.channel ) );        // global interrups are enabled inside setup radio
        zerofill( (BYTE *)&utcLast, 6 ); //utcLast = 0; msLast = 0;
        cmdCount = 0;
        //if( enable2SendData ){ computeADCs( TRUE, FALSE );  } // adjust quartz to temperature only if buffer is already full
        computeADCs( TRUE, FALSE );  // adjust quartz to temperature only if buffer is already full
        ticks[ ReceiveData ] = 1;
        ticks[ Hop ] = 0; cyclesToHop = 0;
    //INT_GLOBAL_ENABLE( INT_ON );
}
void restoreRadio(){
    //ticks[ ReceiveData ] = 1;
    //HAL_INT_ENABLE(INUM_RF,  INT_ON);    // Enable RF general interrupt
    //INT_GLOBAL_ENABLE( INT_ON );             // Enable interrupts globally
    //DMAARM = 0x83; RFST = STROBE_IDLE; 
    ticks[ ReceiveData ] = 1;
}
/******************************************************************************/
void init(void){
BYTE pin;
    P0DIR = 0;    // all port 0 pins are input
    P0INP = 0xFF; // all port 0 pins are tristate
    ADC_ENABLE_CHANNELS( 0xFF );
    for( pin = 0; pin < 8; pin++ ){ IO_ADC_PORT0_PIN( pin, IO_ADC_EN); }

    // 2 top pins are secial function - timer 3 channels
    P1SEL = 0xC0;                          
    // pins 7(oc) 6(ov) 4(on/off), 3(mpp), 1(LED), 0(mpp cycle) are for output
    P1DIR = 0xDB;   // 1101 1011

    T3CTL   = 0x50; // was 0x70, prescaler 1/4, normal operation, mode - free running
    T3CCTL0 = 0x24; // clear output on compare up, set on 0, enabled
    T3CCTL1 = 0x24; // clear output on compare up, set on 0, enabled
    PERCFG  = 0x20; // Timer3 has alternate 2 location - P1_6 & P1_7

    P2SEL   = 0x20; // Timer3 has priority over USART1
    // pull down commented out for version 8
    //P2DIR   = 0x0F; P2_1 = 0; P2_2 = 0;
    
    //P1_3 = 1; //P1 |= 8;  // MPP ON
    zerofill( (BYTE *)adcsTemp, sizeof(adcsTemp) );
    //     LED down,  MPP off, Module Off 
    //P1 &= ~ ( 2 | 8 | 0x10 ); 
    
    P0SEL = 0;
    // Choose the crystal oscillator as the system clock
 //   P1_1 = 0;
 //   halPowerClkMgmtSetMainClkSrc(CRYSTAL);
    // Select frequency and data rate from LCD menu, then configure the radio
    
    // Configure interrupt for every time a packet is sent
    
    FSCTRL0 = 0;
        
    setup_hopper( page.channel );
    dmaRadioSetup();

    //FREQ2 = 0x64;  FREQ1 = 0x6A; FREQ0 = 0xAA; // always FCC setupFrequency();
    def_freq();
    setupRadio( curChannel ); //page.is500Always? page.starting500Channel : page.channel );  // global interrupts are inabled inside the setupRadio

    //calibrateParking();

    invVref0 = 1.0 / page.vrefPolynom[0];
    //__________________________________________________________________________
   // Over Voltage     Over Current

    T3CTL |= 0x14;
    
    HAL_INT_ENABLE(INUM_T1,  INT_ON);    // enable Timer1 interrupt
    HAL_INT_ENABLE(INUM_ADC, INT_ON);    // enable ADC interrupt
    HAL_INT_ENABLE(INUM_RF,  INT_ON);    // Enable RF general interrupt
}
/******************************************************************************/
void ov_startup(void)
{
  // ensure FETs are good/Module can turn on before beginning ov_startup sequence
  // and turning module on.
  if (*ModuleOffRememberedPtr == ModuleStatusCanTurnOn)
  {
    if( page.ov_startup )
    {
      T3CC0 = (page.ov_startup < page.ov) ? page.ov_startup : page.ov; 
      atomicChange( OvStartup, TICKS_IN_SEC );
      //        #ifdef MidString                                                                            
      //        compute98();                                                                                
      //        #endif
    }
    P1_4 = 1;
  }
}
/*
void init_board(void){
    if( (!page.mpp) && page.module ){
        P1_3 = 1;  atomicChange( BypassMode, TICKS_IN_SEC ); // switching mpp on and switching it off (bypass mode) only after delay
    }else{
        if( page.mpp )    P1_3 = 1;  else P1_3 = 0;
        //if( page.module ) P1_4 = 1;  else P1_4 = 0;
    }
    if( page.module ) ov_startup();
}
*/
/******************************************************************************/
float *_coeff, invRef = 0;
float mult( float t, INT16 i ){ return (t)*(float)i; }
float lin_coeff( float f ){ return _coeff[0]*f + _coeff[1]; }
float lin_coeff_t( float f ){    return (_coeff[1] - f )*_coeff[0]; }
float lin( BYTE idx ){
float t  = ((float)adcs[ idx ]) * page.vrefPolynom[0] * invRef; // invRef = 1.0 / (float)adcs[ REF ];
    return lin_coeff_t( t ); //( coeff[1] - t ) * coeff[0];  
}
float cube_coeff( float *t ){ 
//  return ( ( ( _coeff[0]*t + _coeff[1] )*t + _coeff[2] )*t + _coeff[3] ); 
float acc = _coeff[0]; 
BYTE i;
    for( i = 1; i < 4; i++ ) acc = acc*(*t) + _coeff[i];
    return acc;
}
float checkVref;
float processCheck( INT16 val ){ return lin_coeff( ((float)val) * checkVref );  }
/******************************************************************************/
UINT16  last[ N_CHANNELS ]; //Vout, lastPin, lastIout, lastGnd, lastRef, lastVin, lastTex, lastTin;
UINT16  checkVoutOnT  = 0, checkPD = 0;
BOOL  isSteppedDown = FALSE;
void read_page(void){
    mymemcpy( (BYTE *)&page, (BYTE *)PAGEADDR, PAGESIZE ); //+4 ); 
    //if( page.barrier == 0xAAAA ){
    if( page.barrier == 0xAAAA  && page.post_barrier == 0x5555 ){
        /* page F compatibility - not needed anymore
        zerofill( (BYTE *)&page.syncword, 8 );
        page.syncword = 0xF0F0; 
        page.vin_disable_radio = 6;
        page.vin_limit         = 0; 
        page.vin_turn_on       = 120;
        page.vin_switch_off    = 100;
        */
        //BYTE *ptr = nextSlot();
        //if( ptr && ptr > (BYTE *)(PAGEADDR+PAGESIZE+4) ){ ptr -= 4;  mymemcpy( (BYTE *)&energy, ptr, 4 );  }
        //zerofill( (BYTE *)&deltaEnergy, 4 );
    }else{
        //page.barrier = 0xAAAA; page.post_barrier = 0x5555;
        mymemcpy((BYTE *)&page, (BYTE *)default_pb, PAGESIZE );
        // partial PB restoration goes here
        //void (*f)( void ) = ( void (*)( void ) )0x603F;  (*f)();  // to 8k image
        switchTo255(); ch255 = FALSE;
        page.ov         = *((BYTE *)0x3DC);
        page.oc         = *((BYTE *)0x3DD);
        //page.ov_startup = 100;//*((BYTE *)0x3DE);
        //page.myBunch = 0;   page.netId  = 0;
         /*
        page.channel = 255; page.pureOffset = -130;
        page.edAddr    = 2; page.gwAddr     = 1; 
        page.vin_limit = 0; 
        mymemcpy( (BYTE *)page.curKey, (BYTE *)theKey, 32 );
        //page.myBunch = 0;   page.netId  = 0;
        mymemcpy( (BYTE *)&page, (BYTE *)0x3D4, 12 );    // netid, bunch, mac
        //zerofill( (BYTE *)&energy, 8 );
        */
    }
    ch255 = FALSE;
}
//float polynomial[] = { 0, 0, 0, -200 }; // -0.000693, 0.0556, 0.613, -220.0};
void switchTo255(){
    curChannel = page.channel = 255;   
    page.use12kbod  = TRUE;   page.hoppingAllowed = FALSE;
    page.useFEC     = FALSE;  page.is500Always = FALSE; 
    page.isRelay    = FALSE;  page.searchCommunication = FALSE;
    page.vin_disable_radio = 0;
    //page.production = 0;
    //page.fuseComm   = 0;    page.fuseOVOC = 0;
    mymemcpy( (BYTE *)page.curKey, (BYTE *)theKey, 32 ); 
    //mymemcpy( (BYTE *)page.dFk,    (BYTE *)polynomial, 16 );
    page.syncword = 0xF0F0; page.edAddr = 2; page.gwAddr = 1;
    mymemcpy( (BYTE *)&page, (BYTE *)0x3D0, 12 );    // barrier, netid (0), bunch (0), group, mac, 
    ch255 = TRUE;   softKick();
    //page.myBunch    =  0;     page.netId = 0;  
}

static UINT16 dVOut = 0,
  vOutMin = 0,
  vOutMax = 0;

void kickOsc()
{
  P1_3 = 0;
  oscWhatToWaitFor = SetOscOVStartupDelay;
  ticks[OscEvent] = page.oscOVStartupDelay;
  T3CC0 = page.ov_startup;
}

#ifdef MidString
INT16 iOutIin1Diff; // Used in compute ADCs for FET failure
INT16 iOutIin2Diff; // Global to save time with computeADCs
#endif

void checkVin(void){
INT16  t;
UINT16 safe_vin_16; //, dp; // dp is needed for power dissipation protection
static BYTE oscCondition = 0;

#ifdef MidString
    static UINT16 currentFETTickCount = 0;

    if (checkForFETFailure)
      {
	iOutIin1Diff = (INT16)adcs16[2] - (INT16)adcs16[6];
	iOutIin2Diff = (INT16)adcs16[2] - (INT16)adcs16[5];
	if ( ((INT16)adcs16[2] > page.fetCurrentThreshold) &&
	     ( (iOutIin1Diff < page.fetDeltaCurrent) ||
	       (iOutIin2Diff < page.fetDeltaCurrent) )  )
	  {
	    currentFETTickCount += 120;
	    netValue = MainNetValueFETFailing; // indicate FET failure being observed
	    if (currentFETTickCount >= (INT16)page.fetFailureCount)
	      {
		P1_4 = 0; // turn the module off 
		fetFailure = TRUE; // set guard to keep module from being turned on
		checkForFETFailure = FALSE; // No need to check further until power cycle or Module On
		netValue = MainNetValueFETFailed; // indicate FET failure condition
		*ModuleOffRememberedPtr = ModuleStatusOffFetFailure; // Remember module is off
		currentFETTickCount = 0; // Reset to 0
	      }
	  }
	else
	  {
	    currentFETTickCount = 0; // Ensure count is back to default
	    netValue = MainNetValueStandard; // "default" netValue for main image
	  }
      }
#endif

if( ( ticks[ OvStartup ] == 0 ) &&
    ( page.oscLowCurrentLimitInmA != 0 ) &&
    ( page.oscLowCurrentLimitInmA != -1 ) &&
    ( oscWhatToWaitFor == SetOscLowCurrentOnlyDelay ) &&
    ( ((INT16)adcs16[2]) < page.oscLowCurrentLimitInmA) )
  {
    if( dVOut > page.oscDeltaV )
      oscCondition++;
    else
      oscCondition = 0;

    if( oscCondition > 1 )
      {
	oscCondition = 0;
	kickOsc();
      }
  }
 else if(oscWhatToWaitFor == SetOscLowCurrentOnlyDelay)
   {
     ticks[OscEvent] = page.oscLowCurrentOnlyDelay;
     oscCondition = 0;
   }

#ifdef MidString
UINT16 switchTo255Lim = 30000, switchFrom255Lim = 28500, shortCircuitProtectionOff = 8000, shortCircuitProtectionOn = 6000; 
    if( ((UINT16)page.linearK[0][0]) > 16 ) // MS1000
    { switchTo255Lim = 60000; switchFrom255Lim = 57000; shortCircuitProtectionOff = 16000; shortCircuitProtectionOn = 12000;  }
#endif
    checkVref = page.vrefPolynom[0] / ((float)last[4]);
    _coeff = page.linearK[3];
    //safe_vin_16 = (UINT16) processCheck( last[5] );
    safe_vin_16  = convU( lin_coeff( ((float) last[5])*checkVref ) );

    #ifndef MidString
    if( P1_3 == 0 ){
        if( P1_4  && (safe_vin_16 < (100*(UINT16)page.vin_switch_off) ) ){
            ticks[ TurnOn ] = 3 * TICKS_IN_SEC;     P1_4 = 0; 
        }else if( (P1_4 == 0) && turnOnFlag && ( safe_vin_16 > (100*(UINT16)page.vin_turn_on) ) ){
            turnOnFlag = 0; ticks[ BypassMode ] = TICKS_IN_SEC;  
            P1_3 = 1; ov_startup();
        } 
    }
    #endif
    if( (flags[ PackageReceived ]) || (flags[DataSent]) ){ flags[AltCheckVin] = TRUE; return; }
    
    txDisabled = 0;
    if( page.vin_disable_radio ) txDisabled = ( safe_vin_16 < (1000*(UINT16)page.vin_disable_radio) ); 
    
    #ifndef MidString    
    if( page.vin_limit ){
    register BOOL flag = ( safe_vin_16 < (page.vin_limit*100) );
        if( !ch255 && flag ){
          /*
            curChannel = page.channel = 255;   
            page.use12kbod  = TRUE;   page.hoppingAllowed = FALSE;
            page.useFEC     = FALSE;  page.is500Always = FALSE; 
            page.isRelay    = FALSE;  page.searchCommunication = FALSE;
            page.myBunch    =  0;   page.netId = 0;  page.vin_disable_radio = 0;
            page.production = 0;
            //page.fuseComm   = 0;    page.fuseOVOC = 0;
            mymemcpy( (BYTE *)page.curKey, (BYTE *)theKey, 32 ); 
            page.syncword = 0xF0F0; page.edAddr = 2; page.gwAddr = 1;
            ch255 = TRUE;   softKick();
          */
          switchTo255();
        }else if( ch255 && !flag ){
        BYTE curV255 = page.vin_limit;  
            read_page(); 
            page.vin_limit = curV255;
            curChannel = page.channel;
            softKick();
        }
    }else if( ch255 ){
        BYTE curV255 = page.vin_limit;  
        read_page();
        page.vin_limit = curV255;
        curChannel = page.channel;
        softKick();
    }
    #endif
    
    #ifdef MidString    
    if( !ch255 && (safe_vin_16 > switchTo255Lim ) )        switchTo255(); // was 30000 for MS600, 60000 for MS1000 
    else if( ch255 && ( safe_vin_16 < switchFrom255Lim ) ){                 // was 28500 for MS600, 57000 for MS1000
        read_page();   
        if( page.is500Always ) curChannel = page.starting500Channel; 
        softKick();  
    }
    #endif
    if( (flags[ PackageReceived ]) || (flags[DataSent]) ){ flags[AltCheckVin] = TRUE; return; }
    
    // temperature protection
    _coeff = page.linearK[4];
    t =  (INT16) lin_coeff_t( ((float)last[6]) * checkVref );
    // removed UINT16, removed whole T_Superhot
    if( page.t_mod_off && (t > ((INT16)page.t_mod_off)) ){ isSteppedDown = TRUE;  P1_4 = 0;  }   // Critical Temperature protection
    else 
    if( page.fallback_time && page.fallback_time!=0xFF ){                                   // Temperature protection
        if( checkVoutOnT )  checkVoutOnT--; 
        else{ 
            checkVoutOnT = 8*(UINT16)page.fallback_time;
            if( isSteppedDown ){
                if( t < ((INT16)page.ton_fallback) ) {
                    if( !P1_4 ) ov_startup();   // module on, starting OV seq
                    T3CC1 = oc2timer( theOC = setOC ); isSteppedDown = FALSE;
                }else if( P1_4 && /*( t > t_prev ) &&*/ theOC ) T3CC1 = oc2timer( --theOC );  
            }else  if( P1_4 && ( t > ((INT16)page.toff_fallback) ) ) {  
                if( theOC > page.oc_protection ) theOC = page.oc_protection; else theOC--;
                isSteppedDown = TRUE;   T3CC1 = oc2timer( theOC );  
            }
//            #ifdef MidString
//            compute98();
//            #endif
        }
    } 
    #ifdef MidString
    if( (flags[ PackageReceived ]) || (flags[DataSent]) ){ flags[AltCheckVin] = TRUE; return; }
    if( page.shortCircuitLevel && P1_4 ){ // module on and shortCircuitLevel is not 0
        UINT16 vout; 
        _coeff = page.linearK[0]; vout = convU( lin_coeff( ((float) last[0])*checkVref ) );
        if( ocShortCircuitLimit < 255 ){ // Short Circuit Protection is currently ON
          if( vout > shortCircuitProtectionOff ) // was 8000 - 160V on Midstring 600V, 16000 (320 V) on MS1000 turning Short Circuit Protection OFF
              { ocShortCircuitLimit = 255;  T3CC1 = oc2timer( theOC );  }  
        }else{                           // Short Circuit Protection is currently OFF
          if( vout < shortCircuitProtectionOn ) // was 6000 - 120V on Midstring 600V, was 1200 for MS1000, turning Short Circuit Protection ON
              { ocShortCircuitLimit = page.shortCircuitLevel;  T3CC1 = oc2timer( theOC );  }  
        }
    }else ocShortCircuitLimit = 255;  // if module OFF -> no Short Circuit Protection, turning it OFF
    #endif    
    
    /* temporary commented for version 7a !!!
    // noise amplifier reset - MidString only
    #ifdef MidString
    static INT8 noiseAmplCnt = 0;
    if( page.mpp && ( P1_3 || (noiseAmplCnt > -1) ) ) {
        UINT16 vout; INT16 cur; BOOL noiseAmplResetRequired;
        _coeff = page.linearK[0]; vout = convU( lin_coeff( ((float) last[0])*checkVref ) );
        _coeff = page.linearK[2]; cur  = convS( lin_coeff( ((float) last[2])*checkVref ) );
        
        noiseAmplResetRequired  = page.k_oc_cur  && (page.k_oc_cur  != 0xFF) && ( cur  > Cur98  );
        noiseAmplResetRequired |= page.k_ov_volt && (page.k_ov_volt != 0xFF) && ( vout > Vout98 );
        
        if( noiseAmplResetRequired ){
          // reset error amplifier once a second, for one mpp period
          if( noiseAmplCnt <= 0 ){ P1_3 = 0; noiseAmplCnt = 0; }
          if( noiseAmplCnt == 1 ){ P1_3 = page.mpp;  }
        }else{ P1_3 = page.mpp; } // mpp on
    }
    noiseAmplCnt = (noiseAmplCnt+1)%8;
    #endif
    */
}
//#ifdef MidString
//void compute98(){ // computed from values taken from ITF midstrings on Sep. 25
//    Cur98  = convS( (((float)page.k_oc_cur)  * ( 4670 + 38.6*((float)theOC) ))  * 0.005 );
//    Vout98 = convU( (((float)page.k_ov_volt) * ( 68.2*((float)T3CC0) + 10680 )) * 0.005 );
//}
//#endif
BOOL checkItOut( BOOL *flag_ptr ){
  if( *flag_ptr ){ *flag_ptr = FALSE; return TRUE; }
  return FALSE;
}
UINT16 *ptr2RunningTick = NULL;

UINT16 computeTI(UINT16 _pt)
//    { return ( ( _pt <= (*ptr2RunningTick) ) ? ((*ptr2RunningTick) - _pt) : ((*ptr2RunningTick) + (65535 - _pt)) ); } 
      { return ( ( (_pt&0xFF) <= *((BYTE *)0xF538) ) ? (*((BYTE *)0xF538) - (0xFF&_pt)) : (*((BYTE *)0xF538) + (255 - (0xFF&_pt))) ); } 
void tickWait(BYTE wait){ 
    ticks[ TickWait ] = ( (INT16)wait ) + 1; //(reset of watchdog should be only in one place)
    while( ticks[ TickWait ] > 0 ){ P1_2 ^= 1; };//{ WDCTL = 0xA8;  WDCTL = 0x58; *((BYTE *)0xFFFF) = 0x10; } 
    ticks[ TickWait ] = 0;
}

void checkMore(){
BYTE i; UINT16 j; BOOL go8k = FALSE;
    P1_4 = 0;   P1DIR = 0xDB;    // module off
    for( i = 0; (i<15); i++ ){ // the self check
        BYTE *ptr = (BYTE *)( 0x800 + 0x400*i );
        BOOL flag = TRUE; 
        *((BYTE *)0xFFFF) = 0x10;
        for( j = 0; flag && (j < 0x400); j++ ) flag = ( (*ptr++)==0xFF );
        go8k |= flag;
    }
    //if( go8k ) {void (*f)( void ) = ( void (*)( void ) )0x603F; (*f)();} // commented jump to 8k image, Feb 12 2014, 16:35
}

//void reset(void){ WDCTL = 0x8 | 0x3; while( TRUE ) halWait( 10 ); }
//BOOL dec( UINT16 *cntPtr ) { return ((*cntPtr) && (--(*cntPtr) == 0 ))?TRUE:FALSE; }
BOOL dec( UINT16 *cntPtr ){ --(*cntPtr); return (*((BYTE *)cntPtr)==0)?TRUE:FALSE; }
/******************************************************************************
* @fn  main
* @brief
*      Main function. Triggers setup menus and main loops for both receiver
*      and transmitter. This function supports both CC1110 and CC2510.
* Parameters:
* @param  void
* @return void
******************************************************************************/
BYTE sleepCounter = 5, saved_channel;
//void goSleep( void ){ if( sleepCounter && (--sleepCounter==0) && si->radioMode == RADIO_MODE_UNDEF ) PCON |= 1; }
//static BYTE fakeBuf[ 2 ] = { 4, 0xFE };
static BOOL   wasAHardResetOrPowerOn = TRUE;//, need_wiggle = FALSE, was_wiggling = FALSE;
//void readNextValueScheduled(void);
/*
void switch2parking( BYTE j ){
     CHANNR = parking_channels[j]; 
     //FSCAL3 = fscal_parking[j][0];         FSCAL2 = fscal_parking[j][1];          FSCAL1 = fscal_parking[j][2];
     mymemcpy( FSCAL3_ADDR, fscal_parking[j], 3 );
     tickWait( 0 );
}
*/
UINT16 milliSeconds = 0;
BYTE   seconds = 0, minutes = 0;
BYTE was_repeater = 0;
void main_loop(void){
BYTE idx, cwCnt = 1; 
static BYTE xored = 1;
    //while( cwCnt -- ){
        //*((BYTE *)0xFFFF) = 0x20;
        for( idx = 0; idx < N_Of_Flags; idx ++ ){
            if( flags[ idx ] ){
                flags[ idx ] = FALSE;
                switch( idx ){
                    case DataSent:
                        rc();
                        //P1_1 = 0;
                        //P1_1 ^= 1;
                        //while( real_cw_mode ) tickWait( 1 ); 
                        //if( checkItOut(&cleanTxBuffer ) ) radioPktBufferTx[0] = 0; //cleanTxBuffer = FALSE; 
                        //if( was_wiggling ) was_wiggling = FALSE; CHANNR = saved_channel;  MCSM0 = 0x14; }
                        /*
                        if( page.wiggle_dchan && need_wiggle ){
                           setupTxBuffer( fakeBuf );
                           INT_GLOBAL_ENABLE( INT_OFF ); 
                              was_wiggling = TRUE; 
                              saved_channel = CHANNR; MCSM0 = 0x04; // no calibration
                              CHANNR ^= page.wiggle_dchan;
                              si->radioMode = RADIO_MODE_TX;        
                              atomicChange( CheckFlag, TICKS_IN_MS );
                              DMAARM = DMAARM_CHANNEL1;        // Arm DMA channel 1
                              //RFST   = STROBE_TX;            // Switch radio to TX
                              need_wiggle = FALSE; 
                           INT_GLOBAL_ENABLE( INT_ON ); 
                        }else{
                          // switching transmission off
                          if( was_wiggling ) { CHANNR = saved_channel;   MCSM0 = 0x14; }
                          if(      page.isRelay && (!page.is500Always) ) setupRepeater( TRUE ); 
                          else if( page.stay_in_rx )                     receive();
                        }
                        */
                        //if(  page.isRelay && (!page.is500Always) ) setupRepeater( got_gw ); 
                        //else                                       setupRepeater( FALSE ); 
                        ////was_repeater = 0;
                        //receive();
                        //P1_4 = 1;  // debug line !!!
                        if( was_listening )                           { setupRepeater( FALSE );   receive(); }
                        else if( page.isRelay && (!page.is500Always) ){ setupRepeater( got_gw );  receive(); }
                    break;
                    
                    case PrepareJoin:       prepareJoinRequest( );                  break;
                    /*
                    case PrepareData:       prepareData();     // here most of the time is spent
                                            processingTime = computeTI( pt ); //( pt <= si->runningTick ) ? (si->runningTick - pt) : (si->runningTick + (65535-pt));
                    break;
                    */
                    case PackageReceived:   /*P1_4 = 1;*/ parseGWPackage(); /*P1_4 = 0;*/  break; // P1_4  is a debug added on Feb 22 2014, should be removed
                    case AltCheckVin:     // was 8
                      //if( ( ticks[SendData] > 14*TICKS_IN_MS ) || ( ticks[SendData] == 0 ) ) computeADCs( FALSE, TRUE ); //checkVin();
                      if( ( PKTSTATUS & 0x4 ) && ( ( MARCSTATE > 0xC ) && ( MARCSTATE < 0x10 ) ) ) flags[AltCheckVin] = TRUE; // receving a packet
                      else if( flags[PackageReceived] || flags[DataSent] )                         flags[AltCheckVin] = TRUE; // receving a packet
                      else if( ( ( ticks[SendData]      > 14*TICKS_IN_MS ) || ( ticks[SendData] == 0 ) ) &&
                          ( ( ticks[DelayedPrepareData] > 14*TICKS_IN_MS ) || ( ticks[DelayedPrepareData] == 0 ) ) ) 
                            computeADCs( FALSE, TRUE ); //checkVin();
                      else                                                       flags[AltCheckVin] = TRUE;
                      if( (((INT16)adcs16[ 2 ]) < page.cur_noise ) ){ T3CC1 = theOC = 0; atomicChange( OcRamp, TICKS_IN_SEC ); }
                    break;
                    case Cycle:
                            //if( bunchCount ) bunchCount--;
                            /*
                            if( enable2SendData && ( curBunch != 255 ) ){ 
                                if( (curBunch == maxBunch) && zeroPackageWasNotReceived ){ 
                                    BYTE k;
                                    //computeADCs( page.is500Always, TRUE ); 
                                    for( k = 0; k <= maxBunch; k++ ){ ms += lastCycle; if( ms > 1000 ){ utc++; ms-= 1000; } }
                                    lastRssi = 0; 
                                    prepareData();
                                    curBunch = 0; 
                                } else curBunch++; 
                                zeroPackageWasNotReceived = TRUE;
                            }
                            */
                            if( dec( &cycles2SoftKick )  ){ softKick();  cycles2SoftKick  = page.defCycles2SoftKick;  }//cyclesDefaults[4]; } // page.defCycles2SoftKick;  }
                            if( cycles2HardKick < 10 ){ switchTo255(); ch255 = FALSE; }
                            //if( dec( &cycles2HardKick ) /* && page.production */){
                            //    reset();  // a reset
                            //}
                            if( ++cyclesFromStart > 50 ) wasAHardResetOrPowerOn = FALSE;
                            /**/
                            if( (!wasAHardResetOrPowerOn) && page.searchCommunication ){
                                if( page.is500Always ){
                                    if( dec( &cyclesTo10 ) ) { 
                                        cyclesTo500 = page.defCyclesTo500;  cycles2Step10Channel = page.defCyclesToStep10Channel;
                                        //cyclesTo500 = cyclesDefaults[0];  cycles2Step10Channel = cyclesDefaults[3];
                                        page.is500Always = FALSE; //cycles2Step10Channel = page.defCyclesToStep10Channel;
                                        setupRadio( curChannel = page.channel );
                                    }else if( dec( &cyclesToStep500Channel ) ) { 
                                        cyclesToStep500Channel = page.defCyclesToStep500Channel; //cyclesDefaults[2]; //page.defCyclesToStep500Channel; 
                                        page.starting500Channel = curChannel = (curChannel + page.repStep) % 250;
                                        setupRadio( curChannel );
                                    }
                                }else{
                                    if( dec( &cyclesTo500 ) ) { 
                                        cyclesTo10 = page.defCyclesTo10;    cycles2Step10Channel = 0;
                                        //cyclesTo10 = cyclesDefaults[1];    cycles2Step10Channel = 0;
                                        page.is500Always = TRUE; cyclesToStep500Channel = page.defCyclesToStep500Channel; //cyclesDefaults[2]; //page.defCyclesToStep500Channel; 
                                        if( page.starting500Channel == 0xFF ) page.starting500Channel = page.channel;
                                        curChannel = page.starting500Channel;
                                        ticks[ Hop ] = 0;
                                        setupRadio( curChannel );
                                    }else if( dec( &cycles2Step10Channel ) ){
                                        //hop(); 
                                        CHANNR = curChannel; hop(); curChannel = CHANNR; 
                                        cycles2Step10Channel = page.defCyclesToStep10Channel;// cyclesDefaults[3];//page.defCyclesToStep10Channel;
                                    }
                                }
                            }
                            /**/
                    break;
                } // switch
                if( flags[ PackageReceived ] || flags[DataSent] ) break;
            } // if( flags )
        }// after the whole for cycle
    //} // while
  //  *((BYTE *)0xFFFF) = 0x20;
    for( idx = 0; idx < N_Of_Ticks; idx++ ){
        //INT_GLOBAL_ENABLE( INT_OFF ); 
        if( ticks[ idx ] < 0 ){
            ticks[ idx ] = 0;
            //INT_GLOBAL_ENABLE( INT_ON ); 
            switch( idx ){
                //case Reset: reset();
                case FeedTheDog:    
                    WDCTL = 0xA8;  WDCTL = 0x58;  ticks[ FeedTheDog ] = _dogFeeding;          
                break;
                case SendData:
                    if( !bunchCount ) break;
                    if( bunchCount ) bunchCount--;
                    *((BYTE *)0xF538) = 0;
                    was_repeater = ( ADDR == page.gwAddr );
                    if( packagePrepared ){
                        if( !was_repeater )  was_listening = (si->radioMode == RADIO_MODE_RX);
                        //if( reportScaling ){         reportScaling--; if( !reportScaling ) base_ptr = NULL; }
                        if( isCalibrationPackage && reportScaling ){ isCalibrationPackage--; reportScaling--; if( !reportScaling ) base_ptr = NULL; }
                        else if( reportFlashCheck )  reportFlashCheck--;
                        //P1_4 = 0;
                        //P1_1 = 0;
                        sendData( radioPktBufferTx, FALSE );  //cleanTxBuffer = TRUE; 
                        //P1_1 = 1;
                        if( page.gbunch && ( ( page.gbunch <= 7 ) || ( page.rbunch <= 7 ) ) ){
                            BYTE b; 
                            if( page.rbunch && ( page.rbunch < page.gbunch ) ) b = page.rbunch; else b = page.gbunch;
                            if( page.use12kbod ){ if( b <= 18 ) atomicChange( SendData, lastCycle*b - 1 -*((BYTE *)0xF538)-xored); } // was 3
                            else{                 if( b == 1 )  atomicChange( SendData, lastCycle   - 1 -*((BYTE *)0xF538)-xored); } // was 3
                            xored ^= 1;
                        }
                    }//else flags[ DataSent ] = 1;
                break;
                case BypassMode:        P1_3 = 0;                                                   break;
                case TurnOn:            
#ifdef MIDSTRING 
		  if (fetFailure == FALSE)
		    {
		      turnOnFlag = TRUE;
		    }
#else
                    if( page.ov == T3CC0 ) P1_4 = 1; else turnOnFlag = TRUE;                                          
#endif
                break;                
                case Init:              //init_board();                                               
                    if( (!page.mpp) && page.module ){
                        P1_3 = 1;  /*P1 |= 0x18;*/ atomicChange( BypassMode, TICKS_IN_SEC ); // switching mpp on and switching it off (bypass mode) only after delay
                    }else P1_3 = page.mpp;
                    if( page.module ) ov_startup();
                break;
                /*
                case CheckFlag:         if( si->radioMode == RADIO_MODE_TX ){
                    INT_GLOBAL_ENABLE( INT_OFF ); 
                        si->radioMode = RADIO_MODE_UNDEF; 
                        RFIF &= ~IRQ_DONE; S1CON &= ~0x03;  RFIM = IRQ_DONE; 
                        RFST = STROBE_IDLE;
                        DMAARM = 0x80 | DMAARM_CHANNEL1;
                        flags[ DataSent ] = TRUE;
                    INT_GLOBAL_ENABLE( INT_ON ); 
                } break;
                */
	    case OscEvent:
	      if( ( ticks[ OvStartup ] == 0 )
		  && ( page.oscLowCurrentLimitInmA != 0 )
		  && ( page.oscLowCurrentLimitInmA != 0xFFFF ) )
		switch( oscWhatToWaitFor )
		  {
		  case SetOscLowCurrentOnlyDelay:
		    kickOsc();
		    break;
		  case SetOscAfterStartupDelay:
		    oscWhatToWaitFor = SetOscLowCurrentOnlyDelay;
		    ticks[OscEvent] = page.oscLowCurrentOnlyDelay;
		    P1_3 = page.mpp;
		    break;
		  case SetOscOVStartupDelay:
		    if( T3CC0 < page.ov )
		      {
			T3CC0 = T3CC0+1;
			ticks[OscEvent] = page.oscOVStartupDelay;
		      }
		    else
		      {
			oscWhatToWaitFor = SetOscAfterStartupDelay;
			ticks[OscEvent]= page.oscAfterStartupDelay;
		      }
		    break;
		  default:
		    oscWhatToWaitFor = SetOscLowCurrentOnlyDelay;
		    ticks[OscEvent] = page.oscLowCurrentOnlyDelay;
		    break;
		  }
	      else
		{
		  oscWhatToWaitFor = SetOscLowCurrentOnlyDelay;
		  ticks[OscEvent] = page.oscLowCurrentOnlyDelay;
		}
	      break;

                case Hop: if( page.hoppingAllowed && !page.is500Always ){
                    if( si->radioMode == RADIO_MODE_TX  ) { atomicChange( Hop, 1 ); break; }
                    if( cyclesToHop-- ){ 
                        CHANNR = curChannel; hop(); curChannel = CHANNR; 
                        atomicChange( Hop, lastCycle /*- 4*TICKS_IN_MS */); //- _slot );  
                        //setupRepeater( FALSE );     receive();
                    } else   { curChannel = page.channel; CHANNR = curChannel; }
                    ticks[ ReceiveData ] = 0;
                } // go through
                case ReceiveData:  setupRepeater( FALSE );  was_listening=TRUE;   receive();   break;
//                case Ms:   atomicChange( Ms, TICKS_IN_MS );  
//                   milliSeconds++; 
//                   if( milliSeconds >= 1000 ){
                case Ms:   atomicChange( Ms, TICKS_IN_SEC );  
                        cycles2HardKick--;
//                        milliSeconds = 0; seconds++;
//                        if( seconds >= 60 ){
//                            seconds = 0;  minutes++;
//                            if( ( minutes > 0 ) && ( 0 == (minutes % 15 ) ) ) alignMPP = TRUE;
//                            if( minutes >= 60 ) minutes = 0;
//                        }
//                   }
                break;
                case Cycle: // kicking and search logic
                    atomicChange( Cycle, lastCycle );
                    atomicChange( ReceiveData, lastCycle - _slot - 5*TICKS_IN_MS );    // was 25
                    flags[ Cycle ] = TRUE;
                    if(got_gw) got_gw--;
                    if( isOnTestStand ) isOnTestStand--;
                break;
                case OvStartup:
                      if( T3CC0 < page.ov ){ 
                          BYTE cnt = page.ov_step, a = T3CC0;
                          if( cnt == 0 ) cnt = 1;
                          while( cnt-- && (a < page.ov) ) a++;
                          T3CC0 = a;
                          atomicChange( OvStartup, TICKS_IN_SEC ); 
                      } 
//                      #ifdef MidString
//                      compute98();
//                      #endif
                break;
                case OcRamp:
                      #ifdef MidString
                      if( (((INT16)adcs16[ 2 ]) > page.cur_noise ) ){
                            if( theOC < setOC ){ 
                                BYTE cnt = page.oc_step, a = theOC;
                                if( cnt == 0 ) cnt = 10;
                                while( cnt-- && ( a < setOC ) ) a++;  
                                T3CC1 = oc2timer( theOC = a );
                                atomicChange( OcRamp, TICKS_IN_SEC ); 
                            }
                      }else{ T3CC1 = theOC = 0; atomicChange( OcRamp, TICKS_IN_SEC ); }
//                      compute98();
                      #endif
                break;
                case DelayedPrepareData: { // here most of the time is spent
                    *((BYTE *)0xF538) = 0; 
                    //ticks2Prepare = (*ptr2RunningTick)&0xFF; 
                    prepareData(); 
                    //ticks2Prepare = computeTI(ticks2Prepare); 
                    if( page.gbunch ){
                      if(page.use12kbod && ( page.gbunch <= 18 ) )
                      //  { atomicChange( DelayedPrepareData, page.gbunch * lastCycle - ticks2Prepare - 1 ); } // was 3
                          { atomicChange( DelayedPrepareData, page.gbunch * lastCycle - *((BYTE *)0xF538) - 1 -xored); } // was 3
                      else if( !page.use12kbod && ( page.gbunch == 1 ) )
                      //  { atomicChange( DelayedPrepareData, lastCycle - ticks2Prepare - 1 ); } // was 3
                          { atomicChange( DelayedPrepareData, lastCycle - *((BYTE *)0xF538) - 1 -xored); } // was 3                        
                    }
                }break;  
                //case StartOtherImage:   bootOtherImage();
            } // switch
        } //else { INT_GLOBAL_ENABLE( INT_ON ); }
    }// for idx
}
#define BARRIER  0xAA
void quartz();
void longTickWait( BYTE q ){ while( q-- ) tickWait( 50*TICKS_IN_MS ); }
void main(void){
UINT16 i, j;
    *((BYTE *)0xF53B) = 0xAA;
    *((BYTE *)0xFFFF) = 0x10; // was 0x80 on Dec 19th
    if( PARTNUM != 0x81 ) reset(); //{ WDCTL = 0x8 | 0x3; while( TRUE ) halWait( 10 ); } //a reset  
    if( *((BYTE *)0xFFFE) == 0x5A ) goto MainLoopLabel;
    *((BYTE *)0xFFFE) = 0x5A; 
//    WDCTL = 0xA8; WDCTL = 0x58;
/*    T1CTL &= 0xFC;
    SET_WORD(T1CNTH, T1CNTL, 0);
    SET_WORD(T1CC0H, T1CC0L, 600 - 1);
    T1CTL |= 0x04  | 0x02;
    T1CCTL0 = 0x44;
*/    
    HAL_INT_ENABLE(INUM_T1,  INT_ON);    // enable Timer1 interrupt

    //            15
    for( i = 0; i<15; i++ ){ // was 15 the self check !!!! check line 663 if loop limit changed !!!
        BYTE *ptr = (BYTE *)( 0x800 + 0x400*i );
        BOOL flag = TRUE; 
        *((BYTE *)0xFFFF) = 0x10;
        for( j = 0; flag && (j < 0x400); j++ )
           flag = ( (*ptr++)==0xFF );
        //if( flag ){void (*f)( void ) = ( void (*)( void ) )0x603F; (*f)();}
        if( flag ) checkMore();
    }
    
    si->armAdcs       = &armAdcs;
    si->readNextValue = &readNextValue;
    si->flashCallback = &flashCallback;
    //si->rfCallback    = &rfCallback;
    si->adcEnabled    = TRUE;
    // added in order to setup correct MPP frequecy
    si->mppCycle      = _mppCycle;
    si->firstAdcReading = _firstAdcMeasurement;
    flags = si->flags;
    ticks = si->ticks;
    ptr2RunningTick = & (si->runningTick);
    
    //unnecessary if da_boot is right
    //SET_WORD(T1CC0H, T1CC0L, 600 - 1); // 300 - 1    
    
    oscWhatToWaitFor = SetOscLowCurrentOnlyDelay;
    ticks[OscEvent] = page.oscLowCurrentOnlyDelay;

    resetReason = (SLEEP & 0x18)>>3;
    zerofill( (BYTE *)&page, 240 );
    read_page(); 

    if (page.fetFailureCount != 0)
      checkForFETFailure = TRUE;
    else
      checkForFETFailure = FALSE;

    mymemcpy( (BYTE *)&cycles2SoftKick, (BYTE *)&page.defCycles2SoftKick, 4 ); //(BYTE *)&cyclesDefaults[4], 4 );//(BYTE *)&page.defCycles2SoftKick, 4 );
    curChannel = page.is500Always ? page.starting500Channel : page.channel; 
    search_stat = page.searchCommunication; 
    //init();

    switch( resetReason ){
        case 2:  // Watchdog Reset  
          #ifdef MidString
            /*
            P1_3 = 0; P1_4 = 0; 
            longTickWait( 20 );
            P1_3 = 1; 
            atomicChange( Init, ((INT16) TICKS_IN_SEC) * 3 );
            */
	  if (*ModuleOffRememberedPtr == ModuleStatusOffFetFailure)
	    {
	      fetFailure = TRUE;
	      netValue = MainNetValueFETFailed;
	      checkForFETFailure = FALSE;
	      P1_4 = 0;
	    }
	  else if (*ModuleOffRememberedPtr == ModuleStatusOff)
	    {
	      P1_4 = 0;
	    }
	  else
	    {
	      *ModuleOffRememberedPtr = ModuleStatusCanTurnOn;
	    }
	  if (*ModuleOffRememberedPtr == ModuleStatusCanTurnOn)
	    {
	      if(page.module)
		P1_4 = 1;
	      else
		P1_4 = 0;
	    }
	  if( page.mpp )    P1_3 = 1; else P1_3 = 0;
          #else
            // SPT: MPP On, Module On
            P1_3 = 1; P1_4 = 1; 
          #endif
          T3CC0 = page.ov; T3CC1 = oc2timer( setOC = theOC = page.oc ); 
          //wasAHardResetOrPowerOn = FALSE; 
        break; 
        case 1: case 3: 
        case 0:             // Power On Reset      
            //P1 &= ~ ( 8 | 0x10 );  // Mpp Off, Module Off
	  *ModuleOffRememberedPtr = ModuleStatusCanTurnOn;

            if( page.ov_startup ) T3CC0 = page.ov_startup; else T3CC0 = page.ov;    
            P1_3 = 0; P1_4 = 0; 
            //{ BYTE i = 8;  while( i-- ){ WDCTL = 0xA8; WDCTL = 0x58; *((BYTE *)0xFFFF) = 0x10; halWait(125); } }
            //longTickWait( 5 ); // to fill ADC buffer with data should be done later
            #ifdef MidString
                P1_3 = page.mpp; 
                T3CC1 = theOC = 0; setOC = page.oc;  
            #else    
                P1_3 = 1; 
                T3CC1 = oc2timer( setOC = theOC = page.oc );    
            #endif
            atomicChange( Init, ((INT16) TICKS_IN_SEC) * 3 );
            atomicChange( OcRamp, TICKS_IN_SEC );
        break; 
    }
    zerofill( (BYTE *)adcBuf, 2*N_CHANNELS*BUFFER_SIZE );
    init();             // turn the crystall on, exit from third state
    //longTickWait( 12 ); // to fill ADC buffer with data should be done later
    //ticks[ MppCycle ]    = _mppCycle;   // 60 ms
    //ticks[ Ms       ]    = TICKS_IN_MS;
    ticks[ Ms       ]    = TICKS_IN_SEC;     
    ticks[ ReceiveData ] = TICKS_IN_MS;    
    ticks[ FeedTheDog ]  = _dogFeeding; 
    ticks[ Cycle ]       = lastCycle;

    // set of the timer registers
    //SET_WORD(T1CC0H, T1CC0L, 600 - 1);
    //        prescaler = 8      modulo mode            ie
    //T1CTL   = 0x04  | 0x02;    T1CCTL0 = 0x44;
    // check of the timer registers
    //if( ((T1CTL&0xF)!=(0x04|0x02)) || (T1CCTL0!=0x44) || (T1CC0H!=0x2) || (T1CC0L!=0x57) ) reset();
       
    //ticks[ OvStartup ]   = TICKS_IN_SEC;
    
    loadKey( page.curKey ); 
    RNDL = page.myMac[4]; RNDL = page.myMac[5]; //ADCCON1 |= 1; 
 
//    #ifdef MidString
//    compute98();
//    #endif
//    { BYTE counter = 127;
//      SLEEP &= ~OSC_PD_BIT;     // powering down all oscillators
//      while(!XOSC_STABLE && counter-- ) halWait(2);      // waiting until the oscillator is stable
//      asm("NOP");
//      //CLKCON &= ~MAIN_OSC_BITS; // starting the Crystal Oscillator
//      CLKCON = 0x89;  
//      SLEEP |= OSC_PD_BIT;      // powering down the unused oscillator
//    }
    //P1_1 = 0; // to fill ADC buffer with data
    T1CTL &= 0xFC;
    SET_WORD(T1CNTH, T1CNTL, 0);
    SET_WORD(T1CC0H, T1CC0L, 750 - 1);
    T1CTL |= 0x04  | 0x02;
    T1CCTL0 = 0x44;
    longTickWait( 10 ); //20 ); was 20
    if( mymemcmp( (BYTE *)&cycles2SoftKick, (BYTE *)&page.defCycles2SoftKick, 4 ) ) reset(); // (BYTE *)&cyclesDefaults[4]
//    search_stat = page.searchCommunication = 0;
//    softKick();
    P1_1 = 0; // was 1
MainLoopLabel:
  /* We are not turning the quartz on *
    { BYTE counter = 127;
      SLEEP &= ~OSC_PD_BIT;     // powering down all oscillators
      while(!XOSC_STABLE && counter-- ) halWait(2);      // waiting until the oscillator is stable
      asm("NOP");
      //CLKCON &= ~MAIN_OSC_BITS; // starting the Crystal Oscillator
      CLKCON = 0x89;  
      SLEEP |= OSC_PD_BIT;      // powering down the unused oscillator
    }
  **/
    quartz();
    softKick();
    for (;;){ 
        if( *((BYTE *)0xF53B) != 0xAA ) reset(); 
        if( PARTNUM != 0x81 ) reset();
        WDCTL = 8;
        main_loop(); 
        if( flags[ PackageReceived ] || flags[DataSent] ) continue;
        if( lastCycle > (840*TICKS_IN_MS) ) lastCycle = 840*TICKS_IN_MS;
        if( lastCycle == 0 )                lastCycle = 840*TICKS_IN_MS;
        if( ticks[ Cycle ] > lastCycle )    ticks[ Cycle ] = lastCycle;
        if( ticks[ Cycle ] == 0        )    ticks[ Cycle ] = lastCycle;
        if( cycles2SoftKick > page.defCycles2SoftKick /*8*/ )           cycles2SoftKick = page.defCycles2SoftKick; //8;
        if( cycles2SoftKick == 0 )          cycles2SoftKick = page.defCycles2SoftKick; //8;
        if( (cycles2HardKick&0xFF) == 0 )   reset();  // a reset        

//        if( ticks[ MppCycle ] > _mppCycle )    ticks[ MppCycle ] = _mppCycle;
//        if( ticks[ MppCycle ] == 0        )    ticks[ MppCycle ] = _mppCycle;
        //if( ticks[ Ms       ] > TICKS_IN_MS )  ticks[ Ms ]       = TICKS_IN_MS;
        //if( ticks[ Ms ]       == 0          )  ticks[ Ms ]       = TICKS_IN_MS;
        if( ticks[ Ms       ] > TICKS_IN_SEC )  ticks[ Ms ]       = TICKS_IN_SEC;
        if( ticks[ Ms ]       == 0           )  ticks[ Ms ]       = TICKS_IN_SEC;
        // goSleep(); 
        //P1_1 ^= 1;
    }
}
/*==== PRIVATE FUNCTIONS =====================================================*/
/******************************************************************************
* Work with watchdogs and LED's
******************************************************************************/
//void feedTheDog(void){ WDCTL = 0xA8;  WDCTL = 0x58; }
/******************************************************************************
* initiate reading of adc channels
******************************************************************************/
//#pragma optimize=s 9
void armAdcs(void){  
    adcCount = 0;
    si->inCurCycle++; 
    if( si->inCurCycle < 4 ) ticks[ AdcMeasurement ] = _adcMeasurement;  // 5ms
    ADC_SINGLE_CONVERSION( adcChannels[ adcCount ] ); 
}
/******************************************************************************
* Read ADC's once into buffer
******************************************************************************/

//void readNextValue(void){ flags[ IncreaseSleepCnt ] = TRUE;  flags[ ReadNextValue ] = TRUE; } 
#pragma optimize=s 9
void readNextValue(void){ //Scheduled(void){  // 
INT16 value;
static int theShift = 50;
static UINT16 rawVout;

    ADC_GET_VALUE( value ); value >>= 2; value += theShift;  // 200 >> 2 = 50
    
    if (adcCount == 0)
      rawVout = (UINT16)value;
    
    //while( value < 0 ){ value++; theShift++; } // adaptive
    adcsTemp[ adcCount++ ] += (UINT16)value; 

    if( adcCount < N_CHANNELS ){
        ADC_SINGLE_CONVERSION( adcChannels[ adcCount ] ); 
    }else{
      if( tempAdcsCount == 0 )
	vOutMin = vOutMax = rawVout;
      else
	{
	  if( rawVout > vOutMax )
	    vOutMax = rawVout;
	  if( rawVout < vOutMin )
	    vOutMin = rawVout;
	}

        if( ++tempAdcsCount >= 8 ){
        BYTE idx; INT16 grnd = adcsTemp[ 3 ];
            //mymemcpy( (BYTE *)ptr, (BYTE *)adcsTemp, 8*sizeof( INT16 ) );
            for( idx = 0; idx < N_CHANNELS; idx++ ){
                adcBuf[ idx ][ bufferCount ] = adcsTemp[ idx ];
                //last[idx]                    = adcsTemp[ idx ] - grnd;// + ( page.thirty_three_mv ? 649 : 0 );
                if( adcsTemp[ idx ] < grnd ) last[idx] = 0; else last[idx] = adcsTemp[ idx ] - grnd;
                //if( ( idx < 5  ) && ( idx > 0  ) ){ counts1[ idx ] += adcsTemp[ idx ]; /* counts2[ idx ] += adcsTemp[ idx ]; */ }
            }
            //lastRef        = adcsTemp[ 4 ] - adcsTemp[ 3 ];
            /*
            lastVout       = adcsTemp[ 0 ] - lastGnd;
            lastIout       = adcsTemp[ 2 ] - lastGnd;
            lastRef        = adcsTemp[ 4 ] - lastGnd;
            lastTex        = adcsTemp[ 6 ] - lastGnd;
            lastVin        = adcsTemp[ 5 ] - lastGnd;
            */
            flags[ AltCheckVin ] = TRUE;
	    dVOut = vOutMax - vOutMin;
            zerofill( (BYTE *)adcsTemp, sizeof(adcsTemp) );    tempAdcsCount = 0;
            if( ++bufferCount == BUFFER_SIZE ){ enable2SendData = TRUE; bufferCount = 0; }
        }
        adcCount = 0;
    }
}
//________________________________________________________________________________
void prepareTxBuffer(){
     loadIV( page.curIV );  
     encode( 32, (char *)radioPktBuffer,      (char *)(radioPktBufferTx+2) );
     //loadIV( page.curIV );  
     //encode( 16, (char *)radioPktBuffer,      (char *)(radioPktBufferTx+2) );
     //loadIV( page.curIV );  
     //encode( 16, (char *)(radioPktBuffer+16), (char *)(radioPktBufferTx+18) );

     // for SDAG debug 
     //mymemcpy( (char *)(radioPktBufferTx+2), (char *)radioPktBuffer, PACKET_LENGTH_ED-2 );
     
     radioPktBufferTx[0] = PACKET_LENGTH_ED;               // Length byte
     radioPktBufferTx[1] = page.gwAddr;                    // GW address
     packagePrepared = TRUE; 
}
void copyMac( BYTE *ptr) { mymemcpy( ptr, page.myMac, 6); }
//#define copyMac( ptr ) mymemcpy( (BYTE *)ptr, page.myMac, 6); 
BYTE ndev = 0;
INT16 prevRssi = 0;
BYTE coefficients_buffer[ 24 ];
void prepareCoeff(void);
/******************************************************************************
* Prepare data - average the round robin buffer into 10 values
******************************************************************************/
void prepareData(void){  
  UINT16 reportStatus = netValue;

  isCalibrationPackage = FALSE;
  //if(      reportScaling    ) prepareScaling();
  //if(      reportScaling    ){ prepareScaling();   isCalibrationPackage = TRUE;  }
  if(      reportScaling    ){ if( base_ptr==coefficients_buffer ) prepareCoeff(); else prepareScaling();   isCalibrationPackage = TRUE;  }
  //else if( reportFlashCheck ) prepareFlashCheck();
  else if( enable2SendData  ){
      BYTE *ptr  = radioPktBuffer;
      //float e = energy + deltaEnergy;
      // UINT16 value = 50; 
      computeADCs( FALSE /*page.is500Always*/, FALSE ); 
      // Insert the 6 byte timestamp into a static packet buffer
      // if( page.reportUTC ) mymemcpy( ptr, (BYTE *)&utc, 4 ); 
      //else                 mymemcpy( ptr, (BYTE *)&e,   4 );     // 
      mymemcpy( ptr, (BYTE *)&utc, 4 ); 
      ptr +=4;
      *ptr++ = (BYTE)curChannel;  *ptr++ = ( ( page.netId&0xF | ( ( P1 & 0x18 ) << 1 ) ) << 2 );    
      //*ptr++ = (BYTE) adcs16[10];  *ptr++ = (BYTE)(0x3) | ( ( page.netId&0xF | ( ( P1 & 0x18 ) << 1 ) ) << 2 );    
      copyMac( ptr );  
      ptr += 6;                  // mac   
      // 12 bytes gone
      mymemcpy( ptr, (BYTE *)adcs16, 16 ); 
      if( page.is500Always )             reportStatus += 2 + ((UINT16)CHANNR)*100;
      else if( page.isRelay  )           reportStatus += 1 + page.repeaterChannel*100;
      if( page.searchCommunication )     reportStatus += 4;
      if( page.use250kbod )              reportStatus += 8;
      if( isOnTestStand )                reportStatus += 16;
      *((INT16 *)(ptr+14)) = reportStatus;
      /*
      if(      mdm_rate1[0] == 0x0E )   *((INT16 *)(ptr+2)) = 3000;
      else if( mdm_rate1[0] == 0x1E )   *((INT16 *)(ptr+2)) = 1000;
      else                              *((INT16 *)(ptr+2)) = 2000;
      */
      //*((INT16 *)(ptr+12)) = cmdCount;//_slot;
      //*((INT16 *)(ptr+12)) = decodeDelay; // temperatureProtectionDelay;
      ptr += 16;
      //*ptr++ = (BYTE) theRealOffset;    //   lastOff;       // 28
      *ptr++ = lastRssi ? ((BYTE) theRealOffset) : 0x3F; //0x7F;
      if( !lastRssi ) lastRssi = prevRssi; else prevRssi = lastRssi;
      *ptr++ = (BYTE) ( lastRssi );    // 29
      //*ptr++ = T3CC1;  // recalculated OC value for the test of the OC controlling algorithm
      if( P1_4 ){
          *ptr++ = (BYTE)   T3CC0;          
          *ptr   = (BYTE)   theOC; // timer2oc( T3CC1 ); //T3CC1;         // 30 & 31
          #ifdef MidString
            if( *ptr > ocShortCircuitLimit ) *ptr = ocShortCircuitLimit;
          #endif
      }else{
          *ptr++ = page.ov;
          *ptr   = (BYTE)   setOC;   
      }
      lastRssi = 0;
  }else return;
  prepareTxBuffer();        
}
/******************************************************************************
* Prepare scaling coeff to send
******************************************************************************/
/*
void prepareSpecial( BYTE tag, BYTE *p ){
BYTE *ptr = radioPktBuffer;
    //tag |= ( ((UINT16)page.netId) << 10 );
    // *((UINT16 *)(ptr+4)) = tag;
    *(ptr+5) = 3 | (page.netId << 2);
    *(ptr+4) = tag;
    //mymemcpy( ptr+4, (BYTE *)&tag, 2 );
    copyMac( ptr+6 ); 
    mymemcpy( ptr,    p,   4 ); 
    mymemcpy( ptr+12, p+4, 20 );
}
*/
void prepareSpecial( BYTE tag, BYTE *p ){
BYTE *ptr = radioPktBuffer;
    mymemcpy( ptr,    p,   4 ); ptr+=4;
    *ptr++ = tag;
    *ptr++ = 3 | (page.netId << 2);
    copyMac( ptr ); 
    mymemcpy( ptr+6, p+4, 20 );
}
void prepareCoeff(void){
    BYTE idx;
    UINT16 *ptrU16 = (UINT16 *)coefficients_buffer;
    INT16  *ptrI16 = (INT16 *)&(coefficients_buffer[12]);
    for( idx = 0;  idx < 7;  idx++) if( idx != 4 ) *ptrU16 ++ = convU( 1E3 * page.linearK[ idx ][0] );  
    *ptrI16++ = convS( page.linearK[0][1] );
    *ptrI16++ = convS( page.linearK[2][1] );
    *ptrI16++ = convS( page.linearK[5][1] );
    *ptrI16++ = convS( page.linearK[6][1] );
    *ptrI16++ = convS( page.dFk[3] );    
    coefficients_buffer[ 22 ] = (BYTE) T3CC0; 
    coefficients_buffer[ 23 ] = (BYTE) theOC;
    prepareSpecial( 0xEC, coefficients_buffer ); 
}
/*
void prepareFlashCheck(void){
static BYTE arr[ 24 ];
BYTE k = (reportFlashCheck-1) / calibrationRepeat ;
BYTE *flashPtr = (BYTE *)( page.imageAddr & 0xFF00 ) + ( k ) * 0xC00; //24 * 8 * 16;
BYTE bytes, bits, cnt;
    for( bytes = 0; bytes < 24; bytes++ ){
        arr[ bytes ] = 0xFF;
        for( bits = 0; bits < 8; bits++ ){
            cnt = 16; do { if( flashPtr[--cnt] != 0xFF ){ arr[ bytes ] ^= ( 1 << bits );  break; } }while( cnt );
            flashPtr += 16;
        }
    }
    prepareSpecial( 0xFB + (  k ), arr );
}
*/
/*
void prepareScaling(void){ 
BYTE k = (reportScaling-1) / calibrationRepeat;
    page.prepAddr = (UINT16)(&prepareScaling);
    prepareSpecial( 0xED + k, (from_flash ? (BYTE *)PAGEADDR : (BYTE *)&page) + k * 24 ); 
}
*/
//________________________________________________________________________________
void prepareScaling(void){ 
BYTE k = (reportScaling-1) / calibrationRepeat;
    page.prepAddr = (UINT16)(&prepareScaling);
    prepareSpecial( 0xED + k, base_ptr + k * 24 ); 
}
/*
void prepareScaling(void){
BYTE *ptr = radioPktBuffer;
UINT16 tag = 1000 + reportScaling;
//BYTE flg;
    mymemcpy( ptr+4, (BYTE *)&tag, 2 );
    copyMac( ptr+6 ); //memcpy( ptr+6, page.myMac, 6);  // mac   
    switch( reportScaling ){
        case 7:
            mymemcpy( ptr, (BYTE *)&page.defCycles2SoftKick, 4 );
            ptr+=12;
            page.prepAddr = (UINT16)(&prepareScaling);
            mymemcpy( ptr, (BYTE *)&page.defCycles2HardKick, 12 );
        break;
        case 6:
            mymemcpy( ptr, (BYTE *)&page.defCyclesToStep10Channel, 4 );                                                     // 4
            ptr += 12;                                                                                                // 12  
            // edAddr, gwAddr, repeaterChannel, repeaterPower, flags, showState, Critical500, Critical10, 
            // version, cyclesTo500, cyclesTo10, cyclesToStep500,  
            mymemcpy( ptr, (BYTE *)&page.edAddr, 20 );  

            //ptr += 12; // 4*sizeof(UINT16);
            //mymemcpy( ptr, (BYTE *)(&page.version),   2);       ptr += 2;                                              // 22
            //mymemcpy( ptr, (BYTE *)(&page.defCyclesTo500), 8);  ptr += 8;                                              // 30
        break;
        case 5:
            // *ptr++ = page.mpp;     *ptr++ = page.module;      *ptr++ = page.ov;       *ptr++ = page.oc;               // 4
            mymemcpy( ptr, (BYTE *)&page.mpp, 4 );                                                                      // 4
            
            ptr += 12;                                                                                                 // 12
            *ptr++ = page.channel; *ptr++ = page.radioPower;  *ptr++ = page.azimuth;  *ptr++ = page.positionInString; // 16
            *ptr++ = page.netId;   *ptr++ = page.myBunch;                                                             // 18
            mymemcpy( ptr, (BYTE *)(&page.installDate), 4 );  ptr += 4;                                                 // 22

            mymemcpy( ptr, (BYTE *)(&page.groupId),   2);     ptr += 2;                                                 // 24
            mymemcpy( ptr, (BYTE *)(&page.elevation), 8);
        break;
        case 4: 
            mymemcpy( ptr, (BYTE * )page.dFk, 4 );    ptr += 12; 
            mymemcpy( ptr, ((BYTE *)page.dFk)+4, 12); ptr += 12;
            mymemcpy( ptr, (BYTE * )page.vrefPolynom, 8); 
        break;
        case 3:
            mymemcpy( ptr, ((BYTE * )page.vrefPolynom)+8, 4); ptr += 12;
            mymemcpy( ptr, (BYTE * )page.linearK,     20); 
        break;
        case 2:
            mymemcpy( ptr, ((BYTE * )page.linearK)+20, 4); ptr+=12;
            mymemcpy( ptr, ((BYTE * )page.linearK)+24, 20); 
        break;
        case 1:
            mymemcpy( ptr, ((BYTE * )page.linearK)+44, 4);       ptr+= 12;
            // VinTurnOn, VinShutOff, VinDisableRadio, tkCurrent, tkPower
            mymemcpy( ptr, ((BYTE * )&page.VinTurnOn), 20); 
        break;
    }        
}
*/
/******************************************************************************
* Prepare request to join network
******************************************************************************/
void prepareJoinRequest(void){  
BYTE *ptr  = radioPktBuffer;
    //mymemset( ptr, 0xFF, 4); ptr+=4; 
    *ptr++ = 0xFF; *ptr++ = 0xFF; *ptr++ = 0xFF; *ptr++ = 0xFF;
    //mymemcpy( ptr, (BYTE *)&utc, 4 );     mymemcpy( ptr+4, (BYTE *)&ms, 2 ); 
    mymemcpy( ptr, (BYTE *)&utc, 6 ); 
    copyMac( ptr+6 ); //memcpy( ptr+6, page.myMac, 6 );
    prepareTxBuffer();      
    radioPktBufferTx[0] = PACKET_LENGTH_GW_2;
}
void quartz(void){
BYTE counter = 127;
static BOOL q = 0;
      if( q ) return; else q = 1;
      if( rcgen ) return;
      SLEEP &= ~OSC_PD_BIT;     // powering down all oscillators
      while(!XOSC_STABLE && counter-- ) halWait(2);      // waiting until the oscillator is stable
      asm("NOP");
      //CLKCON &= ~MAIN_OSC_BITS; // starting the Crystal Oscillator
      CLKCON = 0x89;  
      SLEEP |= OSC_PD_BIT;      // powering down the unused oscillator
}
/******************************************************************************
* Send the previously prepared data package
******************************************************************************/
extern DMA_DESC dmaConfig[3]; 
void sendData(BYTE *buffer, BOOL isRepeater ){  
    //if( page.fuseComm ) return;
    if( ((buffer[0] != PACKET_LENGTH_ED) && (buffer[0] != PACKET_LENGTH_GW_2)) || txDisabled  )  return; 
    RFST = STROBE_IDLE; si->radioMode = RADIO_MODE_UNDEF;            
    //setupTxBuffer( buffer );
    SET_WORD(dmaConfig[1].SRCADDRH,  dmaConfig[1].SRCADDRL, buffer );
    setupRepeater( isRepeater );
    /*
    if( page.is500Always || (page.isRelay && isRepeater) ){
                                                             atomicChange( CheckFlag, 5*TICKS_IN_MS );
                                                           //if( page.use250kbod ) need_wiggle = TRUE; 
    }else if( page.use12kbod && page.useFEC )                atomicChange( CheckFlag, 2*_slot - TICKS_IN_MS );
    else                                                     atomicChange( CheckFlag, _slot   - TICKS_IN_MS );
    */
    //quartz();
    //INT_GLOBAL_ENABLE(INT_OFF);
        si->radioMode = RADIO_MODE_TX;        
        // Send the packet
        DMAARM = DMAARM_CHANNEL1;     // Arm DMA channel 1
        RFST   = STROBE_TX;            // Switch radio to TX
    //INT_GLOBAL_ENABLE(INT_ON); 
}
/******************************************************************************
* @fn  25*.84
WPackage
* @brief       This function makes appropriate actions after receiving message from Gateway
* Parameters:
* @return void
******************************************************************************/
#define NDEV_MASK   0x1F
#define JOIN_MASK   0x80
#define HOP_MASK    0x40

static UINT32 utcLocal = 0;
static UINT16 msLocal  = 0;

extern INT16 perRssiOffset;
void parseGWPackage(void){
static BYTE bunch, plen;
signed char o = FREQEST, sh = FSCTRL0;
INT16 shift, networkDelay;
BYTE tmp;
//UINT16 temperatureProtectionDelay;
BOOL goodForCommands = FALSE;
//BOOL newProtocol;
    //if( page.fuseComm ) return;
    rc(); was_listening =  FALSE;
    ticks2ProcessCmd = 0;    
    plen = radioPktBufferRx[0];
    /*
    if( ( ( plen != PACKET_LENGTH ) && ( plen != PACKET_LENGTH_GW_2 ) ) ){
        atomicChange( ReceiveData,  TICKS_IN_MS );  // receive in 2 ms after getting any package, even a broken one
        return;
    }
    */
    if( ( 0 == ( radioPktBufferRx[ plen + 2 ] & 0x80 ) ) || ( ( plen != PACKET_LENGTH_GW ) && ( plen != PACKET_LENGTH_GW_2 ) ) ){
        //atomicChange( ReceiveData,  TICKS_IN_MS );  // receive in 2 ms after getting any package, even a broken one
        if( page.isRelay && (!page.is500Always) ) setupRepeater( got_gw );  else setupRepeater( FALSE ); 
        receive();
        return;
    }
    
    if( page.isRelay && (!page.is500Always) && ( radioPktBufferRx[1] == page.gwAddr ) && (ADDR == page.gwAddr) ){ 
        sendData( radioPktBufferRx, FALSE );  return;
    }else if( radioPktBufferRx[1] == page.edAddr ){
        goodForCommands = ( 0 != (PKTSTATUS&0x80) ); 
        //P1_1 ^= 1;
        //decodeDelay = *((BYTE *)0xF538);
        if( (!page.is500Always) && page.isRelay ) sendData( radioPktBufferRx, TRUE );  
        decodeDelay = *((BYTE *)0xF538);
        loadIV( page.curIV );  decode( (plen-2), (char *)(radioPktBufferRx+2), (char *)radioPktBuffer );
        
        // for SDAG debug
        //mymemcpy( (BYTE *)radioPktBuffer, (BYTE *)(radioPktBufferRx+2), (plen-2));
        
        bunch = radioPktBuffer[0]; ndev = radioPktBuffer[1]; 
        //mymemcpy( (BYTE *)&utcLocal, radioPktBuffer+2, 4 );   mymemcpy( (BYTE *)&msLocal, radioPktBuffer+6, 2);
        mymemcpy( (BYTE *)&utcLocal, radioPktBuffer+2, 6) ; //ndev& 0x20 ? 5 : 6 );
        if( 
            ((ndev & NDEV_MASK ) <= 16) && ( msLocal < 1000 ) &&
            //((utcLocal > utcLast) || ( mymemcmp( (BYTE *)&utcLocal, (BYTE *)&utcLast, 4 )==0 && msLocal>msLast)) // ver G
            ((utcLocal > utcLast) || ( (mymemcmp( (BYTE *)&utcLocal, (BYTE *)&utcLast, 4)==0) && (msLocal != msLast) ) ) //protects ONLY from replay attack
        ){
            BYTE goodTime = ( (utcLocal > utcLast) || ( ( mymemcmp( (BYTE *)&utcLocal, (BYTE *)&utcLast, 4)==0) && (msLocal > msLast) ) ); 
            //P1_1 ^= 1;
            //newProtocol = radioPktBufferRx[ plen ] & 1;
            //temperatureProtectionDelay = computeTI( si->packetReceived );
            temperatureProtectionDelay = *((BYTE *)0xF538);
            if( ++cyclesFromStart > 50 ) wasAHardResetOrPowerOn = FALSE;

            //lastRssi = convertRssiByte( (radioPktBufferRx[ plen + 1 ]) );
            tmp = radioPktBufferRx[ plen + 1 ];
            if(tmp < 128)   lastRssi = (tmp >> 1) - perRssiOffset;
            else            lastRssi =(((UINT16)tmp - 256) >> 1) - perRssiOffset;

            if( !page.is500Always ){
                //if(       (o > 0) && (o >  page.max_off) ) o = page.max_off;
                //else if(  (o < 0) && (o < -page.max_off) ) o = -page.max_off;
                if(       (o > 0) && (o >  1) ) o = 1;
                else if(  (o < 0) && (o < -1) ) o = -1;
                shift = (INT16)sh + (INT16)o;
                if( abs( shift ) > 96 )   addDF( shift<<2 );  else  FSCTRL0 = (INT8)shift; 
            }
//            INT32 f = 0; ((BYTE *)(&f))[0] = FREQ0; ((BYTE *)(&f))[1] = FREQ1; ((BYTE *)(&f))[2] = FREQ2; decodeDelay = (INT16)( f-defFreq );

            if( goodForCommands /*&& ( page.is500Always || ( 0!=(LQI & 0x7F) ) )*/ ){
                if( isOnTestStand ){ isOnTestStand--; /* cmdCount--; */}
                if( alignMPP ){ alignMPP = FALSE; ticks[ MppCycle ] = _mppCycle; }
                P1_1 ^= 1;
                bunchCount = 18;
                mymemcpy( (BYTE *)&cycles2SoftKick, (BYTE *)&page.defCycles2SoftKick, 4 ); //&cyclesDefaults[4], 4 );//(BYTE *)&page.defCycles2SoftKick, 4 );
                if( goodTime ) mymemcpy( (BYTE *)&utcLast, (BYTE *)&utcLocal, 6 );
                lastCycle = _slot * ( (ndev & NDEV_MASK) + ((ndev & JOIN_MASK)?1:4) ) + _loop_delay;
                //if( ( (2+(ndev & NDEV_MASK)) % 3 ) == 0 ){ static BYTE cnt = 1; if( 0 == --cnt ){ ticks[ MppCycle ] =  _mppCycle; cnt = 255; } }
                decodeDelay = ticks[ MppCycle ];
                // if( page.synch_freq && ((2+(ndev & NDEV_MASK)%3)==0) ){ static BYTE cnt = 1; if( 0 == --cnt ){ ticks[ MppCycle ] =  page.synch_phase; cnt = page.synch_freq; } }
                if( page.synch_freq && (((2+(ndev & NDEV_MASK))%3)==0) ){ static BYTE cnt = 1; if( 0 == --cnt ){ *((INT16 *)0xF500) = page.synch_phase; cnt = page.synch_freq; } }
                networkDelay = page.is500Always ? 0 : (2*TICKS_IN_MS);
                if( bunch == 0 ){ 
                    packagePrepared = FALSE;
                    atomicChange( DelayedPrepareData, 5*TICKS_IN_MS - networkDelay - temperatureProtectionDelay ); // was 40, give more time 
                    mymemcpy( (BYTE *)&utc, (BYTE *)&utcLocal, 6 ); 
                    if( ticks[ DelayedPrepareData ] <= 0 ) ticks[ DelayedPrepareData ] = -1;
                    //zeroPackageWasNotReceived = FALSE;
                } else {
                    if( page.use12kbod ){
                        if( page.gbunch && (page.gbunch - bunch) <= 18 )
                            atomicChange( DelayedPrepareData, lastCycle*( page.gbunch - bunch ) + 5*TICKS_IN_MS - networkDelay - temperatureProtectionDelay );  // was 40, give more time
                    }else{
                        if( page.gbunch && (page.gbunch - bunch) == 1 )
                            atomicChange( DelayedPrepareData, lastCycle + 5*TICKS_IN_MS - networkDelay - temperatureProtectionDelay );  // was 40, give more time
                    }
                }
                got_gw = 8;
                atomicChange( ReceiveData, lastCycle - _slot - 9*TICKS_IN_MS - temperatureProtectionDelay );  
                curBunch = bunch;
                //if( bunch != 0xFF && maxBunch < curBunch ) maxBunch = curBunch;
                if( !page.is500Always ) {                
                    /*
                    if(       (o > 0) && (o >  page.max_off) ) o = page.max_off;
                    else if(  (o < 0) && (o < -page.max_off) ) o = -page.max_off;
                    shift = (INT16)sh + (INT16)o;
                    if( abs( shift ) > 96 )   addDF( shift<<2 );  else  FSCTRL0 = (INT8)shift; 
                    */
                    if( ndev & HOP_MASK ){ 
                        atomicChange( Hop, ticks[ ReceiveData ] - 0*TICKS_IN_MS /*- temperatureProtectionDelay*/ ); 
                        cyclesToHop = 8;
                    }else                { cyclesToHop = 0;    atomicChange( Hop, 0 ); }
                }
    
                atomicChange( Cycle, lastCycle + 1 );

                if( page.is500Always ){
                      if( lastRssi > -95 ) //page.CriticalLevel500 ) 
                          { cyclesToStep500Channel = page.defCyclesToStep500Channel; cyclesTo10 = page.defCyclesTo10; }
                          //{ cyclesToStep500Channel = cyclesDefaults[2]; cyclesTo10 = cyclesDefaults[1]; }
                      else flags[ Cycle ] = TRUE;
                }else if( lastRssi > -102 ) //page.CriticalLevel10 )  
                          { cyclesTo500 = page.defCyclesTo500;                       cycles2Step10Channel = page.defCyclesToStep10Channel; }
                          //{ cyclesTo500 = cyclesDefaults[0];                       cycles2Step10Channel = cyclesDefaults[3]; }
                      else flags[ Cycle ] = TRUE;
            
                BYTE b = bunch;
                if( b!=255 && page.rbunch ) b %= page.rbunch;
            
                networkDelay = _slot * page.netId + _gw_delay - temperatureProtectionDelay;

                if(    (b!=0xFF) && ( b == page.myBunch ) )
                    atomicChange( SendData, networkDelay );
                else if( page.use12kbod && page.gbunch && (page.myBunch != 0xFF) && (b < page.myBunch) && ( (page.myBunch-b)<=18 ) && ((ndev & NDEV_MASK ) > page.netId) )
                    atomicChange( SendData, (page.myBunch-b)*lastCycle + networkDelay );
                else if( !page.use12kbod && page.gbunch && (page.myBunch != 0xFF) && (b < page.myBunch) && ( (page.myBunch-b)==1 ) && ((ndev & NDEV_MASK ) > page.netId) )
                    atomicChange( SendData, lastCycle + networkDelay );
                else if( (page.myBunch == 0xFF) && (page.netId == 0xFF) && ((ndev & JOIN_MASK) == 0) ) {
                    ticks[ DelayedPrepareData ] = 0; 
                    if( joinRequestWasSend > 0 )  joinRequestWasSend --;    
                    else {
                        ADCCON1 &= 0xF3; ADCCON1 |= 4;
                        joinRequestWasSend = page.network_cnst_1 + RNDH % page.network_cnst_2; 
                        atomicChange( SendData, (ndev & NDEV_MASK) * _slot + (RNDL % 5) * _join_slot + _gw_delay - temperatureProtectionDelay );    // 12 * 
                        flags[ PrepareJoin ] = TRUE; 
                    }
                }            

                if( ticks[ SendData ] ){
                //signed char adjustment = 5*TICKS_IN_MS; // was 24;
                //    if( bunch == 0 )                adjustment -= 8*TICKS_IN_MS;  // was 41;
                signed char adjustment = -3*TICKS_IN_MS; 
                    if( plen == PACKET_LENGTH_GW_2) adjustment += 11*TICKS_IN_MS; // was 57; 
                    if( page.is500Always ){
                        if( page.use250kbod )     adjustment -= 5*TICKS_IN_MS+3; 
                        else                      adjustment -= 5*TICKS_IN_MS; // was 22;
                    }//else if( page.isRelay )        adjustment -= TICKS_IN_MS/2; // 2; // commented 20 Feb 16:45 in order to make sure that repeater works when it is right after slave
                    ticks[ SendData ] +=  adjustment;
                }

                if( (!page.is500Always) && page.isRelay ) tickWait( 2*TICKS_IN_MS+1 ); // was 12
                parseCommands( plen-10 );
                if( page.treatLastByteAsChannel ){ if( !page.is500Always && (cyclesTo500 < 200) ){ curChannel = CHANNR = radioPktBufferRx[ plen ]; } }
                
                //lastRssi = radioPktBufferRx[ plen ];
                //setByte( SET_CUR_CHANNEL, radioPktBuffer[7] );
                
                if( ticks[ SendData ] < 0 ) ticks[ SendData ] = 0;
            }
        }//else ticks[ ReceiveData ] = 2;
    }
    setupRepeater( FALSE ); 
    //if( !enable2SendData ){
    //    flags[ PrepareData ] = 0; flags[ SendData ] = 0; 
    //}else 
//    if( (!page.is500Always) && page.isRelay ) setupRepeater( got_gw ); 
    if( (!page.is500Always) && page.isRelay ){ setupRepeater( got_gw ); receive(); }
//    receive();  // uncomment to get G+ behaviour
}
/******************************************************************************/
BYTE *nextSlot(){
    BYTE *ptr = (BYTE *)(PAGEADDR + PAGESIZE); 
    while( ptr && ( ptr <= (BYTE *)(PAGEADDR+0x3FF) ) ){ 
        BYTE i = 4; 
        do{ if( ptr[--i] != 0xFF ) break; }while(i);
        if( i ) ptr += 4; else return ptr; 
    }
    return NULL;
}
/******************************************************************************
* @fn  receive
* @brief       This function switch radio into receiving radioMode
* Parameters:
* @return void
******************************************************************************/
void receive(void){
    //if( page.fuseComm ) return;
    if( si->radioMode == RADIO_MODE_TX  ) { atomicChange( ReceiveData, 1 ); return; }
    RFST = STROBE_IDLE; si->radioMode = RADIO_MODE_UNDEF;            
    //INT_GLOBAL_ENABLE( INT_OFF );
        si->packetReceived = 0;
        radioPktBufferRx[0] = 0;    radioPktBufferRx[1] = 0;
        //quartz();
        si->radioMode = RADIO_MODE_RX;
        PKTLEN = PACKET_LENGTH_GW;  
        DMAARM = DMAARM_CHANNEL0;             // Arm DMA channel 0
//        P1_1 = 0;        
        RFST = STROBE_RX;                     // Switch radio to RX
    //INT_GLOBAL_ENABLE( INT_ON );
}
/******************************************************************************
* @fn  computeADCs
* @brief       This function makes all the regular computations
* Parameters:  
* @return void
******************************************************************************/
float vref, t;
//static float p1, c1, corrP, corrC;
float computeAdjustment( float *k ){ return 1.0 + t*(*k); } 
float multSaver( INT32 *p32, float *fptr ){ return ((float) *p32) * invRef * (*fptr); }

//#pragma optimize=s 9
void computeADCs(BOOL timeout, BOOL accumulateEnergy){
BYTE i, counter;
// static is a way to reduce stack usage 
INT32  *accptr;
UINT16  *adcptr;
static INT32  acc, df;//, thirtyThree_mV; 
static float mV, rOff;
#ifndef MidString
static float tin;
#endif
static float temp[N_CHANNELS];
float *coeff;
signed char off = FSCTRL0;

    lastOff  = (INT32)off; 
    accptr = adcs;
    adcptr = adcBuf[0]; // using the fact that one buffer is defined right after another
                        // in general it is a VERY DANGEROUS ASSUMPTION, but with this C compiler it works
    if( (flags[ PackageReceived ]) || (flags[DataSent]) ){ flags[AltCheckVin] = TRUE; return; }
    if( accumulateEnergy ){
        adcptr += (bufferCount - 1) & ( BUFFER_SIZE-1 );
        for( i = 0; i < N_CHANNELS; i++, adcptr+=BUFFER_SIZE )
            *accptr++ = *adcptr; 
        adcs[ 3 ] += gnd_shift/8;
    }else{
        for( i = 0; i < N_CHANNELS; i++ ){ // 128 averaging
            counter = BUFFER_SIZE;
            acc = 0; while( counter-- )  acc += *adcptr++;
            if( (flags[ PackageReceived ]) || (flags[DataSent]) ){ flags[AltCheckVin] = TRUE; return; }
           *accptr++ = acc;
        }
        adcs[ 3 ] += gnd_shift;
    }
    if( (flags[ PackageReceived ]) || (flags[DataSent]) ){ flags[AltCheckVin] = TRUE; return; }
    /*
    if( page.thirty_three_mv ){
        thirtyThree_mV = (INT32) ( 33 * ( adcs[ REF ] - adcs[ EXT_GND ] ) / ( page.vrefPolynom[0] - 33 ) );
        adcs[ EXT_GND ] -= thirtyThree_mV;
    }
    */
    substractGround( adcs, 0, N_CHANNELS );
    //if( page.thirty_three_mv ) adcs[ EXT_GND ] += thirtyThree_mV;
    
    if( (flags[ PackageReceived ]) || (flags[DataSent]) ){ flags[AltCheckVin] = TRUE; return; }
    if( adcs[ REF ] == 0 ) return; 
    invRef = 1.0 / (float)adcs[ REF ];
    // compute correct temperature, correct vref, coeffcient to convert sum of counts to physical values
    if( (flags[ PackageReceived ]) || (flags[DataSent]) ){ flags[AltCheckVin] = TRUE; return; }
    _coeff = page.linearK[4];    t   = lin(  TEXT );
#ifndef MidString
    _coeff = page.linearK[5];    tin = -lin( TINT );
#endif
    if( (flags[ PackageReceived ]) || (flags[DataSent]) ){ flags[AltCheckVin] = TRUE; return; }
    adcs16[ 4 ]  = (INT16)mult(t,   100);
#ifndef MidString
    adcs16[ 5 ] = ((INT16)mult(tin, 100));
#endif
    
    _coeff = page.dFk;
    if( (flags[ PackageReceived ]) || (flags[DataSent]) ){ flags[AltCheckVin] = TRUE; return; }
    /*
    if( abs( t - tin ) > 20.0 ){
             rOff = cube_coeff( 0.5*( tin+t ) );
    }else    rOff = cube_coeff( t );    //rOff = ( ( ( coeff[0]*t + coeff[1] )*t + coeff[2] )*t + coeff[3] );
    */
    //if( abs( adcs16[ 4 ] - adcs16[ 5 ] ) > 2000 ) t = 0.5*( tin+t );
    rOff = cube_coeff( &t );    //rOff = ( ( ( coeff[0]*t + coeff[1] )*t + coeff[2] )*t + coeff[3] );
    
    df = (INT32) rOff;
    if( (flags[ PackageReceived ]) || (flags[DataSent]) ){ flags[AltCheckVin] = TRUE; return; }
    theRealOffset = (INT8)( (delta + (lastOff<<2) - ( df )) >> 2 );

    // adjust frequency by temperature
    if( timeout && ( si->radioMode == RADIO_MODE_UNDEF ) ) addDF( df /*+ page.theDelta*/ - delta ); 
    //RFST = STROBE_CAL; 

    coeff = page.vrefPolynom;
    if( (flags[ PackageReceived ]) || (flags[DataSent]) ){ flags[AltCheckVin] = TRUE; return; }
    vref  = coeff[0] * ( 1.0 - coeff[1]*sqr( t - coeff[2] ) );
    t -= 25.0;
    invRef  *= vref; // / ((float)adcs[ REF ]);  
    // scale everything to physical units
    // Vout  0      Pin   1     Iout  2      Gnd ext 3        Ref 4      Vin  5       Text  6     Tin 7
    for( i = 0; i<N_CHANNELS; i++) 
        /*if( indices[ i ] != 0xFF )*/ {
            mV = invRef * ((float)adcs[i]); //temp[i]; // adcs values (all but REF) should be scaled to fit into UINT16 - k should be accordingly set
            if( (flags[ PackageReceived ]) || (flags[DataSent]) ){ flags[AltCheckVin] = TRUE; return; }
#ifdef MidString
            if( (i == 8) && (REFERENCE == ADC_REF_AVDD) ) mV *= computeAdjustment( &page.tkCurrent ); // t - 25
            if( (i == 7) && (REFERENCE == ADC_REF_AVDD) ) mV *= computeAdjustment( &page.tkCurrent ); // t - 25
#else
            if( i == 1 )  mV *= computeAdjustment( &page.tkPower );   // t - 25
#endif
            if( i == 2 )  mV *= computeAdjustment( &page.tkCurrent ); // t - 25
            if( indices[ i ] == 0xFF ){ temp[i] = mV; continue; }
            _coeff = page.linearK[  indices[i] ];
            temp[i] = lin_coeff( mV );
            if( (flags[ PackageReceived ]) || (flags[DataSent]) ){ flags[AltCheckVin] = TRUE; return; }
        }

    //temp[ 1 ] *= vref * invVref0;               // power correction
    // crosstalk   Istr

    //temp[ 0 ] -= mult( temp[2]*0.0001,  page.shunt );                  // Vout
    //                  Power           Vin
    //temp[ 5 ] += mult( temp[1]/temp[5], page.shunt );      // Vin
    
    #ifdef MidString    
        adcs16[ 0 ] = convU( temp[ 0 ] ); // temp[ 0 ] ); 
        adcs16[ 1 ] = convU( temp[ 1 ] ); // temp[ 1 ] ); 
        if( (flags[ PackageReceived ]) || (flags[DataSent]) ){ flags[AltCheckVin] = TRUE; return; }
        adcs16[ 3 ] = convU( temp[ 5 ] ); // temp[ 5 ] );
        adcs16[ 5 ] = convS( temp[ 8 ] ); // Iin2 
        //adcs16[ 7 ] = (INT16)(adcs[7]+adcs[3]);  //temp.debug stuff
        adcs16[ 6 ] = convS( temp[ 7 ] );
    #else
        adcs16[ 0 ] = convU( temp[ 0 ] - mult( temp[2]*0.0001,  page.shunt ) ); //temp[ 0 ] ); 
        adcs16[ 1 ] = convU( temp[ 1 ] *= vref * invVref0 );   //temp[ 1 ] ); 
        adcs16[ 3 ] = convU( temp[ 5 ] + mult( temp[1]/temp[5], page.shunt ) );// temp[ 5 ] );
    #endif    
    adcs16[ 2 ] = convS( temp[ 2 ] );
    
    //adcs16[ 6 ] = (UINT16)adcs[ EXT_GND ];
    if( (flags[ PackageReceived ]) || (flags[DataSent]) ){ flags[AltCheckVin] = TRUE; return; }
    
    if( accumulateEnergy ){    
        //static BYTE _history = 0;
        checkVin();
        /*_history <<= 1; 
        if( ((INT16)adcs16[ 2 ]) > 15000 ){ //page.cur_noise ){ 
            deltaEnergy += (double) ( temp[2] * temp[0] * KE ); // W*h
            _history |= 1;
        }else{
            if( _history == 0xFE ){
                //BYTE *ptr = nextSlot();
                energy += deltaEnergy; 
                zerofill( (BYTE *)&deltaEnergy, 4 ); 
                //if(      ptr > ENERGY_WRITE_LIMIT ) savePB();
                //else if( ptr ) initiateTransfer( (BYTE *)&energy, ptr, 4 );
            }
        }*/
    }
}
//______________________________________________________________________________________________________________________________
//BOOL checkPage( BYTE val ){ return val && val != 0x1F && ( ( val ^ (page.imageAddr>>9) ) & 0x20 ); }
//______________________________________________________________________________________________________________________________
/******************************************************************************
* @fn  setByte
* @brief       set the byte value to register
* Parameters:  byte reg - register, byte val - value
* @return      void
******************************************************************************/
void setByte( BYTE reg, BYTE val ){
BYTE *ptr; BYTE *pptr = (BYTE *)&page;
    /*
    if(page.isRelay && !page.is500Always ) switch(reg){
        case SET_CUR_CHANNEL: case SET_CHANNEL:     case RADIO_POWER:
        case IS_RELAY:        case IS_500_ALWAYS:   case REPEATER_POWER:
            tickWait( 3+2*TICKS_IN_MS );  // give repeater chance to finish transmission
        break;
    }*/
    switch(reg){
        case MPP:  // MPP
            if( val > 1 ) break;
            if( val )  P1_3 = 1;   else P1_3 = 0;
            if( val ) { 
                atomicChange( BypassMode, 0 ); // do not need that delayed mpp off anymore
                if( turnOnFlag || ticks[ TurnOn ] ) { ov_startup(); turnOnFlag = FALSE;  ticks[ TurnOn ] = 0; } //P1 |= 0x10; }
            }
        break;
        case MODULE: // Module on / off
            if( val > 1 ) 
              break;
            if (val == 1) // If Module on, then set FET failure check appropriately
            {
              *ModuleOffRememberedPtr = ModuleStatusCanTurnOn; // Module On command resets this
              fetFailure = FALSE; // Module On command resets this
              netValue = MainNetValueStandard;  // Set base value back for main image
              if (page.fetFailureCount != 0)
                checkForFETFailure = TRUE; // Re-enable check
              else
                checkForFETFailure = FALSE; // PB indicates don't check
            }
            if( val && ((P1 & 0x18)==0) ) { P1_3 = 1; ov_startup(); /*P1 |= 0x18;*/ atomicChange( BypassMode, TICKS_IN_SEC );            } // bypass mode with delay
            //else                         { if( val ) P1 |= 0x10; else P1 &= ~0x10; }
            else 
            { 
              if( val ){ 
                if( P1_4 == 0 ) 
                  ov_startup(); 
              }
              else 
              { 
                P1_4 = 0; 
                *ModuleOffRememberedPtr = ModuleStatusOff; 
              } 
            }
        break;
        case SET_T3CH0:  
            //if( !page.fuseOVOC ){ 
                T3CC0 = val;                                      
//                #ifdef MidString
//                compute98();
//                #endif
            //}
        break;
        case SET_T3CH1:  
          //if( !page.fuseOVOC ){
              T3CC1 = oc2timer( theOC = ( setOC = val ) );      
//              #ifdef MidString
//              compute98();
//              #endif
          //}
        break;

        case MAC0: case MAC1: case MAC2: case MAC3: case MAC4: case MAC5:   page.myMac[ reg-MAC0 ] = val;         break;
        /*
        case SET_CUR_CHANNEL:
            if( !page.is500Always && (cyclesTo500 < 200) ){ RFST = STROBE_IDLE; curChannel = CHANNR = val;  }
        break;
        */
        case SET_CHANNEL:    
            page.channel = val;  setup_hopper( page.channel );   
            if( !page.is500Always ){
                //P1_1 ^= 1;
                curChannel = CHANNR = page.channel;
                ticks[ Hop ] = 0; 
                cycles2Step10Channel = cyclesTo500 = 300;
                RFST = STROBE_IDLE; 
                softKick(); 
            }
        case RANDOMIZE:
            RNDL = page.myMac[4]; RNDL = page.myMac[5]; 
            //srand( page.myMac[4]*256 + page.myMac[5] ); 
        break;

        case FLASH_REFRESH2:   refreshTheFlash();   break;
  
        //case FUSE_Comm:  page.fuseComm = 1; savePB();   break;
        //case FUSE_OVOC:  if( !page.fuseOVOC ){ page.fuseOVOC = 1; page.ov = T3CC0; page.oc = setOC; savePB(); }  break;

        case FLASH_AVAILABLE: 
            page.mpp    = P1_3;
            page.module = P1_4;
            //if( !page.fuseOVOC ){ page.ov = T3CC0; page.oc = setOC; }
            page.ov = T3CC0; page.oc = setOC;
            // go through
        case COEFFICIENTS_AVAILABLE:  
            savePB();
        break;
        /*
        case CW_MODE:   
            //HAL_INT_ENABLE(INUM_RF,  INT_OFF);    // Disable RF general interrupt
            real_cw_mode = TRUE;
            //ticks[ SendData ] = 1; ticks[ ReceiveData ] = 0;
        break;
        */
        //case RESTART_MEASUREMENT:   bufferCount = 0; adcCount = 0xFF; tempAdcsCount = 0; enable2SendData = FALSE;    break;
        case DISSOLVE_NETWORK:      
          RNDL = page.myMac[4]; RNDL = page.myMac[5]; 
          //srand( page.myMac[4]*256 + page.myMac[5] );
          page.netId = page.myBunch =  0xFF;  
          //savePB();
          //maxBunch = 0;  
          ticks[ DelayedPrepareData ] = 0; ticks[ SendData ] = 0; 
        break;

        case CALIBRATION_FROM_FLASH:   pptr = (BYTE *)PAGEADDR; //0x400;        /* go through */
        case REPORT_SCALING:   
          if( !reportScaling ){
                if( base_ptr==NULL ){
                    base_ptr = pptr; //(BYTE *)&page;
                    reportScaling    = (PAGESIZE/24) * calibrationRepeat;      
                }else reportScaling    = val * calibrationRepeat;
          }
        break;
        case READ_COEFFICIENTS: reportScaling = val; base_ptr = coefficients_buffer; if( reportScaling == 0 ) reportScaling = 1; break;

        //case CHECK_FLASH:            reportFlashCheck = calibrationRepeat*5 + 1; /* 5*4-1 */;    break;
        
        case RADIO_POWER: page.radioPower = val;      PA_TABLE0 = val;  break;
        
//        case PRODUCTION:       page.production = val;                   break;
        case ENSURE_WD_RESET:  page.ensureWDReset = val;                break;
        //case USE_12_kBod:   
        //  page.use12kbod = val;  
          //savePB();
          //tickWait(8*TICKS_IN_MS); 
        //  softKick(); 
        //break; /* write to flash as well */
        
        //case USE_FEC:          page.useFEC = val;     if( page.useFEC ) MDMCFG1 |= 0x80; else MDMCFG1 &= 0x7F;     break;
        case ENABLE_HOPPING:   page.hoppingAllowed  = val; break;     
        case REPEATER_CHANNEL: page.repeaterChannel = val; break;     
        case IS_RELAY:         page.isRelay = val;   
                               if( !val ) setupRadio( curChannel );     
        break;   
        case USE250:           page.use250kbod = val; 
                               // go through
        case IS_500_ALWAYS:    
            if( reg == IS_500_ALWAYS ) page.is500Always = val;     
            if( page.is500Always ){ 
                 cyclesTo10 = page.defCyclesTo10; //cyclesDefaults[1]; //page.defCyclesTo10; 
                 cyclesTo500 = 0;             
                 cyclesToStep500Channel =  page.defCyclesToStep500Channel; //cyclesDefaults[2]; //page.defCyclesToStep500Channel; 
                 setupRadio( curChannel = page.starting500Channel );
                 atomicChange( Hop, 0 );                  
            }else{      
                 cyclesTo10 = 0;                  
                 cyclesTo500 = page.defCyclesTo500; 
                 cycles2Step10Channel   = page.defCyclesToStep10Channel;//cyclesDefaults[3]; //page.defCyclesToStep10Channel;
                 curChannel = page.channel;
                 //setupRadio( curChannel = page.channel ); 
            }    
            //softKick();
        break;     
        case REPEATER_POWER:           page.repeaterPower = val;   if( page.is500Always ) setupRadio( page.channel ); break;
        case SEARCH_FOR_COMMUNICATION: page.searchCommunication = search_stat = val;  break;

        // win 7 bytes. 
        case SET_SHOW_STATE:  case SetCriticalLevel500:   case SetCriticalLevel10:
            (&page.showState)[ reg-SET_SHOW_STATE ]  = (signed char)val;            
        break;
        
        case END_OF_TEST: 
            page.channel = 0;
            page.myBunch = 0xFF; page.netId = 0xFF;  page.groupId = 0xFFFF;
            page.edAddr = 2; page.gwAddr = 1;
            savePB();
        // go through ...
        //case CLEAR_MAX_BUNCH:             maxBunch = 0;                         break;
        case SYNCHRONIZE_MPP_CYCLES:      ticks[ MppCycle ]        = _mppCycle; break; 
        case SET_START500_CHANNEL:
            page.starting500Channel = val; if( page.is500Always ) setupRadio( curChannel = val );
        break;
        /*
        case CLEAR_THE_PAGE:
          val <<= 1;  
          if( !P1_4 && ( val > 48 ) && ( val < 64 ) ){
             tickWait( 8 * TICKS_IN_MS );
             clearAndHop( val );
             //clearThePage( val );
             ticks[ ReceiveData ] = 25*TICKS_IN_MS;
             //ticks[ ClearThePage ] = 12*TICKS_IN_MS;
             //page2clear = val;
          }
            //val <<= 1;  if( val && (val < 0x3E) && ( ( val ^ (page.imageAddr>>9) ) & 0x20 ) ) clearThePage( val );
        break;
        */
        case TRY_OTHER_IMAGE:            page.tryOtherImageFirst = val;        break;
        case SWITCH_SECURITY:
            mymemcpy( (BYTE *)page.curKey, nextKey, 16 ); mymemcpy( (BYTE *)page.curIV, nextIV, 16 );
            zerofill( nextKey, 32 );
            loadKey(  page.curKey );
        break;
        case CALIBRATION_REPEAT:
            if( val > 0 && val < 32 ) calibrationRepeat = val; else calibrationRepeat = 4;
        break;
        //case SPECULATIVE_REPORT: page.speculative_report = val;                  break;
        case STAY_IN_RX:         page.stay_in_rx  = val;                         break;
        //case THIRTY_THREE_MV:    page.thirty_three_mv = val;                     break;        
        
        case TEST_1:  case TEST_2:   case TEST_3:   case TEST_4:
            ptr = (BYTE *)&page.tests[reg-TEST_1]; 
            *ptr++ = val; 
        // go through
        case INSTALL_DATE:
            if( reg == INSTALL_DATE ) ptr = (BYTE *)&page.installDate;
            mymemcpy( ptr, (BYTE *)&utcLocal, 4); 
        break;
        
        case REP_STEP:   if( val < 5 && val >50 )  break;  // else go through
        
        case OV_STEP:  case T_OC_JUMP: case OV_STARTUP: 
            //if( page.fuseOVOC )       break;
        // go through 
        case T_SUPERHOT:              case T_HOT:          
        case T_COLD:                  case T_TIME:
        case NETWORK_CNST_1:          case NETWORK_CNST_2:  
        case OC_STEP:                 case SHUNT:  
        case MAX_OFF:                 case VIN_LIMIT: 
        case VIN_TURN_ON:             case VIN_SWITCH_OFF:
            ptr = (BYTE *)&page.repStep; ptr[ reg-REP_STEP ] = val; break;
            
        case SHORT_CIRCUIT_LEVEL: 
            #ifdef MidString
                if( val == 0 )
                    { ocShortCircuitLimit = 255; T3CC1 = oc2timer( theOC ); }
                else if( ocShortCircuitLimit < 255 )
                    { ocShortCircuitLimit = val; T3CC1 = oc2timer( theOC ); }
            #endif
            /*go through */
        case VIN_DISABLE_RADIO: case SET_RBUNCH: case SET_GBUNCH: //DEV250:
            ptr = (BYTE *)&page.vin_disable_radio; ptr[ reg-VIN_DISABLE_RADIO ] = val; 
        break;
        case LOW_UTC: case HIGH_UTC: 
            mymemcpy( ((BYTE *)(&page.utcLow))+4*(reg-LOW_UTC), (BYTE *)&utcLast, 4);
        break;

        //case TEST_STAND: if( val == 1 ) isOnTestStand = 11; break; 
        
        case REPORT_UTC: page.reportUTC = val; break;
        case BANDWIDTH_500:  mdm_rate1[0] = val; softKick(); break;
        case TREAT_LB_AS_CHAN : page.treatLastByteAsChannel = val; break;
        case SYNCH_PHASE:   page.synch_phase = val;  break;
        case SYNCH_FREQ:    page.synch_freq = val;   break;              
        case BANDWIDTH250:  page.bandwidth250 = val; break;
        case TEMP_CALIBRATE:softKick(); break;
        case SET_MPPF:  si->mppCycle = val;  break;
       
        case K_OV_VOLT:  case K_OC_CUR:
            ptr = (BYTE *)&page.k_ov_volt; ptr[ reg - K_OV_VOLT ] = val; 
        break;
        
        case XTAL_OFF: if( val == 1 ){
          //BYTE counter = 127;
          //CLKCON |= OSC_BIT;                    // starting the RC Oscillator
          //while(!HIGH_FREQUENCY_RC_OSC_STABLE && counter-- ) halWait(2); 
          //SLEEP |= OSC_PD_BIT;                  // powering down the unused oscillator
          rcgen = TRUE;
        }break;
    }
}

void savePB(){
//new   rc();
  asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
  asm( "NOP" );
  asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
  asm( "NOP" );
  asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
  asm( "NOP" );
  asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
  asm( "NOP" );
  asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
  asm( "NOP" );
  asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
  asm( "NOP" );
  asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
  asm( "NOP" );
  asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
  asm( "NOP" );
  asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
  asm( "NOP" );
//  if( !isOnTestStand ) return;
  //if( !isOnTestStand ){ isOnTestStand = 20; cmdCount--; return; }
  if( !isOnTestStand ){ isOnTestStand = 20; cmdCount--; CLKCON = 0x89; asm( "NOP" );  return; }  
  if( *((BYTE *)0xF53B) != 0xAA ) reset(); 
  //clearAndHop( 2 ); 
  //clearThePage( 2 );
  //page.barrier = 0xAAAA; page.post_barrier = 0x5555;
  //P1_4 = 1; // debug statement for measuring save PB time
  asm("CLR  0xA8.7");
  asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
  asm( "NOP" );
  asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
  asm( "NOP" );
  asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
  asm( "NOP" );
  asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
  asm( "NOP" );
  asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
  asm( "NOP" );
  asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
  asm( "NOP" );
  if( !isOnTestStand ) reset();  
  FADDRH = 0x2E; FWT = 0x21; FCTL = 1; asm("NOP"); // this line actually clears the page
  FADDRH = 0;   
  asm("SETB 0xA8.7");
  tickWait( 25*TICKS_IN_MS );
//  if( !isOnTestStand ) return;
  initiateTransfer( (BYTE *)&page, (BYTE *)PAGEADDR, PAGESIZE ); //+8 );
  tickWait( 11*TICKS_IN_MS );  // was 5 ms
  //softKick();
  //FADDRH = 0; 
  restoreRadio();
  isOnTestStand = 0;
  //P1_4 = 0; // debug statement for measuring save PB time
  CLKCON = 0x89;  asm( "NOP" );
   asm( "NOP" ); asm( "NOP" ); asm( "NOP" ); asm( "NOP" ); asm( "NOP" );
}
/*
BYTE clearAndHop( BYTE p ){
   clearThePage( p );
   if( ticks[ Hop ] && page.hoppingAllowed && !page.is500Always ){
       CHANNR = curChannel; hop(); curChannel = CHANNR; 
       atomicChange( Hop, lastCycle ); //- _slot );  
   }
   return 1;
}
*/
/******************************************************************************
* @fn  setInt
* @brief       set the int16 value to register
* Parameters:  byte reg - register, int val - value
* @return      void
******************************************************************************/
void setInt( BYTE reg, UINT16 val ){
UINT16 *ptr = &page.defCyclesTo500;
BYTE *pptr = (BYTE *)&page;
INT16 *ptr_int16 = NULL;

    switch(reg){     
        case SET_GROUP_ID: page.groupId = val; break;
        case JOIN: page.netId = (val & 0xF); page.myBunch = ( val >> 8 ) & 0xFF;  break;
        case SET_ED_GW:
            // this check for not 0 is a mistake, because it prohibits setting addr to 0, was a big issue on Remington Hi-Volts !!!
            //if( val & 0xFF   ) page.edAddr = val & 0xFF;
            //if( val & 0xFF00 ) page.gwAddr = ( val >> 8 ) & 0xFF;
            page.edAddr = val & 0xFF;
            page.gwAddr = ( val >> 8 ) & 0xFF;
            ADDR = page.edAddr;
        break;
        case FetFailureCount:
          // The following if is meant to use = (not ==), set value
          // and then compare if fetFailureCount to 0 to set check logic
          if ((page.fetFailureCount = val) != 0)
            checkForFETFailure = TRUE;
          else
            checkForFETFailure = FALSE;
          break;
        case FetCurrentThreshold:                                                                          
          page.fetCurrentThreshold = val;
        break;
        case FetDeltaCurrent:
	  page.fetDeltaCurrent = val;
        break;
        case BootImage: // try before buy
            tickWait( 12 * TICKS_IN_MS);
            // changing timers to boot (old) values
            /*if( val == 0x603F ){
                T1CTL &= 0xFC;
                SET_WORD(T1CNTH, T1CNTL, 0);
                SET_WORD(T1CC0H, T1CC0L, 600 - 1);
                T1CTL |= 0x04  | 0x02;
                T1CCTL0 = 0x44;
                si->mppCycle = 300;   // change when we change 8k image and Boot
            }*/
            if( val == 0x83F ) break;
            {void (*f)( void ) = ( void (*)( void ) )val;  /*si->interImageCommunications[ JustLoaded ] = TRUE;*/ (*f)();}          
        break;
        case SyncWord: 
          SYNC0 = val & 0xFF;    SYNC1 = ( val >> 8 ) & 0xFF;
          page.syncword = val; 
        break;
        /*
        case SetCyclesTo500:            page.defCyclesTo500 = val;            break;
        case SetCyclesTo10:             page.defCyclesTo10  = val;            break;
        case SetCyclesToStep500Channel: page.defCyclesToStep500Channel = val; break;
        case SetCyclesToStep10Channel:  page.defCyclesToStep10Channel  = val; break;
        case SetCyclesToLightKick:      page.defCycles2LightKick = val;       break;
        case SetCyclesToSoftKick:       page.defCycles2SoftKick = val;        break;
        case SetCyclesToHardKick:       page.defCycles2HardKick =val;         break;
        */
        
        case SetCyclesTo500:                    case SetCyclesTo10:             
        case SetCyclesToStep500Channel:         case SetCyclesToStep10Channel:  
        case SetCyclesToSoftKick:               case SetCyclesToHardKick:   
        case SetImageAddr:
        case SetVersionLow:                     case SetVersionHigh:
        //default:
            //if( (reg > (SetCyclesTo500-1) ) && ( reg < (SetVersionHigh+1) ) ){
                //UINT16 *ptr = &page.defCyclesTo500;
                ptr[ reg - SetCyclesTo500 ] = val;
            //}
        break;
        case SetBasePtr:
            if( (base_ptr == NULL) && (((UINT16)val) > 0x3FF) && (((UINT16)val) < 0x800) )  base_ptr = (BYTE *)val;
        break;  
        case SetCurNoise: page.cur_noise = val; break;
        //case GndShift:    gnd_shift = val;      break;
        case SetOscLowCurrentLimitInmA:
        case SetOscDeltaV:
        case SetOscOVStartupDelay:
        case SetOscAfterStartupDelay:
        case SetOscLowCurrentOnlyDelay:
          ptr_int16 = &page.oscLowCurrentLimitInmA;
          ptr_int16[reg - SetOscLowCurrentLimitInmA] = val;
        break;
        case ReadPBOneCommand: 
            if( (val>>12)&0xF ) pptr = (BYTE *)0x5C00; 
            base_ptr = pptr + 24*((val>>8)&0xF);
            calibrationRepeat = (val>>4)&0xF;
            reportScaling  = (val&0xF) * calibrationRepeat; 
        break;
        case SetPBByte: pptr[ ( val >> 8 )&0xFF ] = val & 0xFF; break;
    }
}
/******************************************************************************
* @fn  setLong
* @brief       set the long value to register
* Parameters:  byte reg - register, long val - value
* @return      void
******************************************************************************/
//void setLong( BYTE reg, UINT32 val ){  if( reg == SET_DF_TOLERANCE ){ page.dF_Tolerance = val;  } }
/******************************************************************************
* @fn  setFloat
* @brief       set the float value to register
* Parameters:  byte reg - register, float val - value
* @return      void
******************************************************************************/
void setFloat( BYTE reg, float *ptr){
//BYTE oc = 0;
//float val = *ptr;
BYTE *dst = NULL;
BOOL recomputeOC2Timer = FALSE;
    switch(reg){     
        case SET_DFK0:  case SET_DFK1:  case SET_DFK2:  case SET_DFK3:     
            //page.dFk[reg-SET_DFK0] = val;    
            dst = (BYTE *) &page.dFk[reg-SET_DFK0];
            if( reg == SET_DFK3 ) page.pureOffset = ((INT16) (*ptr))+20;
        break;
        //case SET_VIN_TURN_ON: case SET_VIN_SHUT_OFF: (&page.VinTurnOn)[reg-SET_VIN_TURN_ON]  = val;      return;
        
        case SET_TK_CUR: case SET_TK_POW:          
            //(&page.tkCurrent)[reg-SET_TK_CUR] = val;   
            dst = (BYTE *) &( (&page.tkCurrent)[reg-SET_TK_CUR] );
        break;
        
        //case SET_VIN_DISABLE_RADIO:                                        page.VinDisableRadio  = val;  return;

        case SET_LC00:  case SET_LC01:  case SET_LC10:  case SET_LC11:  
        case SET_LC20:  case SET_LC21:  case SET_LC30:  case SET_LC31:  
        case SET_LC40:  case SET_LC41:  case SET_LC50:  case SET_LC51:  // linearK [6][2]  
        case SET_LC60:  case SET_LC61:
            if( reg == 14 ){ zerofill( (BYTE *)&k_oc2timer, 4 );  recomputeOC2Timer = TRUE; }
            reg -= SET_LC00;
            //page.linearK[reg>>1][reg&1]     = val; 
            dst = (BYTE *) &page.linearK[reg>>1][reg&1]; 
//            if( reg == 4 ){
//                T3CC1 = oc2timer( theOC ); // lin coeff for 
//                #ifdef MidString
//                compute98();
//                #endif
//            }
        break;
        case SET_VREF0:                   invVref0 = 1 / (*ptr);                    // go through
        case SET_VREF1: case SET_VREF2:   
            //mymemcpy( (BYTE *) &page.vrefPolynom[reg-SET_VREF0], (BYTE *) ptr, 4 );
            dst = (BYTE *) &page.vrefPolynom[reg-SET_VREF0];
        break;
    }
    if( dst ) mymemcpy( dst, (BYTE *) ptr, 4 );
    if( recomputeOC2Timer ) T3CC1 = oc2timer( theOC );
}
/******************************************************************************/
#define NULLOP          0
#define SETREG          0x10
#define RESET           0x20
#define SILENCE         0x30
#define CALIBRATE_R     0x40
#define SET_MAC_CMD     0x50
#define BOOT_OVER_AIR   0x60
#define SET_NEXT_KEY    0x70
#define SET_NEXT_IV     0x80
#define ASVOL_NETCMD    0x90
#define SHORT_NETJOIN   0xF0

#define BROADCAST       0
#define STRINGADDR      1
#define NIADDR          2
#define MACADDR         3

#define FLOAT_VAL       0
#define BYTE_VAL        (1<<2)
#define SHORT_VAL       (2<<2)
#define LONG_VAL        (3<<2)

/******************************************************************************
* @fn  parseCommands
* @brief       This function parses commands and sets global flags
* Parameters:
* @return void
******************************************************************************/
void parseCommands(BYTE l){
BYTE *ptr     = radioPktBuffer + 8;
BYTE cmd, reg;
BOOL flag = TRUE, /*addrFlag = FALSE,*/ individualAddr = FALSE;
static UINT16 cmdCnt = 0; //, i;
    while( *ptr && (l--) ){
        cmd = *ptr++; 
        if( (cmd & 0xF0) == SHORT_NETJOIN ){
            reg = *ptr++;
            if( mymemcmp( page.myMac, ptr, 6 ) == 0 ){ page.netId = (cmd & 0xF); page.myBunch = reg; }
            ptr += 6;
        }else{
            if( search_stat == 2 ) search_stat = page.searchCommunication;
            //mymemcpy( (BYTE *)&cmdCnt, ptr, 2 ); 
            cmdCnt = *((UINT16 *)ptr);
            ptr+=2;
            switch( cmd & 0x3 ){ 
                case BROADCAST:                                                             flag = TRUE; break;
                case STRINGADDR: flag = ( *((UINT16 *) ptr) == page.groupId );              ptr += 2;    break;
                case NIADDR:     flag = ( ptr[0] == page.myBunch && ptr[1] == page.netId ); individualAddr = flag; ptr += 2;    break;
                case MACADDR:    
                    flag = ( mymemcmp( page.myMac, ptr, 6 ) == 0 );// flag = TRUE; else flag = FALSE;
                    individualAddr = flag;
                    ptr += 6;
                break;
            }
            //addrFlag = flag;
            if( cmdCnt ){
                if( cmdCount > 0xFF00 &&  cmdCnt < 0x100 ) cmdCount = cmdCnt;
                else                                       flag = flag && (cmdCnt > cmdCount);
            }
            if( cmdCnt > cmdCount ) cmdCount = cmdCnt;
            switch( cmd & 0xF0 ){
                case SET_MAC_CMD:
                  if( flag && ( ( cmd & 0x3 ) == MACADDR ) ) {
                      mymemcpy( page.myMac, ptr, 6 );
                      //savePB();
                  }
                  ptr += 6;
                break;
                case SETREG:
                    if( search_stat != page.searchCommunication ) page.searchCommunication = search_stat;
                    reg = *ptr++;
                    switch( cmd&0xC ){
                        case FLOAT_VAL:  if( flag ) setFloat( reg, (float *)ptr );       ptr+=4;  break;
                        case BYTE_VAL:   if( flag ) setByte ( reg, *ptr );               ptr++;   break;
                        case SHORT_VAL:  if( flag ) setInt  ( reg, *((UINT16*)ptr) );    ptr+=2;  break;
                    }
                break;
                case RESET: if( flag ){ tickWait( 12 * TICKS_IN_MS ); reset(); }else break; // ticks[ Reset ] = TICKS_IN_SEC; else break;
                /*
                case BOOT_OVER_AIR: if( addrFlag && !P1_4 ) {
                UINT16 addr;  
                    page.searchCommunication = FALSE;
                    reg = *ptr++; addr = *((UINT16*)ptr);  
                    if( ( addr > 0x3FF ) && ( 0x4000 & ( addr ^ page.imageAddr ) ) && ( addr < PAGEADDR ) ){
                    //static BYTE bootBuf[18];
                        ptr += 2;
                        //for( cmd = 0; cmd < reg; cmd++ ) bootBuf[ cmd ] = *ptr++;
                        //mymemcpy( bootBuf, ptr, reg );
                        if( reg & 1 ) reg++;
                        tickWait( 12 * TICKS_IN_MS );
                        initiateTransfer( ptr, (BYTE *)addr, reg );
                    }
                }return;
                */
                case SET_NEXT_KEY: case SET_NEXT_IV: 
                    if( individualAddr ){ 
                        loadKey( (char *)0x3E0 ); loadIV( (char *)0x3F0 ); 
                        decode( 16, (char *)ptr, ((cmd&0xF0)==SET_NEXT_IV) ? (char *)nextIV : (char *)nextKey ); 
                        loadKey( page.curKey );
                    }
                return;
                case ASVOL_NETCMD:
                    if( individualAddr ){
                        BYTE *saved_ptr = ptr;
                        asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
                        asm( "NOP" );
                        asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
                        asm( "NOP" );
                        asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
                        asm( "NOP" );
                        asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
                        asm( "NOP" );
                        asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
                        asm( "NOP" );
                        asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
                        asm( "NOP" );
                        asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
                        asm( "NOP" );
                        asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
                        asm( "NOP" );
                        asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
                        asm( "NOP" );
                        if( !isOnTestStand ){ isOnTestStand = 20; cmdCount--; CLKCON = 0x89; asm( "NOP" ); ptr += 7; break; }  
                      
                        cmd = *ptr++;
                        page.channel = page.starting500Channel = *ptr++;                // 7 bytes so far, 1 bit free
                        page.edAddr  =                           *ptr++;
                        page.gwAddr  =                           *ptr++;
                        page.myBunch =                           *ptr++;
                        page.groupId =                           *((UINT16 *) ptr);  ptr += 2;

                        page.netId   =     0xF & cmd;
                        page.use12kbod  = (0!=(0x10 & cmd ) )?1:0;
                        page.useFEC     = (0!=(0x20 & cmd ) )?1:0;
                        page.use250kbod = (0!=(0x40 & cmd ) )?1:0;
                        
                        mymemcpy( (BYTE *)(&page.installDate), (BYTE *)&utcLast, 4);
                        savePB();
                        ptr = saved_ptr+7;
                        setup_hopper( page.channel );
                        setupRadio(   page.channel ); //page.is500Always ? page.starting500Channel : page.channel );  // global interrupts are inabled inside the setupRadio
                    }else ptr+=7;
                break;
            }
            //if( cmdCnt > cmdCount ) cmdCount = cmdCnt;
        }
    }
}
inline void rc(void){
//    CLKCON |= OSC_BIT;                    // starting the RC Oscillator
//    asm( "NOP" );
//    SLEEP |= OSC_PD_BIT;                  // powering down the unused oscillator
}
/*==== END OF FILE ==========================================================*/
