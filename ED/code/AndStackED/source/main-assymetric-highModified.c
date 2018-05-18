/*==== DECLARATION CONTROL ===================================================*/
/*==== INCLUDES ==============================================================*/

#include "hal_main.h"
#include "main.h"
#include "commands.h"
#include "parameter_block.h"
#include "timing.h"

void tickWait(BYTE wait);
//void rc(void);
void quartz(void);
void prepareData(void);
void sendData(BYTE *buffer, BOOL is500 );
void armAdcs(void);
void readNextValue(void);
void parseGWPackage(void);
void computeADCs(BOOL timeout);//(BOOL timeout, BOOL accumulateEnergy);
void prepareJoinRequest(void);
void prepareScaling(void);
void prepareFlashCheck(void);
void setup500( BOOL flag );
void setByte( BYTE reg, BYTE val );
//void checkVin(void);
void savePB();

void parseCommands(BYTE l);

extern void flashCallback(void);
extern void setup_hopper(BYTE curch);
extern void hop(void);

extern void wait_aes(void);
extern void load(const unsigned char what, char *ptr);
extern void code(    unsigned char size,  char *from, char *to );
extern BOOL initiateTransfer(BYTE *ptr, BYTE *addr, BYTE len);

#define loadKey( key )           { load( 0x04, key );     wait_aes(); }
#define loadIV(  iv )            { load( 0x06, iv  );     wait_aes(); }
#define encode( from, to )       code( 0x00, from, to );
#define decode( from, to )       code( 0x02, from, to );

ParameterBlock page; // page is always 242 bytes long

BYTE *base_ptr = NULL;
BYTE  search_stat = 2;//, need2clean = FALSE;//, maxBunch = 0;
INT8 theRealOffset  = 0;

/*==== PUBLIC FUNCTIONS ======================================================*/
#define BUFFER_SIZE     8
#define BUFFER_MASK     0x7    

#define PAGEADDR        0x400
#define N_CHANNELS      9

//INT16  adcBuf  [ N_CHANNELS_128 ][ BUFFER_SIZE    ];
//UINT16 adcs16[   N_CHANNELS ];
UINT16 adcsTemp[ N_CHANNELS ];

//INT32 *adcs;

BYTE tempAdcsCount = 0;

// not used for now
UINT16            
       //ticks2ProcessCmd = 0,       
       cmdCount = 0,
       lastCycle   = 840*TICKS_IN_MS; 
BYTE   cyclesFromStart = 0;
       //cyclesTo500 = 120, cyclesTo10 = 220, cyclesToStep500Channel = 20, cycles2Step10Channel = 30,
       //cycles2SoftKick = 3, /*cycles2HardKick = 400,*/ cyclesFromStart = 0;
//BYTE   cycles2HardKick = 40;
BYTE   cyclesTo500 = 180, cyclesTo10 = 30, cyclesToStep500Channel = 3, cycles2Step10Channel = 4, cycles2SoftKick = 8, cycles2HardKick = 80;
//BYTE   cycles[] = {180, 30, 3, 4, 8, 80};  // might be needed later    
BYTE   curChannel = 0,  curBunch = 0,  
       bufferCount = 0, adcCount = 0,  reportScaling = 0,       joinRequestWasSend = 0,
       resetReason,     reportFlashCheck = 0,                   calibrationRepeat = 4,
       cyclesToHop = 0, setOC,          bunchCount = 0,         got_gw = 0;  


BOOL   enable2SendData  = FALSE,       txDisabled = FALSE,      packagePrepared = FALSE,
       was_listening = FALSE,          isOnTestStand = 0,  isCalibrationPackage = FALSE, main_pb = FALSE,
       /*zeroPackageWasNotReceived = FALSE,*/ turnOnFlag = FALSE;

UINT32 utc;// = 0;
UINT16 ms;//  = 0;

UINT32 utcLast;// = 0;
UINT16 msLast;//  = 0,  /*processingTime,*/ pt;
UINT16 lastVals[N_CHANNELS];
/*
UINT16 *ptr2RunningTick = NULL;
UINT16 computeTI(UINT16 _pt)
    { return ( ( _pt <= (*ptr2RunningTick) ) ? ((*ptr2RunningTick) - _pt) : ((*ptr2RunningTick) + (65535 - _pt)) ); } 
*/
//INT16  text = 0;

//SchedulerInterface *si = (SchedulerInterface *)0xF500;

//static BOOL  *flags;// = si->flags;
//static INT16 *ticks;// = si->ticks;

INT16  lastOff = 0; //, delta = 0;
INT8   lastRssi = 0;
extern INT32 delta; // LBC defined in utils.c, used in function addDF

#define ASSYM_EXT_GND 7
#define REF     8
#define TEXT    4

#ifdef MidString
  #define REFERENCE       ADC_REF_AVDD
  #define REFERENCE_PIN   ADC_AIN7
  #define TIN_CUR_PIN     ADC_AIN6
#else
  #define REFERENCE       ADC_REF_P0_7
  #define REFERENCE_PIN   ADC_AIN6
  #define TIN_CUR_PIN     ADC_TEMP_SENS
#endif
/*
__xdata_rom const BYTE  adcChannels[ N_CHANNELS ] = {  
  // Vout  0                                    Pin 1
    REFERENCE | ADC_12_BIT | ADC_AIN1,  REFERENCE | ADC_12_BIT | ADC_AIN2,
  // Iout  2                                    Vin 3
    REFERENCE | ADC_12_BIT | ADC_AIN3,  REFERENCE | ADC_12_BIT | ADC_AIN0,  
  // Text  4                                    Tin or Iin2 5
    REFERENCE | ADC_12_BIT | ADC_AIN5,  REFERENCE | ADC_12_BIT | TIN_CUR_PIN,
  // Gnd ext 6                                  Ref 7
    REFERENCE | ADC_12_BIT | ADC_AIN4,  REFERENCE | ADC_12_BIT | REFERENCE_PIN
};
*/
__xdata_rom const BYTE  adcChannels[ N_CHANNELS ] = {  
  // Vout  0                                Pin   1                                
    REFERENCE | ADC_12_BIT | ADC_AIN1,      REFERENCE | ADC_12_BIT | ADC_AIN2,    
 // Iout 2                                  Vin  3                                              
    REFERENCE | ADC_12_BIT | ADC_AIN3,      REFERENCE | ADC_12_BIT | ADC_AIN0,      
  // Text  4                                Tin 5                                  
    REFERENCE | ADC_12_BIT | ADC_AIN5,      REFERENCE | ADC_12_BIT | ADC_AIN4,
  //         Iin2 6                         Gnd ext 7
    REFERENCE | ADC_12_BIT | ADC_AIN6,      REFERENCE | ADC_12_BIT | ADC_GND,       
  //         Ref 8
    REFERENCE | ADC_12_BIT | REFERENCE_PIN,    
};

static __xdata_rom const char theKey[ 2*AES_SIZE ] = { 'A', 'N', 'D', 'S',  0,  'l', 'a', 'r', 'A', 'u', 'g', 'u', 's', 't',  0,   7 ,
                                                       'A', 'u', 'g', 'u', 's', 't',  0,   7,  'A', 'u', 'g', 'u', 's', 't',  0,   7 };
/******************************************************************************/
//#define atomicChange( idx, val ) ticks[idx]=val

BYTE nextKey[ AES_SIZE ];
BYTE nextIV [ AES_SIZE ];

INT16 abs( INT16 a ) { return ( a > 0 ) ? a : -a; }
/******************************************************************************/

const BYTE the_frequency[3] = { 0x64, 0x6A, 0xAA };
void def_freq(void){ mymemcpy( (BYTE *)0xDF09, (BYTE *)the_frequency, 3 ); }
void wdWork() { WDCTL = 0xA8;  WDCTL = 0x58; /* *((BYTE *)0xFFFF) = 0x10; */ } 

void setupRadioDR(BYTE dr, BYTE channel ){
    //INT_GLOBAL_ENABLE( INT_OFF );           // Enable interrupts globally
        while( *((BYTE *)0xF53A) == RADIO_MODE_TX) tickWait(1);            
        RFST = STROBE_IDLE; 
        *((BYTE *)0xF53A) = RADIO_MODE_UNDEF;            
        //si->radioMode = RADIO_MODE_UNDEF;            
        S1CON &= ~0x03; RFIF &= ~IRQ_DONE;  RFIM = IRQ_DONE;                     // Mask IRQ_DONE flag only
        if( (dr == DATA_RATE_1_CC2511) && page.use250kbod )   radioConfigure( DATA_RATE_2_CC2511 ); 
        else                                                  radioConfigure( dr ); 
        //radioConfigure( dr/*, _freq */); 
        CHANNR = channel;      
        if( dr == DATA_RATE_1_CC2511 ) PA_TABLE0 = page.repeaterPower;
        else                           PA_TABLE0 = page.radioPower;
        ADDR = page.edAddr;
        SYNC0 = page.syncword & 0xFF;  SYNC1 = ( page.syncword >> 8 ) & 0xFF;
        //RFST = STROBE_CAL; 
        HAL_INT_ENABLE(INUM_RF,  INT_ON);    // Enable RF general interrupt
    //INT_GLOBAL_ENABLE( INT_ON );           // Enable interrupts globally
}
extern BOOL transfer;
void setupRadio( BYTE channel ){ 
    register BYTE dr = DATA_RATE_3_CC2511;
    if( page.is500Always ) dr = DATA_RATE_1_CC2511;
    setupRadioDR( dr, channel ); 
    //setupRadioDR( page.is500Always ? DATA_RATE_1_CC2511 : DATA_RATE_3_CC2511, channel ); 
}
void softKick(void){
    //DMAARM = 0x83; RFST = STROBE_IDLE; 
    //*((BYTE *)0xF53A) = RADIO_MODE_UNDEF; 
    DMAARM = 0x83; 
    //si->radioMode = RADIO_MODE_UNDEF;            
    zerofill( (BYTE *)&delta, 4 ); //delta = 0;     
    FSCTRL0 = 0;      def_freq();
    // loadKey( page.curKey );
    //setupRadio( curChannel );        // global interrups are enabled inside setup radio
    if( !page.is500Always ) curChannel = page.channel;
    setupRadio( curChannel );
    //setupRadio( curChannel = ( page.is500Always ? curChannel : page.channel ) );        // global interrups are enabled inside setup radio
    zerofill( (BYTE *)&utcLast, 6 ); //utcLast = 0; msLast = 0;
    cmdCount = 0;
    computeADCs(TRUE);//( TRUE, FALSE );   // adjust quarz to temperature only if buffer is already full
//    ticks[ ReceiveData ] = 1;
    *((INT16 *)0xF508) = 1;
}

