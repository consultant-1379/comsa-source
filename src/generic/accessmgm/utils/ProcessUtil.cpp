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
* Author: erafodz
* Reviewed: epkadsz
*/

#include <string.h>
#include "ProcessUtil.h"
#include <iostream>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>
#include "ComSA.h"
#include "trace.h"
#define TRACEPOINT_DEFINE
#define TRACEPOINT_PROBE_DYNAMIC_LINKAGE

namespace aaservice
{

    int ProcessUtil::childProcessId = 0;

	ProcessUtil::ProcessUtil()
	{
	  DEBUG_MWSA_GENERAL ("ProcessUtil::ProcessUtil constructor");
	}


	ProcessUtil::~ProcessUtil()
	{
	  DEBUG_MWSA_GENERAL ("ProcessUtil::~ProcessUtil destructor");
	}


	int ProcessUtil::run(const std::string &command, std::vector<std::string> &result)
	{
	  ENTER_MWSA_GENERAL();
		FILE *fpipe;
		const int LINE_SIZE = 1024;
		char line[LINE_SIZE];
		result = std::vector<std::string>();

		if ( !(fpipe = (FILE*)popen(command.c_str(), "r")) )
		  {
		    ERR_MWSA_GENERAL ("ProcessUtil::run popen failed: %s", strerror(errno));
		    return errno;
		}
		#ifdef _TRACE_FLAG
		int fd = fileno(fpipe);
		#else
		fileno(fpipe);
		#endif
		DEBUG_MWSA_GENERAL("Open file desc = %d", fd);
		std::string str;
		while (fgets(line, sizeof(line), fpipe))
		{
			str = line;
			if (str[str.size() - 1] != '\n')
			{
			  ERR_MWSA_GENERAL("ProcessUtil::run fgets failed, returns, line=%s", line);
			  LEAVE_MWSA_GENERAL();
			  return 1;
			}
			assert (str[str.size() - 1] == '\n');
			result.push_back(str.substr(0, str.size() -1));
		}

		int resCode;
		if ((resCode = pclose(fpipe)) == -1)
		{
		    ERR_MWSA_GENERAL("ProcessUtil::run pclose failed: %s", strerror(errno));
			return errno;
		}
		DEBUG_MWSA_GENERAL("Closed file desc = %d", fd);
		DEBUG_MWSA_GENERAL("ProcessUtil::run return %d", WEXITSTATUS(resCode));
		LEAVE_MWSA_GENERAL();
		return WEXITSTATUS(resCode);
	}

	void ProcessUtil::run(const std::string &command)
	{
		pid_t pid, wpid;
		int status;

		pid = fork();
		if(pid < 0) {

			ERR_MWSA_GENERAL("ProcessUtil::run Could not run command '%s', fork() failed with error: '%s'", command.c_str(), strerror(errno));

		}
		else if (pid == 0)//child process
		{
			int retValue = execl("/bin/sh", "/bin/sh", "-c", command.c_str(), (char*)0) ;

			ERR_MWSA_GENERAL("ProcessUtil::run command execution failed ret= %d, err: %d", retValue, errno);
			_exit(errno);

		} else if (pid > 0) { //parent process

			childProcessId = pid;
			DEBUG_MWSA_GENERAL("ProcessUtil::run In parent process,  child pid is: %d", pid);

			int retValue = setpgid(pid, 0);
			if (retValue == -1) {
				ERR_MWSA_GENERAL("ProcessUtil::run setpgid failed with errno %s", strerror(errno));
			}

			wpid = waitpid(pid, &status, 0);

			if(wpid == -1) {
				ERR_MWSA_GENERAL("ProcessUtil::run waitpid() failed");
			}
			//Check whether child has exited normally or with some signals
			if(WIFEXITED(status))
			{
				if(!WEXITSTATUS(status)) {
					DEBUG_MWSA_GENERAL("ProcessUtil::run child exited with success status: %d", WEXITSTATUS(status));
				} else {
					DEBUG_MWSA_GENERAL("ProcessUtil::run child exited with non-zero status: %d", WEXITSTATUS(status));
				}
			} else if(WIFSIGNALED(status)) {
				DEBUG_MWSA_GENERAL("ProcessUtil::run child process exited with signal %d", WTERMSIG(status));
			} else {
				DEBUG_MWSA_GENERAL("ProcessUtil::run abnormal exit of child process");
			}
			childProcessId = 0;
		}
	}
}
