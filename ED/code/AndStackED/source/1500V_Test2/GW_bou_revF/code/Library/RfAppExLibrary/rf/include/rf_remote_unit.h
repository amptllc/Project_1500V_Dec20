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
#ifndef RF_H
#define RF_H
/** \addtogroup module_rfru RF Remote Unit protocol for CC2510/CC1110 (rfru)
 * \brief Simple radio framework intended to be used between RF USB Dongle and CC2510/CC1110 external devices.
 *
 * This is the Remote Unit protocol for CC2510/CC1110 (rfru), it shold be implemented on the CC2510/CC1110.
 *
 * Please see \ref page_rf_usb_library "RF protocol (rfru/rfud)"
 * for an overview of the entire RF framework.
 *
 * To implement the rfru module on the CC2510/CC1110, do the following:
 *
 * Call the function \ref rfruInit() to setup the radio, and to give the RF module the minimum maximum packet size (buffer size).\n
 * See example below:
 * \par Example 1: Setup radio.
 *
 * \code
 * rfruInit(100); //init RF framework, use buffers with length of 100 bytes.
 * \endcode
 *
 * Implement the \ref rfruHookRfEvent() hook in the main application: \n
 * When an RF event occurs, a call will be made to the hook from the RF framework. \n
 * See example below, the \ref module_fifo_buffer "FIFO Buffer (fifo)" is used for buffer handling in the example:
 * \par Example 2: Implement hook.
 *
 * \code
 * void rfruHookRfEvent(BYTE rfEvent, BYTE eventData){
 *     switch (rfEvent) {
 *     case RFRU_EVENT_DATA_PACKET_RECEIVED :
 *         // A data packet is received.
 *         // This will only happen after sending a RF_DATA_REQUEST to RF USB Dongle with rfruSendDataRequest()
 *         // save the packet received, it is located in the buffer given as "pRxPacketBuffer" in the rfruSendDataRequest() call.
 *         fifoFinishedEnqueue(RECEIVED_DATA_BUFFER);
 *         break;
 *     case RFRU_EVENT_RETRANSMSSION_RECEIVED :
 *         // A retransmission is received, this will happen if a RF_ACK packet sent from this device have been lost.
 *         // Do not care, the RF framework will automaticly send a new RF_ACK packet.
 *         // No handling is done here.
 *         break;
 *     case RFRU_EVENT_ACK_RECEIVED :
 *         // A packet successfully sent to the RF USB Dongle
 *         fifoFinishedDequeue(OUTGOING_DATA_BUFFER); //remove packet from the send buffer
 *         break;
 *     case RFRU_EVENT_NACK_RECEIVED :
 *         // The RF USB Dongle did receive the last packet sent, but was unable to process it.
 *         // No handling is done, the packet is left in the OUTGOING_DATA_BUFFER. Application will try to send again later.
 *         break;
 *     case RFRU_EVENT_DATA_REQUEST_NACKED:
 *         // The RF USB Dongle did not have any data ready when it received a RF_DATA_REQUEST.
 *         // No handling is done. Application will send more RF_DATA_REQUEST's later.
 *         break;
 *     case RFRU_EVENT_BIND_SUCCESSFULL :
 *         // After sending a rfruSendBindRequest() the RF USB Dongle have replied and assigned a address to this device.
 *         // The address assigned to this device and the network address is automatically stored in the
 *         // RF framework and inserted into all packets.
 *         // We need to save the device address of the RF USB Dongle, so we know where to send packets and data requests.
 *         dongleAddress = eventData;
 *         break;
 *     case RFRU_EVENT_ACK_TIMEOUT :
 *         // The RF USB Dongle did not receive the last packet sent.
 *         // No handling is done, the packet is left in the OUTGOING_DATA_BUFFER. Application will try to send again later.
 *         break;
 *     case RFRU_EVENT_DATA_PACKET_REQUEST_TIMEOUT:
 *         // The RF USB Dongle did not receive the last RF_DATA_REQUEST sent.
 *         // No handling is done. Application will send more RF_DATA_REQUEST's later.
 *         break;
 *     case RFRU_EVENT_BIND_REQUEST_TIMEOUT:
 *         // The RF USB Dongle did not receive the last RF_BIND_REQUEST sent, or it refused to accept the bind request.
 *         // No handling is done. Application / user will try again later.
 *         break;
 *     }
 * }
 * \endcode
 *
 *
 * Before any data can be transferred between CC2510/CC1110 and the RF USB Dongle, the CC2510/CC1110 must have an address. \n
 * The address can be set in two ways: \n
 * Manually by calling \ref rfruSetMyAddress(). This can be done if both the network address and the device address is known to the
 * CC2510/CC1110 remote device. \n
 * Automatically by performing a full bind sequence with the RF USB Dongle. This is done by calling the \ref rfruSendBindRequest().
 * This will result in a \ref rfruHookRfEvent() with one of the following events (See code example 2 above):
 * \li \ref RFRU_EVENT_BIND_SUCCESSFULL
 * \li \ref RFRU_EVENT_BIND_REQUEST_TIMEOUT
 *
 * To send data to the RF USB Dongle, check if radio is ready by calling \ref rfruIsRadioReady() and then call rfruSendDataPacket().
 * See code example 3 below. \n
 * If the \ref RF_ACK_REQUEST flag is set,
 * this will result in a \ref rfruHookRfEvent() with one of the following events (See code example 2 above):
 * \li \ref RFRU_EVENT_ACK_RECEIVED
 * \li \ref RFRU_EVENT_NACK_RECEIVED
 * \li \ref RFRU_EVENT_ACK_TIMEOUT
 *
 * To check for and receive any data from the RF USB Dongle to this device, check if radio is ready by calling \ref rfruIsRadioReady()
 * and then call \ref rfruSendDataRequest() \n
 * See code example 3 below. \n
 * This will result in a \ref rfruHookRfEvent() with one of the following events (See code example 2 above):
 * \li \ref RFRU_EVENT_DATA_PACKET_RECEIVED
 * \li \ref RFRU_EVENT_NACK_RECEIVED
 * \li \ref RFRU_EVENT_DATA_PACKET_REQUEST_TIMEOUT
 *
 * The \ref module_fifo_buffer "FIFO Buffer (fifo)" is used for buffer handling in example 3 below.
 * \par Example 3: Send data packet or a data request to the RF USB Dongle .
 *
 * \code
 * //this code sequence is included into the main loop:
 *
 * if(rfruIsRadioReady() && dongleAddress) {
 *     // radio is ready, and we have a dongleAddress (a successful bind has occurred)
 *     if(fifoGetUsedSlots(OUTGOING_DATA_BUFFER)) {
 *         // we have a packet ready to send
 *         pTempPacketBuffer = (RF_PACKET __xdata *) fifoStartDequeue(OUTGOING_DATA_BUFFER); // get data packet
 *         pTempPacketBuffer->destAddress = dongleAddress; //set destination address on packet to be sent.
 *         pTempPacketBuffer->flag = RF_ACK_REQUEST; // Clear all flag and set the RF_ACK_REQUEST flag
 *         if(fifoGetUsedSlots(OUTGOING_DATA_BUFFER) > 1) {
 *             //we have more than one data packet ready, so we set the RF_DATA_PENDING flag
 *             pTempPacketBuffer->flag |= RF_DATA_PENDING;
 *         }
 *         rfruSendDataPacket(pTempPacketBuffer); // send the packet
 *     } else {
 *         //we have no data to send, send a data request to see if RF USB Dongle have data for ready for us
 *         if(fifoGetFreeSlots(RECEIVED_DATA_BUFFER)) { // check that we have a receive buffer available.
 *             rfruSendDataRequest(dongleAddress, (RF_PACKET __xdata *) fifoStartEnqueue(RECEIVED_DATA_BUFFER));
 *         }
 *     }
 * }
 * \endcode
 *
 * To stop the radio e.g. before entering power mode 1 - 3, call \ref rfruStopRadio().
 *
 * Please see the RF Application Examples for implementations of the RF framework.
 *
 * @{
 */

