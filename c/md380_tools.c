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
#include "md380_tools.h"
#include "crc.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* Initialize */
void INIT_MD380_Configuration(MD380_Configuration* config)
{
	memset (config, 0, sizeof(*config));
	config->separator = MD380_DEFAULT_SEPARATOR;
	config->updateMode = modeUnset;
	INIT_CSVFileNames(&config->csvFileNames);
}

/* Free */
void FREE_MD380_Configuration(MD380_Configuration* config)
{
	FREE_CSVFileNames(&config->csvFileNames);
	if (config->rdtFileName) {binFree (config->rdtFileName); config->rdtFileName=NULL;}
}

/*=================================================================================
	Writes a fieldValue according to the rules expressed in fieldDescriptor.
	Parameters and return values same as csvWriteToken.
	In addition, it returns MD380ERR_INVALID_ENUM if "fieldDescriptor" reports
	enums and "fieldValue" is not among the enumerated values.
=================================================================================*/
static int md380_CsvWriteReferenceFieldCommon (int* exitNow, const FieldDescriptor* fieldDescriptor, const t_reference* fieldValue, WriteBinaryFunc writeBinaryFunc, void* writeBinaryFuncParameter, char** errorMessage)
{
	int ret = CSVRET_OK;
	assert (fieldDescriptor->fieldType == BL_integer);
	assert (errorMessage != NULL);
	assert ((*errorMessage) == NULL);
	(*exitNow) = 0;

	/* First check if the value corresponds to an explicit enumeration */
	if (fieldDescriptor->enumCount > 0) {
		unsigned i;
		
		/* Find the enumeration among all values */
		for (i=0; i<fieldDescriptor->enumCount; i++) {
			if (fieldDescriptor->fieldEnumerators[i].enumValue == fieldValue->refLineNo) {
				ret = csvWriteToken (fieldDescriptor->fieldEnumerators[i].enumName, -1, writeBinaryFunc, writeBinaryFuncParameter);
				(*exitNow) = 1;
				goto exitFunc;
			}
		}
	}

exitFunc:	
	return ret;
}

/*=================================================================================
	Writes a fieldValue according to the rules expressed in fieldDescriptor.
	Parameters and return values same as csvWriteToken.
	In addition, it returns MD380ERR_INVALID_ENUM if "fieldDescriptor" reports
	enums and "fieldValue" is not among the enumerated values.

	UNICODE KEY VERSION
=================================================================================*/
int md380_CsvWriteReferenceFieldUnicode (const FieldDescriptor* fieldDescriptor, const t_reference* fieldValue, const char* recordName, unsigned lineNo, WriteBinaryFunc writeBinaryFunc, void* writeBinaryFuncParameter, char** errorMessage)
{
	int exitNow;
	int ret = md380_CsvWriteReferenceFieldCommon (&exitNow, fieldDescriptor, fieldValue, writeBinaryFunc, writeBinaryFuncParameter, errorMessage);

	if (!exitNow && ret == CSVRET_OK) {
		if (fieldValue->resolvedName == NULL) {
			assert (fieldValue->refLineNo == 0);
			assert (fieldValue->refId == 0);
		}
		else {
			ret = csvWriteTokenUnicode (fieldValue->resolvedName, writeBinaryFunc, writeBinaryFuncParameter);
			
			if (ret != CSVRET_OK) {
				(*errorMessage) = (char*)binAlloc (strlen(recordName)+strlen(fieldDescriptor->fieldName)+80);
				sprintf ((*errorMessage), "Error in %s, line %u, field %s: %s", recordName, lineNo, fieldDescriptor->fieldName, md380_CsvRetToString(ret));
			}
		}
	}
	return ret;
}

