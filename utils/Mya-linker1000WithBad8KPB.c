// reads device hex file, extracts scaling data
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <limits.h>
#include <errno.h>
#include <string.h>

	      
typedef signed   short  INT16;
typedef unsigned short  UINT16;
typedef unsigned int    UINT32;
typedef unsigned char   BYTE;
typedef unsigned char   BOOL;
typedef int             INT32;

#define AES_SIZE                    16
#define TRUE                        1
#define FALSE                       0

FILE * log = NULL; 

typedef enum
  {
    pb8K = 0,
    pbMain
  } ParameterBlockTypes;

struct{
    UINT16 barrier __attribute__((__packed__));
    UINT16 nDevs __attribute__((__packed__));
    BYTE curKey[ AES_SIZE ] __attribute__((__packed__));
    BYTE curIV [ AES_SIZE ] __attribute__((__packed__));
    BYTE channel __attribute__((__packed__));

    float dFk[4] __attribute__((__packed__));
    float tk[2]  __attribute__((__packed__));

    INT16  theDelta     __attribute__((__packed__));
    INT32  iRef         __attribute__((__packed__));
    BYTE   dF_Tolerance __attribute__((__packed__));
    BOOL   useADC       __attribute__((__packed__));//, use30kbod __attribute__((__packed__));
    BYTE   power        __attribute__((__packed__));
    BOOL   long_packets __attribute__((__packed__));
    BYTE   mac[ 6 ]     __attribute__((__packed__));
//    BOOL   isRxOnly     __attribute__((__packed__));
    BOOL   isHopper     __attribute__((__packed__));
    BYTE   edAddr       __attribute__((__packed__)), gwAddr __attribute__((__packed__));
//    BOOL   useFEC __attribute__((__packed__));
//    BYTE   version __attribute__((__packed__));
    BOOL   longReport __attribute__((__packed__));
    BYTE   maxDevs __attribute__((__packed__));
    BYTE   bootDelay __attribute__((__packed__));
    UINT32 secondsJoinEnabled __attribute__((__packed__));
    BOOL   printSupercycle __attribute__((__packed__));
//    BOOL   f_hopper __attribute__((__packed__));
    UINT16 sw __attribute__((__packed__));
    //BOOL   newProtocol;    
    BYTE   zCount __attribute__((__packed__));
    BOOL   modemSpeed __attribute__((__packed__));
    BOOL   transmitt __attribute__((__packed__));
} page_gw = {                                                                                                   // 68
    0xAAAA, 0,
    { 'A', 'N', 'D', 'S',  0,  'l', 'a', 'r', 'A', 'u', 'g', 'u', 's', 't',  0,   7 },
    { 'A', 'u', 'g', 'u', 's', 't',  0,   7,  'A', 'u', 'g', 'u', 's', 't',  0,   7 },

    0,  //should be 0 in production

    { -0.000627, 0.0469, 0.253, -123.0 },
    { 0.435, 625.0 },
    5,    121000L,  64,
    TRUE, //TRUE,
    0xED,
    TRUE,
    { 0,0,0,0,0,0 },
    //FALSE, 
    FALSE,
    2,     1,
    //FALSE, 0xE2, 
    FALSE,
    16,
    2,
    0, //14L*24L*3600L
    TRUE, //FALSE
    0xF0F0,
    4,
    TRUE,
    TRUE
};

typedef struct { 
  //                                                                                     size offset  total
  UINT16 barrier        __attribute__((__packed__)) __attribute__ ((aligned (1))); //      2      0     (2)
  BYTE   netId          __attribute__((__packed__)) __attribute__ ((aligned (1))), //      1      2     (3)
    myBunch             __attribute__((__packed__)) __attribute__ ((aligned (1))); //      1      3     (4)
  UINT16 groupId        __attribute__((__packed__)) __attribute__ ((aligned (1))); //      2      4     (6)
  // MAC number is exactly a SERIAL NUMBER
  // format:
  // 3bit weekday | 6 bit week | 7 bit year | 1 byte test center | 2 byte seq number | 1 byte random
  BYTE   myMac[6]       __attribute__((__packed__)) __attribute__ ((aligned (1))); //      6      6    (12)                                                                                 // 6
  // dF = -0.0005618976581444 t^3 + 0.0539971103485394 t^2 - 0.8658779608121610 t - 249.5883835010120000
  // t  = (adcs[5]-73000)) / 260
  float dFk[4]          __attribute__((__packed__)) __attribute__ ((aligned (1))); //     16     12    (28)
  float vrefPolynom[3]  __attribute__((__packed__)) __attribute__ ((aligned (1))); //     12     28    (40)
  // 0 Vout  1 Pin   2 Iout   3 Vin   4 Text   5 Iin2   6 Iout+ 
  float linearK [7][2]  __attribute__((__packed__)) __attribute__ ((aligned (1))); //     56     40    (96)
  char curKey[ AES_SIZE ] __attribute__((__packed__)) __attribute__ ((aligned (1))); //   16     96   (112)
  char curIV [ AES_SIZE ] __attribute__((__packed__)) __attribute__ ((aligned (1))); //   16    112   (128)

  INT16 fetFailCount      __attribute__((__packed__)) __attribute__ ((aligned (1))); //    2    128   (130)
  INT16 fetCurrentThreshold __attribute__((__packed__)) __attribute__ ((aligned (1))); //    2    130   (132)
  INT16 fetDeltaCurrent __attribute__((__packed__)) __attribute__ ((aligned (1))); //    2    132   (134)
  INT16 oscLowCurrentLimitInmA   __attribute__((__packed__)) __attribute__ ((aligned (1)));// 2 134   (136)   
  UINT16 oscDeltaV               __attribute__((__packed__)) __attribute__ ((aligned (1)));// 2 136   (138)   
  INT16 oscOVStartupDelay        __attribute__((__packed__)) __attribute__ ((aligned (1)));// 2 138   (140)   
  INT16 oscAfterStartupDelay     __attribute__((__packed__)) __attribute__ ((aligned (1)));//2  140   (142)  
  INT16 oscLowCurrentOnlyDelay __attribute__((__packed__)) __attribute__ ((aligned (1)));//  2  142   (144)
  BYTE tests[4]          __attribute__((__packed__)) __attribute__ ((aligned (1)));  //    4    144   (148)  
  UINT32 installDate      __attribute__((__packed__)) __attribute__ ((aligned (1))); //    4    148   (152)
  BYTE k_ov_volt          __attribute__((__packed__)) __attribute__ ((aligned (1))); //    1    152   (153)  
  BYTE k_oc_cur           __attribute__((__packed__)) __attribute__ ((aligned (1))); //    1    153   (154)  

  UINT16 syncword         __attribute__((__packed__)) __attribute__ ((aligned (1))); //    2    154   (156)
  BYTE vin_disable_radio  __attribute__((__packed__)) __attribute__ ((aligned (1))); //    1    156   (157)
  BYTE   rbunch           __attribute__((__packed__)) __attribute__ ((aligned (1))); //    1    157   (158) 
  BYTE   gbunch           __attribute__((__packed__)) __attribute__ ((aligned (1))); //    1    158   (159)
  BYTE shortCircuitProtection __attribute__((__packed__)) __attribute__ ((aligned (1)));// 1    159   (160) 

  BYTE reserved           __attribute__((__packed__)) __attribute__ ((aligned (1))); //    1    160   (161)
  BYTE synch_phase        __attribute__((__packed__)) __attribute__ ((aligned (1))); //    1    161   (162)  
  BYTE synch_freq         __attribute__((__packed__)) __attribute__ ((aligned (1))); //    1    162   (163)  
  BYTE bandwidth250       __attribute__((__packed__)) __attribute__ ((aligned (1))); //    1    163   (164)  
  BYTE channel            __attribute__((__packed__)) __attribute__ ((aligned (1))); //    1    164   (165) 

  //float VinTurnOn __attribute__((__packed__)), VinShutOff __attribute__((__packed__)),  VinDisableRadio __attribute__((__packed__)), 

  UINT32 utcProgram __attribute__((__packed__)) __attribute__ ((aligned (1))),       //    4    165   (169)
    utcLow          __attribute__((__packed__)) __attribute__ ((aligned (1))),       //    4    169   (173)
    utcHigh         __attribute__((__packed__)) __attribute__ ((aligned (1)));       //    4    173   (177)
  float tkCurrent   __attribute__((__packed__)) __attribute__ ((aligned (1)));       //    4    177   (181)
  float tkPower     __attribute__((__packed__)) __attribute__ ((aligned (1)));       //    4    181   (185)
  BYTE   mpp        __attribute__((__packed__)) __attribute__ ((aligned (1))),       //    1    185   (186) 
    module          __attribute__((__packed__)) __attribute__ ((aligned (1)));       //    1    186   (187)
  BYTE   ov         __attribute__((__packed__)) __attribute__ ((aligned (1))),       //    1    187   (188)
    oc              __attribute__((__packed__)) __attribute__ ((aligned (1)));       //    1    188   (189)
  BYTE  radioPower  __attribute__((__packed__)) __attribute__ ((aligned (1))),       //    1    189   (190)
    edAddr          __attribute__((__packed__)) __attribute__ ((aligned (1))),       //    1    190   (191)  
    gwAddr          __attribute__((__packed__)) __attribute__ ((aligned (1))),       //    1    191   (192)
    repeaterChannel __attribute__((__packed__)) __attribute__ ((aligned (1))),       //    1    192   (193)  
    repeaterPower   __attribute__((__packed__)) __attribute__ ((aligned (1)));       //    1    193   (194)
  BYTE flags        __attribute__((__packed__)) __attribute__ ((aligned (1)));       //    1    194   (195)
  BYTE showState    __attribute__((__packed__)) __attribute__ ((aligned (1)));       //    1    195   (196)
  signed char CriticalLevel500  __attribute__((__packed__)) __attribute__ ((aligned (1))),//1   196   (197) 
    CriticalLevel10  __attribute__((__packed__)) __attribute__ ((aligned (1)));      //    1    197   (198)
  INT16   pureOffset __attribute__((__packed__)) __attribute__ ((aligned (1)));      //    2    198   (200)
  UINT16 defCyclesTo500 __attribute__((__packed__)) __attribute__ ((aligned (1))),   //    2    200   (202)
    defCyclesTo10             __attribute__((__packed__)) __attribute__ ((aligned (1))),// 2    202   (204) 
    defCyclesToStep500Channel __attribute__((__packed__)) __attribute__ ((aligned (1))),// 2    204   (206)
    defCyclesToStep10Channel  __attribute__((__packed__)) __attribute__ ((aligned (1))),// 2    206   (208)
    defCycles2SoftKick        __attribute__((__packed__)) __attribute__ ((aligned (1))),// 2    208   (210)
    defCycles2HardKick        __attribute__((__packed__)) __attribute__ ((aligned (1)));// 2    210   (212)
  UINT16 imageAddr    __attribute__((__packed__)) __attribute__ ((aligned (1)));     //    2    212   (214)
  UINT16 versionLow   __attribute__((__packed__)) __attribute__ ((aligned (1))),     //    2    214   (216)
    versionHigh       __attribute__((__packed__)) __attribute__ ((aligned (1)));     //    2    216   (218)
  BYTE   starting500Channel  __attribute__((__packed__)) __attribute__ ((aligned (1)));//  1    218   (219)
  BYTE   flags2              __attribute__((__packed__)) __attribute__ ((aligned (1)));//  1    219   (220)
  UINT16 prepAddr            __attribute__((__packed__)) __attribute__ ((aligned (1)));//  2    220   (222)
  BYTE   rep_step            __attribute__((__packed__)) __attribute__ ((aligned (1)));//  1    222   (223)
  BYTE   ov_startup          __attribute__((__packed__)) __attribute__ ((aligned (1)));//  1    223   (224)
  BYTE   t_superhot          __attribute__((__packed__)) __attribute__ ((aligned (1)));//  1    224   (225)
  BYTE   t_hot               __attribute__((__packed__)) __attribute__ ((aligned (1)));//  1    225   (226)
  BYTE   t_cold              __attribute__((__packed__)) __attribute__ ((aligned (1)));//  1    226   (227)
  BYTE   fallback_time       __attribute__((__packed__)) __attribute__ ((aligned (1)));//  1    227   (228)
  BYTE   network_cnst_1      __attribute__((__packed__)) __attribute__ ((aligned (1)));//  1    228   (229)
  BYTE   network_cnst_2      __attribute__((__packed__)) __attribute__ ((aligned (1)));//  1    229   (230)
  BYTE   oc_protection       __attribute__((__packed__)) __attribute__ ((aligned (1)));//  1    230   (231)
  BYTE   oc_step             __attribute__((__packed__)) __attribute__ ((aligned (1)));//  1    231   (232)
  INT16  cur_noise           __attribute__((__packed__)) __attribute__ ((aligned (1)));//  2    232   (234)
  BYTE   ov_step             __attribute__((__packed__)) __attribute__ ((aligned (1)));//  1    234   (235)
  BYTE   shunt               __attribute__((__packed__)) __attribute__ ((aligned (1)));//  1    235   (236)
  BYTE   max_off             __attribute__((__packed__)) __attribute__ ((aligned (1)));//  1    236   (237)
  BYTE   vin_limit           __attribute__((__packed__)) __attribute__ ((aligned (1)));//  1    237   (238)
  BYTE   vin_turn_on         __attribute__((__packed__)) __attribute__ ((aligned (1)));//  1    238   (239)
  BYTE   vin_switch_off      __attribute__((__packed__)) __attribute__ ((aligned (1)));//  1    239   (240)
  UINT16 post_barrier        __attribute__((__packed__)) __attribute__ ((aligned (1)));//  1    240   (242)
} ParameterBlock;

