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
#ifndef __CSV_H
#define __CSV_H
#include "binlib.h"

#ifdef __cplusplus
	extern "C" {
#endif

typedef int (*ReadCharFunc)(void*);

typedef int (*WriteBinaryFunc)(void*, const char*, unsigned);

#ifndef csvEOF
#define csvEOF  (-1)
#endif

#ifndef csvQUOT
#define csvQUOT '"'
#endif

#define CSVRET_OK                (0)
#define	CSVRET_EOF               (-10)
#define	CSVRET_EOL               (-11)
#define	CSVRET_UNEXP_QUOT        (-12)
#define CSVRET_MISSING_LF        (-13)
#define CSVRET_INVALID_CH        (-14)
#define	CSVRET_UNEXP_EOF         (-15)
#define CSVRET_INVALID_POST_QUOT (-16)
#define CSVRET_WRITE_ERROR       (-17)
#define CSVRET_OUT_OF_MEMORY     (-18)

/*=================================================================================
	Extracts one token from a CSV file complying with rfc4180.
	It fills the buffer and it returns the length in characters of the token.
	
	Parameters:

	  buffer                the target buffer; it must be allocated of at least
		                      "maxBufferSize" bytes; the function will write an
													ASCIIZ string terminated by "\0" without ever exceed
                          "maxBufferSize" bytes. So the longest string that can
													be read is "maxBufferSize-1" characters long.

    maxBufferSize         See "buffer".

		fieldLength           This output parameter returns the actual length in 
		                      bytes of the field. The returned value does not include the 
													trailing zero. So, if the field is "ABC" the returned value 
													will be 3 but "buffer" will have to be sized to at least 
													"maxBufferSize==4" to host it completely.	If the buffer 
													is not big enough, it will be filled with as many characters 
													it can host and the remaining will be discarded. This event 
													can be detected	by comparing the return value and "maxBufferSize".

		separator             Separator character (for example, ',')

		readCharFunc          Pointer to a function with the following prototype:

		                         int f(void* p)

												  The function must use "p" to remember the status; it
													must read the next character and return a value
													between 0 and 255; in case of end of file, it must
													return "csvEOF".
													The parameter "readCharFuncParameter" is passed as
													"p" to the function.

    readCharFuncParameter See "readCharFunc".


	RETURN VALUES

	The function returns one among the following values:

	CSVRET_OK         The function read a field

	CSVRET_EOF        The function detected an END OF FILE

	CSVRET_EOL        The function detected an END OF LINE

	CSVRET_UNEXP_QUOT The function detected a quote inside the field (for
	                  example: [...,My monitor is 12" wide,...] the '"' is in the
										middle of the string. 
										It should have been: [...,"My monitor is 12"" wide",...]

	CSVRET_MISSING_LF RFC4180 requires the lines to be terminated with CR-LF. If
	                  CR is found without LF, this error is returned.

	CSVRET_INVALID_CH Function "readCharFunc" returned an invalid character.

	CSVRET_UNEXP_EOF  Found an end-of-file inside a quoted string (missing closed quote)

	CSVRET_INVALID_POST_QUOT   After a quoted string, a invalid character occured
	                  (it expects only a separator, a EOL or a EOF).
=================================================================================*/
extern int csvReadToken (char* buffer, int maxBufferSize, int* fieldLength, char separator, ReadCharFunc readCharFunc, void* readCharFuncParameter);

/*=================================================================================
	Writes the given buffer to the output stream.

	  buffer                the source buffer; it can contain any range of characters
		                      including '\0'.

    bufferLength          number of bytes to be written from buffer. If <0, it
		                      will use "strlen".

		writeBinaryFunc       Pointer to a function with the following prototype:

		                         int f(void* p, const char* data, unsigned len)

												  The function must use "p" to remember the status; it
													must write "len" bytes from "data"; it must return
													0 if ok, != in case of error.
													The parameter "writeBinaryFuncParameter" is passed as
													"p" to the function.

	RETURN VALUES

	The function returns one among the following values:

	CSVRET_OK               The function read a field

	CSVRET_WRITE_ERROR      The "writeBinaryFunc" reported an error.

=================================================================================*/
extern int csvWriteToken (const char* buffer, int bufferLength, WriteBinaryFunc writeBinaryFunc, void* writeBinaryFuncParameter);

/*=================================================================================
	Same as csvWriteToken, but it writes an unsigned.
=================================================================================*/
extern int csvWriteTokenUnsigned (unsigned value, WriteBinaryFunc writeBinaryFunc, void* writeBinaryFuncParameter);

/*=================================================================================
	Same as csvWriteToken, but it writes an UNICODE string zero terminated.
	Returns the same values of csvWriteToken, plus CSVRET_OUT_OF_MEMORY.
=================================================================================*/
extern int csvWriteTokenUnicode (const t_unicode* unicodeString, WriteBinaryFunc writeBinaryFunc, void* writeBinaryFuncParameter);

/*=================================================================================
	Writes a separator (i.e. a comma or whatever 'separator' is).
	Parameters and return values are the same as "csvWriteToken".
=================================================================================*/
extern int csvWriteSeparator (char separator, WriteBinaryFunc writeBinaryFunc, void* writeBinaryFuncParameter);

/*=================================================================================
	Writes a CR-LF (newline) sequence.
	Parameters and return values are the same as "csvWriteToken".
=================================================================================*/
extern int csvWriteEndOfLine (WriteBinaryFunc writeBinaryFunc, void* writeBinaryFuncParameter);


#ifdef  __cplusplus
}
#endif

#endif
