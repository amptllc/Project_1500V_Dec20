// reads device hex file, extracts scaling data
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef signed   short  INT16;
typedef unsigned short  UINT16;
typedef unsigned int    UINT32;
typedef unsigned char   BYTE;
typedef unsigned char   BOOL;
typedef int              INT32;

#define AES_SIZE                    16
#define TRUE                        1
#define FALSE                       0
struct {
    UINT16 barrier __attribute__((__packed__));                                                                                         // 2
    BYTE   netId __attribute__((__packed__)), myBunch __attribute__((__packed__));                                                                                  // 2
    UINT16 groupId __attribute__((__packed__));                                                                                         // 2
    // MAC number is exactly a SERIAL NUMBER
    // format:
    // 3bit weekday | 6 bit week | 7 bit year | 1 byte test center | 2 byte seq number | 1 byte random
    BYTE   myMac[6] __attribute__((__packed__));                                                                                        // 6
    // dF = -0.0005618976581444 t^3 + 0.0539971103485394 t^2 - 0.8658779608121610 t - 249.5883835010120000
    // t  = (adcs[5]-73000)) / 260
    float dFk[4] __attribute__((__packed__));                                                                                          // 16

    float vrefPolynom[3] __attribute__((__packed__));                                                                                  //* 12
    // Vout Pin Iout Vin Text Tint
    float linearK [6][2] __attribute__((__packed__));                                                                                  //* 48

    INT32  theDelta __attribute__((__packed__));                                                                                        // 4
    INT32  dF_Tolerance __attribute__((__packed__)); // 16 in the offset register                                                       // 4

    char curKey[ AES_SIZE ] __attribute__((__packed__));                                                                                // 16
    char curIV [ AES_SIZE ] __attribute__((__packed__));                                                                                // 16

    struct {
      BYTE   testNumber __attribute__((__packed__)); // testNumber & 0x80 != 0 - successful, otherwise - not successful, 7 bytes for test Number
      UINT32 testDate __attribute__((__packed__));   // UTC timestamp
    }tests[4] __attribute__((__packed__));                                                                                              // 20

    UINT32 installDate __attribute__((__packed__));  // UTC timestamp                                                                      4
    BYTE   azimuth __attribute__((__packed__)), positionInString __attribute__((__packed__));                                                                       // 2
    UINT16 string __attribute__((__packed__)),
           elevation __attribute__((__packed__)),
           latitude __attribute__((__packed__)),
           longitude __attribute__((__packed__)),
           altitude __attribute__((__packed__));                                                // 10
    BYTE channel __attribute__((__packed__));                                                                                           // 1
    float VinTurnOn __attribute__((__packed__)),
          VinShutOff __attribute__((__packed__)),
          VinDisableRadio __attribute__((__packed__)),
          tkCurrent __attribute__((__packed__)),
          tkPower __attribute__((__packed__));                                      // 20
    BYTE   ov __attribute__((__packed__)),  oc __attribute__((__packed__));                                                                                         // 2
    BOOL   mpp __attribute__((__packed__)),
           module __attribute__((__packed__)),
           radioDebug __attribute__((__packed__));    // 3
    BYTE   radioPower __attribute__((__packed__));
    BOOL   production __attribute__((__packed__)),
           ensureWDReset __attribute__((__packed__));
    BYTE   partNum2511_andReset __attribute__((__packed__)),
           partNum2511_andPOR __attribute__((__packed__)),
           partNum2511_andTR __attribute__((__packed__)),
           hardResets __attribute__((__packed__));
    BOOL   use10kbod __attribute__((__packed__));
} pageC3;
struct {
    UINT16 barrier  __attribute__((__packed__));                                                                                         // 2
    BYTE   netId __attribute__((__packed__)), myBunch __attribute__((__packed__));                                                                                  // 2
    UINT16 groupId __attribute__((__packed__));                                                                                         // 2
    // MAC number is exactly a SERIAL NUMBER
    // format:
    // 3bit weekday | 6 bit week | 7 bit year | 1 byte test center | 2 byte seq number | 1 byte random
    BYTE   myMac[6] __attribute__((__packed__));                                                                                        // 6
    // dF = -0.0005618976581444 t^3 + 0.0539971103485394 t^2 - 0.8658779608121610 t - 249.5883835010120000
    // t  = (adcs[5]-73000)) / 260
    float dFk[4] __attribute__((__packed__));                                                                                          // 16

    float vrefPolynom[3] __attribute__((__packed__));                                                                                  //* 12
    // Vout Pin Iout Vin Text Tint
    float linearK [6][2] __attribute__((__packed__));                                                                                  //* 48

    INT32  theDelta __attribute__((__packed__));                                                                                        // 4
    INT32  dF_Tolerance __attribute__((__packed__)); // 16 in the offset register                                                       // 4

    char curKey[ AES_SIZE ] __attribute__((__packed__));                                                                                // 16
    char curIV [ AES_SIZE ] __attribute__((__packed__));                                                                                // 16

    struct {
      BYTE   testNumber __attribute__((__packed__)); // testNumber & 0x80 != 0 - successful, otherwise - not successful, 7 bytes for test Number
      UINT32 testDate __attribute__((__packed__));   // UTC timestamp
    }tests[4] __attribute__((__packed__));                                                                                              // 20

    UINT32 installDate __attribute__((__packed__));  // UTC timestamp                                                                      4
    BYTE   azimuth __attribute__((__packed__)), positionInString __attribute__((__packed__));                                                                       // 2
    UINT16 string __attribute__((__packed__));
    UINT16 elevation __attribute__((__packed__));
    UINT16 latitude __attribute__((__packed__));
    UINT16 longitude __attribute__((__packed__));
    UINT16 altitude __attribute__((__packed__));                    // 10
    BYTE channel __attribute__((__packed__));                                                                                           // 1
    float VinTurnOn __attribute__((__packed__)), VinShutOff __attribute__((__packed__)),
           VinDisableRadio __attribute__((__packed__)), tkCurrent __attribute__((__packed__)), tkPower __attribute__((__packed__));                                       // 20
    BYTE   ov __attribute__((__packed__)),  oc __attribute__((__packed__)), radioPower __attribute__((__packed__)),
           edAddr __attribute__((__packed__)), gwAddr __attribute__((__packed__)), repeaterChannel __attribute__((__packed__)),
           repeaterPower __attribute__((__packed__));
    BOOL   mpp __attribute__((__packed__)), module __attribute__((__packed__)), radioDebug __attribute__((__packed__)),
           production __attribute__((__packed__)),
           ensureWDReset __attribute__((__packed__)), use10kbod __attribute__((__packed__)), useFEC __attribute__((__packed__)),
           hoppingAllowed __attribute__((__packed__)),
           isRelay __attribute__((__packed__)), is500Always __attribute__((__packed__)),
           searchCommunication __attribute__((__packed__)), showState __attribute__((__packed__));
    UINT16 defCyclesTo500 __attribute__((__packed__)), defCyclesTo10 __attribute__((__packed__)),
           defCyclesToStep500Channel __attribute__((__packed__)), defCyclesToStep10Channel __attribute__((__packed__)),
           defCycles2LightKick __attribute__((__packed__)), defCycles2SoftKick __attribute__((__packed__)),
           defCycles2HardKick __attribute__((__packed__)),
           version __attribute__((__packed__));
    signed char   CriticalLevel500 __attribute__((__packed__)), CriticalLevel12 __attribute__((__packed__));
} pageD8 = {                                                                                                  //
    0xAAAA,
    (BYTE)0xFF, (BYTE)0xFF, 0xFFFF, //  permanent !!! Normal
    { 0, 0, 0, 0, 0, 0 },
    {
      //-0.000784329277, 0.069999928482, -1.557036645365, -229.914041475601 + 32.0
      -0.000693, 0.0556, 0.613, -285.44 //+ 300.0
    }, // temperature compensation
    {  2435.0, 0.0000004, 73.0 }, // vref polynomials
    //  Vout  0              Pin  1               Iout   2         Vin  3         Text 4       Tint  5
    { {24.096 * 0.5, 0.0}, {28.426, 0.0}, {5.536, -502.0}, {22.063 * 0.5, 0.0}, {0.435, 625.0}, {0.39, 755.0 } },
    0L,
    64L,
    { 'A', 'N', 'D', 'S',  0,  'l', 'a', 'r', 'A', 'u', 'g', 'u', 's', 't',  0,   7 },
    { 'A', 'u', 'g', 'u', 's', 't',  0,   7,  'A', 'u', 'g', 'u', 's', 't',  0,   7 },
    {{(BYTE)0x01, 0}, {(BYTE)0x02, 0}, {(BYTE)0x03, 0}, {(BYTE)0x04, 0}},
    0, (BYTE)0, (BYTE)0,
    (unsigned short)0, (unsigned short)0, (unsigned short)0, (unsigned short)0, (unsigned short)0,
    (BYTE)1, //  should be changed to 0 in the production version
    24000.0*0.5, 20000.0*0.5, 0.0, //12000.0*0.5,  // 0 for debug
    0.00002, 0.00005,
    (BYTE)220, (BYTE)200, 0xFF, 2, 1, 0,
    0xFF, // 500kbod Power, 0xCB => -10 db, we may think about +1 - FF, 0 0xFE, -5 0xDB, -15 0x8E, -20 0x4D, -25  0x51
    TRUE, TRUE, FALSE, TRUE,
    TRUE, FALSE, FALSE, TRUE,
//    TRUE, FALSE, FALSE, 1,   // Repeater
//    FALSE, TRUE, TRUE, 1,   // Slave
   FALSE, FALSE, TRUE,   1,  // Search
//   FALSE, FALSE, FALSE, 1,    // Simple
    120, 220, 20, 30,
    405, 7, 400,
    0xE0,
    87, 92
};
struct { //{{{
    UINT16 barrier  __attribute__((__packed__));                                                                                         // 2
    BYTE   netId __attribute__((__packed__)), myBunch __attribute__((__packed__));                                                                                  // 2
    UINT16 groupId __attribute__((__packed__));                                                                                         // 2
    // MAC number is exactly a SERIAL NUMBER
    // format:
    // 3bit weekday | 6 bit week | 7 bit year | 1 byte test center | 2 byte seq number | 1 byte random
    BYTE   myMac[6] __attribute__((__packed__));                                                                                        // 6
    // dF = -0.0005618976581444 t^3 + 0.0539971103485394 t^2 - 0.8658779608121610 t - 249.5883835010120000
    // t  = (adcs[5]-73000)) / 260
    float dFk[4] __attribute__((__packed__));                                                                                          // 16