ParameterBlock page8K __attribute__((__packed__)) __attribute__ ((aligned (1))) = {// 8K Parameter Block
  0xAAAA,    // barrier
  (BYTE)0,   // netId 
  (BYTE)0,   // myBunch 
  0xFFFF,    //  permanent !!! Normal  groupId
  { 0, 0, 0, 0, 0, 0 }, // myMac
  {
    //-0.000784329277, 0.069999928482, -1.557036645365, -229.914041475601 + 32.0    dfK3
    // -0.000693, 0.0556, 0.613, -280 //+ 300.0
    -0.000693, 0.0556, 0.613, -245 //+ 300.0
  }, // temperature compensation
  {  2496.0, 0.0000004, 73.0 }, // vref polynomials  vrefPolynom
  // linearK coefficients and offsets
  //  Vout  0         Vin1  1         Iout   2         Vin2  3         Text 4          Iin1  5         Iin2  6
  // { {27.664, -3575}, {25.083, 0.0}, {7.767, -573.0}, {24.767, 0.0}, {0.435, 625.0}, {7.857, -592 }, {8.037, -545} },
   { {27.64, -3500}, {25.06, 0.0}, {7.75, -570.0}, {24.97, -235.0}, {0.435, 625.0}, {7.9, -532 }, {7.78, -586} },
  { 'A', 'N', 'D', 'S',  0,  'l', 'a', 'r', 'A', 'u', 'g', 'u', 's', 't',  0,   7 },   // curKey
  { 'A', 'u', 'g', 'u', 's', 't',  0,   7,  'A', 'u', 'g', 'u', 's', 't',  0,   7 },   // curIV 
  0, // fetFailCount (2)
  0, // fetCurrentThreshold (2)
  0, // fetDeltaCurrent (2)
  0,    // mA, oscLowCurrentLimitInmA 
  30000, // raw ADC counts, oscDeltaV
  240,   // 120 ms oscOVStartupDelay
  1200,  // 120*5 ms oscAfterStartupDelay
  32760, // 16 sec - maximum delay oscLowCurrentOnlyDelay
  { 0, 0, 0, 0 },  //tests 4
  0xFFFFFFFF,   //install date
  0xFF, 0xFF,   // k_ov_volt, k_oc_cur  
  0xF0F0, // syncword
  0, // vin_disable_radio
  0, // rbunch
  0, // gbunch 
  170,  // shortCircuitProtection
  0xFF, // reserved        
  120,  // sync_phase 
  128,  // synch_freq
  0x4D, // bandwidth250
  (BYTE)0, //  should be changed to 0 in the production version   channel
  0, // utcProgram 
  0, // utcLow 
  0, // utcHigh
  0.00002, // tkCurrent
  0.00005, // tkPower
  FALSE,  // mpp
  FALSE,  // module
  (BYTE)0, // ov 
  (BYTE)0, // oc
  0xFF, 2, 1, 0, // radioPower, edAddr, gwAddr, repeaterChannel
  0xFF,  // repeaterPower
  0x97,  // flags
  1,     // showState
  -92,   // CriticalLevel500
  -102,  // CriticalLevel10
  -200,  // pureOffset
  40,    // defCyclesTo500
  30,    // defCyclesTo10
  3,     // defCyclesToStep500Channel
  4,     // defCyclesToStep10Channel
  8,     // defCycles2SoftKick
  80,    // defCycles2HardKick
  0x83F, // imageAddr
  0x01,  // versionLow
  0x01,  // versionHigh
  0xFF,  // starting500Channel
  (BYTE)0x89, // flags2
  0xFFFF, // prepAddr
    // rep_step, ov_startup, t_superhot, t_hot, t_cold, fallback_time, network cnst1, network_cnst_2,  oc_protection
        25,          70,         110,    100,     90,     10,              7,             11,           60,
    //oc_step,   cur_noise (dis),  ov_step, shunt, max_off,
       10,          -1000,               10,      20,      1,
    // vin_limit, vin_turn_on, vin_switch_off
         0,            0,           0,
    0x5567 // post_barrier
};

ParameterBlock page __attribute__((__packed__)) __attribute__ ((aligned (1))) = {// Main image PB 
  0xAAAA,    // barrier
  (BYTE)0,   // netId 
  (BYTE)0,   // myBunch 
  0xFFFF,    //  permanent !!! Normal  groupId
  { 0, 0, 0, 0, 0, 0 }, // myMac
  {
    //-0.000784329277, 0.069999928482, -1.557036645365, -229.914041475601 + 32.0    dfK3
    // -0.000693, 0.0556, 0.613, -280 //+ 300.0
    -0.000693, 0.0556, 0.613, -245 //+ 300.0
  }, // temperature compensation
  {  2496.0, 0.0000004, 73.0 }, // vref polynomials  vrefPolynom
  // linearK offsets and coefficients
  //  600 MS was Vout  0         Pin  1         Iout   2         Vin  3         Text 4          Iin2  5         Iout+ 6
  //  Vout  0         Vin1  1         Iout   2         Vin2  3         Text 4          Iin1  5         Iin2  6
  // { {27.664, -3575}, {25.083, 0.0}, {7.767, -573.0}, {24.767, 0.0}, {0.435, 625.0}, {7.857, -592 }, {8.037, -545} },
   { {27.64, -3500}, {25.06, 0.0}, {7.75, -570.0}, {24.97, -235.0}, {0.435, 625.0}, {7.9, -532 }, {7.78, -586} },
  { 'A', 'N', 'D', 'S',  0,  'l', 'a', 'r', 'A', 'u', 'g', 'u', 's', 't',  0,   7 },   // curKey
  { 'A', 'u', 'g', 'u', 's', 't',  0,   7,  'A', 'u', 'g', 'u', 's', 't',  0,   7 },   // curIV 
  20000, // fetFailCount (2) (in ticks)
  700, // fetCurrentThreshold (2) (in mA)
  80, // fetDeltaCurrent (2) (in mA)
  0,    // mA, oscLowCurrentLimitInmA 
  30000, // raw ADC counts, oscDeltaV
  240,   // 120 ms oscOVStartupDelay
  1200,  // 120*5 ms oscAfterStartupDelay
  32760, // 16 sec - maximum delay oscLowCurrentOnlyDelay
  { 0, 0, 0, 0 },  //tests 4
  0xFFFFFFFF,   //install date
  0xFF, 0xFF,   // k_ov_volt, k_oc_cur  
  0xF0F0, // syncword
  0, // vin_disable_radio
  0, // rbunch
  0, // gbunch 
  170,  // shortCircuitProtection
  0xFF, // reserved        
  120,  // sync_phase 
  128,  // synch_freq
  0x4D, // bandwidth250
  (BYTE)0, //  should be changed to 0 in the production version   channel
  0, // utcProgram 
  0, // utcLow 
  0, // utcHigh
  0.00002, // tkCurrent
  0.00005, // tkPower
  FALSE,  // mpp
  FALSE,  // module
  (BYTE)100, // ov 
  (BYTE)190, // oc
  0xFF, 2, 1, 0, // radioPower, edAddr, gwAddr, repeaterChannel
  0xFF,  // repeaterPower
  0x97,  // flags
  1,     // showState
  -92,   // CriticalLevel500
  -102,  // CriticalLevel10
  -200,  // pureOffset
  40,    // defCyclesTo500
  30,    // defCyclesTo10
  3,     // defCyclesToStep500Channel
  4,     // defCyclesToStep10Channel
  8,     // defCycles2SoftKick
  80,    // defCycles2HardKick
  0x83F, // imageAddr
  0x01,  // versionLow
  0x01,  // versionHigh
  0xFF,  // starting500Channel
  (BYTE)0x89, // flags2
  0xFFFF, // prepAddr
    // rep_step, ov_startup, t_superhot, t_hot, t_cold, fallback_time, network cnst1, network_cnst_2,  oc_protection
        25,          70,         110,    100,     90,     10,              7,             11,           60,
    //oc_step,   cur_noise (dis),  ov_step, shunt, max_off,
       10,          -1000,               10,      20,      1,
    // vin_limit, vin_turn_on, vin_switch_off
         0,            0,           0,
    0x5555 // post_barrier
};

#define MAXSIZE 512

int dummy = 0;

// float dfk[] = {-0.000693, 0.0556, 0.613, -270};
float dfk[] = {-0.000693, 0.0556, 0.613, -245}; // new dFk[3] value

int original_version = 0x33;

char txt_dir[MAXSIZE];
char hex_dir[MAXSIZE];

#define CRC16_POLY 0x8005 

// taken form http://www.ti.com/lit/an/swra111d/swra111d.pdf
UINT16 crc16_update(UINT16 crcReg, BYTE crcData) 
{ 
  BYTE j; 
  for (j = 0; j < 8; j++) 
    { 
      if (((crcReg & 0x8000) >> 8) ^ (crcData & 0x80)) 
	crcReg = (crcReg << 1) ^ CRC16_POLY; 
      else   
	crcReg = (crcReg << 1); 
      crcData <<= 1; 
    } 
    return crcReg; 
}// crc16_update

char *mac2serial( BYTE * mac)
{
  static char serial[ 32 ];
  unsigned int w, y, n, temp = 0; 
  char letter;

  n    = mac[5] | (mac[4]<<8) | (mac[3]<<16);
  temp = mac[2] | (mac[1]<<8) | (mac[0]<<16);

  letter = ((temp>>6) & 0x1F) + 'A';
  y      = (temp>>11) & 0x7F;
  w      = (temp>>18) & 0x3F;

  sprintf( serial, "%02d%02d%c%06d", w, y, letter, n );

  return serial;
}

void serial2mac( char *serial, BYTE *mac )
{
  unsigned int w, y, n, temp; 
  unsigned char letter;

  sscanf( serial, "%02d%02d%c%06d", &w, &y, &letter, &n );
  letter = (letter >= 'a') ? (letter-'a') : (letter-'A');
  mac[5] = n & 0xFF; mac[4] = (n>>8) & 0xFF; mac[3] = (n>>16)&0xFF;
  n = ( ( w & 0x3F ) << 18 ) | ( ( y & 0x7F ) << 11 ) | ( ( letter & 0x1F ) << 6 );
  mac[2] = n & 0xFF; mac[1] = (n>>8) & 0xFF; mac[0] = (n>>16)&0xFF;
}

BYTE ch2int(   unsigned char ch ) 
{
  if(      ch >= '0' && ch <= '9' ) 
    ch-= '0';
  else if( ch >= 'A' && ch <= 'F' ) 
    ch = 10 + (ch - 'A');
  else if( ch >= 'a' && ch <= 'f' ) 
    ch = 10 + (ch - 'a');
  else
    ch = 0;
  return   ch & 0xF;
}

BOOL writeParameterBlockToHexFile( FILE *f, UINT16 addr );

BYTE readOneByte( char *ptr )
{ 
  return ( (ch2int(ptr[0])<<4) | ch2int(ptr[1]) ) & 0xFF; 
}

char *code( BYTE *ptr, BYTE l, UINT16 addr )
{
  UINT32 checksum = l;
  static char buffer[ 64 ], buf[ 16 ];

  checksum += (addr>>8) & 0xFF;
  checksum += addr & 0xFF;
  sprintf( buffer, ":%02X%04X00", l, addr );

  while( l-- )
    { 
      checksum += *ptr; 
      sprintf( buf, "%02X", *ptr++ ); 
      strcat( buffer, buf ); 
    }

  checksum = 0x100 - ( checksum & 0xFF );
  sprintf( buf, "%02X\r\n", checksum & 0xFF ); strcat( buffer, buf );

  return buffer;
}

char secret[ 3 ][ 64 ];

