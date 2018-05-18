/*-----------------------------------------------------------------------------
|   File:      hal_main.h
|   Target:    cc1110, cc2510
|   Author:    TFL
|   Revised:   2007-09-05
|   Revision:  1.0
+------------------------------------------------------------------------------
|  Copyright 2004-2007 Texas Instruments Incorporated. All rights reserved.
|
|  IMPORTANT: Your use of this Software is limited to those specific rights
|  granted under the terms of a software license agreement between the user who
|  downloaded the software, his/her employer (which must be your employer) and
|  Texas Instruments Incorporated (the "License"). You may not use this Software
|  unless you agree to abide by the terms of the License. The License limits
|  your use, and you acknowledge, that the Software may not be modified, copied
|  or distributed unless embedded on a Texas Instruments microcontroller or used
|  solely and exclusively in conjunction with a Texas Instruments radio
|  frequency transceiver, which is integrated into your product. Other than for
|  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
|  works of, modify, distribute, perform, display or sell this Software and/or
|  its documentation for any purpose.
|
|  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
|  PROVIDED “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
|  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
|  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
|  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
|  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
|  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES INCLUDING
|  BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE OR
|  CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT OF
|  SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
|  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.
|
|  Should you have any questions regarding your right to use this Software,
|  contact Texas Instruments Incorporated at www.TI.com.
|
+------------------------------------------------------------------------------
| Purpose:    Main header file for the hardware abstraction layer
+------------------------------------------------------------------------------
| Decription: This file is the main header for Hardware Abstraction Layer
+----------------------------------------------------------------------------*/

/*==== DECLARATION CONTROL ==================================================*/

#ifndef HAL_MAIN_H
#define HAL_MAIN_H


/*==== INCLUDES ==============================================================*/

// HAL chip specific includes
#include "hal_defines.h"
#include "hal_io_mgmt.h"
#include "hal_power_clk_mgmt.h"
#include "hal_interrupt_mgmt.h"
#include "hal_adc_mgmt.h"

#if(chip == 1110)
#include "ioCC1110.h"
#endif
#if(chip == 2510)
#include "ioCC2510.h"
#endif
#if(chip == 2511)
#include "ioCC2511.h"
#endif

#define CHIPREVISION VERSION

// Typedef void pointer
typedef void (*VFPTR)(void);

// HAL board user interface specific includes
#include "hal_bui_SmartRF04EB.h"


#if(chip == 1110 || chip == 1111 || chip == 2510 || chip == 2511)
#define INT_ENABLE(inum, on)                                                    \
   do {                                                                         \
      if      (inum==INUM_RFTXRX) { RFTXRXIE = on; }                            \
      else if (inum==INUM_ADC)    { ADCIE   = on;  }                            \
      else if (inum==INUM_URX0)   { URX0IE  = on;  }                            \
      else if (inum==INUM_URX1)   { URX1IE  = on;  }                            \
      else if (inum==INUM_ENC)    { ENCIE   = on;  }                            \
      else if (inum==INUM_ST)     { STIE    = on;  }                            \
      else if (inum==INUM_P2INT)  { (on) ? (IEN2 |= 0x02) : (IEN2 &= ~0x02); }  \
      else if (inum==INUM_UTX0)   { (on) ? (IEN2 |= 0x04) : (IEN2 &= ~0x04); }  \
      else if (inum==INUM_DMA)    { DMAIE   = on;  }                            \
      else if (inum==INUM_T1)     { T1IE    = on;  }                            \
      else if (inum==INUM_T2)     { T2IE    = on;  }                            \
      else if (inum==INUM_T3)     { T3IE    = on;  }                            \
      else if (inum==INUM_T4)     { T4IE    = on;  }                            \
      else if (inum==INUM_P0INT)  { P0IE    = on;  }                            \
      else if (inum==INUM_UTX1)   { (on) ? (IEN2 |= 0x08) : (IEN2 &= ~0x08); }  \
      else if (inum==INUM_P1INT)  { (on) ? (IEN2 |= 0x10) : (IEN2 &= ~0x10); }  \
      else if (inum==INUM_RF)     { (on) ? (IEN2 |= 0x01) : (IEN2 &= ~0x01); }  \
      else if (inum==INUM_WDT)    { (on) ? (IEN2 |= 0x20) : (IEN2 &= ~0x20); }  \
   } while (0)