/// \name Functions
//@{

BOOL rfruInit(BYTE bufferSize);
BOOL rfruIsRadioReady(void);
BOOL rfruSendDataPacket(RF_PACKET __xdata* pTxPacket);
BOOL rfruSendDataRequest(BYTE destAdr, RF_PACKET __xdata* pRxPacketBuffer);
void rfruSendBindRequest(BYTE preferredAddress);
void rfruStopRadio(void);
void rfruSetMyAddress(BYTE networkAddress0, BYTE networkAddress1, BYTE networkAddress2, BYTE deviceAddress);
BYTE rfruGetMyDeviceAddress(void);
UINT32 rfruGetNetworkAddress(void);
RF_PACKET __xdata * rfruGetpRxAckPkt(void);

//@}

/// \name Callback function
/// This callBack function must be defined somewhere in the application.
/// It is called from the rfru module when a rf event occurs.
//@{

 /**
 * \brief Called from the rfud module to notify the main application each time a RF event occurs.
 *
 * This function is called from within an interrupt, so it should be short and efficient. \n
 * The parameter \c rfEvent will say which event has occurred. The parameter \c eventData may hold additional information for some of the events. \n
 * The following rfEvents may occur:
 *
 * \li \ref RFRU_EVENT_DATA_PACKET_RECEIVED - A new data packet is received and ready to be read from the rx buffer.
 * It will always be from the device to which the last \ref RF_DATA_REQUEST was sent, using \ref rfruSendDataRequest()
 * \c eventData contain no data.
 *
 * \li \ref RFRU_EVENT_RETRANSMSSION_RECEIVED - A retransmitted data packet is received.
 * Retransmissions are automatically acked, and thrown away by the rfru module.
 * It will always be from the device to which the last \ref RF_DATA_REQUEST was sent, using \ref rfruSendDataRequest()
 * \c eventData contain no data.
 *
 * \li \ref RFRU_EVENT_ACK_RECEIVED - An ACK packet is received for a data packet.
 * It will always be from the device to which the last \ref RF_DATA_PACKET was sent, using \ref rfruSendDataPacket()
 * \c eventData Contain the status flags, see \ref RF_PACKET_TYPE "Packet flags" from the received ACK packet.
 *
 * \li \ref RFRU_EVENT_NACK_RECEIVED - An NACK packet is received for a data packet.
 * It will always be from the device to which the last \ref RF_DATA_PACKET was sent, using \ref rfruSendDataPacket()
 * \c eventData Contain the status flags, see \ref RF_PACKET_TYPE "Packet flags" from the received NACK packet.
 *
 * \li \ref RFRU_EVENT_DATA_REQUEST_NACKED - An NACK packet is received for a data request.
 * It will always be from the device to which the last \ref RF_DATA_REQUEST was sent, using \ref rfruSendDataRequest()
 * \c eventData Contain the status flags, see \ref RF_PACKET_TYPE "Packet flags" from the received NACK packet.
 *
 * \li \ref RFRU_EVENT_BIND_SUCCESSFULL - A successful bind has occurred to the dongle.
 * The address assigned to this unit can be found by calling \ref rfruGetMyDeviceAddress() and \ref rfruGetNetworkAddress().
 * \c eventData The device address of the dongle.
 *
 * \li \ref RFUD_EVENT_ACK_TIMEOUT - No ACK/NACK packet is received within the ack timeout period after sending a \ref RF_DATA_PACKET, using \ref rfruSendDataPacket().
 * \c eventData contain no data.
 *
 * \li \ref RFRU_EVENT_DATA_PACKET_REQUEST_TIMEOUT - No data packet is received within timeout period after sending a \ref RF_DATA_REQUEST using \ref rfruSendDataRequest().
 * \c eventData contain no data.
 *
 * \li \ref RFRU_EVENT_BIND_REQUEST_TIMEOUT - No reply received within timeout period after attempting to bind using \ref rfruSendBindRequest().
 * \c eventData contain no data.
 *
 * \param[in]  rfEvent \n
 * This variable will tell what RF event that has occurred.
 * \param[in]  eventData \n
 * This variable may hold additional information about the event.
 */
