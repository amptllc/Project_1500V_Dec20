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

Filename:     RF04Dev.c
Target:       cc2430, cc2431, cc1110, cc2510
Author:       KJA
Revised:      26/10-2005
Revision:     0.1

Description:
Implementation of commonly used functions for the RF04EB and CC2430DB.

* For use with CC2430DB add "CC2430DB" to project options.
  Project -> Options... -> C compiler -> Preprocessor -> Defined symbols

* For use with RF04EB do _not_ add "CC2430DB to project options.

******************************************************************************/

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

#ifdef CC2430DB
   #include "CC2430DB.h"
#else
   #include "..\EB\include\RF04EB.h"
#endif


/******************************************************************************
* @fn  getJoystickDirection
*
* @brief
*      This function utilizes the ADC to give an indication of the current
*      position of the joystick. Current support is for 90 degrees
*      positioning only.
*
*      The joystick control is encoded as an analog voltage.  Keep on reading
*      the ADC until two consecutive key decisions are the same.
*
*
*      Meassured values from the ADC:
*      ------------------------------------
*      |Direction | REV 'x'   | REV 'y'   |
*      ------------------------------------
*      |DOWN      | 0xFF-0x00 | 0x00      |
*      |LEFT      | 0x2F-0x30 | 0x35-0x36 |
*      |RIGHT     | 0x4C-0x4E | 0x55-0x57 |
*      |UP        | 0x5A-0x5C | 0x65-0x67 |
*      |CENTER    | 0x69-0x6B | 0x76-0x78 |
*      ------------------------------------
*      NOTE: This source code apply to CC1110/ CC1111, CC2510/ CC2511 and
*            CC2430/ CC2431. 'y' is the first revision with improved ADC for
*            each of the different chip.
*
* Parameters:
*
* @param  void
*
* @return JOYSTICK_DIRECTION
*          DOWN:    Joystick direction is down (270 degrees)
*          LEFT:    Joystick direction is left (180 degrees)
*	   RIGHT:   Joystick direction is right (0 degrees)
*	   UP:      Joystick direction is up (90 degrees)
*	   CENTRED: Joystick direction is centred (passive position)
*
******************************************************************************/
JOYSTICK_DIRECTION getJoystickDirection( void ) {
    INT8 adcValue;
    JOYSTICK_DIRECTION direction[2];

    do{
      direction[1] = direction[0];
      adcValue = halAdcSampleSingle(ADC_REF_AVDD, ADC_8_BIT, ADC_INPUT_JOYSTICK);

#if (chip == 2430 || chip == 2431)
      if( CHVER < REV_C )
#endif
#if (chip == 2510 || chip == 2511)
      if( VERSION < REV_E ) // NB: Verify the first revision with bug fixed ADC
#endif
#if (chip == 1110 || chip == 1111)
      if( VERSION < REV_C ) // NB: Verify the first revision with bug fixed ADC
#endif
      {
        // use this limits for "old" revisions
        if (adcValue < 0x10) {
          direction[0] = DOWN;
        } else if (adcValue < 0x40) {
          direction[0] = LEFT;
        } else if (adcValue < 0x50) {
          direction[0] = RIGHT;
        } else if (adcValue < 0x60) {
          direction[0] = UP;
        } else {
          direction[0] = CENTRED;
        }
      }
      else
      {
        // use this limits for "new" revisions
        if (adcValue < 0x10) {
          direction[0] = DOWN;
        } else if (adcValue < 0x40) {
          direction[0] = LEFT;
        } else if (adcValue < 0x60) {
          direction[0] = RIGHT;
        } else if (adcValue < 0x70) {
          direction[0] = UP;
        } else {
          direction[0] = CENTRED;
        }
      }
    } while(direction[0] != direction[1]);

  return direction[0];
}


/******************************************************************************
* @fn  getPotValue
*
* @brief
*      This function utilizes the 8-bit ADC to obtain a digital value for the
*      potentiometer resistance.
*
* Parameters:
*
* @param  void
*
* @return UINT8
*         0xFF
*         0x00
*
******************************************************************************/
UINT8 getPotValue( void ){
    INT8 adcValue;

    adcValue = halAdcSampleSingle(ADC_REF_AVDD, ADC_8_BIT, ADC_INPUT_POT);

    if (adcValue < 0){
      adcValue = 0;
    }

    return adcValue;
}


