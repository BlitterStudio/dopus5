/*

Directory Opus 5
Original APL release version 5.82
Copyright 1993-2012 Jonathan Potter & GP Software

This program is free software; you can redistribute it and/or
modify it under the terms of the AROS Public License version 1.1.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
AROS Public License for more details.

The release of Directory Opus 5 under the GPL in NO WAY affects
the existing commercial status of Directory Opus for Windows.

For more information on Directory Opus for Windows please see:

				 http://www.gpsoft.com.au

	functions.c - Conversion of 68k assem functions to C functions.
*/

#include "amiga.h"

static int rand0;
static int rand1;

/*
	Seed() - literal conversion of 68k assem functions to C functions.
	variable numbers correspond to register numbers (e.g. var0 = d0 etc.).
*/
//#warning replace Seed on L_Seed (was done on asm)
// void Seed(int seed)
void LIBFUNC L_Seed(REG(d0, int seed))
{
	ULONG var0 = seed;	 // move.l d0, d1
	ULONG var1 = ~var0;	 // not.l  d1
	ULONG var2, var3, carry = 0;
	ULONG templow, temphigh;
	unsigned long long temp;

	var0 = var0 & 0xFFFFFF0E;  // andi.b  #$0e,d0
	var0 = var0 | 0x00000020;  // ori.b   #$20,d0
	var2 = var0;			   // move.l  d0,d2
	var3 = var1;			   // move.l  d1,d3
	temp = (long long)var2 + (long long)var2;
	var2 = var2 + var2;	 // add.l   d2,d2
	if (temp != (long long)var2)
		carry = 1;				 // carry = overflow
	var3 = var3 + var3 + carry;	 // addx.l  d3,d3
	carry = 0;					 // clear carry
	temp = (long long)var0 + (long long)var2;
	var0 = var0 + var2;	 // add.l   d2,d0
	if (temp != (long long)var0)
		carry = 1;				 // carry = overflow
	var1 = var1 + var3 + carry;	 // addx.l  d3,d1
	carry = 0;					 // clear carry
	templow = var3 >> 16;		 // swap    d3
	temphigh = var3 << 16;
	var3 = templow + temphigh;
	templow = var2 >> 16;  // swap    d2
	temphigh = var2 << 16;
	var2 = templow + temphigh;
	var3 = (var3 & 0xFFFF0000) + (var2 & 0x0000FFFF);  // move.w  d2,d3
	var2 = var2 & 0xFFFF0000;						   // clr.w   d2
	temp = (long long)var0 + (long long)var2;
	var0 = var0 + var2;	 // add.l   d2,d0
	if (temp != (long long)var0)
		carry = 1;				 // carry = overflow
	var1 = var1 + var3 + carry;	 // addx.l  d3,d1
	rand0 = var0;				 // movem.l d0,d1 rand
	rand1 = var1;
	var0 = var1;  // move.l  d1,d0

	return;	 // rts
}

/*
	Random() - literal conversion of 68k assem functions to C functions.
	variable numbers correspond to register numbers (e.g. var0 = d0 etc.).
*/
//#warning replace Random on L_Random (was done on asm)
// UWORD Random(int limit)
UWORD LIBFUNC L_RandomDopus(REG(d0, int limit))
{
	ULONG var0, var1, var2, var3, preserved_var2;
	ULONG templow, temphigh, carry = 0;
	unsigned long long temp;

	var2 = limit & 0x0000FFFF;	// move.w  d0,d2
	if (var2 == 0)
		return limit;		// beq.s   skip
	preserved_var2 = var2;	// movem.l d2-d3,-(sp)

	var0 = rand0;
	var1 = rand1;
	var0 = var0 & 0xFFFFFF0E;  // andi.b  #$0e,d0
	var0 = var0 | 0x00000020;  // ori.b   #$20,d0
	var2 = var0;			   // move.l  d0,d2
	var3 = var1;			   // move.l  d1,d3
	temp = (long long)var2 + (long long)var2;
	var2 = var2 + var2;	 // add.l   d2,d2
	if (temp != (long long)var2)
		carry = 1;				 // carry = overflow
	var3 = var3 + var3 + carry;	 // addx.l  d3,d3
	carry = 0;					 // clear carry
	temp = (long long)var0 + (long long)var2;
	var0 = var0 + var2;	 // add.l   d2,d0
	if (temp != (long long)var0)
		carry = 1;				 // carry = overflow
	var1 = var1 + var3 + carry;	 // addx.l  d3,d1
	carry = 0;					 // clear carry
	templow = var3 >> 16;		 // swap    d3
	temphigh = var3 << 16;
	var3 = templow + temphigh;
	templow = var2 >> 16;  // swap    d2
	temphigh = var2 << 16;
	var2 = templow + temphigh;
	var3 = (var3 & 0xFFFF0000) + (var2 & 0x0000FFFF);  // move.w  d2,d3
	var2 = var2 & 0xFFFF0000;						   // clr.w   d2
	temp = (long long)var0 + (long long)var2;
	var0 = var0 + var2;	 // add.l   d2,d0
	if (temp != (long long)var0)
		carry = 1;				 // carry = overflow
	var1 = var1 + var3 + carry;	 // addx.l  d3,d1
	rand0 = var0;				 // movem.l d0,d1 rand
	rand1 = var1;
	var0 = var1;			// move.l  d1,d0
	var2 = preserved_var2;	// movem.l (sp)+,d2-d3

	var0 = var0 & 0xFFFF0000;  // clr.w   d0
	templow = var0 >> 16;	   // swap    d0
	temphigh = var0 << 16;
	var0 = templow + temphigh;
	var0 = var0 % var2;	 // divu.w  d2,d0
						 // clr.w   d0
						 // swap    d0
	return (UWORD)var0;	 // rts
}

void LIBFUNC L_BtoCStr(REG(a0, BSTR bstr), REG(a1, char *cstr), REG(d0, int len))
{
	int length = len;
	int size;
#ifdef __AROS__
	char *string = AROS_BSTR_ADDR(bstr);
#else
	char *string = (char *)(bstr << 2);
#endif

	length--;
#ifdef __AROS__
	size = AROS_BSTR_strlen(bstr);
#else
	size = *((UBYTE *)string);
	string++;
#endif
	if (size < len)
		length = size;
	for (size = 0; size < length; size++)
		*(cstr++) = *(string++);
	*cstr = 0;
}

ULONG LIBFUNC L_DivideU(REG(d0, ULONG num),
						REG(d1, ULONG div),
						REG(a0, ULONG *rem),
						REG(a1, struct Library *lib))
{
	ULONG quotient;

#if !defined(__amigaos4__)
	quotient = UDivMod32(num, div);
#else
	quotient = (ULONG)(num / div);
#endif
	*rem = num - (div * quotient);
	return quotient;
}
