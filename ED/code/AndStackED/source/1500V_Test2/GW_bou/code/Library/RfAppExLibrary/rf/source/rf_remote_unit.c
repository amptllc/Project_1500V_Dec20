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
/// \addtogroup module_rfru
/// @{
#include "rf_usb_app_ex_lib_headers.h"
#include "rf\include\rf_remote_unit.h"

BOOL __xdata rfTimeout; //used both by rf isr and timer 2 isr
static BYTE __xdata rfrdAckPkt[10];//must be 10 to hold CRC
static RFRU_DATA __xdata rfruData;/// internal data

//Some functions only used locally in this file, do not call from anywhere else.
static void rfruHandleUnwantedPacket(void);
static void rfruHandleTimeout(void);
static void rfruSBReq(void);

/**
 * \brief Must be called once before any other functions in the rf_usb_dongle module.
 *
 * Initiates radio and everything associated with it. Does not start RX or TX.
 *
 * \note This function calculates the timeout settings for the mac timer based on the
 * CLOCKON.TICKSPD setting. Do not change CLOCKON.TICKSPD after this function has been called
 *
 * \param[in]  bufferSize \n
 * Set the size of the buffers that will later be given to the radio with the \ref rfudStartReceive() function.
 * The radio will use this number to make sure that it does not write outside the buffers.
 */
BOOL rfruInit(BYTE bufferSize)
{
   BYTE i, temp;
   //setup radio, and calibrate
   halRfConfig(RF_FREQUENCY);
   //calibrate when going to RX/TX
   MCSM0 = 0x10;
   //setup radio
   rfruData.maxPacketLength = bufferSize - 3;
   PKTLEN = rfruData.maxPacketLength;
   MCSM1 = 0x00;

    //set rx ack packet Ptr
   rfruData.pRxAckPkt = (RF_PACKET __xdata *) rfrdAckPkt;
   rfruData.rfTimeout = FALSE;
   rfruData.pTxAckPkt.length = 6;//length of tx ack packet
   rfruData.pTxDataRequestPkt.length = 6;//length of data request packet
   rfruData.pTxDataRequestPkt.flag = RF_DATA_REQUEST;

   for(i = 0; i < RF_SEQUENCE_BYTES; i++)
   {
      rfruData.pTxSequenceBits[i] = 0;
      rfruData.pRxSequenceBits[i] = 0;
   }

   //setup all fields in DMA descriptor for RX that newer change
   SET_WORD(rfruData.dmaFromRadioDesc.SRCADDRH, rfruData.dmaFromRadioDesc.SRCADDRL, &X_RFD);
   rfruData.dmaFromRadioDesc.VLEN       = VLEN_1_P_VALOFFIRST_P_2;
   SET_WORD(rfruData.dmaFromRadioDesc.LENH, rfruData.dmaFromRadioDesc.LENL, bufferSize);
   rfruData.dmaFromRadioDesc.WORDSIZE   = WORDSIZE_BYTE;
   rfruData.dmaFromRadioDesc.TMODE      = TMODE_SINGLE;
   rfruData.dmaFromRadioDesc.TRIG       = DMATRIG_RADIO;
   rfruData.dmaFromRadioDesc.DESTINC    = DESTINC_1;
   rfruData.dmaFromRadioDesc.SRCINC     = SRCINC_0;
   rfruData.dmaFromRadioDesc.IRQMASK    = IRQMASK_DISABLE;
   rfruData.dmaFromRadioDesc.M8         = M8_USE_8_BITS;
   rfruData.dmaFromRadioDesc.PRIORITY   = PRI_GUARANTEED;

   //setup all fields in DMA descriptor for TX that newer change
   SET_WORD(rfruData.dmaToRadioDesc.DESTADDRH, rfruData.dmaToRadioDesc.DESTADDRL, &X_RFD);
   rfruData.dmaToRadioDesc.VLEN       = VLEN_1_P_VALOFFIRST;
   SET_WORD(rfruData.dmaToRadioDesc.LENH, rfruData.dmaToRadioDesc.LENL, 256);
   rfruData.dmaToRadioDesc.WORDSIZE   = WORDSIZE_BYTE;
   rfruData.dmaToRadioDesc.TMODE      = TMODE_SINGLE;
   rfruData.dmaToRadioDesc.TRIG       = DMATRIG_RADIO;
   rfruData.dmaToRadioDesc.DESTINC    = DESTINC_0;
   rfruData.dmaToRadioDesc.SRCINC     = SRCINC_1;
   rfruData.dmaToRadioDesc.IRQMASK    = IRQMASK_DISABLE;
   rfruData.dmaToRadioDesc.M8         = M8_USE_8_BITS;
   rfruData.dmaToRadioDesc.PRIORITY   = PRI_GUARANTEED;

   rfruData.radio_state = RFRU_STATE_IDLE;
   RFIF &= ~(IRQ_DONE | IRQ_TXUNF | IRQ_RXOVF);
   RFIM |= (IRQ_DONE | IRQ_TXUNF | IRQ_RXOVF);
   INT_ENABLE(INUM_RF, INT_ON);

   // setup timer 2
   halSetTimer2Period(5000, &temp, &i);
   rfruData.timer2Cnt = temp;
   T2CT = 0;
   T2CTL = 0x10;
   INT_ENABLE(INUM_T2, INT_ON);
   INT_GLOBAL_ENABLE(INT_ON);
   return TRUE;
}

