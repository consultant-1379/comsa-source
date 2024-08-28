 /******************************************************************************
 *   Copyright (C) 2014 by Ericsson AB
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
 * 	 File: debug_log.cc
 *
 *   Author: xadaleg
 *
 *   Date:   2014-12-12
 *
 *****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <list>
#include <vector>
#include <map>
#include <ctype.h>

#include <assert.h>

#ifdef UNIT_TEST
#include <gtest/gtest.h>
#endif

#include "saAis.h"
#include "ComSA.h"

////////////////////////////////////3//////////////////
// LOG
//////////////////////////////////////////////////////
#ifdef  __cplusplus
extern "C" {
#endif

#ifdef UNIT_TEST
#define LOG_PREFIX " "
static void coremw_vlog2(int priority, char const* fmt, va_list ap) {
	char buffer[MAX_PATH_DATA_LENGTH];
	int len = strlen(LOG_PREFIX);
	strcpy(buffer, LOG_PREFIX);
	buffer[len] = ' ';
	len++;
	vsnprintf(buffer + len, sizeof(buffer) - len, fmt, ap);
	printf("DEBUG: %s\n", buffer);
}
#else
#define LOG_PREFIX CC_NAME"_SA"
static void coremw_vlog(int priority, char const* fmt, va_list ap)
{
	char buffer[TRACE_BUFFER_SIZE];
	int len = strlen(LOG_PREFIX);
	strcpy(buffer, LOG_PREFIX);
	buffer[len] = ' ';
	len++;
	vsnprintf(buffer+len, sizeof(buffer)-len, fmt, ap);
	syslog(priority, "%s", buffer);
}
#endif // UNIT_TEST


#ifdef UNIT_TEST
void coremw_log(int priority, const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	coremw_vlog2(priority, fmt, ap);
	va_end(ap);
}


void coremw_debug_log(int priority, const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	coremw_vlog2(priority, fmt, ap);
	va_end(ap);
}
#else
void coremw_log(int priority, char const* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	coremw_vlog(priority, fmt, ap);
	va_end(ap);
}


void coremw_debug_log(int priority, char const* fmt, ...)
{
	if ( trace_flag == 1) {
		va_list ap;
		va_start(ap, fmt);
		coremw_vlog(priority, fmt, ap);
		va_end(ap);
	}
}
#endif //UNIT_TEST

#ifdef __cplusplus
}
#endif


#ifdef UNIT_TEST
void err_quit(char const* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	va_end(ap);
}
#endif