inline void restoreRadio(){
    //ticks[ ReceiveData ] = 1;
    //HAL_INT_ENABLE(INUM_RF,  INT_ON);    // Enable RF general interrupt
    //INT_GLOBAL_ENABLE( INT_ON );             // Enable interrupts globally
    //DMAARM = 0x83; RFST = STROBE_IDLE; 
    //ticks[ ReceiveData ] = 1;
    *((INT16 *)0xF508) = 1;
}

void setupRepeater( BOOL flag ){
  if( flag && (!page.is500Always) ){  
       setupRadioDR( DATA_RATE_1_CC2511, page.repeaterChannel );  
       ADDR = page.gwAddr;  
  }else setupRadio( curChannel );                    
}
/******************************************************************************/
void init(void){
//BYTE pin;

    P0DIR = 0;      // all port 0 pins are input
    // pins 7(oc) 6(ov) 4(on/off), 3(mpp), 1(LED), 0(mpp cycle) are for output
//    P1DIR = 0xDB;   // 1101 1011                                                  // commented to save space, done in ny-boot

    P0INP = 0xFF;   // all port 0 pins are tristate
   
    // 2 top pins are secial function - timer 3 channels
//    PERCFG  = 0x20; // Timer3 has alternate 2 location - P1_6 & P1_7              // commented to save space, done in ny-boot
    ADCCFG  = 0xFF;
    P0SEL   = 0;
//    P1SEL   = 0xC0;                                                               // commented to save space, done in ny-boot
    
//    P2SEL   = 0x20; // Timer3 has priority over USART1                            // commented to save space, done in ny-boot
    //P2DIR   = 0x0F; P2_1 = 0; P2_2 = 0;

//    T3CTL   = 0x50; // was 0x70, normal operation, mode - free running            // commented to save space, done in ny-boot
//    T3CCTL0 = 0x24; // clear output on compare up, set on 0, enabled              // commented to save space, done in ny-boot
//    T3CCTL1 = 0x24; // clear output on compare up, set on 0, enabled              // commented to save space, done in ny-boot
    
    //P1_3 = 1; //P1 |= 8;  // MPP ON
    zerofill( (BYTE *)adcsTemp, sizeof(adcsTemp) );
    //     LED down,  MPP off,  Off 
    //P1 &= ~ ( 2 | 8 | 0x10 ); 
    
    // Choose the crystal oscillator as the system clock
//    halPowerClkMgmtSetMainClkSrc(CRYSTAL);
    { BYTE counter = 127;
      SLEEP &= ~OSC_PD_BIT;     // powering down all oscillators
      while(!XOSC_STABLE && counter-- ) halWait(2);      // waiting until the oscillator is stable
      asm("NOP");
      //CLKCON &= ~MAIN_OSC_BITS; // starting the Crystal Oscillator
      CLKCON = 0x89;  
      SLEEP |= OSC_PD_BIT;      // powering down the unused oscillator
    }
   
    // Select frequency and data rate from LCD menu, then configure the radio
    
    // Configure interrupt for every time a packet is sent
    //FSCTRL0 = 0;
        
    setup_hopper( page.channel );
    dmaRadioSetup();

    //FREQ2 = 0x64;  FREQ1 = 0x6A; FREQ0 = 0xAA; // always FCC setupFrequency();
    def_freq();
    setupRadio( curChannel ); //page.is500Always? page.starting500Channel : page.channel );  // global interrupts are inabled inside the setupRadio
    //__________________________________________________________________________
   // Over Voltage     Over Current
    
    T3CC0 = page.ov;  T3CC1 = page.oc;    T3CTL |= 0x14;
    
    HAL_INT_ENABLE(INUM_T1,  INT_ON);    // enable Timer1 interrupt
    HAL_INT_ENABLE(INUM_ADC, INT_ON);    // enable ADC interrupt
    HAL_INT_ENABLE(INUM_RF,  INT_ON);    // Enable RF general interrupt
    
//    initialize_ecc();
}
/******************************************************************************/
//void ov_startup(void){ if( page.ov_startup ){ T3CC0 = page.ov_startup; atomicChange( OvStartup, TICKS_IN_SEC ); } P1_4 = 1; }
//void ov_startup(void){ if( page.ov_startup ){ T3CC0 = page.ov_startup; *((INT16 *)0xF51E) = TICKS_IN_SEC; } P1_4 = 1; }
void ov_startup(void)
{ 
  if( page.ov_startup )
  { 
    T3CC0 = page.ov_startup<page.ov ? page.ov_startup : page.ov; 
    *((INT16 *)0xF51E) = TICKS_IN_SEC; 
  } 
  P1_4 = 0; // Bug 128 - Do not allow module to turn on 
}