/**
 * \brief Used to check if radio is busy sending or receiving a packet.
 *
 * \return  BOOL \n
 * If TRUE is returned, the radio is in IDLE and the application can start a new packet transmission.
 */
BOOL rfruIsRadioReady(void)
{
   return (rfruData.radio_state == RFRU_STATE_IDLE);
}



/**
 * \brief Used to send a \ref RF_DATA_PACKET to the dongle.
 *
 * After successfully calling this function with the \ref RF_ACK_REQUEST flag set
 * one of the three following \ref RF_EVENT will occur.
 * \li \ref RFRU_EVENT_ACK_RECEIVED - An ACK packet is received, indicating that the dongle has successfully received the packet.
 * \li \ref RFRU_EVENT_NACK_RECEIVED - An NACK packet is received, indicating that the packet did reach the dongle,
 * but the dongle was unable or unwilling to accept it. The application must decide what to do,
 * but in most cases it should wait a short time and then attempt to retransmit the packet.
 * \li \ref RFUD_EVENT_ACK_TIMEOUT - No ACK/NACK packet is received within the ack timeout period.
 * Indicating that either the \ref RF_DATA_PACKET did not reach the dongle,
 * or the \ref RF_ACK / \ref RF_NACK packet was lost on the way back.
 *
 * If the \ref RF_ACK_REQUEST flag is not set, the packet will be sent, but no \ref RF_EVENT will occur.
 * \ref rfruIsRadioReady() can be used to check if radio is ready before calling this function.
 *
 * \param[in]  pTxPacket \n
 * A pointer to the \ref RF_DATA_PACKET.
 * In the packet the following fields/flags must be set by the application before calling this function:
 * \li \ref RF_PACKET.length
 * \li \ref RF_PACKET.destAddress
 * \li \ref RF_PACKET.flag only the \ref RF_ACK_REQUEST and \ref RF_DATA_PENDING flags.
 * \li \ref The packets data payload.
 * All other fields will be set automatically by the rfru module before sending the packet.
 *
 * \return BOOL \n
 * If radio is ready, it will return TRUE and start the transmission.
 * Will return FALSE and refuse to handle this call if radio is busy sending or receiving another packet.
 */
