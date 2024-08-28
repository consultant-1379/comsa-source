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
 *   File:   saname_utils.h
 *
 *   Author: xadaleg
 *
 *   Date:   2014-08-02
 *
 *   This file implements the helper functions to use when converting between
 *   SaNameT and char*.
 *
 *   Modified: xnikvap 2015-08-02  MR42277 Remove old interfaces exposed to COM & CoreMW that are no longer supported for SLES12
 *
 *****************************************************************************/
#ifndef SANAMEUTILS_h
#define SANAMEUTILS_h

#include <stdbool.h>
#include "saAis.h"
#include "saNtf.h"

/**
*  Maximum length of a distinguished name, not counting the terminating NUL
*  character.
*/
#define MAX_DN_LENGTH 2048

#ifdef __cplusplus
extern "C" {
#endif
	extern SaVersionT imm_version_latest;

	extern void saNameInit(void);

	extern bool saNameSet(SaConstStringT value, SaNameT* name);

	extern bool saNameSetLen(SaConstStringT value, unsigned size, SaNameT* name);

	extern bool saNameOverrideLen(unsigned len, SaNameT* name);

	extern SaConstStringT saNameGet(const SaNameT* name);

	extern void saNameDelete(SaNameT* name, bool deleteName);

	extern unsigned saNameLen(const SaNameT* name);

	extern unsigned saNameMaxLen();

	extern unsigned saNameMaxLenNtf();

	extern char* makeCString(const SaNameT* saName);

	extern SaNameT* makeSaNameT(const char* cstr);

	extern bool getClearStorage(char* location);

	extern SaStringT formatAdditionalText( SaNtfThresholdInformationT *thresholdInformation, SaConstStringT measurementTypeDn, SaInt32T thresholdDirection, SaConstStringT moInstance );


#ifdef __cplusplus
}
#endif
#endif
