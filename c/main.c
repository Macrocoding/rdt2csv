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
#include "md380.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <assert.h>
#include "csv.h"
#include "version.h"
#include "crc.h"
#include "md380_tools.h"
#include "md380_file.h"
#include "md380_cli.h"
#include "md380_valid.h"
#include "md380_tables.h"
#include "md380_resolve.h"

/*============================================================================
	LOAD A .RDT FILE
============================================================================*/
t_buffer* loadRdtFile (const char* fileName, unsigned* offset, unsigned* length)
{
	t_buffer* rdtBinFile = NULL;
	struct stat st;
	size_t readSize;
	FILE* f = NULL;
	(*offset) = 0;
	(*length) = FILE_SIZE_TRDTFile;

	if (stat (fileName, &st)) {
		fprintf (stderr, "Error opening file (stat) '%s': %s\n", fileName, strerror(errno));
		goto errorExit;
	}

	/* Size must be exact */
	if (st.st_size != FILE_SIZE_TRDTFile) {
		if (st.st_size != FILE_SIZE_TBINFile) {
			fprintf (stderr, "Error in file '%s': size %u does not match expected size of %u (.rdt) or %u octets (.bin/.img)\n", fileName, (unsigned)st.st_size, FILE_SIZE_TRDTFile, FILE_SIZE_TBINFile);
		goto errorExit;
	}
		else {
			(*offset) = FILE_OFST_TBINFile;
			(*length) = FILE_SIZE_TBINFile;
		}
	}

	f = fopen (fileName, "rb");
	if (f == NULL) {
		fprintf (stderr, "Error opening file (open) '%s': %s\n", fileName, strerror(errno));
		goto errorExit;
	}

rdtBinFile = (t_buffer*)malloc (FILE_SIZE_TRDTFile);
	rdtBinFile = binAlloc (FILE_SIZE_TRDTFile);
	if (rdtBinFile == NULL) {
		fprintf (stderr, "Error allocating %u octets for .RDT file (out of memory?)\n", FILE_SIZE_TRDTFile);
		goto errorExit;
	}
	memset (rdtBinFile, 0, FILE_SIZE_TRDTFile);

	readSize = fread (rdtBinFile+(*offset), 1, (*length), f);
	if (readSize != (*length)) {
		fprintf (stderr, "Error reading file '%s': expected %u bytes, got %u bytes\n", fileName, (unsigned)(*length), (unsigned)readSize);
		goto errorExit;
	}
		
	if (f) fclose (f);
	return rdtBinFile;

errorExit:
	if (f) fclose (f);
	if (rdtBinFile) binFree (rdtBinFile);
	return NULL;
}

#define CLI_SC  0x1F7D1676  /* -sc  */
#define CLI_TAB 0xFFBE32E1  /* -tab */
#define CLI_U   0x419CCDA3  /* -u   */
#define CLI_E   0x5C2BDDC7  /* -e   */
#define CLI_QM  0xD795652D  /* -?   */
#define CLI_H   0x229AA17A  /* -h   */

/*============================================================================
	Show command line help
============================================================================*/
void showCommandHelp ()
{
	int i;
	printf ("USAGE: rdt2csv [-e|-u] <file.rdt/.img/.bin> [-sc|-tab] <csv-files>\n");
	printf ("\n");
	printf ("    -e      export .rdt file to listed .csv files\n");
	printf ("    -u      update .rdt file from listed .csv files\n");
	printf ("    -sc     use semicolon (;) as CSV separator instead of comma\n");
	printf ("    -tab    use tab as CSV separator instead of comma\n");
	printf ("\n<csv-files>:\n");
	
	for (i=0; i<NO_OF_CLI_COMMANDS; i++) {
		printf ("    %-7s .csv filename for %s\n", cliCommands[i].parameter, cliCommands[i].recordName);
	}
	printf ("\nThis program can be freely redistributed.\n");
}