BOOL rfruSendDataPacket(RF_PACKET __xdata * pTxPacket)
{
   if(rfruData.radio_state != RFRU_STATE_IDLE) return FALSE;

   //copy source and network address to the packet
   pTxPacket->sourceAddress = rfruData.pTxAckPkt.sourceAddress;
   pTxPacket->networkAddress[0] = rfruData.pTxAckPkt.networkAddress[0];
   pTxPacket->networkAddress[1] = rfruData.pTxAckPkt.networkAddress[1];
   pTxPacket->networkAddress[2] = rfruData.pTxAckPkt.networkAddress[2];

   //set sequence bit
   if(RF_GET_SEQ_BIT(rfruData.pTxSequenceBits, pTxPacket->destAddress)) { pTxPacket->flag &= ~RF_SEQUENCE_BIT; }
   else { pTxPacket->flag |= RF_SEQUENCE_BIT; }
   //set packet type bit
   pTxPacket->flag &= ~RF_PACKET_TYPE;
   pTxPacket->flag |= RF_DATA_PACKET;
   //set if rf state (ack or no ack expected)
   if(pTxPacket->flag & RF_ACK_REQUEST) {
      rfruData.radio_state = RFRU_STATE_TX_PKT_EXPECT_ACK;
      //Setup DMA descriptor for reception of the ack packet
      rfruData.pRxAckPkt->length = 0;
      SET_WORD(rfruData.dmaFromRadioDesc.DESTADDRH, rfruData.dmaFromRadioDesc.DESTADDRL, (BYTE __xdata *) rfruData.pRxAckPkt);
   }
   else { rfruData.radio_state = RFRU_STATE_TX_PKT_NO_ACK; }
   rfruData.lastPktSentTo = pTxPacket->destAddress;
   SET_WORD(rfruData.dmaToRadioDesc.SRCADDRH, rfruData.dmaToRadioDesc.SRCADDRL, (BYTE __xdata *) pTxPacket);
   RF_START_DMA_TX(&rfruData.dmaToRadioDesc);
   STX();
   return TRUE;
}


/**
 * \brief Used to send a \ref RF_DATA_REQUEST to the CC1111/CC2511 USB Dongle,
 * if the dongle has a \ref RF_DATA_PACKET ready it will send it when receiving this request.
 *
 * After successfully calling this function one of the three following \ref RF EVENT will occur.
 *
 * \li RFRU_EVENT_DATA_PACKET_RECEIVED - A new data packet is received and ready to be read from the rx buffer.
 * If the RF_ACK_REQUEST flag is set, it will automatically be acked.
 *
 * \li rfEvent \ref RFRU_EVENT_DATA_REQUEST_NACKED - An NACK packet is received, indicating that the dongle does not
 * have any data packets to this unit. The \ref RF_NACK packet can be found in the rx buffer.
 *
 * \li \ref RFRU_EVENT_DATA_PACKET_REQUEST_TIMEOUT - No reply is received within timeout period.
 * Indicating that either the \ref RF_DATA_REQUEST did not reach the dongle
 * or the reply packet was lost on the way back. The rx buffer is empty.
 *
 * \ref rfruIsRadioReady() can be used to check if radio is ready before calling this function.
 *
 * \param[in] destAdr \n
 * Device address of the dongle the request should be sent to. (Network address is appended automatically)
 *
 * \param[in]  pRxPacketBuffer \n
 * A pointer to the rx buffer where the received packet should be placed.
 *
 * \return BOOL \n
 * Will return FALSE and refuse to handle this call if radio is busy sending or receiving another packet.
 * If radio is ready, it will return TRUE and start the transmission.
 */
BOOL rfruSendDataRequest(BYTE destAdr, RF_PACKET __xdata * pRxPacketBuffer)
{
   if(rfruData.radio_state != RFRU_STATE_IDLE) return FALSE;
   rfruData.pTxDataRequestPkt.destAddress = destAdr;
   rfruData.lastPktSentTo = destAdr;
   rfruData.radio_state = RFRU_STATE_TX_DATA_REQ;
   //Setup DMA descriptor for reception of the reply packet
   pRxPacketBuffer->length = 0;
   SET_WORD(rfruData.dmaFromRadioDesc.DESTADDRH, rfruData.dmaFromRadioDesc.DESTADDRL, (BYTE __xdata *) pRxPacketBuffer);
   rfruData.pRxPacket = pRxPacketBuffer;
   //setup DMA for TX
   SET_WORD(rfruData.dmaToRadioDesc.SRCADDRH, rfruData.dmaToRadioDesc.SRCADDRL, (BYTE __xdata *) &rfruData.pTxDataRequestPkt);
   RF_START_DMA_TX(&rfruData.dmaToRadioDesc);
   STX(); //start TX
   return TRUE;
}


