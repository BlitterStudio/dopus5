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

*/
#ifndef _READ_ILBM_H
#define _READ_ILBM_H

#ifndef __amigaos3__
	#pragma pack(2)
#endif

/*  IFF types we may encounter  */
#ifndef ID_ILBM
#define ID_ILBM MAKE_ID('I', 'L', 'B', 'M')
#endif

/* ILBM Chunk ID's we may encounter
 * (see iffp/iff.h for some other generic chunks)
 */
#ifndef ID_BMHD
#define ID_BMHD MAKE_ID('B', 'M', 'H', 'D')
#endif
#ifndef ID_CMAP
#define ID_CMAP MAKE_ID('C', 'M', 'A', 'P')
#endif
#ifndef ID_CRNG
#define ID_CRNG MAKE_ID('C', 'R', 'N', 'G')
#endif
#define ID_CCRT MAKE_ID('C', 'C', 'R', 'T')
#ifndef ID_GRAB
#define ID_GRAB MAKE_ID('G', 'R', 'A', 'B')
#endif
#ifndef ID_SPRT
#define ID_SPRT MAKE_ID('S', 'P', 'R', 'T')
#endif
#ifndef ID_DEST
#define ID_DEST MAKE_ID('D', 'E', 'S', 'T')
#endif
#ifndef ID_CAMG
#define ID_CAMG MAKE_ID('C', 'A', 'M', 'G')
#endif

/* Use this constant instead of sizeof(ColorRegister).
 * ONLY VALID FOR size of color register AS STORED in ILBM.CMAP
 */
#define sizeofColorRegister 3

typedef WORD Color4; /* Amiga RAM version of a color-register,
					  * with 4 bits each RGB in low 12 bits.*/

typedef struct
{
	ULONG r;
	ULONG g;
	ULONG b;
} Color32;

/* Maximum number of bitplanes storable in BitMap structure */
#define MAXAMDEPTH 8

/* Use ViewPort->ColorMap.Count instead
#define MAXAMCOLORREG 32
*/

/* Maximum planes this code can save */
#define MAXSAVEDEPTH 24

/* Convert image width to even number of BytesPerRow for ILBM save.
 * Do NOT use this macro to determine the actual number of bytes per row
 * in an Amiga BitMap.  Use BitMap->BytesPerRow for scan-line modulo.
 * Use your screen or viewport width to determine width. Or under
 * V39, use GetBitMapAttr().
 */
#define RowBytes(w) ((((w) + 15) >> 4) << 1)
#define RowBits(w) ((((w) + 15) >> 4) << 4)

/* Flags that should be masked out of old 16-bit CAMG before save or use.
 * Note that 32-bit mode id (non-zero high word) bits should not be twiddled
 */
#define BADFLAGS (SPRITES | VP_HIDE | GENLOCK_AUDIO | GENLOCK_VIDEO)
#define OLDCAMGMASK (~BADFLAGS)

/*  Masking techniques  */
#define mskNone 0
#define mskHasMask 1
#define mskHasTransparentColor 2
#define mskLasso 3

/* We plan to define this value, to mean that the BMHD.transparentColor
 * variable contains a count of alpha channel planes which are stored
 * in the BODY AFTER the image planes.  As always, the count of
 * masking/alpha planes is not included in BMHD.nPlanes.
 * If you have any comments or input, contact Chris Ludwig (CATS US).
 */
#define mskHasAlpha 4

/*  Compression techniques  */
#define cmpNone 0
#define cmpByteRun1 1

/* ---------- BitMapHeader ---------------------------------------------*/
/*  Required Bitmap header (BMHD) structure describes an ILBM */
typedef struct
{
	UWORD w, h;		   /* Width, height in pixels */
	WORD x, y;		   /* x, y position for this bitmap  */
	UBYTE nPlanes;	   /* # of planes (not including mask) */
	UBYTE masking;	   /* a masking technique listed above */
	UBYTE compression; /* cmpNone or cmpByteRun1 */
	UBYTE flags;	   /* as defined or approved by Commodore */
	UWORD transparentColor;
	UBYTE xAspect, yAspect;
	WORD pageWidth, pageHeight;
} BitMapHeader;

/* BMHD flags */

/* Advisory that 8 significant bits-per-gun have been stored in CMAP
 * i.e. that the CMAP is definitely not 4-bit values shifted left.
 * This bit will disable nibble examination by color loading routine.
 */
