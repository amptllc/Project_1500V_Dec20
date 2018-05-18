#define NPAR 6
#define DL   28
#define ML   PACKET_LENGTH
#define MAXDEG 12

void init_exp_table (void);
BYTE ginv( BYTE elt);
BYTE gmult(BYTE a, BYTE b);
/* polynomial arithmetic */
void add_polys(BYTE *dst, BYTE *src);
void scale_poly(BYTE k, BYTE *poly);
void mult_polys(BYTE *dst, BYTE *p1, BYTE *p2);
void zerofill( BYTE *a, BYTE l);

//#define copy_poly(dst, src)             mymemcpy( dst, src, MAXDEG ) 
//#define zero_poly(poly)                 zerofill( poly, MAXDEG )

//static void mul_z_poly (BYTE src[]);
//BYTE compute_discrepancy (BYTE lambda[], BYTE S[], BYTE L, BYTE n);
/* Finds all the roots of an error-locator polynomial with coefficients
 * Lambda[j] by evaluating Lambda at successive values of alpha. 
 * 
 * This can be tested with the decoder's equations case.
 */
//void Find_Roots (void);
/* given Psi (called Lambda in Modified_Berlekamp_Massey) and synBytes,
   compute the combined erasure/error evaluator polynomial as 
   Psi*S mod z^4
  */
//void compute_modified_omega ();
/* gamma = product (1-z*a^Ij) for erasure locs Ij */
//void init_gamma (BYTE gamma[]);
/* From  Cain, Clark, "Error-Correction Coding For Digital Communications", pp. 216. */
//void Modified_Berlekamp_Massey (void);
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
BYTE correct_errors (BYTE codeword[]);
/* CRC-CCITT checksum generator */
/* models crc hardware (minor variation on polynomial division algorithm) */
UINT16 crchware(UINT16 data, UINT16 genpoly, UINT16 accum);
/* Computes the CRC-CCITT checksum on array of byte data, length len */
UINT16 crc_ccitt(unsigned char *msg, BYTE len );
/* Reed Solomon encode/decode routines */
void initialize_ecc(void);
/* Append the parity bytes onto the end of the message */
void build_codeword (unsigned char msg[]);
BYTE decode_data (unsigned char *data);
/* Check if the syndrome is zero */
BYTE check_syndrome (void);
/* Simulate a LFSR with generator polynomial for n byte RS code. 
 * Pass in a pointer to the data array, and amount of data. 
 *
 * The parity bytes are deposited into pBytes[], and the whole message
 * and parity are copied to dest to make a codeword.
 * 
 */
void encode_data(unsigned char *msg);
extern BYTE NErrors;
