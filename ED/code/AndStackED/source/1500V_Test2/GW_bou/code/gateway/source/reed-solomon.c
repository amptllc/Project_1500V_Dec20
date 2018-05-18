#include "hal_main.h"
#include "main.h"
#include "reed-solomon.h"

extern void mymemset(void *a, BYTE c, BYTE l );
void zerofill( register BYTE *a, register BYTE l){ while( l-- ) *a++ = 0; }
extern void mymemcpy(void *a, void *b, BYTE c);

BYTE synBytes[MAXDEG];   // Decoder syndrome bytes 
BYTE genPoly[MAXDEG*2];  // generator polynomial 
/* galois arithmetic tables */
BYTE gexp[256];
BYTE glog[256];
//static BYTE i,j;
BYTE tmp1[ MAXDEG*2 ];
static BYTE tp[ MAXDEG ], tp1[ MAXDEG ];
static BYTE psi[MAXDEG],  psi2[MAXDEG],  D[MAXDEG];//, gamma[MAXDEG];
/* error locations found using Chien's search*/
BYTE ErrorLocs[NPAR];

void copy_poly(BYTE *dst, BYTE *src){ mymemcpy( dst, src, MAXDEG ); }
void zero_poly(BYTE *poly)          { zerofill( poly, MAXDEG ); }

BYTE ginv( BYTE elt){ return (gexp[255-glog[elt]]); }
BYTE gmult(BYTE a, BYTE b){ //{{{
    if( a == 0 || b == 0 ) return 0;
    return gexp[ ( glog[a] + glog[b] ) % 255 ];
}//}}}

/* polynomial arithmetic */
//void add_polys( BYTE *dst, BYTE *src)   { i = MAXDEG; while( i-- ) *dst++ ^= *src++; }
//void scale_poly(BYTE k,    BYTE *poly)  { i = MAXDEG; while( i-- ){ j = *poly; *poly++ = gmult( k, j ); } }
void mult_polys(BYTE *dst, BYTE *p1, BYTE *p2){//{{{
    BYTE i, j;
    zerofill( dst, MAXDEG*2 );
    for( i=0; i<MAXDEG; i++, p1++ ){
        zero_poly( tmp1+MAXDEG );
        //for(j = 0;            j<MAXDEG;       j++)  tp[j] = gmult(p2[j], p1[i]);
        j = MAXDEG;   do{ j--; tmp1[j] = gmult(p2[j], *p1 ); } while( j );
        //for(j = (MAXDEG*2)-1; j >= i;         j--){ tp[j] = tp[j-i]; if(j==i) break; }
        j = MAXDEG*2; do{ j--; tmp1[j] = tmp1[j-i]; }while( j > i );
        //zerofill( tp, i );
        for(j = i; j < (MAXDEG*2); j++)  dst[j] ^= tmp1[j];
        //j = MAXDEG*2; do{ j--; dst[j] ^= tp[j]; }while( j>i );
    }
}//}}}

