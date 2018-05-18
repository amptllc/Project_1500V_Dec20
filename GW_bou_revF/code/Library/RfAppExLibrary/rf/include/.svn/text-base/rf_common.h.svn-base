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
#ifndef RF_COMMON_H
#define RF_COMMON_H
/** \addtogroup module_rfCommon RF protocol common RF USB (rfru/rfud)
 * \brief Simple radio framework intended to be used between RF USB Dongle and external devices.
 *
 * This is the Common section, containing everything that is common for both the rfud and the rfrd modules.
 *
 * Please see \ref page_rf_usb_library "RF protocol common (rfru/rfud)"
 * for an overview of the entire RF framework.
 *
 * @{
 */


///Packet format of all the packets used, including ack and bind packets.
typedef struct{
   BYTE length;            ///< Length byte, length of packet excluding Length, LQI and RSSI bytes
   BYTE destAddress;       ///< Destination address. 8 bit
   BYTE sourceAddress;     ///< Source address. 8bit
   BYTE networkAddress[3]; ///< Network address. 24 bit
   BYTE flag;              ///< Status flags, see \ref RF_PACKET_TYPE "Packet flags"
   BYTE data[1];           ///< First data byte. The rest of the payload should be stored after this byte in the ram.
}RF_PACKET;


/// \name Packet flags
///Flags found in the flag byte of the \ref RF_PACKET header.
//@{

/// Type of packet, see \ref RF_DATA_PACKET "Packet type" for the different packet types.
#define RF_PACKET_TYPE                 0xE0
/// If set, the source has more data pending.
#define RF_DATA_PENDING                0x04
/// Acknowledge for packet is requested.
#define RF_ACK_REQUEST                 0x02
/// Sequence bit, used to detect retransmissions caused by lost acknowledge packets.
#define RF_SEQUENCE_BIT                0x01

//@}


/// \name Packet type
/// All the possible values of the \ref RF_PACKET_TYPE
//@{

/// Packet contains 0 or more bytes of data
#define RF_DATA_PACKET                 0x00
/// Only used by remote unit. Used to poll dongle for data
#define RF_DATA_REQUEST                0x20
/// Acknowledge packet
#define RF_ACK                         0x40
///No acknowledge packet. Used if data is received, but receiver is unable to process them. Or if received a \ref RF_DATA_REQUEST and have no data to send.
#define RF_NACK                        0x60
/// Only used by remote unit. Request bind to dongle.
#define RF_BIND_REQUEST                0x80
/// Only used by dongle. Reply to a bind request.
#define RF_BIND_RESPONSE               0xA0

//@}

#define RF_CRC_OK                0x80
#define HEADER_LENGTH            7
#define CRC_LENGTH               2
#define CRC_OFFSET               (HEADER_LENGTH - CRC_LENGTH)


/// RF_SEQUENCE_BYTES set the size of the table used to hold the sequence bits.
// The number of sequence bytes will limit the number of address that can be used.
// If RF_SEQUENCE_BYTES is set to low and the application sends a packet / ack packet
// to a unsupported address, random xdata memory will be overwritten.
// So leave it at 32 unless running low on memory and understanding what you are doing.
// Each byte holds the status of 8 addresses.
#define RF_SEQUENCE_BYTES        32


///BIND_REQUEST_RSSI_THRESHOLD_CONVERTED hold the RSSI value used as threshold when accepting or discarding bind requests.
//refer to datasheet 15.10.3 RSSI for description of conversion.
#define BIND_REQUEST_RSSI_THRESHOLD_CONVERTED  ((BIND_REQUEST_RSSI_THRESHOLD  + 72) * 2)

/// \name Macros
/// Used internally by the rf modules, should not be called from anywhere else.
//@{

///returns the sequence bit for a given address eg. 0x01 or 0x00
#define RF_GET_SEQ_BIT(pSequenceBits, address)		   ((pSequenceBits[address >> 3] >> (address & 0x07)) & 0x01)
///toggles the sequence bit for a given address.
#define RF_TOGGLE_SEQ_BIT(pSequenceBits, address)	   (pSequenceBits[address >> 3] ^= (0x01 << (address & 0x07)))
///clears (set to 0) the sequence bit for a given address.
#define RF_CLR_SEQ_BIT(pSequenceBits, address)	      (pSequenceBits[address >> 3] &= ~(0x01 << (address & 0x07)))
///setup dma channel 0 for radio TX
#define RF_START_DMA_TX(pDmaDesc)                     do {FSCAL3 |= 0x20; DMA_ABORT_CHANNEL(0); DMA_SET_ADDR_DESC0(pDmaDesc); DMA_ARM_CHANNEL(0);} while (0)
///setup dma channel 0 for radio RX
#define RF_START_DMA_RX(pDmaDesc)                     do {FSCAL3 &= ~0x30; DMA_ABORT_CHANNEL(0); DMA_SET_ADDR_DESC0(pDmaDesc); DMA_ARM_CHANNEL(0);} while (0)
///check for RX overlflow in radio
#define RX_OVERFLOW                                   (MARCSTATE == 0x11)
///check for TX underflow in radio
#define TX_UNDERFLOW                                  (MARCSTATE == 0x16)
///check CRC on packet in rx buffer
#define CHECK_CRC(pPktHeader)                         (pPktHeader->data[(pPktHeader->length) - CRC_OFFSET] & RF_CRC_OK)
///check network address on packet in rx buffer
#define CHECK_NETW_ADDR(pPktHeader, pNetworkAddress)  ((pPktHeader->networkAddress[0] == pNetworkAddress[0]) && (pPktHeader->networkAddress[1] == pNetworkAddress[1]) && (pPktHeader->networkAddress[2] == pNetworkAddress[2]))
///set address recognition to accept only the address found in \c ADDR
#define ADR_CHECK_ON()                                do { PKTCTRL1 &= ~0x03; PKTCTRL1 |= 0x01;} while (0)
///set address recognition to accept address found in \c ADDR and 0x00
#define ADR_CHECK_AND_BROADCAST_0x00_ON()             do {PKTCTRL1 &= ~0x03; PKTCTRL1 |= 0x02;} while (0)
///start timer2/mac timer timeout
#define START_RADIO_TIMEOUT(macTimeout)               do { T2CTL &= ~0x40; T2CT = macTimeout; T2IF = 0; T2IE = 1;} while (0)
///stop/cancel timer2/mac timer timeout
#define STOP_RADIO_TIMEOUT()                          do { T2IE = 0; T2CT = 0; } while (0)
///check if packet is a bindRequest
#define CHECK_IF_BIND_REQUEST(pPktHeader)             ((pPktHeader->networkAddress[0] == 0xFF) && \
                                                       (pPktHeader->networkAddress[1] == 0xFF) && \
                                                       (pPktHeader->networkAddress[2] == 0xFF)) && \
                                                       ((pPktHeader->flag & RF_PACKET_TYPE) == RF_BIND_REQUEST) //&& \
//                                                       (!(pPktHeader->data[0] & 0x80))  && \
//                                                       ((INT8) pPktHeader->data[0] > (INT8) BIND_REQUEST_RSSI_THRESHOLD_CONVERTED))
//@}


#endif  //RF_COMMON_H
///@}
