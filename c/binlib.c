/* 
	This Program is written and owned by Davide Achilli and is released
	under the GPL v.3 license agreement (see gpl-3.0.txt).
	
	IN NO EVENT SHALL THE AUTHOR BE LIABLE TO ANY PARTY FOR DIRECT, 
	INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING 
	OUT OF THE USE OF TH_IS_SOFTWARE, ITS DOCUMENTATION, OR ANY
	DERIVATIVES THEREOF, EVEN IF THE AUTHOR HAS BEEN ADVISED OF THE
	POSSIBILITY OF SUCH DAMAGE.
	
	THE AUTHOR SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING, BUT 
	NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  
	TH_IS_SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, AND THE AUTHOR 
	HAS NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, 
	ENHANCEMENTS, OR MODIFICATIONS.
*******************************************************************************/
#define _CRT_SECURE_NO_WARNINGS
#include "binlib.h"
#include <assert.h>


#define POS(bit) (1 << (7-(bit & 7)))
#define SRC (*(((const unsigned char*) source)+(sourceOffset >> 3)))
#define DST (*(((unsigned char*)destination)+(destOffset >> 3)))

/*------------------------------------------------------------------------------
	This function copies 'bitLen' less significant bits of 'source' char buffer
	from offset 'sourceOffset'. Read t_numeric is returned.
------------------------------------------------------------------------------*/
t_numeric bitsToNumeric (const void* source, unsigned sourceOffset, unsigned bitLen)
{
	unsigned i;
	t_numeric ret=0;
	assert (bitLen <= 8);
	for (i=0; i<bitLen; i++) {
		if ((SRC & POS(sourceOffset)) != 0) ret |= (1 << (bitLen-i-1));
		sourceOffset++;
	}
	return ret;
}

/*------------------------------------------------------------------------------
	This function copies 'bitLen' less significant bits of 'source' t_numeric to
	'destination' ad offset 'destOffset'.
------------------------------------------------------------------------------*/
void numericToBits (void* destination, unsigned destOffset, t_numeric source, unsigned bitLen)
{
	int i;
	assert (bitLen <= 8);
	for (i=bitLen-1; i>=0; i--) {
		if ((source & (1 << i)) != 0) DST |= POS(destOffset);
		else DST &= ~POS(destOffset);
		destOffset++;
	}
}

/*------------------------------------------------------------------------------
	Reads a little-endian value of 'bitLen' number of bits and returns it.
	The 'sourceOffset', in bits, must be octet aligned; 'bitLen' must be an exact
	number of octets.S
------------------------------------------------------------------------------*/
t_numeric binaryToNumeric (const void* source, unsigned sourceOffset, unsigned bitLen)
{
	unsigned i;
	t_numeric ret = 0;
	assert ((sourceOffset & 7) == 0);
	assert ((bitLen & 7) == 0);
	assert (bitLen <= 32);

	bitLen >>= 3;
	sourceOffset >>= 3;
	for (i=bitLen; i>0; i--) {
		ret <<= 8;
		ret |= (t_numeric)(((unsigned char*)source)[i+sourceOffset-1]);
	}
	return ret;
}

/*------------------------------------------------------------------------------
	Writes a little-endian value of 'bitLen' number of bits at 'destination'.
	The 'destOffset', in bits, must be octet aligned; 'bitLen' must be an exact
	number of octets.S
------------------------------------------------------------------------------*/
void numericToBinary (void* destination, unsigned destOffset, t_numeric source, unsigned bitLen)
{
	unsigned i;

	assert ((destOffset & 7) == 0);
	assert ((bitLen & 7) == 0);
	assert (bitLen <= 32);

	/* Convert length and offset in octets */
	bitLen >>= 3;
	destOffset >>= 3;

	for (i=0; i<bitLen; i++) {
		(((unsigned char*)destination)[i+destOffset]) = (unsigned char)(source & 0xFF);
		source >>= 8;
	}
}