BOOL link(char *serial, char *bootSec, char *image1, char *image2, unsigned short startAddr)
{ 
  char buf[ 64 ], fname[ MAXSIZE ];
  char *ret = NULL;
  char *images[ 2 ] = { image1, image2 };
  int i, j;
  UINT16 addr; UINT16 crc;
  FILE *f1, *f;  // f1 is used as a reader handle for input images, f is handle for output hex file.
  BYTE *ptr;
  BOOL retValue = TRUE;

  if( serial == NULL ) 
    serial = mac2serial( page.myMac ); 
  else 
    {
      serial2mac( serial, page.myMac );    // have MAC in both PBs
      serial2mac( serial, page8K.myMac );
    }


  strncpy( fname, hex_dir, MAXSIZE - 1);

  printf("%s:%d INFO: link function, hex_dir : %s\n",
	     __FILE__, __LINE__, fname);
  fprintf(log,"%s:%d INFO: link function, hex_dir : %s\n",
	     __FILE__, __LINE__, fname);
  strcat( fname, serial );
  printf("%s:%d INFO: link function, hex_dir with sn : %s\n",
	     __FILE__, __LINE__, fname);
  fprintf(log, "%s:%d INFO: link function, hex_dir with sn : %s\n",
	     __FILE__, __LINE__, fname);
  strcat (fname, ".hex");
  printf("%s:%d INFO: link function, hex_dir and full out file name : %s\n",
	     __FILE__, __LINE__, fname);
  fprintf(log, "%s:%d INFO: link function, hex_dir and full out file name : %s\n",
	     __FILE__, __LINE__, fname);

  errno = 0;
  f = fopen( fname, "w" );  

  if( f == NULL ) 
    {
      printf("%s:%d ERROR: link function failed to open output hex file %s for writing\n",
	     __FILE__, __LINE__, fname);
      printf("%s:%d ERROR: errno:%d strerror:%s\n",
	     __FILE__, __LINE__,errno, strerror(errno));
      fprintf(log, "%s:%d ERROR: link function failed to open output hex file %s for writing\n",
	     __FILE__, __LINE__, fname);
      fprintf(log, "%s:%d ERROR: errno:%d strerror:%s\n",
	     __FILE__, __LINE__,errno, strerror(errno));
      retValue = FALSE;
    }
  else
    {
      printf("%s:%d Ready to write to hex file %s\n",
	     __FILE__,__LINE__,fname);
      printf("%s:%d Ready to read boot image file %s\n",
	     __FILE__,__LINE__,bootSec);
      fprintf(log, "%s:%d Ready to write to hex file %s\n",
	     __FILE__,__LINE__,fname);
      fprintf(log, "%s:%d Ready to read boot image file %s\n",
	     __FILE__,__LINE__,bootSec);

      for( f1 = fopen( bootSec, "r+" ); f1 && !feof( f1 ); ret = fgets( buf, 64, f1 ) )
	{
	  if( buf[8] == '5' )
	    {
	      printf("\n\n *** %s:%d INFO:CHECK exercising buf[8] == '5' code for boot image *** \n\n",
		     __FILE__, __LINE__);
	      fprintf(log, "\n\n *** %s:%d INFO:CHECK exercising buf[8] == '5' code for boot image *** \n\n",
		     __FILE__, __LINE__);
	      BYTE  tmp[ 16 ];
	      addr = 0x3E0;
	      for( j = 0; j < 2; j++, addr += 0x10 )
		{
		  for( i = 0; i < 16; i++ )  
		    tmp[ i ] = rand() & 0xFF;
		  fputs( code( tmp, 0x10, addr ), f );
		  memset( secret[ j ], 0, 64 );
		  strcpy( secret[ j ], code( tmp, 0x10, addr )+9 );
		}
	      memset( tmp,   0, 16 );
	      memcpy( tmp+6, page.myMac, 6 );
	      *((UINT16 *)tmp) = 0xAAAA;
	      tmp[12] = page.ov;          tmp[13] = page.oc;
	      tmp[14] = page.ov_startup;  tmp[15] = page.ov_step;
	      fputs( code( tmp, 16, 0x3D0 ), f );
	      //fputs( code( (BYTE *)dfk, 16, 0x3C0 ), f );
	    }
	  if( ret && buf[7] == '0') 
	    fputs( buf, f );
	}

      if( f1 )
	{ 
	  fclose( f1 );
	  for( i = 0; i < 2 && retValue == TRUE; i++ )
	    {
	      printf("%s:%d Ready to read image file %s\n",
		     __FILE__,__LINE__,images[i]);
	      fprintf(log, "%s:%d Ready to read image file %s\n",
		     __FILE__,__LINE__,images[i]);
	      for( f1 = fopen( images[ i ], "r+" ); f1 && !feof( f1 ); ret = fgets( buf, 64, f1 ) )
		{
		  if( ret && buf[7] == '0' 
		      && strncmp( buf, ":03000000", 9 ) ) /*&&  strncmp( buf, ":00000001FF", 12 ) */
		    { 
		      fputs( buf, f );
		    }
		}
	      if( f1 ) 
		{
		  fclose( f1 );
		  printf("%s:%d Success reading image file %s\n",
			 __FILE__,__LINE__,images[i]);
		  fprintf(log, "%s:%d Success reading image file %s\n",
			 __FILE__,__LINE__,images[i]);
		}
	      else
		{
		  printf("%s:%d ERROR failed to open/read image file %s\n",
			 __FILE__,__LINE__,images[i]);
		  fprintf(log, "%s:%d ERROR failed to open/read image file %s\n",
			 __FILE__,__LINE__,images[i]);
		  retValue = FALSE;
		}
	    }
	  
	  /* page.crc = 0xFFFF; 
	     i = 240; 
	     ptr = (BYTE *)&page; 
	     while( i-- ) 
	       page.crc = crc16_update( page.crc, *ptr++ );
	     //printf( "CRC is 0x%04X\n", ((int)page.crc) );
	     */

	  if (retValue == TRUE)
	    {
	      if (writeParameterBlockToHexFile( f, 0x400 ) == FALSE)  // PB for main image
		{
		  printf("%s:%d ERROR: writeParameterBlockToHexFile failed for 8K/Boot image.\n",
			 __FILE__,__LINE__);
		  fprintf(log, "%s:%d ERROR: writeParameterBlockToHexFile failed for 8K/Boot image.\n",
			 __FILE__,__LINE__);
		  retValue = FALSE;
		}
	      else if (writeParameterBlockToHexFile( f, 0x5C00 ) == FALSE) // PB for 8K/Boot image
		{
		  printf("%s:%d ERROR: writeParameterBlockToHexFile failed for main image.\n",
			 __FILE__,__LINE__);
		  fprintf(log, "%s:%d ERROR: writeParameterBlockToHexFile failed for main image.\n",
			 __FILE__,__LINE__);
		  retValue = FALSE;
		}
	    }
	}
      else
	{
	  printf("%s:%d ERROR failed to open/read boot image file %s\n",
		 __FILE__,__LINE__,bootSec);
	  fprintf(log, "%s:%d ERROR failed to open/read boot image file %s\n",
		 __FILE__,__LINE__,bootSec);
	  retValue = FALSE;
	}
      fclose( f );
    }
  return retValue;
}

BOOL linkgw( char *serial, char *bootSec, char *image ){
char buf[ 64 ], fname[ 256 ];
char *ret = NULL;
int i, j;
FILE *f1, *f;
UINT16 addr = 0x7C00, l;
BYTE *pagePtr = (BYTE *)&page_gw;
unsigned int  byteCount = sizeof( page_gw );

    if( serial == NULL ) serial = mac2serial( page_gw.mac ); else serial2mac( serial, page_gw.mac );

    strcpy( fname, hex_dir ); strcat( fname, serial );
    //strcpy( fname, serial );
    f = fopen( strcat( fname, "_gw.hex" ), "w" );  if( f == NULL ) return FALSE;
    for( f1 = fopen( bootSec, "r+" ); f1 && !feof( f1 ); ret = fgets( buf, 64, f1 ) )
        if( ret && buf[7] == '0' &&  strncmp( buf, ":00000001FF", 12 )  ) fputs( buf, f );
    if( f1 ) fclose( f1 );
    else { printf( "cannot open boot sector file %s!\n", bootSec ); fclose( f ); return FALSE; }
    for( f1 = fopen( image, "r+" ); f1 && !feof( f1 ); ret = fgets( buf, 64, f1 ) )
        if( ret && buf[7] == '0' && strncmp( buf, ":03000000", 9 ) /*&&  strncmp( buf, ":00000001FF", 12 )*/  ) fputs( buf, f );
    if( f1 ) fclose( f1 );
    else { printf( "cannot open image file %s!\n", image ); fclose( f ); return FALSE; }

    if( *pagePtr != 0xAA ){ puts( "page does not have barrier!" ); fclose( f ); return FALSE; }
    printf( "size of gw page is %d\n", byteCount );
    while( byteCount ){// writing the hex file
        i = min( 0x10, byteCount );
        fputs( code( pagePtr, i, addr ), f );
        pagePtr += i; addr += i; byteCount -= i;
    }

    fclose( f );
    return TRUE;
}

int min(int i, int j)
{ 
  return i < j ? i : j; 
}

// writeParameterBlockToHexFile uses/validates the value of addr to determine which PB to write
BOOL writeParameterBlockToHexFile(FILE *f, UINT16 addr)
{
  // UINT16 l;
  UINT16 eightKPBStartAddress = 0x400;
  UINT16 mainPBStartAddress = 0x5C00;

  BYTE *pagePtr = NULL;
  if (addr == mainPBStartAddress) // Main PB start address
    {
      printf("%s:%d writeParameterBlockToHexFile: writing Main Parameter Block starting at address %04X\n",
	     __FILE__, __LINE__, addr);
      fprintf(log, "%s:%d writeParameterBlockToHexFile: writing Main Parameter Block starting at address %04X\n",
	     __FILE__, __LINE__, addr);
      pagePtr = (BYTE *)&page;
    }
  else if (addr == eightKPBStartAddress) // 8K, boot image PB start address
    {
      printf("%s:%d writeParameterBlockToHexFile: writing 8K/Boot Parameter Block starting at address %04X\n",
	     __FILE__, __LINE__, addr);
      fprintf(log, "%s:%d writeParameterBlockToHexFile: writing 8K/Boot Parameter Block starting at address %04X\n",
	     __FILE__, __LINE__, addr);
      pagePtr = (BYTE *)&page8K;
    }
  else
    {
      printf("%s:%d ERROR: Unknown ParameterBlock starting address passed to writeParameterBlockToHexFile %04X\n",
	     __FILE__, __LINE__, addr);
      printf("%s:%d ERROR: Expects either address %04X for main PB or address %04X for 8K/Boot PB\n",
	     __FILE__, __LINE__, mainPBStartAddress,eightKPBStartAddress);
      fprintf(log, "%s:%d ERROR: Unknown ParameterBlock starting address passed to writeParameterBlockToHexFile %04X\n",
	     __FILE__, __LINE__, addr);
      fprintf(log, "%s:%d ERROR: Expects either address %04X for main PB or address %04X for 8K/Boot PB\n",
	     __FILE__, __LINE__, mainPBStartAddress,eightKPBStartAddress);
      return FALSE;
    }

  size_t pageSize = sizeof(page);
  size_t page8KSize = sizeof(page8K);

  if ( (pageSize) != (page8KSize) )
    {
      printf("%s:%d ERROR: writeParameterBlockToHexFile expects Main PB size %zu and 8K/Boot PB size %zu\n",
	     __FILE__, __LINE__, pageSize, page8KSize);
      printf("to be the same size.\n");
      fprintf(log, "%s:%d ERROR: writeParameterBlockToHexFile expects Main PB size %zu and 8K/Boot PB size %zu\n",
	     __FILE__, __LINE__, pageSize, page8KSize);
      fprintf(log, "to be the same size.\n");
      return FALSE;
    }

  BYTE tmp_page[ pageSize ];  // Both PBs are the same size so tmp_page allocated with sizeof(page)
  unsigned int  byteCount = pageSize; // both PBs the same size, byteCount uses sizeof(page)

  memset( tmp_page, 0, pageSize ); // Can use pageSize for both PBs
  memcpy( tmp_page, pagePtr, pageSize );
  pagePtr = tmp_page;

  if ( *pagePtr != 0xAA && *pagePtr != 0x55 )
    {
      printf("%s:%d ERROR: writeParameterBlockToHexFile expects a barrier of 0xAA or 0x55, found %0x\n",
	     __FILE__, __LINE__, (int)(*pagePtr));
      fprintf(log, "%s:%d ERROR: writeParameterBlockToHexFile expects a barrier of 0xAA or 0x55, found %0x\n",
	     __FILE__, __LINE__, (int)(*pagePtr));
      return FALSE;
    }

  printf( "%s:%d size of ParameterBlock is %d\n", 
	  __FILE__,__LINE__, byteCount );
  fprintf( log, "%s:%d size of ParameterBlock is %d\n", 
	  __FILE__,__LINE__, byteCount );

  unsigned int i = 0;

  while( byteCount )
    {
      i = min( 0x10, byteCount );
      fputs( code( pagePtr, i, addr ), f );
      pagePtr += i; addr += i; byteCount -= i;
    }

  printf("%s:%d writeParameterBlockToHexFile finished as expected ...\n",
	 __FILE__, __LINE__);
  fprintf(log, "%s:%d writeParameterBlockToHexFile finished as expected ...\n",
	 __FILE__, __LINE__);

  return TRUE;
}

BYTE xtob( char *ptr )    
{ 
  unsigned int i; 
  sscanf(ptr, "%02X", &i ); 
  return (BYTE)( i & 0xFF ); 
}

typedef unsigned int pointer;

char *mystrcat( char *fname, char *ext ) 
{ 
  static char buffer[ 256 ];  
  strcpy( buffer, fname );  
  strcat( buffer, ext ); 
}


BOOL copy( char *destFname, char *srcFname )
{
  BOOL retValue = TRUE; // TRUE indicates success
  char buffer[ MAXSIZE ];
  FILE *dest = fopen( destFname, "w" );
  FILE *src  = fopen( srcFname, "r" );
  
  if( ( src == NULL ) || ( dest == NULL ) )
    {
      printf("%s:%d ERROR: destination file:%s or source file:%s failed fopen call.\n",
	     __FILE__, __LINE__, destFname,srcFname);
      fprintf(log, "%s:%d ERROR: destination file:%s or source file:%s failed fopen call.\n",
	     __FILE__, __LINE__, destFname,srcFname);
      retValue = FALSE;
    }
  else
    {
      while( !feof( src ) )
	{
	  int l = fread(   buffer, 1, 512, src  );
	  if( l ) fwrite(  buffer, 1, l,   dest );
	}
      
      fclose( src ); 
      fclose( dest );
    }
  
  return retValue;
}

char bootSect[MAXSIZE],
  imageLow[MAXSIZE],
  imageHigh[MAXSIZE];

char /* *bootSect = "../ED/hex/fc-boot.hex", 
  *imageLow = "../ED/hex/and-stack-ed-g-1k.hex",  
  *imageHigh = "../ED/hex/and-stack-ed-g-15k.hex", */
  *d8 = "p:/Firmware/TI_RevD8/and-stack-ed-v8.hex",
  *gw_boot = "../GW_BOU/hex/boot_gw.hex", 
  *gw_boot_win = "../GW_BOU/hex/boot_gw-win.hex", 
  *gw_image = "../GW_BOU/hex/gw.hex";

char comment[ MAXSIZE ];