/*=================================================================================
	Writes a fieldValue according to the rules expressed in fieldDescriptor.
	Parameters and return values same as csvWriteToken.
	In addition, it returns MD380ERR_INVALID_ENUM if "fieldDescriptor" reports
	enums and "fieldValue" is not among the enumerated values.

	NUMERIC KEY VERSION
=================================================================================*/
int md380_CsvWriteReferenceFieldNumeric (const FieldDescriptor* fieldDescriptor, const t_reference* fieldValue, const char* recordName, unsigned lineNo, WriteBinaryFunc writeBinaryFunc, void* writeBinaryFuncParameter, char** errorMessage)
{
	int exitNow;
	int ret = md380_CsvWriteReferenceFieldCommon (&exitNow, fieldDescriptor, fieldValue, writeBinaryFunc, writeBinaryFuncParameter, errorMessage);

	if (!exitNow && ret == CSVRET_OK) {
		ret = csvWriteTokenUnsigned (fieldValue->refId, writeBinaryFunc, writeBinaryFuncParameter);
		
		if (ret != CSVRET_OK) {
			(*errorMessage) = (char*)binAlloc (strlen(recordName)+strlen(fieldDescriptor->fieldName)+80);
			sprintf ((*errorMessage), "Error in %s, line %u, field %s: %s", recordName, lineNo, fieldDescriptor->fieldName, md380_CsvRetToString(ret));
		}
	}
	return ret;
}

/*=================================================================================
	Writes a fieldValue according to the rules expressed in fieldDescriptor.
	Parameters and return values same as csvWriteToken.
	In addition, it returns MD380ERR_INVALID_ENUM if "fieldDescriptor" reports
	enums and "fieldValue" is not among the enumerated values.
=================================================================================*/
int md380_CsvWriteNumericField (const FieldDescriptor* fieldDescriptor, t_numeric fieldValue, const char* recordName, unsigned lineNo, WriteBinaryFunc writeBinaryFunc, void* writeBinaryFuncParameter, char** errorMessage)
{
	int ret = CSVRET_OK;
	unsigned i;
	assert (fieldDescriptor->fieldType == BL_integer);
	assert (errorMessage != NULL);
	assert ((*errorMessage) == NULL);
	
	if (fieldDescriptor->enumCount == 0) {
		ret = csvWriteTokenUnsigned ((unsigned)fieldValue, writeBinaryFunc, writeBinaryFuncParameter);
	}
	else {
		/* Find the enumeration among all values */
		for (i=0; i<fieldDescriptor->enumCount; i++) {
			if (fieldDescriptor->fieldEnumerators[i].enumValue == fieldValue) {
				ret = csvWriteToken (fieldDescriptor->fieldEnumerators[i].enumName, -1, writeBinaryFunc, writeBinaryFuncParameter);
				break;
			}
		}
		if (i >= fieldDescriptor->enumCount) {
			ret = csvWriteTokenUnsigned ((unsigned)fieldValue, writeBinaryFunc, writeBinaryFuncParameter);
			if (ret == CSVRET_OK) {
				unsigned enumsLen = 0;
				unsigned k;
				#define MAX_ENUMS_TO_SHOW 5
				for (k=0; k<MAX_ENUMS_TO_SHOW && k<fieldDescriptor->enumCount; k++) {
					enumsLen += strlen (fieldDescriptor->fieldEnumerators[k].enumName)+4;
				}
				(*errorMessage) = (char*)binAlloc (strlen(recordName)+strlen(fieldDescriptor->fieldName)+80+enumsLen);
				sprintf ((*errorMessage), "Error in %s, line %u, field %s: invalid value %u. Valid values are ", recordName, lineNo, fieldDescriptor->fieldName, (unsigned)fieldValue);
				for (k=0; k<MAX_ENUMS_TO_SHOW && k<fieldDescriptor->enumCount; k++) {
					if (k>0) strcat ((*errorMessage), ", ");
					strcat ((*errorMessage), fieldDescriptor->fieldEnumerators[k].enumName);
				}
				if (k < fieldDescriptor->enumCount) {
					strcat ((*errorMessage), ", etc... (see docs)");
				}
				ret = MD380ERR_INVALID_ENUM;
				goto exitFunc;
			}
		}
	}
	
	if (ret != CSVRET_OK) {
		(*errorMessage) = (char*)binAlloc (strlen(recordName)+strlen(fieldDescriptor->fieldName)+80);
		sprintf ((*errorMessage), "Error in %s, line %u, field %s: %s", recordName, lineNo, fieldDescriptor->fieldName, md380_CsvRetToString(ret));
	}

exitFunc:	
	return ret;
}

