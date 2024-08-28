#ifndef UTILS_INTERFACE_H
#define UTILS_INTERFACE_H

/**
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
 *   File:   UtilsInterface.h
 *
 *   Author: eaparob,uabjoy
 *
 *   Date:   2014-05-09
 *
 *   This file declares a C-style function for use outside Utils class functions
 *
 */

extern char* _CC_NAME;
extern char* _CC_NAME_UPPERCASE;
extern char* _CC_NAME_SA;

#ifdef  __cplusplus
extern "C" {
#endif

/*
 *  Creates COM SA Trace directory
 */
void createComSaTraceDirectory();
int processOpen(const char* cmd);
char* convertToLowerCase(const char* word);
char* convertToUpperCase(const char* word);

#ifdef  __cplusplus
}
#endif
#endif
