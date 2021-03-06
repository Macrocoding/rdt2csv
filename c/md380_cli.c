/* Generated by Macrocoder - do not edit */
#define _CRT_SECURE_NO_WARNINGS
#include "md380_cli.h"
#include <string.h>
#include "binlib.h"
#include "crc.h"
#include <assert.h>

/* Initialize a CSVFileNames */
void INIT_CSVFileNames(CSVFileNames* fs)
{
	memset (fs, 0, sizeof(*fs));
}

/* Free a CSVFileNames */
void FREE_CSVFileNames(CSVFileNames* fs)
{
	if (fs->Path_ChannelInformation != NULL) binFree(fs->Path_ChannelInformation);
	if (fs->Path_DigitalContact != NULL) binFree(fs->Path_DigitalContact);
	if (fs->Path_DigitalRxGroupList != NULL) binFree(fs->Path_DigitalRxGroupList);
	if (fs->Path_GeneralSettings != NULL) binFree(fs->Path_GeneralSettings);
	if (fs->Path_ScanList != NULL) binFree(fs->Path_ScanList);
	if (fs->Path_TextMessage != NULL) binFree(fs->Path_TextMessage);
	if (fs->Path_ZoneInformation != NULL) binFree(fs->Path_ZoneInformation);

	memset (fs, 0, sizeof(*fs));
}

/* Executes the parsing of the command line. It returns the number */
/* of parameters consumed from the given position or CLI_xxx in case of error */
int ParseCommandLine (CSVFileNames* fs, char** argv, int argc)
{
	unsigned paramCrc;
	char** targetPtr = NULL;
	
	if (argc <= 0) return 0;
	if (strlen (argv[0]) <= 1) return 0;
	if (argv[0][0] != '-') return 0;
	paramCrc = crc32_AddAsciizLowerCase (0, argv[0]+1);
	
	switch (paramCrc) {
		/* '-ch' for ChannelInformation */
		case 0x4C60C3F1: targetPtr = &(fs->Path_ChannelInformation); break;
		/* '-cont' for DigitalContact */
		case 0xE74AF8E0: targetPtr = &(fs->Path_DigitalContact); break;
		/* '-rxgrp' for DigitalRxGroupList */
		case 0xEB2F7B2C: targetPtr = &(fs->Path_DigitalRxGroupList); break;
		/* '-gen' for GeneralSettings */
		case 0x0059D70A: targetPtr = &(fs->Path_GeneralSettings); break;
		/* '-scan' for ScanList */
		case 0xC4B3B3AE: targetPtr = &(fs->Path_ScanList); break;
		/* '-txt' for TextMessage */
		case 0x1C375F45: targetPtr = &(fs->Path_TextMessage); break;
		/* '-zone' for ZoneInformation */
		case 0xA0EBC007: targetPtr = &(fs->Path_ZoneInformation); break;

	}
	
	/* Check for dupe allocations */
	if ((*targetPtr) != NULL) {
		return CLI_DUPE_PARAMETER;
	}
	
	if (argc < 2) {
		return CLI_NOT_ENOUGH_PARAMS;
	}
	
	if (targetPtr) {
		(*targetPtr) = (char*)binAlloc (strlen (argv[1])+1);
		strcpy ((*targetPtr), argv[1]);
		
		return 2;
	}
	else {
		return CLI_PARAMETER_UNKNOWN;
	}
}

CliCommands cliCommands[NO_OF_CLI_COMMANDS] = {
	{"-ch", "ChannelInformation"},
	{"-cont", "DigitalContact"},
	{"-rxgrp", "DigitalRxGroupList"},
	{"-gen", "GeneralSettings"},
	{"-scan", "ScanList"},
	{"-txt", "TextMessage"},
	{"-zone", "ZoneInformation"}
};