/**
 * \brief Used to send a \ref RF_BIND_REQUEST to the dongle, attempting to bind to it.
 *
 * The remote unit will attempt to bind to the dongle the number of times defined in \ref RF_BIND_REQUEST_RETRIES.
 * After successfully calling this function one of the two following \ref RF EVENT will occur.
 *
 * \li RFRU_EVENT_BIND_SUCCESSFULL - The unit has completed a successful bind to the dongle.
 * The device address of the dongle is found in the eventData parameter in \ref rfruHookRfEvent().
 * The address assigned to this unit can be found by calling \ref rfruGetMyDeviceAddress() and \ref rfruGetNetworkAddress().
 *
 * \li \ref RFRU_EVENT_BIND_REQUEST_TIMEOUT - No reply from the dongle received within timeout period after attempting to bind.
 *
 * \param[in] preferredAddress \n
 * The device address this unit would prefer to be assigned, if application has no preferred address set this parameter to 0x00.
 */
void rfruSendBindRequest(BYTE preferredAddress)
{
   rfruData.bindRequestRetries = 0;
   rfruStopRadio();
   if((preferredAddress != 0) && (preferredAddress != 0xFF)) {
      rfruSetMyAddress(0xFF, 0xFF, 0xFF, preferredAddress);
   } else {
      rfruSetMyAddress(0xFF, 0xFF, 0xFF, 0xFF);
   }
   PKTCTRL1 |= 0x03;//Address check, 0 (0x00) and 255 (0xFF) broadcast
   rfruSBReq();
}


/**
 * \brief Used to put the radio in IDLE.
 *
 * This function is used when the application want to stop the radio.
 * Any ongoing transmissions, both RX and TX will be aborted.
 */
void rfruStopRadio(void)
{
   SIDLE();//set radio to idle
   STOP_RADIO_TIMEOUT();
   rfruData.rfTimeout = FALSE;
   rfruData.radio_state = RFRU_STATE_IDLE;
   DMA_ABORT_CHANNEL(0);
}



/**
 * \brief Can be used to set the address of this device.
 *
 * Use only if application both knows the address it should use,
 * and knows that the dongle will accept packets from this address.
 * This function is called automatically from the rfrd module each time a successful bind has occurred.
 * E.g. if \ref rfruSendBindRequest() is used to bind to the dongle, the application should not call this function.
 *
 * \param[in]  networkAddress0 \n
 * Most significant byte in the network address
 *
 * \param[in]  networkAddress1 \n
 * Second most significant byte in the network address
 *
 * \param[in]  networkAddress2 \n
 * Least significant byte in the network address
 *
 * \param[in]  deviceAddress \n
 * Device address.
 */
void rfruSetMyAddress(BYTE networkAddress0, BYTE networkAddress1, BYTE networkAddress2, BYTE deviceAddress)
{
   rfruData.pNetworkAddress[0] = networkAddress0;
   rfruData.pNetworkAddress[1] = networkAddress1;
   rfruData.pNetworkAddress[2] = networkAddress2;

   //setup ack packet
   rfruData.pTxAckPkt.sourceAddress = deviceAddress; //source address
   rfruData.pTxAckPkt.networkAddress[0] = networkAddress0; //network(2) address
   rfruData.pTxAckPkt.networkAddress[1] = networkAddress1; //network(1) address
   rfruData.pTxAckPkt.networkAddress[2] = networkAddress2; //network(0) address

   //  rfruData.pTxDataRequestPkt
   rfruData.pTxDataRequestPkt.sourceAddress = deviceAddress; //source address
   rfruData.pTxDataRequestPkt.networkAddress[0] = networkAddress0; //network(2) address
   rfruData.pTxDataRequestPkt.networkAddress[1] = networkAddress1; //network(1) address
   rfruData.pTxDataRequestPkt.networkAddress[2] = networkAddress2; //network(0) address

   //setup radio hardware address filter
   ADDR = deviceAddress;
   PKTCTRL1 &= ~0x03;
   PKTCTRL1 |= 0x01;//Address check, no broadcast
}