/*------------------------------------------------------------------------------
	Writes a BCD value of 'bitLen' number of bits at 'destination'.
	The 'destOffset', in bits, must be octet aligned; 'bitLen' must be an exact
	number of octets.

	Example: BCD x67 x45 x23 x41 means 41234567

	The BCD values support "invalid BCDs". They are coded as xFF xFF xFF xFF
	and saved in the numeric as INVALID_BCD.
------------------------------------------------------------------------------*/
t_numeric BCDToNumeric (const void* source, unsigned sourceOffset, unsigned bitLen)
{
	unsigned i;
	t_numeric ret = 0;
	t_numeric nUni, nDec;
	int isInvalid = 0;

	assert ((sourceOffset & 7) == 0);
	assert ((bitLen & 7) == 0);
	assert (bitLen <= 32);

	/* Convert length and offset in octets */
	bitLen >>= 3;
	sourceOffset >>= 3;

	for (i=bitLen; i>0; i--) {
		ret *= 100;
		nUni = (t_numeric)(((const unsigned char*)source)[i+sourceOffset-1]);
		nDec = (nUni >> 4);
		nUni = nUni & 0x0F;

		if (nUni > 9) {nUni=9; isInvalid=1;}
		if (nDec > 9) {nDec=9; isInvalid=1;}
		ret += nDec*10 + nUni;
	}
	
	if (isInvalid) ret = INVALID_BCD;

	return ret;
}

/*------------------------------------------------------------------------------
	CTCSS/DCS tones are coded in a custom way. They are coded as BCD, but with
	an exception: the two most significant bits of the second octet are to be 
	extracted and used as follow: 00=CTCSS, 10=DCS-N, 11=DCS-I.
	Once removed, the remaining BCD value is the number.
	For example, D023I is coded as: "x23 xC0". Due to this exception, the maximum
	ABCD number is limited because "A" can use only two bits instead of four.
	The maximum number is therefore 3999, althoug the highest is actually
	2541, used for CTCTSS 254.1.

	The tones format is coded within this program as follows:

	0xK0000 | 0xNNNN

	where K is: 00=CTCSS 10=DCS-N 11=DCS-I and N is the tone value as coded in
	BCD (for example, tone 127.3 is 1273 (0x04F9).
------------------------------------------------------------------------------*/
t_numeric BCDToNumericForTones (const void* source, unsigned sourceOffset)
{
	unsigned char ch[2];
	t_numeric ret;
	t_numeric ret2;
	sourceOffset >>= 3;

	ch[0] = ((const unsigned char*)source)[sourceOffset];
	ch[1] = ((const unsigned char*)source)[sourceOffset+1];

	ret = (((t_numeric)ch[1]) & 0xC0) << 10;
	ch[1] &= ~0xC0;

	ret2 = BCDToNumeric (ch, 0, 16);
	if (ret2 == INVALID_BCD) return INVALID_BCD;

	return ret | ret2;
}

/*------------------------------------------------------------------------------
	Same as "numericToBCD" but with the rules of "BCDToNumericForTones"
------------------------------------------------------------------------------*/
void numericToBCDForTones (void* destination, unsigned destOffset, t_numeric source)
{
	unsigned char toneType = (unsigned char)((source & 0x30000) >> 10);
	source &= 0xFFFF;
	numericToBCD (destination, destOffset, source, 16);

	destOffset >>= 3;

	(((unsigned char*)destination)[destOffset+1]) |= toneType;
}


/*------------------------------------------------------------------------------
	Writes a BCD value of 'bitLen' number of bits at 'destination'.
	The 'destOffset', in bits, must be octet aligned; 'bitLen' must be an exact
	number of octets.
	
	Example: BCD x67 x45 x23 x41 means 41234567

	The BCD values support "invalid BCDs". They are coded as xFF xFF xFF xFF
	and saved in the numeric as INVALID_BCD.
------------------------------------------------------------------------------*/
void numericToBCD (void* destination, unsigned destOffset, t_numeric source, unsigned bitLen)
{
	unsigned i;
	t_numeric nUni, nDec;

	assert ((destOffset & 7) == 0);
	assert ((bitLen & 7) == 0);
	assert (bitLen <= 32);

	/* Convert length and offset in octets */
	bitLen >>= 3;
	destOffset >>= 3;

	if (source == INVALID_BCD) {
		for (i=0; i<bitLen; i++) {
			(((unsigned char*)destination)[i+destOffset]) = 0xFF;
		}
	}
	else {
		for (i=0; i<bitLen; i++) {
			nUni = source % 10;
			source /= 10;
			nDec = source % 10;
			source /= 10;
			(((unsigned char*)destination)[i+destOffset]) = (unsigned char)((nDec << 4) | nUni);
		}
	}
}