    float vrefPolynom[3] __attribute__((__packed__));                                                                                  //* 12
    // Vout Pin Iout Vin Text Tint
    float linearK [6][2] __attribute__((__packed__));                                                                                  //* 48

    INT32  theDelta __attribute__((__packed__));                                                                                        // 4
    INT32  dF_Tolerance __attribute__((__packed__)); // 16 in the offset register                                                       // 4

    char curKey[ AES_SIZE ] __attribute__((__packed__));                                                                                // 16
    char curIV [ AES_SIZE ] __attribute__((__packed__));                                                                                // 16

    struct {
      BYTE   testNumber __attribute__((__packed__)); // testNumber & 0x80 != 0 - successful, otherwise - not successful, 7 bytes for test Number
      UINT32 testDate __attribute__((__packed__));   // UTC timestamp
    }tests[4] __attribute__((__packed__));                                                                                              // 20

    UINT32 installDate __attribute__((__packed__));  // UTC timestamp                                                                      4
    BYTE   azimuth __attribute__((__packed__)), positionInString __attribute__((__packed__));                                                                       // 2
    UINT16 string __attribute__((__packed__));
    UINT16 elevation __attribute__((__packed__));
    UINT16 latitude __attribute__((__packed__));
    UINT16 longitude __attribute__((__packed__));
    UINT16 altitude __attribute__((__packed__));                    // 10
    BYTE channel __attribute__((__packed__));                                                                                           // 1
    float VinTurnOn __attribute__((__packed__)), VinShutOff __attribute__((__packed__)),
           VinDisableRadio __attribute__((__packed__)), tkCurrent __attribute__((__packed__)), tkPower __attribute__((__packed__));                                       // 20
    BOOL   mpp __attribute__((__packed__)), module __attribute__((__packed__));
    BYTE   ov __attribute__((__packed__)),  oc __attribute__((__packed__));
    BYTE   radioPower __attribute__((__packed__)),
           edAddr __attribute__((__packed__)),           gwAddr __attribute__((__packed__)),
           repeaterChannel __attribute__((__packed__)),  repeaterPower __attribute__((__packed__));
    /*
    BOOL   production       : 1 __attribute__((__packed__)),
           ensureWDReset    : 1 __attribute__((__packed__)),
           use10kbod        : 1 __attribute__((__packed__)),
           useFEC           : 1 __attribute__((__packed__)),
           hoppingAllowed   : 1 __attribute__((__packed__)),
           isRelay          : 1 __attribute__((__packed__)),
           is500Always      : 1 __attribute__((__packed__)),
           searchCommunication : 1 __attribute__((__packed__));
    */
    BYTE   flags __attribute__((__packed__));
    BYTE   showState __attribute__((__packed__));
    signed char   CriticalLevel500 __attribute__((__packed__)), CriticalLevel12 __attribute__((__packed__));
    UINT16 version __attribute__((__packed__));
    UINT16 defCyclesTo500 __attribute__((__packed__)), defCyclesTo10 __attribute__((__packed__)),
           defCyclesToStep500Channel __attribute__((__packed__)), defCyclesToStep10Channel __attribute__((__packed__)),
           defCycles2SoftKick __attribute__((__packed__)), defCycles2HardKick __attribute__((__packed__));
    UINT16 imageAddr __attribute__((__packed__));
    UINT16 versionLow __attribute__((__packed__)), versionHigh __attribute__((__packed__));
    BYTE   starting500Channel __attribute__((__packed__));
    //}}}
} pageE0 = {//{{{                                                                                                  //
    0xAAAA,
    (BYTE)0xFF, (BYTE)0xFF, 0xFFFF, //  permanent !!! Normal
    { 0, 0, 0, 0, 0, 0 },
    {
      //-0.000784329277, 0.069999928482, -1.557036645365, -229.914041475601 + 32.0
      -0.000693, 0.0556, 0.613, -285.44 //+ 300.0
    }, // temperature compensation
    {  2435.0, 0.0000004, 73.0 }, // vref polynomials
    //  Vout  0              Pin  1               Iout   2         Vin  3         Text 4       Tint  5
    { {24.096 * 0.5, 0.0}, {28.426, 0.0}, {5.536, -502.0}, {22.063 * 0.5, 0.0}, {0.435, 625.0}, {0.39, 755.0 } },
    0L, 64L,
    { 'A', 'N', 'D', 'S',  0,  'l', 'a', 'r', 'A', 'u', 'g', 'u', 's', 't',  0,   7 },
    { 'A', 'u', 'g', 'u', 's', 't',  0,   7,  'A', 'u', 'g', 'u', 's', 't',  0,   7 },
    {{(BYTE)0x01, 0}, {(BYTE)0x02, 0}, {(BYTE)0x03, 0}, {(BYTE)0x04, 0}},
    0, (BYTE)0, (BYTE)0,
    (unsigned short)0, (unsigned short)0, (unsigned short)0, (unsigned short)0, (unsigned short)0,
    (BYTE)0, //  should be changed to 0 in the production version
    24000.0*0.5, 20000.0*0.5, 12000.0*0.5,  // 0 for debug
    0.00002, 0.00005,
    TRUE, TRUE,  // mpp & module
    (BYTE)220, (BYTE)200, // ov oc
    0xFF, 2, 1, 0, // radioPower, edAddr, gwAddr, repChan
    0xFF, // 500kbod Power, 0xCB => -10 db, we may think about +1 - FF, 0 0xFE, -5 0xDB, -15 0x8E, -20 0x4D, -25  0x51
    /*
    TRUE, TRUE, FALSE, FALSE, TRUE, // production, WDReset, 10kbod, FEC, hopping,
//    TRUE, FALSE, FALSE, 1,   // Repeater
//    FALSE, TRUE, TRUE, 1,   // Slave
    FALSE, FALSE, TRUE,   1,  // Search
//   FALSE, FALSE, FALSE, 1,    // Simple
    */
    //0b10010011, 1
    0x93, 1,
    87, 92,
    0xE1,
    120, 220, 20, 30,
    7, 400,
    0x476,
    0xE1, 0xE1,
    0xFF
};//}}}

struct { //{{{
    UINT16 barrier __attribute__((__packed__));                                                                                         // 2
    BYTE   netId __attribute__((__packed__)), myBunch __attribute__((__packed__));                                                                                  // 2
    UINT16 groupId __attribute__((__packed__));                                                                                         // 2
    // MAC number is exactly a SERIAL NUMBER
    // format:
    // 3bit weekday | 6 bit week | 7 bit year | 1 byte test center | 2 byte seq number | 1 byte random
    BYTE   myMac[6] __attribute__((__packed__));                                                                                        // 6
    // dF = -0.0005618976581444 t^3 + 0.0539971103485394 t^2 - 0.8658779608121610 t - 249.5883835010120000
    // t  = (adcs[5]-73000)) / 260
    float dFk[4] __attribute__((__packed__));                                                                                          // 16

    float vrefPolynom[3] __attribute__((__packed__));                                                                                  //* 12
    // Vout Pin Iout Vin Text Tint
    float linearK [6][2] __attribute__((__packed__));                                                                                  //* 48

    INT32  theDelta __attribute__((__packed__));                                                                                        // 4
    INT32  dF_Tolerance __attribute__((__packed__)); // 16 in the offset register                                                       // 4

    char curKey[ AES_SIZE ] __attribute__((__packed__));                                                                                // 16
    char curIV [ AES_SIZE ] __attribute__((__packed__));                                                                                // 16

    struct {
      BYTE   testNumber __attribute__((__packed__)); // testNumber & 0x80 != 0 - successful, otherwise - not successful, 7 bytes for test Number
      UINT32 testDate __attribute__((__packed__));   // UTC timestamp
    }tests[4] __attribute__((__packed__));                                                                                              // 20

