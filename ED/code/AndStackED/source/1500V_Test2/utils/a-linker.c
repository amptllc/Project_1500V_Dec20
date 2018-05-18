// reads device hex file, extracts scaling data
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef signed   short  INT16;
typedef unsigned short  UINT16;
typedef unsigned int    UINT32;
typedef unsigned char   BYTE;
typedef unsigned char   BOOL;
typedef int              INT32;

#define AES_SIZE                    16
#define TRUE                        1
#define FALSE                       0
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
    BOOL   useADC       __attribute__((__packed__)), use30kbod __attribute__((__packed__));
    BYTE   power        __attribute__((__packed__));
    BOOL   long_packets __attribute__((__packed__));
    BYTE   mac[ 6 ]     __attribute__((__packed__));
    BOOL   isRxOnly     __attribute__((__packed__));
    BOOL   isHopper     __attribute__((__packed__));
    BYTE   edAddr       __attribute__((__packed__)), gwAddr __attribute__((__packed__));
    BOOL   useFEC __attribute__((__packed__));
    BYTE   version __attribute__((__packed__));
    BOOL   longReport __attribute__((__packed__));
    BYTE   maxDevs __attribute__((__packed__));
    BYTE   bootDelay __attribute__((__packed__));
    UINT32 secondsJoinEnabled __attribute__((__packed__));
    BOOL   printSupercycle __attribute__((__packed__));
    BOOL   f_hopper __attribute__((__packed__));
} page_gw = {                                                                                                   // 68
    0xAAAA, 0,
    { 'A', 'N', 'D', 'S',  0,  'l', 'a', 'r', 'A', 'u', 'g', 'u', 's', 't',  0,   7 },
    { 'A', 'u', 'g', 'u', 's', 't',  0,   7,  'A', 'u', 'g', 'u', 's', 't',  0,   7 },

    0,  //should be 0 in production

    { -0.000627, 0.0469, 0.253, -123.0 },
    { 0.435, 625.0 },
    5,    121000L,  64,
    TRUE, TRUE,
    0xED,
    TRUE,
    { 0,0,0,0,0,0 },
    FALSE, FALSE,
    2,     1,
    FALSE, 0xE2, TRUE,
    16,
    2,
    0, //14L*24L*3600L
    TRUE, FALSE
};

struct { //{{{
    UINT16 barrier           __attribute__((__packed__)) __attribute__ ((aligned (1)));                                                                                         // 2
    BYTE   netId             __attribute__((__packed__)) __attribute__ ((aligned (1))), 
           myBunch           __attribute__((__packed__)) __attribute__ ((aligned (1)));                                                                                  // 2
    UINT16 groupId           __attribute__((__packed__)) __attribute__ ((aligned (1)));                                                                                         // 2
    // MAC number is exactly a SERIAL NUMBER
    // format:
    // 3bit weekday | 6 bit week | 7 bit year | 1 byte test center | 2 byte seq number | 1 byte random
    BYTE   myMac[6]          __attribute__((__packed__)) __attribute__ ((aligned (1)));                                                                                        // 6
    // dF = -0.0005618976581444 t^3 + 0.0539971103485394 t^2 - 0.8658779608121610 t - 249.5883835010120000
    // t  = (adcs[5]-73000)) / 260
    float dFk[4]            __attribute__((__packed__)) __attribute__ ((aligned (1)));                                                                                          // 16

    float vrefPolynom[3]    __attribute__((__packed__)) __attribute__ ((aligned (1)));                                                                                  //* 12
    // Vout Pin Iout Vin Text Tint
    float linearK [6][2]    __attribute__((__packed__)) __attribute__ ((aligned (1)));                                                                                  //* 48

    UINT16 syncword          __attribute__((__packed__)) __attribute__ ((aligned (1)));
    INT16  oc_ramp_level     __attribute__((__packed__)) __attribute__ ((aligned (1)));
    BYTE vin_disable_radio   __attribute__((__packed__)) __attribute__ ((aligned (1)));                                                                                           // 1
    BYTE rbunch              __attribute__((__packed__)) __attribute__ ((aligned (1))); 
    BYTE gbunch              __attribute__((__packed__)) __attribute__ ((aligned (1))); 
    BYTE shortCircuitProtection __attribute__((__packed__)) __attribute__ ((aligned (1)));                                                                                          

    char curKey[ AES_SIZE ] __attribute__((__packed__)) __attribute__ ((aligned (1)));                                                                                // 16
    char curIV [ AES_SIZE ] __attribute__((__packed__)) __attribute__ ((aligned (1)));                                                                                // 16

