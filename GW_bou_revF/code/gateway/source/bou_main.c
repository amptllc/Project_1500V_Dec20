/*==== DECLARATION CONTROL ===================================================*/
/*==== INCLUDES ==============================================================*/
#include "hal_main.h"
#include "main.h"
#include "usb_cdc.h"

#include "usb_interrupt.h"
#include "rf_usb_library_headers.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define USB_RX_TIMEOUT           10

CDC_LINE_CODING_STRUCTURE lineCoding;

// Other global variables
extern BYTE radioPktBufferRx[PACKET_LENGTH_ED + 3]; // to accomodate maximum possible package PACKET_LENGTH_ED + 3];     // Buffer for packets to send or receive, sized to match the receiver's needs
extern BYTE radioPktBuffer  [4][PACKET_LENGTH_ED + 3];  // Buffer for packets to send or receive, sized to match the receiver's needs
extern BYTE radioPktBufferTx[PACKET_LENGTH_GW + 3];     // Buffer for packets to send or receive, sized to match the receiver's needs

void setupTimer1(void);
void handleStdUsb(void);
void parseCommand(void); 

#define OUT_BUF_SIZE 512
BYTE output_buffer[ OUT_BUF_SIZE ];
BYTE temp_buff[ 128 ];
BYTE inBuffIdx = 0;
BOOL needCommandParsing = FALSE, echoEnabled = TRUE;
BYTE secondsAcceptingC = 0;

BOOL  readHexArray ( BYTE *arr, BYTE *ptr, BYTE l);
//BYTE  ch2int       ( BYTE ch );
BYTE* rollNW       ( BYTE *ptr );
BYTE* rollW        ( BYTE *ptr );
BYTE* roll2next    ( BYTE *ptr );
void  usb          ( BYTE *s );
//BYTE* printHex     ( BYTE hex, BYTE *ptr );

void _mymemset( BYTE *buff,        BYTE what,    UINT16 counter ){ while( counter-- ) *buff++ = what; }
#define mymemset( a, b, c ) _mymemset( (BYTE *)a, b, c )
void _mymemcpy( BYTE *destination, BYTE *source, BYTE counter ){ while( counter-- ) *destination++ = *source++; }
#define mymemcpy( a, b, c ) _mymemcpy( (BYTE *)a, (BYTE *)b, c )
BOOL _mymemcmp( BYTE *destination, BYTE *source, BYTE counter ){ while( counter-- ) if(*destination++ != *source++) return 1; return 0; }
#define mymemcmp( a, b, c ) _mymemcmp( (BYTE *)a, (BYTE *)b, c )
void clear_inbuff( void ){ mymemset( exchange_block->inBuff, 0, 64 );   inBuffIdx = 0; }

void usb_out(  void );
void start_main_image( void );

extern void dmaSetup( void );

extern void refreshTheFlash(void);
extern void clearThePage(BYTE page);
extern BOOL initiateTransfer(BYTE *ptr, BYTE *addr, UINT16 len);