    UINT32 installDate __attribute__((__packed__));  // UTC timestamp                                                                      4
    BYTE   azimuth __attribute__((__packed__)), positionInString __attribute__((__packed__));                                                                       // 2
    UINT16 string __attribute__((__packed__));
    UINT16 elevation __attribute__((__packed__));
    UINT16 latitude __attribute__((__packed__));
    UINT16 longitude __attribute__((__packed__));
    UINT16 altitude __attribute__((__packed__));                    // 10
    BYTE channel __attribute__((__packed__));                                                                                           // 1
    float VinTurnOn __attribute__((__packed__)), VinShutOff __attribute__((__packed__)),
           VinDisableRadio __attribute__((__packed__)), tkCurrent __attribute__((__packed__)), tkPower __attribute__((__packed__));                                       // 20
    BOOL   mpp __attribute__((__packed__)), module __attribute__((__packed__));
    BYTE   ov __attribute__((__packed__)),  oc __attribute__((__packed__));
    BYTE   radioPower __attribute__((__packed__)),
           edAddr __attribute__((__packed__)),           gwAddr __attribute__((__packed__)),
           repeaterChannel __attribute__((__packed__)),  repeaterPower __attribute__((__packed__));
    /*
    BOOL   production       : 1 __attribute__((__packed__)),
           ensureWDReset    : 1 __attribute__((__packed__)),
           use10kbod        : 1 __attribute__((__packed__)),
           useFEC           : 1 __attribute__((__packed__)),
           hoppingAllowed   : 1 __attribute__((__packed__)),
           isRelay          : 1 __attribute__((__packed__)),
           is500Always      : 1 __attribute__((__packed__)),
           searchCommunication : 1 __attribute__((__packed__));
    */
    BYTE   flags __attribute__((__packed__));
    BYTE   showState __attribute__((__packed__));
    signed char   CriticalLevel500 __attribute__((__packed__)), CriticalLevel12 __attribute__((__packed__));
    UINT16 version __attribute__((__packed__));
    UINT16 defCyclesTo500 __attribute__((__packed__)), defCyclesTo10 __attribute__((__packed__)),
           defCyclesToStep500Channel __attribute__((__packed__)), defCyclesToStep10Channel __attribute__((__packed__)),
           defCycles2SoftKick __attribute__((__packed__)), defCycles2HardKick __attribute__((__packed__));
    UINT16 imageAddr __attribute__((__packed__));
    UINT16 versionLow __attribute__((__packed__)), versionHigh __attribute__((__packed__));
    BYTE   starting500Channel __attribute__((__packed__));
    /*BOOL  tryOtherImageFirst : 1,
            use250kbod         : 1,
            speculative_report : 1,
            stay_in_rx         : 1,
            beat_spikes        : 1,
            theRest            : 3;
            */
    BYTE    flags2        __attribute__((__packed__));
    UINT16  prepAddr      __attribute__((__packed__));
    BYTE    rep_step      __attribute__((__packed__));
    BYTE    ov_startup   __attribute__((__packed__));
    BYTE    t_superhot   __attribute__((__packed__));
    BYTE    t_hot        __attribute__((__packed__));
    BYTE    t_cold        __attribute__((__packed__));
    BYTE    fallback_time __attribute__((__packed__));
    BYTE    network_cnst_1  __attribute__((__packed__));
    BYTE    network_cnst_2  __attribute__((__packed__));
    BYTE    oc_step  __attribute__((__packed__));
    BYTE    power_dissipation_limit  __attribute__((__packed__));
    BYTE    power_dissipation_time   __attribute__((__packed__));
    BYTE    wiggle_dchan             __attribute__((__packed__));
    BYTE    ov_step                  __attribute__((__packed__));
    BYTE    shunt                    __attribute__((__packed__));
    BYTE    max_off                  __attribute__((__packed__));
    //}}}
} pageF0 = {//{{{                                                                                                  //
    0xAAAA,
    (BYTE)0xFF, (BYTE)0xFF, 0xFFFF, //  permanent !!! Normal
    { 0, 0, 0, 0, 0, 0 },
    {
      //-0.000784329277, 0.069999928482, -1.557036645365, -229.914041475601 + 32.0
      -0.000693, 0.0556, 0.613, -285.44 //+ 300.0
    }, // temperature compensation
    {  2435.0, 0.0000004, 73.0 }, // vref polynomials
    //  Vout  0              Pin  1               Iout   2         Vin  3         Text 4       Tint  5
    { {24.096 * 0.5, 0.0}, {28.426, 0.0}, {5.536, -502.0}, {22.063 * 0.5, 0.0}, {0.435, 625.0}, {0.39, 755.0 } },
    0L, 64L,
    { 'A', 'N', 'D', 'S',  0,  'l', 'a', 'r', 'A', 'u', 'g', 'u', 's', 't',  0,   7 },
    { 'A', 'u', 'g', 'u', 's', 't',  0,   7,  'A', 'u', 'g', 'u', 's', 't',  0,   7 },
    {{(BYTE)0x01, 0xFFFFFFFF}, {(BYTE)0x02, 0xFFFFFFFF}, {(BYTE)0x03, 0xFFFFFFFF}, {(BYTE)0x04, 0xFFFFFFFF}},
    0xFFFFFFFF, (BYTE)0xFF, (BYTE)0xFF,
    (unsigned short)0xFFFF, (unsigned short)0xFFFF, (unsigned short)0xFFFF, (unsigned short)0xFFFF, (unsigned short)0xFFFF,
    (BYTE)0, //  should be changed to 0 in the production version
    24000.0*0.5, 20000.0*0.5, 12000.0*0.5,  // 0 for debug
    0.00002, 0.00005,
    TRUE, TRUE,  // mpp & module
    (BYTE)220, (BYTE)200, // ov oc
    0xFF, 2, 1, 0, // radioPower, edAddr, gwAddr, repChan
    0xFF, // 500kbod Power, 0xCB => -10 db, we may think about +1 - FF, 0 0xFE, -5 0xDB, -15 0x8E, -20 0x4D, -25  0x51
    /*
    TRUE, TRUE, FALSE, FALSE, TRUE, // production, WDReset, 10kbod, FEC, hopping,
//    TRUE, FALSE, FALSE, 1,   // Repeater
//    FALSE, TRUE, TRUE, 1,   // Slave
    FALSE, FALSE, TRUE,   1,  // Search
//   FALSE, FALSE, FALSE, 1,    // Simple
    */
    //0b10010011, 1
    0x97, 1,
    -87, -92,
    0xE2,
    120, 220, 20, 30,
    7, 400,
    0x476,
    0xE2, 0xE2,
    0xFF,
    //0b11101000
    1,
    0xFFFF,
    25, 160, 80, 70, 60, 0, 7, 11, 160,
    0,  0,   0, 10, 40, 32
};//}}}

struct { //{{{
    UINT16 barrier __attribute__((__packed__));                                                                                         // 2
    BYTE   netId __attribute__((__packed__)), myBunch __attribute__((__packed__));                                                                                  // 2
    UINT16 groupId __attribute__((__packed__));                                                                                         // 2
    // MAC number is exactly a SERIAL NUMBER
    // format:
    // 3bit weekday | 6 bit week | 7 bit year | 1 byte test center | 2 byte seq number | 1 byte random
    BYTE   myMac[6] __attribute__((__packed__));                                                                                        // 6
    // dF = -0.0005618976581444 t^3 + 0.0539971103485394 t^2 - 0.8658779608121610 t - 249.5883835010120000
    // t  = (adcs[5]-73000)) / 260
    float dFk[4] __attribute__((__packed__));                                                                                          // 16

    float vrefPolynom[3] __attribute__((__packed__));                                                                                  //* 12
    // Vout Pin Iout Vin Text Tint
    float linearK [6][2] __attribute__((__packed__));                                                                                  //* 48

    //INT32  theDelta __attribute__((__packed__));                                                                                        // 4
    //INT32  dF_Tolerance __attribute__((__packed__)); // 16 in the offset register                                                       // 4
    UINT16 syncword __attribute__((__packed__));
    INT16  cur_noise __attribute__((__packed__));
    BYTE vin_disable_radio __attribute__((__packed__));                                                                                           // 1
    BYTE rbunch __attribute__((__packed__)); 
    BYTE gbunch __attribute__((__packed__)); 
    BYTE byte_reserved __attribute__((__packed__));                                                                                          

    char curKey[ AES_SIZE ] __attribute__((__packed__));                                                                                // 16
    char curIV [ AES_SIZE ] __attribute__((__packed__));                                                                                // 16

    struct {
      BYTE   testNumber __attribute__((__packed__)); // testNumber & 0x80 != 0 - successful, otherwise - not successful, 7 bytes for test Number
      UINT32 testDate __attribute__((__packed__));   // UTC timestamp
    }tests[4] __attribute__((__packed__));                                                                                              // 20

