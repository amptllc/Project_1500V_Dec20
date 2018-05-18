#include "hal_main.h"
#include "main.h"

SchedulerInterface __xdata *si = (SchedulerInterface __xdata *)0xF500;

#define MKS_IN_TICK     200
#define TICKS_IN_SEC    5000
#define TICKS_IN_MS     5

#define _mppCycle            ( TICKS_IN_SEC / 16 ) 
#define _firstAdcMeasurement ( TICKS_IN_SEC / 50 )

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
#pragma optimize=s 9
void flagWork(){
    if( si->radioMode == RADIO_MODE_RX){     
        si->flags[ PackageReceived ]  = TRUE;  
    }else if( si->radioMode == RADIO_MODE_TX){
        si->ticks[ CheckFlag ] = 0;
        si->flags[ DataSent  ] = TRUE;
    }
    si->flags[ IncreaseSleepCnt ] = TRUE;
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
        //if( si->rfCallback ) si->rfCallback();
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
        if( ( *ticks  > 0 ) && ( --(*ticks) == 0 ) )
            switch( _idx ){
                case MppCycle:
                    P1_0 ^= 1;
                    *ticks = _mppCycle-1;
                    //si->ticks[ AdcMeasurement ] = _firstAdcMeasurement-1;    // 20 ms
                    ticks[ 1 ] = _firstAdcMeasurement-1;                       // 20 ms
                    si->inCurCycle = 0;                                        // 62.5 ms
                break;
                case AdcMeasurement: si->flags[ CheckVin ] = FALSE;   if( /*(_armAdcs!=NULL) && */ si->adcEnabled ) (*(si->armAdcs))();  break;
                default: *ticks = -1; break;
            }
        ticks++;
    } while( ++_idx < N_Of_Ticks );
     si->flags[ IncreaseSleepCnt ] = TRUE;
}
#pragma vector=DMA_VECTOR
__interrupt void dmaFlash_IRQ(void) {
    DMAIF &= ~IRQ_DONE;
    if( DMAIRQ & 0x4 ){
        HAL_INT_ENABLE(INUM_DMA, INT_OFF); 
        (*(si->flashCallback))();
    }
}
//______________________________________________________________________________________________________________________________