#define BMHDB_CMAPOK 7
#define BMHDF_CMAPOK (1 << BMHDB_CMAPOK)

/* ---------- ColorRegister --------------------------------------------*/
/* A CMAP chunk is a packed array of ColorRegisters (3 bytes each). */
typedef struct
{
	UBYTE red, green, blue; /* MUST be UBYTEs so ">> 4" won't sign extend.*/
} ColorRegister;

/* ---------- Point2D --------------------------------------------------*/
/* A Point2D is stored in a GRAB chunk. */
typedef struct
{
	WORD x, y; /* coordinates (pixels) */
} Point2D;

/* ---------- DestMerge ------------------------------------------------*/
/* A DestMerge is stored in a DEST chunk. */
typedef struct
{
	UBYTE depth;	  /* # bitplanes in the original source */
	UBYTE pad1;		  /* UNUSED; for consistency store 0 here */
	UWORD planePick;  /* how to scatter source bitplanes into destination */
	UWORD planeOnOff; /* default bitplane data for planePick */
	UWORD planeMask;  /* selects which bitplanes to store into */
} DestMerge;

/* ---------- SpritePrecedence -----------------------------------------*/
/* A SpritePrecedence is stored in a SPRT chunk. */
typedef UWORD SpritePrecedence;

/* ---------- Camg Amiga Viewport Mode Display ID ----------------------*/
/* The CAMG chunk is used to store the Amiga display mode in which
 * an ILBM is meant to be displayed.  This is very important, especially
 * for special display modes such as HAM and HALFBRITE where the
 * pixels are interpreted differently.
 * Under V37 and higher, store a 32-bit Amiga DisplayID (aka. ModeID)
 * in the ULONG ViewModes CAMG variable (from GetVPModeID(viewport)).
 * Pre-V37, instead store the 16-bit viewport->Modes.
 * See the current IFF manual for information on screening for bad CAMG
 * chunks when interpreting a CAMG as a 32-bit DisplayID or 16-bit ViewMode.
 * The chunk's content is declared as a ULONG.
 */
typedef struct
{
	ULONG ViewModes;
} CamgChunk;

/* ---------- CRange cycling chunk -------------------------------------*/
#define RNG_NORATE 36 /* Dpaint uses this rate to mean non-active */
/* A CRange is store in a CRNG chunk. */
typedef struct
{
	WORD pad1;		 /* reserved for future use; store 0 here */
	WORD rate;		 /* 60/sec=16384, 30/sec=8192, 1/sec=16384/60=273 */
	WORD active;	 /* bit0 set = active, bit 1 set = reverse */
	UBYTE low, high; /* lower and upper color registers selected */
} CRange;

/* ---------- Ccrt (Graphicraft) cycling chunk -------------------------*/
/* A Ccrt is stored in a CCRT chunk. */
typedef struct
{
	WORD direction;	   /* 0=don't cycle, 1=forward, -1=backwards */
	UBYTE start;	   /* range lower */
	UBYTE end;		   /* range upper */
	LONG seconds;	   /* seconds between cycling */
	LONG microseconds; /* msecs between cycling */
	WORD pad;		   /* future exp - store 0 here */
} CcrtChunk;

/* If you are writing all of your chunks by hand,
 * you can use these macros for these simple chunks.
 */
#define putbmhd(iff, bmHdr) PutCk(iff, ID_BMHD, sizeof(BitMapHeader), (BYTE *)bmHdr)
#define putgrab(iff, point2D) PutCk(iff, ID_GRAB, sizeof(Point2D), (BYTE *)point2D)
#define putdest(iff, destMerge) PutCk(iff, ID_DEST, sizeof(DestMerge), (BYTE *)destMerge)
#define putsprt(iff, spritePrec) PutCk(iff, ID_SPRT, sizeof(SpritePrecedence), (BYTE *)spritePrec)
#define putcamg(iff, camg) PutCk(iff, ID_CAMG, sizeof(CamgChunk), (BYTE *)camg)
#define putcrng(iff, crng) PutCk(iff, ID_CRNG, sizeof(CRange), (BYTE *)crng)
#define putccrt(iff, ccrt) PutCk(iff, ID_CCRT, sizeof(CcrtChunk), (BYTE *)ccrt)