    UINT32 installDate __attribute__((__packed__));  // UTC timestamp                                                                      4
    BYTE   azimuth __attribute__((__packed__)), positionInString __attribute__((__packed__));                                                                       // 2
    UINT16 string __attribute__((__packed__));
    UINT16 elevation __attribute__((__packed__));
    UINT16 latitude __attribute__((__packed__));
    UINT16 longitude __attribute__((__packed__));
    UINT16 altitude __attribute__((__packed__));                    // 10
    BYTE channel __attribute__((__packed__));                                                                                           // 1
    //float VinTurnOn __attribute__((__packed__)), VinShutOff __attribute__((__packed__)),  VinDisableRadio __attribute__((__packed__)), 
    BYTE byte_reserved_2[12] __attribute__((__packed__));                                                                                          
    float tkCurrent __attribute__((__packed__)), tkPower __attribute__((__packed__));                                       // 20
    BOOL   mpp __attribute__((__packed__)), module __attribute__((__packed__));
    BYTE   ov __attribute__((__packed__)),  oc __attribute__((__packed__));
    BYTE   radioPower __attribute__((__packed__)),
           edAddr __attribute__((__packed__)),           gwAddr __attribute__((__packed__)),
           repeaterChannel __attribute__((__packed__)),  repeaterPower __attribute__((__packed__));
    /*
    BOOL   production       : 1 __attribute__((__packed__)),
           ensureWDReset    : 1 __attribute__((__packed__)),
           use10kbod        : 1 __attribute__((__packed__)),
           useFEC           : 1 __attribute__((__packed__)),
           hoppingAllowed   : 1 __attribute__((__packed__)),
           isRelay          : 1 __attribute__((__packed__)),
           is500Always      : 1 __attribute__((__packed__)),
           searchCommunication : 1 __attribute__((__packed__));
    */
    BYTE   flags __attribute__((__packed__));
    BYTE   showState __attribute__((__packed__));
    signed char   CriticalLevel500 __attribute__((__packed__)), CriticalLevel12 __attribute__((__packed__));
    UINT16 version __attribute__((__packed__));
    UINT16 defCyclesTo500 __attribute__((__packed__)), defCyclesTo10 __attribute__((__packed__)),
           defCyclesToStep500Channel __attribute__((__packed__)), defCyclesToStep10Channel __attribute__((__packed__)),
           defCycles2SoftKick __attribute__((__packed__)), defCycles2HardKick __attribute__((__packed__));
    UINT16 imageAddr __attribute__((__packed__));
    UINT16 versionLow __attribute__((__packed__)), versionHigh __attribute__((__packed__));
    BYTE   starting500Channel __attribute__((__packed__));
    /*BOOL    tryOtherImageFirst : 1,
            use250kbod         : 1,
            theRest            : 6;
            */
    BYTE    flags2        __attribute__((__packed__));
    UINT16  prepAddr      __attribute__((__packed__));
    BYTE    rep_step      __attribute__((__packed__));
    BYTE    ov_startup   __attribute__((__packed__));
    BYTE    t_superhot   __attribute__((__packed__));
    BYTE    t_hot        __attribute__((__packed__));
    BYTE    t_cold        __attribute__((__packed__));
    BYTE    fallback_time __attribute__((__packed__));
    BYTE    network_cnst_1  __attribute__((__packed__));
    BYTE    network_cnst_2  __attribute__((__packed__));
    BYTE    oc_step  __attribute__((__packed__));
    BYTE    power_dissipation_limit  __attribute__((__packed__));
    BYTE    power_dissipation_time   __attribute__((__packed__));
    BYTE    wiggle_dchan             __attribute__((__packed__));
    BYTE    ov_step                  __attribute__((__packed__));
    BYTE    shunt                    __attribute__((__packed__));
    BYTE    max_off                  __attribute__((__packed__));
    BYTE    vin_limit                __attribute__((__packed__));
    BYTE    vin_turn_on              __attribute__((__packed__));
    BYTE    vin_switch_off           __attribute__((__packed__));
    float e __attribute__((__packed__)), de __attribute__((__packed__));
    //}}}
} page = {//{{{                                                                                                  //
    0xAAAA,
    (BYTE)0xFF, (BYTE)0xFF, 0xFFFF, //  permanent !!! Normal
    { 0, 0, 0, 0, 0, 0 },
    {
      //-0.000784329277, 0.069999928482, -1.557036645365, -229.914041475601 + 32.0
      -0.000693, 0.0556, 0.613, -285.44 //+ 300.0
    }, // temperature compensation
    {  2495.0, 0.0000004, 73.0 }, // vref polynomials
    //  Vout  0              Pin  1               Iout   2         Vin  3         Text 4       Tint  5
    { {24.096 * 0.5, 0.0}, {28.426, 0.0}, {5.536, -502.0}, {22.063 * 0.5, 0.0}, {0.435, 625.0}, {0.39, 755.0 } },
    0xF0F0, 75, 6, 0, 255, 0,
    { 'A', 'N', 'D', 'S',  0,  'l', 'a', 'r', 'A', 'u', 'g', 'u', 's', 't',  0,   7 },
    { 'A', 'u', 'g', 'u', 's', 't',  0,   7,  'A', 'u', 'g', 'u', 's', 't',  0,   7 },
    {{(BYTE)0x01, 0xFFFFFFFF}, {(BYTE)0x02, 0xFFFFFFFF}, {(BYTE)0x03, 0xFFFFFFFF}, {(BYTE)0x04, 0xFFFFFFFF}},
    0xFFFFFFFF, (BYTE)0xFF, (BYTE)0xFF,
    (unsigned short)0xFFFF, (unsigned short)0xFFFF, (unsigned short)0xFFFF, (unsigned short)0xFFFF, (unsigned short)0xFFFF,
    (BYTE)0, //  should be changed to 0 in the production version
    //24000.0*0.5, 20000.0*0.5, 12000.0*0.5,  // 0 for debug
    
    
    {0,0,0,0,0,0,0,0,0,0,0,0},
    
    0.00002, 0.00005,
    TRUE, TRUE,  // mpp & module
    (BYTE)222, (BYTE)222, // ov oc
    0xFF, 2, 1, 0, // radioPower, edAddr, gwAddr, repChan
    0xFF, // 500kbod Power, 0xCB => -10 db, we may think about +1 - FF, 0 0xFE, -5 0xDB, -15 0x8E, -20 0x4D, -25  0x51
    /*
    TRUE, TRUE, FALSE, FALSE, TRUE, // production, WDReset, 10kbod, FEC, hopping,
//    TRUE, FALSE, FALSE, 1,   // Repeater
//    FALSE, TRUE, TRUE, 1,   // Slave
    FALSE, FALSE, TRUE,   1,  // Search
//   FALSE, FALSE, FALSE, 1,    // Simple
    */
    //0b10010011, 1
    0x97, 1,
    -87, -92,
    0xE2,
    180, 20, 3, 2,
    8, 400,
    0x476,
    0xE2, 0xE2,
    0xFF,
    // 7654 3210
    // 0000 1001
    (BYTE)0x9,
    0xFFFF,
    25, 160, 80, 70, 60, 0, 7, 11, 160,
    0,  0,   1, 10, 40, 32,
    120, 100,
    0.0, 0.0
};//}}}

struct{
    UINT16 barrier __attribute__((__packed__));
    UINT16 nDevs __attribute__((__packed__));
    BYTE curKey[ AES_SIZE ] __attribute__((__packed__));
    BYTE curIV [ AES_SIZE ] __attribute__((__packed__));
    BYTE channel __attribute__((__packed__));

    float dFk[4] __attribute__((__packed__));
    float tk[2]  __attribute__((__packed__));

    INT16  theDelta __attribute__((__packed__));                                                                                        // 2
    INT32  iRef  __attribute__((__packed__));                                                                                            // 4
    BYTE   dF_Tolerance __attribute__((__packed__));
    BOOL   useADC __attribute__((__packed__));
    BYTE   power __attribute__((__packed__));
    BOOL   long_packets __attribute__((__packed__));
    BYTE   mac[ 6 ] __attribute__((__packed__));
    BOOL   isHopper __attribute__((__packed__));
    BYTE   edAddr __attribute__((__packed__)), gwAddr __attribute__((__packed__));
    BOOL   longReport __attribute__((__packed__));
    BYTE   maxDevs __attribute__((__packed__));
    BYTE   bootDelay __attribute__((__packed__));
    UINT32 secondsJoinEnabled __attribute__((__packed__));
    BOOL   printSupercycle __attribute__((__packed__));
    UINT16 sw __attribute__((__packed__));
    //BOOL   newProtocol __attribute__((__packed__));    
    BYTE   zCount __attribute__((__packed__));
} page_gw = {                                                                                                   // 68
    0xAAAA, 0,
    { 'A', 'N', 'D', 'S',  0,  'l', 'a', 'r', 'A', 'u', 'g', 'u', 's', 't',  0,   7 },
    { 'A', 'u', 'g', 'u', 's', 't',  0,   7,  'A', 'u', 'g', 'u', 's', 't',  0,   7 },

    0,  //should be 0 in production

    { -0.000627, 0.0469, 0.253, -123.0 },
    { 0.435, 625.0 },
    5,    121000,  64,
    TRUE,
    0xED,
    TRUE,
    { 0,0,0,0,0,0 },
    FALSE,
    2,     1,
    TRUE,
    16,
    2,
    0, //14L*24L*3600L
    TRUE, 
    0xF0F0,
    //FALSE, 
    4
};

int original_version = 0xF1;

