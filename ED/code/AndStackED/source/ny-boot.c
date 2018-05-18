/*==== DECLARATION CONTROL ===================================================*/
/*==== INCLUDES ==============================================================*/

#include "hal_main.h"
#include "main.h"
#include "timing.h"

SchedulerInterface __xdata *si = (SchedulerInterface __xdata *)0xF500;
//INT16 __xdata  *ticks = (INT16 __xdata *)0xF500; 
//BOOL  __xdata  *flags = (BOOL __xdata  *)0xF520;                         // 16 0xF520

//#define MKS_IN_TICK     500
//#define TICKS_IN_SEC    2000
//#define TICKS_IN_MS     2

//#define _mppCycle            120 
//#define _firstAdcMeasurement ( TICKS_IN_SEC / 50 )

void call_alt_image(   void );
void inc_running_tick( void );
void mpp_work(         void );
//void main_asm(         void );
//void mymemcpy( BYTE *destination, BYTE *source, BYTE counter ){ while( counter-- ) *destination++ = *source++; }
/******************************************************************************
* @fn  main
* @brief
*      Main function. Triggers setup menus and main loops for both receiver
*      and transmitter. This function supports both CC1110 and CC2510.
* Parameters:
* @param  void
* @return void
******************************************************************************/
//void main(void){ 
  //CLKCON |= OSC_BIT;                    // starting the RC Oscillator
  //asm("NOP");
  // //while(!HIGH_FREQUENCY_RC_OSC_STABLE && counter-- ) halWait(2); // waiting until the oscillator is stable
  //SLEEP |= OSC_PD_BIT;                  // powering down the unused oscillator
  //*((BYTE *)0xFFFF) = 0x10;  
  //main_asm(); 
//}  // was 0x10
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
//#pragma optimize=s 9
// LBC flagWork determines if radio package received or transmitted,
// LBC and then takes appropriate action.
// LBC called from rf_IRQ
void flagWork(){
    //if( si->radioMode == RADIO_MODE_RX ){     
    if( *((BYTE *)0xF53A) == RADIO_MODE_RX ){
        //si->flags[ PackageReceived ]  = TRUE;
        *((BYTE *)0xF521) = 1;
        //si->packetReceived =si->runningTick;
        //*((BYTE *)0xF55F) = *((BYTE *)0xF538);
        //*((BYTE *)0xF560) = *((BYTE *)0xF539);
        *((BYTE *)0xF538) = 0;
        //P1_1 = 1;
    //}else if( si->radioMode == RADIO_MODE_TX ){
    }else if( *((BYTE *)0xF53A) == RADIO_MODE_TX ){
        //si->ticks[ CheckFlag ] = 0;
        //*((BYTE *)0xF50E) = 0;  *((BYTE *)0xF50F) = 0;
        //si->flags[ DataSent  ] = TRUE;
        *((BYTE *)0xF522) = 1;
        //P1_4 = 1;
    }
    //si->flags[ IncreaseSleepCnt ] = TRUE;
    //si->radioMode = RADIO_MODE_UNDEF;
    *((BYTE *)0xF53A) = RADIO_MODE_UNDEF;
}

#pragma vector=RF_VECTOR 
//#pragma register_bank=1
__interrupt void rf_IRQ(void) {
    INT_GLOBAL_ENABLE( INT_OFF );
        RFIF &= ~IRQ_DONE;        // Tx/Rx completed, clear interrupt flag
        S1CON &= ~0x03;           // Clear the general RFIF interrupt registers
        DMAARM = 0x80 | DMAARM_CHANNEL1 | DMAARM_CHANNEL0;
        flagWork();        
        //RFST = STROBE_IDLE;
        //if( si->rfCallback ) si->rfCallback();
    INT_GLOBAL_ENABLE( INT_ON );
}
#define read_next_value ((void (*)(void)) (*((UINT16 *)0xF532)))
#pragma vector=ADC_VECTOR 
__interrupt void adc_IRQ(void) {
        ADCIF &= ~IRQ_DONE;     // clear interrupt flag
        //(*(si->readNextValue))();
        read_next_value();
}

#define arm_adcs ((void (*)(void)) (*((UINT16 *)0xF530)))
//BYTE firstMppCycle = 1;
#pragma vector=T1_VECTOR 
__interrupt void t1_IRQ(void) {
register BYTE _idx = 0;
register INT16 __xdata *ticks = (INT16 *)0xF500; //si->ticks;
    TIMIF &= ~IRQ_DONE;     // clear interrupt flag
    //inc_running_tick();
    //P1_4 ^= 1;
    (*((BYTE *)0xF538))++; // inc runnning tick
    if( (*((BYTE *)0xF55B)) == 0 ) (*((INT16 *)0xF55B)) = _mppCycle;
    if( ticks[Ms] == 0 )           ticks[Ms] = TICKS_IN_SEC;
    if( *ticks==0 ) mpp_work();
    do {
        if( ( *ticks  > 0 ) && ( --(*ticks) == 0 ) )
            switch( _idx ){
                case MppCycle: 
                  mpp_work(); 
                  //if( *((BYTE *)0xFFFF) ) (*((BYTE *)0xFFFF))--; 
                  //else call_alt_image(); 
                break;
                case AdcMeasurement: 
                    //if( si->armAdcs ) (*(si->armAdcs))();
                    if( *((INT16 *)0xF530) ) arm_adcs();
                    //else call_alt_image(); 
                break;
                default: *ticks = -1; break;
            }
        ticks++;
    } while( ++_idx < N_Of_Ticks );
}

#define flashCallback ((void (*)(void)) (*((UINT16 *)0xF534)))
#pragma vector=DMA_VECTOR
__interrupt void dmaFlash_IRQ(void) {
    DMAIF &= ~IRQ_DONE;
    //if( DMAIRQ & 0x4 ) (*(si->flashCallback))();
    if( DMAIRQ & 0x4 ) flashCallback(); 
}
//______________________________________________________________________________________________________________________________
/*==== END OF FILE ==========================================================*/
