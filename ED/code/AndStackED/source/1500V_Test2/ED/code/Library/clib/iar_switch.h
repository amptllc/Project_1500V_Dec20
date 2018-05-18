;-----------------------------------------------------------------------------
;
;	File:		iar_switch.h
;
;	Description:	Common macros for iar_switch.s51
;
;       REVISON INFORMATION
;
;       $Revision: 1.6 $
;
;       Log information is available at the end of this file
;
;-----------------------------------------------------------------------------

US_GT_C MACRO nr
	CLR	C
	CLR	A
	MOVC	A,@A+DPTR
	SUBB	A,@R0
	INC	R0
	MOV	A,#1
	MOVC	A,@A+DPTR
	SUBB	A,@R0
        DEC     R0
        ENDM


S_EQ_C MACRO nr
	CLR	A
	MOVC	A,@A+DPTR
	XRL	A,@R0
	JNZ	noz
	INC	R0
	MOV	A,#1
	MOVC	A,@A+DPTR
	XRL	A,@R0
        DEC     R0
	JNZ	noz
noz:
        ENDM

