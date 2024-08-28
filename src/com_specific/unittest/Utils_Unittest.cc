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
 * 	 File: Utils_Unitest.cc
 *
 *   Author: xanhqle
 *
 *   Date:   2014-09-18
 *   Modify:   xanhqle  2014-09-18:  Create Unittest for MR35347 - increase DN length
 *
 *****************************************************************************/
#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <list>
#include <vector>
#include <map>
#include <ctype.h>

#include <assert.h>
#include <gtest/gtest.h>

#include "saAis.h"
#include "ComSA.h"
#include "saname_utils.h"


// UT for MR35347 - increase DN length
extern bool bExtendedNamesInitialized;
extern bool bExtendedNameEnabled;
extern SaVersionT imm_version_latest;
extern bool longDnsAllowed;

// Check SaNameInit() with IMM version
TEST (SaName, saNameInitOld)
{

	// Wrong imm version

	imm_version_latest = {'A',2,11};

	EXPECT_EQ ( false, bExtendedNamesInitialized );
	EXPECT_EQ ( false, bExtendedNameEnabled );

	saNameInit();

	EXPECT_EQ ( false, bExtendedNameEnabled  );
	EXPECT_EQ ( true, bExtendedNamesInitialized );

	// Correct imm version

	imm_version_latest = {immReleaseCode, immMajorVersion, immMinorVersion};
	bExtendedNamesInitialized = false;

	saNameInit();

	EXPECT_EQ ( false, bExtendedNameEnabled  );
	EXPECT_EQ ( true, bExtendedNamesInitialized );
}

// Check SaNameInit() with environment variable
TEST (SaName, saNameInitNew)
{
	// Enable environment variable
	bExtendedNameEnabled = false;
	bExtendedNamesInitialized = false;
	longDnsAllowed = true;

	saNameInit();

	EXPECT_EQ ( true, bExtendedNameEnabled  );
	EXPECT_EQ ( true, bExtendedNamesInitialized );
	longDnsAllowed = false;
}

//Check SaNameSet() with Old functionality
TEST (SaName, saNameSetOld)
{
	SaNameT name;
	char value[MAX_DN_LENGTH];

	memset(value, 'A', MAX_DN_LENGTH);

	// String too long

	value[SA_MAX_UNEXTENDED_NAME_LENGTH+5] = '\0';
	bExtendedNameEnabled = false;

	saNameSet((SaConstStringT) &value, &name);

	EXPECT_EQ ( saNameLen(&name), SA_MAX_UNEXTENDED_NAME_LENGTH  );
}

// Check SaNameSet() with New functionality
TEST (SaName, saNameSetNew)
{
	SaNameT name;
	char value[MAX_DN_LENGTH];

	memset(value, 'A', MAX_DN_LENGTH);
	value[SA_MAX_UNEXTENDED_NAME_LENGTH + 5] = '\0';
	// Use new extended functionality
	bExtendedNameEnabled = true;
	saNameSet((SaConstStringT) &value, &name);
	EXPECT_EQ ( saNameLen(&name), SA_MAX_UNEXTENDED_NAME_LENGTH + 5 );
	saNameDelete(&name, false);
}

//Check SaNameSet() with Old functionality
TEST (SaName, saNameOverrideLenShort)
{
	SaNameT name;
	char value[MAX_DN_LENGTH];
	memset(value, 'A', MAX_DN_LENGTH);
	// String too long
	value[SA_MAX_UNEXTENDED_NAME_LENGTH-5] = '\0';
	bExtendedNameEnabled = false;
	saNameSet((SaConstStringT) &value, &name);
	saNameOverrideLen(SA_MAX_UNEXTENDED_NAME_LENGTH-3, &name);
	EXPECT_EQ ( saNameLen(&name), SA_MAX_UNEXTENDED_NAME_LENGTH-3 );
	saNameOverrideLen(SA_MAX_UNEXTENDED_NAME_LENGTH+3, &name);
	EXPECT_EQ ( saNameLen(&name), SA_MAX_UNEXTENDED_NAME_LENGTH );
	saNameDelete(&name, false);
}

