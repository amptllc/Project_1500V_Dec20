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

Filename:     lcd.h
Target:       cc2430, cc2431, cc1110, cc2510
Author:       KJA
Revised:      26/10-2005
Revision:     0.1

Description:
Function declarations for common LCD functions for use with the SmartRF04EB.

All functions defined here are implemented in lcd.c.

******************************************************************************/

#ifndef LCD_H
#define LCD_H


#include "RF04EB.h"
//#include "hal.h"


#define LINE_SIZE         16    // Line length of LCD
#define LINE1_ADDR      0x80    // Upper line of LCD
#define LINE2_ADDR      0xC0    // Lower line of LCD

#define CHAR1_ADDRESS   0x08

#define LINE1              0
#define LINE2              1

#define CLEARLY_VISIBLE    1
#define BEARLY_VISIBLE     0

//symbol codes
#define ARROW_LEFT      0x10
#define ARROW_RIGHT     0x11
#define ARROW_UP        0x12
#define ARROW_DOWN      0x13


/******************************************************************************
* @fn  initLcd
*
* @brief       Setup I/O, configure display and clear LCD.
*
* Parameters:
*
* @param  void
*
* @return void
*
******************************************************************************/
void initLcd(void);


/******************************************************************************
* @fn  lcdUpdate
*
* @brief
*      This function converts the two text strings from ASCII to the character
*      set used by the LCD display.
*
* Parameters:
*
* @param  char*	 pLine1
*                  Pointer to text string to be written to line 1
* @param  char*	 pLine2
*                  Pointer to text string to be written to line 2
*
* @return void
*
******************************************************************************/
void lcdUpdate(char *pLine1, char *pLine2);


/******************************************************************************
* @fn  lcdUpdateLine
*
* @brief       Write one line of text to LCD.
*
* Parameters:
*
* @param  UINT8	 line
*                  LINE1 or LINE2
* @param  char*	 line_p
*                  pointer to text to be written to _line_
*
* @return void
*
******************************************************************************/
void lcdUpdateLine(UINT8 line, char *line_p);


/******************************************************************************
* @fn  lcdUpdateChar
*
* @brief       Write a single character to LCD.
*
* Parameters:
*
* @param  UINT8	 line
*                  LINE1 or LINE2
* @param  UINT8	 position
*                  position to update, range between 0 and 15
* @param  char	    c
*                  character to write, convert an ascii value to corresponding
*                  symbol on LCD
*
* @return void
*
******************************************************************************/
void lcdUpdateChar(UINT8 line, UINT8 position, char c);


/******************************************************************************
* @fn  lcdUpdateSymbol
*
* @brief       Write a single symbol to LCD.
*
* Parameters:
*
* @param  UINT8	 line
*                  LINE1 or LINE2
* @param  UINT8	 position
*                  position to update, between 0 and 15
* @param  char	    c
*                  symbol to write, do not convert an ascii value to
*                  corresponding symbol on LCD
*
* @return void
*
******************************************************************************/
void lcdUpdateSymbol(UINT8 line, UINT8 position, char c);


/******************************************************************************
* @fn  initNewSymbol
*
* @brief       initialize a new symbol on the specified address in LCD RAM
*
* Parameters:
*
* @param  char*	 symbol
*                  Symbol to intialize
* @param  BYTE	 address
*                  address to store the symbol
*
* @return void
*
******************************************************************************/
void initNewSymbol(char* symbol, BYTE address);


/******************************************************************************
* @fn  waitVisible
*
* @brief       Wait for characters to become visible. 
*
* Parameters:
*
* @param  BOOL	 visibility
*	  CLEARLY_VISIBLE   
*	  BEARLY_VISIBLE   
*
* @return void
*
******************************************************************************/
void waitVisible(BOOL visibility);


/******************************************************************************
* @fn  scrollText
*
* @brief       Scroll a string over lower line of LCD.
*
* Parameters:
*
* @param  char*	 string
*                  Text to show
* @param  UINT8	 length
*                  Length of _string_
*
* @return void
*
******************************************************************************/
void scrollText(char *string, UINT8 length);


#endif