/*=========================================================================
	Convert a return value from a CSV function to a readable string
=========================================================================*/
const char* md380_CsvRetToString (int ret)
{
	const char* desc;
	switch (ret) {
		case CSVRET_OK               : desc = "CSVRET_OK"; break;
		case CSVRET_EOF              : desc = "CSVRET_EOF"; break;
		case CSVRET_EOL              : desc = "CSVRET_EOL"; break;
		case CSVRET_UNEXP_QUOT       : desc = "CSVRET_UNEXP_QUOT"; break;
		case CSVRET_MISSING_LF       : desc = "CSVRET_MISSING_LF"; break;
		case CSVRET_INVALID_CH       : desc = "CSVRET_INVALID_CH"; break;
		case CSVRET_UNEXP_EOF        : desc = "CSVRET_UNEXP_EOF"; break;
		case CSVRET_INVALID_POST_QUOT: desc = "CSVRET_INVALID_POST_QUOT"; break;
		case CSVRET_WRITE_ERROR      : desc = "CSVRET_WRITE_ERROR"; break;
		case CSVRET_OUT_OF_MEMORY    : desc = "CSVRET_OUT_OF_MEMORY"; break;
		case MD380ERR_INVALID_ENUM   : desc = "MD380ERR_INVALID_ENUM"; break;
		case MD380ERR_FILE_ERROR     : desc = "MD380ERR_FILE_ERROR"; break;
		case MD380ERR_INVALID_CSV_HEADER: desc = "MD380ERR_INVALID_CSV_HEADER"; break;
		case MD380ERR_INVALID_CSV_FORMAT: desc = "MD380ERR_INVALID_CSV_FORMAT"; break;
		default: desc = "UNKNWOWN_RET";
	}
	return desc;
}


/*=========================================================================
	CSV Lib callback function compatibile with "ReadCharFunc"
=========================================================================*/
int md380_FILEReadCharFunc (void* f)
{
	int ret = fgetc ((FILE*)f);
	if (ret == EOF) return csvEOF;
	return ret;
}

/*=========================================================================
	CSV Lib callback function compatibile with "WriteBinaryFunc"
=========================================================================*/
int md380_FILEWriteBinaryFunc (void* f, const char* buffer, unsigned length)
{
	int ret = fwrite (buffer, 1, length, (FILE*)f);
	return (ret != (int)length);
}


