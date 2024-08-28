/*
 * MockProcessUtil.h
 *
 *  Created on: Sep 16, 2011
 *      Author: uabjoy
 */
#ifndef MOCK_H_
#define MOCK_H_

#include <string>
#include <vector>
#include "ProcessUtil.h"


namespace aaservice
{


class MockProcessUtil {
public:
	std::string runResult;
	int retVal;
	std::string executedCommand;


	MockProcessUtil(std::string &res, int returnVal): runResult(res), retVal(returnVal) {
		executedCommand = "";
	}

	int run(std::string command, std::vector<std::string> &result) {
		executedCommand += command;
		result.push_back(runResult);
		return retVal;
	}

	void run(std::string command) {
		executedCommand += command;
	}

	const char *getExecutedCommand () {
		return executedCommand.c_str();
	}
};


}

extern aaservice::MockProcessUtil *mymockProcessUtil;



#endif /* MOCK_H_ */
