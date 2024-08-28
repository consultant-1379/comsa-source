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
* Author: erafodz
* Reviewed: epkadsz
*/

#ifndef PROCESSUTIL_H_
#define PROCESSUTIL_H_
#include <stdlib.h>
#include <string>
#include <vector>

namespace aaservice
{
	class ProcessUtil
	{
	public:
		ProcessUtil();
		virtual ~ProcessUtil();
		int run(const std::string &command, std::vector<std::string> &result);
		void run(const std::string &command);

		// Used to close the child process when the com(parent) is gracefully terminated
		static int childProcessId;
	};
}
#endif /* PROCESSUTIL_H_ */