/*------------------------------------------------------------------------------
	Writes a BCD value of 'bitLen' number of bits at 'destination'.
	The 'destOffset', in bits, must be octet aligned; 'bitLen' must be an exact
	number of octets.

	Example: BCD x67 x45 x23 x41 means 41234567

	The BCD values support "invalid BCDs". They are coded as xFF xFF xFF xFF
	and saved in the numeric as INVALID_BCD.
------------------------------------------------------------------------------*/
t_numeric RevBCDToNumeric (const void* source, unsigned sourceOffset, unsigned bitLen)
{
	unsigned i;
	t_numeric ret = 0;
	t_numeric nUni, nDec;
	int isInvalid = 0;

	assert ((sourceOffset & 7) == 0);
	assert ((bitLen & 7) == 0);
	assert (bitLen <= 32);

	/* Convert length and offset in octets */
	bitLen >>= 3;
	sourceOffset >>= 3;

	for (i=0; i<bitLen; i++) {
		ret *= 100;
		nUni = (t_numeric)(((const unsigned char*)source)[sourceOffset+i]);
		nDec = (nUni >> 4);
		nUni = nUni & 0x0F;

		if (nUni > 9) {nUni=9; isInvalid=1;}
		if (nDec > 9) {nDec=9; isInvalid=1;}
		ret += nDec*10 + nUni;
	}
	
	if (isInvalid) ret = INVALID_BCD;

	return ret;
}

/*------------------------------------------------------------------------------
	Writes a BCD value of 'bitLen' number of bits at 'destination'.
	The 'destOffset', in bits, must be octet aligned; 'bitLen' must be an exact
	number of octets.
	
	Example: BCD x67 x45 x23 x41 means 41234567

	The BCD values support "invalid BCDs". They are coded as xFF xFF xFF xFF
	and saved in the numeric as INVALID_BCD.
------------------------------------------------------------------------------*/
void numericToRevBCD (void* destination, unsigned destOffset, t_numeric source, unsigned bitLen)
{
	unsigned i;
	t_numeric nUni, nDec;

	assert ((destOffset & 7) == 0);
	assert ((bitLen & 7) == 0);
	assert (bitLen <= 32);

	/* Convert length and offset in octets */
	bitLen >>= 3;
	destOffset >>= 3;

	if (source == INVALID_BCD) {
		for (i=0; i<bitLen; i++) {
			(((unsigned char*)destination)[(bitLen-1-i)+destOffset]) = 0xFF;
		}
	}
	else {
		for (i=0; i<bitLen; i++) {
			nUni = source % 10;
			source /= 10;
			nDec = source % 10;
			source /= 10;
			(((unsigned char*)destination)[(bitLen-1-i)+destOffset]) = (unsigned char)((nDec << 4) | nUni);
		}
	}
}

/*------------------------------------------------------------------------------
	Reads from binary mode an unicode string
------------------------------------------------------------------------------*/
void binaryToUnicode (const void* source, unsigned sourceOffset, t_unicode* target, unsigned bitLen)
{
	unsigned i;

	assert ((sourceOffset & 7) == 0);
	assert ((bitLen & 15) == 0);

	bitLen >>= 4;

	for (i=0; i<bitLen; i++) {
		target[i] = (t_unicode)binaryToNumeric (source, sourceOffset+16*i, 16);
	}
}

/*------------------------------------------------------------------------------
	Writes in binary mode an unicode string
------------------------------------------------------------------------------*/
void unicodeToBinary (void* destination, unsigned destOffset, const t_unicode* source, unsigned bitLen)
{
	unsigned i;

	assert ((destOffset & 7) == 0);
	assert ((bitLen & 15) == 0);

	bitLen >>= 4;

	for (i=0; i<bitLen; i++) {
		numericToBinary (destination, destOffset+16*i, source[i], 16);
	}
}

/*------------------------------------------------------------------------------
	Reads from binary mode an ascii string
------------------------------------------------------------------------------*/
void asciiBinaryToUnicode (const void* source, unsigned sourceOffset, t_unicode* target, unsigned bitLen)
{
	unsigned i;
	int isEmpty = 1;

	assert ((sourceOffset & 7) == 0);
	assert ((bitLen & 7) == 0);

	bitLen >>= 3;
	sourceOffset >>= 3;

	for (i=0; i<bitLen; i++) {
		if (((const unsigned char*)source) [i+sourceOffset] != 0xFF) {isEmpty = 0; break;}
	}
	
	if (isEmpty) {
		for (i=0; i<bitLen; i++) {
			target[i] = 0;
		}
	}
	else {
		for (i=0; i<bitLen; i++) {
			target[i] = (t_unicode)((const unsigned char*)source) [i+sourceOffset];
		}
	}
}

