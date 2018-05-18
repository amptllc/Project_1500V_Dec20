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
*/
#ifndef RF_USB_APP_EX_LIB_CONFIG_H
#define RF_USB_APP_EX_LIB_CONFIG_H

/*
 *
 * Description:
 * This file is used to configures the RF USB App Ex library.
 * The definitions not needed in a project can be commented out.
 *
 *
 *
 *             ********** IMPORTANT! *************
 *
 * If any library functions are to be used, copy the rf_usb_lib_config_template.h file
 * into the project catalog.
 * Rename it to rf_usb_lib_config.h, and edit it to get the desired setup of the library.
 *
 *             ***********************************
 *
 */



//-------------------------------------------------------------------------------------------------------
// RF framework setup

// RSSI threshold given in dBm is used to decide if a bind request should be accepted or rejected by
//the CC2511 USB Dongle (range is -9 to -136 dBm)
#if (chip == 2511)
// When using -55 the CC2511 USB Dongle will accept bind requests from CC2510 within approx 1 to 1.5 meter.
#define BIND_REQUEST_RSSI_THRESHOLD   -55
#endif

#if (chip == 1111)
// When using -55 the CC1111 USB Dongle will accept bind requests from CC1110 within approx 1 to 1.5 meter.
#define BIND_REQUEST_RSSI_THRESHOLD   -71     //TDB confirm this number
#endif

// Number of bind retries the remote device will perform before sending a
// RFRU_EVENT_BIND_REQUEST_TIMEOUT event to application.
#define RF_BIND_REQUEST_RETRIES  3
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
// FIFO BUFFER setup

// Set the number of individual FIFO buffers wanted
#define FIFO_SETUP_NUMBER_OF_FIFO_BUFFERS           2
// Set the number of slots within each of the individual FIFO buffers
#define FIFO_SETUP_SLOTS_PER_FIFO                   { 5, 5 }
// Set the size of each slot within each of the individual FIFO buffers
#define FIFO_SETUP_BYTES_PER_SLOT                   { 32, 32} //{ 137, 137 }
// Set the total size of the FIFO buffer
// Must be equal to the sum of all FIFO_SETUP_SLOTS_PER_FIFO multiplied
// with the corresponding FIFO_SETUP_BYTES_PER_SLOT.
#define FIFO_SETUP_TOTAL_SIZE_OF_BUFFER             320 //1370
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
// LIO BUFFER setup

// set the number of individual LIO buffers wanted
#define LIO_SETUP_NUMBER_OF_LIO_BUFFERS              0
// set the size of each slot within each of the individual LIO buffers
#define LIO_SETUP_BYTES_PER_SLOT                    { }
// set the total size of the LIO buffer
// this equals the sum of all LIO_SETUP_BYTES_PER_SLOT multiplied by two.
#define LIO_SETUP_TOTAL_SIZE_OF_BUFFER               0
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
// Timer 4 manager setup

// Enter the number items in the callback table. Note that the time spent in the timer 4 interrupt
// increases with this number
#define T4MGR_SETUP_NUMBER_OF_CALLBACK_FUNCTIONS	 4
//-------------------------------------------------------------------------------------------------------


#endif //RF_USB_APP_EX_LIB_CONFIG_H