void rfruHookRfEvent(BYTE rfEvent, BYTE eventData);

//@}



/// \name Module Data
//@{

/// RFRD internal module data
typedef struct {
   DMA_DESC dmaToRadioDesc;                  ///< Holds the DMA descriptor that is used to send data to the radio.
   DMA_DESC dmaFromRadioDesc;                ///< Holds the DMA descriptor that is used to read data from the radio.
   BYTE timer2Cnt;                           ///< Holds the reset value for timer 2 / mac timer, used to time out replyes.
   BYTE pNetworkAddress[3];                  ///< Holds the current network address.
   BYTE pTxSequenceBits[RF_SEQUENCE_BYTES];  ///< Holds sequence bit used in outgoing packets.
   BYTE pRxSequenceBits[RF_SEQUENCE_BYTES];  ///< Holds sequence bit used in incomming packets.
   RF_PACKET pTxAckPkt;                      ///< Buffer that holds an ack packet ready to be transmitted.
   RF_PACKET pTxDataRequestPkt;              ///< Buffer that holds an data request packet ready to be trasnmitted.
   RF_PACKET __xdata * pRxAckPkt;            ///< Buffer that incoming ack packet are stored in.
   RF_PACKET __xdata * pRxPacket;            ///< Pointer to where incoming (non ack) packets should be stored.
   BYTE maxPacketLength;                     ///< Max packet length that can be received (buffer size).
   BYTE lastPktSentTo;                       ///< Address to witch the last packet was sent, and ack is expected from.
   BYTE radio_state;                         ///< The radio state machines current state
   BYTE bindRequestRetries;                  ///< Used to keep track of number of bind requests sent
   BOOL rfTimeout;                           ///< Is set to TRUE when a timeout occures.
} RFRU_DATA;

