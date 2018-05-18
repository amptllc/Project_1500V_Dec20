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
#ifndef RF_USB_DONGLE_H
#define RF_USB_DONGLE_H
/** \addtogroup module_rfud RF USB Dongle protocol for RF USB (rfud)
* \brief Simple radio framework intended to be used between RF USB Dongle and external devices.
 *
 * This is the USB Dongle protocol for RF USB dongle (rfud) it shold be implemented on the RF USB Dongle.
 *
 * Please see \ref page_rf_usb_library "RF protocol (rfru/rfud)"
 * for an overview of the entire RF framework.
 *
 * To implement the rfud module on the RF USB Dongle, do the following:
 *
 * Call the function \ref rfudInit() to setup the radio, and to give the RF module the packet size (buffer size). \n
 * Call the function \ref rfudSetMyAddress() to give the address the RF USB Dongle should use.
 * See example below:
 * \par Example 1: Setup radio and setting the address.
 *
 * \code
 * rfudInit(100); //init RF framework, use buffers with length of 100 bytes.
 * rfudSetMyAddress(0x11, 0x22, 0x33, 0x01); //set RF USB Dongle address, network address = 0x112233, device address = 0x01
 * \endcode
 *
 * Implement the following hooks in the main application:
 * \li \ref rfudHookAckPkt()
 * \li \ref rfudHookBindRequest()
 * \li \ref rfudHookDataPktRequest()
 * \li \ref rfudHookRfEvent()
 *
 * When an RF event occurs, a call will be made to the callback functions / hooks from the RF framework.
 * See example below, the \ref module_fifo_buffer "FIFO Buffer (fifo)" is used for buffer handling in the example:
 * \par Example 2: Implementing hooks.
 *
 * \code
 * //A DATA_PACKET is received, the RF module needs to know it should send ACK or NACK
 * BOOL rfudHookAckPkt(BYTE sourceAddress, BYTE __xdata * ackPktflag) {
 *     if(sourceAddress == remoteUnitAddress) { return TRUE; } //packet is from the correct remote device, send ACK
 *     else { return FALSE; } //Packet is from unknown sender, send NACK
 * }
 *
 * //A DATA_REQUEST is received, the RF module needs to know it should send a DATA_PACKET or a NACK
 * RF_PACKET __xdata * rfudHookDataPktRequest(BYTE sourceAddress) {
 *     RF_PACKET __xdata * pPktToSend;
 *     //if packet is from the right remote unit and we have data ready, send a packet
 *     if((sourceAddress == remoteUnitAddress) && (there is a data packet ready...)) {
 *         pPktToSend = (RF_PACKET __xdata *) pointer to packet...;
 *         if(more than one packet ready...) {
 *             //more than one packet of data ready, set data pending bit
 *             pPktToSend->flag |= RF_DATA_PENDING;
 *         }
 *         pPktToSend->flag |= RF_ACK_REQUEST;//if a ACK is wanted, set the ACK_REQUEST flag
 *         return pPktToSend;
 *     }
 *     return NULL;
 * }
 *
 * //A BIND_REQUEST is received, the RF module need to know it the main application want to bind to the remote device
 * // and if so, what address it should assign.
 * BYTE rfudHookBindRequest(BYTE preferredAddress)
 * {
 *     if(application want to bind to the remote device...) {
 *         //if the remote unit requests a valid address, let it have it. Else give it address 0x02.
 *         if((preferredAddress > 1) && (preferredAddress != 0xFF)) {
 *             remoteUnitAddress = preferredAddress; //store the address of the remote device for later use
 *             return preferredAddress;                        //return the address to the RF framework
 *         } else {
 *             remoteUnitAddress = 0x02; //store the address of the remote device for later use
 *             return 0x02;                        //return the address to the RF framework
 *         }
 *     }
 *     else { return 0; } //do not want to bind with the device.
 * }
 *
 * // A RF event ha occurred, this hook notifies the main application
 * void rfudHookRfEvent(BYTE rfEvent, BYTE eventData){
 *     RF_PACKET __xdata * pPktSendt;
 *     switch (rfEvent) {
 *     case RFUD_EVENT_DATA_PACKET_RECEIVED :
 *         if(eventData == remoteUnitAddress) { //if a data packet from remote device, save packet
 *            fifoFinishedEnqueue(RADIO_TO_USB_BUFFER);
 *         }
 *         break;
 *     case RFUD_EVENT_ACK_RECEIVED :
 *         //will only receive this callback a when receiving a ACK packet from the correct device after sending a DATA_PACKET
 *         //Hence this means a packet is successfully sent, removing it from the buffer
 *         fifoFinishedDequeue(USB_TO_RADIO_BUFFER);
 *         break;
 *     case RFUD_EVENT_NACK_RECEIVED :
 *         //Any code needed when a NACK is received
 *         break;
 *     case RFUD_EVENT_ACK_TIMEOUT :
 *         //Any code needed when a packet is sent and no ACK of NACK is received within the timeout period.
 *         break;
*      }
 * }
 * \endcode
 *
 * In addition to handling calls to the hooks, the main application is responsible for supplying the RF framework with a receive buffer.
 * A buffer will be needed when starting the RF framework for the firs time and then after each received packet.
 * To check if the RF framework need a new buffer, call \ref rfudRxBufferNeeded(). \n
 * To provide the rfud module with a receive buffer, call \ref  rfudStartReceive(). \n
 * Make sure that the buffer supplied to the RF framework is at least as large as specified in \ref rfudInit(), if the buffer is smaller
 * the RF module may overwrite other data in the RAM.
 * If the main application does not provide the RF framework with an rx buffer the radio will enter IDLE state,
 * and no data will be received or transmitted until a buffer is provided. \n
 * To ensure that the RF module always have a receive buffer the code example below should be inserted into the main loop.
 * See example below, the \ref module_fifo_buffer "FIFO Buffer (fifo)" is used for buffer handling:
 * \par Example 3: Providing rfud module with rx buffer.
 *
 * \code
 * if(rfudRxBufferNeeded() && fifoGetFreeSlots(RADIO_TO_USB_BUFFER)) {
 *     //if radio needs a buffer, and a buffer is ready in the fifo module.
 *     rfudStartReceive((RF_PACKET __xdata *) fifoStartEnqueue(RADIO_TO_USB_BUFFER));  //give a rx buffer to the rfud module
 * }
 * \endcode
 *
 * To turn the radio off, call \ref rfudStopRadio().
 *
 * Please see the RF USB Application Examples for implementations of the RF framework.
 *
 * @{
 */

