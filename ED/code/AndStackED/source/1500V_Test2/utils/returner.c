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
void die( char *why ){ puts( why ); exit( 1 ); }
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

void main(int argc, char **argv ){
int startAddr = -1, endAddr = -1, retAddr = -1, i;
char inputFile[512], outputFile[512]; 
FILE *input, *output;
BYTE image[ 0x8000 ];
char data[256], buffer[256];
BYTE jmp = 1, step = 3;
    memset( image, 0xFF, 0x8000 );
    memset( inputFile, 0, 512 );    memset( outputFile, 0, 512 );
    for( i = 1; i < argc; i++ ){ // command line parameters parsing
        if( strcmp( argv[ i ], "-startAddr" ) == 0 ) startAddr = (int) strtoul( argv[ ++i ], 0, 16 ); 
        if( strcmp( argv[ i ], "-endAddr"   ) == 0 ) endAddr = (int) strtoul( argv[ ++i ], 0, 16 );   
        if( strcmp( argv[ i ], "-retAddr"   ) == 0 ) retAddr = (int) strtoul( argv[ ++i ], 0, 16 );   
        if( strcmp( argv[ i ], "-i"         ) == 0 ) strcpy( inputFile, argv[ ++i ] );   
        if( strcmp( argv[ i ], "-o"         ) == 0 ) strcpy( outputFile, argv[ ++i ] );  
        if( strcmp( argv[ i ], "-rc"        ) == 0 ){ jmp = 0; step = 4; }
    }

    if( startAddr < 0 ) 
        die( "Usage: returner.exe -i <inpFile> -o <outFile> -startAddr <hexStartaddr> -endAddr <hexEndaddr> -retAddr <hexRetaddr>\nYou did not specified startAddr." );
    if( endAddr < 0 ) 
        die( "Usage: returner.exe -i <inpFile> -o <outFile> -startAddr <hexStartaddr> -endAddr <hexEndaddr> -retAddr <hexRetaddr>\nYou did not specified endAddr." );
    if( retAddr < 0 ) 
        die( "Usage: returner.exe -i <inpFile> -o <outFile> -startAddr <hexStartaddr> -endAddr <hexEndaddr> -retAddr <hexRetaddr>\nYou did not specified retAddr." );
    if( strlen( inputFile ) == 0 ) 
        die( "Usage: returner.exe -i <inpFile> -o <outFile> -startAddr <hexStartaddr> -endAddr <hexEndaddr> -retAddr <hexRetaddr>\nYou did not specified inputFile." );
    if( strlen( outputFile ) == 0 ) 
        die( "Usage: returner.exe -i <inpFile> -o <outFile> -startAddr <hexStartaddr> -endAddr <hexEndaddr> -retAddr <hexRetaddr>\nYou did not specified outputFile." );

    input = fopen( inputFile, "r" );
    if( input == NULL )
        die( "Usage: returner.exe -i <inpFile> -o <outFile> -startAddr <startaddr> -endAddr <endaddr> -retAddr <retaddr>\nCannot open inputFile." );
    while( fgets ( buffer , 256 , input ) != NULL ){
        int l = 0, addr = 0;
        if( memcmp( buffer+7, "00", 2 ) != 0 ) continue;
        sscanf( buffer, ":%2x%4x00%s", &l, &addr, data );  
        if( l ) for( i = 0; i<l; i++ ) image[ addr + i ] = hex2byte( &data[ i*2 ] );
    }
    fclose( input );
    
    for( i = startAddr; i < endAddr && i < (0x8000-3);  i+= step )
        if( jmp ){ image[i] = 2; image[i+1] = (retAddr >> 8 ) & 0xFF; image[ i+2 ] = retAddr & 0xFF; }
        else     { image[i] = 0x43; image[i+1] = 0xC6; image[ i+2 ] = 0x49; image[ i+3 ] = 0; }
            
    output = fopen( outputFile, "w" );
    if( output == NULL )
        die( "Usage: returner.exe -i <inpFile> -o <outFile> -startAddr <startaddr> -endAddr <endaddr> -retAddr <retaddr>\nCannot open outputFile." );
    for( i = 0; i < 0x8000; i+=16 ){
        int empty = 1, j; for( j = 0; j < 16; j++ ) if( image[ i+j ] != 0xFF ){ empty = 0; break; }
        if( !empty ) fputs( code( &image[i], 16, i ), output );
    }
    fclose( output );
    
}