/* Reed Solomon encode/decode routines */
void initialize_ecc(void){//{{{
    BYTE z, pinit, p1, p2, p3, p4, p5, p6, p7, p8, i;
    //BYTE *ptr;
    //zerofill( &pinit, 9 );
    pinit = p2 = p3 = p4 = p5 = p6 = p7 = p8 = 0;
    p1 = 1;
    
    gexp[255] = gexp[0] = 1;
    glog[0] = 0;			/* shouldn't log[0] be an error? */
    for (i = 1; i < 255; i++) {
        pinit = p8;
       
        p8 = p7;    p7 = p6;    p6 = p5;
        p5 = p4;    p4 = p3;    p3 = p2;
        p2 = p1;    p1 = pinit;

        p5 ^= pinit;    p4 ^= pinit;    p3 ^= pinit;
        
        //z = p1 + p2*2 + p3*4 + p4*8 + p5*16 + p6*32 + p7*64 + p8*128;
        z = p1 + 2*(p2 + 2*( p3 + 2*( p4 + 2*( p5 + 2*( p6 + 2*( p7 + 2*p8 ))))));
        //for( j = 7, ptr = &p7, z = p8; j>0; j--, ptr-- ) z = z*2 + *ptr;
        gexp[i] = z;
        glog[z] = i;  
    }
    /* multiply (x + a^n) for n = 1 to nbytes */
    zero_poly(tp1);
    tp1[0] = 1;
    for(i = 1; i <= NPAR; i++) {
        zero_poly( tp );
        tp[0] = gexp[i];		/* set up x+a^n */
        tp[1] = 1;
        mult_polys( genPoly, tp, tp1 );
        copy_poly(  tp1, genPoly );
    }
}//}}}
BYTE NErrors;
BYTE decode_data (unsigned char data[]){ //{{{
    BYTE sum, r, err, nz = 0, i, j; //kk,
    BYTE n, L, L2, d; 
    INT8 k; 
    zerofill( ErrorLocs, NPAR+1 );    
    for(r = 0; r < NPAR;  r++) {
        sum	= 0;
        for(n = 0; n < ML; n++)  sum = data[n] ^ gmult(gexp[r+1], sum);
        synBytes[r]  = sum;
        if( sum != 0 ) nz = 1;
    }
    // check if syndrome is all zeros 
    if(nz){
        /* Combined Erasure And Error Magnitude Computation 
         * 
         * Pass in the codeword, its size in bytes, as well as
         * an array of any known erasure locations, along the number
         * of these erasures.
         * 
         * Evaluate Omega(actually Psi)/Lambda' at the roots
         * alpha^(-i) for error locs i. 
         *
         * Returns 1 if everything ok, or 0 if an out-of-bounds error is found
         *
         */
        /* From  Cain, Clark, "Error-Correction Coding For Digital Communications", pp. 216. */
        //Modified_Berlekamp_Massey();
        zero_poly( D );   D[0] = 1;   //  initialize Gamma, the erasure locator polynomial
        copy_poly( psi, D);	
        
        k = -1; L = 0;
        for(n = 0; n < NPAR; n++) {
            //for (i = MAXDEG-1; i > 0; i--) D[i] = D[i-1];
            i = MAXDEG-1; do{ D[i] = D[i-1]; }while( --i );
            D[0] = 0;
    
            //d = compute_discrepancy(psi, synBytes, L, n);
            d = 0; for (i = 0; i <= L; i++) d ^= gmult(psi[i], synBytes[n-i]);
            //d = 0; i = L+1; do{ i--; d ^= gmult(psi[i], synBytes[n-i]); }while(i);
            if( d==0 ) continue;
            // psi2 = psi - d*D 
            //for (i = 0; i < MAXDEG; i++) psi2[i] = psi[i] ^ gmult(d, D[i]);
            i = MAXDEG; do{ i--;  psi2[i] = psi[i] ^ gmult(d, D[i]); }while( i );
            L2 = n-k;
            if( L < L2 ) {
                k = n-L;
                // D = scale_poly(ginv(d), psi); 
                for (i = 0; i < MAXDEG; i++) D[i] = gmult(psi[i], ginv(d));
                //for ( ; i < MAXDEG; i++) D[i] = gmult(psi[i], ginv(d)); // i is already 0 ...
                //i = MAXDEG; do{ i--; D[i] = gmult(psi[i], ginv(d)); }while( i );
                L = L2;
            }
            // psi = psi2 
            copy_poly( psi, psi2 );
        }
        /*
        mult_polys( product, psi, synBytes);	
        zero_poly(  Omega );  
        mymemcpy(   Omega, product, NPAR );
        */
        mult_polys( tp, psi, synBytes);	
        zero_poly(  psi2 );  
        mymemcpy(   psi2, tp, NPAR );
      
        /* Finds all the roots of an error-locator polynomial with coefficients
         * Lambda[j] by evaluating Lambda at successive values of alpha. 
         * 
         * This can be tested with the decoder's equations case.
         */
        NErrors = 0;
        //for(r = 1; r > 0 ; r++) {
        r = 0; do{ r++; 
            sum = 0;
            /// evaluate lambda at r 
            n = NPAR+1; do{ n--; sum ^= gmult( gexp[ ( n * r ) % 255 ], psi[ n ]); }while( n );
            if (sum == 0) ErrorLocs[NErrors++] = (255-r);
            if( NErrors > NPAR/2 ) return NErrors;
        }while( r<255 );
        
        if( NErrors ) { 
            // first check for illegal error locs 
            r = NErrors; do{ r--;
            //for (r = 0; r < NErrors; r++) {
                static BYTE num, denom;
                if (ErrorLocs[r] > (ML-1)) return (NErrors = NPAR/2+1);
                n = ErrorLocs[r];
                // evaluate Omega at alpha^(-i) 
                num = 0;
                //for (j = 0; j < MAXDEG; j++)  num ^= gmult( Omega[j], gexp[ ((255-i)*j) % 255 ]);
                j = MAXDEG; do{ j--; num ^= gmult( psi2[j], gexp[ ((255-n)*j) % 255 ]); }while( j );
                // evaluate Lambda' (derivative) at alpha^(-i) ; all odd powers disappear 
                denom = 0;
                for (j = 1; j < MAXDEG; j += 2) denom ^= gmult( psi[j], gexp[ ((255-n)*(j-1)) % 255 ]);
                err = gmult(num, ginv(denom));
                data[(ML-1)-n] ^= err;
            //}
            }while(r);
        }//else return 0;
    }
    return NErrors;
}//}}}

/* Simulate a LFSR with generator polynomial for n byte RS code. 
 * Pass in a pointer to the data array, and amount of data. 
 *
 * The parity bytes are deposited into pBytes[], and the whole message
 * and parity are copied to dest to make a codeword.
 * 
 */
void encode_data (unsigned char msg[]){//{{{
    /*static BYTE LFSR[ML]; */BYTE i, j, dbyte;
    zero_poly( tmp1 );
    for( i = 0; i < ML-NPAR; i++) {
        dbyte = msg[i] ^ tmp1[NPAR-1];
        for( j = NPAR-1; j > 0; j-- ) tmp1[j] = tmp1[j-1] ^ gmult( genPoly[j], dbyte );
        tmp1[0] = gmult( genPoly[0], dbyte );
    }
    for (i = 0; i < NPAR; i++) msg[i+(ML-NPAR)] = tmp1[(NPAR-1)-i];    
}//}}}
