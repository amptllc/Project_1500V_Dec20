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
/// \addtogroup module_rfud
/// @{
#include "rf_usb_app_ex_lib_headers.h"
#include "rf\include\rf_usb_dongle.h"

static BYTE __xdata AckPkt[10];//must be 10 to hold CRC
static RFUD_DATA __xdata rfudData;///internal module data

////internal functions, do not call from outside of this file
static rfudResetRadio(void);
static rfudSendDataPacket(RF_PACKET __xdata * pTxPacket);
static BYTE handleDataPacket(void);


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
void rfudInit(BYTE bufferSize){
   BYTE i, temp;
   //set rx ack packet Ptr
   rfudData.pRxAckPkt = (RF_PACKET __xdata *) AckPkt;
   //setup radio, and calibrate
   halRfConfig(RF_FREQUENCY);
   //calibrate when going to RX/TX
   MCSM0 = 0x10;
   rfudData.maxPacketLength = bufferSize - 3;
   PKTLEN = rfudData.maxPacketLength;
   MCSM1 = 0x00;
   rfudData.rxBufferNeeded = TRUE;

   rfudData.pTxAckPkt.length = 6;//length of tx ack packet

   for(i = 0; i < RF_SEQUENCE_BYTES; i++)
   {
      rfudData.pTxSequenceBits[i] = 0;
      rfudData.pRxSequenceBits[i] = 0;
   }
   //setup all fields in DMA descriptor for RX that newer change
   SET_WORD(rfudData.dmaFromRadioDesc.SRCADDRH, rfudData.dmaFromRadioDesc.SRCADDRL, &X_RFD);
   //SET_WORD(rfudData.dmaFromRadioDesc.DESTADDRH, rfudData.dmaFromRadioDesc.DESTADDRL, pRxBuffer);
   rfudData.dmaFromRadioDesc.VLEN       = VLEN_1_P_VALOFFIRST_P_2;
   SET_WORD(rfudData.dmaFromRadioDesc.LENH, rfudData.dmaFromRadioDesc.LENL, bufferSize);
   rfudData.dmaFromRadioDesc.WORDSIZE   = WORDSIZE_BYTE;
   rfudData.dmaFromRadioDesc.TMODE      = TMODE_SINGLE;
   rfudData.dmaFromRadioDesc.TRIG       = DMATRIG_RADIO;
   rfudData.dmaFromRadioDesc.DESTINC    = DESTINC_1;
   rfudData.dmaFromRadioDesc.SRCINC     = SRCINC_0;
   rfudData.dmaFromRadioDesc.IRQMASK    = IRQMASK_DISABLE;
   rfudData.dmaFromRadioDesc.M8         = M8_USE_8_BITS;
   rfudData.dmaFromRadioDesc.PRIORITY   = PRI_GUARANTEED;

   //setup all fields in DMA descriptor for TX that newer change
   SET_WORD(rfudData.dmaToRadioDesc.DESTADDRH, rfudData.dmaToRadioDesc.DESTADDRL, &X_RFD);
   rfudData.dmaToRadioDesc.VLEN       = VLEN_1_P_VALOFFIRST;
   SET_WORD(rfudData.dmaToRadioDesc.LENH, rfudData.dmaToRadioDesc.LENL, 256);
   rfudData.dmaToRadioDesc.WORDSIZE   = WORDSIZE_BYTE;
   rfudData.dmaToRadioDesc.TMODE      = TMODE_SINGLE;
   rfudData.dmaToRadioDesc.TRIG       = DMATRIG_RADIO;
   rfudData.dmaToRadioDesc.DESTINC    = DESTINC_0;
   rfudData.dmaToRadioDesc.SRCINC     = SRCINC_1;
   rfudData.dmaToRadioDesc.IRQMASK    = IRQMASK_DISABLE;
   rfudData.dmaToRadioDesc.M8         = M8_USE_8_BITS;
   rfudData.dmaToRadioDesc.PRIORITY   = PRI_GUARANTEED;

   rfudData.radio_state = RFUD_STATE_IDLE;
   RFIF &= ~(IRQ_DONE | IRQ_TXUNF | IRQ_RXOVF);
   RFIM |= (IRQ_DONE | IRQ_TXUNF | IRQ_RXOVF);
   INT_ENABLE(INUM_RF, INT_ON);

   // setup timer 2, used for ACK timeout
   halSetTimer2Period(3000, &temp, &i);
   rfudData.timer2Cnt = temp;
   T2CT = 0;
   T2CTL = 0x10;
   INT_ENABLE(INUM_T2, INT_ON);
   INT_GLOBAL_ENABLE(INT_ON);
}

