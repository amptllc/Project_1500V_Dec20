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

Filename:     eeprom.c
Target:       cc2430, cc2431
Author:       EFU
Revised:      26/10-2005
Revision:     0.1

Description:
Function implementations for common EEPROM functions for use with the CC2430DB.

******************************************************************************/


#include "eeprom.h"
#if(chip == 2430)
#include "ioCC2430.h"
#endif
#if(chip == 2431)
#include "ioCC2431.h"
#endif
#include "hal.h"
#include "CC2430DB.h"

// Prototypes for internal EEPROM functions
BYTE writeEEPromBlock(BYTE* buffer, INT8 length, WORD address);
void smbEESend(BYTE *buffer, const UINT8 n);
BOOL smbEESendByte(BYTE b);
void smbEEWrite(BOOL value);
void smbEEReceive(BYTE address, UINT16 length, BYTE *buffer);
BYTE smbEEReceiveByte(BOOL lastByte);
BOOL EEPromWrapCheck(INT8 length, WORD address);
void smbEEClock(BOOL value);
void smbEEStart();
void smbEEStop();
void waitEE();


#define DATA_HIGH()    do{IO_DIR_PORT_PIN(0, 2, IO_IN);} while(0)
#define DATA_LOW()     do{IO_DIR_PORT_PIN(0, 2, IO_OUT); EE_SDA = 0;}while(0)

#define EEPROM_WRITEADDRESS   0xA0
#define EEPROM_READADDRESS    0xA1

#define PAGE_SIZE 32
// 32 k bit = 4096 BYTE
#define PROM_SIZE (0x1000)

#define WAIT_FOR_WRITE_DELAY 0x03


/******************************************************************************
* See eeprom.h for a description of this function.
******************************************************************************/
void initEEProm(void)
{
   // Setting the ports as inputs.
   IO_DIR_PORT_PIN(0, 2, IO_IN);
   IO_DIR_PORT_PIN(0, 3, IO_IN);

   // Setting P0_2 and P0_3 for general IO operation.
   IO_FUNC_PORT_PIN(0, 2, IO_FUNC_GIO);
   IO_FUNC_PORT_PIN(0, 3, IO_FUNC_GIO);

   // Setting ports for pull-up.
   IO_IMODE_PORT_PIN(0, 2, IO_IMODE_PUD);
   IO_IMODE_PORT_PIN(0, 3, IO_IMODE_PUD);
   IO_PUD_PORT(0, IO_PULLUP);
}


/******************************************************************************
* See eeprom.h for a description of this function.
******************************************************************************/
BOOL readEEProm(BYTE* buffer, WORD address, UINT8 length)
{
   BYTE i;
   BYTE transmitBuffer[3];

   transmitBuffer[0] = EEPROM_WRITEADDRESS;
   transmitBuffer[1] = (BYTE) (address >> 8);
   transmitBuffer[2] = (BYTE) (address);

   smbEEStart();
   for(i = 0; i < 3; i++){
      while(!smbEESendByte(transmitBuffer[i])); //Keep sending until ACK received
   }

   smbEEClock(0);
   waitEE();
   DATA_HIGH();
   smbEEClock(1);

   smbEEReceive(EEPROM_READADDRESS, length, buffer);

   return TRUE;
}


/******************************************************************************
* See eeprom.h for a description of this function.
******************************************************************************/
BYTE writeEEProm(BYTE* buffer, WORD address, UINT8 length)
{
   UINT8 i;
   UINT8 currentPageLength;
   UINT8 pageOffset;

   if(((WORD)address + length) > (WORD)PROM_SIZE){
      // Trying to write to a higher address than max address
      return FALSE;
   }

   if( pageOffset = (address & 0x1F) ){
      // Write to first page if address is not 32 bytes aligned
      if( pageOffset + length > PAGE_SIZE ){
         currentPageLength = PAGE_SIZE - pageOffset;
      }
      else{
         currentPageLength = length;
      }

      writeEEPromBlock(&buffer[0], currentPageLength, address);
      // Wait for internal write cycle to finish
      halWait(WAIT_FOR_WRITE_DELAY);
   }
   else{
      // Address is 32 bytes aligned
      pageOffset = 0;
      currentPageLength = 0;
   }

   for( i = currentPageLength; i < length; i += PAGE_SIZE ){
      if( i + PAGE_SIZE < length ) {
         currentPageLength = PAGE_SIZE;
      }
      else{
         currentPageLength = length - i;
      }
      writeEEPromBlock(&buffer[i], currentPageLength, address + i);
      // Wait for internal write cycle to finish
      halWait(WAIT_FOR_WRITE_DELAY);
   }

   return TRUE;
}


/******************************************************************************
* See eeprom.h for a description of this function.
* length must be less or equal 32
******************************************************************************/
BYTE writeEEPromBlock(BYTE* buffer, INT8 length, WORD address)
{
   UINT8 i = 0;
   UINT8 j;
   BYTE transmitBuffer[35];

   transmitBuffer[i++] = EEPROM_WRITEADDRESS;
   transmitBuffer[i++] = (BYTE) (address >> 8);
   transmitBuffer[i++] = (BYTE) (address);

   for(j = 0; j < length; j++)
   {
      transmitBuffer[i++] = buffer[j];
   }

   smbEESend(transmitBuffer, i);
   return TRUE;
}


