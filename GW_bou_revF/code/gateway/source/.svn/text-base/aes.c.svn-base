#include "hal_main.h"

#define AES_SIZE 16
#define NOPS     24

void load(unsigned char what, char *ptr){
unsigned char i;
    ENCCS = what | 0x01;
    for (i = 0; i < AES_SIZE; i++) ENCDI = ptr[i]; 
}
void wait(void){ while((ENCCS & 0x08) == 0) asm("NOP"); }
void nops(void){ unsigned char i = NOPS; while( i-- ) asm("NOP"); }
void code(    unsigned char what, unsigned char size,  char *from, char *to ){
unsigned int cnt, i;
    if( (size % AES_SIZE) == 0 ){
        
        for( cnt = size / AES_SIZE; cnt; cnt--, from+=AES_SIZE, to+=AES_SIZE){
            load( what, from );
            nops(); 
            for( i = 0; i<AES_SIZE; i++ ) to[i] = ENCDO; 
            wait();
        }
        
        //for( i = 0; i<size; i++) to[i] = from[i];
    }
}
void loadKey( char *key ){ load( 0x04, key ); wait(); }
void loadIV(  char *iv ){  load( 0x06, iv  ); wait(); }
void encode( unsigned char size,  char *from, char *to ){ code( 0x00, size, from, to ); }
void decode( unsigned char size,  char *from, char *to ){ code( 0x02, size, from, to ); }