char *mac2serial( BYTE * mac){ //{{{
static char serial[ 32 ];
    unsigned int w, y, n, temp = 0; char letter;
    n    = mac[5] | (mac[4]<<8) | (mac[3]<<16);
    temp = mac[2] | (mac[1]<<8) | (mac[0]<<16);
    letter = ((temp>>6) & 0x1F) + 'A';
    y      = (temp>>11) & 0x7F;
    w      = (temp>>18) & 0x3F;
    sprintf( serial, "%02d%02d%c%06d", w, y, letter, n );
    return serial;
}//}}}
void serial2mac( char *serial, BYTE *mac ){//{{{
unsigned int w, y, n, temp; unsigned char letter;
    sscanf( serial, "%02d%02d%c%06d", &w, &y, &letter, &n );
    letter = (letter >= 'a') ? (letter-'a') : (letter-'A');
    mac[5] = n & 0xFF; mac[4] = (n>>8) & 0xFF; mac[3] = (n>>16)&0xFF;
    n = ( ( w & 0x3F ) << 18 ) | ( ( y & 0x7F ) << 11 ) | ( ( letter & 0x1F ) << 6 );
    mac[2] = n & 0xFF; mac[1] = (n>>8) & 0xFF; mac[0] = (n>>16)&0xFF;
}//}}}
BYTE ch2int(   unsigned char ch ) { //{{{
    if(      ch >= '0' && ch <= '9' ) ch-= '0';
    else if( ch >= 'A' && ch <= 'F' ) ch = 10 + (ch - 'A');
    else if( ch >= 'a' && ch <= 'f' ) ch = 10 + (ch - 'a');
    else                              ch = 0;
    return   ch & 0xF;
}//}}}
BOOL printPage( FILE *f );
BYTE readOneByte( char *ptr ){ return ( (ch2int(ptr[0])<<4) | ch2int(ptr[1]) ) & 0xFF; }
char *code( BYTE *ptr, BYTE l, UINT16 addr ){ //{{{
UINT32 checksum = l;
static char buffer[ 64 ], buf[ 16 ];
    checksum += (addr>>8) & 0xFF;
    checksum += addr & 0xFF;
    sprintf( buffer, ":%02X%04X00", l, addr );
    while( l-- ){ checksum += *ptr; sprintf( buf, "%02X", *ptr++ ); strcat( buffer, buf ); }
    checksum = 0x100 - ( checksum & 0xFF );
    sprintf( buf, "%02X\r\n", checksum & 0xFF ); strcat( buffer, buf );
    return buffer;
}//}}}

char secret[ 3 ][ 64 ];

BOOL link(char *serial, char *bootSec, char *image1, char *image2, unsigned short startAddr){ //{{{
char buf[ 64 ], fname[ 256 ];
char *ret = NULL;
char *images[ 2 ] = { image1, image2 };
int i, j;
UINT16 addr;
FILE *f1, *f;
    if( serial == NULL ) serial = mac2serial( page.myMac ); else serial2mac( serial, page.myMac );
    strcpy( fname, serial );
    f = fopen( strcat( fname, ".hex" ), "w" );  if( f == NULL ) return FALSE;

    for( f1 = fopen( bootSec, "r+" ); f1 && !feof( f1 ); ret = fgets( buf, 64, f1 ) ){
        if( buf[8] == '5' ){
            BYTE  tmp[ 16 ];
            addr = 0x3E0;
            for( j = 0; j < 2; j++, addr += 0x10 ){
                for( i = 0; i < 16; i++ )  tmp[ i ] = rand() & 0xFF;
                fputs( code( tmp, 0x10, addr ), f );
                memset( secret[ j ], 0, 64 );
                strcpy( secret[ j ], code( tmp, 0x10, addr )+9 );
            }
            //fputs( code( page.myMac, 6, 0x3DA ), f );
            memcpy( tmp, page.myMac, 6 ); tmp[6] = page.ov; tmp[7] = page.oc;
            strcpy( secret[ 2 ], code( tmp, 8, 0x3DA )+9 );
            fputs( code( tmp, 8, 0x3C8 ), f );
        }
        if( ret && buf[7] == '0') fputs( buf, f );
    }
    if( f1 ) fclose( f1 );

    for( i = 0; i < 2; i++ ){
        for( f1 = fopen( images[ i ], "r+" ); f1 && !feof( f1 ); ret = fgets( buf, 64, f1 ) )
            if( ret && buf[7] == '0' && strncmp( buf, ":03000000", 9 ) /*&&  strncmp( buf, ":00000001FF", 12 )*/  ) fputs( buf, f );
        if( f1 ) fclose( f1 );
    }

    printPage( f );
    fclose( f );
    return TRUE;
}//}}}

BOOL linkgw( char *serial, char *bootSec, char *image ){
char buf[ 64 ], fname[ 256 ];
char *ret = NULL;
int i, j;
FILE *f1, *f;
UINT16 addr = 0x7C00, l;
BYTE *pagePtr = (BYTE *)&page_gw;
unsigned int  byteCount = sizeof( page_gw );

    if( serial == NULL ) serial = mac2serial( page_gw.mac ); else serial2mac( serial, page_gw.mac );
    strcpy( fname, serial );
    f = fopen( strcat( fname, "_gw.hex" ), "w" );  if( f == NULL ) return FALSE;
    for( f1 = fopen( bootSec, "r+" ); f1 && !feof( f1 ); ret = fgets( buf, 64, f1 ) )
        if( ret && buf[7] == '0' &&  strncmp( buf, ":00000001FF", 12 )  ) fputs( buf, f );
    if( f1 ) fclose( f1 );
    else { printf( "cannot open boot sector file %s!\n", bootSec ); fclose( f ); return FALSE; }
    for( f1 = fopen( image, "r+" ); f1 && !feof( f1 ); ret = fgets( buf, 64, f1 ) )
        if( ret && buf[7] == '0' && strncmp( buf, ":03000000", 9 ) /*&&  strncmp( buf, ":00000001FF", 12 )*/  ) fputs( buf, f );
    if( f1 ) fclose( f1 );
    else { printf( "cannot open image file %s!\n", image ); fclose( f ); return FALSE; }

    if( *pagePtr != 0xAA ){ puts( "page does not have barrier!" ); fclose( f ); return FALSE; }
    printf( "size of gw page is %d\n", byteCount );
    while( byteCount ){//{{{ writing the hex file
        i = min( 0x10, byteCount );
        fputs( code( pagePtr, i, addr ), f );
        pagePtr += i; addr += i; byteCount -= i;
    }//}}}

    fclose( f );
    return TRUE;
}

BOOL printPageD8(FILE *f){//{{{
UINT16 addr = 0x7C00, l;
BYTE *pagePtr = (BYTE *)&pageD8;
unsigned int  byteCount = sizeof( pageD8 ), i;
    if( *pagePtr != 0xAA ) return FALSE;
    printf( "size of page is %d\n", byteCount );
    while( byteCount ){//{{{ writing the hex file
        i = min( 0x10, byteCount );
        fputs( code( pagePtr, i, addr ), f );
        pagePtr += i; addr += i; byteCount -= i;
    }//}}}
    return TRUE;
}//}}}


BOOL linkD8(char *serial, char *d8){ //{{{
char buf[ 64 ], fname[ 256 ];
char *ret = NULL;
FILE *f1, *f;
    if( serial == NULL ) serial = mac2serial( page.myMac ); else serial2mac( serial, page.myMac );
    strcpy( fname, serial );
    f = fopen( strcat( fname, ".hex" ), "w" );  if( f == NULL ) return FALSE;

    for( f1 = fopen( d8, "r+" ); f1 && !feof( f1 ); ret = fgets( buf, 64, f1 ) )
        if( ret && buf[7] == '0' && /*strncmp( buf, ":03000000", 9 ) &&*/  strncmp( buf, ":00000001FF", 12 )  ) fputs( buf, f );
    if( f1 ) fclose( f1 );

    printPageD8( f );
    fclose( f );
    return TRUE;
}//}}}

int min(int i, int j){ return i < j ? i : j; }
BOOL printPage(FILE *f){//{{{
UINT16 addr = 0x7C00, l;
BYTE *pagePtr = (BYTE *)&page;
unsigned int  byteCount = sizeof( page ), i;
    if( *pagePtr != 0xAA ) return FALSE;
    printf( "size of page is %d\n", byteCount );
    while( byteCount ){//{{{ writing the hex file
        i = min( 0x10, byteCount );
        fputs( code( pagePtr, i, addr ), f );
        pagePtr += i; addr += i; byteCount -= i;
    }//}}}
    return TRUE;
}//}}}