/**
 * \brief Returns the device address of this device.
 *
 * \param[out]  BYTE \n
 * The device address of this device, returns 0 if no address is yet assigned.
 */
BYTE rfruGetMyDeviceAddress(void)
{
   return ADDR;
}

/**
 * \brief Returns the network address of this device.
 *
 * Use \ref rfruGetMyDeviceAddress() to find out if a valid address is assigned to this device.
 *
 * \return UINT32 \n
 * The network address of this device is found in the 24 least significant bits.
 */
UINT32 rfruGetNetworkAddress(void)
{
   UINT32 nAddr;
   nAddr = rfruData.pNetworkAddress[0];
   nAddr <<= 8;
   nAddr |= rfruData.pNetworkAddress[1];
   nAddr <<= 8;
   nAddr |= rfruData.pNetworkAddress[2];
   return nAddr;
}


/**
 * \brief Returns a pointer to the buffer where received RF_ACK 7 RF_NACK packets are stored.
 *
 * If the application need to check anything in the received \ref RF_ACK / \ref RF_NACK packets,
 * for instance want to read the RSSI value (Received Signal Strength Indicator),
 * this function will return a pointer to where they are stored.
 *
 * \return RF_PACKET \n
 * A pointer to where received RF_ACK /RF_NACK packets are stored
 */
RF_PACKET __xdata * rfruGetpRxAckPkt()
{
   return rfruData.pRxAckPkt;
}

//internal function, do not call from outside of this file
static void rfruSBReq(void)
{
   //use the RxAckPkt buffer, a little slower than having a designated buffer. But this function is seldom used.
   rfruData.pRxAckPkt->length = 6;
   rfruData.pRxAckPkt->destAddress = 0xFF;
   rfruData.pRxAckPkt->sourceAddress = rfruData.pTxAckPkt.sourceAddress;
   rfruData.pRxAckPkt->networkAddress[0] = 0xFF;
   rfruData.pRxAckPkt->networkAddress[1] = 0xFF;
   rfruData.pRxAckPkt->networkAddress[2] = 0xFF;
   rfruData.pRxAckPkt->flag = RF_BIND_REQUEST | RF_ACK_REQUEST;

   rfruData.radio_state = RFRU_STATE_TX_BIND_REQUEST;
   DMA_ABORT_CHANNEL(0);

   //Setup DMA descriptor for reception of the reply packet
   SET_WORD(rfruData.dmaFromRadioDesc.DESTADDRH, rfruData.dmaFromRadioDesc.DESTADDRL, (BYTE __xdata *) rfruData.pRxAckPkt);

   //setup DMA for TX
   SET_WORD(rfruData.dmaToRadioDesc.SRCADDRH, rfruData.dmaToRadioDesc.SRCADDRL, (BYTE __xdata *) rfruData.pRxAckPkt);
   RF_START_DMA_TX(&rfruData.dmaToRadioDesc);
   STX(); //start TX
}

//internal function, do not call from outside of this file
static void rfruHandleUnwantedPacket(void)
{
   if(rfruData.rfTimeout) {
      rfruHandleTimeout();
   } else {
      RF_START_DMA_RX(&rfruData.dmaFromRadioDesc);
      SRX();
   }
}

