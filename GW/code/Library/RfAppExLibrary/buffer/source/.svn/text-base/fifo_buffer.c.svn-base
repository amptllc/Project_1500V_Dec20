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
/// \addtogroup module_fifo_buffer
/// @{
#include "rf_usb_app_ex_lib_headers.h"

#if FIFO_SETUP_NUMBER_OF_FIFO_BUFFERS //only include if FIFO buffers are setup in rf_usb_lib_config.h

//These are set up based on settings in rf_usb_lib_config.h
const BYTE __code pSlotsPerFifo[FIFO_SETUP_NUMBER_OF_FIFO_BUFFERS] = FIFO_SETUP_SLOTS_PER_FIFO;
const UINT16 __code pBytesPerSlot[FIFO_SETUP_NUMBER_OF_FIFO_BUFFERS] = FIFO_SETUP_BYTES_PER_SLOT;
BYTE __xdata pMainFifoBuffer[FIFO_SETUP_TOTAL_SIZE_OF_BUFFER];
FIFO_BUFFER_CTRL __xdata fifoBufferCtrl[FIFO_SETUP_NUMBER_OF_FIFO_BUFFERS];

/** \brief	Init function for FIFO buffer.
 * Init all the FIFO_BUFFER_CTRL structs so that the buffers can be used. \n
 * This function must be called initially inbefore any other fifo functions are called.
 * \param   void
 * \return  void
 */
void fifoInit(void) {
   BYTE i;
   BYTE __xdata *pStartOfFifo = pMainFifoBuffer;

   for(i = 0; i < FIFO_SETUP_NUMBER_OF_FIFO_BUFFERS; i++){
      fifoBufferCtrl[i].writeSlotIndex = 0;
      fifoBufferCtrl[i].readSlotIndex = 0;
      fifoBufferCtrl[i].usedSlots = 0;
      fifoBufferCtrl[i].pStartOfBuffer = pStartOfFifo;
      pStartOfFifo += ((UINT16) pSlotsPerFifo[i]) * ((UINT16) pBytesPerSlot[i]);
   }
}

/** \brief	Returns number of used slots
 * Returns the number of slots that are ready to be dequeued. \n
 * If the selected \c fifoBuffer is empty, the function returns 0.
 * \note \c fifoInit() should be called first, before using any other fifo functions. \n
 * \param[in]   fifoBuffer     The index of the FIFO buffer to check.
 * \return      Number of slots in the choosen buffer that is ready to be dequeued. \n 0 if buffer is empty.
 */
BYTE fifoGetUsedSlots(BYTE fifoBuffer) {   return fifoBufferCtrl[fifoBuffer].usedSlots;  }

/** \brief	Returns number of free slots available in the chosen buffer
 * If the selected \c fifoBuffer is full, the function returns 0.
 * \note \c fifoInit() should be called first, before using any other fifo functions. \n
 * \param[in]   fifoBuffer     The index of the FIFO buffer to check.
 * \return      Number of free slots in the choosen buffer. \n 0 if buffer is full.
 */
BYTE fifoGetFreeSlots(BYTE fifoBuffer) {   return (pSlotsPerFifo[fifoBuffer] - fifoBufferCtrl[fifoBuffer].usedSlots); }

/** \brief	Returns a pointer to the next slot to enqueue
 * User can start to write data to the position given in the returned
 * pointer. The user may write up to \c FIFO_SETUP_BYTES_PER_SLOT bytes, incrementing the pointer
 * with one after each write. \n
 * It is up to the calling functions to keep track of the number of bytes written to each slot,
 *       this can be done in two ways:
 *       \li Using only fixed length packets in the slots.
 *       \li Using a length byte, eg. the first byte written contains the length of the data
 *       inserted into the slot.
 * When all the data are written, call \c fifoFinshedEnqueue() to tell the FIFO that the enqueue is completed
 * Until then this function will return the same pointer if called multiple times.
 * Function will return a NULL pointer if the chosen FIFO is full.
 * Hence one should check this before starting to write data to the pointer
 * \note \c fifoInit() should be called first, before using any other fifo functions. \n
 * \param[in]    fifoBuffer     The index of the FIFO buffer get a pointer to.
 * \return		 A xdata pointer to the first BYTE of the slot to enqueue to.     NULL if buffer is full.
 */