BYTE xtob( char *ptr )    { unsigned int i; sscanf(ptr, "%02X", &i ); return (BYTE)( i & 0xFF ); }
void readFromHexFile( char *fname, void *_ptr, int len, UINT16 startAddr ){ //{{{
FILE *f = fopen( fname, "r+" );
char buffer[ 64 ];
BYTE *ptr = _ptr;
int l, addr, type, i, j;
    memset( ptr, 0xFF, len );
    while( !feof( f ) ){
        if( !fgets( buffer, 64, f ) ){  break; } //puts( "Problems readiding input HEX file" );  exit( 2 ); }
        sscanf( buffer, ":%02X%04X%02X", &l, &addr, &type );
        if( ( type == 0 ) && ( ( startAddr <= (addr+l) ) && ( addr < ( startAddr + len ) ) ) )
            for( i = 0; i < l; i++, addr++ )
                if( ( addr >= startAddr ) && ( addr < ( startAddr + len ) ) ) ptr[ addr - startAddr ] = xtob( &buffer[ 2*i + 9 ] );
        if( addr > (startAddr + len) ) break;
    }
    fclose( f );
}//}}}
typedef unsigned int pointer;
void readC3Page( char *fname ){ //{{{
    readFromHexFile( fname, &pageC3, sizeof( pageC3 ), 0x7C00 );
    if( pageC3.barrier != 0xAAAA ) exit( 1 );

    memcpy( &page, &pageC3, (int)( (pointer)&page.mpp - (pointer)&page ) );

    page.mpp = pageC3.mpp; page.module = pageC3.module;
    page.ov  = pageC3.ov;  page.oc     = pageC3.oc;

    page.linearK[0][0]  *= 0.5;
    page.linearK[3][0]  *= 0.5;
    //page.VinTurnOn       *= 0.5;
    //page.VinShutOff      *= 0.5;
    //page.VinDisableRadio *= 0.5;

    memcpy( &pageD8, &pageC3, (int)( (pointer)&pageD8.mpp - (pointer)&pageD8 ) );

    pageD8.mpp = pageC3.mpp; pageD8.module = pageC3.module;
    pageD8.ov  = pageC3.ov;  pageD8.oc     = pageC3.oc;

    pageD8.linearK[0][0]  *= 0.5;
    pageD8.linearK[3][0]  *= 0.5;
    pageD8.VinTurnOn       *= 0.5;
    pageD8.VinShutOff      *= 0.5;
    pageD8.VinDisableRadio *= 0.5;

    page.defCyclesTo500 = 120;                                              page.defCyclesTo10 = 220;
    page.defCyclesToStep500Channel = 15;                                    page.defCyclesToStep10Channel = 30;
    page.defCycles2SoftKick = 7;                                            page.defCycles2HardKick = 400;
    page.CriticalLevel500 = pageD8.CriticalLevel500;                        page.CriticalLevel12 = pageD8.CriticalLevel12;
}//}}}
void d8_to_e2( void ){
    memcpy( &page, &pageD8, (int)( (pointer)&page.mpp - (pointer)&page ) );

    page.mpp = pageD8.mpp; page.module = pageD8.module;
    page.ov  = pageD8.ov;  page.oc     = pageD8.oc;

    page.radioPower = pageD8.radioPower;                                    page.edAddr = pageD8.edAddr;
    page.gwAddr = pageD8.gwAddr;                                            page.repeaterChannel = pageD8.repeaterChannel;
    page.repeaterPower  = pageD8.repeaterPower;                             page.showState = pageD8.showState;
    //120, 220, 20, 30, 7, 400
    page.defCyclesTo500 = 120;                                              page.defCyclesTo10 = 220;
    page.defCyclesToStep500Channel = 15;                                    page.defCyclesToStep10Channel = 30;
    page.defCycles2SoftKick = 7;                                            page.defCycles2HardKick = 400;

    page.CriticalLevel500 = -82;                                            page.CriticalLevel12 = -92;

    page.flags = 0;
    if( pageD8.production )          page.flags |= 0x1;
    if( pageD8.ensureWDReset )       page.flags |= 0x2;
    if( pageD8.use10kbod )           page.flags |= 0x4;
    if( pageD8.useFEC )              page.flags |= 0x8;
    if( pageD8.hoppingAllowed )      page.flags |= 0x10;
    if( pageD8.isRelay )             page.flags |= 0x20;
    if( pageD8.is500Always )         page.flags |= 0x40;
    if( pageD8.searchCommunication ) page.flags |= 0x80;

    page.rep_step      = 25;
    //page.ov_fallback   = page.oc_fallback = page.toff_fallback = page.ton_fallback = page.fallback_time = 0;
    page.network_cnst_1  = 7;    page.network_cnst_2  = 11;
    page.showState = 1;
}
void d8_to_f( void ){
    d8_to_e2();
    page.rep_step = 25;
    page.ov_startup = page.ov;
    page.t_superhot = 80;
    page.t_hot      = 70;
    page.t_cold     = 60;
    page.fallback_time = 0;
    page.oc_step    = 160;
    page.power_dissipation_limit    = 200;
    page.power_dissipation_time     = 0;
    page.wiggle_dchan               = 1;
    page.ov_step                    = 10;
    page.shunt                      = 40;
    page.max_off                    = 32;

}
void e1_to_e2( void ){
    memcpy( &page, &pageE0, sizeof( pageE0 ) );
    page.rep_step      = 25;
    //page.ov_fallback   = page.oc_fallback = page.toff_fallback = page.ton_fallback = page.fallback_time = 0;
    page.network_cnst_1  = 7;    page.network_cnst_2  = 11;
    page.showState = 1;
}

void readD8Page( char *fname ){ //{{{
    readFromHexFile( fname, &pageD8, sizeof( pageD8 ), 0x7C00 );
    if( pageD8.barrier != 0xAAAA ) exit( 1 );
    d8_to_f();
}//}}}
void readE0Page( char *fname ){
    readFromHexFile( fname, &pageE0, sizeof( pageE0 ), 0x7C00 );
    if( pageE0.barrier != 0xAAAA ) exit( 1 );
    e1_to_e2();
}
char *mystrcat( char *fname, char *ext ){ static char buffer[ 256 ]; strcpy( buffer, fname ); strcat( buffer, ext ); }
void copy( char *destFname, char *srcFname ){ //{{{
char buffer[ 512 ];
FILE *dest = fopen( destFname, "w" );
FILE *src  = fopen( srcFname, "r" );
    if( ( src == NULL ) || ( dest == NULL ) ) return;
    while( !feof( src ) ){
        int l = fread(   buffer, 1, 512, src  );
        if( l ) fwrite(  buffer, 1, l,   dest );
    }
    fclose( src ); fclose( dest );
}//}}}

char *bootSect = "../ED/hex/fc-boot.hex",            *imageLow = "../ED/hex/and-stack-ed-g-1k.hex",    *imageHigh = "../ED/hex/and-stack-ed-g-15k.hex",
     *d8 = "p:/Firmware/TI_RevD8/and-stack-ed-v8.hex",
     *gw_boot = "../GW_BOU/hex/boot_gw.hex", *gw_boot_win = "../GW_BOU/hex/boot_gw-win.hex", *gw_image = "../GW_BOU/hex/gw.hex";
char comment[ 512 ];

void outPage( char *filename ){//{{{
FILE * f = fopen( filename, "w" );
time_t rawtime;
struct tm * timeinfo;
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    secret[2][12] = 0;
    fprintf( f, "MAC %s Converted on %s to version %X from version %X\r\n", secret[ 2 ], asctime( timeinfo ), page.version, original_version );
    fprintf( f, "Key is %s\r\n", secret[ 0 ] );
    fprintf( f, "IV  is %s\r\n", secret[ 1 ] );
    fprintf( f, "%s\r\n", comment );
    fprintf( f, "boot %s low %s high %s\n", bootSect, imageLow, imageHigh );
    //{{{ Page copied from C3 hexhacker
    fprintf( f, "serial number:%s\r\n", mac2serial( page.myMac ) );
    fprintf( f, "Bunch %x id %x stringId %x, MAC %02x%02x%02x%02x%02x%02x\r\n",
        page.myBunch, page.netId, page.groupId,
        page.myMac[0], page.myMac[1], page.myMac[2], page.myMac[3], page.myMac[4], page.myMac[5] );
    fprintf( f, "dFk %g %g %g %g\r\n", page.dFk[0], page.dFk[1], page.dFk[2], page.dFk[3] );
    fprintf( f, "vrefPolynom %g %g %g\r\n", page.vrefPolynom[0], page.vrefPolynom[1], page.vrefPolynom[2] );
    fprintf( f, "Vout %g %g \r\nPin %g %g\r\nIout %g %g\r\nVin %g %g\r\nText %g %g\r\nTint %g %g\r\n",
        page.linearK[0][0], page.linearK[0][1],
        page.linearK[1][0], page.linearK[1][1],
        page.linearK[2][0], page.linearK[2][1],
        page.linearK[3][0], page.linearK[3][1],
        page.linearK[4][0], page.linearK[4][1],
        page.linearK[5][0], page.linearK[5][1] );
    fprintf( f, "VinTurnOn %g \r\nVinShutOff %g\r\nVinDisableRadio %g\r\ntkCurrent %g\r\ntkPower %g\r\n",
          page.vin_turn_on*100.0, 
          page.vin_switch_off*100.0,
          page.vin_disable_radio*100.0,
          page.tkCurrent,
          page.tkPower);
    fprintf( f, "defaults: OV %d OC %d MPP %s Module %s channel %d\r\n",
        page.ov, page.oc, page.mpp?"ON":"OFF", page.module?"ON":"OFF", page.channel);
    //}}}
    fprintf( f, "Repeater %s Slave500 %s Searching %s 12kbod %s FEC %s Hopping %s\r\n",
        page.flags&0x20 ? "YES" : "NO", page.flags&0x40 ? "YES" : "NO", page.flags&0x80 ? "YES" : "NO",
        page.flags&0x4  ? "YES" : "NO", page.flags&0x8  ? "YES" : "NO", page.flags&0x10 ? "YES" : "NO" );
    fprintf( f, "Repeater channel %d 500kbod Power 0x%X Start500 Channel %d StartAddr 0x%X\r\n",
        page.repeaterChannel, page.repeaterPower, page.starting500Channel, page.imageAddr );
    fprintf( f, "Critical 500 %d   Critical 30 %d     Cycles to 500 %d    Cycles to 10 %d \r\nCycles to Step 500 channel %d Cycles to step 10 channel %d Cycles to Soft Kick %d  Cycles to Hard Kick %d \r\n",
        page.CriticalLevel500, page.CriticalLevel12, page.defCyclesTo500, page.defCyclesTo10,
        page.defCyclesToStep500Channel, page.defCyclesToStep10Channel,  page.defCycles2SoftKick, page.defCycles2HardKick );
    fprintf( f, "edAddr: %d gwAddr %d\r\n",  page.edAddr, page.gwAddr);
    fclose( f );
}//}}}