//internal function, do not call from outside of this file
static void rfruHandleTimeout(void)
{
   switch (rfruData.radio_state)
   {
   case (RFRU_STATE_RX_ACK) :
      rfruStopRadio();
      rfruHookRfEvent(RFRU_EVENT_ACK_TIMEOUT, 0);
      break;

   case (RFRU_STATE_RX_DATA_PACKET) :
      rfruStopRadio();
      rfruHookRfEvent(RFRU_EVENT_DATA_PACKET_REQUEST_TIMEOUT, 0);
      break;

   case (RFRU_STATE_RX_BIND_RESPONSE) :
      rfruStopRadio();

      if(++rfruData.bindRequestRetries < RF_BIND_REQUEST_RETRIES) {
         rfruSBReq();
      } else {
         rfruSetMyAddress(0xFF, 0xFF, 0xFF, 0xFF);
         rfruHookRfEvent(RFRU_EVENT_BIND_REQUEST_TIMEOUT, 0);
      }
      break;
   }
}

//internal function, do not call from outside of this file
static BYTE handleDataPacket(void)
{
   BYTE callbackEvent;
   if(rfruData.pRxPacket->flag & RF_ACK_REQUEST){
      rfruData.pTxAckPkt.destAddress = rfruData.pRxPacket->sourceAddress;
      rfruData.pTxAckPkt.flag = 0;


      if(RF_GET_SEQ_BIT(rfruData.pRxSequenceBits, rfruData.pRxPacket->sourceAddress) != (rfruData.pRxPacket->flag & RF_SEQUENCE_BIT)) {
         //if sequence bit is ok packet is acknowledged. Toggle sequence bit and send packet received callback
         RF_TOGGLE_SEQ_BIT(rfruData.pRxSequenceBits, rfruData.pRxPacket->sourceAddress);
         rfruData.pTxAckPkt.flag |= RF_ACK;  //send ack
         callbackEvent = RFRU_EVENT_DATA_PACKET_RECEIVED;
      }
      else {//a retransmission
         rfruData.pTxAckPkt.flag |= RF_ACK;  //send ack
         callbackEvent = RFRU_EVENT_RETRANSMSSION_RECEIVED;
      }
      rfruData.radio_state = RFRU_STATE_TX_ACK_NACK;

      //setup DMA for TX and start TX
      SET_WORD(rfruData.dmaToRadioDesc.SRCADDRH, rfruData.dmaToRadioDesc.SRCADDRL, (BYTE __xdata *) &rfruData.pTxAckPkt);
      RF_START_DMA_TX(&rfruData.dmaToRadioDesc);
      STX();
   }
   else { //no ack requested, disregard sequence bit.
      rfruData.radio_state = RFRU_STATE_IDLE;
      callbackEvent = RFRU_EVENT_DATA_PACKET_RECEIVED;
   }
   return callbackEvent;
}




