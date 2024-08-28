/*
 * MockProcessUtil.cpp
 *
 *  Created on: Sep 16, 2011
 *      Author: uabjoy
 */

#include "ProcessUtil.h"
#include "MockProcessUtil.h"

int aaservice::ProcessUtil::childProcessId = 0;

int aaservice::ProcessUtil::run(const std::string &command, std::vector<std::string> &result)
{
	return mymockProcessUtil->run(command, result);
}

void aaservice::ProcessUtil::run(const std::string &command)
{
	mymockProcessUtil->run(command);
}

aaservice::ProcessUtil::ProcessUtil() {

}

aaservice::ProcessUtil::~ProcessUtil() {

}


