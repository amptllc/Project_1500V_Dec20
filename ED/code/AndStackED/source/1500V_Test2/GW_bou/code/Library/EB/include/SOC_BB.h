/*
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

Filename:     SOC_BB.h
Target:       cc2430, cc2431
Author:       KJA
Revised:      23/12-2005
Revision:     0.1

Description:
Commonly used macros and function prototypes for use with SOC Battery Board.

* For use of SOC_BB add "SOC_BB" to project options.
  Project -> Options... -> C compiler -> Preprocessor -> Defined symbols

Functions decleared in this header is implemented in RF04Dev.c.

See RF04Dev.c for function description
******************************************************************************/

#ifndef SOC_BB_H
#define SOC_BB_H

#if(chip == 2430)
#include "ioCC2430.h"
#endif
#if(chip == 2431)
#include "ioCC2431.h"
#endif

#include "hal.h"


/******************************************************************************
* Button S1
*
******************************************************************************/
#define BUTTON_PUSH         P0_1
#define BUTTON_PRESSED()    (!BUTTON_PUSH)
#define INIT_BUTTON()       (P0DIR &= ~0x02)

BOOL buttonPushed( void );



/******************************************************************************
* LED
*
* LED = GLED (green)
******************************************************************************/
#define LED_OFF 1
#define LED_ON  0

#define LED           P1_0

#define RLED          LED1

#define INIT_LED()    do { LED = LED_OFF; IO_DIR_PORT_PIN(1, 0, IO_OUT); P1SEL &= ~0x01;} while (0)
#define INIT_RLED()   INIT_LED()

#define SET_LED()   (LED  = LED_ON)
#define SET_RLED()  (RLED = LED_ON)

#define CLR_LED()   (LED  = LED_OFF)
#define CLR_RLED()  (RLED = LED_OFF)

#define SET_LED_MASK( n )                            \
    do {                                             \
        if ((n) & 0x01) SET_LED(); else CLR_LED();   \
    } while (0)


/******************************************************************************
* UART
*
******************************************************************************/
#define UART_RX          P1_6
#define UART_TX          P1_7

#endif