// Check SaNameSet() with New functionality
TEST (SaName, saNameOverrideLenLong)
{
	SaNameT name;
	char value[MAX_DN_LENGTH];
	memset(value, 'A', MAX_DN_LENGTH);
	value[SA_MAX_UNEXTENDED_NAME_LENGTH + 5] = '\0';
	// Use new extended functionality
	bExtendedNameEnabled = true;
	saNameSet((SaConstStringT) &value, &name);
	saNameOverrideLen(SA_MAX_UNEXTENDED_NAME_LENGTH + 5, &name);
	EXPECT_EQ ( saNameLen(&name), SA_MAX_UNEXTENDED_NAME_LENGTH + 5 );
	saNameDelete(&name, false);
}

//Check SaNameGet() with Old functionality
TEST (SaName, saNameGetOld)
{
	SaNameT name;
	SaNameT nameNull;
	char value[MAX_DN_LENGTH];
	char valueNull[MAX_DN_LENGTH];
	// In case name is not NULL
	memset(value, 'A', MAX_DN_LENGTH);
	value[SA_MAX_UNEXTENDED_NAME_LENGTH-5] = '\0';
	// Don't enable ExtendedName
	bExtendedNameEnabled = false;
	saNameSet((SaConstStringT) &value, &name);
	saNameGet(&name);
	char valueT[MAX_DN_LENGTH];
	strcpy(valueT, value);
	int TestStatus = strcmp(valueT, saNameGet(&name));
	EXPECT_EQ ( 0, TestStatus );
	// In case name is NULL
	valueNull[0] = '\0';
	saNameSet((SaConstStringT) &valueNull, &nameNull);
	saNameGet(&nameNull);
	printf("Value of saNameGet() %s:", saNameGet(&nameNull));
	char valueNullT[MAX_DN_LENGTH];
	strcpy(valueNullT, valueNull);
	printf("Value of valueT %s:", valueNullT);
	int TestStatusNull = strcmp(valueNullT, saNameGet(&nameNull));
	EXPECT_EQ ( 0, TestStatusNull );
	// In case name is NULL
	TestStatusNull = strcmp("", saNameGet(NULL));
	EXPECT_EQ ( 0, TestStatusNull );
}

//Check SaNameGet() with New functionality
TEST (SaName, saNameGetNew)
{
	SaNameT name;
	char value[MAX_DN_LENGTH];
	memset(value, 'A', MAX_DN_LENGTH);
	value[SA_MAX_UNEXTENDED_NAME_LENGTH - 10] = '\0';
	// Enable ExtendedName
	bExtendedNameEnabled = true;
	saNameSet((SaConstStringT) &value, &name);
	saNameGet(&name);
	printf("Value of variable value %s:", value);
	printf("Value of saNameGet() %s:", saNameGet(&name));
	int TestStatus = strcmp(value, saNameGet(&name));
	EXPECT_EQ ( 0, TestStatus );
}

//Check SaNameGet() with New functionality (LongDN)
TEST (SaName, saNameGetNewLongDN)
{
	SaNameT name;
	char value[MAX_DN_LENGTH];
	memset(value, 'A', MAX_DN_LENGTH);
	value[SA_MAX_UNEXTENDED_NAME_LENGTH + 5] = '\0';
	// Enable ExtendedName
	bExtendedNameEnabled = true;
	saNameSet((SaConstStringT) &value, &name);
	printf("Value of variable value %s:", value);
	printf("Value of saNameGet() %s:", saNameGet(&name));
	int TestStatus = strcmp(value, saNameGet(&name));
	EXPECT_EQ ( 0, TestStatus );
	saNameDelete(&name, false);
}

//Check saNameMaxLen() and saNameMaxLenNtf()
TEST (SaName, saNameMaxLEN)
{
	// Don't Enable ExtendedName
	bExtendedNameEnabled = false;

	EXPECT_EQ ( saNameMaxLen(), SA_MAX_UNEXTENDED_NAME_LENGTH );
	EXPECT_EQ ( saNameMaxLenNtf(), SA_MAX_UNEXTENDED_NAME_LENGTH + 1 );

	// Enable ExtendedName
	bExtendedNameEnabled = true;

	EXPECT_EQ ( saNameMaxLen(), MAX_DN_LENGTH );
	EXPECT_EQ ( saNameMaxLenNtf(), MAX_DN_LENGTH );

	bExtendedNameEnabled = false;
}