#define ID_DPAN MAKE_ID('D', 'P', 'A', 'N')
#ifndef ID_DLTA
#define ID_DLTA MAKE_ID('D', 'L', 'T', 'A')
#endif

typedef struct
{
	UWORD version;
	UWORD nframes;
	unsigned char framespersecond;
	char pad;
	UWORD flags;
} DPAnimChunk;

typedef struct
{
	UBYTE operation;
	UBYTE mask;
	UWORD w, h;
	WORD x, y;
	ULONG abstime;
	ULONG reltime;
	UBYTE interleave;
	UBYTE pad0;
	ULONG bits;
	UBYTE pad[16];
} AnimHdr;

#define ANIMF_LONG 1
#define ANIMF_XOR 2

typedef struct
{
	struct MinNode node;

	AnimHdr header;		   // ANIM header
	unsigned char *delta;  // DLTA data
} ANIMFrame;

typedef struct
{
	DPAnimChunk header;	 // DeluxePaint anim header

	struct MinList frames;		 // Frame list (for ANIMs)
	ANIMFrame *current;			 // Current frame
	unsigned short frame_count;	 // Number of frames
} ANIMHandle;

typedef struct
{
	APTR memory;  // Memory handle

	BitMapHeader header;  // ILBM header
	ULONG mode_id;		  // Mode ID
	ULONG flags;		  // Flags

	union
	{
		struct
		{
			unsigned char *data;  // BODY data
			unsigned long size;	  // BODY size
		} body;

		struct
		{
			struct BitMap *bitmap;	// Displayable bitmap
		} bitmap;

		struct
		{
			unsigned short **planes;  // Image plane data (not chip memory)
		} planes;
	} image;

	ULONG *palette;	 // Image palette

	ANIMHandle *anim;  // Animation data
} ILBMHandle;

#define ILBMF_GET_BODY (1 << 0)		  // Get BODY
#define ILBMF_GET_BITMAP (1 << 1)	  // Get Bitmap
#define ILBMF_GET_PLANES (1 << 2)	  // Get plane data
#define ILBMF_NO_ANIM (1 << 3)		  // Specifically don't want an animation
#define ILBMF_ANIM_ONE_ONLY (1 << 4)  // Get maximum of one animation frame
#define ILBMF_NO_PALETTE (1 << 5)	  // Don't bother about palette
#define ILBMF_NO_24BIT (1 << 6)		  // Don't support 24 bit

#define ILBMF_GOT_BODY (1 << 0)	   // Got BODY
#define ILBMF_GOT_BITMAP (1 << 1)  // Got Bitmap
#define ILBMF_GOT_PLANES (1 << 2)  // Got plane data
#define ILBMF_IS_ANIM (1 << 3)	   // Is an animation

#define ILBMF_GOT_HEADER (1 << 4)	  // Got header
#define ILBMF_GOT_PALETTE (1 << 5)	  // Got palette
#define ILBMF_GOT_MODE (1 << 6)		  // Got mode ID
#define ILBMF_IS_ANIM_BRUSH (1 << 7)  // Is an anim brush
#define ILBMF_CHIP_PLANES (1 << 8)	  // Planes are in chip

typedef struct
{
	unsigned char *sourceptr;	 // Source data
	unsigned char **destplanes;	 // Destination planes
	unsigned short imagebpr;	 // Bytes-per-row in source
	unsigned short imageheight;	 // Image height
	unsigned short imagedepth;	 // Image depth
	unsigned short destbpr;		 // Bytes-per-row in destination
	unsigned short destheight;	 // Destination height
	unsigned short destdepth;	 // Destination depth
	char masking;				 // Image masking
	char compression;			 // Compression type
	unsigned long offset;		 // Offset in destination
} RLEinfo;

ILBMHandle *ReadILBM(char *name, ULONG flags);
void FreeILBM(ILBMHandle *ilbm);
void DecodeILBM(char *source,
				unsigned short width,
				unsigned short height,
				unsigned short depth,
				struct BitMap *dest,
				unsigned long flags,
				char comp);
void DecodeRLE(RLEinfo *rle);
ILBMHandle *FakeILBM(UWORD *imagedata, ULONG *palette, short width, short height, short depth, ULONG flags);

#define DIF_MASK (1 << 0)
#define DIF_WRITEPIX (1 << 6)

#ifndef __amigaos3__
	#pragma pack()
#endif

#endif
