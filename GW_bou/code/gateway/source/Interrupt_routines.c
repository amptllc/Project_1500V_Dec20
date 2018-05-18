#include "hal_main.h"
#include "main.h"

SchedulerInterface __xdata *si = (SchedulerInterface __xdata *)EXCHANGE_BUFFER;

BYTE stage = 0, ticks2FullPower = 0;
void  usb          ( BYTE *s );
/*==== INTERRUPT SERVICE ROUTINES ============================================*/
/******************************************************************************
* @fn  rf_IRQ
* @brief
*      The only interrupt flag which throws this interrupt is the IRQ_DONE interrupt.
*      So this is the code which runs after a packet has been received or transmitted.
* Parameters:
* @param  void
* @return void
******************************************************************************/
//#define NSteps          12
//#define DecStep         10
//#define ParkingChannel  87
//static BYTE stored_channel = 0, stored_fscal_1, stored_fscal_2, stored_fscal_3;
#pragma optimize=s 9
void flagWork(){
    if( si->radioMode == RADIO_MODE_RX){     
        si->flags[ PackageReceived ]  = TRUE;  
        si->timeR = si->runningTick;
    }else if( si->radioMode == RADIO_MODE_TX ){
        //stored_channel = CHANNR;
        //stored_fscal_1 = FSCAL1;    stored_fscal_2 = FSCAL2;    stored_fscal_3 = FSCAL3;
        //if(CHANNR > ParkingChannel) CHANNR = stored_channel - ( 7 + (stored_channel%7) );  
        //else                        CHANNR = stored_channel + ( 7 - (stored_channel%7) ); 
        //stage = NSteps; 
        //ticks2FullPower = 2; 

        si->ticks[ Start2Receive ] = 4*TICKS_IN_MS;
        DMAARM |= 0x80 | DMAARM_CHANNEL1;
        si->flags[ DataSent ] = TRUE;
    }
    si->radioMode = RADIO_MODE_UNDEF;
}
#pragma vector=RF_VECTOR 
//#pragma register_bank=1
__interrupt void rf_IRQ(void) {
    INT_GLOBAL_ENABLE( INT_OFF );
        RFIF &= ~IRQ_DONE;        // Tx/Rx completed, clear interrupt flag
        S1CON &= ~0x03;           // Clear the general RFIF interrupt registers
        DMAARM = 0x80 | DMAARM_CHANNEL1 | DMAARM_CHANNEL0;
        flagWork();
        RFST = STROBE_IDLE;
    INT_GLOBAL_ENABLE( INT_ON );
}
#pragma vector=ADC_VECTOR 
__interrupt void adc_IRQ(void) {
//    INT_GLOBAL_ENABLE( INT_OFF );
        ADCIF &= ~IRQ_DONE;     // clear interrupt flag
//    INT_GLOBAL_ENABLE( INT_ON );
        (*(si->readNextValue))();
}
#pragma vector=T1_VECTOR 
__interrupt void t1_IRQ(void) {
register BYTE _idx = 0;
register INT16 __xdata *ticks = si->ticks;
     TIMIF &= ~IRQ_DONE;     // clear interrupt flag
     si->runningTick++;
     do {
         if( ( *ticks  > 0 ) && ( --(*ticks) == 0 ) ){  
             if( _idx == Ms ){ *ticks = TICKS_IN_MS; (si->ms)++; if(si->ms>=1000){ si->utc++; si->ms %= 1000; } }
             else              *ticks = -1; 
             if( _idx == SendRequest ) si->runningTick = 0;
         }
         ticks++;
    }while( ++_idx < N_Of_Ticks );
    /*
    if( ticks2FullPower && (--ticks2FullPower==0) ){
        if( stage > 6 ){ 
            stage--;  
            if( CHANNR > si->parking_channels[ 3 ] ) CHANNR = CHANNR - DecStep;  else  stage = 5;  
            ticks2FullPower = 1; 
        }else if( stage > 2 ){
            while( CHANNR < si->parking_channels[ stage - 3 ] && stage > 2 ) stage--;
            if( CHANNR > si->parking_channels[ stage - 3 ] && stage > 2 ){
                CHANNR = si->parking_channels[ stage - 3 ];  
                FSCAL1 = si->fscal_parking_1[stage-3];  FSCAL2 = si->fscal_parking_2[stage-3];  FSCAL3 = si->fscal_parking_3[stage-3];
                stage--;
            }
            ticks2FullPower = 1;
        }else if( stage == 2 ){ stage--; RFST = STROBE_IDLE; ticks2FullPower = 1; }
        else  if( stage == 1 ){ 
            stage--;
            CHANNR = stored_channel;   FSCAL1 = stored_fscal_1;   FSCAL2 = stored_fscal_2;     FSCAL3 = stored_fscal_3;
        } 
    }
    */
}
//______________________________________________________________________________________________________________________________