    /*
    struct {
      BYTE   testNumber              __attribute__((__packed__)); // testNumber & 0x80 != 0 - successful, otherwise - not successful, 7 bytes for test Number
      UINT32 testDate                __attribute__((__packed__));   // UTC timestamp
    }tests[4]                        __attribute__((__packed__));                                                                                              // 20
    */
    BYTE   tests[20]                 __attribute__((__packed__)) __attribute__ ((aligned (1)));  
    UINT32 installDate               __attribute__((__packed__)) __attribute__ ((aligned (1)));  // UTC timestamp                                                                      4
    /*
    BYTE   azimuth                   __attribute__((__packed__)) __attribute__ ((aligned (1))), 
           positionInString          __attribute__((__packed__)) __attribute__ ((aligned (1)));                                                                       // 2
    UINT16 string                    __attribute__((__packed__)) __attribute__ ((aligned (1)));
    UINT16 elevation                 __attribute__((__packed__)) __attribute__ ((aligned (1)));
    UINT16 latitude                  __attribute__((__packed__)) __attribute__ ((aligned (1)));
    UINT16 longitude                 __attribute__((__packed__)) __attribute__ ((aligned (1)));
    UINT16 altitude                  __attribute__((__packed__)) __attribute__ ((aligned (1)));                    // 10
    */
    BYTE k_ov_volt                   __attribute__((__packed__)) __attribute__ ((aligned (1)));  
    BYTE k_oc_cur                    __attribute__((__packed__)) __attribute__ ((aligned (1)));  
    BYTE   reserved[7]               __attribute__((__packed__)) __attribute__ ((aligned (1)));  
    BYTE synch_phase                 __attribute__((__packed__)) __attribute__ ((aligned (1)));  
    BYTE synch_freq                  __attribute__((__packed__)) __attribute__ ((aligned (1)));  
    BYTE bandwidth250                __attribute__((__packed__)) __attribute__ ((aligned (1)));  
    BYTE channel                     __attribute__((__packed__)) __attribute__ ((aligned (1)));  
    // 1
    //float VinTurnOn __attribute__((__packed__)), VinShutOff __attribute__((__packed__)),  VinDisableRadio __attribute__((__packed__)), 
    UINT32 utcProgram      __attribute__((__packed__)) __attribute__ ((aligned (1))), 
           utcLow          __attribute__((__packed__)) __attribute__ ((aligned (1))), 
           utcHigh         __attribute__((__packed__)) __attribute__ ((aligned (1)));
    float tkCurrent                 __attribute__((__packed__)) __attribute__ ((aligned (1)));
    float tkPower                   __attribute__((__packed__)) __attribute__ ((aligned (1)));                                       // 20
    BYTE   mpp                       __attribute__((__packed__)) __attribute__ ((aligned (1))), 
           module                    __attribute__((__packed__)) __attribute__ ((aligned (1)));
    BYTE   ov                        __attribute__((__packed__)) __attribute__ ((aligned (1))),  
           oc                        __attribute__((__packed__)) __attribute__ ((aligned (1)));
    BYTE   radioPower                __attribute__((__packed__)) __attribute__ ((aligned (1))),
           edAddr                    __attribute__((__packed__)) __attribute__ ((aligned (1))),           
           gwAddr                    __attribute__((__packed__)) __attribute__ ((aligned (1))),
           repeaterChannel           __attribute__((__packed__)) __attribute__ ((aligned (1))),  
           repeaterPower             __attribute__((__packed__)) __attribute__ ((aligned (1)));
    /*
    BOOL   production       : 1 __attribute__((__packed__)),
           ensureWDReset    : 1 __attribute__((__packed__)),
           use10kbod        : 1 __attribute__((__packed__)),
           useFEC           : 1 __attribute__((__packed__)),
           hoppingAllowed   : 1 __attribute__((__packed__)),
           isRelay          : 1 __attribute__((__packed__)),
           is500Always      : 1 __attribute__((__packed__)),
           searchCommunication : 1 __attribute__((__packed__));
    */
    BYTE   flags                     __attribute__((__packed__)) __attribute__ ((aligned (1)));
    BYTE   showState                 __attribute__((__packed__)) __attribute__ ((aligned (1)));
    signed char   CriticalLevel500  __attribute__((__packed__)) __attribute__ ((aligned (1))), 
                    CriticalLevel12  __attribute__((__packed__)) __attribute__ ((aligned (1)));
    INT16   pureOffset               __attribute__((__packed__)) __attribute__ ((aligned (1)));
    UINT16 defCyclesTo500            __attribute__((__packed__)) __attribute__ ((aligned (1))), 
           defCyclesTo10             __attribute__((__packed__)) __attribute__ ((aligned (1))),
           defCyclesToStep500Channel __attribute__((__packed__)) __attribute__ ((aligned (1))), 
           defCyclesToStep10Channel  __attribute__((__packed__)) __attribute__ ((aligned (1))),
           defCycles2SoftKick        __attribute__((__packed__)) __attribute__ ((aligned (1))), 
           defCycles2HardKick        __attribute__((__packed__)) __attribute__ ((aligned (1)));
    UINT16 imageAddr                 __attribute__((__packed__)) __attribute__ ((aligned (1)));
    UINT16 versionLow                __attribute__((__packed__)) __attribute__ ((aligned (1))), 
           versionHigh               __attribute__((__packed__)) __attribute__ ((aligned (1)));
    BYTE   starting500Channel        __attribute__((__packed__)) __attribute__ ((aligned (1)));
    /*BOOL    tryOtherImageFirst : 1,
            use250kbod         : 1,
            theRest            : 6;
            */
    BYTE    flags2                   __attribute__((__packed__)) __attribute__ ((aligned (1)));
    UINT16  prepAddr                 __attribute__((__packed__)) __attribute__ ((aligned (1)));
    BYTE    rep_step                 __attribute__((__packed__)) __attribute__ ((aligned (1)));
    BYTE    ov_startup               __attribute__((__packed__)) __attribute__ ((aligned (1)));
    BYTE    t_superhot               __attribute__((__packed__)) __attribute__ ((aligned (1)));
    BYTE    t_hot                    __attribute__((__packed__)) __attribute__ ((aligned (1)));
    BYTE    t_cold                   __attribute__((__packed__)) __attribute__ ((aligned (1)));
    BYTE    fallback_time            __attribute__((__packed__)) __attribute__ ((aligned (1)));
    BYTE    network_cnst_1           __attribute__((__packed__)) __attribute__ ((aligned (1)));
    BYTE    network_cnst_2           __attribute__((__packed__)) __attribute__ ((aligned (1)));
    BYTE    oc_protection            __attribute__((__packed__)) __attribute__ ((aligned (1)));
    BYTE    oc_step                  __attribute__((__packed__)) __attribute__ ((aligned (1)));
    BYTE    reserved1                __attribute__((__packed__)) __attribute__ ((aligned (1)));
    BYTE    reserved2                __attribute__((__packed__)) __attribute__ ((aligned (1)));
    BYTE    ov_step                  __attribute__((__packed__)) __attribute__ ((aligned (1)));
    BYTE    shunt                    __attribute__((__packed__)) __attribute__ ((aligned (1)));
    BYTE    max_off                  __attribute__((__packed__)) __attribute__ ((aligned (1)));
    BYTE    vin_limit                __attribute__((__packed__)) __attribute__ ((aligned (1)));
    BYTE    vin_turn_on              __attribute__((__packed__)) __attribute__ ((aligned (1)));
    BYTE    vin_switch_off           __attribute__((__packed__)) __attribute__ ((aligned (1)));
    //UINT16  crc                      __attribute__((__packed__)) __attribute__ ((aligned (1)));
    UINT16  post_barrier             __attribute__((__packed__)) __attribute__ ((aligned (1)));
    //}}}
} page __attribute__((__packed__)) __attribute__ ((aligned (1))) = {//{{{                                                                                                  //
    0xAAAA,
    (BYTE)0, (BYTE)0, 0xFFFF, //  permanent !!! Normal
    { 0, 0, 0, 0, 0, 0 },
    {
      //-0.000784329277, 0.069999928482, -1.557036645365, -229.914041475601 + 32.0
      -0.000693, 0.0556, 0.613, -200 //+ 300.0
    }, // temperature compensation
    {  2496.0, 0.0000004, 73.0 }, // vref polynomials
    //  Vout  0          Pin  1          Iout   2     Vin  3         Text 4          Iin2  5
    { {13.87, -1670}, {12.65, 0.0}, {7.93, -584.0}, {12.49, 0.0}, {0.435, 625.0}, {7.76, -591 } },
    0xF0F0, -1000, 0, 0, 0, 170,
    { 'A', 'N', 'D', 'S',  0,  'l', 'a', 'r', 'A', 'u', 'g', 'u', 's', 't',  0,   7 },
    { 'A', 'u', 'g', 'u', 's', 't',  0,   7,  'A', 'u', 'g', 'u', 's', 't',  0,   7 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    0xFFFFFFFF, 
    0xFF, 0xFF,    
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, //0xFF, 0xFF, 0xFF },
    120, 128, 0x4D,
    //(BYTE)0xFF, 
    //(BYTE)0xFF,
    //(unsigned short)0xFFFF, (unsigned short)0xFFFF, (unsigned short)0xFFFF, (unsigned short)0xFFFF, (unsigned short)0xFFFF,
    
    (BYTE)0, //  should be changed to 0 in the production version
    //24000.0*0.5, 20000.0*0.5, 12000.0*0.5,  // 0 for debug
    
    0, 0, 0,
    0.00002, 0.00005,
    TRUE, TRUE,  // mpp & module
    (BYTE)200, (BYTE)210, // ov oc
    0xFF, 2, 1, 0, // radioPower, edAddr, gwAddr, repChan
    0xFF, // 500kbod Power, 0xCB => -10 db, we may think about +1 - FF, 0 0xFE, -5 0xDB, -15 0x8E, -20 0x4D, -25  0x51
    /*
    TRUE, TRUE, FALSE, FALSE, TRUE, // production, WDReset, 10kbod, FEC, hopping,
//    TRUE, FALSE, FALSE, 1,   // Repeater
//    FALSE, TRUE, TRUE, 1,   // Slave
    FALSE, FALSE, TRUE,   1,  // Search
//   FALSE, FALSE, FALSE, 1,    // Simple
    */
    //0b10010111, 1
    
/*
    INT16   pureOffset               __attribute__((__packed__)) __attribute__ ((aligned (1)));
    UINT16 defCyclesTo500            __attribute__((__packed__)) __attribute__ ((aligned (1))), 
           defCyclesTo10             __attribute__((__packed__)) __attribute__ ((aligned (1))),
           defCyclesToStep500Channel __attribute__((__packed__)) __attribute__ ((aligned (1))), 
           defCyclesToStep10Channel  __attribute__((__packed__)) __attribute__ ((aligned (1))),
           defCycles2SoftKick        __attribute__((__packed__)) __attribute__ ((aligned (1))), 
           defCycles2HardKick        __attribute__((__packed__)) __attribute__ ((aligned (1)));
*/
    
    0x97, 1,
    -92, -102,
    -200,
    40, 30, 3, 4,
    8, 80,
    0x83F,
    0xF2, 0xF2,
    0xFF,
    // 7654 3210
    // 1000 1001
    (BYTE)0x89,
    0xFFFF,
    25, 200, 110, 100, 90, 10, 7, 11, 60,
    10,  0,  0, 10, 0, 1,
    0, 0, 0,
    0x5555 // was 0xFFFF
};//}}}
int dummy = 0;

float dfk[] = {-0.000693, 0.0556, 0.613, -270};

int original_version = 0x33;
char txt_dir[512];
char hex_dir[512];
#define CRC16_POLY 0x8005 
// taken form http://www.ti.com/lit/an/swra111d/swra111d.pdf
UINT16 crc16_update(UINT16 crcReg, BYTE crcData) { 
BYTE j; 
    for (j = 0; j < 8; j++) { 
        if (((crcReg & 0x8000) >> 8) ^ (crcData & 0x80)) crcReg = (crcReg << 1) ^ CRC16_POLY; 
        else                                               crcReg = (crcReg << 1); 
        crcData <<= 1; 
    } 
    return crcReg; 
}// culCalcCRC

char *mac2serial( BYTE * mac){ //{{{
static char serial[ 32 ];
    unsigned int w, y, n, temp = 0; char letter;
    n    = mac[5] | (mac[4]<<8) | (mac[3]<<16);
    temp = mac[2] | (mac[1]<<8) | (mac[0]<<16);
    letter = ((temp>>6) & 0x1F) + 'A';
    y      = (temp>>11) & 0x7F;
    w      = (temp>>18) & 0x3F;
    sprintf( serial, "%02d%02d%c%06d", w, y, letter, n );
    return serial;
}//}}}
void serial2mac( char *serial, BYTE *mac ){//{{{
unsigned int w, y, n, temp; unsigned char letter;
    sscanf( serial, "%02d%02d%c%06d", &w, &y, &letter, &n );
    letter = (letter >= 'a') ? (letter-'a') : (letter-'A');
    mac[5] = n & 0xFF; mac[4] = (n>>8) & 0xFF; mac[3] = (n>>16)&0xFF;
    n = ( ( w & 0x3F ) << 18 ) | ( ( y & 0x7F ) << 11 ) | ( ( letter & 0x1F ) << 6 );
    mac[2] = n & 0xFF; mac[1] = (n>>8) & 0xFF; mac[0] = (n>>16)&0xFF;
}//}}}
BYTE ch2int(   unsigned char ch ) { //{{{
    if(      ch >= '0' && ch <= '9' ) ch-= '0';
    else if( ch >= 'A' && ch <= 'F' ) ch = 10 + (ch - 'A');
    else if( ch >= 'a' && ch <= 'f' ) ch = 10 + (ch - 'a');
    else                              ch = 0;
    return   ch & 0xF;
}//}}}
BOOL printPage( FILE *f, UINT16 addr );
BYTE readOneByte( char *ptr ){ return ( (ch2int(ptr[0])<<4) | ch2int(ptr[1]) ) & 0xFF; }
char *code( BYTE *ptr, BYTE l, UINT16 addr ){ //{{{
UINT32 checksum = l;
static char buffer[ 64 ], buf[ 16 ];
    checksum += (addr>>8) & 0xFF;
    checksum += addr & 0xFF;
    sprintf( buffer, ":%02X%04X00", l, addr );
    while( l-- ){ checksum += *ptr; sprintf( buf, "%02X", *ptr++ ); strcat( buffer, buf ); }
    checksum = 0x100 - ( checksum & 0xFF );
    sprintf( buf, "%02X\r\n", checksum & 0xFF ); strcat( buffer, buf );
    return buffer;
}//}}}

char secret[ 3 ][ 64 ];

BOOL link(char *serial, char *bootSec, char *image1, char *image2, unsigned short startAddr){ //{{{
char buf[ 64 ], fname[ 512 ];
char *ret = NULL;
char *images[ 2 ] = { image1, image2 };
int i, j;
UINT16 addr; UINT16 crc;
FILE *f1, *f;
BYTE *ptr;
    if( serial == NULL ) serial = mac2serial( page.myMac ); else serial2mac( serial, page.myMac );

    strcpy( fname, hex_dir ); strcat( fname, serial );
    f = fopen( strcat( fname, ".hex" ), "w" );  if( f == NULL ) return FALSE;

    for( f1 = fopen( bootSec, "r+" ); f1 && !feof( f1 ); ret = fgets( buf, 64, f1 ) ){
        if( buf[8] == '5' ){
            BYTE  tmp[ 16 ];
            addr = 0x3E0;
            for( j = 0; j < 2; j++, addr += 0x10 ){
                for( i = 0; i < 16; i++ )  tmp[ i ] = rand() & 0xFF;
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
        if( ret && buf[7] == '0') fputs( buf, f );
    }
    if( f1 ) fclose( f1 );

    for( i = 0; i < 2; i++ ){
        for( f1 = fopen( images[ i ], "r+" ); f1 && !feof( f1 ); ret = fgets( buf, 64, f1 ) )
            if( ret && buf[7] == '0' && strncmp( buf, ":03000000", 9 ) /*&&  strncmp( buf, ":00000001FF", 12 )*/  ) fputs( buf, f );
        if( f1 ) fclose( f1 );
    }

    //page.crc = 0xFFFF; i = 240; ptr = (BYTE *)&page; while( i-- ) page.crc = crc16_update( page.crc, *ptr++ );
    //printf( "CRC is 0x%04X\n", ((int)page.crc) );
    printPage( f, 0x400 );
    printPage( f, 0x5C00 );
    
    fclose( f );
    return TRUE;
}//}}}

BOOL linkgw( char *serial, char *bootSec, char *image ){
char buf[ 64 ], fname[ 256 ];
char *ret = NULL;
int i, j;
FILE *f1, *f;
UINT16 addr = 0x7C00, l;
BYTE *pagePtr = (BYTE *)&page_gw;
unsigned int  byteCount = sizeof( page_gw );

    if( serial == NULL ) serial = mac2serial( page_gw.mac ); else serial2mac( serial, page_gw.mac );
    strcpy( fname, serial );
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
    while( byteCount ){//{{{ writing the hex file
        i = min( 0x10, byteCount );
        fputs( code( pagePtr, i, addr ), f );
        pagePtr += i; addr += i; byteCount -= i;
    }//}}}

    fclose( f );
    return TRUE;
}


int min(int i, int j){ return i < j ? i : j; }
BOOL printPage(FILE *f, UINT16 addr){//{{{
UINT16 l;
BYTE tmp_page[ sizeof( page ) ];
BYTE *pagePtr = (BYTE *)&page;
unsigned int  byteCount = sizeof( page ), i;
    memset( tmp_page, 0, sizeof( page ) );
    memcpy( tmp_page, pagePtr, sizeof( page ) );
    pagePtr = tmp_page;
    if( *pagePtr != 0xAA && *pagePtr != 0x55 ) return FALSE;
    printf( "size of page is %d\n", byteCount );
    while( byteCount ){//{{{ writing the hex file
        i = min( 0x10, byteCount );
        fputs( code( pagePtr, i, addr ), f );
        pagePtr += i; addr += i; byteCount -= i;
    }//}}}
    return TRUE;
}//}}}

BYTE xtob( char *ptr )    { unsigned int i; sscanf(ptr, "%02X", &i ); return (BYTE)( i & 0xFF ); }
void readFromHexFile( char *fname, void *_ptr, int len, UINT16 startAddr ){ //{{{
FILE *f = fopen( fname, "r+" );
char buffer[ 64 ];
BYTE *ptr = _ptr;
int l, addr, type, i, j;
    memset( ptr, 0xFF, len );
    while( !feof( f ) ){
        if( !fgets( buffer, 64, f ) ){  break; } //puts( "Problems readiding input HEX file" );  exit( 2 ); }
        sscanf( buffer, ":%02X%04X%02X", &l, &addr, &type );
        if( ( type == 0 ) && ( ( startAddr <= (addr+l) ) && ( addr < ( startAddr + len ) ) ) )
            for( i = 0; i < l; i++, addr++ )
                if( ( addr >= startAddr ) && ( addr < ( startAddr + len ) ) ) ptr[ addr - startAddr ] = xtob( &buffer[ 2*i + 9 ] );
        if( addr > (startAddr + len) ) break;
    }
    fclose( f );
}//}}}
typedef unsigned int pointer;
char *mystrcat( char *fname, char *ext ) { static char buffer[ 256 ];  strcpy( buffer, fname );  strcat( buffer, ext ); }
char *mystrcat1( char *fname, char *ext ){ static char buffer1[ 256 ]; strcpy( buffer1, fname ); strcat( buffer1, ext ); }

void copy( char *destFname, char *srcFname ){ //{{{
char buffer[ 512 ];
FILE *dest = fopen( destFname, "w" );
FILE *src  = fopen( srcFname, "r" );
    if( ( src == NULL ) || ( dest == NULL ) ) return;
    while( !feof( src ) ){
        int l = fread(   buffer, 1, 512, src  );
        if( l ) fwrite(  buffer, 1, l,   dest );
    }
    fclose( src ); fclose( dest );
}//}}}

char *bootSect = "../ED/hex/fc-boot.hex",            *imageLow = "../ED/hex/and-stack-ed-g-1k.hex",    *imageHigh = "../ED/hex/and-stack-ed-g-15k.hex",
     *d8 = "p:/Firmware/TI_RevD8/and-stack-ed-v8.hex",
     *gw_boot = "../GW_BOU/hex/boot_gw.hex", *gw_boot_win = "../GW_BOU/hex/boot_gw-win.hex", *gw_image = "../GW_BOU/hex/gw.hex";
char comment[ 512 ];

void outPage( char *filename ){//{{{
FILE * f = fopen( filename, "w" );
time_t rawtime;
struct tm * timeinfo;
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    secret[2][12] = 0;
    fprintf( f, "MAC %s Converted on %s to version %X from version %X\r\n", secret[ 2 ], asctime( timeinfo ), 0, original_version );
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
    fprintf( f, "Vout %g %g \r\nPin %g %g\r\nIout %g %g\r\nVin %g %g\r\nText %g %g\r\nTint %g %g\r\n",
        page.linearK[0][0], page.linearK[0][1],
        page.linearK[1][0], page.linearK[1][1],
        page.linearK[2][0], page.linearK[2][1],
        page.linearK[3][0], page.linearK[3][1],
        page.linearK[4][0], page.linearK[4][1],
        page.linearK[5][0], page.linearK[5][1] );
    fprintf( f, "VinTurnOn %g \r\nVinSwitchOff %g\r\nVinDisableRadio %g\r\nVinLimit %g\r\ntkCurrent %g\r\ntkPower %g\r\n",
          page.vin_turn_on*100.0, 
          page.vin_switch_off*100.0,
          page.vin_disable_radio*1000.0,
          page.vin_limit*100.0,
          page.tkCurrent,
          page.tkPower);
    fprintf( f, "defaults: OV %d OV_Startup %d OV_Step %d OC %d MPP %s Module %s channel %d\r\n",
        page.ov, page.ov_startup, page.ov_step,
        page.oc, 
        page.mpp?"ON":"OFF", page.module?"ON":"OFF", page.channel);
    fprintf( f, "Temp Protection: TSuperHot %d THot %d TCold %d OCStep %d FallbackTime %d OC Protection %d\r\n",
        page.t_superhot, page.t_hot, page.t_cold, page.oc_step, page.fallback_time, page.oc_protection   );
    fprintf( f, "OC Ramp Level: %d OV Volt K %d OC Cur K %d \r\n",
        page.oc_ramp_level, page.k_ov_volt, page.k_oc_cur   );

    fprintf( f, "Repeater %s Slave %s Searching %s 12kbod %s FEC %s Hopping %s 250_kbod %s\r\n",
        page.flags&0x20 ? "YES" : "NO", page.flags&0x40 ? "YES" : "NO", page.flags&0x80 ? "YES" : "NO",
        page.flags&0x4  ? "YES" : "NO", page.flags&0x8  ? "YES" : "NO", page.flags&0x10 ? "YES" : "NO",
        page.flags2&0x2 ? "YES" : "NO"
    );
    fprintf( f, "Radio Power 0x%X \r\n", page.radioPower );
    fprintf( f, "Repeater channel %d R/S Power 0x%X Start R/S Channel %d StartAddr 0x%X RepStep %d\r\n",
        page.repeaterChannel, page.repeaterPower, page.starting500Channel, page.imageAddr, page.rep_step );
    fprintf( f, "Critical R/S %d   Critical Direct %d     Cycles to R/S %d    Cycles to Direct %d \r\nCycles to Step R/S channel %d Cycles to step Direct channel %d Cycles to Soft Kick %d  Cycles to Hard Kick %d \r\n",
        page.CriticalLevel500, page.CriticalLevel12, page.defCyclesTo500, page.defCyclesTo10,
        page.defCyclesToStep500Channel, page.defCyclesToStep10Channel,  page.defCycles2SoftKick, page.defCycles2HardKick );
    fprintf( f, "edAddr: %d gwAddr %d\r\n",  page.edAddr, page.gwAddr      );
    fprintf( f, "wiggle: 0 shunt %d max-off %d syncword 0x%x \r\n",   page.shunt, page.max_off, page.syncword );
    fclose( f );
}//}}}


void outPageGw( char *filename ){//{{{
FILE * f = fopen( filename, "w" );
time_t rawtime;
struct tm * timeinfo;
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    secret[2][12] = 0;
    fprintf( f, "Gateway\n" );
    fprintf( f, "MAC %s Converted on %s version %X\r\n", page_gw.mac, asctime( timeinfo ), page_gw.version );
    fprintf( f, "%s\r\n", comment );
    fprintf( f, "boot %s radio %s \n", gw_boot, gw_image );
    fprintf( f, "serial number:%s\r\n", mac2serial( page_gw.mac ) );
    fprintf( f, "MAC %02x%02x%02x%02x%02x%02x\r\n",
        page_gw.mac[0], page_gw.mac[1], page_gw.mac[2], page_gw.mac[3], page_gw.mac[4], page_gw.mac[5] );
    fprintf( f, "dFk %g %g %g %g\r\n", page_gw.dFk[0], page_gw.dFk[1], page_gw.dFk[2], page_gw.dFk[3] );
    fprintf( f, "Tk %g %g\r\n",        page_gw.tk[0],  page_gw.tk[1] );
    fprintf( f, "Channel %d\r\n",      page_gw.channel);
    fprintf( f, "12kbod %s FEC %s Hopping %s\r\n",
        page_gw.use30kbod ? "NO" : "YES", page_gw.useFEC ? "YES" : "NO", page_gw.isHopper? "YES" : "NO");
    fprintf( f, "edAddr: %d gwAddr %d\r\n",  page_gw.edAddr, page_gw.gwAddr);
    fclose( f );
}//}}}

int main(int argc, char **argv){ //{{{
int i, channel = 250, ver = 0xF1, ea = 2, ga = 1;
char *sn    = NULL;
char *inputC3 = NULL, *inputD8 = NULL, *inputE = NULL, *additionalOutput = NULL, *inputE2 = NULL;
UINT16 startAddr = 0x83F ;
float vShutUp   = -1/*6000.0*/, vInK = -1/*22.063*0.5*/, vOutK = -1 /*24.096*0.5*/;
BYTE bunch = 0xFF, netId = 0xFF, rep_step = 25, channel500 = 0;
BOOL keep_network = FALSE, keep_levels = FALSE, keep_channel = FALSE, d8_output = FALSE, keep_channel500 = FALSE,
     ask_bar_code = FALSE, two_fifty_kbod = FALSE, speculative_reports = FALSE,
     is_repeater = FALSE, is_slave = FALSE, search = TRUE; 
signed char level500 = -92, level12 = -102;
BOOL is_gw = FALSE, is_windows = FALSE, use_adc = TRUE, is_no_slave = FALSE;
int ov_startup = -1, t_superhot = -1, t_hot = -1, t_cold = -1, oc_protection = -1, fallback_time = -1,
    //power_dissipation_limit  = -1, power_dissipation_time =-1,  wiggle_dchan = -1, 
    ov_step = -1, oc_step = -1,
    shunt = -1, max_off = -1, tenKbod = -1;
int ov = -1, oc = -1, vCh255 = -1, parking = -1, stay_in_rx = -1, vinLimit= -1, radioPower = -1,
    k_ov_volt = -1, k_oc_cur = -1;
float pInK = -1, iOutK = -1, vModOff = -1, vWakeUp = -1;
float dfk3 = 1E6, dfk2 = 1E6, dfk1 = 1E6, dfk0 = 1E6, 
       iOutOff = 1E6, vOutOff = 1E6, pInOff = 1E6, vInOff = 1E6,
       tExtK = 1E6, tExtOff = 1E6,   tIntK = 1E6, tIntOff = 1E6;
int shortCircuitProtection = -1;
       
int ocRampLevel = -100000;
int syncword = -1;
unsigned int barrier = 0xAAAA;
int modOff = 0, mppOff = 0;
unsigned char rbunch = 0, gbunch = 0;


    strcpy(txt_dir, "./");
    strcpy(hex_dir, "./");
    srand( time( 0 ) ); memset( comment, 0, sizeof( comment ) );
    for( i = 1; i < argc; i++ ){ // command line parameters parsing
        if( strcmp( argv[ i ], "-mppOff"   ) == 0 )    { mppOff = 1; }
        if( strcmp( argv[ i ], "-modOff"   ) == 0 )    { modOff = 1; }
        if( strcmp( argv[ i ], "-txt-dir"   )    == 0 ){ i++; strcpy(txt_dir, argv[ i ]); }
        if( strcmp( argv[ i ], "-hex-dir"   )    == 0 ){ i++; strcpy(hex_dir, argv[ i ]); }
        if( strcmp( argv[ i ], "-sn"   )    == 0 ){ i++; sn = argv[ i ]; }
        if( strcmp( argv[ i ], "-mac"  )    == 0 ){ i++; sn = mac2serial( argv[ i ] ); }
        if( strcmp( argv[ i ], "-boot" )    == 0 ){ i++; bootSect = argv[ i ];  gw_boot_win = gw_boot = argv[ i ]; }
        if( strcmp( argv[ i ], "-low"  )    == 0 ){ i++; imageLow = argv[ i ];  }
        if( strcmp( argv[ i ], "-high" )    == 0 ){ i++; imageHigh = argv[ i ]; }
        if( strcmp( argv[ i ], "-ch"   )    == 0 ){ i++; channel = atoi( argv[ i ] ); }
        if( strcmp( argv[ i ], "-gbunch" )  == 0 ){ i++; gbunch  = atoi( argv[ i ] ); }
        if( strcmp( argv[ i ], "-rbunch" )  == 0 ){ i++; rbunch  = atoi( argv[ i ] ); }
        if( strcmp( argv[ i ], "-addr" )    == 0 ){ i++; startAddr = (UINT16) strtoul( argv[ i ], 0, 16 ); }
        if( strcmp( argv[ i ], "-short-circuit-protection" ) == 0 ){ i++; sscanf( argv[ i ], "%d", &shortCircuitProtection ); }
        if( strcmp( argv[ i ], "-oc-ramp-level" ) == 0 ){ i++; sscanf( argv[ i ], "%d", &ocRampLevel ); }
        if( strcmp( argv[ i ], "-output" )  == 0 ){ i++; additionalOutput = argv[ i ]; }
        if( strcmp( argv[ i ], "-v-dis-radio" ) == 0 ){ i++; sscanf( argv[ i ], "%g", &vShutUp ); }
        if( strcmp( argv[ i ], "-vin-k" ) == 0 )  { i++; sscanf( argv[ i ], "%g", &vInK ); }
        if( strcmp( argv[ i ], "-vout-k" ) == 0 ) { i++; sscanf( argv[ i ], "%g", &vOutK ); }
        if( strcmp( argv[ i ], "-bunch" )   == 0 ){ i++; bunch = atoi( argv[ i ] ); }
        if( strcmp( argv[ i ], "-netId" )   == 0 ){ i++; netId = atoi( argv[ i ] ); }
        if( strcmp( argv[ i ], "-ver" )     == 0 ){ i++; ver = (UINT16) strtoul( argv[ i ], 0, 16 ); }
        if( strcmp( argv[ i ], "-syncword" )== 0 ){ i++; syncword = (UINT16) strtoul( argv[ i ], 0, 16 ); }
        if( strcmp( argv[ i ], "-ea" )      == 0 ){ i++; ea = atoi( argv[ i ] ); }
        if( strcmp( argv[ i ], "-ga" )      == 0 ){ i++; ga = atoi( argv[ i ] ); }
        if( strcmp( argv[ i ], "-keep-network" ) == 0 )  keep_network = TRUE;
        if( strcmp( argv[ i ], "-level500" )     == 0 ){ i++; level500 = atoi( argv[ i ] ); }
        if( strcmp( argv[ i ], "-level12" )      == 0 ){ i++; level12  = atoi( argv[ i ] ); }
        if( strcmp( argv[ i ], "-keep-levels" )  == 0 )     keep_levels    = TRUE;
        if( strcmp( argv[ i ], "-keep-channel" ) == 0 )     keep_channel   = TRUE;
        if( strcmp( argv[ i ], "-keep-channel500" ) == 0 )  keep_channel500   = TRUE;
        if( strcmp( argv[ i ], "-12kbod" ) == 0 )           tenKbod        = TRUE;
        if( strcmp( argv[ i ], "-30kbod" ) == 0 )           tenKbod        = FALSE;
        if( strcmp( argv[ i ], "-250kbod" ) == 0 )          two_fifty_kbod = TRUE;
        if( strcmp( argv[ i ], "-ask-barcode" ) == 0 )      ask_bar_code   = TRUE;
        if( strcmp( argv[ i ], "-speculative-reports" ) == 0 )  speculative_reports = TRUE;
        if( strcmp( argv[ i ], "-rep-step" )      == 0 ){ i++; rep_step   = atoi( argv[ i ] ); }
        if( strcmp( argv[ i ], "-channel500" )    == 0 ){ i++; channel500 = atoi( argv[ i ] ); }
        if( strcmp( argv[ i ], "-repeater" )      == 0 )    is_repeater = TRUE;
        if( strcmp( argv[ i ], "-slave" )      == 0 )       is_slave = TRUE;
        if( strcmp( argv[ i ], "-no-slave" )      == 0 )    is_no_slave = TRUE;
        if( strcmp( argv[ i ], "-gw" )         == 0 )       is_gw = TRUE;
        if( strcmp( argv[ i ], "-radio_gw" )   == 0 )     {  i++; is_gw = TRUE; gw_image = argv[ i ]; }
        if( strcmp( argv[ i ], "-win" )        == 0 )        is_windows = TRUE;
        if( strcmp( argv[ i ], "-noadc" )     == 0 )        use_adc = FALSE;
        if( strcmp( argv[ i ], "-no-search" )  == 0 )        search  = FALSE;
        if( strcmp( argv[ i ], "-ov-startup" )  == 0 )    { i++; ov_startup = atoi( argv[ i ] );}
        if( strcmp( argv[ i ], "-t-superhot" )  == 0 )    { i++; t_superhot = atoi( argv[ i ] );}
        if( strcmp( argv[ i ], "-t-hot" )  == 0 )         { i++; t_hot      = atoi( argv[ i ] );}
        if( strcmp( argv[ i ], "-t-cold" )  == 0 )        { i++; t_cold     = atoi( argv[ i ] );}
        if( strcmp( argv[ i ], "-oc-protection" )  == 0 )       { i++; oc_protection    = atoi( argv[ i ] );}
        if( strcmp( argv[ i ], "-fallback-time" )  == 0 )       { i++; fallback_time = atoi( argv[ i ] );}
        
        if( strcmp( argv[ i ], "-ov-step" )  == 0 )                 { i++; ov_step = atoi( argv[ i ] );}
        if( strcmp( argv[ i ], "-oc-step" )  == 0 )                 { i++; oc_step = atoi( argv[ i ] );}
        if( strcmp( argv[ i ], "-stay-in-rx" )  == 0 )              stay_in_rx = TRUE; 
        if( strcmp( argv[ i ], "-dont-stay-in-rx" )  == 0 )         stay_in_rx = FALSE;
        if( strcmp( argv[ i ], "-parking" )  == 0 )                 parking = TRUE; 
        if( strcmp( argv[ i ], "-no-parking" )  == 0 )              parking = FALSE;
        if( strcmp( argv[ i ], "-shunt" )  == 0 )                   {  i++; shunt = atoi( argv[ i ] ); }
        if( strcmp( argv[ i ], "-treat-last-byte-as-channel" ) == 0){  page.flags |= 0x8; }
        if( strcmp( argv[ i ], "-max-off" )  == 0 )                 {  i++; max_off = atoi( argv[ i ] ); }

        if( strcmp( argv[ i ], "-ov" )  == 0 )                      { i++; ov = atoi( argv[ i ] ); }
        if( strcmp( argv[ i ], "-oc" )  == 0 )                      { i++; oc = atoi( argv[ i ] ); }
        if( strcmp( argv[ i ], "-iout-k" ) == 0 )                   { i++; sscanf( argv[ i ], "%g", &iOutK ); }
        if( strcmp( argv[ i ], "-iout-off" ) == 0 )                 { i++; sscanf( argv[ i ], "%g", &iOutOff ); }

        if( strcmp( argv[ i ], "-vout-off" ) == 0 )                 { i++; sscanf( argv[ i ], "%g", &vOutOff ); }
        if( strcmp( argv[ i ], "-pin-off" ) == 0 )                  { i++; sscanf( argv[ i ], "%g", &pInOff ); }
        if( strcmp( argv[ i ], "-vin-off" ) == 0 )                  { i++; sscanf( argv[ i ], "%g", &vInOff ); }
        if( strcmp( argv[ i ], "-pin-k" ) == 0 )                    { i++; sscanf( argv[ i ], "%g", &pInK ); }
        
        if( strcmp( argv[ i ], "-text-k" ) == 0 )                   { i++; sscanf( argv[ i ], "%g", &tExtK ); }
        if( strcmp( argv[ i ], "-text-off" ) == 0 )                 { i++; sscanf( argv[ i ], "%g", &tExtOff ); }
        if( strcmp( argv[ i ], "-tint-k" ) == 0 )                   { i++; sscanf( argv[ i ], "%g", &tIntK ); }
        if( strcmp( argv[ i ], "-tint-off" ) == 0 )                 { i++; sscanf( argv[ i ], "%g", &tIntOff ); }
        
        
        if( strcmp( argv[ i ], "-dfk0" ) == 0 )                     { i++; sscanf( argv[ i ], "%g", &dfk0 ); }
        if( strcmp( argv[ i ], "-dfk1" ) == 0 )                     { i++; sscanf( argv[ i ], "%g", &dfk1 ); }
        if( strcmp( argv[ i ], "-dfk2" ) == 0 )                     { i++; sscanf( argv[ i ], "%g", &dfk2 ); }
        if( strcmp( argv[ i ], "-dfk3" ) == 0 )                     { i++; sscanf( argv[ i ], "%g", &dfk3 ); }
        if( strcmp( argv[ i ], "-v-sw-off" ) == 0 )                 { i++; sscanf( argv[ i ], "%g", &vModOff ); }
        if( strcmp( argv[ i ], "-v-turn-on" ) == 0 )                { i++; sscanf( argv[ i ], "%g", &vWakeUp ); }
        
        if( strcmp( argv[ i ], "-v-limit" ) == 0 )                  { i++; sscanf( argv[ i ], "%d", &vCh255 ); }
        if( strcmp( argv[ i ], "-radio-power" ) == 0 )              { i++; radioPower = (UINT16) strtoul( argv[ i ], 0, 16 ); }

        if( strcmp( argv[ i ], "-k-ov-volt" ) == 0 )                { i++; sscanf( argv[ i ], "%d", &k_ov_volt ); }
        if( strcmp( argv[ i ], "-k-oc-cur" )  == 0 )                { i++; sscanf( argv[ i ], "%d", &k_oc_cur ); }
        
    }

    if( ask_bar_code ) {
        char line[128];
        char tmp_buf[128];
        puts("Enter the barcode and press <Enter>:" );
        if( gets( line ) ){
            if( sscanf(  line, "33070003 E %s\n", tmp_buf ) )  printf(  "read '%s'\n",  sn = tmp_buf );
            else  printf( "wrong format of barcode - '%s'\n", line );
        }else puts( "read NULL instead of barcode\n" );
    }

    if( !is_gw ){
        if( additionalOutput ) remove( additionalOutput );
        if( sn ) { remove( mystrcat( mystrcat( hex_dir, sn ), ".hex" ) ); remove( mystrcat( mystrcat( hex_dir, sn ), ".txt" ) );  }

        if( dfk3 < 1E6 ){  page.dFk[3] = dfk3; page.pureOffset = (INT16) dfk3; }
        if( dfk2 < 1E6 )   page.dFk[2] = dfk2;
        if( dfk1 < 1E6 )   page.dFk[1] = dfk1;
        if( dfk0 < 1E6 )   page.dFk[0] = dfk0;
        
        if( vWakeUp >= 0 ) page.vin_turn_on       = (BYTE) (vWakeUp/100);
        if( vModOff >= 0 ) page.vin_switch_off    = (BYTE) (vModOff/100);
        if( vShutUp >= 0 ) page.vin_disable_radio = (BYTE) (vShutUp/1000);
        if(  vCh255 >= 0 ) page.vin_limit         = (BYTE) (vCh255/100);

        if((k_ov_volt >= 0) && (k_ov_volt < 255)) page.k_ov_volt = (BYTE) k_ov_volt;
        if((k_oc_cur  >= 0) && (k_oc_cur  < 255)) page.k_oc_cur  = (BYTE) k_oc_cur;

        if( modOff )       page.module            = 0;
        if( mppOff )       page.mpp               = 0;
        
        if(  radioPower >= 0 ) page.radioPower    = (BYTE) (radioPower&0xFF);
        
        page.imageAddr = startAddr;
        if( vOutK > 0 ) page.linearK[0][0]   = vOutK;
        if( pInK > 0 )  page.linearK[1][0]   = pInK;
        if( iOutK > 0 ) page.linearK[2][0]   = iOutK;
        
        if( iOutOff <1E6 ) page.linearK[2][1]   = iOutOff;
        if( vOutOff <1E6 ) page.linearK[0][1]   = vOutOff;
        if( pInOff  <1E6 ) page.linearK[1][1]   = pInOff;
        if( vInOff  <1E6 ) page.linearK[3][1]   = vInOff;

        if( tExtK   <1E6 ) page.linearK[4][0]   = tExtK;
        if( tExtOff <1E6 ) page.linearK[4][1]   = tExtOff;
        if( tIntK   <1E6 ) page.linearK[5][0]   = tIntK;
        if( tIntOff <1E6 ) page.linearK[5][1]   = tIntOff;
        
        if( ocRampLevel > -99999 ) page.oc_ramp_level = (short) ocRampLevel;
        if( shortCircuitProtection >=0 ) page.shortCircuitProtection = shortCircuitProtection;

        if( vInK > 0 )  page.linearK[3][0]   = vInK;
        if( ! keep_channel )     page.channel = channel;
        if( ! keep_network )     {
            page.myBunch          = bunch;      page.netId           = netId;
            if( tenKbod != -1 ) { if( tenKbod )     page.flags  |= 4;   else   page.flags  &= ~4; }
            if( is_repeater )    page.flags  |= 0x20;
            if( is_slave )       page.flags  |= 0x40;
            if( is_no_slave )    page.flags  &= ~0x40;
            if( search )         page.flags  |= 0x80; else page.flags &= 0x7F;
            if( two_fifty_kbod ) page.flags2 |= 2;
            if( speculative_reports ) page.flags2 |= 4;
            page.edAddr = ea; page.gwAddr = ga;   page.rep_step = rep_step;
        }
        if( ! keep_levels  )     { page.CriticalLevel500 = level500;   page.CriticalLevel12 = level12; }
        if( ! keep_channel500 )  {
            if( channel500 )    page.repeaterChannel = page.starting500Channel = channel500;
            else                page.repeaterChannel = page.starting500Channel = page.channel;
        }
        if(  stay_in_rx>0 ){
            if( stay_in_rx ) page.flags2 |= 0x8; else page.flags2 &= ~0x8;
        }
        if(  parking>0 ){
            if( parking ) page.flags2 |= 0x20; else page.flags2 &= ~0x20;
        }
        if(  stay_in_rx )      page.flags2 |= 0x8;
        if(  ov >= 0 ) page.ov = ov & 0xFF;
        if(  oc >= 0 ) page.oc = oc & 0xFF;
        if(  ov_startup >= 0 ) page.ov_startup = ov_startup & 0xFF;
        if(  t_superhot >= 0 ) page.t_superhot = t_superhot & 0xFF;
        if(  t_hot >= 0 )      page.t_hot = t_hot & 0xFF;
        if(  t_cold >= 0 )     page.t_cold = t_cold & 0xFF;
        if(  oc_protection >= 0 )    page.oc_protection = oc_protection & 0xFF;
        if(  syncword >= 0 )   page.syncword = syncword & 0xFFFF;
        if(  fallback_time >= 0 )    page.fallback_time = fallback_time & 0xFF;
        //if(  power_dissipation_limit >= 0 )    page.power_dissipation_limit = power_dissipation_limit & 0xFF;
        //if(  power_dissipation_time >= 0 )     page.power_dissipation_time  = power_dissipation_time & 0xFF;
        //if(  wiggle_dchan >= 0 )               page.wiggle_dchan  = wiggle_dchan & 0xFF;
        if(  ov_step >= 0 )                    page.ov_step       = ov_step & 0xFF;
        if(  oc_step >= 0 )                    page.oc_step       = oc_step & 0xFF;
        if(  shunt >= 0 )                      page.shunt         = shunt & 0xFF;
        if(  max_off >= 0 )                    page.max_off       = max_off & 0xFF;
        if(  gbunch > 0 )                      page.gbunch        = gbunch;
        if(  rbunch > 0 )                      page.rbunch        = rbunch;
        
        page.versionLow   = page.versionHigh   = ver;
        page.barrier = barrier;
        page.utcProgram = page.utcLow = page.utcHigh = time(0);
        
        if( !link( sn, bootSect, imageLow, imageHigh, startAddr ) ) exit( 1 );

        if( sn ) outPage( mystrcat( mystrcat( txt_dir, sn ), ".txt" )  );  
        else     outPage( mystrcat( mystrcat( txt_dir, mac2serial( page.myMac ) ) , ".txt" )  );
        if( additionalOutput ) copy( additionalOutput, mystrcat( mystrcat( hex_dir, mac2serial( page.myMac ) ), ".hex" ) );
    }else{ // create gateway bootable image
        if( is_windows ) gw_boot = gw_boot_win;
        page_gw.edAddr = ea; page_gw.gwAddr = ga;
        page_gw.channel = channel;
        page_gw.useADC = use_adc;
        if( tenKbod != -1 ) page_gw.use30kbod = !tenKbod;
        page_gw.version =  ver;
        if( !linkgw( sn, gw_boot, gw_image ) ) exit( 1 );
        if( sn ) outPageGw( mystrcat( sn, "_gw.txt" )  );  
        else     outPageGw( mystrcat( mac2serial( page.myMac ) , "_gw.txt" )  );
        if( additionalOutput ) copy( additionalOutput, mystrcat( mac2serial( page.myMac ), "_gw.hex" ) );
    }
    exit( 0 );
}//}}}