/*=========================================================================
	Reads a title line from a CSV source.
	
	PARAMTERS
	
	fieldDescriptors   pointer to the table of FieldDescriptor for the record.
	                     
	noOfFields         number of entries in the "fieldDescriptors"
	
	fieldsMap          this array must be prepared allocated with "noOfFields"
	                   elements. This function will write at fieldsMap[0]
	                   the number of the entry of fieldDescriptors that
	                   matches the title at column 0 and so on.
	                   Unused fields range is marked with MD380_UNUSED_FIELD.

	readCharFunc       See csvReadToken
	readCharFuncParameter  See csvReadToken
	
	Returns CSVRET_xxx and MD380ERR_xxx values.
=========================================================================*/
int md380_ReadColumnsMapping (const FieldDescriptor* fieldDescriptors, unsigned noOfFields, unsigned* fieldsMap, unsigned* numberOfColumns, char separator, ReadCharFunc readCharFunc, void* readCharFuncParameter, char** errorMessage)
{
	int ret = CSVRET_OK;
	unsigned i;
	char* antiDupe = NULL;
	char* buffer = NULL;
	unsigned longestFieldName = 0;
	
	assert (errorMessage);
	assert ((*errorMessage) == NULL);
	(*numberOfColumns) = 0;
	
	/* Reset the fields map */
	for (i=0; i<noOfFields; i++) {
		fieldsMap[i] = MD380_UNUSED_FIELD;
	}
	
	/* Allocate the anti-dupe map. This map will be used to detect */
	/* whether a field has been already found in a previous column */
	antiDupe = (char*)binAlloc(noOfFields);
	if (antiDupe == NULL) {ret = CSVRET_OUT_OF_MEMORY; goto exitFunc;}
	memset (antiDupe, 0, noOfFields*sizeof(antiDupe[0]));

	/* Calculate the longest field name */
	for (i=0; i<noOfFields; i++) {
		unsigned len = (unsigned)strlen (fieldDescriptors[i].fieldName);
		if (len > longestFieldName) longestFieldName = len;
	}
	longestFieldName += 10;
	
	/* Allocate a buffer that is enough for the longest field name. If longer, it */
	/* won't be a valid field name anyway */
	buffer = (char*)binAlloc(longestFieldName);
	if (buffer == NULL) {ret = CSVRET_OUT_OF_MEMORY; goto exitFunc;}
	memset (buffer, 0, longestFieldName);
	
	/* Read fields */
	for (i=0; ret == CSVRET_OK; i++) {
		int fieldLength;
		unsigned j;
		unsigned nameCrc;
		
		ret = csvReadToken (buffer, longestFieldName, &fieldLength, separator, readCharFunc, readCharFuncParameter);
		if (ret == CSVRET_OK || ret == CSVRET_EOL || ret == CSVRET_EOF) {
			/* Calculate the crc-32 of the lowercase name */
			nameCrc = crc32_AddAsciizLowerCase (0, buffer);	
			
			/* See if this name is known */
			for (j=0; j<noOfFields; j++) {
				if (fieldDescriptors[j].fieldNameCRC == nameCrc) break;
			}
			
			/* Field does not exist */
			if (j >= noOfFields) {
				(*errorMessage) = binAlloc (strlen (buffer) + 40);
				if ((*errorMessage) == NULL) {ret = CSVRET_OUT_OF_MEMORY; goto exitFunc;}
				sprintf ((*errorMessage), "unknown field '%s' at column %u", buffer, i);
				ret = MD380ERR_INVALID_CSV_HEADER;
				goto exitFunc;
			}
			
			/* Field already set */
			if (antiDupe[j]) {
				(*errorMessage) = binAlloc (strlen (buffer) + 40);
				if ((*errorMessage) == NULL) {ret = CSVRET_OUT_OF_MEMORY; goto exitFunc;}
				sprintf ((*errorMessage), "field '%s' at column %u already defined", buffer, i);
				ret = MD380ERR_INVALID_CSV_HEADER;
				goto exitFunc;
			}
			
			antiDupe[j] = 1;
			fieldsMap[i] = j;
			(*numberOfColumns)++;
			assert ((*numberOfColumns) == i+1);
		}
	}

	if (ret == CSVRET_EOL || ret == CSVRET_EOF) ret = CSVRET_OK;
	
exitFunc:
	if (antiDupe) binFree (antiDupe);
	if (buffer) binFree (buffer);
	return ret;
}