/// \name Functions
//@{
//Function are described in the rf_usb_dongle.c file and in doxygen documentation.
void rfudInit(BYTE bufferSize);
void rfudSetMyAddress(BYTE networkAddress0, BYTE networkAddress1, BYTE networkAddress2, BYTE deviceAddress);
BOOL rfudRxBufferNeeded(void);
BOOL rfudStartReceive(RF_PACKET __xdata * pRxPacketBuffer);
void rfudStopRadio(void);
//@}

/// \name Callback functions
/// These callBack functions must be defined somewhere in the application.
/// They are called from rfud module when a rf event occurs.
//@{


/**
 * \brief Called when a packet is received and application must decide if a ack should be sent or not.
 *
 * Called each time the radio receives a packet with the \c RF_ACK_REQUEST bit set. \n
 * The packet can be found in the rxBuffer supplied to the RF module with \ref rfudStartReceive(). \n
 * To make the delay before a ack is sent as short as possible, this function must be
 * completed in the shortest time possible. No handling of the packet,
 * except what is necessary to decide to ack or not should be done. \n
 * As soon as the ack transmission has started, a new callback \ref rfudHookRfEvent()
 * will tell the application to start handling the packet received. \n
 * If the application returns \c TRUE, a \ref RF_ACK packet is sent. When the ack is sent
 * a \ref rfudHookRfEvent() will tell the application to start handle the packet. \n
 * If \c FALSE is returned, a \ref RF_NACK packet is sent and \ref rfudHookRfEvent() will NOT be called.
 *
 *\param[in]  pktSourceAddress \n
 * The device address of the device that the packet originates from. The network address will be equal to
 * the network address on the RF USB Dongle.
 *
 * \param[in]  pAckPktflag \n
 * A pointer to the flags in the \ref RF_ACK / \ref RF_NACK packet that will be sent. \n
 * The application may set the flag:  \ref RF_DATA_PENDING.
 *
 * \return  BOOL \n
 * TRUE if application want to send a \ref RF_ACK packet. \n
 * FALSE if application want to send a \ref RF_NACK packet.
 */
