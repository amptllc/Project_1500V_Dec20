#include "hal_main.h"
void main(void){
    // 2 top pins are secial function - timer 3 channels
    P1SEL   = 0xC0;                          
    // pins 7(oc) 6(ov) 4(on/off), 3(mpp), 1(LED), 0(mpp cycle) are for output
    P1DIR   = 0xDB;   // 1101 1011

    T3CTL   = 0x50; // was 0x70, prescaler 1/4, normal operation, mode - free running
    T3CCTL0 = 0x24; // clear output on compare up, set on 0, enabled
    T3CCTL1 = 0x24; // clear output on compare up, set on 0, enabled
    PERCFG  = 0x20; // Timer3 has alternate 2 location - P1_6 & P1_7
    P2SEL   = 0x20; // Timer3 has priority over USART1
    
    T3CC0 = 128; T3CC1 = 64;
    P1_4 = 1;

    { //BYTE counter = 127, i, j;
      SLEEP &= ~OSC_PD_BIT;     // powering down all oscillators
      /*
      while(!XOSC_STABLE && counter-- )       // waiting until the oscillator is stable
          for( i = 0; i<127; i++ )
              for( j = 0; j<127; j++ )
                   P1_1 ^= 1;
      */
      asm("NOP");
      //CLKCON &= ~MAIN_OSC_BITS; // starting the Crystal Oscillator
      CLKCON = 0x80;  // was 0x89  
      SLEEP |= OSC_PD_BIT;      // powering down the unused oscillator
    }
    
    while( TRUE ){ 
        if( PARTNUM != 0x81 ){ WDCTL = 0x8; while( TRUE ) P1_2 ^= 1; }//PARTNUM = 0x81; //break;
        P1_1 ^= 1; 
        //if( CLKCON != 0x80 ) {
          SLEEP &= 0xFC;
          SLEEP &= ~OSC_PD_BIT;     // powering down all oscillators
          asm("NOP");
          //CLKCON &= ~MAIN_OSC_BITS; // starting the Crystal Oscillator
          CLKCON = 0x80;  // was 0x89  
          SLEEP |= OSC_PD_BIT;      // powering down the unused oscillator
        
        //}
    }
}
