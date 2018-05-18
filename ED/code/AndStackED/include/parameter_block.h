
    #define PAGE_ADDR            ((BYTE *) 0x400 )
    #define PAGESIZE             242
//    #define ENERGY_WRITE_LIMIT   ((BYTE *) 0x7FF )

typedef struct {
    UINT16 barrier;                                                                                         // 2
    BYTE   netId, myBunch;                                                                                  // 2
    UINT16 groupId;                                                                                         // 2
    // MAC number is exactly a SERIAL NUMBER
    // format:
    // 3bit weekday | 6 bit week | 7 bit year | 1 byte test center | 2 byte seq number | 1 byte random
    BYTE   myMac[6];                                                                                        // 6
    // dF = -0.0005618976581444 t^3 + 0.0539971103485394 t^2 - 0.8658779608121610 t - 249.5883835010120000  
    // t  = (adcs[5]-73000)) / 260
    float dFk[4];                                                                                          // 16

    float vrefPolynom[3];                                                                                  //* 12
    // Vout Pin Iout Vin Text Tint
    // Vout Vin1 Iout Vin2 Text Iin1 In2   
    float linearK [7][2];  // was 6

    // this is always 0 and not used anymore
    //INT32  theDelta;                                                                                        // 4
    //INT32  dF_Tolerance; // 16 in the offset register                                                       // 4 

    //UINT16 syncword;
    //INT16  cur_noise;
    //BYTE vin_disable_radio, rbunch, gbunch, shortCircuitLevel; //dev250;

    char curKey[ AES_SIZE ];                                                                                // 16
    char curIV [ AES_SIZE ];                                                                                // 16

    INT16 fetFailureCount;
    INT16 fetCurrentThreshold;
    INT16 fetDeltaCurrent;

    INT16  LowCurrentLimitTo255;
    INT16  VoutLimitCount;
    UINT16 VoutLimit;
    INT16  IoutLimitCount;
    INT16  IoutLimit;
    
    // 06/27/2017 Version info added, tests changed from tests[4] to test
    // Version info is now in code, VersionLow, VersionHigh, and these 3 Bytes used
    // to pass version info back via parameterblock info
    BYTE versionSuffix[3];
    BYTE tests; // was size 20 before fet and ocs registers added                                                                                                                                
    // this is not settable and it is not used              
    // next 20 bytes is garbage, may be reused for something meaningful 
    // struct { 
    //  BYTE   testNumber; // testNumber & 0x80 != 0 - successful, otherwise - not successful, 7 bytes for test Number
    //  UINT32 testDate;   // UTC timestamp
    // }tests[4];                                                                                           // 20
    // this is not settable and it is not used
    UINT32 installDate;  // UTC timestamp                                                                      4
    
    // all the next fields are settable and readable, but not used now in middleware
    //BYTE   azimuth, positionInString;                                                                       // 2
    //UINT16 string, elevation, latitude, longitude, altitude;                                                // 10
    BYTE k_ov_volt, k_oc_cur;
//    BYTE reserved[7]; 
    UINT16 syncword;
    BYTE   vin_disable_radio, rbunch, gbunch, shortCircuitLevel, reserved;
    BYTE synch_phase, synch_freq, bandwidth250;
    BYTE channel;                                                                                           // 1
    //float VinTurnOn, VinShutOff, VinDisableRadio;
    UINT32 utcProgram, utcLow, utcHigh;
    float   tkCurrent, tkPower;                                       // 20
    BOOL    mpp, module;
    BYTE    ov,  oc;
    BYTE    radioPower, edAddr, gwAddr, repeaterChannel, repeaterPower;
    BOOL
            production          :      1,         // least significant bit
            ensureWDReset       :      1, 
            use12kbod           :      1, 
            treatLastByteAsChannel :   1, 
            hoppingAllowed      :      1,
            isRelay             :      1, 
            is500Always         :      1, 
            searchCommunication :      1;         // most significant bit
            
    BYTE    showState;
    signed char CriticalLevel500, CriticalLevel10;
    INT16  pureOffset;
    
    UINT16  defCyclesTo500, defCyclesTo10, defCyclesToStep500Channel, defCyclesToStep10Channel, 
            defCycles2SoftKick, defCycles2HardKick;
    UINT16  imageAddr;    // magic number - addr 0x4D4
    UINT16  versionLow, versionHigh;
    BYTE    starting500Channel;
    BOOL    tryOtherImageFirst : 1,    // least significant bit
            use250kbod         : 1,    
            speculative_report : 1,    
            stay_in_rx         : 1,    
            useFEC             : 1,    
            fuseOVOC           : 1,    
            fuseComm           : 1,    
            reportUTC          : 1;    // most significant bit
    UINT16  prepAddr;
    BYTE    repStep, ov_startup, t_mod_off, toff_fallback, ton_fallback, fallback_time, network_cnst_1, network_cnst_2, oc_protection, 
            //power_dissipation_limit, power_dissipation_time, wiggle_dchan, 
            oc_step; //reserved1, reserved2,
    INT16   cur_noise;
    BYTE    ov_step, shunt, max_off;
    BYTE    vin_limit, vin_turn_on, vin_switch_off;
    UINT16  post_barrier;
} ParameterBlock;