BOOL rfudHookAckPkt(BYTE pktSourceAddress, BYTE __xdata * pAckPktflag);


/**
 * \brief Called when a bind request is received and application must decide if it should be accepted
 *
 * Called each time the radio receives a \ref RF_BIND_REQUEST packet.
 * To make the delay before a reply is sent as short as possible, this function must be
 * completed in the shortest time possible.
 *
 * \param[in]  preferredAddress \n
 * The address the remote device has requested, if 0x00 the remote device has no preferred address.
 * However it is up the dongle to assign addresses and the dongle may assign a different address
 * than the one the remote device requests.
 *
 * \return  BYTE \n
 * The address the dongle assigns to the remote device. \n
 * return 0 if no reply should be sent.(do not want to bind) \n
 * Note that node address 0xFF is reserved. It is used during the binding sequence, and can not be assigned.
 */
BYTE rfudHookBindRequest(BYTE preferredAddress);


/**
 * \brief Called when a \ref RF_DATA_REQUEST packet is received, application can send a \ref RF_DATA_PACKET if it has one ready.
 *
 * To make the delay before a reply is sent as short as possible, this function must be
 * completed in the shortest time possible.
 *
 * All address fields in the packet is automaticly set by the RF framework. \n
 * The following flags/fields in the \ref RF_PACKET.flag are also automatically set before packet is sent: \n
 * \li \ref RF_PACKET_TYPE \n
 * \li \ref RF_SEQUENCE_BIT \n
 *
 * The following flags/fields in the \ref RF_PACKET.flag must be set/cleared by the application before
 * returning the pointer to the packet: \n
 * \li \ref RF_DATA_PENDING \n
 * \li \ref RF_ACK_REQUEST \n
 *
 * \param[in]  sourceAddress \n
 * The device address of the remote device that sent the \ref RF_DATA_REQUEST packet.
 *
 * \return  RF_PACKET __xdata * \n
 * If application has a packet ready, return a pointer to the packet. \n
 * If no packet is ready, return 0.
 */
RF_PACKET __xdata * rfudHookDataPktRequest(BYTE sourceAddress);


/**
 * \brief Called from the rfud module to notify the main application each time a RF event occurs.
 *
 * This function is called from within an interrupt, so it should be short and efficient. \n
 * The parameter \c rfEvent will say which event has occurred. The parameter \c eventData hold additional information. \n
 * The following rfEvents may occur:
 * \li \ref RFUD_EVENT_DATA_PACKET_RECEIVED  - A new data packet is received and ready to be read from the rx buffer.
 * \c eventData contain the deviceAddress the received data packet originates from.
 *
 * \li \ref RFUD_EVENT_ACK_RECEIVED - An ACK packet is received.
 * \c eventData contain the deviceAddress the received ACK packet originates from.
 *
 * \li \ref RFUD_EVENT_NACK_RECEIVED - An NACK packet is received.
 * \c eventData contain the deviceAddress the received NACK packet originates from.
 *
 * \li \ref RFUD_EVENT_ACK_TIMEOUT - No ACK/NACK packet is received within the ack timeout period.
 * \c eventData contain the deviceAddress the last packet was sent to e.g. the deviceAddress of the device that did not reply.
 *
 * \param[in]  rfEvent \n
 * This variable will tell what RF event that has occurred.
 * \param[in]  eventData \n
 * This variable hold additional information about the event.
 */
