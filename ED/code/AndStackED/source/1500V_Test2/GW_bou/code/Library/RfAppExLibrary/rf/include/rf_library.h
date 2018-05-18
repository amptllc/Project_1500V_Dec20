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
/** \page page_rf_usb_library RF USB Library
  * \brief Simple radio framework intended to be used between CC2511/CC1111 USB Dongle and CC2510/CC1110 external devices.
 *
 * This RF framework will set up the radio and provide simple framework for
 * binding one or multiple CC2510/CC1110 external devices to a RF USB Dongle, and transferring data between the devices. \n
 *
 * The framework:
 * \li Use acknowledge messages and sequence bit for guarantied lossless, error free data transfer.
 * \li Is completely interrupt driven and has a set of hooks / callback functions that notify the main
 * application when data is received or another RF event occurs.
 * \li Use a 32 bit addresses to identify the different devices. (24 bit network address and 8 bit node address)
 * \li Addresses are distributed by the RF USB Dongle when a remote unit send a bind request to it.
 * \li Have kiss-binding, e.g. will only bind to devices close to the RF USB Dongle. No need for a button on the dongle.
 * \li Power saving on the remote devices is easily implemented.
 *
 * Only communication between one or multiple CC2510 remote device(s) and a RF USB Dongle is supported.
 * Communication between CC2510 remote devices is not supported.
 *
 * The framework leave the RF USB Dongle in RX all the time, only replying to messages from the other devices.
 * While the remote devices initiates all the communication. This makes it easy to save power on the remote unit. \n
 * This is done under the assumption that the dongle, which is connected to a PC has an "unlimited" power supply,
 * while the remote unit may be battery powered.
 *
 * The packet format is fixed, and the same for all packets. See figure below \image html Packet_format.gif
 * The different byte fields are defined in the \c RF_PACKET struct.
 *
 * The RF framework will require exclusive use of the following resources on the MCU and radio.
 * \li DMA channel 0.
 * \li Timer 2 and the Timer 2 interrupt.
 * \li Radio and the RF interrupt.
 * \li The CLKCON.TICKSPD must not be changed after initializing the RF modules.
 *
 * None of the interrupts in the RF framework are time critical, if they are delayed nothing will crash.
 * However if interrupts are delayed packets may be lost and have to be retransmitted, resulting in a lower throughput.
 *
 * The RF framework consists of two modules:
 * \li RF USB Dongle protocol for CC2511/CC1111 (rfud), this module should be implemented on the CC2511/CC111 USB Dongle.
 * \li RF Remote Unit protocol for CC2510/CC1110 (rfru), this module should be implemented on the CC2510/CC1110 external device
 *
 * To use the framework:
 * \li On the CC2510/CC1110 see \ref module_rfru "RF Remote Unit protocol for CC2510/CC1110 (rfru)".
 * \li On the CC2511/CC1111 USB Dongle see \ref module_rfud "RF USB Dongle protocol for CC2511/CC1111 (rfud)".
 * @{
 * @}
 */