/*=========================================================================
	Reads one unicode field from a CSV source
	
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
int md380_ReadFieldUnicode (const FieldDescriptor* fieldDescriptor, t_unicode* fieldPointer, unsigned unicodeChars, const char* fileName, unsigned lineNo, char separator, ReadCharFunc readCharFunc, void* readCharFuncParameter, char** errorMessage)
{
	int ret = CSVRET_OK;
	#define BUFFER_SIZE 128
	char buffer[BUFFER_SIZE];
	int i;
	int fieldLength;
	int errorLength = strlen (fieldDescriptor->fieldName) + strlen(fileName) + 256;
	
	memset (fieldPointer, 0, unicodeChars*sizeof(t_unicode));
	assert (errorMessage != NULL);
	assert ((*errorMessage) == NULL);
	assert (fieldDescriptor->fieldType == BL_unicode);
	
	ret = csvReadToken (buffer, BUFFER_SIZE, &fieldLength, separator, readCharFunc, readCharFuncParameter);
	switch (ret) {
		case CSVRET_OK: break;
		case CSVRET_EOF: {
			if (fieldLength == 0) return MD380ERR_EMPTY_EOF;
			break;
		}
		case CSVRET_EOL: break;
		default: {
			(*errorMessage) = (char*)binAlloc (errorLength);
			if ((*errorMessage) == NULL) return CSVRET_OUT_OF_MEMORY;
			sprintf ((*errorMessage), "file %s, line %u, field '%s' error %s", fileName, lineNo, fieldDescriptor->fieldName, md380_CsvRetToString(ret));
			return MD380ERR_INVALID_CSV_FORMAT;
		}
	}
	
	if (fieldLength > (int)unicodeChars) {
		(*errorMessage) = (char*)binAlloc (errorLength);
		if ((*errorMessage) == NULL) return CSVRET_OUT_OF_MEMORY;
		sprintf ((*errorMessage), "file %s, line %u, field '%s' too long", fileName, lineNo, fieldDescriptor->fieldName);
		return MD380ERR_INVALID_CSV_FORMAT;
	}

	for (i=0; i<fieldLength; i++) {
		fieldPointer[i] = (unsigned char)buffer[i];
	}
	
	return ret;
}

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
static int md380_ReadFieldReferenceCommon (char* buffer, unsigned bufferSize, int* fieldLengthPtr, const FieldDescriptor* fieldDescriptor, t_reference* fieldPointer, const char* fileName, unsigned lineNo, char separator, ReadCharFunc readCharFunc, void* readCharFuncParameter, char** errorMessage)
{
	int ret = CSVRET_OK;
	int errorLength = strlen (fieldDescriptor->fieldName) + strlen(fileName) + 256;
	
	assert (errorMessage != NULL);
	assert ((*errorMessage) == NULL);
	assert (fieldDescriptor->fieldType == BL_integer);
	
	ret = csvReadToken (buffer, bufferSize, fieldLengthPtr, separator, readCharFunc, readCharFuncParameter);
	switch (ret) {
		case CSVRET_OK: break;
		case CSVRET_EOF: {
			if ((*fieldLengthPtr) == 0) return MD380ERR_EMPTY_EOF;
			break;
		}
		case CSVRET_EOL: break;
		default: {
			(*errorMessage) = (char*)binAlloc (errorLength);
			if ((*errorMessage) == NULL) return CSVRET_OUT_OF_MEMORY;
			sprintf ((*errorMessage), "file %s, line %u, field '%s' error %s", fileName, lineNo, fieldDescriptor->fieldName, md380_CsvRetToString(ret));
			return MD380ERR_INVALID_CSV_FORMAT;
		}
	}
	
	fieldPointer->refLineNo = 0;
	fieldPointer->resolvedName = NULL;
	
	return ret;
}

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
int md380_ReadFieldReferenceUnicode (const FieldDescriptor* fieldDescriptor, t_reference* fieldPointer, const char* fileName, unsigned lineNo, char separator, ReadCharFunc readCharFunc, void* readCharFuncParameter, char** errorMessage)
{
	int ret;
	#define BUFFER_SIZE 128
	char buffer[BUFFER_SIZE];
	int fieldLength;
	int i;

	ret = md380_ReadFieldReferenceCommon (buffer, BUFFER_SIZE, &fieldLength, fieldDescriptor, fieldPointer, fileName, lineNo, separator, readCharFunc, readCharFuncParameter, errorMessage);

	fieldPointer->refId = crc32_AddAsciizLowerCase (0, buffer);
	for (i=0; i<fieldLength && i<BL_TEXT_SAMPLE_LEN; i++) {
		fieldPointer->textSample[i] = buffer[i];
	}
	fieldPointer->textSample[i] = '\0';
	
	return ret;
}

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
int md380_ReadFieldReferenceNumeric (const FieldDescriptor* fieldDescriptor, t_reference* fieldPointer, const char* fileName, unsigned lineNo, char separator, ReadCharFunc readCharFunc, void* readCharFuncParameter, char** errorMessage)
{
	int ret;

	#define BUFFER_SIZE 128
	char buffer[BUFFER_SIZE];
	int fieldLength;
	int i;
	int errorLength = strlen (fieldDescriptor->fieldName) + strlen(fileName) + 256;

	ret = md380_ReadFieldReferenceCommon (buffer, BUFFER_SIZE, &fieldLength, fieldDescriptor, fieldPointer, fileName, lineNo, separator, readCharFunc, readCharFuncParameter, errorMessage);

	/* If the field supports some enumeration, calculate the CRC */
	if (ret == CSVRET_OK && fieldDescriptor->enumCount) {
		unsigned textCrc = crc32_AddAsciizLowerCase (0, buffer);
		for (i=0; i<(int)fieldDescriptor->enumCount; i++) {
			if (fieldDescriptor->fieldEnumerators[i].enumNameCRC == textCrc) {
				fieldPointer->refId = fieldDescriptor->fieldEnumerators[i].enumValue;
				goto exitFunc;
			}
		}
	}
	
	if (ret == CSVRET_OK) {
		char* endPtr;
		fieldPointer->refId = strtoul (buffer, &endPtr, 10);
		if ((*endPtr) != '\0') {
			(*errorMessage) = (char*)binAlloc (errorLength+strlen (buffer));
			if ((*errorMessage) == NULL) return CSVRET_OUT_OF_MEMORY;
			sprintf ((*errorMessage), "file %s, line %u, field '%s': invalid value '%s'", fileName, lineNo, fieldDescriptor->fieldName, buffer);
			return MD380ERR_INVALID_CSV_FORMAT;
		}
		for (i=0; i<fieldLength && i<BL_TEXT_SAMPLE_LEN; i++) {
			fieldPointer->textSample[i] = buffer[i];
		}
		fieldPointer->textSample[i] = '\0';
	}
	