/*==== PUBLIC FUNCTIONS ======================================================*/
/******************************************************************************
* @fn  main
* @brief      Main function. Triggers setup menus and main loops for both receiver
*             and transmitter. This function supports both CC1110 and CC2510.
* Parameters:
* @param  void
* @return void
******************************************************************************/
void bou_loop( void );
static SchedulerInterface * exchange_block;
extern DMA_DESC dmaConfig[3]; 
void main(void){
   exchange_block = (SchedulerInterface __xdata *)EXCHANGE_BUFFER; 
   mymemset( (BYTE *)exchange_block, 0, sizeof( SchedulerInterface ) );

   exchange_block->ticks[ Ms ]              =  TICKS_IN_MS;
 
   P1SEL &= 0xFD;  P1DIR |= 0x02; P1 |= 0x02; 
   WDCTL = 0x8; //releaseTheDog();
   
   //___________________________________________________________________________________________________________________________
   CLKCON = 0x80;                // Enable XOSC
   while ((SLEEP & 0x40) == 0);  // Wait until XOSC/USB clock has stabalized

   //set default line coding.
#ifdef windows
   lineCoding.dteRate    = 115200;
#else
   lineCoding.dteRate    = 230400;
#endif
   lineCoding.charFormat = CDC_CHAR_FORMAT_1_STOP_BIT;
   lineCoding.parityType = CDC_PARITY_TYPE_NONE;
   lineCoding.dataBits   = 8;

   dmaSetup();
   setupTimer1();   

   //___________________________________________________________________________________________________________________________
   // Choose the crystal oscillator as the system clock
   halPowerClkMgmtSetMainClkSrc(CRYSTAL);

   // Configure interrupt for every received packet
   //___________________________________________________________________________________________________________________________
   // Initialize the USB framework
   usbfwInit();
   // Initialize the USB interrupt handler with bit mask containing all processed USBIRQ events
   usbirqInit(0xFFFF);

   USBFW_SELECT_ENDPOINT(5);
   USBMAXO = 0;    USBMAXI = 0xFF;
   USBCSIH |= USBCSIH_IN_DBL_BUF;
   
   HAL_INT_ENABLE(INUM_T1,  INT_ON);    // enable Timer1 interrupt 
   INT_GLOBAL_ENABLE(TRUE);
   
   //setup exchange_block->pointers
   exchange_block->readHexArray     = readHexArray;
   //exchange_block->ch2int       =   ch2int;
   exchange_block->rollNW           = rollNW;       
   exchange_block->rollW            = rollW;        
   exchange_block->roll2next        = roll2next;    
   exchange_block->usb              = usb;
   //exchange_block->printHex     =   printHex;
   exchange_block->mymemset         = _mymemset;    
   exchange_block->mymemcpy         = _mymemcpy;    
   exchange_block->mymemcmp         = _mymemcmp;
   exchange_block->refreshTheFlash  = refreshTheFlash;
   exchange_block->clearThePage     = clearThePage;
   exchange_block->initiateTransfer = initiateTransfer;
   exchange_block->strtoul          = strtoul;
   exchange_block->halWait          = halWait;
   exchange_block->bou_loop         = bou_loop;
   exchange_block->clear_inbuff     = clear_inbuff;
   
   exchange_block->temp             = temp_buff;
   exchange_block->dma_config       = dmaConfig;
   exchange_block->radioPktBuffer   = (BYTE *)radioPktBuffer;
   exchange_block->radioPktBufferRx = radioPktBufferRx; // to accomodate maximum possible package  PACKET_LENGTH_ED + 3];
   exchange_block->radioPktBufferTx = radioPktBufferTx;
   exchange_block->transfer         = FALSE;
   
   exchange_block->ticks[ FeedTheDog ]     = TICKS_IN_SEC / 8;
   exchange_block->ticks[ StartMainImage ] = 5 * TICKS_IN_MS;
   //___________________________________________________________________________________________________________________________
   while (TRUE) bou_loop();
}
/*
void tickWait(BYTE wait){ 
    exchange_block->ticks[ TickWait ] = ( (INT16)wait ) + 1;
    while( exchange_block->ticks[ TickWait ] > 0 ){ WDCTL = 0xA8;  WDCTL = 0x58; } 
    exchange_block->ticks[ TickWait ] = 0;
}
*/
void bou_loop( void ){
UINT16 outCounter, j;
BYTE *inBuff = exchange_block->inBuff;
  handleMyStdUsb();
  INT_GLOBAL_ENABLE( FALSE );
      USBFW_SELECT_ENDPOINT(5);
      while ((USBCSOL & USBCSOL_OUTPKT_RDY)){
           outCounter = 256*USBCNTH; outCounter += USBCNTL;
           if( outCounter == 0 ) break; 
           usbfwReadFifo(&USBF5, outCounter, temp_buff);
           USBCSOL &= ~USBCSOL_OUTPKT_RDY;
           for(j = 0; j<outCounter; inBuffIdx++, j++ ){
               inBuff[ inBuffIdx ] = temp_buff[ j ];
               if(      inBuff[ inBuffIdx ] == '\r' ){ inBuff[ ++inBuffIdx ] = '\n'; inBuff[ ++inBuffIdx ] = 0; needCommandParsing = TRUE; break; }
               else if( inBuff[ inBuffIdx ] == '\n' ){ inBuff[ ++inBuffIdx ] = '\r'; inBuff[ ++inBuffIdx ] = 0; needCommandParsing = TRUE; break; }
           }
           // is it OK to ignore the rest of allAdcs when if( j < outCounter ) ???
           temp_buff[ outCounter ] = 0;
           if( echoEnabled  && ( ( USBCSIL & 1 ) == 0 ) ) usb( temp_buff );
      }
  INT_GLOBAL_ENABLE( TRUE );
  usb_out();
  if( needCommandParsing ){ parseCommand();  needCommandParsing = FALSE; }
  if( !exchange_block->main_loop && exchange_block->ticks[ FeedTheDog ] < 0 ) { WDCTL = 0xA8;  WDCTL = 0x58;  exchange_block->ticks[ FeedTheDog ] = TICKS_IN_SEC/8; }
  if( exchange_block->ticks[ StartMainImage ] < 0 )                           { start_main_image(); exchange_block->ticks[ StartMainImage ] = 0; }
  if( exchange_block->ms > 1000 ){
     if( secondsAcceptingC ) secondsAcceptingC--;
     if( exchange_block->main_loop == NULL ) exchange_block->ms %= 1000;
  }    
}
/*==== PRIVATE FUNCTIONS =====================================================*/
/*==== INTERRUPT SERVICE ROUTINES ============================================*/
/*
if( ticks2MsTime && (--ticks2MsTime==0) ){           
    ms ++; ticks2MsTime = TICKS_IN_MS;  
    if( ms == 1000 ){ 
        ms = 0; secondGone = TRUE; 
        CHECK( sec2USBWatchDog,       time2Restart )
    }
}
*/
//______________________________________________________________________________________________________________________________
/******************************************************************************
* Set up Timer 1 - here it is 24 MHz, sys tick is 100 mks, prescaler = 8, modulo opertaion with counter = 300, generate interrupt
******************************************************************************/
void setupTimer1(void){
    SET_WORD(T1CC0H, T1CC0L, 600 - 1); // 200 mks per tick
    //        prescaler = 8      modulo mode
    T1CTL   = 0x04             | 0x02;
    //        ie
    T1CCTL0 = 0x44;
}
/*****************************************************************************/
#define FROMHEX(ch) ( (ch<='9')&&(ch>='0') ? (ch - '0') : ( (ch<='F')&&(ch>='A') ? ((ch-'A')+10) : ((ch<='F')&&(ch>='A') ? ((ch-'A')+10) : -1 ) ) ) 
#define HEX(h)      ( (h<10) ? (h + '0') : ((h-10)+'A') )
//____________________________________________________________________________
/******************************************************************************
* Receive an Report from End Device, format it into the string and send this string through the USB.
******************************************************************************/
static UINT16 free_start = 0, free_finish = OUT_BUF_SIZE; 
#define OUT_SIZE 64
void usb_out( void ){
UINT16 rest = OUT_SIZE, curl = 0;
    if( ( free_finish != OUT_BUF_SIZE ) && (0 == (USBCSIL&1)) ){
        USBFW_SELECT_ENDPOINT(5);
        if( free_finish < free_start ){
            curl = free_start - free_finish;
            if( curl > OUT_SIZE ) curl = OUT_SIZE;
            usbfwWriteFifo(&USBF5, curl, &output_buffer[ free_finish ] );
            free_finish += curl;
        }else{
            curl = OUT_BUF_SIZE - free_finish;
            if( curl > OUT_SIZE ) curl = OUT_SIZE;
            usbfwWriteFifo(&USBF5, curl, &output_buffer[ free_finish ] );
            free_finish += curl; if( free_finish >= OUT_BUF_SIZE ) free_finish = 0; 
            rest        -= curl;
            if( rest ){
                curl = free_start - free_finish;
                if( curl ){
                    if( curl > rest ) curl = rest;
                    usbfwWriteFifo(&USBF5, curl, &output_buffer[ free_finish ] );
                    free_finish += curl;
                }
            }
        }
        USBCSIL |= USBCSIL_INPKT_RDY;
        if( free_start == free_finish ){ free_start = 0; free_finish = OUT_BUF_SIZE; }
        //tickWait( 1 );
    }
}
void usb( BYTE *s ){
UINT16 idx;
BOOL wasThere = FALSE;
    //INT_ENABLE(INUM_P2INT, INT_OFF);
        idx = free_start;
        while( *s && ( 
           ( idx < free_finish ) || 
           ( ( free_finish <= free_start ) && ( free_start <= idx ) && ( idx < OUT_BUF_SIZE ) ) 
        ) ){ 
            if( !wasThere && ( *s == '\n' || *s == '\r' ) ){ output_buffer[ idx++ ] = '\r'; wasThere = TRUE;       }
            else if( wasThere )                            { output_buffer[ idx++ ] = '\n'; wasThere = FALSE; s++; }
            else                                             output_buffer[ idx++ ] = *s++;
            if( idx >= OUT_BUF_SIZE ) idx = 0; 
        }
        free_start = idx; if( free_finish == OUT_BUF_SIZE ) free_finish = 0;
    //INT_ENABLE(INUM_P2INT, INT_ON);
    usb_out();
}
//BYTE * printHex( BYTE hex, BYTE *ptr ) { *ptr++ = HEX( ((hex>>4)&0xF) );  *ptr++ = HEX( (hex&0xF) ); return ptr; }
/******************************************************************************/
BYTE* rollNW( BYTE *ptr ) { 
    while( *ptr ) if( *ptr != ' ' && *ptr != '\t' && *ptr != '\n' && *ptr != '\r' ) ptr++; else break; 
    return ptr;
}
BYTE* rollW( BYTE *ptr ) { 
    while( *ptr ) if( *ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r' ) ptr++; else break; 
    return ptr;
}
BYTE* roll2next( BYTE *ptr ) {     return rollW( rollNW( ptr ) );  }
/******************************************************************************/
/*
BYTE ch2int( BYTE ch ) { 
    if(      ch >= '0' && ch <= '9' ) ch-= '0';
    else if( ch >= 'A' && ch <= 'F' ) ch = 10 + (ch - 'A');
    else if( ch >= 'a' && ch <= 'f' ) ch = 10 + (ch - 'a');
    else                              ch = 0;
    return   ch & 0xF; 
}
*/
BOOL readHexArray(BYTE *arr, BYTE *ptr, BYTE l){
BYTE i, j;
    mymemset( arr, 0, l );
    for( i = 0; i < l; i++, arr++ )
        for( j = 0; j < 2; j++, ptr++ )
             if(      ( *ptr >= '0') && (*ptr <= '9') ) *arr |= (*ptr - '0')      << (j?0:4);
             else if( ( *ptr >= 'A') && (*ptr <= 'F') ) *arr |= (*ptr - 'A' + 10) << (j?0:4);
             else if( ( *ptr >= 'a') && (*ptr <= 'f') ) *arr |= (*ptr - 'a' + 10) << (j?0:4);
             else return FALSE;
    return TRUE;
}
/******************************************************************************
* Parse Commands from main computer and take actions
* Commands are:
*
*   Addr:  all  string  inidividual   MAC         
*           *   |sNNNN | #NNNN      | mNNNNNNNNNN
*
*   Value: logical 0/1  byte hex  short hex  32bit hex    float
*           lN         | bNN     | sNNNN    | LNNNNNNNN | fNNNNNNNN 
*
*   'A 0/1 HHHH KKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKK VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv'                     
*                           send AES vector
*                               0 - send new AES key, 1 - send new AES IV
*                               HHHH => network address in hex, first 2 letters - network id, second 2 letters - bunch
*                               KKK...KKK => 16 bytes of 'real secret' AES key in hex
*                               VVV...VVV => 16 bytes of 'real secret' AES initialization vector in hex
*                               vvv...vvv => 16 bytes of new network key in hex
*   'A 2/3 vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv'                     
*                           set AES vector to GW
*                               0 - send new AES key, 1 - send new AES IV
*                               vvv...vvv => 16 bytes of new network key in hex
*   'a 0/1'                 ADCs on /off
*   'B NNNN'                set edAddr and gwAddr for this gateway, NNNN is in hex
*   'd 0/1'                 turn on/off 'bunch 255' mode - needed for boot over the air
*   'c addr bPage'          clear the page, page from 1 to 30 inclusive
*   'b addr L Addr HexSeq'  send n bytes for 'boot over the air' - 
*                               addr - address in network, as always,
*                               L    - length, in hex
*                               Addr - address in flash, hex
*                               HexSeq - data to send
*   'e'                     start sending requests in One second, used for synchronization of several gateways
*   'F Addr'                allow to write to flash for End Devices + store defaults - overV, overC, module, mpp
*   'f Addr'                allow to write to flash for End Devices
*   'G'                     write to flash
*   'g NN'                  set shift to GW offset register
*   'h NN'                  set channel - 8 bit command, BYTE for new channel number      
*   'H 0/1'                 hopping on/off
*   'i'                     GW info
*   'J (sNNNN | mNNNN | hNNNN | dNNNN)'     JOIN enabled for nnnn seconds/minutes/hours/days
*   'k NN'                  make nDevs=NN
*   'K N FF.F'              set Gateway coefficient 4 - tk0, 5 - tk1, 0 - dFk1, 1 - dFk2, 2 - dFk3, 3 - dFk4
*   'l 0/1'                 long packets (34 bytes) only
*   'L 0/1                  long reports / short reports
*   'm Addr'                restart measurement - clear buffer on specified ED. Needed for test purposes.
*   'M mac'                 set mac for this GW
*   'N', 'n'                dissolve network - all ED forget about their id's, GW network id goes to 0
*   'O'                     get gateway coefficients
*   'o Addr'                get coeeficients
*   'p'                     advance command counter by 1000
*   'P HH'                  set power - PA_TABLE0 to 
*   'Q'                     reset the commands counter - suppose to be done every night (before active day) by middleware
*   'r Addr'                reset processor on end device
*   'R'                     reset gateway
*   's' 'S Addr Reg Value'      set register in specified end device
*   'T UTC MS'              set up time in UTC + milliseconds
*   'u'                     read from flash
*   'v 0/1'                 fcc (30 kbod, 2410 start, ~250 kHz interchannel space ) mode off/on
*   'V 0/1'                 use FEC
*   'w Addr'                switch to CW mode for selected devices
*   'W'                     switch this gateway to CW mode
*   'X'                     clear the id distribution history
*   'x'                     watchdog start
*   'Y 0|1' 'y 0/1'         0 means Rx Only, 1 means Normal
*   'Z Addr'                allow for flash rejuvenation on end devices
*   'z N'                   set default amount of repetition

* In the boot sector:
*   'I'                 hello from Boot Over USB
*   'C 8..30'           clear the page on the GW                     
*   'E 0/1'             Echo on / off
*   'R'                 Reset
*   'D L Addr HexSeq'   send n bytes for 'boot over the USB' - 
*                               L    - length, in hex
*                               Addr - address in flash, hex
*                               HexSeq - data to send
*    'S'                Start main program
*   'U nnn'                Arm GW for the 'C' command                   
******************************************************************************/
void parseCommand(void){
BYTE *ptr = exchange_block->inBuff; //, *tst;
//BYTE debug[16];
//static const __xdata_rom BYTE arr[] = "BOU\n";
    ptr = rollW( ptr );  // leading spaces
    switch( *ptr++ ){
        case 'C': if( secondsAcceptingC ) {
        BYTE page;
            exchange_block->ticks[ StartMainImage ] = 0;
            for( page = UPPER_PAGE_BEGIN; page < UPPER_PAGE_END; page ++ ) clearThePage( page << 1 );
            //if( exchange_block->main_loop != NULL ){
            exchange_block->armAdcs          = NULL;
            exchange_block->readNextValue    = NULL;
            exchange_block->main_loop        = NULL;
            clear_inbuff();
            while (TRUE) bou_loop();
            //}
        }else usb( "Command was not Armed!" );
        case 'E': ptr = rollW( ptr ); echoEnabled = (*ptr!='0');   break;
        case 'R': HAL_INT_ENABLE(INUM_T1,  INT_OFF);               break;
        case 'I': usb( "BOU\n" );                                  break;
        case 'D': if( exchange_block->main_loop == NULL ) {
            exchange_block->ticks[ StartMainImage ] = 0;
            BYTE   len;
            UINT16 addr;
            ptr = rollW( ptr );     len  = (BYTE)(strtoul(   (char *)ptr, 0, 16)&0xFF);
            //debug[0] = HEX( (len & 0xF) ); debug[1] = '\n'; debug[2] = 0; usb( debug );
            ptr = roll2next( ptr ); addr = (UINT16)(strtoul( (char *)ptr, 0, 16)&0xFFFF);
            if( ( len <= 16 ) && ( addr >= UPPER_BEGIN ) && ( (addr+len) < UPPER_END ) ){
            BYTE   buf[16];
                ptr = roll2next( ptr );     readHexArray( buf, ptr, len );  
                while( !initiateTransfer( buf, (BYTE *)addr, len ) ) halWait( 5 ); 
            }
        }break;
        case 'U': secondsAcceptingC = (BYTE)(strtoul(   (char *)ptr, 0, 16)&0xFF);   break;
        case 'S': start_main_image();                                        break;
        default:  exchange_block->flags[ ParseCommand ] = TRUE;              break; 
    }
    if( !exchange_block->main_loop || !exchange_block->flags[ ParseCommand ] ) clear_inbuff();
}
void start_main_image( void ){
    if( !exchange_block->main_loop ){
        clear_inbuff();
        void (__near_func * entry_point)(void); 
        entry_point = (void (__near_func *)(void)) ( 0x76 + UPPER_BEGIN );
        (*entry_point)();
    }else usb( "Radio Image already started!\n" );
}
//__________________________________________________________________________________________________________________________