BOOL logParameterBlockInformation( char *filename, ParameterBlock page)
{
  BOOL retValue = TRUE;

  FILE * f = fopen( filename, "w" );
  if (f == NULL)
    {
      retValue = FALSE;
      printf("%s:%d ERROR failed to open log file for Parameter Block\n",
	     __FILE__, __LINE__);
      fprintf(log, "%s:%d ERROR failed to open log file for Parameter Block\n",
	     __FILE__, __LINE__);
    }
  else
    {
      time_t rawtime;
      struct tm * timeinfo;

      time ( &rawtime );
      timeinfo = localtime ( &rawtime );
      secret[2][12] = 0;
      fprintf( f, "MAC %s Converted on %s to version %X from version %X\r\n", 
	       secret[ 2 ], asctime( timeinfo ), 0, original_version );
      fprintf( f, "Key is %s\r\n", secret[ 0 ] );
      fprintf( f, "IV  is %s\r\n", secret[ 1 ] );
      fprintf( f, "%s\r\n", comment );
      fprintf( f, "boot %s low %s high %s\r\n", bootSect, imageLow, imageHigh );
      
      fprintf( f, "serial number:%s\r\n", mac2serial( page.myMac ) );
      fprintf( f, "Bunch %x id %x stringId %x, MAC %02x%02x%02x%02x%02x%02x\r\n",
	       page.myBunch, page.netId, page.groupId, 
	       page.myMac[0], page.myMac[1], page.myMac[2], page.myMac[3], page.myMac[4], page.myMac[5] );
      fprintf( f, "dFk %g %g %g %g\r\n", page.dFk[0], page.dFk[1], page.dFk[2], page.dFk[3] );
      fprintf( f, "vrefPolynom %g %g %g\r\n", page.vrefPolynom[0], page.vrefPolynom[1], page.vrefPolynom[2] );
      fprintf( f, "Vout %g %g \r\nVin1 %g %g\r\nIout %g %g\r\nVin2 %g %g\r\nText %g %g\r\nIin2 %g %g\r\nIin1 %g %g\r\n",
	       page.linearK[0][0], page.linearK[0][1],
	       page.linearK[1][0], page.linearK[1][1],
	       page.linearK[2][0], page.linearK[2][1],
	       page.linearK[3][0], page.linearK[3][1],
	       page.linearK[4][0], page.linearK[4][1],
	       page.linearK[5][0], page.linearK[5][1],
	       page.linearK[6][0], page.linearK[6][1] );
      fprintf( f, "VinTurnOn %g \r\nVinSwitchOff %g\r\nVinDisableRadio %g\r\nVinLimit %g\r\ntkCurrent %g\r\ntkPower %g\r\n",
	       page.vin_turn_on*100.0, 
	       page.vin_switch_off*100.0,
	       page.vin_disable_radio*1000.0,
	       page.vin_limit*100.0,
	       page.tkCurrent,
	       page.tkPower);
      fprintf( f, "fetFailCount %d\r\n",page.fetFailCount);
      fprintf( f, "fetCurrentThreshold %d\r\n",page.fetCurrentThreshold);
      fprintf( f, "fetDeltaCurrent %d\r\n",page.fetDeltaCurrent);
      fprintf( f, "Oscillation Params: oscLowCurrentLimit %d oscDeltaV %d oscOVStartupDelay %d\r\noscAfterStartupDelay %d oscLowCurrentOnlyDelay %d\r\n",
	       page.oscLowCurrentLimitInmA,  
	       page.oscDeltaV, 
	       page.oscOVStartupDelay, 
	       page.oscAfterStartupDelay, 
	       page.oscLowCurrentOnlyDelay);

      fprintf( f, "defaults: OV %d OV_Startup %d OV_Step %d OC %d MPP %s Module %s channel %d\r\n",
	       page.ov, page.ov_startup, page.ov_step,
	       page.oc, 
	       page.mpp?"ON":"OFF", page.module?"ON":"OFF", page.channel);
      fprintf( f, "Temp Protection: TSuperHot %d THot %d TCold %d OCStep %d FallbackTime %d OC Protection %d\r\n",
	       page.t_superhot, page.t_hot, page.t_cold, 
	       page.oc_step, page.fallback_time, page.oc_protection   );
      fprintf( f, "Current Noise: %d OV Volt K %d OC Cur K %d \r\n",
	       page.cur_noise, page.k_ov_volt, page.k_oc_cur   );
      
      fprintf( f, "Repeater %s Slave %s Searching %s 12kbod %s FEC %s Hopping %s 250_kbod %s\r\n",
	       page.flags&0x20 ? "YES" : "NO", page.flags&0x40 ? "YES" : "NO", page.flags&0x80 ? "YES" : "NO",
	       page.flags&0x4  ? "YES" : "NO", page.flags&0x8  ? "YES" : "NO", page.flags&0x10 ? "YES" : "NO",
	       page.flags2&0x2 ? "YES" : "NO" );
      
      fprintf( f, "Radio Power 0x%X \r\n", page.radioPower );
      fprintf( f, "Repeater channel %d R/S Power 0x%X Start R/S Channel %d StartAddr 0x%X RepStep %d\r\n",
	       page.repeaterChannel, page.repeaterPower, page.starting500Channel, page.imageAddr, page.rep_step );
      
      fprintf( f, "Critical R/S %d   Critical Direct %d     Cycles to R/S %d    Cycles to Direct %d \r\nCycles to Step R/S channel %d Cycles to step Direct channel %d Cycles to Soft Kick %d  Cycles to Hard Kick %d \r\n",
	       page.CriticalLevel500, page.CriticalLevel10, page.defCyclesTo500, page.defCyclesTo10,
	       page.defCyclesToStep500Channel, page.defCyclesToStep10Channel,  page.defCycles2SoftKick, 
	       page.defCycles2HardKick );
      fprintf( f, "edAddr: %d gwAddr %d\r\n",  page.edAddr, page.gwAddr      );
      fprintf( f, "wiggle: 0 shunt %d max-off %d syncword 0x%x \r\n",   
	       page.shunt, page.max_off, page.syncword );
      fprintf( f, "SKU: %d:%d\r\n", page.utcHigh,page.utcLow);
      fclose( f );
    }

  return retValue;
}

void outPageGw( char *filename ){
FILE * f = fopen( filename, "w" );
time_t rawtime;
struct tm * timeinfo;
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    secret[2][12] = 0;
    fprintf( f, "Gateway\n" );
    fprintf( f, "MAC %s Converted on %s version: 21 Apr 2014\r\n", page_gw.mac, asctime( timeinfo ) );
    fprintf( f, "%s\r\n", comment );
    fprintf( f, "boot %s radio %s \n", gw_boot, gw_image );
    fprintf( f, "serial number:%s\r\n", mac2serial( page_gw.mac ) );
    fprintf( f, "MAC %02x%02x%02x%02x%02x%02x\r\n",
        page_gw.mac[0], page_gw.mac[1], page_gw.mac[2], page_gw.mac[3], page_gw.mac[4], page_gw.mac[5] );
    fprintf( f, "dFk %g %g %g %g\r\n", page_gw.dFk[0], page_gw.dFk[1], page_gw.dFk[2], page_gw.dFk[3] );
    fprintf( f, "Tk %g %g\r\n",        page_gw.tk[0],  page_gw.tk[1] );
    fprintf( f, "Channel %d\r\n",      page_gw.channel);
    fprintf( f, "12kbod %s Hopping %s\r\n",
        page_gw.modemSpeed ? "YES" : "NO", page_gw.isHopper? "YES" : "NO");
    fprintf( f, "edAddr: %d gwAddr %d\r\n",  page_gw.edAddr, page_gw.gwAddr);
    fclose( f );
}

// Begin Status values returned from program
static const int OK = 0;
static const int ERROR_CMD_LINE_ARG = 1;
static const int ERROR_MODULE_SETTING = 2;
static const int ERROR_MPP_SETTING = 3;
static const int ERROR_FAILED_LINK_STEP = 4;
static const int ERROR_FAILED_LOGGING_PB = 5;
static const int ERROR_ADDITIONAL_OUTPUT = 6;
static const int ERROR_LINK_GW = 7;
static const int ERROR_NO_MAIN_IMAGE_VERSION = 8;
static const int ERROR_NO_8K_IMAGE_VERSION = 9;
static const int ERROR_NO_8K_IMAGE = 10;
static const int ERROR_NO_MAIN_IMAGE = 11;
static const int ERROR_NO_BOOT_SECTOR_IMAGE = 12;
static const int ERROR_NO_TXT_DIR = 13;
static const int ERROR_NO_HEX_DIR = 14;
static const int ERROR_TXT_DIR_NAME = 15;
static const int ERROR_HEX_DIR_NAME = 16;
static const int ERROR_ARGUMENT_CONVERSION = 17;
static const int ERROR_LOG_FILE = 18;
static const int ERROR_BAD_SKU = 19;
static const int ERROR_NO_PRODUCT_SKU = 20;

static const int INFO_HELP_ONLY = 100;

// End Status values returned from program

int myStrToL(char StrValue[], long * val, char ArgumentFlag[])
{
  int retValue = OK;
  char *endptr, *str;

  str = StrValue;
  errno = 0;    /* To distinguish success/failure after call */
  *val = strtol(str, &endptr, 10);

  /* Check for various possible errors */
  if ((errno == ERANGE && 
       (*val == LONG_MAX || *val == LONG_MIN))
      || (errno != 0 && *val == 0)) 
    {
      printf("%s:%d ERROR failed to convert %s argument %s to a number\n",
	     __FILE__,__LINE__, ArgumentFlag, str);
      fprintf(log, "%s:%d ERROR failed to convert %s argument %s to a number\n",
	     __FILE__,__LINE__, ArgumentFlag, str);
      retValue = ERROR_ARGUMENT_CONVERSION;
    }
  else if (endptr == str) 
    {
      printf("%s:%d ERROR no digits found in %s argument %s\n",
	     __FILE__,__LINE__, ArgumentFlag, str);
      fprintf(log, "%s:%d ERROR no digits found in %s argument %s\n",
	     __FILE__,__LINE__, ArgumentFlag, str);
      retValue = ERROR_ARGUMENT_CONVERSION;
    }
  return retValue;
}

void usage( char * argv[] )
{
  printf("usage: %s < argument list>\n\n",argv[ 0 ]);
  printf("purpose : To set main and 8K image parameter blocks for Midstring 1000 hex files\n\n");
  printf("arguments\n");
  printf("-help <optional, no arg> print out this message and exit.\n");
  printf("-sku  <required,UINT32 (8 chars)> set SKU in Parameter Blocks .\n");
  printf("-mpp  <optional,byte> set mpp field of the main parameter block (byte register 4) .\n");
  printf("-module  <optional,byte> set module field of the main parameter block (byte register 5).\n");
  printf("-comment  <optional,string> set comment field of the output listing of this program.\n");
  printf("-fetFailCount  <optional,int> set fetFailCount field of the main parameter block (0 disables).\n");
  printf("-fetCurrentThreshold  <optional,int> set fetCurrentThreshold field of the main parameter block.\n");
  printf("-fetDeltaCurrent  <optional,int> set fetDeltaCurrent field of the main parameter block.\n");
  printf("-txt-dir  <required,string> sets the directory location of where text output will reside.\n");
  printf("-hex-dir  <required,string> sets the directory location of where hex input files are found.\n");
  printf("-sn  <optional,string> can be used to determine MAC address of unit.\n");
  printf("-mac  <optional,string> the MAC address of the unit.\n");
  printf("-boot  <required,string> the location of the boot sector image.\n");
  printf("-low  <required,string> the location of the low (main) image.\n");
  printf("-high  <required,string> the location of the high (8K) image.\n");
  printf("-ch  <optional,int> the channel that will be used for the main image.\n");
  printf("-rbunch  <optional,byte> set value for byte register 80 (rbunch).\n");
  printf("-gbunch  <optional,byte> set value for byte register 81 (gbunch).\n");
  printf("-addr  <optional,int> set value for integer register 10 (either 0x83F or 0x603F).\n");
  printf("-short-circuit-protection  <optional,byte> set value for shortCircuitLevel byte register.\n");
  printf("-oscLowCurrentLimitInmA  <optional,int> set value for integer register 19 (0 disables).\n");
  printf("-oscDeltaV  <optional,int> set value for integer register 20.\n");
  printf("-oscOVStartupDelay <optional, int> set value for integer register 21.\n");
  printf("-oscAfterStartupDelay <optional, int> set value for integer register 22.\n");
  printf("-oscLowCurrentOnlyDelay <optional, int> set value for integer register 23.\n");
  printf("-cur_noise <optional, int> set value for parameter block field cur_noise.\n");
  printf("-output <optional, string> allows additional output to be produced during code execution.\n");
  printf("-vin-dis-radio <optional, byte> set value for byte parameter block (byte register 79).\n");
  printf("-dfk0 <optional, float> set value a in quadratic ax^3 + bx^2 + cx + d (float register 1).\n" );
  printf("-dfk1 <optional, float> set value b in quadratic ax^3 + bx^2 + cx + d (float register 2).\n" );
  printf("-dfk2 <optional, float> set value c in quadratic ax^3 + bx^2 + cx + d (float register 3).\n" );
  printf("-dfk3 <optional, float> set value d in quadratic ax^3 + bx^2 + cx + d (float register 4).\n" );
  printf("-v-sw-off <optional, float> set value for float register 7.\n" ); 
  printf("-v-turn-on <optional, float> set value for float register 8.\n" );
  printf("-vout-k <optional, float> set value of Linear coefficient for Vout (float register 10).\n");
  printf("-vout-off <optional, float> set value for Vout offset (float register 11).\n" );
  printf("-vin1-k <optional, float> set value of Linear coefficient for Vin1 (float register 12).\n" );
  printf("-vin1-off <optional, float> set value for Vout offset (float register 13).\n" );
  printf("-iout-k <optional, float> set value of Linear coefficient for Iout (float register 14).\n" );
  printf("-iout-off <optional, float> set value for Iout offset (float register 15).\n" );
  printf("-vin2-k <optional, float> set value of Linear coefficient for Vin2 (float register 16).\n" );
  printf("-vin2-off <optional, float> set value for Vin2 offset (float register 17).\n" );
  printf("-text-k <optional, float> set value of Linear coefficient for Text (float register 18).\n" );
  printf("-text-off <optional, float> set value for Text offset (float register 19).\n" );
  printf("-in1-k <optional, float> set value of Linear coefficient for Iin1 (float register 20).\n" );
  printf("-in1-off <optional, float> set value for Iin1 offset (float register 21).\n" );
  printf("-in2-k <optional, float> set value of Linear coefficient for Iin2 (float register 22).\n" );
  printf("-in2-off <optional, float> set value for Iin2 offset (float register 23).\n" );
  printf("-bunch <optional, byte> set value for bunch number (byte pb field myBunch).\n");
  printf("-netId <optional, byte> set value for netId value  (byte pb field netId).\n");
  printf("-versionHighMain <required, int> set value for integer register 12.\n");
  printf("-versionLowMain <required, int> set value for integer register 11.\n");
  printf("-versionHigh8K <required, int> set value for integer register 12 (in 8K image).\n");
  printf("-versionLow8K <required, int> set value for integer register 11 (in 8K image).\n");
  printf("-syncword <optional, int> set value for parameter block field syncword.\n");
  printf("-ea <optional, byte> set value for parameter block field edAddr.\n");
  printf("-ga <optional, byte> set value for parameter block field gwAddr.\n");
  printf("-level500 <optional, byte> set value for parameter block field CriticalLevel500.\n");
  printf("-level10 <optional, byte> set value for parameter block field CriticalLevel10.\n");
  printf("-keep-network <optional, none> if passed prohibits normal network params from being set.\n");
  printf("-keep-levels <optional, none> if passed prohibits levels (500,10) from being set.\n");
  printf("-keep-channel <optional, none> if passed prohibits the channel from being set.\n");
  printf("-keep-channel500 <optional, none> if passed prohibits repeater params from being set.\n");
  printf("-12kbod <optional, none> set boolean value for use12kbod in flag parameter field.\n");
  printf("-30kbod <optional, none> clear boolean value for use12kbod in flag parameter field.\n");
  printf("-250kbod <optional, none> set boolean value for use250kbod in flags parameter field.\n");
  printf("-ask-barcode <optional, none> if passed program will use barcode.\n");
  printf("-speculative-reports <optional, none> if passed program sets speculative_reports field of flags2.\n");
  printf("-rep-step <optional, byte> set the rep_step field value of parameter block (byte register 61).\n");
  printf("-channel-500 <optional, byte> set starting500Channel field value of parameter block.\n");
  printf("-repeater <optional, byte> set boolean isRelay field value of parameter block field named flag.\n");
  printf("-slave <optional, byte> set boolean is500Always field value of parameter block field named flag.\n");
  printf("-no-slave <optional, byte> clear boolean is500Always field value of parameter block field named flag.\n");
  printf("-gw <optional, none> invocation of program is meant to flash a gateway\n");
  printf("-no-search <optional, byte> clear boolean searchCommunication field value of field named flag.\n");
  printf("-ov-startup <optional, byte> sets field ov_startup (byte register 62).\n");
  printf("-shunt <optional, byte> sets field shunt (byte register 74).\n");


}

