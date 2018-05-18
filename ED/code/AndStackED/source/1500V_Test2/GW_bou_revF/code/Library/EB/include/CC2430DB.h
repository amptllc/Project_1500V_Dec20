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

Filename:     CC2430DB.h
Target:       cc2430, cc2431
Author:       KJA
Revised:      26/10-2005
Revision:     0.1

Description:
Commonly used macros and function prototypes for use with CC2430DB.

* For use of CC2430DB add "CC2430DB" to project options.
  Project -> Options... -> C compiler -> Preprocessor -> Defined symbols

Functions decleared in this header is implemented in RF04Dev.c.

See RF04Dev.c for function description
******************************************************************************/

#ifndef CC2430DB_H
#define CC2430DB_H

#if(chip == 2430)
#include "ioCC2430.h"
#endif
#if(chip == 2431)
#include "ioCC2431.h"
#endif

#include "hal.h"


/******************************************************************************
* VDD control
*
******************************************************************************/
#define VDD_CTRL                    P1_2
#define ENABLE_DB_PERIPHERALS()     do { P1DIR |= 0x04; VDD_CTRL = 0; } while (0)
#define DISABLE_DB_PERIPHERALS()    (VDD_CTRL = 1)


/******************************************************************************
* Button S1
*
******************************************************************************/
#define BUTTON_PUSH         P0_1
#define BUTTON_PRESSED()    (!BUTTON_PUSH)
#define INIT_BUTTON()       (P0DIR &= ~0x02)

BOOL buttonPushed( void );


/******************************************************************************
* Joystick
*
******************************************************************************/
#define JOYSTICK_PUSH         P2_0
#define JOYSTICK_PRESSED()    JOYSTICK_PUSH
#define INIT_JOYSTICK_PUSH() \
    do {                     \
        P2DIR &= ~0x01;      \
        P2INP |= 0x01;       \
    } while (0)

BOOL joystickPushed( void );

typedef enum {CENTRED, LEFT, RIGHT, UP, DOWN} JOYSTICK_DIRECTION;

#define JOYSTICK              P0_6
#define INIT_JOYSTICK()       IO_DIR_PORT_PIN(0, 6, IO_IN)
#define ADC_INPUT_JOYSTICK    0x06

JOYSTICK_DIRECTION getJoystickDirection( void );


/******************************************************************************
* LED
*
* LED1 = GLED (green)
* LED1 = RLED (red)
******************************************************************************/
#define LED_OFF 1
#define LED_ON  0

#define LED1          P1_0
#define LED2          P1_1

#define GLED          LED1
#define RLED          LED2

#define INIT_LED1()   do { LED1 = LED_OFF; IO_DIR_PORT_PIN(1, 0, IO_OUT); P1SEL &= ~0x01;} while (0)
#define INIT_LED2()   do { LED2 = LED_OFF; IO_DIR_PORT_PIN(1, 1, IO_OUT); P1SEL &= ~0x02;} while (0)

#define INIT_GLED()   INIT_LED1()
#define INIT_RLED()   INIT_LED2()


#define SET_LED1()  (LED1 = LED_ON)
#define SET_LED2()  (LED2 = LED_ON)

#define SET_GLED()  (GLED = LED_ON)
#define SET_RLED()  (RLED = LED_ON)


#define CLR_LED1()  (LED1 = LED_OFF)
#define CLR_LED2()  (LED2 = LED_OFF)

#define CLR_GLED()  (GLED = LED_OFF)
#define CLR_RLED()  (RLED = LED_OFF)

#define SET_LED_MASK( n )                            \
    do {                                             \
        if ((n) & 0x01) SET_LED1(); else CLR_LED1(); \
        if ((n) & 0x02) SET_LED2(); else CLR_LED2(); \
    } while (0)


/******************************************************************************
* EEPROM
*
* See eeprom.h for eeprom fuctions
******************************************************************************/
#define EE_SDA         P0_2
#define EE_SCL         P0_3


/******************************************************************************
* Accelerometer
*
******************************************************************************/
#define ACC_X    P0_4
#define ADC_INPUT_ACC_X  0x04

#define ACC_Y    P0_5
#define ADC_INPUT_ACC_Y  0x05

UINT8 getXacceleration( void );
UINT8 getYacceleration( void );


/******************************************************************************
* Potentiometer
*
******************************************************************************/
#define POT             P0_7
#define INIT_POT()      IO_DIR_PORT_PIN(0, 7, IO_IN);
#define ADC_INPUT_POT   0x07

UINT8 getPotValue( void );


/******************************************************************************
* Light dependent resistor (LDR)
*
******************************************************************************/
#define LDR               P0_0
#define INIT_LDR()        IO_DIR_PORT_PIN(0, 0, IO_IN)
#define ADC_INPUT_LDR     0x00

UINT8 getLdrValue( void );

#endif