/******************************************************************************
* @fn  buttonPushed
*
* @brief
*      This function detects if the button is being pushed. The function
*      implements software debounce. Return true only if previuosly called
*      with button not pushed. Return true only once each time the button
*      is pressed.
*
* Parameters:
*
* @param  void
*
* @return BOOL
*          TRUE: Button is being pushed
*          FALSE: Button is not being pushed
*
******************************************************************************/
#define   BUTTON_ACTIVE_TIMEOUT   10
BOOL buttonPushed( void ) {
   UINT8 i;
   BOOL value;
   static BOOL prevValue;

   if (value = BUTTON_PRESSED()){
      for(i = 0;i < BUTTON_ACTIVE_TIMEOUT; i++){
         if(!BUTTON_PRESSED()){
            value = FALSE;
            break;
         }
      }
   }

   if(value){
      if (!prevValue){
         value = prevValue = TRUE;
      }
      else{
         value = FALSE;
      }
   }
   else{
      prevValue = FALSE;
   }
   return value;
}


/******************************************************************************
* @fn  joystickPushed
*
* @brief
*      This function detects if the joystick is being pushed. The function
*      implements software debounce. Return true only if previuosly called
*      with joystick not pushed. Return true only once each time the joystick
*      is pressed.
*
* Parameters:
*
* @param  void
*
* @return BOOL
*          TRUE: Button is being pushed
*          FALSE: Button is not being pushed
*
******************************************************************************/
BOOL joystickPushed( void ) {
   UINT8 i;
   BOOL value;
   static BOOL prevValue;

   if (value = JOYSTICK_PRESSED()){
      for(i = 0;i < BUTTON_ACTIVE_TIMEOUT; i++){
         if(!JOYSTICK_PRESSED()){
            value = FALSE;
            break;
         }
      }
   }

   if(value){
      if (!prevValue){
         value = prevValue = TRUE;
      }
      else{
         value = FALSE;
      }
   }
   else{
      prevValue = FALSE;
   }

   return value;
}

#ifdef CC2430DB
/******************************************************************************
* @fn  getLdrValue
*
* @brief
*      Returns a value indicating the illumination of the light dependent
*      resistor (LDR). Values range from 0xFF (255) for saturated sensor
*      (resistor value around 5 kOhm), to 0 for dark sensor (resistor value
*      around 20MOhm).
*
* Parameters:
*
* @param  void
*
* @return UINT8
*         0xFF (255)  maximum illumination (saturated sensor)
*         0x00        dark sensor
*
******************************************************************************/
UINT8 getLdrValue( void ){
   INT8 adcValue = halAdcSampleSingle(ADC_REF_AVDD, ADC_8_BIT, ADC_INPUT_LDR);

   adcValue = (adcValue > 0) ? adcValue : 0;
   // max 8 bit value from ADC is 0x7F (127)
   adcValue *= 2;

   return (255 - adcValue);
}

/******************************************************************************
* @fn  getXacceleration
*
* @brief
*      This function returns the value of the current acceleration of the of
*      accelerometer in the x-axis.
*
* Parameters:
*
* @param  void
*
* @return UINT8
*         0xFF (255)	+18g (theoretical value)
*	  0x7F (127)      0g no acceleration
*         0x00          -18g (theoretical value)
*
* Wait 20 ms after VCC is turned on before calling this function.
******************************************************************************/
UINT8 getXacceleration( void ){
   INT8 adcValue = halAdcSampleSingle(ADC_REF_AVDD, ADC_8_BIT, ADC_INPUT_ACC_X);
   return (adcValue > 0) ? adcValue : 0;
}

/******************************************************************************
* @fn  getYacceleration
*
* @brief
*      This function returns the value of the current acceleration of the of
*      accelerometer in the y-axis.
*
* Parameters:
*
* @param  void
*
* @return UINT8
*         0xFF (255)	+18g (theoretical value)
*	  0x7F (127)      0g no acceleration
*         0x00          -18g (theoretical value)
*
* Wait 20 ms after VCC is turned on before calling this function.
******************************************************************************/
UINT8 getYacceleration( void ){
   INT8 adcValue = halAdcSampleSingle(ADC_REF_AVDD, ADC_8_BIT, ADC_INPUT_ACC_Y);
   return (adcValue > 0) ? adcValue : 0;
}
#endif