/**
 * \brief Set the device address and network address on the rf_usb_dongle unit.
 *
 * Must be called once, after calling the \ref rfudInit().
 * Can be called later if the dongle want to change its address.
 * Normally the dongle will have device address 0x01, but this is up to the application.
 * The address can be seen as a single 32 bit address, however only devices with identical
 * network address can communicate with each other.
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
void rfudSetMyAddress(BYTE networkAddress0, BYTE networkAddress1, BYTE networkAddress2, BYTE deviceAddress)
{
   rfudData.pNetworkAddress[0] = networkAddress0;
   rfudData.pNetworkAddress[1] = networkAddress1;
   rfudData.pNetworkAddress[2] = networkAddress2;

   //setup ack packet
   rfudData.pTxAckPkt.sourceAddress = deviceAddress; //source address
   rfudData.pTxAckPkt.networkAddress[0] = networkAddress0; //network(2) address
   rfudData.pTxAckPkt.networkAddress[1] = networkAddress1; //network(1) address

   rfudData.pTxAckPkt.networkAddress[2] = networkAddress2; //network(0) address

   //setup radio hardware address filter
   ADDR = deviceAddress;
   PKTCTRL1 |= 0x03;//Address check, 0 (0x00) and 255 (0xFF) broadcast
}


/**
 * \brief Used to check if the radio have a RX buffer, or if one need to be supplied.
 *
 * It is the applications responsibility to supply the rfud module with RX buffers when needed.
 * A RX buffer will be needed initially when first starting the radio, and then after each received packet.
 * If the rfud module does not have a RX buffer, the radio will be put in IDLE until one
 * is supplied.
 * This function should be included in the Main loop to regularly check if a buffer is needed.
 * If a buffer is needed, use the \ref rfudStartReceive() function to supply the rfud module
 * with a RX buffer.
 *
 * \note The buffer supplied must be at least as big as specified in the \ref rfudInit() bufferSize.
 * \return  BOOL \n
 * If \c TRUE the rfud module does not have a RX buffer, one must be supplied. \n
 * If \c FALSE the rfud module already have a RX buffer.
 */
BOOL rfudRxBufferNeeded(void)
{
   return (rfudData.rxBufferNeeded);
}


/**
 * \brief Gives the rfud module a RX buffer, and put the radio in RX.
 *
 * This function should be called when \ref rfudRxBufferNeeded() indicates that a new
 * RX buffer is needed by the radio.
 * If the rfud module does not have a RX buffer, the radio will be put in IDLE until one
 * is supplied.
 * This function will not accept a new buffer if the rfud module already have one, (will return FALSE).
 * If for some reason the buffer currently in use by the rfud module should not be used
 * any longer, call \ref rfudStopRadio(). The old buffer will then be discarded and a new
 * buffer can then be given to the rfud module with this function.
 *
 * \note The buffer supplied must be at least as big as specified in the \ref rfudInit() bufferSize.
 *
 * \param[in]  pRxPacketBuffer \n
 * A pointer to the RX buffer.
 *
 * \return  BOOL \n
 * If the rfud module already have a RX buffer this function will return FALSE. And the
 * rfud module will continue to use the old buffer. \n
 * If the rfud module did not have a RX buffer, this function will return TRUE, and the
 * supplied buffer will be used.
 */
BOOL rfudStartReceive(RF_PACKET __xdata * pRxPacketBuffer)
{
   //if we already have a RX buffer, dont change it
   if(!(rfudData.rxBufferNeeded)) return FALSE;

   INT_ENABLE(INUM_RF, INT_OFF);//neccessary because radio state change is done both here, in and in RF ISR
   pRxPacketBuffer->length = 0;
   rfudData.pRxPacket = pRxPacketBuffer;
   rfudData.rxBufferNeeded = FALSE;
   if(rfudData.radio_state == RFUD_STATE_IDLE) {
      rfudResetRadio();
   }
   INT_ENABLE(INUM_RF, INT_ON);

   return TRUE;
}


/**
 * \brief Used to put the radio in IDLE, and discard the RX buffer currently in use.
 *
 * This function is used when the application want to stop the radio, for example before
 * entering USB suspend.
 * Any ongoing transmissions, both RX and TX will be aborted. The current RX buffer
 * will be discarded, so a new one must be given using \ref rfudStartReceive() to start the
 * radio again.
 */