void rfudHookRfEvent(BYTE rfEvent, BYTE eventData);

//@}
/// \name Module Data
//@{

/// RFUD internal module data, do not modify from outside this module.
typedef struct {
   DMA_DESC dmaToRadioDesc;                  ///< Holds the DMA descriptor that is used to send data to the radio
   DMA_DESC dmaFromRadioDesc;                ///< Holds the DMA descriptor that is used to read data from the radio
   BYTE timer2Cnt;                           ///< Holds the reset value for timer 2 / mac timer, used to time out replyes
   BYTE pNetworkAddress[3];                  ///< Holds the current network address
   BYTE pTxSequenceBits[RF_SEQUENCE_BYTES];  ///< Holds sequence bit used in outgoing packets
   BYTE pRxSequenceBits[RF_SEQUENCE_BYTES];  ///< Holds sequence bit used in incomming packets
   RF_PACKET pTxAckPkt;                      ///< Buffer that holds an ack packet ready to be transmitted.
   RF_PACKET __xdata * pRxAckPkt;            ///< Buffer that incoming ack packet are stored in.
   RF_PACKET __xdata * pRxPacket;            ///< Pointer to where incoming (non ack) packets should be stored.
   BYTE maxPacketLength;                     ///< Max packet length that can be received (buffer size).
   BYTE lastPktSentTo;                       ///< Address to witch the last packet was sent, and ack is expected from.
   BOOL rxBufferNeeded;                      ///< If true the radio has a pRxPacket buffer ready, and can receive packets.
   BYTE radio_state;                         ///< The radio state machines current state
} RFUD_DATA;

//@}



/// \name RF EVENT
///Used as a parameter in the \ref rfudHookRfEvent() to tell application what rf event has occurred
//@{

/// A new data packet is received and ready to be read from the rx buffer.
#define RFUD_EVENT_DATA_PACKET_RECEIVED        0x01
/// An ACK packet is received.
#define RFUD_EVENT_ACK_RECEIVED                0x02
/// An NACK packet is received.
#define RFUD_EVENT_NACK_RECEIVED               0x03
/// No ACK/NACK packet i received within the ack timeout period.
#define RFUD_EVENT_ACK_TIMEOUT                 0x04

//@}


/// \name radio_state
/// Used by the rf modules internal state machine, do not modify from outside this module.
//@{

/// Radio is in idle.
#define RFUD_STATE_IDLE                         0x00
/// The radio is sending a packet, with the \ref RF_ACK_REQUEST bit set.
#define RFUD_STATE_TX_PKT_EXPECT_ACK            0x01
/// The radio is sending a packet, with the \ref RF_ACK_REQUEST bit cleared.
#define RFUD_STATE_TX_PKT_NO_ACK                0x02
/// Sending a \ref RF_ACK  packet.
#define RFUD_STATE_TX_ACK                       0x03
/// Sending a \ref RF_NACK packet.
#define RFUD_STATE_TX_NACK                      0x04
/// Sending a \ref RF_BIND_RESPONSE (dongle).
#define RFUD_STATE_TX_BIND_RESPONSE             0x05
/// The radio is waiting for / receiving a packet.
#define RFUD_STATE_GENERAL_RX                   0x06
/// The radio is waiting for / receiving a \ref RF_ACK or \ref RF_NACK packet.
#define RFUD_STATE_RX_ACK                       0x07

//@}

#endif  //RF_USB_DONGLE_H
///@}
