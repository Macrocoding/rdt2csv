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
#include "lookup.h"
#include <assert.h>
#include "binlib.h"
#include "crc.h"

/* Initialize a lookup table */
void INIT_LookupTable (LookupTable* tab, unsigned maxNumberOfElements)
{
	unsigned sz;
	memset (tab, 0, sizeof(*tab));
	sz = (unsigned)(maxNumberOfElements * sizeof(tab->elements[0]));
	tab->elements = (LookupElement*)binAlloc (sz);
	if (tab->elements) {
		memset (tab->elements, 0, sz);
		tab->allocatedElements = maxNumberOfElements;
	}
}

/* Free a lookup table */
void RESET_LookupTable (LookupTable* tab)
{
	#ifndef NDEBUG
	unsigned i;
	#endif
	assert (tab->usedElements <= tab->allocatedElements);
	
	#ifndef NDEBUG
	for (i=0; i<tab->usedElements; i++) {
		tab->elements[i].key = 0;
		tab->elements[i].lineNo = 0;
	}
	for (; i<tab->allocatedElements; i++) {
		assert (tab->elements[i].key == 0);
		assert (tab->elements[i].lineNo == 0);
	}
	#endif
	tab->usedElements = 0;
}

/* Free a lookup table */
void FREE_LookupTable (LookupTable* tab)
{
	RESET_LookupTable (tab);
	binFree(tab->elements);
	tab->elements = NULL;
}

#define PCA_NOT_FOUND 0x7FFFFFFF

/*==============================================================================
	Find the position of an entry in a sorted array defined as a macro.
	It fills "Position" with the position found or where it should be inserted.
	"Found" is set to 1 if found, 0 if it has to be inserted.
==============================================================================*/
#define PCA_FastFindOrInsert(arr,NoOfElements,SearchedKey,OP_LessThan,OP_EqualTo,Position,Found) \
	{\
		int __l=0, __r=(NoOfElements)-1, __x=0, __nn=(NoOfElements);\
		(Position) = PCA_NOT_FOUND; (Found)=0;\
		while (__r >= __l) {\
			__x = (__l+__r) >> 1;\
			if (OP_LessThan ((SearchedKey), (arr) [__x])) __r = __x-1; else __l=__x+1;\
			if (OP_EqualTo ((SearchedKey), (arr) [__x])) {\
				(Position) = __x;\
				(Found) = 1;\
				break;\
			}\
		}\
		if (!(Found)) {\
			if (__nn == 0) {\
				(Position) = 0;\
			} else {\
				if (OP_LessThan ((SearchedKey), (arr) [__x])) {\
					(Position) = __x;\
				} else {\
					(Position) = __x+1;\
				}\
			}\
		}\
		else while ((Position) > 0 && OP_EqualTo ((SearchedKey), (arr) [(Position)-1])) (Position)--;\
	}

/* Add an entry to the lookup table. Returns 0 if ok, lineNum of the other occurrence if dupe, <0 in case of error (overflow) */
int ADD_LookupTableUnicode (LookupTable* tab, const t_unicode* key, unsigned lineNo)
{
	unsigned position;
	unsigned found;
	unsigned keyCrc;
	
	assert (lineNo > 0);
	keyCrc = crc32_AddUnicodeLowerCase (0, key);
	
	#define OPLT(k,a) k < a.key
	#define OPEQ(k,a) k == a.key
	PCA_FastFindOrInsert (tab->elements, tab->usedElements, keyCrc, OPLT, OPEQ, position, found)
	#undef OPLT
	#undef OPEQ

	if (found) {
		assert (tab->elements[position].lineNo > 0);
		return tab->elements[position].lineNo;
	}
	if (tab->usedElements >= tab->allocatedElements) {
		return -1;
	}
	memmove	(tab->elements+position+1, tab->elements+position, (tab->usedElements-position)*sizeof(tab->elements[0]));
	tab->usedElements++;
	tab->elements[position].key = keyCrc;
	tab->elements[position].keyText = key;
	tab->elements[position].lineNo = lineNo;

	#ifndef NDEBUG
	{
		unsigned k;
		for (k=1; k<tab->usedElements; k++) {
			assert (tab->elements[k-1].key < tab->elements[k].key);
		}
	}
	#endif
		
	return 0;
}

