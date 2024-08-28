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
 *   File:   saAis_dummy.cc
 *
 *   Author: xadaleg
 *
 *   Date:   2014-08-18
 *
 *   This file implements the helper functions to use when contacting IMM.
 *
 *   Modify: xadaleg 2014-08-18  MR35347 - increase DN length
 *
 *****************************************************************************/
#include <unistd.h>
#include "saAis.h"
#include <string.h>

/* ----------------------------------------------------------------------
 * IMM call wrappers; This wrapper interface off loads the burden to
 * handle return values and retries for each and every IMM-call. It
 * makes the code cleaner.
 */
typedef struct {
	SaUint16T length;
	SaUint8T value[SA_MAX_UNEXTENDED_NAME_LENGTH];
} OldSaNameT2;

enum {
	/* Index in the SaNameT._opaque array where the string pointer will be
	   stored when the distinguished name is longer than 255 bytes. By
	   storing the pointer at an aligned address, Valgrind will be able to
	   detect the pointer and thus the memory leak detection in Valgrind
	   will work with these strings. Note though that since the largest
	   member in the SaNameT structure is a 16-bit integer, there is no
	   guarantee that the SaNameT structure itself is stored at an aligned
	   address. */
	kExtendedNamePointerOffset = sizeof(SaConstStringT) / sizeof(SaUint16T),
	/**
	*  Magic number stored in the .length field (the first 16-bit word) of the
	*  legacy SaNameT type, to indicate that it contains a string longer than or
	*  equal to SA_MAX_UNEXTENDED_NAME_LENGTH bytes. A pointer to the string is
	*  stored immediately after the first 16-bit word (typically in the first
	*  four or eight bytes of the .value field of the legacy SaNameT type.
	*/
	kOsafExtendedNameMagic = 0xcd2b,

	/**
	*  Maximum length of a distinguished name, not counting the terminating NUL
	*  character.
	*/
	kOsafMaxDnLength = 2048
};

static inline SaConstStringT get_ptr(const SaNameT* name)
{
	union {
		SaConstStringT pointer;
		SaUint8T bytes[sizeof(SaConstStringT)];
	} tmp;
	memcpy(tmp.bytes, name->_opaque + kExtendedNamePointerOffset,
		sizeof(SaConstStringT));
	return tmp.pointer;
}

static inline void set_ptr(SaConstStringT value, SaNameT* name)
{
	union {
		SaConstStringT pointer;
		SaUint8T bytes[sizeof(SaConstStringT)];
	} tmp;
	tmp.pointer = value;
	name->_opaque[0] = kOsafExtendedNameMagic;
	memcpy(name->_opaque + kExtendedNamePointerOffset, tmp.bytes,
		sizeof(SaConstStringT));
}

void saAisNameLend(SaConstStringT value, SaNameT* name)
{
	size_t length = strlen(value);
	if (length < SA_MAX_UNEXTENDED_NAME_LENGTH) {
		name->_opaque[0] = length;
		memcpy(name->_opaque + 1, value, length + 1);
	} else {
		set_ptr(value, name);
	}
}

SaConstStringT saAisNameBorrow(const SaNameT* name)
{
	size_t length = name->_opaque[0];
	SaConstStringT value;
	if (length != kOsafExtendedNameMagic) {
		value = (SaConstStringT) (name->_opaque + 1);
	} else {
		value = get_ptr(name);
	}
	return value;
}