exitFunc:
	return ret;

}

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
int md380_ReadFieldNumeric (const FieldDescriptor* fieldDescriptor, t_numeric* fieldPointer, const char* fileName, unsigned lineNo, char separator, ReadCharFunc readCharFunc, void* readCharFuncParameter, char** errorMessage)
{
	int ret = CSVRET_OK;
	#define BUFFER_SIZE 128
	char buffer[BUFFER_SIZE];
	unsigned i;
	int fieldLength;
	int errorLength = strlen (fieldDescriptor->fieldName) + strlen(fileName) + 256;
	
	assert (errorMessage != NULL);
	assert ((*errorMessage) == NULL);
	assert (fieldDescriptor->fieldType == BL_integer);
	
	ret = csvReadToken (buffer, BUFFER_SIZE, &fieldLength, separator, readCharFunc, readCharFuncParameter);
	switch (ret) {
		case CSVRET_OK: break;
		case CSVRET_EOF: {
			if (fieldLength == 0) return MD380ERR_EMPTY_EOF;
			break;
		}
		case CSVRET_EOL: break;
		default: {
			(*errorMessage) = (char*)binAlloc (errorLength);
			if ((*errorMessage) == NULL) return CSVRET_OUT_OF_MEMORY;
			sprintf ((*errorMessage), "file %s, line %u, field '%s': error %s", fileName, lineNo, fieldDescriptor->fieldName, md380_CsvRetToString(ret));
			return MD380ERR_INVALID_CSV_FORMAT;
		}
	}

	/* If the field supports some enumeration, calculate the CRC */
	if (fieldDescriptor->enumCount) {
		unsigned textCrc = crc32_AddAsciizLowerCase (0, buffer);
		for (i=0; i<fieldDescriptor->enumCount; i++) {
			if (fieldDescriptor->fieldEnumerators[i].enumNameCRC == textCrc) break;
		}
		
		if (i >= fieldDescriptor->enumCount) {
			unsigned enumsLen = 0;
			unsigned k;
			for (k=0; k<MAX_ENUMS_TO_SHOW && k<fieldDescriptor->enumCount; k++) {
				enumsLen += strlen (fieldDescriptor->fieldEnumerators[k].enumName)+8;
			}

			(*errorMessage) = (char*)binAlloc (errorLength+strlen (buffer));
			if ((*errorMessage) == NULL) return CSVRET_OUT_OF_MEMORY;
			sprintf ((*errorMessage), "file %s, line %u, field '%s': invalid value '%s'. Valid values are ", fileName, lineNo, fieldDescriptor->fieldName, buffer);
			for (k=0; k<MAX_ENUMS_TO_SHOW && k<fieldDescriptor->enumCount; k++) {
				strcat ((*errorMessage), (k >0 && k+1==fieldDescriptor->enumCount) ? " and " : (k>0 ? ", '" : "'"));
				strcat ((*errorMessage), fieldDescriptor->fieldEnumerators[k].enumName);
				strcat ((*errorMessage), "'");
			}
			if (k < fieldDescriptor->enumCount) {
				strcat ((*errorMessage), ", etc... (see docs)");
			}
			return MD380ERR_INVALID_CSV_FORMAT;
		}
		else {
			(*fieldPointer) = fieldDescriptor->fieldEnumerators[i].enumValue;
		}
	}
	/* Normal value */
	else {
		char* endPtr;
		(*fieldPointer) = strtoul (buffer, &endPtr, 10);
		if ((*endPtr) != '\0') {
			(*errorMessage) = (char*)binAlloc (errorLength+strlen (buffer));
			if ((*errorMessage) == NULL) return CSVRET_OUT_OF_MEMORY;
			sprintf ((*errorMessage), "file %s, line %u, field '%s': invalid value '%s'", fileName, lineNo, fieldDescriptor->fieldName, buffer);
			return MD380ERR_INVALID_CSV_FORMAT;
		}
	}
	
	return ret;
}