/*------------------------------------------------------------------------------
	Writes in binary mode an ascii string
------------------------------------------------------------------------------*/
void unicodeToAsciiBinary (void* destination, unsigned destOffset, const t_unicode* source, unsigned bitLen)
{
	unsigned i;

	assert ((destOffset & 7) == 0);
	assert ((bitLen & 7) == 0);

	bitLen >>= 3;
	destOffset >>= 3;

	if (source[0] == 0) {
		/* Encode empty strings as 0xFF */
		for (i=0; i<bitLen; i++) {
			((unsigned char*)destination) [i+destOffset] = 0xFF;;
		}
	}
	else {
		for (i=0; i<bitLen; i++) {
			unsigned char c = (unsigned char)source[i];
			
			/* Convert to lower case */
			if (c >= 'A' && c <= 'Z') {c = c - 'A' + 'a';}
			
			((unsigned char*)destination) [i+destOffset] = c;
		}
	}
}


#ifndef NDEBUG

void testLittleEndian ()
{
	unsigned char N[] = {0xFF, 0xFF, 0x78, 0x56, 0x34, 0x12, 0xFF, 0xFF};
	unsigned char M[8];
	int i;
	t_numeric num;
	for (i=0; i<sizeof(M); i++) M[i]=0xFF;

	num = binaryToNumeric (N, 16, 32);
	assert (num == 0x12345678);

	numericToBinary (M, 16, num, 32);
	for (i=0; i<sizeof(M); i++) {
		assert (M[i] == N[i]);
	}
}

void testBCD ()
{
	unsigned char N[] = {0xFF, 0xFF, 0x78, 0x56, 0x34, 0x12, 0xFF, 0xFF};
	unsigned char M[8];
	int i;
	t_numeric num;
	for (i=0; i<sizeof(M); i++) M[i]=0xFF;

	num = BCDToNumeric (N, 16, 32);
	assert (num == 12345678);

	numericToBCD (M, 16, num, 32);
	for (i=0; i<sizeof(M); i++) {
		assert (M[i] == N[i]);
	}
}

void testRevBCD ()
{
	unsigned char N[] = {0xFF, 0xFF, 0x12, 0x34, 0x56, 0x78, 0xFF, 0xFF};
	unsigned char M[8];
	int i;
	t_numeric num;
	for (i=0; i<sizeof(M); i++) M[i]=0xFF;

	num = RevBCDToNumeric (N, 16, 32);
	assert (num == 12345678);

	numericToRevBCD (M, 16, num, 32);
	for (i=0; i<sizeof(M); i++) {
		assert (M[i] == N[i]);
	}
}

void testUnicode ()
{
	unsigned char N[] = {0xFF, 0xFF, 0x41, 0x00, 0x34, 0x12, 0xFF, 0xFF};
	t_unicode txt [2];
	unsigned char M[8];
	int i;
	for (i=0; i<sizeof(M); i++) M[i]=0xFF;
	
	binaryToUnicode (N, 16, txt, 32);

	unicodeToBinary (M, 16, txt, 32);

	for (i=0; i<sizeof(M); i++) {
		assert (M[i] == N[i]);
	}
}

void runBinlibTest ()
{
	testLittleEndian ();
	testBCD ();
	testRevBCD ();
	testUnicode ();
}

#endif

#ifdef ED_DEBUG_ALLOC
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
size_t ED_MemAllocated = 0;
size_t ED_MaxMemAllocated = 0;
unsigned ED_AllocationId = 0;

#define ED_DEBUG_MEM_HEADER (sizeof (size_t)+sizeof (unsigned))

typedef struct SEDMemHeader {
	unsigned Signature;
	size_t Size;
	unsigned AllocationId;

	/* Pointer to the next header */
	struct SEDMemHeader* NextHeader;

	/* Pointer to the pointer pointing to this in the previous header */
	struct SEDMemHeader** PrevHeaderPointer;
	
} TEDMemHeader;

typedef struct SEDMemFooter {
	unsigned Signature;
	size_t Size;
	unsigned AllocationId;
} TEDMemFooter;

