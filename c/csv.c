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
#include "csv.h"
#include <string.h>
#include <stdio.h>

/*=================================================================================
	Add one character to the buffer without exceeding its size.
=================================================================================*/
static void csvAddChar (int ch, char* buffer, int maxBufferSize, int* fieldLength)
{
	if ((*fieldLength)+1 < maxBufferSize) {
		buffer [(*fieldLength)] = (char)ch;
		buffer [(*fieldLength)+1] = '\0';
	}
	(*fieldLength)++;
}


/*=================================================================================
	Extracts one token from a CSV file. See csv.h
=================================================================================*/
int csvReadToken (char* buffer, int maxBufferSize, int* fieldLength, char separator, ReadCharFunc readCharFunc, void* readCharFuncParameter)
{
	int ch;

	/*------------------------------------------------------------------------------
		STATE MACHINE
		The state machine expects the first character in the stream to be
		the first character of the field.
		
		State values are:
	------------------------------------------------------------------------------*/
	enum {
		WaitingForFirstChar,   /* Waiting for the first character. */
		ReadingUnquoted,       /* Reading a string with unquoted rules */
		ReadingQuoted,         /* First character was a quote, reading the internal characters */
		ReadInnerQuote,        /* Read a QUOTE ("): next character can be another quote or a record separator (SEPARATOR, CR or EOF) */
		WaitingForLF           /* Read CR, next character must be LF */
	} state = WaitingForFirstChar;

	(*fieldLength) = 0;

	/* Prepare the return string as empty */
	if (maxBufferSize > 0) buffer [0] = '\0';

	for (;;) {
		/* Fetch one character */
		ch = readCharFunc (readCharFuncParameter);

		/* Verify the returned character is in range */
		if (ch != csvEOF && (ch < 0x00 || ch > 0xFF)) return CSVRET_INVALID_CH;

		switch (state) {
			/* Waiting for the first character. */
			case WaitingForFirstChar: {
				switch (ch) {
					case csvEOF: return CSVRET_EOF;
					case '\r': state = WaitingForLF; break;
					/* Empty string */
					case csvQUOT: state = ReadingQuoted; break;
					default: {
						if (ch == separator) {
							return CSVRET_OK;
						}
						csvAddChar (ch, buffer, maxBufferSize, fieldLength); state = ReadingUnquoted;
					}
				}
				break;
			}

			/* Reading a string with unquoted rules */
			case ReadingUnquoted: {
				switch (ch) {
					case csvEOF: return CSVRET_EOF;
					case '\r': state = WaitingForLF; break;
					case csvQUOT: return CSVRET_UNEXP_QUOT;
					default: {
						if (ch == separator) return CSVRET_OK;
						csvAddChar (ch, buffer, maxBufferSize, fieldLength);
					}
				}
				break;
			}

			/* First character was a quote, reading the internal characters */
			case ReadingQuoted: {
				switch (ch) {
					case csvEOF: return CSVRET_UNEXP_EOF;
					case csvQUOT: state = ReadInnerQuote; break;
					default: csvAddChar (ch, buffer, maxBufferSize, fieldLength);
				}
				break;
			}

			/* Read a QUOTE ("): next character can be another quote or a record separator (SEPARATOR, CR or EOF) */
			case ReadInnerQuote: {
				switch (ch) {
					case csvQUOT: csvAddChar (ch, buffer, maxBufferSize, fieldLength); state = ReadingQuoted; break;
					case '\r': state = WaitingForLF; break;
					case csvEOF: return CSVRET_EOF;
					default: {
						if (ch == separator) return CSVRET_OK;
						return CSVRET_INVALID_POST_QUOT;
					}
				}
				break;
			}

			case WaitingForLF: {
				switch (ch) {
					case '\n': return CSVRET_EOL;
					default: return CSVRET_MISSING_LF;
				}
				break;
			}
		}
	}
}