void rfudStopRadio(void)
{
   //set radio to idle
   SIDLE();
   RFIF = 0;
   INT_SETFLAG(INUM_RF, INT_CLR);

   STOP_RADIO_TIMEOUT();

   //set radio state
   rfudData.radio_state = RFUD_STATE_IDLE;

   //clear RX buffer
   rfudData.rxBufferNeeded = TRUE;

   //Abort any DMA transfers
   DMA_ABORT_CHANNEL(0);
}

//internal function, do not call from outside of this file
static rfudResetRadio(void)
{

   SIDLE();
   RFIF = 0;
   INT_SETFLAG(INUM_RF, INT_CLR);

   //set radio state
   if(rfudData.rxBufferNeeded) { rfudData.radio_state = RFUD_STATE_IDLE; }
   else {
      rfudData.radio_state = RFUD_STATE_GENERAL_RX;
      PKTLEN = rfudData.maxPacketLength;//setup max packet length
      //Setup DMA descriptor for reception of the packet
      rfudData.pRxPacket->length = 0;
      SET_WORD(rfudData.dmaFromRadioDesc.DESTADDRH, rfudData.dmaFromRadioDesc.DESTADDRL, (BYTE __xdata *) rfudData.pRxPacket);
      RF_START_DMA_RX(&rfudData.dmaFromRadioDesc);
      SRX();
   }
}

//internal function, do not call from outside of this file
static rfudSendDataPacket(RF_PACKET __xdata * pTxPacket)
{
   SIDLE();
   //copy source and network address to the packet
   pTxPacket->sourceAddress = rfudData.pTxAckPkt.sourceAddress;
   pTxPacket->networkAddress[0] = rfudData.pTxAckPkt.networkAddress[0];
   pTxPacket->networkAddress[1] = rfudData.pTxAckPkt.networkAddress[1];
   pTxPacket->networkAddress[2] = rfudData.pTxAckPkt.networkAddress[2];

   //set sequence bit
   if(RF_GET_SEQ_BIT(rfudData.pTxSequenceBits, pTxPacket->destAddress)) { pTxPacket->flag &= ~RF_SEQUENCE_BIT; }
   else { pTxPacket->flag |= RF_SEQUENCE_BIT; }

   //set packet type bit
   pTxPacket->flag &= ~RF_PACKET_TYPE;
   pTxPacket->flag |= RF_DATA_PACKET;

   //set if rf state (ack or nack expected)
   if(pTxPacket->flag & RF_ACK_REQUEST) {
      rfudData.radio_state = RFUD_STATE_TX_PKT_EXPECT_ACK;
      //Setup DMA descriptor for reception of the ack packet
      rfudData.pRxAckPkt->length = 0;
      SET_WORD(rfudData.dmaFromRadioDesc.DESTADDRH, rfudData.dmaFromRadioDesc.DESTADDRL, (BYTE __xdata *) rfudData.pRxAckPkt);
   }
   else { rfudData.radio_state = RFUD_STATE_TX_PKT_NO_ACK; }

   rfudData.lastPktSentTo = pTxPacket->destAddress;
   //setup DMA
   SET_WORD(rfudData.dmaToRadioDesc.SRCADDRH, rfudData.dmaToRadioDesc.SRCADDRL, (BYTE __xdata *) pTxPacket);
   RF_START_DMA_TX(&rfudData.dmaToRadioDesc);
   //start TX
   STX();
}

//internal function, do not call from outside of this file
static BYTE handleDataPacket(void)
{
   BYTE callbackEvent = 0;
   if(rfudData.pRxPacket->flag & RF_ACK_REQUEST){
      rfudData.pTxAckPkt.length = 6;
      rfudData.pTxAckPkt.destAddress = rfudData.pRxPacket->sourceAddress;
      rfudData.pTxAckPkt.flag = 0;

      //if sequence bit is ok
      if(RF_GET_SEQ_BIT(rfudData.pRxSequenceBits, rfudData.pRxPacket->sourceAddress) != (rfudData.pRxPacket->flag & RF_SEQUENCE_BIT)) {
         //ask application if packet should be acked.
         if(rfudHookAckPkt(rfudData.pRxPacket->sourceAddress, &rfudData.pTxAckPkt.flag)) {
            //if packet is acknowledged, toggle sequence bit and send packet received callback
            RF_TOGGLE_SEQ_BIT(rfudData.pRxSequenceBits, rfudData.pRxPacket->sourceAddress);

            rfudData.pTxAckPkt.flag |= RF_ACK;  //send ack
            rfudData.radio_state = RFUD_STATE_TX_ACK;
            callbackEvent = RFUD_EVENT_DATA_PACKET_RECEIVED;
         }
         //packet is nacked, dont send packet received callback, just finish the nack sending and go back to rx
         else {
            rfudData.pTxAckPkt.flag |= RF_NACK;//send nack
            rfudData.radio_state = RFUD_STATE_TX_NACK;
         }
      }
      else {//a retransmission
         rfudData.pTxAckPkt.flag |= RF_ACK;  //send ack
         rfudData.radio_state = RFUD_STATE_TX_ACK;
      }
      //setup DMA for TX
      SET_WORD(rfudData.dmaToRadioDesc.SRCADDRH, rfudData.dmaToRadioDesc.SRCADDRL, &rfudData.pTxAckPkt);
      RF_START_DMA_TX(&rfudData.dmaToRadioDesc);
      STX();
   }
   else { //no ack requested, disregard sequence bit.
      rfudData.radio_state = RFUD_STATE_IDLE;
      rfudData.rxBufferNeeded = TRUE;
      callbackEvent = RFUD_EVENT_DATA_PACKET_RECEIVED;
   }
   return callbackEvent;
}




