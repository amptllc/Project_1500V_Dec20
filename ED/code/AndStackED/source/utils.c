#include "hal_main.h"

//void reset(void){ WDCTL = 0x8 | 0x3; while( TRUE ) *((BYTE *)0xFFFF) = 0x80; }
// LBC function reset, Watchdog is enabled and timer interval selected, mark memory as to why reset
void reset(void){ while( TRUE ){ WDCTL = 0x8 | 0x3;  *((BYTE *)0xFFFF) = 0x80; } }

INT32 delta = 0;
/******************************************************************************
* @fn  addDF
* @brief       This adds function shift to compensate frequency offset
* Parameters:  frequency shuft
* @return void
******************************************************************************/
void addDF(INT32 dFreq){
INT32 freq;
    INT_GLOBAL_ENABLE( INT_OFF );
        //freq = 1L*FREQ0;               freq += 256L*FREQ1;                 freq += 65536L*FREQ2;    
        ((BYTE *)(&freq))[0] = FREQ0; ((BYTE *)(&freq))[1] = FREQ1; ((BYTE *)(&freq))[2] = FREQ2;
        //freq = FREQ0; freq |= FREQ1<<8; freq |= ((UINT32)FREQ2)<<16;
        freq += dFreq;
        //FREQ0 = (BYTE)(freq&0xFF);     FREQ1 = (BYTE)((freq>>8) &0xFF);    FREQ2 = (BYTE)((freq>>16)&0xFF);
        FREQ0 = ((BYTE *)(&freq))[0]; FREQ1 = ((BYTE *)(&freq))[1]; FREQ2 = ((BYTE *)(&freq))[2];
        delta += dFreq;
        FSCTRL0 = 0; 
    INT_GLOBAL_ENABLE( INT_ON );
}

#define EXT_GND 3
#define INT_GND 9

// LBC - subtractGround : subtractGround over range of from to to if not EXT_GND or INT_GND
void substractGround( INT32 *ptr, BYTE from, BYTE to ){
//INT32 acc = ptr[EXT_GND];
    for( ; from < to; from++) if( from != EXT_GND && from != INT_GND ) ptr[from] -= ptr[EXT_GND]; 
}
extern void tickWait( BYTE ms );
//void secDelay( void ){BYTE cwCnt; for(cwCnt = 0; cwCnt < 10; cwCnt++){ WDCTL = 0xA8;  WDCTL = 0x58; halWait( 100 ); } } // 1 sec delay
/*
#define STROBE_TX                    0x03   // Strobe commands for the RFST
#define RADIO_MODE_UNDEF             0xF0
void cw( void ){
    PKTCTRL0 = 0x20; PKTLEN = 0x55; RFST = STROBE_TX;
    while( !RFTXRXIF ) asm("NOP");
}
*/
//void zerofill( BYTE *ptr, BYTE size){  while( size-- ) *ptr++ = 0; } //mymemset( (BYTE *)ptr, 0, size ); }
void zerofill( BYTE *ptr, BYTE size){  if( ((UINT16)ptr) > 0x8000) while( size-- ) *ptr++ = 0; } //mymemset( (BYTE *)ptr, 0, size ); }
//void mymemset( BYTE *buff,        BYTE what,    BYTE counter ){ while( counter-- ) *buff++ = what; }
//void mymemcpy( BYTE *destination, BYTE *source, BYTE counter ){ while( counter-- ) *destination++ = *source++; }
void mymemcpy( BYTE *destination, BYTE *source, BYTE counter ){ if( ((UINT16)destination) > 0x8000) while( counter-- ) *destination++ = *source++; }
BOOL mymemcmp( BYTE *destination, BYTE *source, BYTE counter ){ while( counter-- ) if(*destination++ != *source++) return 1; return 0; }

// LBC convS and convU convert floats to signed or unsigned 16bit, avoiding overflow
INT16  convS( float f ){ INT32 t = (INT32)f; if( t > 32767L ) return 32767; else if( t < -32767L ) return -32767; return (INT16) t; }
UINT16 convU( float f ){ INT32 t = (INT32)f; if( t > 65535L ) return 65535; else if( t < 0L )      return 0;      return (UINT16)t; }