void outPageGw( char *filename ){//{{{
FILE * f = fopen( filename, "w" );
time_t rawtime;
struct tm * timeinfo;
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    secret[2][12] = 0;
    fprintf( f, "Gateway\n" );
    fprintf( f, "MAC %s Converted on %s\r\n", page_gw.mac, asctime( timeinfo ) );
    fprintf( f, "%s\r\n", comment );
    fprintf( f, "boot %s radio %s \n", gw_boot, gw_image );
    fprintf( f, "serial number:%s\r\n", mac2serial( page_gw.mac ) );
    fprintf( f, "MAC %02x%02x%02x%02x%02x%02x\r\n",
        page_gw.mac[0], page_gw.mac[1], page_gw.mac[2], page_gw.mac[3], page_gw.mac[4], page_gw.mac[5] );
    fprintf( f, "dFk %g %g %g %g\r\n", page_gw.dFk[0], page_gw.dFk[1], page_gw.dFk[2], page_gw.dFk[3] );
    fprintf( f, "Tk %g %g\r\n",        page_gw.tk[0],  page_gw.tk[1] );
    fprintf( f, "Channel %d\r\n",      page_gw.channel);
    fprintf( f, "12kbod YES FEC NO Hopping %s\r\n", page_gw.isHopper? "YES" : "NO");
    fprintf( f, "edAddr: %d gwAddr %d\r\n",  page_gw.edAddr, page_gw.gwAddr);
    fclose( f );
}//}}}

