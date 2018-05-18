#include "hal_main.h"

#define AES_SIZE 16
#define NOPS     12

void load(const unsigned char what, char *ptr){
unsigned char i;
    ENCCS = what | 0x01;
    i = AES_SIZE; while( i-- ) ENCDI = *ptr++;
}

void wait_aes(void){ 
    while((ENCCS & 0x08) == 0) asm("NOP"); 
    //asm( "WC2: MOV A, 0xB3" );
    //asm( "ANL  A, #8h" );
    //asm( "JZ   WC2" );  
}

//void wait_aes( void );
//void wait_nops( void );
void code(    unsigned char what, char *from, char *to ){
    unsigned char i;//, cnt;
    //if( (size % AES_SIZE) == 0 ){
        //for( cnt = size / AES_SIZE; cnt; cnt--, from+=AES_SIZE ){
            load( what, from );
            i = NOPS;     while( i-- ) asm("NOP"); 
            //asm( "MOV R1, #0x20"); asm("WAIT_NOPS: NOP"); asm( "DJNZ R1, WAIT_NOPS" );
            //wait_nops();
            i = AES_SIZE; while( i-- ) *to++ = ENCDO;
            wait_aes();
        //}
    //}
}
/*
void loadKey( char *key ){ load( 0x04, key ); wait(); }
void loadIV(  char *iv ){  load( 0x06, iv  ); wait(); }
void encode( unsigned char size,  char *from, char *to ){ code( 0x00, size, from, to ); }
void decode( unsigned char size,  char *from, char *to ){ code( 0x02, size, from, to ); }
*/