/******************************************************************************
* See eeprom.h for a description of this function.
******************************************************************************/
void eraseEEProm(void)
{
   WORD j;
   UINT8 i;
   BYTE transmitBuffer[35];

   transmitBuffer[0] = EEPROM_WRITEADDRESS;
   for(i = 0; i < PAGE_SIZE; i++)
   {
      transmitBuffer[i + 3] = 0;
   }

   for(j = 0; j < PROM_SIZE; j += PAGE_SIZE)
   {
      transmitBuffer[1] = (BYTE) (j >> 8);
      transmitBuffer[2] = (BYTE) (j);

      smbEESend(transmitBuffer, 35);
      // Wait for internal write cycle to finish
      halWait(WAIT_FOR_WRITE_DELAY);
   }
}


/******************************************************************************
* Internal function for eeprom.c
******************************************************************************/
void smbEESend(BYTE *buffer, const UINT8 n){
   UINT8 i = 0;
   smbEEStart();
   for(i = 0; i < n; i++){
      while(!smbEESendByte(buffer[i])); //Keep sending until ACK received
   }
   smbEEClock(0);
   smbEEClock(1);
   DATA_LOW();
   smbEEStop();
}


/******************************************************************************
* Internal function for eeprom.c
******************************************************************************/
BOOL smbEESendByte(BYTE b){
   UINT8 i;
   BOOL ack;
   for (i = 0; i < 8; i++){
      smbEEWrite(b & 0x80);
      b = (b <<  1);
   }
   smbEEClock(0);
   DATA_LOW();
   smbEEClock(1);
   ack = !EE_SDA;
   return ack; //high = ACK received, else ACK not received
}


/******************************************************************************
* Internal function for eeprom.c
* Function for writing bit to the data line. Setting the port as input
* make the SMBus go high because of the pull-up resistors.
*
******************************************************************************/
void smbEEWrite(BOOL value){
   smbEEClock(0);
   if(value){
      DATA_HIGH();
   }
   else{
      DATA_LOW();
   }
   smbEEClock(1);
}


/******************************************************************************
* Internal function for eeprom.c
******************************************************************************/
void smbEEReceive(BYTE address, UINT16 length, BYTE *buffer){
   UINT16 i;

   smbEEStart();
   DATA_HIGH();
   //smbEEClock(1);

   while (!smbEESendByte(address)); //Keep sending address until ACK'ed
   smbEEClock(0);
   DATA_HIGH();

   // (CLK is high and data is high)
   for(i = 0; i < length; i++){
      buffer[i] = smbEEReceiveByte( i==(length-1) ); //TRUE if last byte
      halWait(0x01);
   }

   smbEEClock(1);
   smbEEStop();
}


/******************************************************************************
* Internal function for eeprom.c
******************************************************************************/
BYTE smbEEReceiveByte(BOOL lastByte){
   INT16 i;
   BYTE inData = 0;

   DATA_HIGH();

   for(i = 0; i < 8 * 2; i++){
      if(EE_SCL == 1){ 	//clk high
         if(EE_SDA){ 	//read '1'
            inData = (inData<<1) | 0x01;
         } else {
            inData = (inData<<1) & ~0x01;
         }
         smbEEClock(0);
      }
      else{
         smbEEClock(1);
      }
   }
   smbEEClock(0);

   //do not ACK if last byte
   //lastByte ? smbEEWrite(1) :  smbEEWrite(0);

   DATA_LOW();
   smbEEClock(1);
   smbEEClock(0);

   return inData;
}


/******************************************************************************
* Internal function for eeprom.c
* This function is used to clock the SMBus connected to the LCD. If a negative
* edge is to follow, the pin is set as an output and driven low. If a positive
* edge is to follow, the pin is set as an input and the pull-up resistor is
* to drive the node high. This way the slave device can hold the node low if
* a longer setup time is desired.
*
******************************************************************************/
void smbEEClock(BOOL value){
   if(!value){
      IO_DIR_PORT_PIN(0, 3, IO_OUT);
      EE_SCL = 0;
   }
   else {
      IO_DIR_PORT_PIN(0, 3, IO_IN);
   }
   waitEE();
}

/******************************************************************************
* Internal function for eeprom.c
* This function initiates SMBus communication. It makes sure that both the
* data and the clock of the SMBus are high. Then the data pin is set low
* while the clock is kept high. This initializes SMBus transfer.
*
******************************************************************************/
void smbEEStart(){
   while (! (EE_SDA && EE_SCL) )
   {
      DATA_HIGH();
      smbEEClock(0);
      smbEEClock(1);
   }//wait for Data and clk high
   DATA_LOW();
   waitEE();
   smbEEClock(0);
}

/******************************************************************************
* Internal function for eeprom.c
* This function terminates SMBus communication. It makes sure that the data
* and clock of the SMBus are low and high, respectively. Then the data pin is
* set high while the clock is kept high. This terminates SMBus transfer.
*
******************************************************************************/
void smbEEStop(){
   while (! (!EE_SDA && EE_SCL))
   {
      DATA_LOW();
      smbEEClock(0);
      smbEEClock(1);
   }
   DATA_HIGH();
   smbEEClock(1);
   waitEE();
}


/******************************************************************************
* Internal function for eeprom.c
******************************************************************************/
void waitEE(){
   UINT8 j = 0x01;
   while(j--);
}
