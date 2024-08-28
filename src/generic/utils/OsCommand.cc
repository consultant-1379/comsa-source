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

#include <string.h>
#include "OsCommand.h"
#include <iostream>
#include <stdio.h>
#include <errno.h>
#include <sys/wait.h>
#include "ComSA.h"
#include "trace.h"

OsCommand::OsCommand()
{
	ENTER_MWSA_GENERAL();
	DEBUG_MWSA_GENERAL("OsCommand::OsCommand constructor");
	LEAVE_MWSA_GENERAL();
}

OsCommand::~OsCommand()
{
	ENTER_MWSA_GENERAL();
	DEBUG_MWSA_GENERAL("OsCommand::~OsCommand destructor");
	LEAVE_MWSA_GENERAL();
}

int OsCommand::executeOsCommand(const std::string &command, std::vector<std::string> &result)
{
	ENTER_MWSA_GENERAL();
	FILE *fpipe;
	const int LINE_SIZE = 1024;
	char line[LINE_SIZE];
	result = std::vector<std::string>();

	DEBUG_MWSA_GENERAL("OsCommand::executeOsCommand: command: %s", command.c_str());

	if ( !(fpipe = (FILE*)popen(command.c_str(), "r")) )
	  {
		ERR ("OsCommand::executeOsCommand popen failed: %s", strerror(errno));
		LEAVE_MWSA_GENERAL();
		return errno;
	}
	#ifdef _TRACE_FLAG
	int fd = fileno(fpipe);
	#else
	fileno(fpipe);
	#endif
	DEBUG_MWSA_GENERAL("OsCommand::executeOsCommand Open file desc = %d", fd);
	std::string str;
	while (fgets(line, sizeof(line), fpipe))
	{
		str = line;
		if (str[str.size() - 1] != '\n')
		{
		  ERR ("OsCommand::executeOsCommand fgets failed, returns, line=%s", line);
		  LEAVE_MWSA_GENERAL();
		  return 1;
		}
		std::string res = str.substr(0, str.size() -1);
		DEBUG_MWSA_GENERAL("OsCommand::executeOsCommand(): output line: \"%s\"", res.c_str());
		result.push_back(res);
	}

	int resCode;
	if ((resCode = pclose(fpipe)) == -1)
	{
		ERR_MWSA_GENERAL("OsCommand::executeOsCommand pclose failed: %s", strerror(errno));
		LEAVE_MWSA_GENERAL();
		return errno;
	}
	DEBUG_MWSA_GENERAL("OsCommand::executeOsCommand Closed file desc = %d", fd);
	DEBUG_MWSA_GENERAL("OsCommand::executeOsCommand return %d", WEXITSTATUS(resCode));
	LEAVE_MWSA_GENERAL();
	return WEXITSTATUS(resCode);
}