/*============================================================================
	ANALIZE COMMAND LINE
	Returns 0=ok  non-zero=error
============================================================================*/
int analyzeCommandLine (int argc, char** argv, MD380_Configuration* config)
{
	char** initialArgv = argv;
	#define PARNO (argv-initialArgv+1)
	if (argc == 0) {showCommandHelp (); return 1;}
	
	while (argc > 0) {
		unsigned argCrc = crc32_AddAsciizLowerCase (0, *argv);
		switch (argCrc) {
			/*============================
				HELP
			============================*/
			case CLI_QM:
			case CLI_H: showCommandHelp (); return 1;
			
			/*============================
				READ SEPARATORS 
			============================*/
			case CLI_SC:
			case CLI_TAB: {
				if (config->separator != MD380_DEFAULT_SEPARATOR) {
					fprintf (stderr, "Error in parameter %d (%s): separator already defined on a previous parameter with -sc or -tab\n", (int)PARNO, *argv);
					return 1;
				}
				switch (argCrc) {
					case CLI_SC: config->separator = ';'; break;
					case CLI_TAB: config->separator = '\t'; break;
				}
				break;
			}
			/* READ ACTION COMMAND */
			case CLI_U:
			case CLI_E: {
				if (config->updateMode != modeUnset) {
					fprintf (stderr, "Error in parameter %d (%s): %s already defined in previous parameter\n", (int)PARNO, *argv, (config->updateMode == modeExport ? "-e" : "-u"));
					return 1;
				}
				if (argc <= 1) {
					fprintf (stderr, "Error in parameter %d (%s): missing .rdt file name\n", (int)PARNO, *argv);
					return 1;
				}
				/* Fetch RDT file name */
				argc--;
				argv++;
				assert (config->rdtFileName == NULL);
				config->rdtFileName = binAlloc (strlen (*argv)+1);
				if (config->rdtFileName == NULL) {
					fprintf (stderr, "Error in parameter %d (%s): out of memory\n", (int)PARNO, *argv);
					return 1;
				}
				strcpy (config->rdtFileName, *argv);
				switch (argCrc) {
					case CLI_U: config->updateMode = modeUpdate; break;
					case CLI_E: config->updateMode = modeExport; break;
				}
				break;
			}
			/* READ CSV FILE NAMES */
			default: {
				int ret;
				char* err = NULL;
				
				ret = ParseCommandLine (&config->csvFileNames, argv, argc);
				switch (ret) {
					case CLI_PARAMETER_UNKNOWN: err = "parameter unknown"; break;
					case CLI_NOT_ENOUGH_PARAMS: err = "missing .csv file name"; break;
					case CLI_DUPE_PARAMETER   : err = ".csv file name already defined"; break;
					case CLI_OUT_OF_MEMORY    : err = "out of memory"; break;
					default: {
						assert (ret > 0);
					}
				}
				if (err) {
					fprintf (stderr, "Error in parameter %d (%s): %s\n", (int)PARNO, *argv, err);
					return 1;
				}
				argc-= (ret-1);
				argv+= (ret-1);
			}
		}
		argc--;
		argv++;
	}
	
	return 0;
}

/*============================================================================
	Report violation
============================================================================*/
void ReportViolationFunc (void* param, const char* recordType, int recordNumber, const char* fieldName, const char* text)
{
	fprintf ((FILE*)param, "Record %s, ", recordType);
	if (recordNumber >= 0) {
		fprintf ((FILE*)param, "line %d, ", (int)(recordNumber+1));
	}
	fprintf ((FILE*)param, "field %s, violation: %s\n", fieldName, text);
}

#if 0
void testTone (unsigned char ch1, unsigned char ch2, t_numeric value)
{
	unsigned char bin1 [2];
	unsigned char bin2 [2];
	t_numeric n;
	
	bin1[0] = ch1;
	bin1[1] = ch2;
	n = BCDToNumericForTones (bin1, 0);
	assert (n == value);
	numericToBCDForTones (bin2, 0, n);
	assert (bin1[0] == bin2[0]);
	assert (bin1[1] == bin2[1]);
}

