#include "hal_main.h"
#include <stdlib.h>

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

BYTE hopping_channels[ HOP_N_CHANNELS ] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24};
void setup_hopper( BYTE curch ){
BYTE idx = 0, idx1 = 0, swp;
    for( idx = 0; idx < HOP_N_CHANNELS; idx++ ) hopping_channels[ idx ] = ( curch + HOP_K_CHANNELS * idx ) & 0xFF;
    srand( curch );
    for( idx = 0; idx < HOP_N_CHANNELS; idx++ ) {
        idx1 = rand() % HOP_N_CHANNELS; 
        swp = hopping_channels[ idx ];  hopping_channels[ idx ] = hopping_channels[ idx1 ]; hopping_channels[ idx1 ] = swp;
    }
}
void hop(void){
    BYTE idx, state; BOOL loopFlag;
    for( idx = 0; idx < HOP_N_CHANNELS; idx ++ ) if( CHANNR == hopping_channels[ idx ] )  break;
    idx = (idx + 1) % HOP_N_CHANNELS;
    state = MARCSTATE;
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
    RFST   = STROBE_IDLE;               
    CHANNR = hopping_channels[ idx ];
    switch( state ){
        case RX: RFST = STROBE_RX; break;  
    }
}
