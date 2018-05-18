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
Target:       cc2430, cc2431, cc1110, cc2510
Author:       KJA
Revised:      26/10-2005
Revision:     0.1

Description:
Commonly used macros and function prototypes for use with RF04EB.
Functions decleared in this header is implemented in RF04Dev.c.

See RF04Dev.c for function description
******************************************************************************/

#ifndef RF04EB_H
#define RF04EB_H

#if(chip == 2430)
#include "ioCC2430.h"
#endif
#if(chip == 2431)
#include "ioCC2431.h"
#endif
#if(chip == 1110)
#include "ioCC1110.h"
#endif
#if(chip == 2510)
#include "ioCC2510.h"
#endif
#if(chip == 2511)
#include "ioCC2510.h"
#endif
//#include "hal.h"

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
#define JOYSTICK_PUSH         P0_5
#define JOYSTICK_PRESSED()    JOYSTICK_PUSH
#define INIT_JOYSTICK_PUSH() \
    do {                     \
        P0DIR &= ~0x20;      \
        P0INP |= 0x20;       \
    } while (0)

BOOL joystickPushed( void );

//typedef enum {CENTRED, LEFT, RIGHT, UP, DOWN} JOYSTICK_DIRECTION;

#define JOYSTICK              P0_6
#define INIT_JOYSTICK()       IO_DIR_PORT_PIN(0, 6, IO_IN)
#define ADC_INPUT_JOYSTICK    0x06

//JOYSTICK_DIRECTION getJoystickDirection( void );

/******************************************************************************
* LED
*
* LED1 = GLED (green)
* LED2 = RLED (red)
* LED3 = YLED (yellow)
* LED4 = BLED (blue)
*
#if (chip == 2430 || chip == 2431)
* LED2 and LED4 are not connected to CC2430
#endif
******************************************************************************/
#define LED_OFF 1
#define LED_ON  0


#define LED1          P1_0
#if (chip == 0000)
#define LED2          P1_2
#endif
#define LED3          P1_3
#if (chip == 0000)
#define LED4          P2_0
#endif

#define GLED          LED1
#if (chip == 0000)
#define RLED          LED2
#endif
#define YLED          LED3
#if (chip == 0000)
#define BLED          LED4
#endif

#define INIT_LED1()      do { LED1 = LED_OFF; IO_DIR_PORT_PIN(1, 0, IO_OUT); P1SEL &= ~0x01;} while (0)
#if (chip == 0000)
#define INIT_LED2()      do { LED2 = LED_OFF; IO_DIR_PORT_PIN(1, 2, IO_OUT); P1SEL &= ~0x04;} while (0)
#endif
#define INIT_LED3()      do { LED3 = LED_OFF; IO_DIR_PORT_PIN(1, 3, IO_OUT); P1SEL &= ~0x08;} while (0)
#if (chip == 0000)
#define INIT_LED4()      do { LED4 = LED_OFF; IO_DIR_PORT_PIN(2, 0, IO_OUT); P2SEL &= ~0x01;} while (0)
#endif

#define INIT_GLED()      INIT_LED1()
#if (chip == 0000)
#define INIT_RLED()      INIT_LED2()
#endif
#define INIT_YLED()      INIT_LED3()
#if (chip == 0000)
#define INIT_BLED()      INIT_LED4()
#endif


#define SET_LED1()  (LED1 = LED_ON)
#if (chip == 0000)
#define SET_LED2()  (LED2 = LED_ON)
#endif
#define SET_LED3()  (LED3 = LED_ON)
#if (chip == 0000)
#define SET_LED4()  (LED4 = LED_ON)
#endif

#define SET_GLED()  (GLED = LED_ON)
#if (chip == 0000)
#define SET_RLED()  (RLED = LED_ON)
#endif
#define SET_YLED()  (YLED = LED_ON)
#if (chip == 0000)
#define SET_BLED()  (BLED = LED_ON)
#endif


#define CLR_LED1()  (LED1 = LED_OFF)
#if (chip == 0000)
#define CLR_LED2()  (LED2 = LED_OFF)
#endif
#define CLR_LED3()  (LED3 = LED_OFF)
#if (chip == 0000)
#define CLR_LED4()  (LED4 = LED_OFF)
#endif

#define CLR_GLED()  (GLED = LED_OFF)
#if (chip == 0000)
#define CLR_RLED()  (RLED = LED_OFF)
#endif
#define CLR_YLED()  (YLED = LED_OFF)
#if (chip == 0000)
#define CLR_BLED()  (BLED = LED_OFF)
#endif

#if (chip == 0000)
#define SET_LED_MASK( n )                            \
    do {                                             \
        if ((n) & 0x01) SET_LED1(); else CLR_LED1(); \
        if ((n) & 0x02) SET_LED2(); else CLR_LED2(); \
        if ((n) & 0x04) SET_LED3(); else CLR_LED3(); \
        if ((n) & 0x08) SET_LED4(); else CLR_LED4(); \
    } while (0)
#endif

#define SET_LED_MASK( n )                            \
    do {                                             \
        if ((n) & 0x01) SET_LED1(); else CLR_LED1(); \
        if ((n) & 0x04) SET_LED3(); else CLR_LED3(); \
    } while (0)


/******************************************************************************
* LCD
*
* See lcd.h for lcd fuctions
******************************************************************************/
#define LCD_SDA  P1_2
#define LCD_SCL  P2_0


/******************************************************************************
* UART
*
******************************************************************************/
#define UART_RD          P0_2
#define UART_TD          P0_3


/******************************************************************************
* PWM sound
*
******************************************************************************/
#define SOUND_OUT        P1_2
#define SOUND_IN         P0_0


/******************************************************************************
* Potentiometer
*
******************************************************************************/
#define POT             P0_7
#define INIT_POT()      IO_DIR_PORT_PIN(0, 7, IO_IN);
#define ADC_INPUT_POT   0x07

UINT8 getPotValue( void );

#endif
