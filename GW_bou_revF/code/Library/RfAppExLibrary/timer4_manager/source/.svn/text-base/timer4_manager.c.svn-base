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
/// \addtogroup module_timer4_manager
/// @{
#include "rf_usb_app_ex_lib_headers.h"

/// Callback table
T4MGR_CALLBACK_INFO __xdata pT4mgrCallbacks[T4MGR_SETUP_NUMBER_OF_CALLBACK_FUNCTIONS];



/** \brief	Initializes the timer 4 manager for interrupts at the specified tick interval
 *
 * The number of callbacks, \c T4MGR_SETUP_NUMBER_OF_CALLBACK_FUNCTIONS, must be configured in
 * "rf_usb_lib_config.h". Note that the total CPU time spent in the interrupt increases with falling
 * \c tickInterval and increasing \c T4MGR_SETUP_NUMBER_OF_CALLBACK_FUNCTIONS.
 *
 * \param[in]		tickInterval
 *		The number of microseconds between each tick. Note that not all intervals are possible. Please
 *      examine the resulting prescaler and reload settings for timer 4, to verify that value is OK.
 */
void t4mgrInit(UINT16 tickInterval) {
   UINT8 n;

   // Initialize the callback table
   for (n = 0; n < T4MGR_SETUP_NUMBER_OF_CALLBACK_FUNCTIONS; n++) {
      pT4mgrCallbacks[n].period = T4MGR_UNUSED_CALLBACK;
   }

   // Configure timer 4 for interrupts at the given interval
   TIMER34_INIT(4);
   halSetTimer34Period(4, tickInterval);
   T4CTL |= 0x08;
   INT_ENABLE(INUM_T4, INT_ON);
   TIMER4_RUN(TRUE);

} // t4mgrInit




/** \brief	Configures a callback to occur at the specified period
 *
 * The first callback will occur after one whole period has elapsed (the counter starts at 0).
 * Note that the allocation is done backwards, so the last entry in the table is used first.
 *
 * \param[in]		period
 *		The number of ticks between each callback. Use \c T4MGR_SUSPENDED_CALLBACK to begin in suspended
 *      mode.
 * \param[in]		CallbackFunc
 *		A pointer to the callback function, of type "void Func(void) { ... }"
 *
 * \return
 *		A zero-based index into the callback table, or \c T4MGR_INVALID_INDEX if there were no unused
 *      entries.
 */
UINT8 t4mgrSetupCallback(UINT8 period, VFPTR CallbackFunc) {
   UINT8 n = T4MGR_SETUP_NUMBER_OF_CALLBACK_FUNCTIONS - 1;
   BYTE critSect;
   volatile UINT8 __xdata *pPeriod = &pT4mgrCallbacks[T4MGR_SETUP_NUMBER_OF_CALLBACK_FUNCTIONS - 1].period;

   // Search backwards to make it go faster
   do {

      // The period value is 0 for unused entries. pPeriod points to pCallbackInfo[n].period
      if (!*pPeriod) {

         // Protect the table updating operation to make the function reentrant
         critSect = utilEnterCriticalSection();
         if (!*pPeriod) {
            pT4mgrCallbacks[n].period = period;
            pT4mgrCallbacks[n].counter = 0;
            pT4mgrCallbacks[n].CallbackFunc = CallbackFunc;
            utilLeaveCriticalSection(critSect);
            return n;
         }
         utilLeaveCriticalSection(critSect);
      }

      // Move on to the next entry
      pPeriod -= sizeof(T4MGR_CALLBACK_INFO);
   } while (n--);

   // None available :(
   return T4MGR_INVALID_INDEX;

} // t4mgrSetupCallback




/** \brief	Modifies the period and counter of a callback
 *
 * \param[in]		index
 *		The index returned by \c t4mgrSetupCallback
 * \param[in]		period
 *		The new period. Use \c T4MGR_KEEP_PERIOD to keep the current value.
 * \param[in]		counter
 *		The new period. Use \c T4MGR_KEEP_COUNTER to keep the current value.
 */
void t4mgrModifyCallback(UINT8 index, UINT8 period, UINT8 counter) {
   BYTE critSect;
   critSect = utilEnterCriticalSection();
   if (period != T4MGR_KEEP_PERIOD) pT4mgrCallbacks[index].period = period;
   if (counter != T4MGR_KEEP_COUNTER) pT4mgrCallbacks[index].counter = counter;
   utilLeaveCriticalSection(critSect);
} // t4mgrModifyCallback




/** \brief	Cancels a callback
 *
 * This function must be called whenever a callback entry is to be reused.
 *
 * \param[in]		index
 *		The index returned by \c t4mgrSetupCallback
 */
void t4mgrCancelCallback(UINT8 index) {
   pT4mgrCallbacks[index].period = 0;
} // t4mgrCancelCallback




/** \brief	Timer 4 tick interrupt
 *
 * The interrupt increments all counters that are not unused or suspended. The counter is changed before
 * making the callback.
 */
#pragma vector=T4_VECTOR
__interrupt void t4mgrIsr(void) {
   UINT8 n = T4MGR_SETUP_NUMBER_OF_CALLBACK_FUNCTIONS - 1;

   // Search backwards to make it go faster
   do {

      // Turn interrupts off while checking period vs. counter to make the setup, modify and cancel
      // functions safe when called from higher priority interrupts
      INT_GLOBAL_ENABLE(INT_OFF);
      if ((pT4mgrCallbacks[n].period != T4MGR_UNUSED_CALLBACK) &&
		  (pT4mgrCallbacks[n].period != T4MGR_SUSPENDED_CALLBACK)) {
         if (pT4mgrCallbacks[n].period == ++pT4mgrCallbacks[n].counter) {
            pT4mgrCallbacks[n].counter = 0;
            INT_GLOBAL_ENABLE(INT_ON);
            pT4mgrCallbacks[n].CallbackFunc();
         }
      }
      INT_GLOBAL_ENABLE(INT_ON);
   } while (n--);

} // t4mgrIsr


/// @}