#endif

#if(chip == 1110 || chip == 1111 || chip == 2510 || chip == 2511)
#define INT_GETFLAG(inum) (                       \
   (inum==INUM_RFTXRX)      ? RFTXRXIF          : \
   (inum==INUM_ADC)         ? ADCIF             : \
   (inum==INUM_URX0)        ? URX0IF            : \
   (inum==INUM_URX1)        ? URX1IF            : \
   (inum==INUM_ENC)         ? ENCIF_0           : \
   (inum==INUM_ST)          ? STIF              : \
   (inum==INUM_P2INT)       ? P2IF              : \
   (inum==INUM_UTX0)        ? UTX0IF            : \
   (inum==INUM_DMA)         ? DMAIF             : \
   (inum==INUM_T1)          ? T1IF              : \
   (inum==INUM_T2)          ? T2IF              : \
   (inum==INUM_T3)          ? T3IF              : \
   (inum==INUM_T4)          ? T4IF              : \
   (inum==INUM_P0INT)       ? P0IF              : \
   (inum==INUM_UTX1)        ? UTX1IF            : \
   (inum==INUM_P1INT)       ? P1IF              : \
   (inum==INUM_RF)          ? S1CON &= ~0x03    : \
   (inum==INUM_WDT)         ? WDTIF             : \
   0                                              \
)
#endif

#if(chip == 1110 || chip == 1111 || chip == 2510 || chip == 2511)
#define INT_SETFLAG(inum, f)                                                    \
   do {                                                                         \
      if      (inum==INUM_RFTXRX){ RFTXRXIF = f; }                              \
      else if (inum==INUM_ADC)   { ADCIF  = f; }                                \
      else if (inum==INUM_URX0)  { URX0IF = f; }                                \
      else if (inum==INUM_URX1)  { URX1IF = f; }                                \
      else if (inum==INUM_ENC)   { ENCIF_1 = ENCIF_0 = f; }                     \
      else if (inum==INUM_ST)    { STIF  = f;  }                                \
      else if (inum==INUM_P2INT) { P2IF  = f;  }                                \
      else if (inum==INUM_UTX0)  { UTX0IF= f;  }                                \
      else if (inum==INUM_DMA)   { DMAIF = f;  }                                \
      else if (inum==INUM_T1)    { T1IF  = f;  }                                \
      else if (inum==INUM_T2)    { T2IF  = f;  }                                \
      else if (inum==INUM_T3)    { T3IF  = f;  }                                \
      else if (inum==INUM_T4)    { T4IF  = f;  }                                \
      else if (inum==INUM_P0INT) { P0IF  = f;  }                                \
      else if (inum==INUM_UTX1)  { UTX1IF= f;  }                                \
      else if (inum==INUM_P1INT) { P1IF  = f;  }                                \
      else if (inum==INUM_RF)    { (f) ? (S1CON |= 0x03) : (S1CON &= ~0x03); }  \
      else if (inum==INUM_WDT)   { WDTIF = f;  }                                \
   } while (0)
#endif


/*==== CONSTS ================================================================*/
/*==== TYPES =================================================================*/
/*==== EXPORTS ===============================================================*/

/******************************************************************************
* @fn  halWait
*
* @brief
*      This function waits approximately a given number of m-seconds
*      regardless of main clock speed. (Implementation in hal_wait.c)
*
* Parameters:
*
* @param  BYTE	 wait
*         The number of m-seconds to wait.
*
* @return void
*
******************************************************************************/
void halWait(BYTE wait);

#endif /* HAL_MAIN_H */

/*==== END OF FILE ==========================================================*/










