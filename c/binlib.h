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
#ifndef __BINLIB_H
#define __BINLIB_H
#include <string.h>

#ifdef __cplusplus
	extern "C" {
#endif

#define BL_TEXT_SAMPLE_LEN 17

typedef unsigned char t_buffer;
typedef unsigned short t_unicode;
typedef unsigned long t_numeric;
typedef struct {
	/* Set if resolved */
	unsigned refLineNo;
	
	/* Reference to the key UNICODE string within the T_xxx structure */
	/* NULL if the reference is to a numeric field */
	const t_unicode* resolvedName;

	/* Contains the CRC-32 of the lowercase string if the referenced key is UNICODE */
	/* Otherwise, it contains the value of the key itself if numeric */
	unsigned refId;
	
	/* Sample of the text in the CSV file, used to report errors. Used if key is UNICODE */
	char textSample [BL_TEXT_SAMPLE_LEN+1];
} t_reference;

#define NOWARN_UNUSED(a) ((void)(a))

#define INVALID_BCD     0

#ifndef binAlloc
#ifdef ED_DEBUG_ALLOC
#define binAlloc EDDebugAlloc
#else
#include <stdlib.h>
#define binAlloc malloc
#endif
#endif

#ifndef binFree
#ifdef ED_DEBUG_ALLOC
#define binFree EDDebugFree
#else
#include <stdlib.h>
#define binFree free
#endif
#endif
#ifdef ED_DEBUG_ALLOC
void* EDDebugAlloc (size_t size);
void  EDDebugFree (void* ptr);
int EDDebugPrintSummary ();
void EDDebugCheckMemory();
#endif
typedef enum {
	BL_integer,
	BL_unicode
} BinType;

/*------------------------------------------------------------------------------
	This function copies 'bitLen' less significant bits of 'source' char buffer
	from offset 'sourceOffset'. Read t_numeric is returned.
------------------------------------------------------------------------------*/
extern t_numeric bitsToNumeric (const void* source, unsigned sourceOffset, unsigned bitLen);

/*------------------------------------------------------------------------------
	This function copies 'bitLen' less significant bits of 'source' t_numeric to
	'destination' ad offset 'destOffset'.
------------------------------------------------------------------------------*/
extern void numericToBits (void* destination, unsigned destOffset, t_numeric source, unsigned bitLen);

/*------------------------------------------------------------------------------
	Reads a little-endian value of 'bitLen' number of bits and returns it.
	The 'sourceOffset', in bits, must be octet aligned; 'bitLen' must be an exact
	number of octets.
------------------------------------------------------------------------------*/
extern t_numeric binaryToNumeric (const void* source, unsigned sourceOffset, unsigned bitLen);

/*------------------------------------------------------------------------------
	Writes a little-endian value of 'bitLen' number of bits at 'destination'.
	The 'destOffset', in bits, must be octet aligned; 'bitLen' must be an exact
	number of octets.
------------------------------------------------------------------------------*/
extern void numericToBinary (void* destination, unsigned destOffset, t_numeric source, unsigned bitLen);

/*------------------------------------------------------------------------------
	Reads a little-endian array of 'bitLen' number of bits and returns it.
	The 'sourceOffset', in bits, must be octet aligned; 'bitLen' must be an exact
	number of octets.
------------------------------------------------------------------------------*/
extern t_numeric binaryToNumeric (const void* source, unsigned sourceOffset, unsigned bitLen);

/*------------------------------------------------------------------------------
	Writes a little-endian value of 'bitLen' number of bits at 'destination'.
	The 'destOffset', in bits, must be octet aligned; 'bitLen' must be an exact
	number of octets.
------------------------------------------------------------------------------*/
extern void numericToBinary (void* destination, unsigned destOffset, t_numeric source, unsigned bitLen);

/*------------------------------------------------------------------------------
	Writes a BCD value of 'bitLen' number of bits at 'destination'.
	The 'destOffset', in bits, must be octet aligned; 'bitLen' must be an exact
	number of octets.

	Example: BCD x67 x45 x23 x41 means 41234567

	The BCD values support "invalid BCDs". They are coded as xFF xFF xFF xFF
	and saved in the numeric as INVALID_BCD.
	Little endian version.
------------------------------------------------------------------------------*/
extern t_numeric BCDToNumeric (const void* source, unsigned sourceOffset, unsigned bitLen);

/*------------------------------------------------------------------------------
	Writes a BCD value of 'bitLen' number of bits at 'destination'.
	The 'destOffset', in bits, must be octet aligned; 'bitLen' must be an exact
	number of octets.

	The BCD values support "invalid BCDs". They are coded as xFF xFF xFF xFF
	and saved in the numeric as INVALID_BCD.
	Little endian version.
------------------------------------------------------------------------------*/
extern void numericToBCD (void* destination, unsigned destOffset, t_numeric source, unsigned bitLen);

/*------------------------------------------------------------------------------
	Writes a BCD value of 'bitLen' number of bits at 'destination'.
	The 'destOffset', in bits, must be octet aligned; 'bitLen' must be an exact
	number of octets.

	Example: BCD x12 x34 x56 x78 means 12345678

	The BCD values support "invalid BCDs". They are coded as xFF xFF xFF xFF
	and saved in the numeric as INVALID_BCD.
	Big endian version.
------------------------------------------------------------------------------*/
extern t_numeric RevBCDToNumeric (const void* source, unsigned sourceOffset, unsigned bitLen);

/*------------------------------------------------------------------------------
	Writes a BCD value of 'bitLen' number of bits at 'destination'.
	The 'destOffset', in bits, must be octet aligned; 'bitLen' must be an exact
	number of octets.

	The BCD values support "invalid BCDs". They are coded as xFF xFF xFF xFF
	and saved in the numeric as INVALID_BCD.
	Big endian version.
------------------------------------------------------------------------------*/
extern void numericToRevBCD (void* destination, unsigned destOffset, t_numeric source, unsigned bitLen);

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
extern t_numeric BCDToNumericForTones (const void* source, unsigned sourceOffset);

/*------------------------------------------------------------------------------
	Same as "numericToBCD" but with the rules of "BCDToNumericForTones"
------------------------------------------------------------------------------*/
extern void numericToBCDForTones (void* destination, unsigned destOffset, t_numeric source);

/*------------------------------------------------------------------------------
	Reads from binary mode an unicode string
------------------------------------------------------------------------------*/
extern void binaryToUnicode (const void* source, unsigned sourceOffset, t_unicode* target, unsigned bitLen);

/*------------------------------------------------------------------------------
	Writes in binary mode an unicode string
------------------------------------------------------------------------------*/
extern void unicodeToBinary (void* destination, unsigned destOffset, const t_unicode* source, unsigned bitLen);

/*------------------------------------------------------------------------------
	Reads from binary mode an ascii string
------------------------------------------------------------------------------*/
extern void asciiBinaryToUnicode (const void* source, unsigned sourceOffset, t_unicode* target, unsigned bitLen);

/*------------------------------------------------------------------------------
	Writes in binary mode an ascii string
------------------------------------------------------------------------------*/
extern void unicodeToAsciiBinary (void* destination, unsigned destOffset, const t_unicode* source, unsigned bitLen);


#ifndef NDEBUG
extern void runBinlibTest ();
#endif

#ifdef  __cplusplus
}
#endif

#endif
