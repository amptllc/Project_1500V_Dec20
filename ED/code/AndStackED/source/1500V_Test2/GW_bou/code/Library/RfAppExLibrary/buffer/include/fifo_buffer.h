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
#ifndef FIFO_BUFFER_H
#define FIFO_BUFFER_H
/** \addtogroup module_fifo_buffer FIFO Buffer (fifo)
 * \brief Provides easy creation and use of one or more XDATA FIFO buffers
 *
 * The fifo's work as first in, first out buffers. \n
 * No data is ewer lost or overwritten.
 * If the buffer get full, it will refuse to enqueue any more data until some is read out.
 *
 * The module creates a user selectable number of xdata ram FIFO structures of user selectable size.
 * To use the FIFO buffer the following constants in the rf_usb_lib_config_template.h must be set correctly.
 *
 * \li \c #FIFO_SETUP_NUMBER_OF_FIFO_BUFFERS
 * \li \c #FIFO_SETUP_SLOTS_PER_FIFO
 * \li \c #FIFO_SETUP_BYTES_PER_SLOT
 * \li \c #FIFO_SETUP_TOTAL_SIZE_OF_BUFFER
 *
 * Example:
 * \code
 *    #define FIFO_SETUP_NUMBER_OF_FIFO_BUFFERS           3
 *    #define FIFO_SETUP_SLOTS_PER_FIFO                   { 5, 6, 3 }
 *    #define FIFO_SETUP_BYTES_PER_SLOT                   { 20, 100, 60}
 *    #define FIFO_SETUP_TOTAL_SIZE_OF_BUFFER             880
 * \endcode
 *
 * In the example above three FIFO buffers are set up.
 *
 * The first (FIFO buffer 0) has the following parameters:\n
 * 5 slots, eg. it can hold up to five packets, each with a maximum size of 20 BYTE
 *
 * The second (FIFO buffer 1) has the following parameters:\n
 * 6 slots, eg. it can hold up to five packets, each with a maximum size of 100 BYTE
 *
 * The third (FIFO buffer 2) has the following parameters:\n
 * 3 slots, eg. it can hold up to three packets, each with a maximum size of 60 BYTE
 *
 * In this example the FIFO_SETUP_TOTAL_SIZE_OF_BUFFER must be 880. \n
 * Because (5 * 20) + (6 * 100) + (3 * 60) = 880.
 *
 *
 *
 * If the FIFO buffers are not to be used. Write the following in the rf_usb_lib_config_template.h:
 * \code
 *    #define FIFO_SETUP_NUMBER_OF_FIFO_BUFFERS           0
 *    #define FIFO_SETUP_SLOTS_PER_FIFO                   { }
 *    #define FIFO_SETUP_BYTES_PER_SLOT                   { }
 *    #define FIFO_SETUP_TOTAL_SIZE_OF_BUFFER             0
 * \endcode
 *
 * @{
 */

/// \name Functions
//@{
//Function are described in the fifo_buffer.c file and in doxygen documentation.
void fifoInit(void);
BYTE fifoGetUsedSlots(BYTE fifoBuffer);
BYTE fifoGetFreeSlots(BYTE fifoBuffer);
BYTE __xdata * fifoStartEnqueue(BYTE fifoBuffer);
void fifoFinishedEnqueue(BYTE fifoBuffer);
BYTE __xdata * fifoStartDequeue(BYTE fifoBuffer);
void fifoFinishedDequeue(BYTE fifoBuffer);
//@}


/// \name Module Data
//@{
/// Struct used internaly in fifo_buffer to keep track of the status of the fifo's.
/// Do not change or use theese parameters outside of the \c fifo_buffer.c and \c fifo_buffer.h files
typedef struct{
   BYTE writeSlotIndex; ///< Next slot index to write to.
   BYTE readSlotIndex;  ///< Next slot index to read from.
   BYTE usedSlots;      ///< Number of slots filled with data.
   BYTE __xdata *pStartOfBuffer; ///< Pointer to the start of the fifo buffer in xdata.
}FIFO_BUFFER_CTRL;
//@}



/// @}
#endif //FIFO_BUFFER_H