typedef unsigned char uint8_t;

uint8_t *
hex_decode(const char *in, size_t len,uint8_t *out)
{
  unsigned int i, t;
  unsigned char hn, ln;
  unsigned char aByte = 0;

  for (t = 0,i = 0; i < len; i+=2,++t) 
    {
      hn = in[i] > '9' ? in[i] - 'A' + 10 : in[i] - '0';
      ln = in[i+1] > '9' ? in[i+1] - 'A' + 10 : in[i+1] - '0';

      aByte = (hn << 4 ) | ln;
      out[t] = aByte;
      printf("out[%d] => %d\n",t,out[t]);
    }

  return out;
}

int convertSKU(char * productID, 
	       UINT32 * firstPartOfSKU) 
{
  int retValue = ERROR_BAD_SKU; // Set to OK if good
  long productIDLength = strlen(productID);
  const long int expectedLength = 8; // XXXXXXXX 
  char * token;

  if (productIDLength == expectedLength)
    {
      printf("%s:%d SKU %s valid length of (%ld)\n",
	     __FILE__,__LINE__, productID, productIDLength);
      fprintf(log,"%s:%d SKU %s valid length of (%ld)\n",
	     __FILE__,__LINE__, productID, productIDLength);

      // Ensure 9th byte (position 8) is a -
      unsigned char firstPart[9];
      memset(firstPart,0,9);
      hex_decode(productID, 8,&firstPart[0]);

      retValue = OK;
      *firstPartOfSKU = 0;
      UINT32 tmp = 0;
      tmp = firstPart[0];
      *firstPartOfSKU = ((tmp << 24) & 0xFF000000);
			      
      tmp = firstPart[1];
      *firstPartOfSKU |= ((tmp << 16) & 0x00FF0000);
			      
      tmp = firstPart[2];
      *firstPartOfSKU |= ((tmp << 8) & 0x0000FF00);
			      
      tmp = firstPart[3];
      *firstPartOfSKU |= (tmp & 0x000000FF);
			      
      printf("%s:%d Converted SKU %d\n",
	     __FILE__,__LINE__, 
	     *firstPartOfSKU);

      fprintf(log,"%s:%d SKU Converted : %d\n",
	      __FILE__,__LINE__, *firstPartOfSKU);
    }
  else
    {
      printf("%s:%d ERROR: length of productID (%s) is %ld, expect 8\n",
             __FILE__,__LINE__,productID,strlen(productID));
      fprintf(log,"%s:%d ERROR: length of productID (%s) is %ld, expect 8\n",
	      __FILE__,__LINE__,productID,strlen(productID));
    }

  return retValue;
}