/******************************************************************************/
//INT16   last[ 8 ]; //Vout, lastPin, lastIout, lastGnd, lastRef, lastVin, lastTex, lastTin;
void read_page(void){
    //mymemcpy( (BYTE *)&page, PAGEADDR, PAGESIZE ); 
    mymemcpy( (BYTE *)&page, (BYTE *)0x5C00, PAGESIZE ); 
    if( ( page.barrier != 0xAAAA ) || ( page.post_barrier != 0x5555 ) ) mymemcpy( (BYTE *)&page, (BYTE *)0x400, PAGESIZE ); 
    else page.imageAddr = *((INT16 *)0x4D4);
    //if( page.barrier != UINT_BARRIER ){     // partial PB restoration goes here
    if( ( page.barrier != 0xAAAA ) || ( page.post_barrier != 0x5555 ) ){
        BYTE *ptr = &page.repeaterPower;        *(++ptr) = 0x17;  // in 8b search become off - was 0x97
        //page.barrier = 0xAAAA; page.post_barrier = 0x5555;
        page.post_barrier = 0x5555;
        ptr = &page.starting500Channel;   *(++ptr) = 0x09;
      
        page.channel = 255; page.radioPower = 255;
        //page.channel = 255; 
        // page.channel, page.radioPower
          
        page.pureOffset = -265;
        page.edAddr = 2; page.gwAddr = 1; page.syncword = 0xF0F0;
        page.vin_limit = 0;
        mymemcpy( (BYTE *)page.curKey, (BYTE *)theKey, 32 );
        mymemcpy( (BYTE *)&page,       (BYTE *)0x3D0,  12 );    // netid, bunch, mac
        page.ov         = *((BYTE *)0x3DC);
        page.oc         = *((BYTE *)0x3DD);
        page.ov_startup = 100;//*((BYTE *)0x3DE);
//        page.ov_step    = 10; //*((BYTE *)0x3DF);
//        page.oc_step    = 10;
    }
}
BOOL checkItOut( BOOL *flag_ptr ){
  if( *flag_ptr ){ *flag_ptr = FALSE; return TRUE; }
  return FALSE;
}
/*
BYTE clearAndHop( BYTE p ){
   clearThePage( p );
   if( ticks[ Hop ] && page.hoppingAllowed && !page.is500Always ){
       CHANNR = curChannel; hop(); curChannel = CHANNR; 
       atomicChange( Hop, lastCycle ); //- _slot );  
   }
   return 1;
}
*/
//UINT16 *ptr2RunningTick = NULL;
/*
UINT16 computeTI(UINT16 _pt)
{ return ( ( _pt <= (*ptr2RunningTick) ) ? ((*ptr2RunningTick) - _pt) : ((*ptr2RunningTick) + (65535 - _pt)) ); } 
*/
void tickWait(BYTE wait){ 
    /*ticks[ TickWait ] = ( (INT16)wait ) + 1;
    while( ticks[ TickWait ] > 0 ) { P1_2 ^= 1; }
    ticks[ TickWait ] = 0;*/
static INT16 *ticks = (INT16 *)0xF500; //si->ticks;
    *((INT16 *)0xF51C) = ++wait;
    while( 1 ){ if( ticks[ TickWait ] < 0 ) break; }
    *((INT16 *)0xF51C) = 0;
}
BOOL dec( UINT16 *cntPtr ) { return ((*cntPtr) && (--(*cntPtr) == 0 ))?TRUE:FALSE; }
/******************************************************************************
* @fn  main
* @brief
*      Main function. Triggers setup menus and main loops for both receiver
*      and transmitter. This function supports both CC1110 and CC2510.
* Parameters:
* @param  void
* @return void
******************************************************************************/
static BOOL wasAHardResetOrPowerOn = TRUE;
void main_loop(void){
BOOL  *flags = (BOOL *)0xF520;  //si->flags;
INT16 *ticks = (INT16 *)0xF500; //si->ticks;
BYTE idx; 
    idx = N_Of_Flags; do{ 
        if( flags[ --idx ] ){
            flags[ idx ] = FALSE;
            switch( idx ){
                case PrepareJoin:       prepareJoinRequest( );                  break;
                case PackageReceived:
                    //pt = (*ptr2RunningTick); //si->runningTick;   
                    parseGWPackage();
                break; // retransmitting slave package to gateway 
                case DataSent:
                    //rc();
                    //if( page.isRelay && (!page.is500Always) ) setupRepeater( got_gw );  
                    //else                                      setupRepeater( FALSE ); 
                    //receive(); 
                    if( page.isRelay && (!page.is500Always) ){ setupRepeater( got_gw );  receive(); }
                    else if( was_listening )                 { setupRepeater( FALSE );   receive(); }
                break;
                case Cycle:
                        if( got_gw )     got_gw--;
                        if( bunchCount ) bunchCount--;
                        //if( dec( &cycles2SoftKick )  ){ softKick();  cycles2SoftKick  = page.defCycles2SoftKick;  }
                        if( 0 == --cycles2SoftKick ){ softKick();  cycles2SoftKick  = (BYTE)page.defCycles2SoftKick;  }
//                        if( dec( &cycles2HardKick ) && page.production ) reset();  // a reset
//                        if( dec( &cycles2HardKick ) ) reset();  // a reset
//                        if( 0 == --cycles2HardKick ) reset();
                        if( ++cyclesFromStart > 50 ) wasAHardResetOrPowerOn = FALSE;
                        if( (!wasAHardResetOrPowerOn) && page.searchCommunication ){
                            if( page.is500Always ){
//                                if( dec( &cyclesTo10 ) ) { 
                                if( cyclesTo10 && ( 0 == --cyclesTo10 ) ){
                                    //cyclesTo500 = page.defCyclesTo500;  cycles2Step10Channel = page.defCyclesToStep10Channel;
                                    cyclesTo500 = (BYTE)page.defCyclesTo500;  cycles2Step10Channel = (BYTE)page.defCyclesToStep10Channel;
                                    page.is500Always = FALSE; //cycles2Step10Channel = page.defCyclesToStep10Channel;
                                    setupRadio( curChannel = page.channel );
                                //}else if( dec( &cyclesToStep500Channel ) ) { 
                                }else if( cyclesToStep500Channel && ( 0 == --cyclesToStep500Channel ) ) { 
                                    //cyclesToStep500Channel = page.defCyclesToStep500Channel; 
                                    cyclesToStep500Channel = (BYTE)page.defCyclesToStep500Channel;
                                    page.starting500Channel = curChannel = (curChannel + page.repStep) % 250;
                                    setupRadio( curChannel );
                                }
                            }else{
//                                if( dec( &cyclesTo500 ) ) { 
                                if( cyclesTo500 && ( 0 == --cyclesTo500 ) ) { 
                                    //cyclesTo10 = page.defCyclesTo10;    cycles2Step10Channel = 0;
                                    cyclesTo10 = (BYTE)page.defCyclesTo10;    cycles2Step10Channel = 0;
                                    //page.is500Always = TRUE; cyclesToStep500Channel = page.defCyclesToStep500Channel; 
                                    page.is500Always = TRUE; cyclesToStep500Channel = (BYTE)page.defCyclesToStep500Channel; 
                                    if( page.starting500Channel == 0xFF ) page.starting500Channel = page.channel;
                                    curChannel = page.starting500Channel;
                                    //ticks[ Hop ] = 0;
                                    *((INT16 *)0xF510) = 0;
                                    setupRadio( curChannel );
                                //}else if( dec( &cycles2Step10Channel ) ){
                                }else if( cycles2Step10Channel && ( 0 == --cycles2Step10Channel ) ){
                                    CHANNR = curChannel; hop(); curChannel = CHANNR; 
                                    //cycles2Step10Channel = page.defCyclesToStep10Channel;
                                    cycles2Step10Channel = (BYTE)page.defCyclesToStep10Channel;
                                }
                            }
                        }
                break;
            } // switch
        } // if( flags )
    }while(idx);// after the whole for cycle
    //for( idx = 0; idx < N_Of_Ticks; idx++ )
    idx = N_Of_Ticks; do{ 
        //INT_GLOBAL_ENABLE( INT_OFF ); 
        if( ticks[ --idx ] < 0 ){
            ticks[ idx ] = 0;
            //INT_GLOBAL_ENABLE( INT_ON ); 
            switch( idx ){
                //case Reset: reset();
                case FeedTheDog:    wdWork();      *((INT16 *)0xF504) = _dogFeeding; break;// ticks[ FeedTheDog ] = _dogFeeding; 
                case SendData:          
                    if( !bunchCount ) break;
                    if( packagePrepared ){
                        //if( reportScaling ){         reportScaling--; if( !reportScaling ) base_ptr = NULL; }
                        if( isCalibrationPackage && reportScaling ){ isCalibrationPackage--; reportScaling--; if( !reportScaling ) base_ptr = NULL; }
                        else if( reportFlashCheck )  reportFlashCheck--;
                        //was_listening = (si->radioMode == RADIO_MODE_RX);                        
                        was_listening = ((*((BYTE *)0xF53A)) == RADIO_MODE_RX);   
                        //P1_1 = 0;
                        sendData( radioPktBufferTx, FALSE ); 
                    }else *((BYTE *)0xF522) = 1; //flags[ DataSent ] = 1;
                    if( page.gbunch && ( ( page.gbunch <= 7 ) || ( page.rbunch <= 7 ) ) ){
                        BYTE b; 
                        if( page.rbunch && ( page.rbunch < page.gbunch ) ) b = page.rbunch; else b = page.gbunch;
                        if( b <= 7 ) *((INT16 *)0xF506) = lastCycle*b - 3; // atomicChange( SendData, lastCycle*b - 3 );
                    }
                    //atomicChange( CheckFlag, _slot );
                break;
                //case CheckFlag:         if( si->radioMode == RADIO_MODE_TX )  flags[ DataSent ] = TRUE; break;        
                case Hop: if( page.hoppingAllowed && !page.is500Always ){
                    //if( si->radioMode == RADIO_MODE_TX  ) { atomicChange( Hop, 1 ); break; }
                    if( (*((BYTE *)0xF53A)) == RADIO_MODE_TX  ) { *((INT16 *)0xF510) =  1; break; }
                    if( cyclesToHop-- ){ 
                        CHANNR = curChannel; hop(); curChannel = CHANNR; 
                        //atomicChange( Hop, lastCycle - 10 ); //- _slot );  
                        *((INT16 *)0xF510) = lastCycle - 10;
//                        ticks[ ReceiveData ] = 0;
                        *((INT16 *)0xF508) = 0;
                    } else   { curChannel = page.channel; CHANNR = curChannel; }
                } // go through
                case ReceiveData: setupRepeater( FALSE );     receive();                      break;
//                case Ms:   atomicChange( Ms, TICKS_IN_MS );                                   break;
                case Ms:  (*((INT16 *)0xF516)) = TICKS_IN_SEC; if( 0 == --cycles2HardKick ) reset(); break;// atomicChange( Ms, TICKS_IN_SEC ); 
                case Cycle: // kicking and search logic
                    //atomicChange( Cycle, lastCycle ); 
                    *((INT16 *)0xF512) = lastCycle;
                    //atomicChange( ReceiveData, lastCycle - _slot - 25  );    
                    *((INT16 *)0xF508) = lastCycle - _slot - 25;
//                    flags[ Cycle ] = TRUE;   
                    *((BYTE *)0xF529) = 1;
                break;
                case DelayedPrepareData: { // here most of the time is spent
                    //UINT16 ticks2Prepare = (*ptr2RunningTick); 
                    prepareData(); 
                    //ticks2Prepare = computeTI(ticks2Prepare); 
                    //if( page.gbunch && ( page.gbunch <= 7 ) ) atomicChange( DelayedPrepareData, page.gbunch * lastCycle - 44 );
                    if( page.gbunch && ( page.gbunch <= 7 ) ) *((INT16 *)0xF51A) = page.gbunch * lastCycle - 44;
                }break;  
                case OvStartup:{
                      register BOOL b = 0;
                      //BYTE cnt = (T3CC0<page.ov)?(page.ov - T3CC0):0; if(cnt>10) cnt = 10;
                      //T3CC0 += cnt;
                      if( T3CC0 < page.ov ){ T3CC0 = T3CC0+1; b++; }
                      #ifdef MidString
                          if( (((INT16)lastVals[ 2 ]) > 54 ) ){
                                if( T3CC1 < setOC ){ T3CC1 = T3CC1+1;  b++; }
                          }else                      T3CC1 = 0;
                      #endif
                      if( b ) *((INT16 *)0xF51E) = TICKS_IN_SEC / 8; // atomicChange( OvStartup, TICKS_IN_SEC / 8 );
                }break;
            } // switch
        }
    }while( idx );// for idx
}
/** timer-based waits in quants equal 50 ms */
inline void longTickWait( BYTE q ){ while( q-- ) tickWait( 50*TICKS_IN_MS ); }
//void reset_kicks(){ mymemcpy( (BYTE *)&cycles2SoftKick, (BYTE *)&page.defCycles2SoftKick, 4 ); }
//void reset_kicks(){ mymemcpy( (BYTE *)&cycles2SoftKick, (BYTE *)&page.defCycles2SoftKick, 4 ); }
//void reset_kicks(){ mymemcpy( (BYTE *)&cycles2SoftKick, (BYTE *)&page.defCycles2SoftKick, 2 );  cycles2HardKick = 40; }
void reset_kicks(){ cycles2SoftKick = (BYTE)page.defCycles2SoftKick;  cycles2HardKick = (BYTE)page.defCycles2HardKick; }
void main(void){
    *((BYTE *)0xF53B) = 0x55;
//    if( PARTNUM != 0x81 ) reset(); //{ WDCTL = 0x8 | 0x3; while( TRUE ) halWait( 10 ); } //a reset  
    //WDCTL = 0xA8; WDCTL = 0x58;
//    *((BYTE *)0xFFFF) = 0x10; 
    //si->armAdcs       = &armAdcs;
    //si->readNextValue = &readNextValue;
    *((INT16 *)0xF530) = (INT16) &armAdcs;
    *((INT16 *)0xF532) = (INT16) &readNextValue;
    //si->mppCycle      = _mppCycle; // not necessary because 8k image cannot be changed and it will stay the same as boot sector
//    *((INT16 *)0xF55B) = _mppCycle;   // it is already done in boot sector
    *((BOOL  *)0xF537) = 1;   //si->adcEnabled    = TRUE;
//    flags = (BOOL *)0xF520;  //si->flags;
//    ticks = (INT16 *)0xF500; //si->ticks;
    //ptr2RunningTick = & (si->runningTick);
    
    //unnecessary if da_boot is right
    //SET_WORD(T1CC0H, T1CC0L, 600 - 1); // 300 - 1    
    
    resetReason = (SLEEP & 0x18)>>3;
      
    read_page();
    
    reset_kicks();
    curChannel  = page.is500Always ? page.starting500Channel : page.channel; 
    search_stat = page.searchCommunication; 
    //init();

//    P1_4 = 1;  P1_3 = 1; // MPP always on
    P1_4 = 0; // Bug 128 - dont allow module to turn on  
    P1_3 = page.mpp; 
    if( resetReason  == 2 )
    { 
      setOC = page.oc; 
      T3CC0 = page.ov; 
    }
    else
    {
            #ifdef MidString
                T3CC1 = 0; 
                setOC = page.oc;  
            #else    
                T3CC1 = page.oc; 
            #endif
//            ov_startup();
            if( P1_4 ) 
                ov_startup();
    }
    init();
    //ticks[ MppCycle ]    = _mppCycle;   // 62.5 ms
//    ticks[ Ms       ]    = TICKS_IN_MS;
    *((INT16 *)0xF516) = TICKS_IN_SEC;
//    ticks[ ReceiveData ] = TICKS_IN_MS;    
    *((INT16 *)0xF508) = TICKS_IN_MS;
    //ticks[ FeedTheDog ]  = _dogFeeding; 
    *((INT16 *)0xF504) = _dogFeeding;
    //ticks[ Cycle ]       = lastCycle;
    *((INT16 *)0xF512) = lastCycle;

    loadKey( page.curKey ); 
    RNDL = page.myMac[4]; 
    RNDL = page.myMac[5]; 

    //longTickWait( 20 );
    longTickWait( 10 );
    softKick();
    
    for(;;)
    {
        if( *((BYTE *)0xF53B) != 0x55 ) 
          reset(); 
        if( PARTNUM != 0x81 ) 
           reset(); //{ WDCTL = 0x8 | 0x3; while( TRUE ) halWait( 10 ); } //a reset  
        main_loop(); 
        if( lastCycle > (840*TICKS_IN_MS) ) 
          lastCycle = 840*TICKS_IN_MS;
        //if( cycles2HardKick > 400 )         cycles2HardKick = 400;
    }
}
/*==== PRIVATE FUNCTIONS =====================================================*/
/******************************************************************************
* Work with watchdogs and LED's
******************************************************************************/
//void feedTheDog(void){ WDCTL = 0xA8;  WDCTL = 0x58; }
/******************************************************************************
* initiate reading of adc channels
******************************************************************************/
//#pragma optimize=s 9
void armAdcs(void){  
    adcCount = 0;
    //si->inCurCycle++; 
    (*((BYTE *)0xF536))++;
    //if( si->inCurCycle < 4 ) ticks[ AdcMeasurement ] = _adcMeasurement;  // 5ms
    if( *((BYTE *)0xF536) < 4 ) *((INT16 *)0xF502) = _adcMeasurement;  //ticks[ AdcMeasurement ] = _adcMeasurement;  // 5ms
    ADC_SINGLE_CONVERSION( adcChannels[ adcCount ] ); 
}
/******************************************************************************
* Read ADC's once into buffer
******************************************************************************/
//#pragma optimize=s 9
void readNextValue(void){
INT16 value;
    ADC_GET_VALUE( value ); value+=200; value >>= 4; //6
    if( adcCount == 0xFF ){ adcCount = 0; return; }
    adcsTemp[ adcCount++ ] += value;

    if( adcCount < N_CHANNELS ){
        ADC_SINGLE_CONVERSION( adcChannels[ adcCount ] ); 
    }else{
        if( ++tempAdcsCount > 7 ){  // >= 8
            mymemcpy( (BYTE *)lastVals, (BYTE *)adcsTemp, N_CHANNELS*2 );
            zerofill( (BYTE *)adcsTemp, sizeof(adcsTemp) );    tempAdcsCount = 0;
            if( ++bufferCount == BUFFER_SIZE ){ enable2SendData = TRUE; bufferCount = 0; }
        }
        adcCount = 0;
    }
}
//________________________________________________________________________________
void prepareTxBuffer(){
     loadIV( page.curIV );  
     encode( (char *)radioPktBuffer,      (char *)(radioPktBufferTx+2)    );
     encode( (char *)(radioPktBuffer+16), (char *)(radioPktBufferTx+18)    );
     radioPktBufferTx[0] = PACKET_LENGTH;               // Length byte
     radioPktBufferTx[1] = page.gwAddr;                 // GW address
     packagePrepared = TRUE;  
}
//________________________________________________________________________________
void copyMac( BYTE *ptr) { mymemcpy( ptr, page.myMac, 6); }
//#define copyMac( ptr ) mymemcpy( (BYTE *)ptr, page.myMac, 6); 
/******************************************************************************
* Prepare data - average the round robin buffer into 10 values
******************************************************************************/
//extern BYTE ErrorLocs[];
void prepareData(void){ 
  isCalibrationPackage = FALSE;
  //if(      reportScaling    ) prepareScaling();
  if(      reportScaling    ){ prepareScaling();   isCalibrationPackage = TRUE;  }
  else if( reportFlashCheck ) prepareFlashCheck();
  else if( enable2SendData  ){
      BYTE *ptr  = radioPktBuffer;
      UINT16 value = 30; 
      computeADCs( FALSE ); 
      // Insert the 6 byte timestamp into a static packet buffer
      //zerofill( radioPktBuffer, 4 );
      (*(UINT16 *)radioPktBuffer ) = lastVals[ REF ];
      *((UINT16 *)ptr) = lastVals[ REF ];
      ptr +=4;  // do not use UTC
      *ptr++ = (BYTE)CHANNR;
      *ptr++ = ( ( page.netId&0xF | ( ( P1 & 0x18 ) << 1 ) ) << 2 );    // 0 is the tag - 2 lower bytes
      copyMac( ptr );   ptr += 6;                  // mac   
      mymemcpy( ptr, (BYTE *)lastVals, 16 );
      if( page.is500Always )             value += 2 + ((UINT16)CHANNR)*100;
      else if( page.isRelay  )           value += 1 + page.repeaterChannel*100;
      if( page.searchCommunication )     value += 4;
      if( page.use250kbod )              value += 8;
      //mymemcpy( ptr+14, (BYTE *)value, 2 );
      *((INT16 *)(ptr+14)) = value;
      ptr += 16;
      *ptr++ = lastRssi ? ((BYTE) theRealOffset) : 0x7F;
      *ptr++ = (BYTE) ( lastRssi );    // 29
      *ptr++ = (BYTE)   T3CC0;          
      *ptr++ = (BYTE)   T3CC1;// timer2oc( T3CC1 ); //T3CC1;         // 30 & 31
  }else return;
  prepareTxBuffer();      
}
/******************************************************************************
* Prepare scaling coeff to send
******************************************************************************/
void prepareSpecial( BYTE tag, BYTE *p ){
BYTE *ptr = radioPktBuffer;
    mymemcpy( ptr,    p,   4 ); ptr+=4;
    *ptr++ = tag;
    *ptr++ = 3 | (page.netId << 2);
    copyMac( ptr ); 
    mymemcpy( ptr+6, p+4, 20 );
}
//________________________________________________________________________________
void prepareFlashCheck(void){
BYTE arr[ 20 ];
BYTE k = (reportFlashCheck-1) / calibrationRepeat ;
//BYTE *flashPtr = (BYTE *)( page.imageAddr & 0xFF00 ) + ( /*(reportFlashCheck-1) / calibrationRepeat*/ k ) * 0xA00; //24 * 8 * 16;
BYTE *flashPtr = (BYTE *)( 0x800 ) + k * 0xC00; //24 * 8 * 16;
BYTE bytes, bits, cnt;
    for( bytes = 0; bytes < 24; bytes++ ){
        arr[ bytes ] = 0xFF;
        for( bits = 0; bits < 8; bits++ ){
            cnt = 16; do { if( flashPtr[--cnt] != 0xFF ){ arr[ bytes ] ^= ( 1 << bits );  break; } }while( cnt );
            flashPtr += 16;
        }
    }
    prepareSpecial( 0xF8 + ( /* (reportFlashCheck-1) / calibrationRepeat */ k ), arr );
}
//________________________________________________________________________________
void prepareScaling(void){ 
BYTE k = (reportScaling-1) / calibrationRepeat;
    page.prepAddr = (UINT16)(&prepareScaling);
    prepareSpecial( 0xED + k, base_ptr + k * 24 ); 
}
/******************************************************************************
* Prepare request to join network
******************************************************************************/
void prepareJoinRequest(void){  // need to be changed drastically
BYTE *ptr  = radioPktBuffer;
//register BYTE i = 4; do{ ptr[--i] = 0xFF; }while( i );
    *ptr++ = 0xFF; *ptr++ = 0xFF; *ptr++ = 0xFF; *ptr++ = 0xFF;
    //mymemset( ptr, 0xFF, 4); 
    //mymemcpy( ptr, (BYTE *)&utc, 6 ); 
    copyMac( ptr+6 ); //memcpy( ptr+6, page.myMac, 6 );
    prepareTxBuffer();   
    radioPktBufferTx[0] = PACKET_LENGTH_GW_2;
}
/******************************************************************************
* Send the previously prepared data package
******************************************************************************/
extern DMA_DESC dmaConfig[3]; 
void sendData(BYTE *buffer, BOOL isRepeater ){  
    if( txDisabled ) return;
    RFST = STROBE_IDLE; 
    //setupTxBuffer( buffer );
    SET_WORD(dmaConfig[1].SRCADDRH,  dmaConfig[1].SRCADDRL, buffer );
    setupRepeater( isRepeater );
    //INT_GLOBAL_ENABLE(INT_OFF); 
        //si->radioMode = RADIO_MODE_TX;  
        *((BYTE *)0xF53A) = RADIO_MODE_TX;  
        // Send the packet
        DMAARM = DMAARM_CHANNEL1;      // Arm DMA channel 1
        //quartz();
        RFST   = STROBE_TX;            // Switch radio to TX
    //INT_GLOBAL_ENABLE(INT_ON); 
}
#define NDEV_MASK   0x1F
#define JOIN_MASK   0x80
#define HOP_MASK    0x40
static UINT32 utcLocal;// = 0;
static UINT16 msLocal; // = 0;  LBC not referenced directory, TB used so 6 bytes in memcpy
extern INT16  perRssiOffset;
/******************************************************************************
* @fn  parseGWPackage
* @brief       This function makes appropriate actions after receiving message from Gateway
* Parameters:
* @return void
******************************************************************************/
void parseGWPackage(void){
BYTE bunch;//, plen;//, lqi;
INT8 o = FREQEST, sh = FSCTRL0;
INT16 shift, _lastRssi = 0;
BYTE tmp = RSSI;
BYTE ndev = 0, plen;
BYTE buff[ PACKET_LENGTH + 3 ];
BOOL goodForCommands = (PKTSTATUS&0x80); //FALSE;
    //rc();
    plen = radioPktBufferRx[0]; 
    if( page.isRelay && (!page.is500Always) && ( radioPktBufferRx[1] == page.gwAddr ) && (ADDR == page.gwAddr) ){
        //sendData( radioPktBufferRx, FALSE );  return;
        if( goodForCommands ) sendData( radioPktBufferRx, FALSE );  
        return;
    }else if( radioPktBufferRx[1] == page.edAddr ){
//        goodForCommands = ( 0 != (PKTSTATUS&0x80) ); 
//        ticks2ProcessCmd = (*ptr2RunningTick); 
        mymemcpy( buff, radioPktBufferRx, radioPktBufferRx[0]+3 );
        if( ++cyclesFromStart > 50 ) wasAHardResetOrPowerOn = FALSE;
        if( !page.is500Always  ) {
            //if( page.isRelay ){ sendData( radioPktBufferRx, TRUE ); tickWait( 3+2*TICKS_IN_MS ); }
            if(       o > page.max_off  ) o = page.max_off;
            else if(  o < -page.max_off ) o = -page.max_off;
            shift = (INT16)sh + (INT16)o;
            if( abs( shift ) > 96 ) addDF( shift<<2 );  else  FSCTRL0 = (INT8)shift;
        }
        bunchCount = 7;
        
        if(tmp < 128)   _lastRssi = (tmp >> 1)               ;// - perRssiOffset;
        else            _lastRssi =(((UINT16)tmp - 256) >> 1);// - perRssiOffset;
        _lastRssi -= perRssiOffset;
        lastRssi = (INT8)_lastRssi;

        loadIV( page.curIV );  
        decode( (char *)(buff+2),  (char *)radioPktBuffer );
        //if( buff[0] > 18 ) 
        decode( (char *)(buff+18), (char *)(radioPktBuffer+16) );
        //mymemcpy( radioPktBuffer+16, buff+18, 16 );
        bunch = radioPktBuffer[0]; ndev = radioPktBuffer[1]; 
        mymemcpy( (BYTE *)&utcLocal, radioPktBuffer+2, 6 );
        if( ((ndev & NDEV_MASK ) <= 16) && //(msLocal < 1000) && 
            //((utcLocal > utcLast) || ( (utcLocal == utcLast) && (msLocal != msLast) ) ) //protects ONLY from replay attack
            ((utcLocal >= utcLast)/* || ( (mymemcmp( (BYTE *)&utcLocal, (BYTE *)&utcLast, 4)==0) && (msLocal != msLast) )*/ ) //protects ONLY from replay attack
        ){
            P1_1 ^= 1;
            if( !page.is500Always ) {
                if(       o > page.max_off  ) o =  page.max_off;
                else if(  o < -page.max_off ) o = -page.max_off;
                shift = (INT16)sh + (INT16)o;
                if( abs( shift ) > 96 ) addDF( shift<<2 );  else  FSCTRL0 = (INT8)shift;
            }
            if( goodForCommands /*&& ( page.is500Always || (0!=(LQI & 0x7F) ) )*/ ){
                got_gw = 8;
                if( (!page.is500Always) && page.isRelay ) sendData( radioPktBufferRx, TRUE );  
                //mymemcpy( (BYTE *)&cycles2SoftKick, (BYTE *)&page.defCycles2SoftKick, 4 );
                reset_kicks();
                msLast = 0; msLocal = 0; ms = 0; // to prevent deletion of this variables by optimizer
                mymemcpy( (BYTE *)&utcLast, (BYTE *)&utcLocal, 6 );
                lastCycle = _slot * ( (ndev & NDEV_MASK) + ((ndev & JOIN_MASK)?1:4) ) + _loop_delay;
                if( page.synch_freq && (((2+(ndev & NDEV_MASK))%3)==0) ){ static BYTE cnt = 1; if( 0 == --cnt ){ *((INT16 *)0xF500) = page.synch_phase; cnt = page.synch_freq; } }
                if( (bunch == 0) || ( page.gbunch && (page.gbunch - bunch) <= 7 ) ){ 
                    packagePrepared = FALSE;
                    //atomicChange( DelayedPrepareData , 6*TICKS_IN_MS + ((bunch!=0)?lastCycle*( page.gbunch - bunch ):0) );  
                    *((INT16 *)0xF51A) = 6*TICKS_IN_MS + ((bunch!=0)?lastCycle*( page.gbunch - bunch ):0);
                    mymemcpy( (BYTE *)&utc, (BYTE *)&utcLocal, 6 ); 
                } 
                //atomicChange( ReceiveData, lastCycle - _slot - 10 /*- processingTime*/ );  
                *((INT16 *)0xF508) = lastCycle - _slot - 10;
                curBunch = bunch;
                if( !page.is500Always  ) {
                    if( ndev & HOP_MASK ){ 
//                        atomicChange( Hop,  ticks[ ReceiveData ] - 2*TICKS_IN_MS ); 
                        //atomicChange( Hop,  *((INT16 *)0xF508) - 2*TICKS_IN_MS ); 
                        *((INT16 *)0xF510) = *((INT16 *)0xF508) - 2*TICKS_IN_MS;
                        cyclesToHop = 8;
                    }else { cyclesToHop = 0;  *((INT16 *)0xF510) = 0; }//  atomicChange( Hop, 0 ); }
                }
                //atomicChange(  Cycle, lastCycle + TICKS_IN_MS );
                *((INT16 *)0xF512) = lastCycle + TICKS_IN_MS;
                
                if( page.is500Always ){
                      if( lastRssi > page.CriticalLevel500 ) 
                          { cyclesToStep500Channel = page.defCyclesToStep500Channel; cyclesTo10 = page.defCyclesTo10; }
                      else *((BYTE *)0xF529) = 1; // flags[ Cycle ] = TRUE;
                }else if( lastRssi > page.CriticalLevel10 )  
                          { cyclesTo500 = page.defCyclesTo500;   cycles2Step10Channel = page.defCyclesToStep10Channel; }
                      else *((BYTE *)0xF529) = 1; // flags[ Cycle ] = TRUE;
    
                if( bunch != 0xFF ){
                BYTE b = bunch;
                INT16 sd = _slot * page.netId + _gw_delay;// - /*ticks2ProcessCmd - */processingTime;
                    if( page.rbunch ) b %= page.rbunch;
                    if( b == page.myBunch ){
                        //if( sd < ( ticks[ DelayedPrepareData ] + 60 ) ) sd = ( ticks[ DelayedPrepareData ] + 60 );
                        *((INT16 *)0xF506) =  sd; // atomicChange( SendData, sd );   // - TICKS_IN_MS; 
                    }else if( (page.myBunch == 0xFF) && ((ndev & JOIN_MASK) == 0) ) {
                        *((INT16 *)0xF51A) =  0; //ticks[ DelayedPrepareData ] = 0;
                        if( joinRequestWasSend > 0 )  joinRequestWasSend --;    
                        else {
                            ADCCON1 &= 0xF3; ADCCON1 |= 4;
                            joinRequestWasSend = page.network_cnst_1 + RNDH % page.network_cnst_2; 
                            //atomicChange( SendData, (ndev & NDEV_MASK) * _slot + (RNDL % 3) * _join_slot + _gw_delay /*- ticks2ProcessCmd */);  // 12 * 
                            *((INT16 *)0xF506) = (ndev & NDEV_MASK) * _slot + (RNDL % 3) * _join_slot + _gw_delay;
                            *((BYTE *)0xF523) = 1; //flags[ PrepareJoin ] = TRUE;
                        }
                    }else if( page.gbunch && (b < page.myBunch) && ((page.myBunch-b)<7) ){ 
                        //atomicChange( SendData, (page.myBunch-b)*lastCycle + sd);// - TICKS_IN_MS; 
                        *((INT16 *)0xF506) = (page.myBunch-b)*lastCycle + sd;
                    }
                }
                /*if( ticks[ SendData ] ){
                INT8 adjustment = 24;
                    if( bunch == 0 )                adjustment -= 41;
                    if( plen == PACKET_LENGTH_GW_2) adjustment += 57;
                    if( page.is500Always )          adjustment -= 22;
                    else if( page.isRelay )         adjustment -=  2;
                    ticks[ SendData ] +=  adjustment;
                }*/
                if( *((INT16 *)0xF506) ){// ticks[ SendData ] ){
                //signed char adjustment = 5*TICKS_IN_MS+1; // was 24;
                //    if( bunch == 0 )                adjustment -= 8*TICKS_IN_MS;  // was 41;
                signed char adjustment = -3*TICKS_IN_MS;
                    if( plen == PACKET_LENGTH_GW_2) adjustment += 11*TICKS_IN_MS; // was 57; 
                    if( page.is500Always ){
                        if( page.use250kbod )     adjustment -= (5*TICKS_IN_MS+3); 
                        else                      adjustment -= 5*TICKS_IN_MS+1; // was 22;
                    }//else if( page.isRelay )        adjustment -= TICKS_IN_MS/2; // 2; //in order to make sure that repeater works when it is right after slave
                    *((INT16 *)0xF506) += adjustment; //ticks[ SendData ] +=  adjustment;
                }
                tickWait( 10 );
                parseCommands( buff[0] );
                //if( newProtocol ) setByte( SET_CUR_CHANNEL, radioPktBuffer[7] );                
                //if( page.treatLastByteAsChannel ){ if( !page.is500Always && (cyclesTo500 < 200) ){ curChannel = CHANNR = radioPktBufferRx[ plen ]; } }
                if( !page.is500Always && (cyclesTo500 < 200) ){ curChannel = CHANNR = radioPktBufferRx[ plen ]; } 
            }
        }//else ticks[ ReceiveData ] = 2;
    }
    //setupRepeater( FALSE );  // uncomment to get G+ behavior
    /*
    if( !enable2SendData ){  flags[ PrepareData ] = 0;   flags[ SendData ] = 0; 
    }else 
    */
    //if( (!page.is500Always) && page.isRelay ) setupRepeater( got_gw );  
    //receive();   // uncomment to get G+ behavior
    if( (!page.is500Always) && page.isRelay ){ setupRepeater( got_gw ); receive(); }     
}
/******************************************************************************
* @fn  receive
* @brief       This function switch radio into receiving radioMode
* Parameters:
* @return void
******************************************************************************/
void receive(void){
    //if( si->radioMode == RADIO_MODE_TX  ) { atomicChange( ReceiveData, 1 ); return; }
    if( (*((BYTE *)0xF53A)) == RADIO_MODE_TX  ) *((INT16 *)0xF508) = 1; //atomicChange( ReceiveData, 1 ); 
    else{
        RFST = STROBE_IDLE;       
        //INT_GLOBAL_ENABLE( INT_OFF );
            //radioPktBufferRx[0] = 0;    
            radioPktBufferRx[1] = 0;
            //si->radioMode = RADIO_MODE_RX;
            *((BYTE *)0xF53A) = RADIO_MODE_RX;  
            DMAARM = DMAARM_CHANNEL0;            // Arm DMA channel 0
            //quartz();
            RFST = STROBE_RX;                     // Switch radio to RX
        //INT_GLOBAL_ENABLE( INT_ON );
    }
}
/******************************************************************************
* @fn  computeADCs
* @brief       This function makes all the regular computations
* Parameters:  
* @return void
******************************************************************************/
extern BYTE buff[ ];
//#pragma optimize=s 9
void computeADCs(BOOL timeout){//BOOL timeout, BOOL accumulate_energy){
INT8 off = FSCTRL0, i;
INT16  df;
UINT16 temp2; // temp1
    lastOff  = (INT16)off;          // 75=600/8
    {   temp2 = lastVals[ ASSYM_EXT_GND ]-75; i = 9; do{ lastVals[--i] -= temp2; }while(i);
        lastVals[ ASSYM_EXT_GND ] = temp2; 
/* ----- counts in transmission 
        #ifdef MidString
            // using 2496 here instead of 2495 ...  2496 = 26 * 16 * 6
            temp2 = lastVals[ REF ] / 26;  i = 8; do{ temp1 = lastVals[--i]*16; lastVals[i] = 6 * ( temp1 / temp2 ); }while(i);
        #else        
            // using 2436 here instead of 2435 ...  2436 = 29 * 14 * 6
            temp2 = lastVals[ REF ] / 29;  i = 8; do{ temp1 = lastVals[--i]*14; lastVals[i] = 6 * ( temp1 / temp2 ); }while(i);
        #endif
        //t = (INT16)( adcs[ TEXT ] * 2495 ) / adcs[ REF ];
        lastVals[ TEXT ] = ( ((625 - lastVals[ TEXT ])*87)/2 ); 
  ----- counts in transmission */        
        /* no adjustments - useless
        t /= 100;
        ttt = t; 
        if( t < t1 ) ttt = t1;
        if( t > t2 ) ttt = t2;
        df = off + k*( t-ttt)/10;
        */
        df = page.pureOffset; //-285;        
    }
    
    // dfk = -0.000693, 0.0556, 0.613, -285.44
    //df = rlOff;
    theRealOffset = (INT8)( (delta + (lastOff<<2) - df) >> 2 );

    // adjust frequency by temperature
    if( timeout ){
        RFST = STROBE_IDLE;
        addDF( df /*+ page.theDelta*/ - delta ); 
        setupRepeater( FALSE );     receive();
    }
}
//______________________________________________________________________________________________________________________________
//BOOL checkPage( BYTE val ){ return val && val != 0x1F && ( ( val ^ (page.imageAddr>>9) ) & 0x20 ); }
//______________________________________________________________________________________________________________________________
/******************************************************************************
* @fn  setByte
* @brief       set the byte value to register
* Parameters:  byte reg - register, byte val - value
* @return      void
******************************************************************************/
void setByte( BYTE reg, BYTE val )
{
  BYTE *ptr; BYTE *pptr = (BYTE *)&page;
    /*
    if(page.isRelay && !page.is500Always ) switch(reg){
        case SET_CUR_CHANNEL: case SET_CHANNEL:     case RADIO_POWER:
        case IS_RELAY:        case IS_500_ALWAYS:   case REPEATER_POWER:
            tickWait( 3+2*TICKS_IN_MS );  // give repeater chance to finish transmission
        break;
    }*/
    switch(reg)
    {
        case MPP:  // MPP
          break; // Bug 128 - Do nothing, Don't turn on MPP either
          /*
            if( val )
              P1_3 = 1;
            else 
              P1_3 = 0;
          */
            /* commented out - Bug 128  
            if( val ) { 
                //atomicChange( BypassMode, 0 ); // do not need that delayed mpp off anymore
                *((INT16 *)0xF50A) = 0; //atomicChange( BypassMode, 0 ); // do not need that delayed mpp off anymore
                //if( turnOnFlag || ticks[ TurnOn ] ) { ov_startup(); turnOnFlag = FALSE;  ticks[ TurnOn ] = 0; } //P1 |= 0x10; }
                if( turnOnFlag || *((INT16 *)0xF514) ) { ov_startup(); turnOnFlag = FALSE;  *((INT16 *)0xF514) = 0; } //P1 |= 0x10; }  }*/
          break;
        case MODULE: // Module on / off
          break; // Do nothing, Bug 128          
            //if( val && ((P1 & 0x18)==0) ){ P1_3 = 1; ov_startup(); /*P1 |= 0x18;*/ atomicChange( BypassMode, TICKS_IN_SEC );            } // bypass mode with delay
            // Bug 128 if( val && ((P1 & 0x18)==0) ){ P1_3 = 1; ov_startup(); /*P1 |= 0x18;*/  *((INT16 *)0xF50A) = TICKS_IN_SEC ;  } // bypass mode with delay          
            //else                         { if( val ) P1 |= 0x10; else P1 &= ~0x10; }
            //else                         { if( val ){ if( P1_4 == 0 ) ov_startup(); }else P1_4 = 0; }
            // Bug 128 else                         { if( val ){ if( P1_4 == 0 ){ T3CC1 = 0; ov_startup(); } }else P1_4 = 0; }
            // Bug 128 break;
        case SET_T3CH0:
          break; // Bug 128 Don't set
          /*if( !page.fuseOVOC ) This was already commented out */ 
          /* T3CC0 = val; 
          break; */
        case SET_T3CH1:
          break; /* Bug 128 Don't set */
          /*if( !page.fuseOVOC ) This was already commented out*/ 
          /* T3CC1 = val; 
          break; */
        /*
        case SET_CUR_CHANNEL:
            if( !page.is500Always ){ RFST = STROBE_IDLE; curChannel = CHANNR = val;  }
        break;
        */
        case SET_CHANNEL: 
          page.channel = val; 
          setup_hopper( page.channel ); 
          if( !page.is500Always ) 
            curChannel = CHANNR = val;
          break;
          //case RANDOMIZE:    RNDL = page.myMac[4]; RNDL = page.myMac[5];         break;
          //Nobody should be able to shut it down when in high assymetric image
          //case FUSE_OVOC:      page.fuseOVOC = 0;
          // go through
        case EIGHT_K_WRITEMFPB:
          main_pb = 1; // go through
        case FLASH_AVAILABLE: 
          page.mpp    = P1_3;
          page.module = P1_4;
          //if( !page.fuseOVOC ){ page.ov = T3CC0; page.oc = setOC; }
          page.ov = T3CC0; 
          page.oc = setOC; 
            // go through
        case COEFFICIENTS_AVAILABLE:
          savePB();
          break;
        case EIGHT_K_WRITEMPB:
          main_pb = 1; 
          savePB(); 
          break;
        case DISSOLVE_NETWORK:      
          RNDL = page.myMac[4]; RNDL = page.myMac[5]; 
          page.netId = page.myBunch =  0xFF;  //maxBunch = 0;  
          *((BYTE *)0xF524) = 0; *((INT16 *)0xF506) = 0; //ticks[ SendData ] = 0; flags[ PrepareData ] = FALSE; 
          savePB();
        break;
        /*
        case CALIBRATION_FROM_FLASH: 
          if( (!reportScaling) && (base_ptr==NULL) ){
                base_ptr = (BYTE *)0x400;
                reportScaling    = (PAGESIZE/24) * calibrationRepeat;      
          }
        // go through
        case REPORT_SCALING:   
          if( !reportScaling ){
                if( base_ptr==NULL ){
                    base_ptr = (BYTE *)&page;
                    reportScaling    = (PAGESIZE/24) * calibrationRepeat;      
                }else reportScaling    = val * calibrationRepeat;
          }
        break;
        */
        case CALIBRATION_FROM_FLASH:
          pptr = (BYTE *)0x400;  /* go through */
        case EIGHT_K_READMFPB: 
          if( reg != CALIBRATION_FROM_FLASH ) 
            pptr = (BYTE *)0x5C00; /* go through */
        case REPORT_SCALING:   
          if( !reportScaling )
          {
            if( base_ptr == NULL )
            {
              base_ptr = pptr; //(BYTE *)&page;
              reportScaling    = (PAGESIZE/24) * calibrationRepeat;
            }
            else 
              reportScaling    = val * calibrationRepeat;
          }
          break;
        case CHECK_FLASH:
          reportFlashCheck = calibrationRepeat*8 + 1; /* 5*4-1 */;
          break;
        case RADIO_POWER:
          page.radioPower = val;
          PA_TABLE0 = val;  
          break;
        case PRODUCTION:
          page.production = val;
          break;
        //case ENSURE_WD_RESET:  page.ensureWDReset = val;                    break;

        //case ENABLE_HOPPING:   page.hoppingAllowed = val;  break;     
        case REPEATER_CHANNEL: 
          page.repeaterChannel = val; 
          break;     
        case IS_RELAY:
          page.isRelay = val;   
          if( !val ) 
            setupRadio( curChannel );
          break;
        case USE_12_kBod:
          page.use12kbod = val;
          break;
        case USE250:           
          page.use250kbod = val; //break;
        case IS_500_ALWAYS:    
            if( reg == IS_500_ALWAYS ) 
              page.is500Always = val;     
            if( page.is500Always )
            { 
              cyclesTo10 = page.defCyclesTo10; 
              cyclesTo500 = 0;
              cyclesToStep500Channel = page.defCyclesToStep500Channel; 
              setupRadio( curChannel = page.starting500Channel );
              //atomicChange( Hop, 0 ); 
              *((INT16 *)0xF510) = 0;
            }
            else
            {
              cyclesTo10 = 0;
              cyclesTo500 = page.defCyclesTo500; 
              cycles2Step10Channel = page.defCyclesToStep10Channel;
              curChannel = page.channel;
              //if( reg == IS_500_ALWAYS ) setupRadio( curChannel = page.channel ); 
            }
            break;     
        //case REPEATER_POWER:           page.repeaterPower = val;   if( page.is500Always ) setupRadio( page.channel ); break;
        case SEARCH_FOR_COMMUNICATION:
          page.searchCommunication = search_stat = val;
          break;
        case SetCriticalLevel500: // win 7 bytes 
        case SetCriticalLevel10:
          (&page.showState)[ reg-SET_SHOW_STATE ]  = (signed char)val;            
          break;
        case SET_START500_CHANNEL:
            page.starting500Channel = val; 
            if( page.is500Always ) 
              setupRadio( curChannel = val );
            break;
        case CLEAR_THE_PAGE:
          //val <<= 1;  
          // this is always high image
          //if( val && (val < 0x3E) && ( ( val ^ (page.imageAddr>>9) ) & 0x20 ) ){
          //if( ( val > 2 ) && ( val < 24 ) ){
             //tickWait( 5 * TICKS_IN_MS );
             //clearAndHop( val << 1 );
             //tickWait( 2 * TICKS_IN_MS );
             asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
             asm( "NOP" );
             asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
             asm( "NOP" );
//             asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
//             asm( "NOP" );
//             asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
//             asm( "NOP" );
//             asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
//             asm( "NOP" );
//             asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
//             asm( "NOP" );
//             if( !isOnTestStand ){ isOnTestStand = 20; cmdCount--;   CLKCON = 0x89;  asm( "NOP" ); asm( "NOP" );  asm( "NOP" ); break; }
             clearThePage( val << 1 );
             tickWait( 25 * TICKS_IN_MS );
//             CLKCON = 0x89;  asm( "NOP" );
//             asm( "NOP" );   asm( "NOP" );             
             //softKick();
             //restoreRadio();
             //ticks[ ReceiveData ] = 25*TICKS_IN_MS;
             quartz(); softKick();
          //}
        break;
        //case TRY_OTHER_IMAGE:            page.tryOtherImageFirst = val;        break;
        case CALIBRATION_REPEAT:
          if( val&0x1F  ) 
            calibrationRepeat = val; 
          else 
            calibrationRepeat = 4;
          break;
        case REP_STEP:
          if( val < 5 && val >50 )
          {
            break;
          }
          // else go through
        case NETWORK_CNST_1: 
        case NETWORK_CNST_2: 
        case MAX_OFF:
        case OC_STEP: 
          ptr = (BYTE *)&page.repStep; 
          ptr[ reg-REP_STEP ] = val; 
          break;
        case VIN_DISABLE_RADIO: 
        case SET_RBUNCH: 
        case SET_GBUNCH: 
        case SHORT_CIRCUIT_LEVEL: //DEV250:
          ptr = (BYTE *)&page.vin_disable_radio; 
          ptr[ reg-VIN_DISABLE_RADIO ] = val; 
          break;
        /* saving the space
        case LOW_UTC: case HIGH_UTC: 
            mymemcpy( ((BYTE *)(&page.utcLow))+4*(reg-LOW_UTC), (BYTE *)&utcLast, 4);
        break;
        */
        //case TREAT_LB_AS_CHAN : page.treatLastByteAsChannel = val; break;
        case K_OV_VOLT:
        case K_OC_CUR:
            ptr = (BYTE *)&page.k_ov_volt; 
            ptr[ reg - K_OV_VOLT ] = val;
            break;       
    }
}
void savePB(){  // delays can be 20 ms and 2400 mks, delays below are overkill
  //clearAndHop( 2 ); 
  //page.barrier = 0xAAAA; page.post_barrier = 0x5555;
//  asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
//  asm( "NOP" );
  asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
  asm( "NOP" );
  asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
  asm( "NOP" );
  asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
  asm( "NOP" );
  asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
  asm( "NOP" );
  asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
  asm( "NOP" );
  if( !isOnTestStand ){ isOnTestStand = 20; cmdCount--;   CLKCON = 0x89;  asm( "NOP" ); asm( "NOP" );  asm( "NOP" ); return; }
  //clearThePage( 2 );
  if( main_pb ) clearThePage( 0x2E ); else clearThePage( 2 );
  tickWait( 25*TICKS_IN_MS );
  //initiateTransfer( (BYTE *)&page, (BYTE *)PAGEADDR, PAGESIZE );
  initiateTransfer( (BYTE *)&page, (BYTE *)(main_pb ? 0x5C00 : PAGEADDR), PAGESIZE );
  main_pb = FALSE; 
  tickWait( 11*TICKS_IN_MS );  // was 5 ms
  CLKCON = 0x89;  asm( "NOP" );
  asm( "NOP" );   asm( "NOP" );             
  //softKick();
  restoreRadio();
}
/******************************************************************************
* @fn  setInt
* @brief       set the int16 value to register
* Parameters:  byte reg - register, int val - value
* @return      void
******************************************************************************/
void setInt( BYTE reg, UINT16 val ){
UINT16 *ptr = &page.defCyclesTo500;
BYTE *pptr = (BYTE *)&page;
    /*
    if(page.isRelay && !page.is500Always ) switch(reg){
        case SET_ED_GW:       case BootImage:     case SyncWord:
            tickWait( 3+2*TICKS_IN_MS );  // give repeater chance to finish transmission
        break;
    }
    */
    switch(reg){     
        case SET_GROUP_ID: page.groupId = val; break;
        case JOIN: page.netId = (val & 0xF); page.myBunch = ( val >> 8 ) & 0xFF;  break;
        case SET_ED_GW:
            // this check for not 0 is a mistake, because it prohibits setting addr to 0, was a big issue on Remington Hi-Volts !!!
            //if( val & 0xFF   ) page.edAddr = val & 0xFF;
            //if( val & 0xFF00 ) page.gwAddr = ( val >> 8 ) & 0xFF;
            
            ADDR = page.edAddr = val; // & 0xFF;
            page.gwAddr = ( val >> 8 ); // & 0xFF;
        break;
        case BootImage:
            //tickWait( 12 * TICKS_IN_MS);
            {void (*f)( void ) = ( void (*)( void ) )val;  /*si->interImageCommunications[ JustLoaded ] = TRUE;*/ (*f)();}          
        break;
        case SyncWord: 
          SYNC0 = val;    SYNC1 = ( val >> 8 ); // & 0xFF;
          page.syncword = val; 
        break;
        
        case SetCyclesTo500:                    case SetCyclesTo10:             
        case SetCyclesToStep500Channel:         case SetCyclesToStep10Channel:  
        case SetCyclesToSoftKick:               case SetCyclesToHardKick:   
        case SetImageAddr:
        case SetVersionLow:                     case SetVersionHigh:
        //default:
            //if( (reg > (SetCyclesTo500-1) ) && ( reg < (SetVersionHigh+1) ) ){
                //UINT16 *ptr = &page.defCyclesTo500;
                ptr[ reg - SetCyclesTo500 ] = val;
            //}
        break;
        case SetBasePtr:
            //if( (base_ptr == NULL) && (((UINT16)val) > 0x3FF) && (((UINT16)val) < 0x800) )  base_ptr = (BYTE *)val; // to save space
          base_ptr = (BYTE *)val; 
        break;
        case MicroOffset: page.pureOffset = (INT16)val; break;
        case ReadPBOneCommand: 
            switch( (val>>12)&0xF ){
                case 1: pptr = (BYTE *)0x400;  break;
                case 2: pptr = (BYTE *)0x5C00; break;
            }
            base_ptr = pptr + 24*((val>>8)&0xF);
            calibrationRepeat = (val>>4)&0xF;
            reportScaling  = (val&0xF) * calibrationRepeat; 
        break;
        case SetPBByte: pptr[ ( val >> 8 )&0xFF ] = val & 0xFF; break;
    }
}

