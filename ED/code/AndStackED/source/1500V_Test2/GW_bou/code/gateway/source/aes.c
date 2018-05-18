#include "hal_main.h"

#define AES_SIZE 16
#define NOPS     32

void load( BYTE what, BYTE *ptr){
BYTE i;
    ENCCS = what | 0x01;
    for (i = 0; i < AES_SIZE; i++) ENCDI = ptr[i]; 
}
void wait(void){ while((ENCCS & 0x08) == 0)  asm("NOP"); }
void nops(void){ BYTE i = NOPS; while( i-- ) asm("NOP"); }
void code(       BYTE what, BYTE *from, BYTE *to ){
BYTE i;
//    if( (size % AES_SIZE) == 0 ){
//        for( cnt = size / AES_SIZE; cnt; cnt--, from+=AES_SIZE, to+=AES_SIZE){
            load( what, from );
            nops(); 
            for( i = 0; i<AES_SIZE; i++ ) to[i] = ENCDO; 
            wait();
//        }
//    }
}

void loadKey( BYTE *key ){ load( 0x04, key ); wait(); }
void loadIV(  BYTE *iv ){  load( 0x06, iv  ); wait(); }
void encode(  BYTE *from, BYTE *to ){ code( 0x00, from, to ); }
void decode(  BYTE *from, BYTE *to ){ code( 0x02, from, to ); }