int main(int argc, char **argv)
{
  char logFileName[] = "Mya-linker1000Log.txt";

  int i, channel = 250,
    channel8K = 0,
    versionHighMain = 0x0,
    versionLowMain = 0x0,
    versionHigh8K = 0x0,
    versionLow8K = 0x0,
    ea = 2, 
    ga = 1,
    ov_startup = -1, 
    ov_startup8K = -1, 
    t_superhot = -1, 
    t_superhot8K = -1, 
    t_hot = -1,
    t_hot8K = -1,
    t_cold = -1,
    t_cold8K = -1,
    oc_protection = -1, 
    oc_protection8K = -1, 
    fallback_time = -1,
    fallback_time8K = -1,
    //power_dissipation_limit  = -1, power_dissipation_time =-1,  wiggle_dchan = -1, 
    ov_step = -1, 
    ov_step8K = -1, 
    oc_step = -1,
    oc_step8K = -1,
    shunt = -1, 
    max_off = -1, 
    max_off8K = -1, 
    tenKbod = -1,
    ov = -1,
    ov8K = -1,
    oc = -1, 
    oc8K = -1,
    vCh255 = -1, 
    vCh2558K = -1, 
    parking = -1, 
    stay_in_rx = -1, 
    vinLimit= -1, 
    radioPower = -1,
    radioPower8K = -1,
    k_ov_volt = -1, 
    k_ov_volt8K = -1, 
    k_oc_cur = -1,
    k_oc_cur8K = -1,
    shortCircuitProtection = -1,
    shortCircuitProtection8K = -1,
    curNoise = -100000, 
    curNoise8K = -100000, 
    syncword = -1,
    syncword8K = -1,
    mod = -1, 
    mpp = -1,
    mod8K = -1,
    mpp8K = -1,
    retValue = OK;

  INT16  fetFailCount = 0;
  INT16  fetCurrentThreshold = 0;
  INT16  fetDeltaCurrent = 0;

  INT16  fetFailCount8K = 0;
  INT16  fetCurrentThreshold8K = 0;
  INT16  fetDeltaCurrent8K = 0;

  INT16  oscLowCurrentLimitInmA = 0;    // mA,  
  UINT16 oscDeltaV = 30000;              // raw ADC counts, 
  INT16  oscOVStartupDelay = 240;        // 120 ms 
  INT16  oscAfterStartupDelay = 1200;    // 120*5 ms 
  INT16  oscLowCurrentOnlyDelay = 32760; // 16 sec - maximum delay 

  INT16  oscLowCurrentLimitInmA8K = 0;    // mA,  
  UINT16 oscDeltaV8K = 30000;              // raw ADC counts, 
  INT16  oscOVStartupDelay8K = 240;        // 120 ms 
  INT16  oscAfterStartupDelay8K = 1200;    // 120*5 ms 
  INT16  oscLowCurrentOnlyDelay8K = 32760; // 16 sec - maximum delay 

  UINT32 firstPartOfSKU = 0;

  const unsigned int barrier = 0xAAAA;
  UINT16 startAddr = 0x83F;
  char *sn    = NULL,
    *additionalOutput = NULL,
    *txtDir = NULL,
    *hexDir = NULL;
#if defined(_WIN32)
  char slash[] = "\\";
#else
  char slash[] = "/";
#endif

  signed char level500 = -92, 
    level10 = -102;
  unsigned char rbunch = 0,
    rbunch8K = 0,
    gbunch = 0,
    gbunch8K = 0;
  BYTE bunch = 0xFF, 
    netId = 0xFF, 
    rep_step = 25, 
    channel500 = 0;
  BOOL keep_network = FALSE, 
    keep_levels = FALSE, 
    keep_channel = FALSE, 
    d8_output = FALSE, 
    keep_channel500 = FALSE,
    ask_bar_code = FALSE, 
    two_fifty_kbod = FALSE, 
    speculative_reports = FALSE,
    is_repeater = FALSE, 
    is_slave = FALSE, 
    search = TRUE, 
    is_gw = FALSE, 
    is_windows = FALSE, 
    use_adc = TRUE, 
    is_no_slave = FALSE;
  float vIn1K = -1, 
    vIn1Off = 1E6, 
    iOutK = -1, 
    vModOff = -1, 
    vModOff8K = -1, 
    vWakeUp = -1,
    vWakeUp8K = -1,
    dfk3 = 1E6, 
    dfk2 = 1E6, 
    dfk1 = 1E6, 
    dfk0 = 1E6, 
    iOutOff = 1E6, 
    vOutOff = 1E6, 
    vIn2K = -1, /*22.063*0.5*/ 
    vIn2Off = 1E6,
    tExtK = 1E6, 
    tExtOff = 1E6,   
    iIn1K = 1E6, 
    iIn1Off = 1E6,
    iIn2K = 1E6, 
    iIn2Off = 1E6,
    vShutUp   = -1, /*6000.0*/ 
    vShutUp8K   = -1, /*6000.0*/ 
    vOutK = -1; /*24.096*0.5*/
  
  srand( time( 0 ) ); 

  memset( txt_dir, 0, sizeof( txt_dir ) );
  memset( hex_dir, 0, sizeof( hex_dir ) );
  memset( bootSect, 0, sizeof( bootSect ) );
  memset( imageLow, 0, sizeof( imageLow ) );
  memset( imageHigh, 0, sizeof( imageHigh ) );
  memset( comment, 0, sizeof( comment ) );

  strcpy(txt_dir, "./");
  strcpy(hex_dir, "./");

  errno = 0;
  log = fopen (logFileName, "w");
  if (log == NULL)
    {
      printf("%s:%d ERROR: main function failed to open log file %s for writing\n",
	     __FILE__, __LINE__, logFileName);
      printf("%s:%d ERROR: errno:%d strerror:%s    Exiting ...\n",
	     __FILE__, __LINE__,errno, strerror(errno));
      return ERROR_LOG_FILE;
    }

  for( i = 1; i < argc && retValue == OK; i++ )
    { // command line parameters parsing (in future change to getopt?)
      printf("%s:%d current argument being parsed: %d  => %s\n",__FILE__,__LINE__,i , argv[i]);
      fprintf(log, "%s:%d current argument being parsed: %d  => %s\n",__FILE__,__LINE__,i , argv[i]);

      if( strcmp( argv[ i ], "-help"   ) == 0 )
	{ 
	  // Output usage information and exit
	  usage(argv);
	  retValue = INFO_HELP_ONLY;
	}
      else if( strcmp( argv[ i ], "-mpp"   ) == 0 )
	{ 
	  i++;
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-mpp")) == OK)
	    {
	      mpp = (int)val;
	    }
	}
      else if( strcmp( argv[ i ], "-mpp8K"   ) == 0 )
	{ 
	  i++;
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-mpp8K")) == OK)
	    {
	      mpp8K = (int)val;
	    }
	}
      else if( strcmp( argv[ i ], "-module"   ) == 0 ) 
	{ 
	  i++; 
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-module")) == OK)
	    {
	      mod = (int)val;
	    }
	}
      else if( strcmp( argv[ i ], "-module8K"   ) == 0 ) 
	{ 
	  i++; 
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-module8K")) == OK)
	    {
	      mod8K = (int)val;
	    }
	}
      else if( strcmp( argv[ i ], "-comment" ) == 0 ) 
	{ 
	  i++; 
	  strncpy(comment,argv[ i ],MAXSIZE - 1); 
	}
      else if( strcmp( argv[ i ], "-fetFailCount" ) == 0 ) 
	{ 
	  i++; 
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-fetFailCount")) == OK)
	    {
	      fetFailCount = (int)val; 
	    }
	}
      else if( strcmp( argv[ i ], "-fetFailCount8K" ) == 0 ) 
	{ 
	  i++; 
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-fetFailCount8K")) == OK)
	    {
	      fetFailCount8K = (int)val; 
	    }
	}
      else if( strcmp( argv[ i ], "-fetCurrentThreshold" ) == 0 ) 
	{ 
	  i++; 
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-fetCurrentThreshold")) == OK)
	    {
	      fetCurrentThreshold = (int)val; 
	    }
	}
      else if( strcmp( argv[ i ], "-fetCurrentThreshold8K" ) == 0 ) 
	{ 
	  i++; 
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-fetCurrentThreshold8K")) == OK)
	    {
	      fetCurrentThreshold8K = (int)val; 
	    }
	}
      else if( strcmp( argv[ i ], "-fetDeltaCurrent" ) == 0 ) 
	{ 
	  i++; 
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-fetDeltaCurrent")) == OK)
	    {
	      fetDeltaCurrent = (int)val; 
	    }
	}
      else if( strcmp( argv[ i ], "-fetDeltaCurrent8K" ) == 0 ) 
	{ 
	  i++; 
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-fetDeltaCurrent8K")) == OK)
	    {
	      fetDeltaCurrent8K = (int)val; 
	    }
	}
      else if( strcmp( argv[ i ], "-txt-dir"   )    == 0 )
	{ 
	  i++; 
	  strncpy(txt_dir, argv[ i ],MAXSIZE - 1); 
	  txtDir = argv[ i ];
	  strcat(txt_dir,slash);
	  printf("%s:%d txt_dir = %s  txtDIR = %s argv[i] = %s\n",
		 __FILE__,__LINE__, txt_dir, txtDir, argv[i]);
	  fprintf(log, "%s:%d txt_dir = %s  txtDIR = %s argv[i] = %s\n",
		  __FILE__,__LINE__, txt_dir, txtDir, argv[i]);
	}
      else if( strcmp( argv[ i ], "-hex-dir"   )    == 0 )
	{ 
	  i++; 
	  strncpy(hex_dir, argv[ i ], MAXSIZE - 1); 
	  hexDir = argv[ i ];
	  strcat(hex_dir,slash);
  	  printf("%s:%d hex_dir = %s  hexDIR = %s argv[i] = %s\n",
		 __FILE__,__LINE__, hex_dir, hexDir, argv[i]);
  	  fprintf(log, "%s:%d hex_dir = %s  hexDIR = %s argv[i] = %s\n",
		 __FILE__,__LINE__, hex_dir, hexDir, argv[i]);
	}
      else if( strcmp( argv[ i ], "-sn"   )    == 0 ){ i++; sn = argv[ i ]; }
      else if( strcmp( argv[ i ], "-mac"  )    == 0 ){ i++; sn = mac2serial( argv[ i ] ); }
      else if( strcmp( argv[ i ], "-boot" )    == 0 )
	{ 
	  i++; 
	  strncpy(bootSect,argv[ i ],MAXSIZE - 1);  
	  gw_boot_win = gw_boot = argv[ i ]; 
	}
      else if( strcmp( argv[ i ], "-low"  )    == 0 )
	{ 
	  i++; 
	  strncpy(imageLow,argv[ i ],MAXSIZE - 1);  
	}
      else if( strcmp( argv[ i ], "-high" )    == 0 )
	{ 
	  i++; 
	  strncpy(imageHigh, argv[ i ],MAXSIZE -1); 
	}
      else if( strcmp( argv[ i ], "-ch"   )    == 0 )
	{ 
	  i++; 
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-ch")) == OK)
	    {
	      channel = (BYTE)val;
	    }
	}
      else if( strcmp( argv[ i ], "-ch8K"   )    == 0 )
	{ 
	  i++; 
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-ch8K")) == OK)
	    {
	      channel8K = (BYTE)val;
	    }
	}
      else if( strcmp( argv[ i ], "-gbunch" )  == 0 )
	{ 
	  i++;
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-gbunch")) == OK)
	    {
	      gbunch  = (unsigned char)val;
	    }
	}
      else if( strcmp( argv[ i ], "-gbunch8K" )  == 0 )
	{ 
	  i++;
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-gbunch8K")) == OK)
	    {
	      gbunch8K  = (unsigned char)val;
	    }
	}
      else if( strcmp( argv[ i ], "-rbunch" )  == 0 )
	{ 
	  i++; 
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-rbunch")) == OK)
	    {
	      rbunch  = (unsigned char)val;
	    }
	}
      else if( strcmp( argv[ i ], "-rbunch8K" )  == 0 )
	{ 
	  i++; 
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-rbunch8K")) == OK)
	    {
	      rbunch8K  = (unsigned char)val;
	    }
	}
      else if( strcmp( argv[ i ], "-addr" )    == 0 )
	{ 
	  i++; 
	  startAddr = (UINT16) strtoul( argv[ i ], 0, 16 ); 
	}
      else if( strcmp( argv[ i ], "-short-circuit-protection" ) == 0 )
	{ 
	  i++; 
	  sscanf( argv[ i ], "%d", &shortCircuitProtection );
	}
      else if( strcmp( argv[ i ], "-short-circuit-protection8K" ) == 0 )
	{ 
	  i++; 
	  sscanf( argv[ i ], "%d", &shortCircuitProtection8K );
	}
      else if( strcmp( argv[ i ], "-oscLowCurrentLimitInmA" ) == 0 )
	{ 
	  i++;
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-oscLowCurrentLimitInmA")) == OK)
	    {
	      oscLowCurrentLimitInmA=  0x7FFF & (int)val; 
	    }
	}
      else if( strcmp( argv[ i ], "-oscLowCurrentLimitInmA8K" ) == 0 )
	{ 
	  i++;
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-oscLowCurrentLimitInmA8K")) == OK)
	    {
	      oscLowCurrentLimitInmA8K = 0x7FFF & (int)val; 
	    }
	}
      else if( strcmp( argv[i], "-oscDeltaV" ) == 0 )
	{
	  i++;
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-oscDeltaV")) == OK)
	    {
	      oscDeltaV = 0xFFFF & (int) val; // raw ADC counts,
	    }
	} 
      else if( strcmp( argv[i], "-oscDeltaV8K" ) == 0 )
	{
	  i++;
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-oscDeltaV8K")) == OK)
	    {
	      oscDeltaV8K = 0xFFFF & (int) val; // raw ADC counts,
	    }
	} 
      else if( strcmp( argv[i], "-oscOVStartupDelay" ) == 0 )
	{
	  i++;
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-oscOVStartupDelay")) == OK)
	    {
	      oscOVStartupDelay =  0x7FFF & (int)val;      // 120 ms 
	    }
	}
      else if( strcmp( argv[i], "-oscOVStartupDelay8K" ) == 0 )
	{
	  i++;
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-oscOVStartupDelay8K")) == OK)
	    {
	      oscOVStartupDelay8K =  0x7FFF & (int)val;      // 120 ms 
	    }
	}
      else if( strcmp( argv[i], "-oscAfterStartupDelay" ) == 0 )
	{
	  i++;
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-oscAfterStartupDelay")) == OK)
	    {
	      oscAfterStartupDelay = 0x7FFF & (int)val;    // 120*5 ms 
	    }
	}
      else if( strcmp( argv[i], "-oscAfterStartupDelay8K" ) == 0 )
	{
	  i++;
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-oscAfterStartupDelay8K")) == OK)
	    {
	      oscAfterStartupDelay8K = 0x7FFF & (int)val;    // 120*5 ms 
	    }
	}
      else if( strcmp( argv[i], "-oscLowCurrentOnlyDelay" ) == 0 )
	{
	  i++;
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-oscLowCurrentOnlyDelay")) == OK)
	    {
	      oscLowCurrentOnlyDelay =  0x7FFF & (int)val; // 16 sec - maximum delay 
	    }
	}        
      else if( strcmp( argv[i], "-oscLowCurrentOnlyDelay8K" ) == 0 )
	{
	  i++;
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-oscLowCurrentOnlyDelay8K")) == OK)
	    {
	      oscLowCurrentOnlyDelay8K =  0x7FFF & (int)val; // 16 sec - maximum delay 
	    }
	}        
      else if( strcmp( argv[ i ], "-cur-noise" ) == 0 ){ i++; sscanf( argv[ i ], "%d", &curNoise ); }
      else if( strcmp( argv[ i ], "-cur-noise8K" ) == 0 ){ i++; sscanf( argv[ i ], "%d", &curNoise8K ); }

      else if( strcmp( argv[ i ], "-output" )  == 0 ){ i++; additionalOutput = argv[ i ]; }

      else if( strcmp( argv[ i ], "-vin-dis-radio" ) == 0 ){ i++; sscanf( argv[ i ], "%g", &vShutUp ); }
      else if( strcmp( argv[ i ], "-vin-dis-radio8K" ) == 0 ){ i++; sscanf( argv[ i ], "%g", &vShutUp8K ); }

      else if( strcmp( argv[ i ], "-bunch" )   == 0 )
	{ 
	  i++; 
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-bunch")) == OK)
	    {
	      bunch = (BYTE)val; 
	    }
	}
      else if( strcmp( argv[ i ], "-netId" )   == 0 )
	{ 
	  i++; 
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-netId")) == OK)
	    {
	      netId = (BYTE)val; 
	    }
	}
      else if( strcmp( argv[ i ], "-versionHighMain" )  == 0 )
	{ i++; versionHighMain = (UINT16) strtoul( argv[ i ], 0, 16 ); }
      else if( strcmp( argv[ i ], "-versionLowMain" )  == 0 )
	{ i++; versionLowMain = (UINT16) strtoul( argv[ i ], 0, 16 ); }
      else if( strcmp( argv[ i ], "-versionHigh8K" )  == 0 )
	{ i++; versionHigh8K = (UINT16) strtoul( argv[ i ], 0, 16 ); }
      else if( strcmp( argv[ i ], "-versionLow8K" )  == 0 )
	{ i++; versionLow8K = (UINT16) strtoul( argv[ i ], 0, 16 ); }
      else if( strcmp( argv[ i ], "-syncword" )== 0 )
	{ 
	  i++; 
	  syncword = (UINT16) strtoul( argv[ i ], 0, 16 ); 
	}
      else if( strcmp( argv[ i ], "-syncword8K" )== 0 )
	{ 
	  i++; 
	  syncword8K = (UINT16) strtoul( argv[ i ], 0, 16 ); 
	}
      else if( strcmp( argv[ i ], "-ea" )      == 0 )
	{ 
	  i++; 
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-ea")) == OK)
	    {
	      ea = (int)val;
	    }
	}
      else if( strcmp( argv[ i ], "-ga" )      == 0 )
	{ 
	  i++; 
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-ga")) == OK)
	    {
	      ga = (int)val;
	    }
	}
      else if( strcmp( argv[ i ], "-keep-network" ) == 0 )  keep_network = TRUE;
      else if( strcmp( argv[ i ], "-level500" )     == 0 )
	{ 
	  i++; 
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-level500")) == OK)
	    {
	      level500 = (signed char)val; 
	    }
	}
      else if( strcmp( argv[ i ], "-level10" )      == 0 )
	{ 
	  i++; 
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-level10")) == OK)
	    {
	      level10 = (signed char)val; 
	    }
	}
      else if( strcmp( argv[ i ], "-keep-levels" )  == 0 )     keep_levels    = TRUE;
      else if( strcmp( argv[ i ], "-keep-channel" ) == 0 )     keep_channel   = TRUE;
      else if( strcmp( argv[ i ], "-keep-channel500" ) == 0 )  keep_channel500   = TRUE;
      else if( strcmp( argv[ i ], "-12kbod" ) == 0 )           tenKbod        = TRUE;
      else if( strcmp( argv[ i ], "-30kbod" ) == 0 )           tenKbod        = FALSE;
      else if( strcmp( argv[ i ], "-250kbod" ) == 0 )          two_fifty_kbod = TRUE;
      else if( strcmp( argv[ i ], "-ask-barcode" ) == 0 )      ask_bar_code   = TRUE;
      else if( strcmp( argv[ i ], "-speculative-reports" ) == 0 )  speculative_reports = TRUE;
      else if( strcmp( argv[ i ], "-rep-step" )      == 0 )
	{ 
	  i++; 
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-rep-step")) == OK)
	    {
	      rep_step = (BYTE)val; 
	    }
	}
      else if( strcmp( argv[ i ], "-channel500" )    == 0 )
	{ 
	  i++; 
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-channel500")) == OK)
	    {
	      channel500 = (BYTE)val; 
	    }
	}
      else if( strcmp( argv[ i ], "-repeater" )      == 0 )    is_repeater = TRUE;
      else if( strcmp( argv[ i ], "-slave" )      == 0 )       is_slave = TRUE;
      else if( strcmp( argv[ i ], "-no-slave" )      == 0 )    is_no_slave = TRUE;
      else if( strcmp( argv[ i ], "-gw" )         == 0 )       is_gw = TRUE;
      else if( strcmp( argv[ i ], "-radio_gw" )   == 0 )     {  i++; is_gw = TRUE; gw_image = argv[ i ]; }
      else if( strcmp( argv[ i ], "-win" )        == 0 )        is_windows = TRUE;
      else if( strcmp( argv[ i ], "-noadc" )     == 0 )        use_adc = FALSE;
      else if( strcmp( argv[ i ], "-no-search" )  == 0 )        search  = FALSE;
      else if( strcmp( argv[ i ], "-ov-startup" )  == 0 )    
	{ 
	  i++; 
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-ov-startup")) == OK)
	    {
	      ov_startup = (int)val; 
	    }
	}
      else if( strcmp( argv[ i ], "-ov-startup8K" )  == 0 )    
	{ 
	  i++; 
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-ov-startup8K")) == OK)
	    {
	      ov_startup8K = (int)val; 
	    }
	}
      else if( strcmp( argv[ i ], "-t-superhot" )  == 0 )    
	{ 
	  i++; 
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-t-superhot")) == OK)
	    {
	      t_superhot = (int)val;
	    }
	}
      else if( strcmp( argv[ i ], "-t-superhot8K" )  == 0 )    
	{ 
	  i++; 
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-t-superhot8K")) == OK)
	    {
	      t_superhot8K = (int)val;
	    }
	}
      else if( strcmp( argv[ i ], "-t-hot" )  == 0 )         
	{ 
	  i++; 
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-t-hot")) == OK)
	    {
	      t_hot = (int) val;
	    }
	}
      else if( strcmp( argv[ i ], "-t-hot8K" )  == 0 )         
	{ 
	  i++; 
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-t-hot8K")) == OK)
	    {
	      t_hot8K = (int) val;
	    }
	}
      else if( strcmp( argv[ i ], "-t-cold" )  == 0 )        
	{ 
	  i++; 
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-t-cold")) == OK)
	    {
	      t_cold = (int) val;
	    }
	}
      else if( strcmp( argv[ i ], "-t-cold8K" )  == 0 )        
	{ 
	  i++; 
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-t-cold8K")) == OK)
	    {
	      t_cold8K = (int) val;
	    }
	}
      else if( strcmp( argv[ i ], "-oc-protection" )  == 0 ) 
	{ 
	  i++;
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-oc-protection")) == OK)
	    {
	      oc_protection = (int)val;
	    }
	}
      else if( strcmp( argv[ i ], "-oc-protection8K" )  == 0 ) 
	{ 
	  i++;
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-oc-protection8K")) == OK)
	    {
	      oc_protection8K = (int)val;
	    }
	}
      else if( strcmp( argv[ i ], "-fallback-time" )  == 0 ) 
	{ 
	  i++; 
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-fallback-time")) == OK)
	    {
	      fallback_time = (int)val;
	    }
	}
      else if( strcmp( argv[ i ], "-fallback-time8K" )  == 0 ) 
	{ 
	  i++; 
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-fallback-time8K")) == OK)
	    {
	      fallback_time8K = (int)val;
	    }
	}
      else if( strcmp( argv[ i ], "-ov-step" )  == 0 )         
	{ 
	  i++; 
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-ov-step")) == OK)
	    {
	      ov_step = (int)val;
	    }
	}
      else if( strcmp( argv[ i ], "-ov-step8K" )  == 0 )         
	{ 
	  i++; 
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-ov-step8K")) == OK)
	    {
	      ov_step8K = (int)val;
	    }
	}
      else if( strcmp( argv[ i ], "-oc-step" )  == 0 )         
	{ 
	  i++; 
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-oc-step")) == OK)
	    {
	      oc_step = (int)val;
	    }
	}
      else if( strcmp( argv[ i ], "-oc-step8K" )  == 0 )         
	{ 
	  i++; 
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-oc-step8K")) == OK)
	    {
	      oc_step8K = (int)val;
	    }
	}

      else if( strcmp( argv[ i ], "-stay-in-rx" )  == 0 )      stay_in_rx = TRUE; 
      else if( strcmp( argv[ i ], "-dont-stay-in-rx" )  == 0 ) stay_in_rx = FALSE;
      else if( strcmp( argv[ i ], "-parking" )  == 0 )         parking = TRUE; 
      else if( strcmp( argv[ i ], "-no-parking" )  == 0 )      parking = FALSE;
      else if( strcmp( argv[ i ], "-shunt" )  == 0 )           
	{  
	  i++; 
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-shunt")) == OK)
	    {
	      shunt = (int)val;
	    }
	}
      else if( strcmp( argv[ i ], "-treat-last-byte-as-channel" ) == 0)
	{  
	  page.flags |= 0x8; 
	  page8K.flags |= 0x8; 
	}
      else if( strcmp( argv[ i ], "-max-off" )  == 0 )         
	{  
	  i++; 
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-max_off")) == OK)
	    {
	      max_off = (int)val; 
	    }
	}
      else if( strcmp( argv[ i ], "-max-off8K" )  == 0 )         
	{  
	  i++; 
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-max_off8K")) == OK)
	    {
	      max_off8K = (int)val; 
	    }
	}
      else if( strcmp( argv[ i ], "-ov" )  == 0 )              
	{ 
	  i++; 
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-ov")) == OK)
	    {
	      ov = (int)val; 
	    }
	}
      else if( strcmp( argv[ i ], "-ov8K" )  == 0 )              
	{ 
	  i++; 
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-ov8K")) == OK)
	    {
	      ov8K = (int)val; 
	    }
	}
      else if( strcmp( argv[ i ], "-oc" )  == 0 )              
	{ 
	  i++; 
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-oc")) == OK)
	    {
	      oc = (int)val; 
	    }
	}
      else if( strcmp( argv[ i ], "-oc8K" )  == 0 )              
	{ 
	  i++; 
	  long val;

	  if ((retValue = myStrToL(argv[i], &val, "-oc8K")) == OK)
	    {
	      oc8K = (int)val; 
	    }
	}
      // Share argument between 8K and main PBs
      else if( strcmp( argv[ i ], "-iout-k" ) == 0 )           { i++; sscanf( argv[ i ], "%g", &iOutK ); }
      else if( strcmp( argv[ i ], "-iout-off" ) == 0 )         { i++; sscanf( argv[ i ], "%g", &iOutOff ); }
      else if( strcmp( argv[ i ], "-vout-off" ) == 0 )         { i++; sscanf( argv[ i ], "%g", &vOutOff ); }
      else if( strcmp( argv[ i ], "-vout-k" ) == 0 ) { i++; sscanf( argv[ i ], "%g", &vOutK ); }
      else if( strcmp( argv[ i ], "-vin1-k" ) == 0 )  { i++; sscanf( argv[ i ], "%g", &vIn1K ); }
      else if( strcmp( argv[ i ], "-vin1-off" ) == 0 )          { i++; sscanf( argv[ i ], "%g", &vIn1Off ); }
      else if( strcmp( argv[ i ], "-vin2-k" ) == 0 )            { i++; sscanf( argv[ i ], "%g", &vIn2K ); }
      else if( strcmp( argv[ i ], "-vin2-off" ) == 0 )          { i++; sscanf( argv[ i ], "%g", &vIn2Off ); }
      
      else if( strcmp( argv[ i ], "-text-k" ) == 0 )           { i++; sscanf( argv[ i ], "%g", &tExtK ); }
      else if( strcmp( argv[ i ], "-text-off" ) == 0 )         { i++; sscanf( argv[ i ], "%g", &tExtOff ); }
      else if( strcmp( argv[ i ], "-in1-k" ) == 0 )           { i++; sscanf( argv[ i ], "%g", &iIn1K ); }
      else if( strcmp( argv[ i ], "-in1-off" ) == 0 )         { i++; sscanf( argv[ i ], "%g", &iIn1Off ); }
      else if( strcmp( argv[ i ], "-in2-k" ) == 0 )           { i++; sscanf( argv[ i ], "%g", &iIn2K ); }
      else if( strcmp( argv[ i ], "-in2-off" ) == 0 )         { i++; sscanf( argv[ i ], "%g", &iIn2Off ); }
            
      else if( strcmp( argv[ i ], "-dfk0" ) == 0 )             { i++; sscanf( argv[ i ], "%g", &dfk0 ); }
      else if( strcmp( argv[ i ], "-dfk1" ) == 0 )             { i++; sscanf( argv[ i ], "%g", &dfk1 ); }
      else if( strcmp( argv[ i ], "-dfk2" ) == 0 )             { i++; sscanf( argv[ i ], "%g", &dfk2 ); }
      else if( strcmp( argv[ i ], "-dfk3" ) == 0 )             { i++; sscanf( argv[ i ], "%g", &dfk3 ); }
      // END Share argument between 8K and main PBs

      else if( strcmp( argv[ i ], "-v-sw-off" ) == 0 )         { i++; sscanf( argv[ i ], "%g", &vModOff ); }
      else if( strcmp( argv[ i ], "-v-sw-off8K" ) == 0 )         { i++; sscanf( argv[ i ], "%g", &vModOff8K ); }

      else if( strcmp( argv[ i ], "-v-turn-on" ) == 0 )        { i++; sscanf( argv[ i ], "%g", &vWakeUp ); }
      else if( strcmp( argv[ i ], "-v-turn-on8K" ) == 0 )        { i++; sscanf( argv[ i ], "%g", &vWakeUp8K ); }
      
      else if( strcmp( argv[ i ], "-v-limit" ) == 0 )          { i++; sscanf( argv[ i ], "%d", &vCh255 ); }
      else if( strcmp( argv[ i ], "-v-limit8K" ) == 0 )          { i++; sscanf( argv[ i ], "%d", &vCh2558K ); }

      else if( strcmp( argv[ i ], "-radio-power" ) == 0 )      { i++; radioPower = (UINT16) strtoul( argv[ i ], 0, 16 ); }
      else if( strcmp( argv[ i ], "-radio-power8K" ) == 0 )      { i++; radioPower8K = (UINT16) strtoul( argv[ i ], 0, 16 ); }
      
      else if( strcmp( argv[ i ], "-k-ov-volt" ) == 0 )        { i++; sscanf( argv[ i ], "%d", &k_ov_volt ); }
      else if( strcmp( argv[ i ], "-k-ov-volt8K" ) == 0 )        { i++; sscanf( argv[ i ], "%d", &k_ov_volt8K ); }

      else if( strcmp( argv[ i ], "-k-oc-cur" )  == 0 )        { i++; sscanf( argv[ i ], "%d", &k_oc_cur ); }
      else if( strcmp( argv[ i ], "-k-oc-cur8K" )  == 0 )        { i++; sscanf( argv[ i ], "%d", &k_oc_cur8K ); }
      else if( strcmp( argv[ i ], "-sku" )  == 0 )        
	{ 
	  i++;
	  retValue = convertSKU(argv[i],&firstPartOfSKU);
	}
      else
	{
	  printf("%s:%d ERROR Unmatch command line argument %s  Exiting ...\n",
		 __FILE__,__LINE__,argv[ i ]);
	  fprintf(log, "%s:%d ERROR Unmatch command line argument %s  Exiting ...\n",
		 __FILE__,__LINE__,argv[ i ]);
	  retValue = ERROR_CMD_LINE_ARG;
	}
      printf("%s:%d Current argument is  %s\n", __FILE__,__LINE__,argv[ i ]);
      fprintf(log, "%s:%d Current argument is  %s\n", __FILE__,__LINE__,argv[ i ]);
    }

  // Check for "required" arguments
  if ((versionLowMain == 0) || (versionHighMain == 0))
    {
      printf("%s:%d ERROR non zero version(s) for main image must be passed as arguments\n",
	     __FILE__,__LINE__);
      fprintf(log, "%s:%d ERROR non zero version(s) for main image must be passed as arguments\n",
	     __FILE__,__LINE__);
      retValue = ERROR_NO_MAIN_IMAGE_VERSION;
    }
  else if ((versionLow8K == 0) || (versionHigh8K == 0))
    {
      printf("%s:%d ERROR non zero version(s) for 8K image must be passed as arguments\n",
	     __FILE__,__LINE__);
      fprintf(log, "%s:%d ERROR non zero version(s) for 8K image must be passed as arguments\n",
	     __FILE__,__LINE__);
      retValue = ERROR_NO_8K_IMAGE_VERSION;
    }
  else if (strlen(bootSect) < 8) // 8 is arbitrary length
    {
      printf("%s:%d ERROR file name for boot sector appears not to be set.\n",
	     __FILE__,__LINE__);
      fprintf(log, "%s:%d ERROR file name for boot sector appears not to be set.\n",
	     __FILE__,__LINE__);
      retValue = ERROR_NO_BOOT_SECTOR_IMAGE;
    }
  else if (strlen(imageLow) < 8) // 8 is arbitrary length
    {
      printf("%s:%d ERROR file name for low image appears not to be set.\n",
	     __FILE__,__LINE__);
      fprintf(log, "%s:%d ERROR file name for low image appears not to be set.\n",
	     __FILE__,__LINE__);
      retValue = ERROR_NO_8K_IMAGE;
    }
  else if (strlen(imageHigh) < 8) // 8 is arbitrary length, valid file name should be longer
    {
      printf("%s:%d ERROR file name for high image appears not to be set.\n",
	     __FILE__,__LINE__);
      fprintf(log, "%s:%d ERROR file name for high image appears not to be set.\n",
	     __FILE__,__LINE__);
      retValue = ERROR_NO_MAIN_IMAGE;
    }
  else if (strlen(txt_dir) < 2)
    {
      printf("%s:%d ERROR file name for txt_dir appears not to be set.\n",
	     __FILE__,__LINE__);
      fprintf(log, "%s:%d ERROR file name for txt_dir appears not to be set.\n",
	     __FILE__,__LINE__);
      retValue = ERROR_NO_TXT_DIR;
    }
  else if (strlen(hex_dir) < 2)
    {
      printf("%s:%d ERROR file name for hex_dir appears not to be set.\n",
	     __FILE__,__LINE__);
      fprintf(log, "%s:%d ERROR file name for hex_dir appears not to be set.\n",
	     __FILE__,__LINE__);
      retValue = ERROR_NO_HEX_DIR;
    }
  else if (firstPartOfSKU == 0)
    {
      printf("%s:%d ERROR The product SKU appears not to be set.\n",
	     __FILE__,__LINE__);
      fprintf(log, "%s:%d ERROR The product SKU appears not to be set.\n",
	     __FILE__,__LINE__);
      retValue = ERROR_NO_PRODUCT_SKU;
    }
  if (retValue == OK)
    {
      if( ask_bar_code ) 
	{
	  char line[128];
	  char tmp_buf[128];
	  puts("Enter the barcode and press <Enter>:" );
	  if( fgets( line,127,stdin ) )
	    {  // replaced gets(line)
	      if( sscanf(  line, "33070003 E %s\n", tmp_buf ) )  
		printf(  "read '%s'\n",  sn = tmp_buf );
	      else  
		printf( "wrong format of barcode - '%s'\n", line );
	    }
	  else 
	    puts( "read NULL instead of barcode\n" );
	}

      if( !is_gw )
	{
	  // start by check that values are "legal" (more to come)
	  if ((mod < 0) || (mod > 1))
	    {
	      printf("%s:%d ERROR module can only have a value of 0 or 1, received %d\n",
		     __FILE__,__LINE__,mod);
	      fprintf(log, "%s:%d ERROR module can only have a value of 0 or 1, received %d\n",
		     __FILE__,__LINE__,mod);
	      retValue = ERROR_MODULE_SETTING;
	    }
	  else if ((mod8K < 0) || (mod8K > 1))
	    {
	      printf("%s:%d ERROR 8K module can only have a value of 0 or 1, received %d\n",
		     __FILE__,__LINE__,mod8K);
	      fprintf(log, "%s:%d ERROR 8K module can only have a value of 0 or 1, received %d\n",
		     __FILE__,__LINE__,mod8K);
	      retValue = ERROR_MODULE_SETTING;
	    }
	  else if ((mpp < 0) || (mpp > 1))
	    {
	      printf("%s:%d ERROR MPP can only have a value of 0 or 1, received %d\n",
		     __FILE__, __LINE__, mpp);
	      fprintf(log, "%s:%d ERROR MPP can only have a value of 0 or 1, received %d\n",
		     __FILE__, __LINE__, mpp);
	      retValue = ERROR_MPP_SETTING;
	    }
	  else if ((mpp8K < 0) || (mpp8K > 1))
	    {
	      printf("%s:%d ERROR 8K MPP can only have a value of 0 or 1, received %d\n",
		     __FILE__, __LINE__, mpp8K);
	      fprintf(log, "%s:%d ERROR 8K MPP can only have a value of 0 or 1, received %d\n",
		     __FILE__, __LINE__, mpp8K);
	      retValue = ERROR_MPP_SETTING;
	    }
	  else
	    {
	      if( additionalOutput ) 
		remove( additionalOutput );

	      if( sn ) 
		{ 
		  remove( mystrcat( mystrcat( hex_dir, sn ), ".hex" ) ); 
		  remove( mystrcat( mystrcat( hex_dir, sn ), ".txt" ) );  
		}
      
	      page.versionLow   = versionLowMain;
	      page8K.versionLow = versionLow8K;


	      page.versionHigh   = versionHighMain;	      
	      page8K.versionHigh = versionHigh8K;

	      if( dfk3 < 1E6 )
		{  
		  page.dFk[3] = dfk3; 
		  page.pureOffset = (INT16) dfk3; 
		  page8K.dFk[3] = dfk3; 
		  page8K.pureOffset = (INT16) dfk3; 
		}

	      if( dfk2 < 1E6 ) 
		{
		  page.dFk[2] = dfk2;
		  page8K.dFk[2] = dfk2;
		}
	      if( dfk1 < 1E6 )
		{
		  page.dFk[1] = dfk1;
		  page8K.dFk[1] = dfk1;
		}
	      if( dfk0 < 1E6 ) 
		{
		  page.dFk[0] = dfk0;
		  page8K.dFk[0] = dfk0;
		}

	      if( vWakeUp >= 0 ) 
		page.vin_turn_on = (BYTE) (vWakeUp/100);
	      if( vWakeUp8K >= 0 ) 
		page8K.vin_turn_on = (BYTE) (vWakeUp8K/100);

	      if( vModOff >= 0 ) 
		page.vin_switch_off = (BYTE) (vModOff/100);
	      if( vModOff8K >= 0 ) 
		page8K.vin_switch_off = (BYTE) (vModOff8K/100);

	      if( vShutUp >= 0 ) 
		page.vin_disable_radio = (BYTE) (vShutUp/1000);
	      if( vShutUp8K >= 0 ) 
		page8K.vin_disable_radio = (BYTE) (vShutUp8K/1000);

	      if(  vCh255 >= 0 ) 
		page.vin_limit  = (BYTE) (vCh255/100);
	      if(  vCh2558K >= 0 ) 
		page8K.vin_limit  = (BYTE) (vCh2558K/100);
      
	      if((k_ov_volt >= 0) && (k_ov_volt < 255)) 
		page.k_ov_volt = (BYTE) k_ov_volt;
	      if((k_ov_volt8K >= 0) && (k_ov_volt8K < 255)) 
		page8K.k_ov_volt = (BYTE) k_ov_volt8K;

	      if((k_oc_cur  >= 0) && (k_oc_cur  < 255)) 
		page.k_oc_cur  = (BYTE) k_oc_cur;
	      if((k_oc_cur8K  >= 0) && (k_oc_cur8K  < 255)) 
		page8K.k_oc_cur  = (BYTE) k_oc_cur8K;
      
	      page.module = (BYTE)mod;
	      page.mpp    = (BYTE)mpp;
      
	      page8K.module = (BYTE)mod8K;
	      page8K.mpp    = (BYTE)mpp8K;

	      if ( fetFailCount ) 
		page.fetFailCount = fetFailCount;
	      else
		page.fetFailCount = 0;
		
	      if ( fetFailCount8K ) 
		page8K.fetFailCount = fetFailCount8K;
	      else
		page8K.fetFailCount = 0;		

	      if ( fetCurrentThreshold ) 
		page.fetCurrentThreshold = fetCurrentThreshold;
	      if ( fetCurrentThreshold8K ) 
		page8K.fetCurrentThreshold = fetCurrentThreshold8K;

	      if ( fetDeltaCurrent ) 
		page.fetDeltaCurrent = fetDeltaCurrent;
	      if ( fetDeltaCurrent8K ) 
		page8K.fetDeltaCurrent = fetDeltaCurrent8K;

	      if(  radioPower >= 0 ) 
		page.radioPower    = (BYTE) (radioPower&0xFF);
	      if(  radioPower8K >= 0 ) 
		page8K.radioPower    = (BYTE) (radioPower8K&0xFF);
      
	      page.imageAddr = startAddr;

	      if( vOutK > 0 )
		{ 
		  page.linearK[0][0]   = vOutK;
		  page8K.linearK[0][0]   = vOutK;
		}
	      if( vOutOff <1E6 )
		{ 
		  page.linearK[0][1]   = vOutOff;
		  page8K.linearK[0][1]   = vOutOff;
		}
	      if( vIn1K > 0 )
		{  
		  page.linearK[1][0]   = vIn1K;
		  page8K.linearK[1][0]   = vIn1K;
		}
	      if( vIn1Off  <1E6 ) 
		{
		  page.linearK[1][1]   = vIn1Off;
		  page8K.linearK[1][1]   = vIn1Off;
		}
	      if( iOutK > 0 ) 
		{
		  page.linearK[2][0]   = iOutK;
		  page8K.linearK[2][0]   = iOutK;
		}
	      if( iOutOff <1E6 )
		{ 
		  page.linearK[2][1]   = iOutOff;
		  page8K.linearK[2][1]   = iOutOff;
		}
	      if( vIn2K > 0 )  
		{
		  page.linearK[3][0]   = vIn2K;
		  page8K.linearK[3][0]   = vIn2K;
		}
	      if( vIn2Off  <1E6 )
		{ 
		  page.linearK[3][1]   = vIn2Off;
		  page8K.linearK[3][1]   = vIn2Off;
		}
	      if( tExtK   <1E6 ) 
		{
		  page.linearK[4][0]   = tExtK;
		  page8K.linearK[4][0]   = tExtK;
		}
	      if( tExtOff <1E6 ) 
		{
		  page.linearK[4][1]   = tExtOff;
		  page8K.linearK[4][1]   = tExtOff;
		}
	      if( iIn1K   <1E6 )
		{ 
		  page.linearK[6][0]   = iIn1K;
		  page8K.linearK[6][0]   = iIn1K;
		}
	      if( iIn1Off <1E6 ) 
		{
		  page.linearK[6][1]   = iIn1Off;
		  page8K.linearK[6][1]   = iIn1Off;
		}
	      if( iIn2K   <1E6 )
		{ 
		  page.linearK[5][0]   = iIn2K;
		  page8K.linearK[5][0]   = iIn2K;
		}
	      if( iIn2Off <1E6 )
		{ 
		  page.linearK[5][1]   = iIn2Off;
		  page8K.linearK[5][1]   = iIn2Off;
		}

	      if( curNoise > -99999 ) 
		page.cur_noise = (short) curNoise;
	      if( curNoise8K > -99999 ) 
		page8K.cur_noise = (short) curNoise8K;

	      if( shortCircuitProtection >=0 ) 
		page.shortCircuitProtection = shortCircuitProtection;
	      if( shortCircuitProtection8K >=0 ) 
		page8K.shortCircuitProtection = shortCircuitProtection8K;
      
	      if( ! keep_channel )
		{     
		  page.channel = channel;
		  page8K.channel = channel8K;
		}
	      if( ! keep_network )     
		{
		  page.myBunch = bunch;
		  page.netId = netId;
		  if( tenKbod != -1 ) 
		    { 
		      if ( tenKbod )     
			page.flags  |= 4;   
		      else   
			page.flags  &= ~4; 
		    }
		  if( is_repeater )    
		    page.flags  |= 0x20;
		  if( is_slave )       
		    page.flags  |= 0x40;
		  if( is_no_slave )    
		    page.flags  &= ~0x40;
		  if( search )
		    {
		      page.flags  |= 0x80;
		      page8K.flags |= 0x80;
		    }
		  else
		    {
		      page.flags &= 0x7F;
		      page8K.flags &= 0x7F;
		    }
		  if( two_fifty_kbod )
		    { 
		      page.flags2 |= 2;
		      page8K.flags2 |= 2;
		    }
		  if( speculative_reports ) 
		    page.flags2 |= 4;
		  page.edAddr = ea; 
		  page.gwAddr = ga;   
		  page.rep_step = rep_step;
		}

	      if( ! keep_levels  )     
		{ page.CriticalLevel500 = level500;   
		  page.CriticalLevel10 = level10; 
		}

	      if( ! keep_channel500 )  
		{
		  if( channel500 )    
		    page.repeaterChannel = page.starting500Channel = channel500;
		  else         
		    page.repeaterChannel = page.starting500Channel = page.channel;
		}

	      if(  stay_in_rx>0 )
		{
		  if( stay_in_rx ) 
		    page.flags2 |= 0x8; 
		  else 
		    page.flags2 &= ~0x8;
		}

	      if(  parking>0 )
		{
		  if( parking ) 
		    page.flags2 |= 0x20; 
		  else 
		    page.flags2 &= ~0x20;
		}

	      if(  stay_in_rx )      
		page.flags2 |= 0x8;

	      if(  ov >= 0 ) 
		page.ov = ov & 0xFF;
	      if(  ov8K >= 0 ) 
		page8K.ov = ov8K & 0xFF;

	      if(  oc >= 0 ) 
		page.oc = oc & 0xFF;
	      if(  oc8K >= 0 ) 
		page8K.oc = oc8K & 0xFF;

	      if(  ov_startup >= 0 ) 
		page.ov_startup = ov_startup & 0xFF;
	      if(  ov_startup8K >= 0 ) 
		page8K.ov_startup = ov_startup8K & 0xFF;

	      if(  t_superhot >= 0 ) 
		page.t_superhot = t_superhot & 0xFF;
	      if(  t_superhot8K >= 0 ) 
		page8K.t_superhot = t_superhot8K & 0xFF;

	      if(  t_hot >= 0 )      
		page.t_hot = t_hot & 0xFF;
	      if(  t_hot8K >= 0 )      
		page8K.t_hot = t_hot8K & 0xFF;

	      if(  t_cold >= 0 )     
		page.t_cold = t_cold & 0xFF;
	      if(  t_cold8K >= 0 )     
		page8K.t_cold = t_cold8K & 0xFF;

	      if(  oc_protection >= 0 )    
		page.oc_protection = oc_protection & 0xFF;
	      if(  oc_protection8K >= 0 )    
		page8K.oc_protection = oc_protection8K & 0xFF;

	      if(  syncword >= 0 )   
		page.syncword = syncword & 0xFFFF;
	      if(  syncword8K >= 0 )   
		page8K.syncword = syncword8K & 0xFFFF;

	      if(  fallback_time >= 0 )    
		page.fallback_time = fallback_time & 0xFF;
	      if(  fallback_time8K >= 0 )    
		page8K.fallback_time = fallback_time8K & 0xFF;

	      //if(  power_dissipation_limit >= 0 )    page.power_dissipation_limit = power_dissipation_limit & 0xFF;
	      //if(  power_dissipation_time >= 0 )     page.power_dissipation_time  = power_dissipation_time & 0xFF;
	      //if(  wiggle_dchan >= 0 )               page.wiggle_dchan  = wiggle_dchan & 0xFF;

	      if(  ov_step >= 0 )
		page.ov_step     = ov_step & 0xFF;
	      if(  ov_step8K >= 0 )
		page8K.ov_step     = ov_step8K & 0xFF;

	      if(  oc_step >= 0 )
		page.oc_step     = oc_step & 0xFF;
	      if(  oc_step8K >= 0 )
		page8K.oc_step     = oc_step8K & 0xFF;

	      if(  shunt >= 0 )
		{
		  page.shunt       = shunt & 0xFF;
		  page8K.shunt     = shunt & 0xFF;
		}

	      if(  max_off >= 0 )
		page.max_off     = max_off & 0xFF;
	      if(  max_off8K >= 0 )
		page8K.max_off     = max_off8K & 0xFF;

	      if(  gbunch > 0 )        
		page.gbunch      = gbunch;
	      if(  gbunch8K > 0 )        
		page8K.gbunch      = gbunch8K;

	      if(  rbunch > 0 )      
                page.rbunch      = rbunch;
	      if(  rbunch8K > 0 )      
                page8K.rbunch      = rbunch8K;
      
	      page.barrier = barrier;

	      page.oscLowCurrentLimitInmA = oscLowCurrentLimitInmA;  // mA,  
	      page.oscDeltaV = oscDeltaV;               // raw ADC counts, 
	      page.oscOVStartupDelay = oscOVStartupDelay;      // 120 ms 
	      page.oscAfterStartupDelay = oscAfterStartupDelay;    // 120*5 ms 
	      page.oscLowCurrentOnlyDelay = oscLowCurrentOnlyDelay; // 16 sec - maximum delay 

	      // page.utcProgram = page.utcLow = page.utcHigh = time(0);
	      page.utcProgram = time(0);

	      page.utcHigh = firstPartOfSKU;
	      page8K.utcHigh = firstPartOfSKU;
      
	      page.utcLow = 0;
	      page8K.utcLow = 0;

	      if( !link( sn, bootSect, imageLow, imageHigh, startAddr ) ) 
		{
		  printf("%s:%d ERROR: link step failed. Exiting ...\n",
			 __FILE__,__LINE__);
		  fprintf(log, "%s:%d ERROR: link step failed. Exiting ...\n",
			 __FILE__,__LINE__);
		  retValue = ERROR_FAILED_LINK_STEP;
		}
	      else
		{
		  char pb8KTxt[] = "_8KPB.txt";
		  char pbMainTxt[] = "_MainPB.txt";

		  if ( sn ) 
		    {
		      if ((! logParameterBlockInformation( mystrcat( mystrcat( txt_dir, sn ), pb8KTxt ),
							   page8K)) 
			  || (! logParameterBlockInformation( mystrcat( mystrcat( txt_dir, sn ), pbMainTxt ),
							      page)))
			{
			  printf("%s:%d ERROR Failure to log ParameterBlock information. Exiting ...\n",
				 __FILE__, __LINE__);
			  fprintf(log, "%s:%d ERROR Failure to log ParameterBlock information. Exiting ...\n",
				 __FILE__, __LINE__);
			  retValue = ERROR_FAILED_LOGGING_PB;
			}
		    }  
		  else
		    {
		      if ((! logParameterBlockInformation( mystrcat( mystrcat( txt_dir, mac2serial( page.myMac ) ), 
								     pb8KTxt ), page8K))
			  || (! logParameterBlockInformation( mystrcat( mystrcat( txt_dir, mac2serial( page.myMac ) ), 
									pbMainTxt ), page)))
			{
			  printf("%s:%d ERROR Failure to log ParameterBlock information. Exiting ...\n",
				 __FILE__, __LINE__);
			  fprintf(log, "%s:%d ERROR Failure to log ParameterBlock information. Exiting ...\n",
				 __FILE__, __LINE__);
			  retValue = ERROR_FAILED_LOGGING_PB;
			}
		    }
	  
		  if( (retValue == OK) && additionalOutput ) 
		    {
		      if ( copy( additionalOutput, 
				 mystrcat( mystrcat( hex_dir, mac2serial( page.myMac ) ), ".hex" ) ) == FALSE)
			{
			  printf("%s:%d ERROR: copy of additional output failed, Exiting ...\n",
				 __FILE__, __LINE__);
			  fprintf(log, "%s:%d ERROR: copy of additional output failed, Exiting ...\n",
				 __FILE__, __LINE__);
			  retValue = ERROR_ADDITIONAL_OUTPUT;
			}
		    }
		}
	    }
	}
      else
	{ // create gateway bootable image
	  if( is_windows ) 
	    gw_boot = gw_boot_win;
	  
	  page_gw.edAddr = ea; 
	  page_gw.gwAddr = ga;
	  page_gw.channel = channel;
	  page_gw.useADC = use_adc;
	  
	  if( tenKbod != -1 ) 
	    page_gw.modemSpeed = tenKbod;
	  if( !linkgw( sn, gw_boot, gw_image ) )
	    {
	      retValue = ERROR_LINK_GW;
	    }
	  else
	    {
	      if( sn ) 
		outPageGw( mystrcat( mystrcat( txt_dir, sn ), "_gw.txt" )  );  
	      else     
		outPageGw( mystrcat( mystrcat( txt_dir, mac2serial( page_gw.mac ) ), "_gw.txt" )  );
	      
	      if( additionalOutput ) 
		{
		  copy( additionalOutput, mystrcat( mystrcat( hex_dir, mac2serial( page_gw.mac ) ), "_gw.hex" ) );
		}
	    }
	}
    }
  if (log)
    fclose(log);
  return retValue; // Status of program returned
}
