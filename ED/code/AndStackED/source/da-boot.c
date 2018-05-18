/*==== DECLARATION CONTROL ===================================================*/
/*==== INCLUDES ==============================================================*/

#include "hal_main.h"
#include "main.h"

/******************************************************************************
* @fn  main
* @brief
*      Main function. Triggers setup menus and main loops for both receiver
*      and transmitter. This function supports both CC1110 and CC2510.
* Parameters:
* @param  void
* @return void
******************************************************************************/

//void mymemset( BYTE *buff,        BYTE what,    BYTE counter ){ while( counter-- ) *buff++ = what; }
typedef struct {  void (__near_func * entry_point)(void); } pblock;
#pragma optimize=z 9
void main(void){
pblock __xdata  *pb = (pblock __xdata  *)0x7CD4;
BYTE   __xdata *dst = (BYTE __xdata *)0xF500;
BYTE cnt = 0xFF;
    WDCTL = 0x8; 
    // Select frequency and data rate from LCD menu, then configure the radio
    SET_WORD(T1CC0H, T1CC0L, 600 - 1);
    //SET_WORD( T1CC0H, T1CC0L, 300 - 1);
    //        prescaler = 8      modulo mode            ie
    T1CTL   = 0x04             | 0x02;                  T1CCTL0 = 0x44;
    //mymemset( (BYTE *) 0xF500, 0, 0xFF );
    while( cnt-- ) *dst++ = 0;
    // long jump to code, 1k or 16k ...
    (*(pb->entry_point))();
}
/*==== END OF FILE ==========================================================*/
