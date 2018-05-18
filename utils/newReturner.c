// INPUT   : main image hex file as produced from IAR compiler,
//           or output hex file produced from "first pass"
//           invocation of this code. 
// OUTPUT  : 32K hex file
// PURPOSE : This program takes as input the "main"
//           (not 8K) hex file of MidString (G+) code.
//           The intention is to "inject" code snippets
//           within the main image block that will allow
//           program execution to "return" to normal 
//           execution as the injected code is outside
//           where normal exectution would/should occur.
//           Noise mitigation issue (Tim B. and Anatoli L.)
//
// First Pass Invocation
//  ./returner.exe -i "../ED/hex/ampt-ms-i-full-low.hex" \
//                 -o "../ED/hex/ampt-ms-i-full-low+ret.hex" \
//                 -startAddr 5800 -endAddr 5BE1 \
//                 -retAddr 0 -rc
//
//  This "injects" two assembly intructions from addresses
//  0x5800 to 0x5BF1, ORL C6 #49 and NOP (over and over) 
//
// Second Pass Invocation (Takes output file from above as input,
//                         and overwrites this file)
//  ./returner.exe -i "../ED/hex/ampt-ms-i-full-low+ret.hex" \
//                 -o "../ED/hex/ampt-ms-i-full-low+ret.hex" \
//                 -startAddr 5BF0 -endAddr 5BF1 -retAddr 0
//
//  This injects assembly to jump to address 0x0000 (boot loader)
 
#include <stdio.h> // printf, scanf, remove require
#include <stdlib.h>
#include <string.h> // memset requires
#include <time.h>
#include <unistd.h>  // access function requires
const int OK = 0;
const int ERROR_START_ADDRESS = 255;
const int ERROR_DIED = 254;

typedef signed   short  INT16;
typedef unsigned short  UINT16;
typedef unsigned int    UINT32;
typedef unsigned char   BYTE;
typedef unsigned char   BOOL;
typedef int             INT32;

// #define _DEBUG_ 1  // Uncomment line to "trace program execution

//const int OK = 0;
//const int ERROR_START_ADDRESS = 255;
//const int ERROR_DIED = 254;

void die( char *why )
{ 
  puts( why ); 
  exit( ERROR_DIED); 
}

// This function determines and adds a valid 
// checksum to a Intel Hex Data Record line
// and returns the complete line.
char *code( BYTE *ptr, BYTE l, UINT16 addr )
{ //{{{
  UINT32 checksum = l;
  static char buffer[ 64 ], buf[ 16 ];
  checksum += (addr>>8) & 0xFF;
  checksum += addr & 0xFF;
  sprintf( buffer, ":%02X%04X00", l, addr );
  while( l-- )
    { 
      checksum += *ptr; 
      sprintf( buf, "%02X", *ptr++ ); 
      strcat( buffer, buf ); 
    }
    checksum = 0x100 - ( checksum & 0xFF );
    sprintf( buf, "%02X\r\n", checksum & 0xFF ); 
    strcat( buffer, buf );

    return buffer;
}//}}}


BYTE nibble( char ch )
{
  if( (ch >= 'A') && (ch <='F') ) return ch -'A' +10;
  if( (ch >= 'a') && (ch <='f') ) return ch -'a' +10;
  if( (ch >= '0') && (ch <='9') ) return ch -'0';

  return 0xFF;
}

BYTE hex2byte( char *ptr )
{ 
  return nibble( ptr[1] ) | ( nibble( ptr[0] ) << 4 ); 
}