//@}

/// \name RF_EVENT
///Used as a parameter in the \ref rfCallBackRfEvent() to tell application what rf event has occurred
//@{

/// A new data packet is received and ready to be read from the rx buffer.
#define RFRU_EVENT_DATA_PACKET_RECEIVED        0x01
/// A packet is received, the sequence indicates that it is a retransmission
#define RFRU_EVENT_RETRANSMSSION_RECEIVED      0x02
/// A ack packet is received and ready to be read from the rx buffer.
#define RFRU_EVENT_ACK_RECEIVED                0x03
/// A nack packet is received and ready to be read form the rx buffer
#define RFRU_EVENT_NACK_RECEIVED               0x04
/// A nack packet is received and ready to be read form the rx buffer
#define RFRU_EVENT_DATA_REQUEST_NACKED         0x05
/// A successful bind has occurred to the dongle
#define RFRU_EVENT_BIND_SUCCESSFULL            0x06
/// No ack packet i received within  the ack timeout period.
#define RFRU_EVENT_ACK_TIMEOUT                 0x07
/// No data packet i received after sending a data packet request within timeout period.
#define RFRU_EVENT_DATA_PACKET_REQUEST_TIMEOUT 0x08
/// No  \ref RF_BIND_REQUEST received within timeout period.
#define RFRU_EVENT_BIND_REQUEST_TIMEOUT        0x09

//@}





/// \name radio_state
/// Used by the rf modules internal state machine.
//@{

/// Radio is in idle.
#define RFRU_STATE_IDLE                      0x00
/// The radio is sending a packet, with the \ref RF_ACK_REQUEST bit set.
#define RFRU_STATE_TX_PKT_EXPECT_ACK         0x01
/// The radio is sending a packet, with the \ref RF_ACK_REQUEST bit cleared.
#define RFRU_STATE_TX_PKT_NO_ACK             0x02
/// Sending a \ref RF_ACK of \ref RF_NACK packet.
#define RFRU_STATE_TX_ACK_NACK               0x03
/// The radio is sending a data request.
#define RFRU_STATE_TX_DATA_REQ               0x04
/// Sending a \ref RF_BIND_REQUEST (remote unit)
#define RFRU_STATE_TX_BIND_REQUEST           0x05
/// The radio is waiting for / receiving a packet.
#define RFRU_STATE_RX_DATA_PACKET            0x06
/// The radio is waiting for / receiving a \ref RF_ACK or \ref RF_NACK packet.
#define RFRU_STATE_RX_ACK                    0x07
/// In rx, waiting for or receiving a \ref RF_BIND_RESPONSE (remote unit).
#define RFRU_STATE_RX_BIND_RESPONSE          0x08

//@}


#endif  //RF_H
///@}
