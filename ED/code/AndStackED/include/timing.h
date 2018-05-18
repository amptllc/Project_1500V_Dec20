// LBC Header file contains all the defines relating
// LBC to time (TICKS) or when to take action based on
// LBC time.

//#define MKS_IN_TICK     200
//#define TICKS_IN_SEC    5000
//#define TICKS_IN_MS     5

#define MKS_IN_TICK     500
#define TICKS_IN_SEC    2000
#define TICKS_IN_MS     2

#define DATA_PREP       5
//2400
#define  _slot_2400              ( 200 * TICKS_IN_MS )
#define  _join_slot_2400         ( 200 * TICKS_IN_MS )
#define  _gw_delay_2400          ( 150 * TICKS_IN_MS )
#define  _loop_delay_2400        ( 200 * TICKS_IN_MS )

//12000
//#define  _slot_12k               ( 40 * TICKS_IN_MS )
//#define  _join_slot_12k          ( 40 * TICKS_IN_MS )
//#define  _gw_delay_12k           ( 30 * TICKS_IN_MS )
//#define  _loop_delay_12k         ( 40 * TICKS_IN_MS )

//12000
#define  _slot               ( 40 * TICKS_IN_MS )
#define  _join_slot          ( 40 * TICKS_IN_MS )
#define  _gw_delay           ( 30 * TICKS_IN_MS )
#define  _loop_delay         ( 40 * TICKS_IN_MS )

//1200
//#define  _slot               ( 350 * TICKS_IN_MS )
//#define  _join_slot          ( 350 * TICKS_IN_MS )
//#define  _gw_delay           ( 350 * TICKS_IN_MS )
//#define  _loop_delay         ( 350 * TICKS_IN_MS )

//#define _mppCycle            ( TICKS_IN_SEC / 16 ) 
#define _mppCycle            ( 60 * TICKS_IN_MS )
#define _dogFeeding          ( TICKS_IN_SEC / 16 )
#define _adjustFrequency     ( (UINT16)TICKS_IN_SEC * 2 )
#define _firstAdcMeasurement ( TICKS_IN_SEC / 50 )
#define _adcMeasurement      ( TICKS_IN_SEC / 100 )
#define _calibrationDelay    ( TICKS_IN_MS  )

