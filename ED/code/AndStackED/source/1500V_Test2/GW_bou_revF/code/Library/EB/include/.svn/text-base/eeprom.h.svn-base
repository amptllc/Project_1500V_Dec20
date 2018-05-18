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

Filename:     eeprom.h
Target:       cc2430, cc2431
Author:       EFU
Revised:      26/10-2005
Revision:     0.1

Description:
Function declarations for common EEPROM functions for use with the CC2430DB.

All functions defined here are implemented in eeprom.c.

******************************************************************************/
#ifndef EEPROM_H
#define EEPROM_H

#include "hal.h"


/******************************************************************************
* @fn  initEEProm
*
* @brief
*      Initializes the EEPROM. Configures pins EE_SDA and EE_SCL as input.
*
* Parameters:
*
* @param  void
*
* @return void
*
******************************************************************************/
void initEEProm( void );


/******************************************************************************
* @fn  eraseEEProm
*
* @brief
*      Erases the entire EEPROM, by writing 0x00 to all memory locations.
*
* Parameters:
*
* @param  void
*
* @return void
*
******************************************************************************/
void eraseEEProm( void );


/******************************************************************************
* @fn  writeEEProm
*
* @brief
*      This function writes _length_ bytes of data starting at the _buffer_
*      location to the EEPROM address _address_.
*
* Parameters:
*
* @param  BYTE*    buffer
* 	  Pointer to the first byte to be written to EEPROM. _length_ bytes
*	  starting from this address will be written to EEPROM address _address_.
*
* @param  WORD	   address
*	  Target EEPROM address for write operation.
*
* @param  UINT8    length
*	  Number of bytes to be written to EEPROM.
*
* @return BYTE
*         TRUE: Write operation successful.
*         FALSE: Write operation NOT successful.
*
******************************************************************************/
BYTE writeEEProm(BYTE* buffer, WORD address, UINT8 length);


/******************************************************************************
* @fn  readEEProm
*
* @brief
*      Initiates sequential read of EEPROM content from the given address. The
*      read operation transfer _length_ bytes to _buffer_, starting at EEPROM
*      address _address_, ending at _adress_ + _length_.
*
* Parameters:
*
* @param  BYTE*    buffer
*	  Pointer to buffer for storing read values.
*	
* @param  WORD     address
*	  Start address for read operation. Valid range: 0 - 0xFFF.
*
* @param  UINT8    length
*	  Number of bytes to be read.
*
* @return BOOL
*         TRUE: Read operation successful.
*         FALSE: Read operation NOT successful.
*
******************************************************************************/
BOOL readEEProm(BYTE* buffer, WORD address, UINT8 length);

#endif