BYTE __xdata * fifoStartEnqueue(BYTE fifoBuffer) {
   if(fifoBufferCtrl[fifoBuffer].usedSlots >= pSlotsPerFifo[fifoBuffer]) return NULL; //Buffer is full
   //return a pointer to the correct position in the buffer.
   return (fifoBufferCtrl[fifoBuffer].pStartOfBuffer + ((UINT16) fifoBufferCtrl[fifoBuffer].writeSlotIndex * pBytesPerSlot[fifoBuffer]));
}

/** \brief	Updates the fifo status after a enqueue is completed.
 * Call this function after fist calling \c fifoStartEnqueue() and competed writing the data to the slot.
 * \note \c         fifoInit()     should be called first, before using any other fifo functions.
 * \param[in]       fifoBuffer     The index of the used FIFO buffer.
 * \return		    void
 */
void fifoFinishedEnqueue(BYTE fifoBuffer) {
   if(fifoBufferCtrl[fifoBuffer].usedSlots >= pSlotsPerFifo[fifoBuffer]) return; //Buffer is full
   //update ctrl struct
   if (++fifoBufferCtrl[fifoBuffer].writeSlotIndex == pSlotsPerFifo[fifoBuffer]) fifoBufferCtrl[fifoBuffer].writeSlotIndex = 0;
   fifoBufferCtrl[fifoBuffer].usedSlots++;
}

/** \brief	Returns a pointer to where the next slot to dequeue
 * User can start to read data from position given in the returned pointer.
 * It is up to the calling functions to keep track of the number of bytes written to each slot,
 *       this can be done in two ways:
 *       \li Using only fixed length packets in the slots.
 *       \li Using a length byte, eg. the first byte written contains the length of the data
 *       inserted into the slot.
 *
 * When all the data are read, call \c fifoFinishedDequeue() to tell the FIFO that the dequeue is completed
 * Until then this function will return the same pointer if called multiple times.
 * Function will return a NULL pointer if the chosen FIFO is empty.
 * Hence one should check this before starting to read data.
 *
 * \note \c fifoInit() should be called first, before using any other fifo functions. \n
 * \param[in]       fifoBuffer      The index of the FIFO buffer get a pointer to start dequeue from.
 * \return		    A xdata pointer to the first BYTE of the slot to dequeue to.  NULL if buffer is empty.
 */
BYTE __xdata * fifoStartDequeue(BYTE fifoBuffer) {
   if(fifoBufferCtrl[fifoBuffer].usedSlots == 0) return NULL; //Buffer is empty
   //return a pointer to the correct position in the buffer.
   return (fifoBufferCtrl[fifoBuffer].pStartOfBuffer + (fifoBufferCtrl[fifoBuffer].readSlotIndex * pBytesPerSlot[fifoBuffer]));
}

/** \brief	Updates the fifo status after a dequeue is completed.
 * Call this function after fist calling \c fifoStartDequeue() and competed reading the data from the slot.
 * \note \c fifoInit() should be called first, before using any other fifo functions.
 * \param[in]       fifoBuffer*     The index of the used FIFO buffer.
 * \return		     void
 */
void fifoFinishedDequeue(BYTE fifoBuffer) {
   if(fifoBufferCtrl[fifoBuffer].usedSlots == 0) return; //Buffer is empty
   //update ctrl struct
   if (++fifoBufferCtrl[fifoBuffer].readSlotIndex == pSlotsPerFifo[fifoBuffer])  fifoBufferCtrl[fifoBuffer].readSlotIndex = 0;
   fifoBufferCtrl[fifoBuffer].usedSlots--;
}
#endif //FIFO_SETUP_NUMBER_OF_FIFO_BUFFERS
/// @}