/******************************************************************************
* @fn  parseCommands
* @brief       This function parses commands and sets global flags
* Parameters:
* @return void
******************************************************************************/
void parseCommands(BYTE l){
BYTE *ptr     = radioPktBuffer + 8;
BYTE cmd, reg;
BOOL flag = TRUE, addrFlag = FALSE; //, boa2 = FALSE;//, individualAddr = FALSE;;
UINT16 cmdCnt = 0; //, i;
    //while( (*ptr != NULLOP) && (ptr < barrier) ){
    while( *ptr && (l--) ){
        cmd = *ptr++; 
        if( (cmd & 0xF0) == SHORT_NETJOIN ){
            reg = *ptr++;
            if( mymemcmp( page.myMac, ptr, 6 ) == 0 ){ page.netId = (cmd & 0xF); page.myBunch = reg; }
            ptr += 6;
        }else{
            if( search_stat == 2 ) search_stat = page.searchCommunication;
            cmdCnt = *((UINT16 *)ptr);
            ptr += 2;
            switch( cmd & 0x3 ){ 
                case BROADCAST:                                                             flag = TRUE; break;
                case STRINGADDR: flag = ( *((UINT16 *) ptr) == page.groupId );              ptr += 2; /*individualAddr = flag;*/ break;
                case NIADDR:     flag = ( ptr[0] == page.myBunch && ptr[1] == page.netId ); ptr += 2;    break;
                case MACADDR:    
                    flag = ( mymemcmp( page.myMac, ptr, 6 ) == 0 );// flag = TRUE; else flag = FALSE;
                    //individualAddr = flag;
                    ptr += 6;
                break;
            }
            addrFlag = flag;
            if( cmdCnt ){
                if( cmdCount > 0xFF00 && cmdCnt < 0x100 ) cmdCount = cmdCnt;
                else                                      flag = flag && (cmdCnt > cmdCount);
            }
            if( cmdCnt > cmdCount ) cmdCount = cmdCnt;            
            switch( cmd & 0xF0 ){
                case SETREG:
                    if( search_stat != page.searchCommunication ) page.searchCommunication = search_stat;
                    reg = *ptr++;
                    switch( cmd&0xC ){
                        case BYTE_VAL:   if( flag ) setByte ( reg, *ptr );               ptr++;   break;
                        case SHORT_VAL:  if( flag ) setInt  ( reg, *((UINT16*)ptr) );    ptr+=2;  break;
                    }
                break;
                case RESET: if( flag ) reset(); else break; // ticks[ Reset ] = TICKS_IN_SEC; else break;
                
                //case BOA_CHANNEL:   boa2 = TRUE;
                case BOOT_OVER_AIR: if( addrFlag ) {
                UINT16 addr;  
//                        asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
//                        asm( "NOP" );
//                        asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
                        asm( "NOP" );
                        asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
                        asm( "NOP" );
                        asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
                        asm( "NOP" );
                        asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
                        asm( "NOP" );
                        asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
                        asm( "NOP" );
                        asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
                        asm( "NOP" );
                        if( !isOnTestStand ){ isOnTestStand = 20; cmdCount--; CLKCON = 0x89; asm( "NOP" );  return; }  
                    page.searchCommunication = FALSE;
                    reg = *ptr++; addr = *((UINT16*)ptr);  
                    //if( boa2 ){ setByte( SET_CUR_CHANNEL, reg ); reg = 16; }
                    //if( ( addr > 0x3FF ) && ( addr < 0x6000 ) ){ // to save space - all other addresses are protected anyway
                        ptr += 2;
                        //if( reg & 1 ) reg++;
                        //tickWait( 12 * TICKS_IN_MS );
                        initiateTransfer( ptr, (BYTE *)addr, reg );
                        tickWait( 4 );  // delay should be 160 mks
                        //softKick();
                    //}
                    CLKCON = 0x89; asm( "NOP" );
                }return;
                /*
                case SET_NEXT_KEY: case SET_NEXT_IV: 
                    if( individualAddr ){ 
                        loadKey( (char *)0x3E0 ); loadIV( (char *)0x3F0 ); 
                        decode( (char *)ptr, ((cmd&0xF0)==SET_NEXT_IV) ? (char *)nextIV : (char *)nextKey ); 
                        loadKey( page.curKey );
                    }
                return;
                case ASVOL_NETCMD:
                    if( individualAddr ){                   
                        cmd = *ptr++;
                        page.channel = page.starting500Channel = *ptr++;                // 7 bytes so far, 1 bit free
                        page.edAddr  =                           *ptr++;
                        page.gwAddr  =                           *ptr++;
                        page.myBunch =                           *ptr++;
                        page.groupId =                           *((UINT16 *) ptr);  

                        page.netId   =                           0xF & cmd;
                        //page.use12kbod  = (0!=(0x10 & cmd ) )?1:0;
                        //page.useFEC     = (0!=(0x20 & cmd ) )?1:0;
                        //page.use250kbod = (0!=(0x40 & cmd ) )?1:0;

                        savePB();
                        setup_hopper( page.channel );
                        setupRadio(   page.channel ); //page.is500Always ? page.starting500Channel : page.channel );  // global interrupts are inabled inside the setupRadio
                    }
                break;
                */
            }
//            if( cmdCnt > cmdCount ) cmdCount = cmdCnt;
        }
    }
}