int main(int argc, char **argv )
{
  int retValue = OK;

  int startAddr = -1, endAddr = -1, retAddr = -1, i;
  int highestDataRecordAddress = 0;

  char inputFile[512], outputFile[512]; 
  FILE *input, *output;
  BYTE image[ 0x8000 ];
  char data[256], buffer[256];

  BYTE jmp = 1, step = 3;

  memset( image, 0xFF, 0x8000 );
  memset( inputFile, 0, 512 );    
  memset( outputFile, 0, 512 );

  for( i = 1; i < argc; i++ )
    { // command line parameters parsing
      if( strcmp( argv[ i ], "-startAddr" ) == 0 ) 
		startAddr = (int) strtoul( argv[ ++i ], 0, 16 ); 
      if( strcmp( argv[ i ], "-endAddr"   ) == 0 ) 
		endAddr = (int) strtoul( argv[ ++i ], 0, 16 );   
      if( strcmp( argv[ i ], "-retAddr"   ) == 0 ) 
		retAddr = (int) strtoul( argv[ ++i ], 0, 16 );   
      if( strcmp( argv[ i ], "-i"         ) == 0 ) 
		strcpy( inputFile, argv[ ++i ] );   
      if( strcmp( argv[ i ], "-o"         ) == 0 ) 
		strcpy( outputFile, argv[ ++i ] );  
      if( strcmp( argv[ i ], "-rc"        ) == 0 )
		{ 
		  jmp = 0; 
		  step = 4; 
		}
    }

  printf("Command Line Arguments: inputFile %s outputFile %s startAddr %04X endAddr %04X retAddr %04X\n\n",
		 inputFile, outputFile, startAddr, endAddr, retAddr);
  
  if( startAddr < 0 ) 
    die( "Usage: returner.exe -i <inpFile> -o <outFile> -startAddr <hexStartaddr> -endAddr <hexEndaddr> -retAddr <hexRetaddr>\nYou did not specified startAddr." );
  if( endAddr < 0 ) 
    die( "Usage: returner.exe -i <inpFile> -o <outFile> -startAddr <hexStartaddr> -endAddr <hexEndaddr> -retAddr <hexRetaddr>\nYou did not specified endAddr." );
  if (endAddr > 0x5BFD)
    die( "Usage: returner.exe -i <inpFile> -o <outFile> -startAddr <hexStartaddr> -endAddr <hexEndaddr> -retAddr <hexRetaddr>\nendAddr must be less than or equal to 0x5BFD." );
  if( retAddr < 0 ) 
    die( "Usage: returner.exe -i <inpFile> -o <outFile> -startAddr <hexStartaddr> -endAddr <hexEndaddr> -retAddr <hexRetaddr>\nYou did not specified retAddr." );
  if( strlen( inputFile ) == 0 ) 
    die( "Usage: returner.exe -i <inpFile> -o <outFile> -startAddr <hexStartaddr> -endAddr <hexEndaddr> -retAddr <hexRetaddr>\nYou did not specified inputFile." );
  if( strlen( outputFile ) == 0 ) 
    die( "Usage: returner.exe -i <inpFile> -o <outFile> -startAddr <hexStartaddr> -endAddr <hexEndaddr> -retAddr <hexRetaddr>\nYou did not specified outputFile." );
  
  input = fopen( inputFile, "r" );

  if( input == NULL )
    die( "Usage: returner.exe -i <inpFile> -o <outFile> -startAddr <startaddr> -endAddr <endaddr> -retAddr <retaddr>\nCannot open inputFile." );
  
  while( fgets ( buffer , 256 , input ) != NULL )
    {
#ifdef _DEBUG__
      printf("buffer:%s",buffer);
#endif

      int l = 0, addr = 0;
      // If not a data record, skip
      if( memcmp( buffer+7, "00", 2 ) != 0 ) 
		{
#ifdef _DEBUG_
		  printf("\tskipping non data record:%s",buffer);
#endif
		  continue;
		}
#ifdef _DEBUG_
      printf("\tdata record:%s",buffer);
#endif
      sscanf( buffer, ":%2x%4x00%s", &l, &addr, data );  
	  
      if (addr > highestDataRecordAddress)
		highestDataRecordAddress = addr;
	  
      if( l )
		{ 
		  for( i = 0; i<l; i++ ) 
			image[ addr + i ] = hex2byte( &data[ i*2 ] );
		}
    }
  fclose( input );
    
#ifdef _DEBUG_
  printf("\tHighestDataRecordAddress %04X\n",highestDataRecordAddress);
#endif

  if ((startAddr - 16) < highestDataRecordAddress)
    { 
	  printf("ERROR: image file appears to have grown into returner space.\n");
      printf("\t: (startAddr - 16) = %04X  (highestDataRecordAddress) = %04X\n",
			 (startAddr - 16), highestDataRecordAddress);
	  printf("\tsupplied startAddr argument must be greater than %04X\n\n",highestDataRecordAddress);
      retValue = ERROR_START_ADDRESS;
	  
	  if (access( outputFile, F_OK) != -1)
		if (remove(outputFile) == 0)
		  printf("\tThe pre-existing outputfile %s has been successfully removed.\n",outputFile);
		else
		  printf("\tERROR: removing outputfile %s FAILED, it should not be used to create the file hex file.\n", outputFile);
	  else
		printf("\tThe outputfile %s was not created and does not exist.\n",outputFile);
	}
  else
    {
      for( i = startAddr; i < endAddr && i < (0x8000-3);  i+= step )
	{
	  if( jmp )
	    { 
	      // Adding lines LJMP retAddr (if retAddr 0, then boot loader)
	      image[i] = 2; 
	      image[i+1] = (retAddr >> 8 ) & 0xFF; 
	      image[i+2] = retAddr & 0xFF; 
	    }
	  else     
	    { 
	      // Adding 2 Assembly instructs.
	      // ORL C6 0x49
	      // NOP
	      image[i] = 0x43; 
	      image[i+1] = 0xC6; 
	      image[i+2] = 0x49; 
	      image[i+3] = 0;
	    }
	}   
      output = fopen( outputFile, "w" );
      
      if( output == NULL )
	die( "Usage: returner.exe -i <inpFile> -o <outFile> -startAddr <startaddr> -endAddr <endaddr> -retAddr <retaddr>\nCannot open outputFile." );
      
      for( i = 0; i < 0x8000; i+=16 )
	{
	  int empty = 1, j; 
	  // if any byte in the 16 bytes in image is NOT 0xFF
	  // then the line is not EMPTY (set empty to 0) and
	  // line will be included into the output file.
	  // The output file will include unaltered code from
	  // the input file as well as any code added by the
	  // invocation of this program
	  for( j = 0; j < 16; j++ ) 
	    if( image[ i+j ] != 0xFF )
	      {
		empty = 0; 
		break; 
	      }
	  if( !empty ) 
	    fputs( code( &image[i], 16, i ), output );
	  //#ifdef _DEBUG_
	  //else
	  //printf("image address has no data record  i:%04X\n",i);
	  //#endif
	}
      fclose( output );
    }
  return retValue;
}