int main (int argc, char* argv[])
{
	testTone (0x70, 0x07, 770);
	testTone (0x23, 0x80, 0x20000 | 23);
	testTone (0x23, 0xC0, 0x30000 | 23);
	return 0;
}
#endif
/*============================================================================
	MAIN
============================================================================*/
int main (int argc, char* argv[])
{
	t_buffer* rdtBinFile = NULL;
	TRDTFile* container = NULL;
	int ret = 0;
	MD380_Configuration config;
	char* errorMessage = NULL;
	MD380Tables md380tables;
	int noOfViolations=0;
	unsigned offset, length;
	
	/* Print copyright information */	
	fprintf (stderr, "rdt2csv r.%u - (c)%s by Davide Achilli IZ2UUF - iz2uuf@iz2uuf.net\n", SUBVERSION_RELEASE_N, LATEST_COMPILATION_YEAR);

	INIT_MD380_Configuration (&config);
	INIT_MD380Tables (&md380tables);

#ifndef NDEBUG
	TEST_LOOKUP ();
	runBinlibTest ();
#endif

	ret = analyzeCommandLine (argc-1, argv+1, &config);
	if (ret) goto exitMain;

	/*--------------------------------------------------------------
		The rdt file name must be available
	--------------------------------------------------------------*/
	if (config.rdtFileName == NULL) {
		fprintf (stderr, "Error, no .rdt file specified (specify either -e or -u)\n");
		ret = 1;
		goto exitMain;
	}

	/*--------------------------------------------------------------
		Alloc the container and set it to zero
	--------------------------------------------------------------*/
	container = (TRDTFile*)binAlloc (sizeof (TRDTFile));
	if (container == NULL) {
		fprintf (stderr, "Error allocating %u octets for internal container (out of memory?)\n", (unsigned)sizeof (TRDTFile));
		ret = 2;
		goto exitMain;
	}
	memset (container, 0, sizeof (TRDTFile));

	/*--------------------------------------------------------------
		Load the RDT file
	--------------------------------------------------------------*/
	rdtBinFile = loadRdtFile (config.rdtFileName, &offset, &length);
	if (rdtBinFile == NULL) {ret = 1; goto exitMain;}

	decodeBinary_TRDTFile (rdtBinFile, container);

	/*--------------------------------------------------------------
		Validate the RDT file
	--------------------------------------------------------------*/
	noOfViolations = validateContainer (container, ReportViolationFunc, stderr);
	noOfViolations += registerContainerNames (&md380tables, container, ReportViolationFunc, stderr);
	noOfViolations += bindReferences (container, ReportViolationFunc, stderr);
	if (noOfViolations) {
		fprintf (stderr, "Found %d rules violation(s) in input file '%s'\n", noOfViolations, config.rdtFileName);
		goto exitMain;
	}

	/*--------------------------------------------------------------
		If specified on command line, save the CSV files
	--------------------------------------------------------------*/
	if (config.updateMode == modeExport) {
		int saveRet = saveCSVFileAll (&config.csvFileNames, container, config.separator, &errorMessage);
		if (errorMessage) {
			fprintf (stderr, "ERROR: %s\n", errorMessage);
			binFree (errorMessage);
			errorMessage = NULL;
		}
		if (saveRet != CSVRET_OK) {
			ret = 1;
			goto exitMain;
		}
	}

	/*--------------------------------------------------------------
		If specified on command line, save the RDT file
	--------------------------------------------------------------*/
	if (config.updateMode == modeUpdate) {
		FILE* f;
		int loadRet;
		
		/* Load the CSV files */
		loadRet = loadCSVFileAll (&config.csvFileNames, container, config.separator, &errorMessage);
		if (errorMessage) {
			fprintf (stderr, "ERROR: %s\n", errorMessage);
			binFree (errorMessage);
			errorMessage = NULL;
		}
		if (loadRet != CSVRET_OK) {
			ret = 1;
			goto exitMain;
		}

		/* Validate the file */
		noOfViolations = validateContainer (container, ReportViolationFunc, stderr);
		noOfViolations += registerContainerNames (&md380tables, container, ReportViolationFunc, stderr);
		noOfViolations += resolveReferences (&md380tables, container, ReportViolationFunc, stderr);
		if (noOfViolations) {
			fprintf (stderr, "File not saved due to %d rules violation(s)\n", noOfViolations);
			goto exitMain;
		}
		
		/* Transfer the container to the binary file */
		encodeBinary_TRDTFile (rdtBinFile, container);

		/* Save the RDT file */
		f = fopen (config.rdtFileName, "wb");
		if (f == NULL) {
			fprintf (stderr, "Error opening '%s' for writing (%s)\n", config.rdtFileName, strerror(errno));
			ret = 4;
			goto exitMain;
		}
		encodeBinary_TRDTFile (rdtBinFile, container);
		fwrite (rdtBinFile+offset, 1, length, f);
		fclose (f);
		fprintf (stderr, "File '%s' updated\n", config.rdtFileName);
	}

exitMain:
	FREE_MD380Tables (&md380tables);
	FREE_MD380_Configuration (&config);
	if (rdtBinFile) binFree (rdtBinFile);
	if (container) binFree (container);

	#ifdef ED_DEBUG_ALLOC
	EDDebugPrintSummary ();
	EDDebugCheckMemory();
	#endif

	return ret;
}