#pragma vector=RF_VECTOR
__interrupt void rfIrq(void)
{

   BYTE callBack;
   RF_PACKET __xdata * pTxPacket;
   INT_SETFLAG(INUM_RF, INT_CLR);    // Clear MCU interrupt flag
   INT_ENABLE(INUM_T2, INT_OFF);//neccessary because rfudData.radio_state is changed both here (RF ISR) and in timer2 ISR

   if(RFIF & (IRQ_TXUNF | IRQ_RXOVF) ) {//rx overflow or tx underflow
      STOP_RADIO_TIMEOUT();
      rfudResetRadio();
      RFIF = 0;
   }
   //check if radio has finished receiving or sending a packet.
   else if(RFIF & IRQ_DONE)
   {
      RFIF = ~IRQ_DONE;
      SIDLE();
      //check the rfudData.radio_state
      switch(rfudData.radio_state)
      {
         //finished sending a packet that does not expect any answer
      case (RFUD_STATE_TX_ACK) :
      case (RFUD_STATE_TX_PKT_NO_ACK) :
      case (RFUD_STATE_TX_NACK) :
      case (RFUD_STATE_TX_BIND_RESPONSE) :
         rfudResetRadio();
         break;
         //sent packet, expect ack
      case (RFUD_STATE_TX_PKT_EXPECT_ACK) :
         rfudData.radio_state = RFUD_STATE_RX_ACK;
         //setup max packet length to avoid overflowing the ack packet buffer
         PKTLEN = 6;
         //start rx
         RF_START_DMA_RX(&rfudData.dmaFromRadioDesc);
         SRX();
         //start ack timeout
         START_RADIO_TIMEOUT(rfudData.timer2Cnt);
         break;
      case (RFUD_STATE_GENERAL_RX) :
         //verify length and CRC
         if((rfudData.pRxPacket->length < 6) || !(CHECK_CRC(rfudData.pRxPacket)))
         {
            //packet not valid, go back to RX.
            rfudResetRadio();
         }
         //if correct network address
         else if(CHECK_NETW_ADDR(rfudData.pRxPacket, rfudData.pNetworkAddress))
         {
            //packet is valid, check packet type on the received packet
            switch (rfudData.pRxPacket->flag & RF_PACKET_TYPE) {
            // a incoming data packet.
            case (RF_DATA_PACKET) :
               callBack = handleDataPacket();
               if(callBack)
               {
                  rfudData.rxBufferNeeded = TRUE; //RX buffer is no longer available
                  rfudHookRfEvent(callBack, rfudData.pRxPacket->sourceAddress);
               }
               break;
            // a incoming data request.
            case (RF_DATA_REQUEST) :
               rfudData.radio_state = RFUD_STATE_IDLE;
               //send a callback to application to find out what to do
               pTxPacket = rfudHookDataPktRequest(rfudData.pRxPacket->sourceAddress);
               //if a data packet should be sent
               if(pTxPacket) {
                  pTxPacket->destAddress = rfudData.pRxPacket->sourceAddress;//set destination address.
                  rfudSendDataPacket(pTxPacket);
               }
               //if not send a NACK data packet
               else {
                  rfudData.pTxAckPkt.flag = RF_NACK;
                  rfudData.pTxAckPkt.length = 6;
                  rfudData.pTxAckPkt.destAddress = rfudData.pRxPacket->sourceAddress;
                  rfudData.radio_state = RFUD_STATE_TX_NACK;
                  //setup DMA
                  SET_WORD(rfudData.dmaToRadioDesc.SRCADDRH, rfudData.dmaToRadioDesc.SRCADDRL, (BYTE __xdata *) &rfudData.pTxAckPkt);
                  RF_START_DMA_TX(&rfudData.dmaToRadioDesc);
                  //start TX
                  STX();
               }
               break;
               // a unwanted packet, go back to RX
            default:
               rfudResetRadio();
               break;
            }
         }
         //Check if packet is a bind request, and the RSSI value is high enough.
         else if(CHECK_IF_BIND_REQUEST(rfudData.pRxPacket))
         {
            //send a callback to application, asking if a reply should be sent and if so what address the unit should get.
            rfudData.pTxAckPkt.data[0] = rfudHookBindRequest(rfudData.pRxPacket->sourceAddress);
            //if application wants to send a bind response
            if(rfudData.pTxAckPkt.data[0])
            {
               //setup bind respond packet, use rfudData.pTxAckPkt to save memory.
               rfudData.pTxAckPkt.length = 7;
               rfudData.pTxAckPkt.destAddress = 0xFF;
               rfudData.pTxAckPkt.flag = RF_BIND_RESPONSE;

               rfudData.radio_state = RFUD_STATE_TX_BIND_RESPONSE;

               //setup DMA for TX
               SET_WORD(rfudData.dmaToRadioDesc.SRCADDRH, rfudData.dmaToRadioDesc.SRCADDRL, (BYTE __xdata *) &rfudData.pTxAckPkt);
               RF_START_DMA_TX(&rfudData.dmaToRadioDesc);
               STX();
               //clear the sequence bit, both rx and tx
               RF_CLR_SEQ_BIT(rfudData.pRxSequenceBits, rfudData.pTxAckPkt.data[0]);
               RF_CLR_SEQ_BIT(rfudData.pTxSequenceBits, rfudData.pTxAckPkt.data[0]);
            }
            else
            {  // dont want to bind, go back to RX
               rfudResetRadio();
            }
         }
         else
         {  // a unwanted packet, go back to RX
            rfudResetRadio();
         }
         break; //case (RFUD_STATE_GENERAL_RX)
         // expect a ack/nack packet
      case (RFUD_STATE_RX_ACK) :
         //verify length, CRC, and address in packet.
         if((rfudData.pRxAckPkt->length == 6) && CHECK_CRC(rfudData.pRxAckPkt) && CHECK_NETW_ADDR(rfudData.pRxAckPkt, rfudData.pNetworkAddress) && (rfudData.pRxAckPkt->sourceAddress == rfudData.lastPktSentTo))
         {
            if((rfudData.pRxAckPkt->flag & RF_PACKET_TYPE) == RF_ACK)
            {
               STOP_RADIO_TIMEOUT();
               rfudResetRadio();
               //toggle sequence bit
               RF_TOGGLE_SEQ_BIT(rfudData.pTxSequenceBits, rfudData.pRxAckPkt->sourceAddress);
               rfudHookRfEvent(RFUD_EVENT_ACK_RECEIVED, rfudData.pRxAckPkt->sourceAddress);
            }
            else if((rfudData.pRxAckPkt->flag & RF_PACKET_TYPE) == RF_NACK)
            {
               STOP_RADIO_TIMEOUT();
               rfudResetRadio();
               rfudHookRfEvent(RFUD_EVENT_NACK_RECEIVED, rfudData.pRxAckPkt->sourceAddress);
            }
            // a unwanted packet, go back to RX
            else{
               rfudResetRadio();
               INT_ENABLE(INUM_T2, INT_ON);//Turn timer 2 interrupt back on to enable timeout
            }
         }else
         {  //packet not valid, go back to RX.
            rfudResetRadio();
            INT_ENABLE(INUM_T2, INT_ON);//Turn timer 2 interrupt back on to enable timeout
         }
         break;//case (RFUD_STATE_RX_ACK)
      default :
         rfudResetRadio();
         break;
      }//switch(rfudData.radio_state)
   }
}

//timer 2 / MAC timer Interrupt Service Routine.
//MAC timer is used to timeout ack/nack messages and other replies.
#pragma vector=T2_VECTOR
__interrupt void t2Irq(void)
{
   INT_ENABLE(INUM_RF, INT_OFF);//neccessary because radio state change is done both here and in RF ISR
   if(rfudData.radio_state == RFUD_STATE_RX_ACK)
   {
      if(!(PKTSTATUS & 0x88)){
         rfudHookRfEvent(RFUD_EVENT_ACK_TIMEOUT, rfudData.lastPktSentTo);
         rfudResetRadio();
      }
   }
   INT_ENABLE(INUM_RF, INT_ON);
}
/// @}
