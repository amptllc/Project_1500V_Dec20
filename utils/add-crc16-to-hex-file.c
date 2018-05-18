#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* 
    Reads hex file and adds an CRC16 to the end of the file.
    It is assumede that the hex file contains main image of MidString in
    pages 2 - 21 (addresses 0x800 - 0x57FD inclusive). CRC 16 is placed at
    address 0x57FE. All lines containing 0xFF only are ignored. CRC16 is 
    computed by polynom X^16 + X^15 +X^2+1. Initial value - 0xFFFF. 
    Function is taken from http://www.nongnu.org/avr-libc/user-manual/group__util__crc.html
    
*/
typedef unsigned char   BYTE;
typedef unsigned short  UINT16;
typedef unsigned int    UINT32;
typedef unsigned int    BOOL;

#define TRUE  1
#define FALSE 0

char *usage = "Nothing was done.\nusage: add-crc-16-to-hex-file input-name.hex output-name.hex (--(dec/hex)-crc-location NNN) (--(dec/hex)-image-start NNN) (--(dec/hex)-image-end NNN) --one-crc";

#define CRC16_POLY 0x8005 
// taken form http://www.ti.com/lit/an/swra111d/swra111d.pdf
UINT16 crc16_update(UINT16 crcReg, BYTE crcData) { 
BYTE i; 
    for (i = 0; i < 8; i++) { 
        if (((crcReg & 0x8000) >> 8) ^ (crcData & 0x80)) crcReg = (crcReg << 1) ^ CRC16_POLY; 
        else                                               crcReg = (crcReg << 1); 
        crcData <<= 1; 
    } 
    return crcReg; 
}// culCalcCRC

// generate Intel hex line 
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


BYTE nibble( char ch ){
    if( (ch >= 'A') && (ch <='F') ) return ch -'A' +10;
    if( (ch >= 'a') && (ch <='f') ) return ch -'a' +10;
    if( (ch >= '0') && (ch <='9') ) return ch -'0';
    return 0xFF;
}
BYTE hex2byte( char *ptr ){ return nibble( ptr[1] ) | ( nibble( ptr[0] ) << 4 ); }

int max( int a, int b ){ return (a > b) ? a : b; }

int EOI = 0x5C00 - 2;
int BOI = 0x800;
int LOI;

BYTE image[ 0x8000 ];

void main( int argc, char **argv ){
UINT32 i, crc = 0xFFFF, crcAddr = 0x5C00 - 2, crc2 = 0;
FILE *f;
char input_file[512];
char output_file[512];
char buffer[256];
char data[256];
BOOL two_crc = FALSE;
unsigned char test_data[] = {0x03, 0x41, 0x42, 0x43};
    for( i = 0; i<4; i++ ) crc = crc16_update( crc, test_data[i] );
    printf( "Test CRC16 is 0x%04X\n", crc );
    crc = 0xFFFF;
    if( argc < 3 ){ puts( usage ); exit( 1 ); }   
    memset( input_file, 0, 512 );   memset( output_file, 0, 512 );
    for( i = 1; i < argc; i++ ){
        if(       strcmp( "--dec-crc-location", argv[i] ) == 0 ) crcAddr = strtoul( argv[ ++i ], NULL, 10 );
        else if( strcmp( "--dec-image-start",  argv[i] ) == 0 ) BOI     = strtoul( argv[ ++i ], NULL, 10 );
        else if( strcmp( "--dec-image-end",    argv[i] ) == 0 ) EOI     = strtoul( argv[ ++i ], NULL, 10 );
        else if( strcmp( "--hex-crc-location", argv[i] ) == 0 ) crcAddr = strtoul( argv[ ++i ], NULL, 16 );
        else if( strcmp( "--hex-image-start",  argv[i] ) == 0 ) BOI     = strtoul( argv[ ++i ], NULL, 16 );
        else if( strcmp( "--hex-image-end",    argv[i] ) == 0 ) EOI     = strtoul( argv[ ++i ], NULL, 16 );
        else if( strcmp( "--one-crc",          argv[i] ) == 0 ) two_crc = FALSE;
        else{
            if(       strlen( input_file  ) == 0 )  strcpy( input_file, argv[i] );
            else if( strlen( output_file ) == 0 ) strcpy( output_file, argv[i] );
            else { puts( usage ); exit( 1 ); }    
        }
    }
    printf( "Input file '%s' output file '%s' start 0x%X (%d) end 0x%X (%d) crcAddr 0x%X (%d)\n", input_file, output_file, BOI, BOI, EOI, EOI, crcAddr, crcAddr );
    LOI = EOI-BOI;
    memset( image, 0xFF, 0x8000 );
    // read hex file into image
    f = fopen( input_file, "r"  );
    if( f == NULL ){   printf( "Input file '%s' does not exists.\n%s\n", input_file, usage ); exit( 1 ); }
    while( fgets ( buffer , 256 , f ) != NULL ){
        int l = 0, addr = 0;
        sscanf( buffer, ":%2x%4x00%s", &l, &addr, data );  
        if( l && (addr>=BOI) ) for( i = 0; i<l; i++ ) if( (addr+i) < EOI ) image[ addr + i ] = hex2byte( &data[ i*2 ] );
        /* debug code
        printf( "line '%s'\naddr %x length %d\n", buffer, addr, l );
        for( i = 0; i<l; i++ ) printf( "%02X", image[ addr - BOI + i ] );
        puts("");    
        */        
    }
    fclose( f );
    // compute CRC
    for( i = BOI; i<EOI; i++ ) crc = crc16_update( crc, image[i] );
    image[ crcAddr ] = crc & 0xFF; image[ crcAddr+1 ] = (crc>>8) & 0xFF;
    if( two_crc ){
        for( i = BOI; i<EOI; i++ ) crc2 = crc16_update( crc2, image[i] );
        image[ crcAddr+2 ] = crc2 & 0xFF; image[ crcAddr+3 ] = (crc2>>8) & 0xFF;
    }
    // write the output file
    f = fopen( output_file, "w" );
    if( f == NULL ){ printf( "Cannot write output file '%s'.\n%s\n", output_file, usage ); exit( 1 ); }
    for( i = BOI; i < max( EOI, crcAddr + (two_crc ? 4 : 2) ); i+=16 ){
        int j, flag = 1; for( j = 0; flag && (j<16); j++ ) flag = (0xFF==image[ i + j ] );
        if( !flag ) fputs( code( &image[ i ], 16, i ), f ); 
    }
    fclose( f );
}

