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
#ifndef TIMER4_MANAGER_H
#define TIMER4_MANAGER_H
/** \addtogroup module_timer4_manager  Timer 4 Manager (t4mgr)
 * \brief Provides run-time individuably configurable, periodical callbacks.
 *
 * The module uses the 8-bit timer 4 to generate interrupts at an interval specified in the initial call
 * to \ref t4mgrInit. Each timer 4 interrupt is called a "tick".
 *
 * The information for each callback is stored in a structure called \ref T4MGR_CALLBACK_INFO, which
 * contains a period, a counter and a function pointer. The counter is incremented for each timer tick,
 * until it reaches the specified period. The counter is then reset (to 0), and the callback is made.
 *
 * Up to \ref T4MGR_SETUP_NUMBER_OF_CALLBACK_FUNCTIONS callbacks can be configured at a time, with a period
 * of 1 to 254 timer ticks. Note that there are two special values for the period number:
 *     \li An entry in the callback table is unused when the period is set to
 *         \ref T4MGR_UNUSED_CALLBACK = 0.
 *     \li An entry in the callback table is suspended (i.e. the tick counter is halted) as long as the
 *         period is set to \ref T4MGR_SUSPENDED_CALLBACK = 255.
 *
 * A callback is configured by calling \ref t4mgrSetupCallback. The counter and the period can later on be
 * changed by calling \ref t4mgrModifyCallback. The callback can be removed by calling
 * \ref t4mgrCancelCallback.
 *
 * @{
 */


/// All entries in the callback table are currently in use (returned by \c t4mgrSetupCallback)
#define T4MGR_INVALID_INDEX			0xFF
/// Special value for \c T4MGR_CALLBACK_INFO.period: The entry in the callback table is unused
#define T4MGR_UNUSED_CALLBACK		0x00
/// Special value for \c T4MGR_CALLBACK_INFO.period: The entry in the callback table is suspended
#define T4MGR_SUSPENDED_CALLBACK	0xFF


/// Special value for \c t4mgrModifyCallback: When the \c period parameter is T4MGR_KEEP_PERIOD,
/// the current value is kept
#define T4MGR_KEEP_PERIOD			0x00
/// Special value for \c t4mgrModifyCallback: When the \c counter parameter is T4MGR_KEEP_COUNTER,
/// the current value is kept
#define T4MGR_KEEP_COUNTER			0xFF


/// Callback table
typedef struct {
   UINT8 counter;		///< Incrementing counter
   UINT8 period;        ///< Number of timer ticks between each callback (counter = ++counter \% period)
   VFPTR CallbackFunc;  ///< Pointer to the callback function
} T4MGR_CALLBACK_INFO;


// Initializes the timer 4 manager for interrupts at the specified tick interval
void t4mgrInit(UINT16 tickInterval);
// Configures a callback to occur at the specified period
UINT8 t4mgrSetupCallback(UINT8 period, VFPTR CallbackFunc);
// Modifies the period and counter of a callback
void t4mgrModifyCallback(UINT8 index, UINT8 period, UINT8 counter);
// Cancels a callback
void t4mgrCancelCallback(UINT8 index);


/// @}
#endif