TEDMemHeader* ED_MemoryRoot = NULL;

#define ED_SIGNATURE_HEADER 0x12345678
#define ED_SIGNATURE_FOOTER 0xADACADAB

void EDDebugCheckMemory ()
{
	const TEDMemHeader* hdr;
	
	for (hdr = ED_MemoryRoot; hdr != NULL; hdr = hdr->NextHeader) {
		TEDMemFooter* footer =(TEDMemFooter*)(((char*)hdr)+(sizeof (TEDMemHeader)+hdr->Size));
		
		assert (hdr->Signature == ED_SIGNATURE_HEADER);
		assert (footer->Signature == ED_SIGNATURE_FOOTER);
		assert (hdr->Size == footer->Size);
		assert (hdr->AllocationId == footer->AllocationId);
		assert (*(hdr->PrevHeaderPointer) == hdr);
	}
}
#define ED_TRACE_ALLOCS
void* EDDebugAlloc (size_t size)
{
	TEDMemHeader* ret;
	TEDMemFooter* footer;
	assert (size > 0);
	ED_MemAllocated += size;
	if (ED_MemAllocated > ED_MaxMemAllocated) ED_MaxMemAllocated = ED_MemAllocated;
	ret = (TEDMemHeader*)malloc (sizeof (TEDMemHeader)+size+sizeof (TEDMemFooter));
	footer = (TEDMemFooter*)(((char*)ret)+(sizeof (TEDMemHeader)+size));
	ret->Size = size;
	ret->Signature = ED_SIGNATURE_HEADER;
	ret->AllocationId = ED_AllocationId;
	footer->Size = size;
	footer->Signature = ED_SIGNATURE_FOOTER;
	footer->AllocationId = ED_AllocationId;
	memset (ret+1, 0xAB, size);
	ED_AllocationId++;
	
	/* Setup the linked list */
	ret->NextHeader = ED_MemoryRoot;
	ret->PrevHeaderPointer = &ED_MemoryRoot;
	ED_MemoryRoot = ret;
	if (ret->NextHeader) {
		ret->NextHeader->PrevHeaderPointer = &ret->NextHeader;
	}
	
	if (ret->AllocationId == 0) {
		printf ("");
	}
	
#ifdef ED_TRACE_ALLOCS
	{
		static int firstTime = 1;
		FILE* f=fopen ("memory.log", (firstTime?"w":"a"));
		firstTime = 0;
		fprintf (f, "ALLC [ID:%08X] %08X %6d\n", ret->AllocationId, (unsigned)(ret+1), (int)size);
		fclose (f);
		if (ret->AllocationId == 0x00000010) {
			printf ("");
		}
	}
#endif
	EDDebugCheckMemory ();
	return (ret+1);
}

void EDDebugFree (void* ptr)
{
	TEDMemHeader* sptr = ((TEDMemHeader*)ptr)-1;
	TEDMemFooter* footer =(TEDMemFooter*)(((char*)sptr)+(sizeof (TEDMemHeader)+sptr->Size));

	EDDebugCheckMemory ();
	
	assert (sptr->Signature == ED_SIGNATURE_HEADER);
	assert (footer->Signature == ED_SIGNATURE_FOOTER);
	assert (sptr->Size == footer->Size);
	assert (sptr->AllocationId == footer->AllocationId);
	memset (ptr, 0xCD, sptr->Size);
	
	(*sptr->PrevHeaderPointer) = sptr->NextHeader;
	if (sptr->NextHeader) {
		sptr->NextHeader->PrevHeaderPointer = sptr->PrevHeaderPointer;
	}
	
	ED_MemAllocated -= sptr->Size;
#ifdef ED_TRACE_ALLOCS
	{
		FILE* f=fopen ("memory.log", "a");
		if (sptr->AllocationId == 0x00000010) {
			printf ("");
		}
		fprintf (f, "FREE [ID:%08X] %08X %6d\n", sptr->AllocationId, (unsigned)ptr, (unsigned)sptr->Size);
		fclose (f);
	}
#endif
	EDDebugCheckMemory ();
	free (sptr);
}

int EDDebugPrintSummary ()
{
	printf ("Memory leaked: %d bytes  Max ED_MemAllocated: %d bytes\n", ED_MemAllocated, ED_MaxMemAllocated);
	return (ED_MemAllocated != 0);
}
#endif
