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

Filename:     RF04EB.h
Target:       cc2511
Author:       OLS
Revised:      31/03-2005
Revision:     0.1

Description:
Macros for the CC2511 USB Dongle

******************************************************************************/

#ifndef CC2511DONGLE_H
#define CC2511DONGLE_H

#include "ioCC2511.h"
#include "hal.h"

/******************************************************************************
* Button
*
******************************************************************************/
#define BUTTON_PRESSED()    (!P1_2)
#define INIT_BUTTON()       do { P1DIR &= ~0x04; P1SEL &= ~0x04; } while (0)



/******************************************************************************
* LED
*
******************************************************************************/
#define LED          P1_1

#define INIT_LED()      do { P1SEL &= ~0x02; P1DIR |= 0x02; LED = 0; } while (0)

#define SET_LED()  (LED = 1)

#define CLR_LED()  (LED = 0)

/******************************************************************************
* D+ Pull-up
*
******************************************************************************/
#define USB_CONNECT()          do { P1_0 = 1; P1DIR |= 0x01; } while (0)

#define USB_DISCONNECT()       (P1DIR &= ~0x01)

#endif