/* Add an entry to the lookup table. Returns 0 if ok, lineNum of the other occurrence if dupe, <0 in case of error (overflow) */
int ADD_LookupTableNumeric (LookupTable* tab, t_numeric key, unsigned lineNo)
{
	unsigned position;
	unsigned found;
	
	assert (lineNo > 0);
	
	#define OPLT(k,a) k < a.key
	#define OPEQ(k,a) k == a.key
	PCA_FastFindOrInsert (tab->elements, tab->usedElements, key, OPLT, OPEQ, position, found)
	#undef OPLT
	#undef OPEQ

	if (found) {
		assert (tab->elements[position].lineNo > 0);
		return tab->elements[position].lineNo;
	}
	if (tab->usedElements >= tab->allocatedElements) {
		return -1;
	}
	memmove	(tab->elements+position+1, tab->elements+position, (tab->usedElements-position)*sizeof(tab->elements[0]));
	tab->usedElements++;
	tab->elements[position].key = key;
	tab->elements[position].keyText = NULL;
	tab->elements[position].lineNo = lineNo;

	#ifndef NDEBUG
	{
		unsigned k;
		for (k=1; k<tab->usedElements; k++) {
			assert (tab->elements[k-1].key < tab->elements[k].key);
		}
	}
	#endif
		
	return 0;
}

/* Find an entry in the lookup table. Returns 0 if not found, >=1 (the line number) if found */
extern unsigned FIND_LookupTableId (const LookupTable* tab, unsigned keyCrc, const t_unicode** keyTextPtr)
{
	unsigned position;
	unsigned found;
	unsigned lineNo=0;
	
	#define OPLT(k,a) k < a.key
	#define OPEQ(k,a) k == a.key
	PCA_FastFindOrInsert (tab->elements, tab->usedElements, keyCrc, OPLT, OPEQ, position, found)
	#undef OPLT
	#undef OPEQ
	if (found) {
		lineNo = tab->elements[position].lineNo;
		if (keyTextPtr) (*keyTextPtr) = tab->elements[position].keyText;
		assert (lineNo > 0);
	}
	return lineNo;
}

/* Find an entry in the lookup table. Returns 0 if not found, >=1 (the line number) if found */
unsigned FIND_LookupTableAsciiz (const LookupTable* tab, const char* key)
{
	return FIND_LookupTableId (tab, crc32_AddAsciizLowerCase (0, key), NULL);
}

#ifndef NDEBUG
void TEST_LOOKUP ()
{
	LookupTable tab;
	int ret;
	static const t_unicode msg1 [] = {'a', 'b', 'c', 0x00};
	static const t_unicode msg2 [] = {'a', 'b', 'c', 0x00};
	static const t_unicode msg3 [] = {'x', 'y', 'z', 0x00};
	INIT_LookupTable (&tab, 32);
	
	ret = ADD_LookupTableUnicode (&tab, msg1, 1);
	assert (ret == 0);
	ret = ADD_LookupTableUnicode (&tab, msg2, 2);
	assert (ret == 1);
	ret = ADD_LookupTableUnicode (&tab, msg3, 3);
	assert (ret == 0);
	
	ret = FIND_LookupTableAsciiz(&tab, "abc");
	assert (ret == 1);
	ret = FIND_LookupTableAsciiz(&tab, "xyz");
	assert (ret == 3);
	ret = FIND_LookupTableAsciiz(&tab, "nnn");
	assert (ret == 0);
	
	FREE_LookupTable (&tab);
}
#endif
