#include "hal_main.h"
//#include <stdlib.h>

#define TX          0x13
#define TX_END      0x14
#define STARTCAL    0x8
#define ENDCAL      0xC
#define RX          0xD
#define RX_END      0xE
#define RX_RST      0xF

#define FALSE       0
#define TRUE        1

#define STROBE_TX                    0x03   // Strobe commands for the RFST
#define STROBE_RX                    0x02   // register
#define STROBE_CAL                   0x01  
#define STROBE_IDLE                  0x04   

#define HOP_N_CHANNELS 25
#define HOP_K_CHANNELS 10

//static UINT32 __next_rand;
#define RAND_MAX 0x7FFF
/*
inline void srand(unsigned int seed){
  __next_rand = seed;
}
inline int rand(void){
  __next_rand = __next_rand * 1103515245 + 12345;
  return (unsigned int) (__next_rand >> 16) & RAND_MAX;
}
*/

BYTE hopping_channels[ HOP_N_CHANNELS ];
void setup_hopper( BYTE curch ){
static BYTE idx = 0, idx1 = 0, swp;
static UINT32 __next_rand;
    idx = HOP_N_CHANNELS; while( idx-- ) hopping_channels[ idx ] = ( curch + HOP_K_CHANNELS * idx ) & 0xFF;
    //srand( curch );
    __next_rand = curch;
    //RNDL = curch; RNDL = 0xFF - curch; 
    for( idx = 0; idx < HOP_N_CHANNELS; idx++ ) {
        //idx1 = rand() % HOP_N_CHANNELS; 

        __next_rand = __next_rand * 1103515245 + 12345;
        idx1 = ( (unsigned int) (__next_rand >> 16) & RAND_MAX ) % HOP_N_CHANNELS;
      
        //ADCCON1 &= 0xF3; ADCCON1 |= 4; idx1 = RNDL % HOP_N_CHANNELS;
        swp = hopping_channels[ idx ];  hopping_channels[ idx ] = hopping_channels[ idx1 ]; hopping_channels[ idx1 ] = swp;
    }
}
void hop(void){
    BYTE idx;//, state; //BOOL loopFlag;
    idx = HOP_N_CHANNELS; while( idx-- ) if( CHANNR == hopping_channels[ idx ] )  break;
    idx = (idx + 1) % HOP_N_CHANNELS;
    //state = MARCSTATE;
    /* ???
    do{
        loopFlag  = FALSE;
        switch( MARCSTATE ){
            case STARTCAL:  case ENDCAL:
            case TX: case TX_END: 
            case RX_END:
                 halWait( 1 ); 
                 loopFlag  = TRUE;
            break;
        }
    }while( loopFlag );
    */
    RFST   = STROBE_IDLE;               
    CHANNR = hopping_channels[ idx ];
    //if( state == RX ) RFST = STROBE_RX; 
}