/*==================================================================
	Resolve the references in a container
	Returns 0 if ok, != 0 in case of error
==================================================================*/
int md380_ResolveReference (const FieldDescriptor* fieldDescriptor, t_reference* ref, const LookupTable* tab, const char* recordType, int recordNumber, const char* fieldName, const char* searchedTable, ReportErrorFunc reportErrorFunc, void* reportErrorParam)
{
	int ret = 0;
	unsigned lineNo, i;
	if (ref->refLineNo == 0xFFFF) return 0;
	
	if (ref->refId == 0) {
		assert (ref->resolvedName == NULL);
		assert (ref->refLineNo == 0);
		return 0;
	}
	ref->refLineNo = 0;
	ref->resolvedName = NULL;
	assert (ref->refId != 0);
	
	/* See if the value mathces an enumeration value */
	for (i=0; i<fieldDescriptor->enumCount; i++) {
		if (fieldDescriptor->fieldEnumerators[i].enumNameCRC == ref->refId) {
			ref->refLineNo = fieldDescriptor->fieldEnumerators[i].enumValue;
			goto exitFunc;
		}
	}

	lineNo = FIND_LookupTableId(tab, ref->refId, &ref->resolvedName);
	if (lineNo == 0) {
		char* txt = (char*)binAlloc (BL_TEXT_SAMPLE_LEN+60+strlen(searchedTable));
		if (txt) {
			sprintf (txt, "name '%s%s' not found in table %s", ref->textSample, (strlen(ref->textSample)>=BL_TEXT_SAMPLE_LEN ? "..." : ""), searchedTable);
			reportErrorFunc (reportErrorParam, recordType, recordNumber, fieldName, txt);
			binFree(txt);
			ret = 1;
		}
	}
	else {
		ref->refLineNo = lineNo;
	}
	exitFunc:
	return ret;
}

