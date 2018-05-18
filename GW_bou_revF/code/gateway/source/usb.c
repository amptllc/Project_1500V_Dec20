/*==== INCLUDES ==============================================================*/
#include "hal_main.h"
#include "main.h"
#include "usb_cdc.h"

#include "rf_usb_app_ex_lib_headers.h"
#include "rf_usb_library_headers.h"
#include "rf\include\rf_usb_dongle.h"
extern void usb_out( void );
// ****************************************************************************************
// All Hooks and functions required by the USB library.
// ****************************************************************************************
/******************************************************************************
* Handle standard USB stuff
******************************************************************************/
void handleMyStdUsb(void){
    //USB related
    // Handle reset signaling on the bus
    if (USBIRQ_GET_EVENT_MASK() & USBIRQ_EVENT_RESET) {
        USBIRQ_CLEAR_EVENTS(0xFFFF);
        usbfwResetHandler();
        //usbfwData.ep0Status = EP_STALL;
        USBPOW = USBPOW_SUSPEND_EN;
    }
    // Handle packets on EP0
    if (USBIRQ_GET_EVENT_MASK() & USBIRQ_EVENT_SETUP) {
       USBIRQ_CLEAR_EVENTS(USBIRQ_EVENT_SETUP);
       usbfwSetupHandler();
       /*
       if( usbfwData.ep0Status == EP_STALL ){
            HAL_INT_ENABLE(INUM_T1,  INT_ON);
       }
       */
    }
    // Handle USB suspend
    if (USBIRQ_GET_EVENT_MASK() & USBIRQ_EVENT_SUSPEND) {
       USBIRQ_CLEAR_EVENTS(USBIRQ_EVENT_SUSPEND); // Clear USB suspend interrupt
       //rfudStopRadio(); // turn off the radio
       usbsuspEnter();// calling this function will take the chip into PM1 until a USB resume is deteceted.
       //usbfwData.ep0Status = EP_STALL;
       USBIRQ_CLEAR_EVENTS(USBIRQ_EVENT_RESUME); // Clear USB resume interrupt.
    }
}
// **************** Process USB class requests with OUT data phase ************************
void usbcrHookProcessOut(void) {
   // Process USB class requests with OUT data phase, or stall endpoint 0 when unsupported
   //usbfwData.ep0Status = EP_STALL;
   if (usbSetupHeader.request == CDC_SET_CONTROL_LINE_STATE) {

   } else if(usbSetupHeader.request == CDC_SET_LINE_CODING) {
      //TODO implement
      if(usbfwData.ep0Status == EP_IDLE){
         usbSetupData.pBuffer = (BYTE *) &lineCoding;
         usbfwData.ep0Status = EP_RX;
      }else if(usbfwData.ep0Status == EP_RX) { }
   }
   // Unknown request?
   else {
      usbfwData.ep0Status = EP_STALL;
      //usbfwData.ep0Status = EP_TX;
   }
}
// **************** Process USB class requests with IN data phase *************************
void usbcrHookProcessIn(void) {
   // Process USB class requests with IN data phase, or stall endpoint 0 when unsupported
   if (usbSetupHeader.request == CDC_GET_LINE_CODING) {
      // First the endpoint status is EP_IDLE...
      if (usbfwData.ep0Status == EP_IDLE) {
         usbSetupData.pBuffer = (BYTE *) &lineCoding;
         usbSetupData.bytesLeft = 7;
         usbfwData.ep0Status = EP_TX;
         // Then the endpoint status is EP_TX (remember: we did that here when setting up the buffer)
      } else if (usbfwData.ep0Status == EP_TX) {
         // usbfwData.ep0Status is automatically reset to EP_IDLE when returning to usbfwSetupHandler()
      }
      // Unknown request?
   } else {
      //usbData.ep0Status = EP_STALL;
      usbfwData.ep0Status = EP_TX;
   }
}
// ********************************  Unsupported USB hooks ********************************
void usbvrHookProcessOut(void)    { usbfwData.ep0Status = EP_STALL; }
void usbvrHookProcessIn(void)     { usbfwData.ep0Status = EP_STALL; }
// ************************  unsupported/unhandled standard requests **********************
void usbsrHookSetDescriptor(void) { usbfwData.ep0Status = EP_STALL; }
void usbsrHookSynchFrame(void)    { usbfwData.ep0Status = EP_STALL; }
void usbsrHookClearFeature(void)  { usbfwData.ep0Status = EP_STALL; }
void usbsrHookSetFeature(void)    { usbfwData.ep0Status = EP_STALL; }
void usbsrHookModifyGetStatus(BYTE recipient, BYTE index, WORD __xdata *pStatus) { }
// ************************ USB standard request event processing *************************
void usbsrHookProcessEvent(BYTE event, UINT8 index) {
   // Process relevant events, one at a time.
   switch (event) {
       case USBSR_EVENT_CONFIGURATION_CHANGING : //(the device configuration is about to change)
       break;
       case USBSR_EVENT_CONFIGURATION_CHANGED :// (the device configuration has changed)
       break;
       case USBSR_EVENT_INTERFACE_CHANGING ://(the alternate setting of the given interface is about to change)
       break;
       case USBSR_EVENT_INTERFACE_CHANGED : //(the alternate setting of the given interface has changed)
       break;
       case USBSR_EVENT_REMOTE_WAKEUP_ENABLED ://(remote wakeup has been enabled by the host)
       break;
       case USBSR_EVENT_REMOTE_WAKEUP_DISABLED ://(remote wakeup has been disabled by the host)
       break;
       case USBSR_EVENT_EPIN_STALL_CLEARED ://(the given IN endpoint's stall condition has been cleared the host)
       break;
       case USBSR_EVENT_EPIN_STALL_SET ://(the given IN endpoint has been stalled by the host)
       break;
       case USBSR_EVENT_EPOUT_STALL_CLEARED ://(the given OUT endpoint's stall condition has been cleared the host)
       break;
       case USBSR_EVENT_EPOUT_STALL_SET ://(the given OUT endpoint has been stalled by the PC)
       break;
   }
}
// ************************ USB interrupt event processing ********************************
void usbirqHookProcessEvents(void) {
    // Handle events that require immediate processing here
    //INT_GLOBAL_ENABLE( FALSE );
        //handleMyStdUsb();
        //usbcrHookProcessOut(); 
        //usbcrHookProcessIn();
    //INT_GLOBAL_ENABLE( TRUE );
}
