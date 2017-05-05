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
#ifndef __CRC_H
#define __CRC_H
#include "binlib.h"
#ifdef __cplusplus
	extern "C" {
#endif

extern unsigned crc32Table [];


/*--------------------------------------------------------------------------
	Add an one byte to the CRC
--------------------------------------------------------------------------*/
#define crc32_Add(base,ch) (TPCCrc32Table [(((base)^ 0xffffffffUL) ^ ((unsigned char)(ch))) & 0xff] ^ (((base)^ 0xffffffffUL) >> 8) ^ 0xffffffffUL)

/*--------------------------------------------------------------------------
	Add an ASCIIZ string to the CRC.
--------------------------------------------------------------------------*/
unsigned crc32_AddAsciiz (unsigned base, const char* string);

/*--------------------------------------------------------------------------
	Add an ASCIIZ string to the CRC. The string is considered lowercase.
--------------------------------------------------------------------------*/
unsigned crc32_AddAsciizLowerCase (unsigned base, const char* string);

/*--------------------------------------------------------------------------
	Add an UNICODE string to the CRC. The string is considered lowercase.
--------------------------------------------------------------------------*/
unsigned crc32_AddUnicodeLowerCase (unsigned base, const t_unicode* string);

#ifdef  __cplusplus
}
#endif

#endif
