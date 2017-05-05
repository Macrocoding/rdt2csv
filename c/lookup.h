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
#ifndef __LOOKUP_H
#define __LOOKUP_H
#include "binlib.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	/* Sorting unique key (CRC32 of the lower case string) */
	unsigned key;
	
	/* Pointer to the unicode string */
	const t_unicode* keyText;
	
	/* Line containing the given key. 0=unset, 1=first line */
	unsigned lineNo;

} LookupElement;

typedef struct {
	/* Array of elements */
	LookupElement* elements;
	
	/* Number of allocated elements */
	unsigned allocatedElements;
	
	/* Number of used elements */
	unsigned usedElements;

} LookupTable;

/* Initialize a lookup table */
extern void INIT_LookupTable (LookupTable* tab, unsigned maxNumberOfElements);

/* Reset a lookup table */
extern void RESET_LookupTable (LookupTable* tab);

/* Free a lookup table */
extern void FREE_LookupTable (LookupTable* tab);

/* Add an entry to the lookup table. Returns 0 if ok, lineNum of the other occurrence if dupe, <0 in case of error (overflow) */
extern int ADD_LookupTableUnicode (LookupTable* tab, const t_unicode* key, unsigned lineNo);

/* Add an entry to the lookup table. Returns 0 if ok, lineNum of the other occurrence if dupe, <0 in case of error (overflow) */
extern int ADD_LookupTableNumeric (LookupTable* tab, t_numeric key, unsigned lineNo);

/* Find an entry in the lookup table. Returns 0 if not found, >=1 (the line number) if found */
extern unsigned FIND_LookupTableId (const LookupTable* tab, unsigned keyCrc, const t_unicode** keyTextPtr);

/* Find an entry in the lookup table. Returns 0 if not found, >=1 (the line number) if found */
extern unsigned FIND_LookupTableAsciiz (const LookupTable* tab, const char* key);

#ifndef NDEBUG
extern void TEST_LOOKUP ();
#endif

#ifdef  __cplusplus
}
#endif

#endif
