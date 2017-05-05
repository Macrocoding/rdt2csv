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
#ifndef __MD380_TOOLS_H
#define __MD380_TOOLS_H
#include "md380.h"
#include "csv.h"
#include "md380_cli.h"
#include "lookup.h"

#ifdef __cplusplus
	extern "C" {
#endif

#define MD380ERR_EMPTY_EOF           (-10000)
#define MD380ERR_INVALID_ENUM        (-10001)
#define MD380ERR_FILE_ERROR          (-10002)
#define MD380ERR_INVALID_CSV_HEADER  (-10003)
#define MD380ERR_INVALID_CSV_FORMAT  (-10004)

#define MD380_UNUSED_FIELD 0xFFFFFFFF
#define MD380_CSV_BUFFER_SIZE 192

#define MD380_DEFAULT_SEPARATOR ','
#define FILE_SIZE_TBINFile 262144
#define FILE_OFST_TBINFile 0x225

/* Pointer to function for reporting validation errors */
typedef void (*ReportErrorFunc)(void* reportErrorParam, const char* recordType, int recordNumber, const char* fieldName, const char* text);

/*=========================================================================
	Configuration structure
=========================================================================*/
typedef struct {
	/* Separator to be used */
	char separator;
	
	/* Filename of the .rdt file */
	char* rdtFileName;
	
	/* False if read, true if export */
	enum {modeUnset, modeExport, modeUpdate} updateMode;
	
	/* CSV File Names */
	CSVFileNames csvFileNames;

} MD380_Configuration;

/* Initialize */
extern void INIT_MD380_Configuration(MD380_Configuration* config);

/* Free */
extern void FREE_MD380_Configuration(MD380_Configuration* config);

/*=================================================================================
	Writes a fieldValue according to the rules expressed in fieldDescriptor.
	Parameters and return values same as csvWriteToken.
	In addition, it returns MD380ERR_INVALID_ENUM if "fieldDescriptor" reports
	enums and "fieldValue" is not among the enumerated values.
=================================================================================*/
extern int md380_CsvWriteNumericField (const FieldDescriptor* fieldDescriptor, t_numeric fieldValue, const char* recordName, unsigned lineNo, WriteBinaryFunc writeBinaryFunc, void* writeBinaryFuncParameter, char** errorMessage);

/*=================================================================================
	Writes a fieldValue according to the rules expressed in fieldDescriptor.
	Parameters and return values same as csvWriteToken.
	In addition, it returns MD380ERR_INVALID_ENUM if "fieldDescriptor" reports
	enums and "fieldValue" is not among the enumerated values.
=================================================================================*/
extern int md380_CsvWriteReferenceFieldUnicode (const FieldDescriptor* fieldDescriptor, const t_reference* fieldValue, const char* recordName, unsigned lineNo, WriteBinaryFunc writeBinaryFunc, void* writeBinaryFuncParameter, char** errorMessage);
extern int md380_CsvWriteReferenceFieldNumeric (const FieldDescriptor* fieldDescriptor, const t_reference* fieldValue, const char* recordName, unsigned lineNo, WriteBinaryFunc writeBinaryFunc, void* writeBinaryFuncParameter, char** errorMessage);

/*=========================================================================
	Convert a return value from a CSV function to a readable string
=========================================================================*/
extern const char* md380_CsvRetToString (int ret);

/*=========================================================================
	CSV Lib callback function compatibile with "ReadCharFunc"
=========================================================================*/
extern int md380_FILEReadCharFunc (void* f);

/*=========================================================================
	CSV Lib callback function compatibile with "WriteBinaryFunc"
=========================================================================*/
extern int md380_FILEWriteBinaryFunc (void* f, const char* buffer, unsigned length);

/*=========================================================================
	Reads a title line from a CSV source.
	
	PARAMTERS
	
	fieldDescriptors   pointer to the table of FieldDescriptor for the record.
	                     
	noOfFields         number of entries in the "fieldDescriptors"
	
	fieldsMap          this array must be prepared allocated with "noOfFields"
	                   elements. This function will write at fieldsMap[0]
	                   the number of the entry of fieldDescriptors that
	                   matches the title at column 0 and so on.
	
	numberOfColumns    number of columns available in "fieldsMap"

	readCharFunc       See csvReadToken
	readCharFuncParameter  See csvReadToken
	
	Returns CSVRET_xxx and MD380ERR_xxx values.
=========================================================================*/
extern int md380_ReadColumnsMapping (const FieldDescriptor* fieldDescriptors, unsigned noOfFields, unsigned* fieldsMap, unsigned* numberOfColumns, char separator, ReadCharFunc readCharFunc, void* readCharFuncParameter, char** errorMessage);

/*=========================================================================
	Reads one unicode field from a CSV source
	
	PARAMTERS
	
	fieldDescriptor    pointer to the FieldDescriptor for this field
	fieldPointer       pointer to the field: t_numeric* if numeric, t_unicode* if unicode
	readCharFunc       See csvReadToken
	readCharFuncParameter  See csvReadToken
	
	Returns CSVRET_xxx and MD380ERR_xxx values.
=========================================================================*/
extern int md380_ReadFieldUnicode (const FieldDescriptor* fieldDescriptor, t_unicode* fieldPointer, unsigned unicodeChars, const char* fileName, unsigned lineNo, char separator, ReadCharFunc readCharFunc, void* readCharFuncParameter, char** errorMessage);

/*=========================================================================
	Reads one reference field from a CSV source
	
	PARAMTERS
	
	fieldDescriptors   pointer to the FieldDescriptor for this field
	fieldPointer       pointer to the t_unicode field
	unicodeChars       number of character of the unicode
	lineNo             line number in the CSV file (used for error reporting)
	separator          separator used in the CSV file
	readCharFunc       See csvReadToken
	readCharFuncParameter  See csvReadToken
	
	Returns CSVRET_xxx and MD380ERR_xxx values.
=========================================================================*/
extern int md380_ReadFieldReferenceUnicode (const FieldDescriptor* fieldDescriptor, t_reference* fieldPointer, const char* fileName, unsigned lineNo, char separator, ReadCharFunc readCharFunc, void* readCharFuncParameter, char** errorMessage);
extern int md380_ReadFieldReferenceNumeric (const FieldDescriptor* fieldDescriptor, t_reference* fieldPointer, const char* fileName, unsigned lineNo, char separator, ReadCharFunc readCharFunc, void* readCharFuncParameter, char** errorMessage);

/*=========================================================================
	Reads one numeric field from a CSV source
	
	PARAMTERS
	
	fieldDescriptors   pointer to the FieldDescriptor for this field
	fieldPointer       pointer to the t_numeric field
	lineNo             line number in the CSV file (used for error reporting)
	separator          separator used in the CSV file
	readCharFunc       See csvReadToken
	readCharFuncParameter  See csvReadToken
	
	Returns CSVRET_xxx and MD380ERR_xxx values.
=========================================================================*/
extern int md380_ReadFieldNumeric (const FieldDescriptor* fieldDescriptor, t_numeric* fieldPointer, const char* fileName, unsigned lineNo, char separator, ReadCharFunc readCharFunc, void* readCharFuncParameter, char** errorMessage);


/*==================================================================
	Resolve the references in a container
	Returns 0 if ok, != 0 in case of error
==================================================================*/
extern int md380_ResolveReference (const FieldDescriptor* fieldDescriptor, t_reference* ref, const LookupTable* tab, const char* recordType, int recordNumber, const char* fieldName, const char* searchedTable, ReportErrorFunc reportErrorFunc, void* reportErrorParam);

/*==================================================================
	Bind the references in a container
	Returns 0 if ok, != 0 in case of error
==================================================================*/
extern int md380_BindReferenceUnicode (const FieldDescriptor* fieldDescriptor, t_reference* ref, unsigned noOfRecsInReferencedTable, const t_unicode* referencedString, const char* recordType, int recordNumber, const char* fieldName, const char* searchedTable, ReportErrorFunc reportErrorFunc, void* reportErrorParam);

/*==================================================================
	Bind the references in a container
	Returns 0 if ok, != 0 in case of error
==================================================================*/
extern int md380_BindReferenceNumeric (const FieldDescriptor* fieldDescriptor, t_reference* ref, unsigned noOfRecsInReferencedTable, t_numeric referencedValue, const char* recordType, int recordNumber, const char* fieldName, const char* searchedTable, ReportErrorFunc reportErrorFunc, void* reportErrorParam);

#ifdef __cplusplus
	}
#endif

#endif
