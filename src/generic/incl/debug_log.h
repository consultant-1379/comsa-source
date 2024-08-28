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
 * 	 File: Common_unitest.h
 *
 *   Author: xadaleg
 *
 *   Date:   2014-12-12
 *
 *****************************************************************************/

#ifndef COMMON_UNITTEST_H_
#define COMMON_UNITTEST_H_

#ifdef REDIRECT_LOG
////////////////////////////////////3//////////////////
// LOG
//////////////////////////////////////////////////////
#ifdef  __cplusplus
extern "C" {
#endif
#define LOG_PREFIX " "
static void coremw_vlog2(int priority, char const* fmt, va_list ap);
void coremw_log(int priority, const char* fmt, ...);
void coremw_debug_log(int priority, const char* fmt, ...);

#ifdef  __cplusplus
}
#endif

#endif //REDIRECT_LOG

void err_quit(char const* fmt, ...);

#endif