#pragma vector=RF_VECTOR
__interrupt void rfIrq(void)
{

   BYTE callBack;

   INT_ENABLE(INUM_T2, INT_OFF);//neccessary because rfruData.radio_state is changed both here (RF ISR) and in timer2 ISR
   INT_SETFLAG(INUM_RF, INT_CLR);    // Clear MCU interrupt flag
   //Check if error has occurred
   if(RFIF & (IRQ_TXUNF | IRQ_RXOVF) ) {//rx overflow or tx underflow
      SIDLE();
      DMA_ABORT_CHANNEL(0);
      rfruData.radio_state = RFRU_STATE_IDLE;
      STOP_RADIO_TIMEOUT();
      rfruData.rfTimeout = FALSE;
      RFIF = 0;
   }
   //check if radio has finished receiving or sending a packet.
   else if(RFIF & IRQ_DONE)
   {
      RFIF = ~IRQ_DONE;
      SIDLE();
      //check the rfruData.radio_state
      switch(rfruData.radio_state)
      {
      // *********** rfruData.radio_state indication radio was in TX, e.g. a packet is sent ***********

         //finished sending a packet, does not expect a ansver
      case (RFRU_STATE_TX_ACK_NACK) :
      case (RFRU_STATE_TX_PKT_NO_ACK) :
         rfruData.radio_state = RFRU_STATE_IDLE;
         break;

         //sent packet, expect ack
      case (RFRU_STATE_TX_PKT_EXPECT_ACK) :
         rfruData.radio_state = RFRU_STATE_RX_ACK;
         PKTLEN = 6; //setup max packet length to avoid overflowing the ack packet buffer
         RF_START_DMA_RX(&rfruData.dmaFromRadioDesc);
         SRX(); //start rx
         rfruData.rfTimeout = FALSE;
         START_RADIO_TIMEOUT(rfruData.timer2Cnt); //start ack timeout
         break;

         //sent data request packet, expect data packet back
      case (RFRU_STATE_TX_DATA_REQ) :
         rfruData.radio_state = RFRU_STATE_RX_DATA_PACKET; //set radio state
         PKTLEN = rfruData.maxPacketLength; //setup max packet length to avoid overflowing the rx packet buffer
         RF_START_DMA_RX(&rfruData.dmaFromRadioDesc);
         SRX(); //start rx
         rfruData.rfTimeout = FALSE;
         START_RADIO_TIMEOUT(rfruData.timer2Cnt); //start packet/NAKC timeout
         break;

         //sent a bind request, expect a bind response
      case (RFRU_STATE_TX_BIND_REQUEST) :
         rfruData.radio_state = RFRU_STATE_RX_BIND_RESPONSE; //set radio state
         PKTLEN = 7; //setup max packet length to avoid overflowing the ack packet buffer
         rfruData.pRxAckPkt->length = 0;
         RF_START_DMA_RX(&rfruData.dmaFromRadioDesc);
         SRX(); //start rx
         rfruData.rfTimeout = FALSE;
         START_RADIO_TIMEOUT(rfruData.timer2Cnt); //start bind reply timeout
         break;


      // *********** rfruData.radio_state indication radio was in RX, e.g. a packet is received ***********

      case (RFRU_STATE_RX_DATA_PACKET) :
         //verify length, CRC, and address in packet.
         if((rfruData.pRxPacket->length < 6) || !(CHECK_CRC(rfruData.pRxPacket)) || !(CHECK_NETW_ADDR(rfruData.pRxPacket, rfruData.pNetworkAddress)) || (rfruData.pRxPacket->sourceAddress != rfruData.lastPktSentTo))
         {
            rfruHandleUnwantedPacket(); // a unwanted packet, go back to RX or timeout
            INT_ENABLE(INUM_T2, INT_ON);//turn on timeout interrupt againg
         }
         //packet is valid, check packet type on the received packet
         else if((rfruData.pRxPacket->flag & RF_PACKET_TYPE) == RF_DATA_PACKET)
         {
            // a incoming data packet.
            STOP_RADIO_TIMEOUT();
            rfruData.rfTimeout = FALSE;
            callBack = handleDataPacket();
            rfruHookRfEvent(callBack, 0);
         }
         else if((rfruData.pRxPacket->flag & RF_PACKET_TYPE) == RF_NACK)
         {
            //a nack packet is received (can happen after sending a data request)
            STOP_RADIO_TIMEOUT();
            rfruData.rfTimeout = FALSE;
            rfruData.radio_state = RFRU_STATE_IDLE;
            rfruHookRfEvent(RFRU_EVENT_DATA_REQUEST_NACKED, rfruData.pRxPacket->flag);
         }
         else
         {
            rfruHandleUnwantedPacket(); // a unwanted packet, go back to RX or timeout
            INT_ENABLE(INUM_T2, INT_ON);//turn on timeout interrupt againg
         }
         break; //case (RFRU_STATE_RX_DATA_PACKET)

         // expect a ack/nack packet
      case (RFRU_STATE_RX_ACK) :

         //verify CRC, and address in packet.
         if((rfruData.pRxAckPkt->length != 6) || !(CHECK_CRC(rfruData.pRxAckPkt)) || !(CHECK_NETW_ADDR(rfruData.pRxAckPkt, rfruData.pNetworkAddress)) || (rfruData.pRxAckPkt->sourceAddress != rfruData.lastPktSentTo))
         {
            rfruHandleUnwantedPacket(); // a unwanted packet, go back to RX or timeout
            INT_ENABLE(INUM_T2, INT_ON);//turn on timeout interrupt againg
         }
         else if((rfruData.pRxAckPkt->flag & RF_PACKET_TYPE) == RF_ACK)
         {
            STOP_RADIO_TIMEOUT();
            rfruData.rfTimeout = FALSE;
            rfruData.radio_state = RFRU_STATE_IDLE;
            RF_TOGGLE_SEQ_BIT(rfruData.pTxSequenceBits, rfruData.pRxAckPkt->sourceAddress); //toggle sequence bit
            rfruHookRfEvent(RFRU_EVENT_ACK_RECEIVED, rfruData.pRxAckPkt->flag);
         }
         else if((rfruData.pRxAckPkt->flag & RF_PACKET_TYPE) == RF_NACK)
         {
            STOP_RADIO_TIMEOUT();
            rfruData.rfTimeout = FALSE;
            rfruData.radio_state = RFRU_STATE_IDLE;
            rfruHookRfEvent(RFRU_EVENT_NACK_RECEIVED, rfruData.pRxAckPkt->flag);
         }
         else{
            rfruHandleUnwantedPacket(); // a unwanted packet, go back to RX or timeout
            INT_ENABLE(INUM_T2, INT_ON);//turn on timeout interrupt againg
         }
         break;//case (RFRU_STATE_RX_ACK)

      case (RFRU_STATE_RX_BIND_RESPONSE) :
         if ((rfruData.pRxAckPkt->length != 7) || CHECK_CRC(rfruData.pRxAckPkt) && ((rfruData.pRxAckPkt->flag & RF_PACKET_TYPE) == RF_BIND_RESPONSE))
         {
            STOP_RADIO_TIMEOUT();
            rfruData.rfTimeout = FALSE;
            rfruData.radio_state = RFRU_STATE_IDLE; //set rfruData.radio_state
            rfruSetMyAddress(rfruData.pRxAckPkt->networkAddress[0], rfruData.pRxAckPkt->networkAddress[1], rfruData.pRxAckPkt->networkAddress[2], rfruData.pRxAckPkt->data[0]);
            RF_CLR_SEQ_BIT(rfruData.pRxSequenceBits, rfruData.pTxAckPkt.destAddress);//clear the sequence bit, both rx
            RF_CLR_SEQ_BIT(rfruData.pTxSequenceBits, rfruData.pTxAckPkt.destAddress);//...and tx
            rfruHookRfEvent(RFRU_EVENT_BIND_SUCCESSFULL, rfruData.pRxAckPkt->sourceAddress);
         }
         else
         {
            rfruHandleUnwantedPacket(); // a unwanted packet, go back to RX or timeout
            INT_ENABLE(INUM_T2, INT_ON);//turn on timeout interrupt againg
         }
         break;
      default:
         SIDLE();
         DMA_ABORT_CHANNEL(0);
         rfruData.radio_state = RFRU_STATE_IDLE;
         break;
      }//switch(rfruData.radio_state)
   }
}


//timer 2 / MAC timer Interrupt Service Routine.
//MAC timer is used to timeout ack/nack messages and other replies.
#pragma vector=T2_VECTOR
__interrupt void t2Irq(void)
{
   INT_ENABLE(INUM_RF, INT_OFF);//neccessary because radio state change is done both here and in RF ISR
   //check if we have started receiving or finished receiving a packet
   if(PKTSTATUS & 0x88){
      rfruData.rfTimeout = TRUE;
   }
   else{
      //No SFD found or packet received.
      rfruHandleTimeout();
   }
   INT_ENABLE(INUM_RF, INT_ON);
}

/// @}