/*=================================================================================
	Writes the given buffer to the output stream. See csv.h.
=================================================================================*/
int csvWriteToken (const char* buffer, int bufferLength, WriteBinaryFunc writeBinaryFunc, void* writeBinaryFuncParameter)
{
	int i;
	char ch;

	/* True if the string needs quoting */
	int needsQuoting = 0;
	if (bufferLength < 0) bufferLength = strlen (buffer);

	#define WRCH(c) {ch = c; if (writeBinaryFunc (writeBinaryFuncParameter, &ch, 1)) return CSVRET_WRITE_ERROR;}

	for (i=0; i<bufferLength; i++) {
		if (!
			(
				(buffer[i] >= 'A' && buffer[i] <= 'Z')
				||
				(buffer[i] >= 'a' && buffer[i] <= 'z')
				||
				(buffer[i] >= '0' && buffer[i] <= '9')
				||
				(buffer[i] >= '_')
			)
		) {needsQuoting = 1; break;}
	}

	if (needsQuoting) {
		WRCH('"');
		for (i=0; i<bufferLength; i++) {
			if (buffer[i] == '"') WRCH('"');
			WRCH(buffer[i]);
		}
		WRCH('"');
	}
	else {
		if (writeBinaryFunc (writeBinaryFuncParameter, buffer, bufferLength)) return CSVRET_WRITE_ERROR;
	}
	
	return CSVRET_OK;
}

/*=================================================================================
	Same as csvWriteToken, but it writes an unsigned.
=================================================================================*/
int csvWriteTokenUnsigned (unsigned value, WriteBinaryFunc writeBinaryFunc, void* writeBinaryFuncParameter)
{
	char buffer [32];
	sprintf (buffer, "%u", value);
	return csvWriteToken (buffer, -1, writeBinaryFunc, writeBinaryFuncParameter);
}


/*=================================================================================
	Writes a separator (i.e. a comma or whatever 'separator' is).
	Parameters and return values are the same as "csvWriteToken".
=================================================================================*/
int csvWriteSeparator (char separator, WriteBinaryFunc writeBinaryFunc, void* writeBinaryFuncParameter)
{
	if (writeBinaryFunc (writeBinaryFuncParameter, &separator, 1)) return CSVRET_WRITE_ERROR;
	return CSVRET_OK;
}

/*=================================================================================
	Writes a CR-LF (newline) sequence.
	Parameters and return values are the same as "csvWriteToken".
=================================================================================*/
int csvWriteEndOfLine (WriteBinaryFunc writeBinaryFunc, void* writeBinaryFuncParameter)
{
	if (writeBinaryFunc (writeBinaryFuncParameter, "\r\n", 2)) return CSVRET_WRITE_ERROR;
	return CSVRET_OK;
}

/*=================================================================================
	Same as csvWriteToken, but it writes an UNICODE string zero terminated
=================================================================================*/
int csvWriteTokenUnicode (const t_unicode* unicodeString, WriteBinaryFunc writeBinaryFunc, void* writeBinaryFuncParameter)
{
	int i;
	int len;
	int ret;
	char* buffer;
	
	/* Calculate the length of the UNICODE string */
	for (len=0; unicodeString[len]; len++);
	
	/* Allocate the buffer */
	buffer = (char*)binAlloc(len+1);
	if (buffer == NULL) return CSVRET_OUT_OF_MEMORY;
	
	for (i=0; i<len; i++) {
		if (unicodeString [i] > 0 && unicodeString [i] <= 255) {
			buffer[i] = (char)(unsigned char)unicodeString [i];
		}
		else {
			buffer[i] = '?';
		}
	}
	buffer[i] = '\0';
	
	ret = csvWriteToken (buffer, len, writeBinaryFunc, writeBinaryFuncParameter);
	
	binFree (buffer);
	
	return ret;
}

#if 0
static void TEST_CSV ()
{
	char buffer [64];
	const char* csvName = "c:\\tmp\\prova.csv";
	int fieldLength;
	int ret;

	FILE* f = fopen (csvName, "rb");
	if (f == NULL) {
		fprintf (stderr, "Error opening file '%s': %s\n", csvName, strerror (errno));
		return;
	}

	do {
		ret = csvReadToken (buffer, sizeof(buffer), &fieldLength, ';', md380_FILEReadCharFunc, f);
		printf ("%-20s len=%4d [%s]\n", md380_CsvRetToString (ret), fieldLength, buffer);
	}
	while (ret == CSVRET_OK || ret == CSVRET_EOL);

	fclose (f);
}
#endif
