/*
* Copyright (C) 2010 by Ericsson AB
* S - 125 26  STOCKHOLM
* SWEDEN, tel int + 46 10 719 0000
*
* The copyright to the computer program herein is the property of
* Ericsson AB. The program may be used and/or copied only with the
* written permission from Ericsson AB, or in accordance with the terms
* and conditions stipulated in the agreement/contract under which the
* program has been supplied.
*
* All rights reserved.
*
* Date: 2014-05-13
*
* Author: eaparob, uabjoy
*
*/

#include "Utils.h"

#ifndef UNIT_TEST
char* _CC_NAME =  NULL;
char* _CC_NAME_UPPERCASE = NULL;
char* _CC_NAME_SA = NULL;
#else
char* _CC_NAME = "com";
char* _CC_NAME_UPPERCASE = "COM";
char* _CC_NAME_SA = "COM_SA";
#endif

Utils::Utils()
{
	ENTER_MWSA_GENERAL();
	DEBUG_MWSA_GENERAL("Utils::Utils constructor");
	LEAVE_MWSA_GENERAL();
}

Utils::~Utils()
{
	ENTER_MWSA_GENERAL();
	DEBUG_MWSA_GENERAL("Utils::~Utils destructor");
	LEAVE_MWSA_GENERAL();
}

int Utils::osCommand(const std::string &command, std::vector<std::string> &result)
{
	ENTER_MWSA_GENERAL();
	int ret = OsCommand::executeOsCommand(command, result);
	LEAVE();
	return ret;
}

bool Utils::directoryExists(std::string directoryPath)
{
	ENTER_MWSA_GENERAL();
	bool retVal = false;
	std::vector<std::string> result;
	std::string command = "ls " + directoryPath;
	DEBUG_MWSA_GENERAL("Utils::directoryExists: execute command \"%s\"",command.c_str());
	if(OsCommand::executeOsCommand(command, result) == 0)
	{
		DEBUG_MWSA_GENERAL("Utils::directoryExists: return true");
		retVal = true;
	}
	else
	{
		DEBUG_MWSA_GENERAL("Utils::directoryExists: return false");
		retVal = false;
	}
	LEAVE_MWSA_GENERAL();
	return retVal;
}

bool Utils::createDirectory(std::string directoryPath)
{
	ENTER_MWSA_GENERAL();
	bool retVal = false;
	std::vector<std::string> result;
	std::string command = "mkdir " + directoryPath;
	int ret = OsCommand::executeOsCommand(command, result);
	DEBUG_MWSA_GENERAL("Utils::createDirectory: OS command returned (%d)",ret);
	LEAVE_MWSA_GENERAL();
	return retVal;

}
bool Utils::createComSaTraceDirectory()
{
	ENTER_MWSA_GENERAL();
	bool retVal = true;

	std::string comsaTraceDirectoryPath = "/var/opt/";
	comsaTraceDirectoryPath += (const char*)_CC_NAME;
	comsaTraceDirectoryPath += "sa";

	DEBUG_MWSA_GENERAL("Utils::createSaTraceDirectory: %s", comsaTraceDirectoryPath.c_str());
	if(!directoryExists(comsaTraceDirectoryPath))
	{
		createDirectory(comsaTraceDirectoryPath);
	}
	LEAVE_MWSA_GENERAL();
	return retVal;
}

int Utils::processOpen(const char *command)
{
        ENTER_MWSA_GENERAL();
        std::vector<std::string> result;
        DEBUG_MWSA_GENERAL("Utils::processOpen: execute command \"%s\"",command);
        int rc = OsCommand::executeOsCommand(command, result);
        LEAVE_MWSA_GENERAL();
        return rc;
}

char* Utils::convertToLowerCase(const char* word)
{
        if (NULL == word)
        {
                return NULL;
        }

        char* ret = strdup(word);
        int i;
        for(i = 0; word[i] != '\0'; i++)
        {
                 ret[i] = tolower(word[i]);
        }

        return ret;
}

char* Utils::convertToUpperCase(const char* word)
{
        if (NULL == word)
        {
                return NULL;
        }

        char* ret = strdup(word);
        int i;
        for(i = 0; word[i] != '\0'; i++)
        {
                ret[i] = toupper(word[i]);
        }

        return ret;
}

// C interface functions
void createComSaTraceDirectory()
{
	ENTER_MWSA_GENERAL();
	Utils::createComSaTraceDirectory();
	LEAVE_MWSA_GENERAL();
}

int processOpen(const char *command)
{
	ENTER_MWSA_GENERAL();
	int rc = Utils::processOpen(command);
	LEAVE_MWSA_GENERAL();
	return rc;
}

char* convertToLowerCase(const char* word)
{
	ENTER_MWSA_GENERAL();
	char* res = Utils::convertToLowerCase(word);
	LEAVE_MWSA_GENERAL();
	return res;
}

char* convertToUpperCase(const char* word)
{
	ENTER_MWSA_GENERAL();
	char* res = Utils::convertToUpperCase(word);
	LEAVE_MWSA_GENERAL();
	return res;
}