/*==================================================================
	Bind the references in a container
	Returns 0 if ok, < 0 in case of error
==================================================================*/
static int md380_BindReferenceCommon (const FieldDescriptor* fieldDescriptor, t_reference* ref, unsigned noOfRecsInReferencedTable, const char* recordType, int recordNumber, const char* fieldName, const char* searchedTable, ReportErrorFunc reportErrorFunc, void* reportErrorParam)
{
	int ret = 0;
	
	assert (ref->resolvedName == 0);
	assert (ref->refId == 0);

	/* Verify if the value is a valid enumeration */
	if (fieldDescriptor->enumCount > 0) {
		unsigned i;
		for (i=0; i<fieldDescriptor->enumCount; i++) {
			if (fieldDescriptor->fieldEnumerators[i].enumValue == ref->refLineNo) {
				ret = 1;
				goto exitFunc;
			}
		}
	}

	if (ref->refLineNo == 0) {ret=1; goto exitFunc;}

	if (ref->refLineNo > noOfRecsInReferencedTable) {
		char* txt = (char*)binAlloc (60+strlen(searchedTable));
		if (txt) {
			sprintf (txt, "line number '%u' not found in table %s", ref->refLineNo, searchedTable);
			reportErrorFunc (reportErrorParam, recordType, recordNumber, fieldName, txt);
			binFree(txt);
			ret = -1;
			goto exitFunc;
		}
	}

exitFunc:
	return ret;
}


/*==================================================================
	Bind the references in a container
	Returns 0 if ok, != 0 in case of error
==================================================================*/
int md380_BindReferenceUnicode (const FieldDescriptor* fieldDescriptor, t_reference* ref, unsigned noOfRecsInReferencedTable, const t_unicode* referencedString, const char* recordType, int recordNumber, const char* fieldName, const char* searchedTable, ReportErrorFunc reportErrorFunc, void* reportErrorParam)
{
	unsigned ret = md380_BindReferenceCommon (fieldDescriptor, ref, noOfRecsInReferencedTable, recordType, recordNumber, fieldName, searchedTable, reportErrorFunc, reportErrorParam);

	if (ret == 0) {
		unsigned i;
		ref->resolvedName = referencedString;
		ref->refId = crc32_AddUnicodeLowerCase (0, referencedString);
		for (i=0; referencedString[i] && i<BL_TEXT_SAMPLE_LEN; i++) {
			ref->textSample[i] = (char)(unsigned char)referencedString[i];
		}
		ref->textSample[i] = '\0';
	}
	else if (ret > 0) {ret = 0;}

	return ret;
}

/*==================================================================
	Bind the references in a container
	Returns 0 if ok, != 0 in case of error
==================================================================*/
int md380_BindReferenceNumeric (const FieldDescriptor* fieldDescriptor, t_reference* ref, unsigned noOfRecsInReferencedTable, t_numeric referencedValue, const char* recordType, int recordNumber, const char* fieldName, const char* searchedTable, ReportErrorFunc reportErrorFunc, void* reportErrorParam)
{
	unsigned ret = md380_BindReferenceCommon (fieldDescriptor, ref, noOfRecsInReferencedTable, recordType, recordNumber, fieldName, searchedTable, reportErrorFunc, reportErrorParam);

	if (ret == 0) {
		char buf [65];
		ref->resolvedName = NULL;
		ref->refId = (unsigned)referencedValue;
		sprintf (buf, "%u", (unsigned)referencedValue);
		strncpy (ref->textSample, buf, BL_TEXT_SAMPLE_LEN);
		ref->textSample[BL_TEXT_SAMPLE_LEN] = '\0';
	}
	else if (ret > 0) {ret = 0;}

	return ret;
}