int main(int argc, char **argv){ //{{{
int i, channel = 0, ver = 0xF1, ea = 2, ga = 1;
char *sn    = NULL;
char *inputC3 = NULL, *inputD8 = NULL, *inputE = NULL, *additionalOutput = NULL, *inputE2 = NULL;
UINT16 startAddr = 0x476 ;
float vShutUp   = -1/*6000.0*/, vInK = -1/*22.063*0.5*/, vOutK = -1 /*24.096*0.5*/;
BYTE bunch = 0xFF, netId = 0xFF, rep_step = 25, channel500 = 0;
BOOL keep_network = FALSE, keep_levels = FALSE, keep_channel = FALSE, d8_output = FALSE, keep_channel500 = FALSE,
     ask_bar_code = FALSE, two_fifty_kbod = FALSE, speculative_reports = FALSE,
     is_repeater = FALSE, is_slave = FALSE, search = TRUE; 
signed char level500 = -87, level12 = -92;
BOOL is_gw = FALSE, is_windows = FALSE, use_adc = TRUE, is_no_slave = FALSE;
int ov_startup = -1, t_superhot = -1, t_hot = -1, t_cold = -1, oc_step = -1, fallback_time = -1,
    power_dissipation_limit  = -1, power_dissipation_time =-1,  wiggle_dchan = -1, ov_step = -1,
    shunt = -1, max_off = -1, tenKbod = -1;
int ov = -1, oc = -1, vCh255 = -1, parking = -1, stay_in_rx = -1;
float pInK = -1, iOutK = -1, vModOff = -1, vWakeUp = -1;
float dfk3 = 1E6, iOutOff = 1E6;

    srand( time( 0 ) ); memset( comment, 0, sizeof( comment ) );
    for( i = 1; i < argc; i++ ){ // command line parameters parsing
        if( strcmp( argv[ i ], "-sn"   )    == 0 ){ i++; sn = argv[ i ]; }
        if( strcmp( argv[ i ], "-mac"  )    == 0 ){ i++; sn = mac2serial( argv[ i ] ); }
        if( strcmp( argv[ i ], "-boot" )    == 0 ){ i++; bootSect = argv[ i ];  gw_boot_win = gw_boot = argv[ i ]; }
        if( strcmp( argv[ i ], "-low"  )    == 0 ){ i++; imageLow = argv[ i ];  }
        if( strcmp( argv[ i ], "-high" )    == 0 ){ i++; imageHigh = argv[ i ]; }
        if( strcmp( argv[ i ], "-ch"   )    == 0 ){ i++; channel = atoi( argv[ i ] ); }
        if( strcmp( argv[ i ], "-C3"   )    == 0 ){ i++; inputC3 = argv[ i ];  original_version = 0xC3; }
        if( strcmp( argv[ i ], "-D8"   )    == 0 ){ i++; inputD8 = argv[ i ];  original_version = 0xD8; }
        if( strcmp( argv[ i ], "-E1"   )    == 0 ){ i++; inputE  = argv[ i ];  original_version = 0xE1; }
        if( strcmp( argv[ i ], "-E2"   )    == 0 ){ i++; inputE2 = argv[ i ];  original_version = 0xE2; }
        if( strcmp( argv[ i ], "-addr" )    == 0 ){ i++; startAddr = (UINT16) strtoul( argv[ i ], 0, 16 ); }
        if( strcmp( argv[ i ], "-output" )  == 0 ){ i++; additionalOutput = argv[ i ]; }
        if( strcmp( argv[ i ], "-vshutup" ) == 0 ){ i++; sscanf( argv[ i ], "%g", &vShutUp ); }
        if( strcmp( argv[ i ], "-vin-k" ) == 0 )  { i++; sscanf( argv[ i ], "%g", &vInK ); }
        if( strcmp( argv[ i ], "-vout-k" ) == 0 ) { i++; sscanf( argv[ i ], "%g", &vOutK ); }
        if( strcmp( argv[ i ], "-bunch" )   == 0 ){ i++; bunch = atoi( argv[ i ] ); }
        if( strcmp( argv[ i ], "-netId" )   == 0 ){ i++; netId = atoi( argv[ i ] ); }
        if( strcmp( argv[ i ], "-ver" )     == 0 ){ i++; ver = (UINT16) strtoul( argv[ i ], 0, 16 ); }
        if( strcmp( argv[ i ], "-ea" )      == 0 ){ i++; ea = atoi( argv[ i ] ); }
        if( strcmp( argv[ i ], "-ga" )      == 0 ){ i++; ga = atoi( argv[ i ] ); }
        if( strcmp( argv[ i ], "-keep-network" ) == 0 )  keep_network = TRUE;
        if( strcmp( argv[ i ], "-level500" )     == 0 ){ i++; level500 = atoi( argv[ i ] ); }
        if( strcmp( argv[ i ], "-level12" )      == 0 ){ i++; level12  = atoi( argv[ i ] ); }
        if( strcmp( argv[ i ], "-keep-levels" )  == 0 )     keep_levels    = TRUE;
        if( strcmp( argv[ i ], "-keep-channel" ) == 0 )     keep_channel   = TRUE;
        if( strcmp( argv[ i ], "-d8-output" ) == 0 )        d8_output      = TRUE;
        if( strcmp( argv[ i ], "-keep-channel500" ) == 0 )  keep_channel500   = TRUE;
        if( strcmp( argv[ i ], "-12kbod" ) == 0 )           tenKbod        = TRUE;
        if( strcmp( argv[ i ], "-30kbod" ) == 0 )           tenKbod        = FALSE;
        if( strcmp( argv[ i ], "-250kbod" ) == 0 )          two_fifty_kbod = TRUE;
        if( strcmp( argv[ i ], "-ask-barcode" ) == 0 )      ask_bar_code   = TRUE;
        if( strcmp( argv[ i ], "-speculative-reports" ) == 0 )  speculative_reports = TRUE;
        if( strcmp( argv[ i ], "-rep-step" )      == 0 ){ i++; rep_step   = atoi( argv[ i ] ); }
        if( strcmp( argv[ i ], "-channel500" )    == 0 ){ i++; channel500 = atoi( argv[ i ] ); }
        if( strcmp( argv[ i ], "-repeater" )      == 0 )    is_repeater = TRUE;
        if( strcmp( argv[ i ], "-slave" )      == 0 )       is_slave = TRUE;
        if( strcmp( argv[ i ], "-no-slave" )      == 0 )    is_no_slave = TRUE;
        if( strcmp( argv[ i ], "-gw" )         == 0 )       is_gw = TRUE;
        if( strcmp( argv[ i ], "-radio_gw" )   == 0 )     {  i++; is_gw = TRUE; gw_image = argv[ i ]; }
        if( strcmp( argv[ i ], "-win" )        == 0 )        is_windows = TRUE;
        if( strcmp( argv[ i ], "-noadc" )     == 0 )        use_adc = FALSE;
        if( strcmp( argv[ i ], "-no-search" )  == 0 )        search  = FALSE;
        if( strcmp( argv[ i ], "-ov-startup" )  == 0 )    { i++; ov_startup = atoi( argv[ i ] );}
        if( strcmp( argv[ i ], "-t-superhot" )  == 0 )    { i++; t_superhot = atoi( argv[ i ] );}
        if( strcmp( argv[ i ], "-t-hot" )  == 0 )         { i++; t_hot      = atoi( argv[ i ] );}
        if( strcmp( argv[ i ], "-t-cold" )  == 0 )        { i++; t_cold     = atoi( argv[ i ] );}
        if( strcmp( argv[ i ], "-oc-step" )  == 0 )       { i++; oc_step    = atoi( argv[ i ] );}
        if( strcmp( argv[ i ], "-oc-time" )  == 0 )       { i++; fallback_time = atoi( argv[ i ] );}
        if( strcmp( argv[ i ], "-power-dissipation-limit" ) == 0 )  { i++; power_dissipation_limit = atoi( argv[ i ] );}
        if( strcmp( argv[ i ], "-power-dissipation-time" )  == 0 )  { i++; power_dissipation_time = atoi( argv[ i ] );}
        if( strcmp( argv[ i ], "-wiggle-dchan" )  == 0 )            { i++; wiggle_dchan = atoi( argv[ i ] );}
        if( strcmp( argv[ i ], "-ov-step" )  == 0 )                 { i++; ov_step = atoi( argv[ i ] );}
        if( strcmp( argv[ i ], "-stay-in-rx" )  == 0 )              stay_in_rx = TRUE; 
        if( strcmp( argv[ i ], "-dont-stay-in-rx" )  == 0 )         stay_in_rx = FALSE;
        if( strcmp( argv[ i ], "-parking" )  == 0 )                 parking = TRUE; 
        if( strcmp( argv[ i ], "-no-parking" )  == 0 )              parking = FALSE;
        if( strcmp( argv[ i ], "-shunt" )  == 0 )                   {  i++; shunt = atoi( argv[ i ] ); }
        if( strcmp( argv[ i ], "-max-off" )  == 0 )                 {  i++; max_off = atoi( argv[ i ] ); }
        if( strcmp( argv[ i ], "-fdef" )  == 0 )    {
          ov_step = 10; ov_startup = 160; t_superhot = 110; t_hot = 105; t_cold = 95; oc_step = 110;
          fallback_time = 10; power_dissipation_limit = 0;  power_dissipation_time = 0;
          wiggle_dchan = 1; shunt = 40; max_off = 32; vModOff = 10000; vWakeUp = 12000; dfk3 = -200;
        }

        if( strcmp( argv[ i ], "-ov" )  == 0 )                      {  i++; ov = atoi( argv[ i ] ); }
        if( strcmp( argv[ i ], "-oc" )  == 0 )                      {  i++; oc = atoi( argv[ i ] ); }
        if( strcmp( argv[ i ], "-iout-k" ) == 0 )                   { i++; sscanf( argv[ i ], "%g", &iOutK ); }
        if( strcmp( argv[ i ], "-iout-off" ) == 0 )                 { i++; sscanf( argv[ i ], "%g", &iOutOff ); }
        if( strcmp( argv[ i ], "-pin-k" ) == 0 )                    { i++; sscanf( argv[ i ], "%g", &pInK ); }
        if( strcmp( argv[ i ], "-dfk3" ) == 0 )                     { i++; sscanf( argv[ i ], "%g", &dfk3 ); }
        if( strcmp( argv[ i ], "-v-mod-off" ) == 0 )                { i++; sscanf( argv[ i ], "%g", &vModOff ); }
        if( strcmp( argv[ i ], "-v-wake-up" ) == 0 )                { i++; sscanf( argv[ i ], "%g", &vWakeUp ); }
        
        if( strcmp( argv[ i ], "-v-ch255" ) == 0 )                  { i++; sscanf( argv[ i ], "%d", &vCh255 ); }

        if( strcmp( argv[ i ], "-f-v50x2" )  == 0 )    {
            ov = 180; oc = 20; ov_step = 10; ov_startup = 160;
            vOutK = 21.58; pInK = 54.5; iOutK = 5.67; iOutOff = -511; vInK = 21.06;
            vShutUp = 60000;
            t_superhot = 110; t_hot = 105; t_cold = 95; oc_step = 110; fallback_time = 10;
            shunt = 40; vModOff = 10000; vWakeUp = 12000; dfk3 = -200;
        }
    }

    if( ask_bar_code ) {
        char line[128];
        char tmp_buf[128];
        puts("Enter the barcode and press <Enter>:" );
        if( gets( line ) ){
            if( sscanf(  line, "33070003 E %s\n", tmp_buf ) )  printf(  "read '%s'\n",  sn = tmp_buf );
            else  printf( "wrong format of barcode - '%s'\n", line );
        }else puts( "read NULL instead of barcode\n" );
    }

    if( !is_gw ){
        if(       inputC3 ) { readC3Page( inputC3 ); strcat( comment, "converted from version C3\r\n" ); }
        else if( inputD8 ) { readD8Page( inputD8 ); strcat( comment, "converted from version D8\r\n" ); }
        else if( inputE  ) { readE0Page( inputE  ); strcat( comment, "converted from version E1\r\n" ); }
        else if( inputE2 ) {
            printf( "Mark 0 %s %s %s\n", bootSect, imageLow, imageHigh );
            readFromHexFile( inputE2, &page, sizeof( page ), 0x7C00 ); strcat( comment, "converted from version E2\r\n" );
            sn = mac2serial( page.myMac ); puts( sn );
        }

        if( additionalOutput ) remove( additionalOutput );
        if( sn ) { remove( mystrcat( sn, ".hex" ) ); remove( mystrcat( sn, ".txt" ) );  }

        if( d8_output ){
            if( vShutUp >= 0 ) pageD8.VinDisableRadio = vShutUp;
            if( vOutK > 0 ) pageD8.linearK[0][0]   = vOutK;
            if( vInK > 0 ) pageD8.linearK[3][0]   = vInK;
            if( ! keep_channel )  pageD8.channel = channel;
            if( ! keep_network) { pageD8.myBunch          = bunch;      pageD8.netId           = netId; }
            if( ( tenKbod != -1 ) && tenKbod    ) pageD8.use10kbod = TRUE;
            pageD8.edAddr = ea; pageD8.gwAddr = ga;
            if( !linkD8( sn, d8 ) ) exit( 1 );
            d8_to_f();
        }else{
            if( dfk3 < 1E6 )   page.dFk[3]   = dfk3;
            if( vWakeUp >= 0 ) page.vin_turn_on  = (BYTE) (vWakeUp/100);
            if( vModOff >= 0 ) page.vin_switch_off = (BYTE) (vModOff/100);
            if( vShutUp >= 0 ) page.vin_disable_radio = (BYTE) (vShutUp/100);
            page.imageAddr = startAddr;
            if( vOutK > 0 ) page.linearK[0][0]   = vOutK;
            if( pInK > 0 )  page.linearK[1][0]   = pInK;
            if( iOutK > 0 ) page.linearK[2][0]   = iOutK;
            if( iOutOff <1E6 ) page.linearK[2][1]   = iOutOff;
            if( vInK > 0 )  page.linearK[3][0]   = vInK;
            if( ! keep_channel )     page.channel = channel;
            if( ! keep_network )     {
                page.myBunch          = bunch;      page.netId           = netId;
                if( tenKbod != -1 ) { if( tenKbod )     page.flags  |= 4;   else   page.flags  &= ~4; }
                if( is_repeater )    page.flags  |= 0x20;
                if( is_slave )       page.flags  |= 0x40;
                if( is_no_slave )    page.flags  &= ~0x40;
                if( search )         page.flags  |= 0x80; else page.flags &= 0x7F;
                if( two_fifty_kbod ) page.flags2 |= 2;
                if( speculative_reports ) page.flags2 |= 4;
                page.edAddr = ea; page.gwAddr = ga;   page.rep_step = rep_step;
            }
            if( ! keep_levels  )     { page.CriticalLevel500 = level500;   page.CriticalLevel12 = level12; }
            if( ! keep_channel500 )  {
                if( channel500 )    page.repeaterChannel = page.starting500Channel = channel500;
                else                page.repeaterChannel = page.starting500Channel = page.channel;
            }
            if(  stay_in_rx>0 ){
                if( stay_in_rx ) page.flags2 |= 0x10; else page.flags2 &= ~0x10;
            }
            if(  stay_in_rx )      page.flags2 |= 0x10;
            if(  ov >= 0 ) page.ov = ov & 0xFF;
            if(  oc >= 0 ) page.oc = oc & 0xFF;
            if(  ov_startup >= 0 ) page.ov_startup = ov_startup & 0xFF;
            if(  t_superhot >= 0 ) page.t_superhot = t_superhot & 0xFF;
            if(  t_hot >= 0 )      page.t_hot = t_hot & 0xFF;
            if(  t_cold >= 0 )     page.t_cold = t_cold & 0xFF;
            if(  oc_step >= 0 )    page.oc_step = oc_step & 0xFF;
            if(  fallback_time >= 0 )    page.fallback_time = fallback_time & 0xFF;
            if(  power_dissipation_limit >= 0 )    page.power_dissipation_limit = power_dissipation_limit & 0xFF;
            if(  power_dissipation_time >= 0 )     page.power_dissipation_time  = power_dissipation_time & 0xFF;
            if(  wiggle_dchan >= 0 )               page.wiggle_dchan  = wiggle_dchan & 0xFF;
            if(  ov_step >= 0 )                    page.ov_step       = ov_step & 0xFF;
            if(  shunt >= 0 )                      page.shunt         = shunt & 0xFF;
            if(  max_off >= 0 )                    page.max_off       = max_off & 0xFF;
            if(  vCh255 >= 0 )                     page.vin_limit     = vCh255 & 0xFF;
            page.version    = page.versionLow   = page.versionHigh   = ver;
            if( !link( sn, bootSect, imageLow, imageHigh, startAddr ) ) exit( 1 );
        }
        if( sn ) outPage( mystrcat( sn, ".txt" )  );  else outPage( mystrcat( mac2serial( page.myMac ) , ".txt" )  );
        if( additionalOutput ) copy( additionalOutput, mystrcat( mac2serial( page.myMac ), ".hex" ) );
    }else{ // create gateway bootable image
        if( is_windows ) gw_boot = gw_boot_win;
        page_gw.edAddr = ea; page_gw.gwAddr = ga;
        page_gw.channel = channel;
        page_gw.useADC = use_adc;
        if( !linkgw( sn, gw_boot, gw_image ) ) exit( 1 );
        if( sn ) outPageGw( mystrcat( sn, "_gw.txt" )  );  else outPageGw( mystrcat( mac2serial( page.myMac ) , "_gw.txt" )  );
        if( additionalOutput ) copy( additionalOutput, mystrcat( mac2serial( page.myMac ), "_gw.hex" ) );
    }
    exit( 0 );
}//}}}
