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

#ifndef PROCESSUTIL_H_
#define PROCESSUTIL_H_
#include <stdlib.h>
#include <string>
#include <vector>

class OsCommand
{
public:
	static int executeOsCommand(const std::string &command, std::vector<std::string> &result);
private:
	OsCommand();
	virtual ~OsCommand();
};
#endif /* PROCESSUTIL_H_ */
