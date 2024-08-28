/******************************************************************************
 *   Copyright (C) 2010 by Ericsson AB
 *   S - 125 26  STOCKHOLM
 *   SWEDEN, tel int + 46 10 719 0000
 *
 *   The copyright to the computer program herein is the property of
 *   Ericsson AB. The program may be used and/or copied only with the
 *   written permission from Ericsson AB, or in accordance with the terms
 *   and conditions stipulated in the agreement/contract under which the
 *   program has been supplied.
 *
 *   All rights reserved.
 *
 *
 *   File:   Utils.h
 *
 *   Author: eaparob, uabjoy
 *
 *   Date:   2014-05-13
 *
 *   This file defines utility functions for use within ComSa
 *
 *****************************************************************************/

#ifndef utils_h
#define utils_h

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <algorithm>
#include <errno.h>
#include "ComSA.h"
#include "OsCommand.h"
#include "UtilsInterface.h"

class Utils
{
public:
	static bool createComSaTraceDirectory();
	static int osCommand(const std::string &command, std::vector<std::string> &result);
	static bool directoryExists(std::string path);
	static bool createDirectory(std::string path);
	static int processOpen(const char *command);
	static char* convertToLowerCase(const char* word);
	static char* convertToUpperCase(const char* word);
private:
	Utils();
	virtual ~Utils();
};

#endif /* utils_h */