void clearThePage(BYTE page){
  //asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
  //asm( "NOP" );
  asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
  asm( "NOP" );
  asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
  asm( "NOP" );
  asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
  asm( "NOP" );
  asm( "ORL 0xC6, #0x40");              // starting the RC Oscillator
  asm( "NOP" );
  if( *((BYTE *)0xF53B) != 0x55 ) reset(); 
  if( !isOnTestStand ){ isOnTestStand = 20; cmdCount--; return; }
  //rc();
  asm("CLR  0xA8.7");
  if( !isOnTestStand ) reset();  
  FADDRH = page; FWT = 0x21; FCTL = 1; asm("NOP");
  FADDRH = 0;
  asm("SETB 0xA8.7");
  //FADDRH = 0;
  isOnTestStand = 0;  
}
void quartz(void){
BYTE counter = 127;
      SLEEP &= ~OSC_PD_BIT;     // powering down all oscillators
      while(!XOSC_STABLE && counter-- ) halWait(2);      // waiting until the oscillator is stable
      asm("NOP");
      //CLKCON &= ~MAIN_OSC_BITS; // starting the Crystal Oscillator
      CLKCON = 0x89;  
      SLEEP |= OSC_PD_BIT;      // powering down the unused oscillator
}
/*inline void rc(){
    CLKCON |= OSC_BIT;                    // starting the RC Oscillator
    asm( "NOP" );
    SLEEP |= OSC_PD_BIT;                  // powering down the unused oscillator
}*/
/*==== END OF FILE ==========================================================*/
