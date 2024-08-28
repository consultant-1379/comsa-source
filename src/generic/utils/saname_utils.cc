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
 *   File:   saname_utils.cc
 *
 *   Author: xadaleg
 *
 *   Date:   2014-08-02
 *
 *   This file implements the helper functions to use when converting between
 *   SaNameT and char*.
 *
 *   Modified: xnikvap 2015-08-02 MR42277 Remove old interfaces exposed to COM & CoreMW that are no longer supported for SLES12
 *
 *****************************************************************************/

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "saname_utils.h"
#include "ComSA.h"
#include "trace.h"
#include "ImmCmd.h"
#include <sstream>
#include "Utils.h"

#define DEBUG_SANAME 0

bool bExtendedNamesInitialized = false;
bool bExtendedNameEnabled = false;
SaVersionT imm_version_latest;
#ifdef UNIT_TEST
bool longDnsAllowed = false;
#endif

typedef struct {
	SaUint16T length;
	SaUint8T value[SA_MAX_UNEXTENDED_NAME_LENGTH];
} OldSaNameT;


/**
 * Initialise the saName utility to determine if extended names are supported
 *
 * @param[in]   void
 * @return 		No return value.
 */
void saNameInit(void)
{
	ENTER_IMM_OM();
	DEBUG_IMM_OM("saNameInit ENTER SaVersionT %c %u %u", (char) imm_version_latest.releaseCode, imm_version_latest.majorVersion, imm_version_latest.minorVersion);
	const int output_len = 100;
	char enable[output_len] = {0};
	if (!bExtendedNamesInitialized) {
		bExtendedNameEnabled = false;
		// Determine if extended name are enabled
#ifndef UNIT_TEST
		char *longDnConfigure = getenv("LONG_DN_ENABLED_IN_COREMW");
		if (longDnConfigure != NULL)
		{
			DEBUG_IMM_OM("saNameInit [%s]", longDnConfigure);
			strcpy(enable, longDnConfigure);
		}
		else
		{
			strcpy(enable, "longDnsAllowed: object not found.");
			DEBUG_IMM_OM("saNameInit [%s]", enable);
		}
#else
		if (longDnsAllowed) {
			strcpy(enable, "longDnsAllowed=1");
		}
		else {
			strcpy(enable, "longDnsAllowed=0");
		}
#endif
		if (strncmp(enable, "longDnsAllowed=1", sizeof("longDnsAllowed=1")) == 0) {
			bExtendedNameEnabled = true;
		} else {
			bExtendedNameEnabled = false;
		}
		bExtendedNamesInitialized = true;
	}
	LOG_IMM_OM("saNameInit SA_ENABLE_EXTENDED_NAMES [%s] bExtendedNameEnabled %d bExtendedNamesInitialized %d", enable, bExtendedNameEnabled, bExtendedNamesInitialized);
	DEBUG_IMM_OM("saNameInit LEAVE");
	LEAVE_IMM_OM();
}


/**
 * Set the name variable from the value
 *
 * @param[in]	value - the name string
 * @param[out]	name - the name type
 * @return 		true, if the name was truncated
 *
 * NOTE: prefer using saNameSetLen() whenever possible instead.
 */
bool saNameSet(SaConstStringT value, SaNameT* name)
{
	ENTER_IMM_OM();
	if (DEBUG_SANAME) DEBUG_IMM_OM("saNameSet [%s]", value);
	bool wasNameTruncated = false;
	unsigned len = strlen(value);
	if (!bExtendedNameEnabled) {
		OldSaNameT* oldName = (OldSaNameT*) name;
		memset(oldName, 0, sizeof(OldSaNameT));
		if (len > SA_MAX_UNEXTENDED_NAME_LENGTH) {
			LOG_IMM_OM("saNameSet length truncated from %u to %d, original name: %s", len, SA_MAX_UNEXTENDED_NAME_LENGTH, value);
			len = SA_MAX_UNEXTENDED_NAME_LENGTH;
			wasNameTruncated = true;
		}
		oldName->length = len;
		memcpy(oldName->value, value, len);
		oldName->value[len] = '\0';
		if (DEBUG_SANAME) DEBUG_IMM_OM("saNameSet oldName %u [%s]", oldName->length, oldName->value);
	}
	else {
#ifndef UNIT_TEST
		if (dlsym(RTLD_DEFAULT, "saAisNameLend") != NULL)
#endif
		{
			if (DEBUG_SANAME) DEBUG_IMM_OM("saNameSet saAisNameLend %u [%s]", len, value);
			char *tmpVal = NULL;
			if (len < SA_MAX_UNEXTENDED_NAME_LENGTH) {
				tmpVal = (char*) value;
			}
			else {
				tmpVal = new char[len + 1];
				strncpy(tmpVal, value, len);
				tmpVal[len] = '\0';
			}
			if (tmpVal != NULL) {
				saAisNameLend(tmpVal, name);
			}
			else {
				ERR_IMM_OM("saNameSet tmpVal == NULL");
			}
		}
	}
	LEAVE_IMM_OM();
	return wasNameTruncated;
}

/**
 * Set the name variable from the value
 * @param[in]  value - the name string
 * @param[in]  size  - size of the particular data
 * @param[out] name  - the name type
 * @return     true  - if the name is truncated
 */
bool saNameSetLen(SaConstStringT value, unsigned size, SaNameT* name)
{
	ENTER_IMM_OM();
	bool wasNameTruncated = false;
	unsigned len = size;
	DEBUG_IMM_OM("saNameSetLen[%d] [%u][%s]", bExtendedNameEnabled, len, value);
	if (!bExtendedNameEnabled) {
		OldSaNameT* oldName = (OldSaNameT*) name;
		memset(oldName, 0, sizeof(OldSaNameT));
		if (len > SA_MAX_UNEXTENDED_NAME_LENGTH) {
			LOG_IMM_OM("saNameSetLen length truncated from %u to %d, original name: %s", len, SA_MAX_UNEXTENDED_NAME_LENGTH, value);
			len = SA_MAX_UNEXTENDED_NAME_LENGTH;
			wasNameTruncated = true;
		}
		oldName->length = len;
		memcpy(oldName->value, value, len);
		oldName->value[len] = '\0';
		DEBUG_IMM_OM("saNameSetLen oldName %u [%s]", oldName->length, oldName->value);
	}
	else {
#ifndef UNIT_TEST
		if (dlsym(RTLD_DEFAULT, "saAisNameLend") != NULL)
#endif
		{
			DEBUG_IMM_OM("saNameSetLen saAisNameLend");

			if (len < SA_MAX_UNEXTENDED_NAME_LENGTH) {
				OldSaNameT* oldName = (OldSaNameT*) name;
				memset(oldName, 0, sizeof(OldSaNameT));
				oldName->length = len;
				memcpy(oldName->value, value, len);
				oldName->value[len] = '\0';
				DEBUG_IMM_OM("saNameSetLen oldName %u [%s]", oldName->length, oldName->value);
			}
			else
			{
				char *tmpVal = NULL;
				tmpVal = new char[len + 1];
				strncpy(tmpVal, value, len);
				tmpVal[len] = '\0';

				if (tmpVal != NULL) {
					saAisNameLend(tmpVal, name);
				}
				else {
					ERR_IMM_OM("saNameSetLen tmpVal == NULL");
				}
			}
		}
	}
	LEAVE_IMM_OM();
	return wasNameTruncated;
}

/**
 * Set the length in the name variable
 *
 * @param[in]	len - the name string length
 * @param[out]	name - the name type
 * @return 		true, if the name was truncated
 */
bool saNameOverrideLen(unsigned len, SaNameT* name)
{
	ENTER_IMM_OM();
	if (DEBUG_SANAME) DEBUG_IMM_OM("saNameOverrideLen %u %u [%s]", len, saNameLen(name), saNameGet(name));
	bool wasNameTruncated = false;
	unsigned getLen = saNameLen(name);
	if (getLen <= SA_MAX_UNEXTENDED_NAME_LENGTH) {
		OldSaNameT* oldName = (OldSaNameT*) name;
		if (len > SA_MAX_UNEXTENDED_NAME_LENGTH) {
			LOG_IMM_OM("saNameOverrideLen length truncated from %u to %d, original name: %s", len, SA_MAX_UNEXTENDED_NAME_LENGTH, saNameGet(name));
			len = SA_MAX_UNEXTENDED_NAME_LENGTH;
			wasNameTruncated = true;
		}
		oldName->length = len;
		oldName->value[len] = '\0';
	}
	else {
		ERR_IMM_OM("saNameOverrideLen unable to override length for long DN");
	}
	if (DEBUG_SANAME) DEBUG_IMM_OM("saNameOverrideLen %u [%s]", saNameLen(name), saNameGet(name));
	LEAVE_IMM_OM();
	return wasNameTruncated;
}


/**
 * Get the name string from the name type
 *
 * @param[in]	name - the name type
 * @return 		The name string
 */
SaConstStringT saNameGet(const SaNameT* name)
{
	ENTER_IMM_OM();
	SaConstStringT value = NULL;
	if (!bExtendedNameEnabled) {
		OldSaNameT* oldName = (OldSaNameT*) name;
		if (name != NULL) {
			value = (char*) oldName->value;
		}
		else {
			value = "";
		}
	}
	else {
#ifndef UNIT_TEST
		if (dlsym(RTLD_DEFAULT, "saAisNameBorrow") != NULL)
#endif
		{
			value = saAisNameBorrow(name);
			if (DEBUG_SANAME) DEBUG_IMM_OM("saNameGet saAisNameBorrow");
		}
	}
	if (NULL != value)
	{
		if (DEBUG_SANAME) DEBUG_IMM_OM("saNameGet %d [%s]", (int) strlen(value), value);
	}
	LEAVE_IMM_OM();
	return value;
}


/**
 * Delete the name string from the name type
 *
 * @param[in]	name - the name type
 * @param[in]	deleteName - true, if the SaNameT struct is to be deleted
 * @return 		No return value.
 */
void saNameDelete(SaNameT* name, bool deleteName)
{
	ENTER_IMM_OM();
	SaConstStringT value = saNameGet(name);
	if (value != NULL) {
		int len = strlen(value);
		if (DEBUG_SANAME) DEBUG_IMM_OM("saNameDelete %d [%s]", len, value);
		if (bExtendedNameEnabled) {
			if (len >= SA_MAX_UNEXTENDED_NAME_LENGTH) {
				delete [] value;
				value = NULL;
			}
		}
	}
	else {
		ERR_IMM_OM("saNameDelete value == NULL");
	}
	if (deleteName && (name != NULL)) {
		delete name;
		name = NULL;
	}
	LEAVE_IMM_OM();
}

/**
 * Get the length of the name type
 *
 * @param[in]	name - the name type
 * @return 		Length of the name
 */
unsigned saNameLen(const SaNameT* name)
{
	ENTER_IMM_OM();
	if(NULL == name)
	{
		if (DEBUG_SANAME) DEBUG_IMM_OM("saNameLen name is NULL, return 0");
		LEAVE_IMM_OM();
		return 0;
	}
	unsigned length = 0;
	if (!bExtendedNameEnabled) {
		OldSaNameT* oldName = (OldSaNameT*) name;
		length = oldName->length;
	}
	else {
		length = strlen(saNameGet(name));
	}
	if (DEBUG_SANAME) DEBUG_IMM_OM("saNameLen %u  [%s]", length, saNameGet(name));
	LEAVE_IMM_OM();
	return length;
}

/**
 * Get the maximum length of the name
 *
 * @return 		Maximum length of the name
 */
unsigned saNameMaxLen()
{
	ENTER_IMM_OM();
	unsigned maxLen = 0;
	if (!bExtendedNameEnabled) {
		maxLen = SA_MAX_UNEXTENDED_NAME_LENGTH;
	}
	else {
		maxLen = MAX_DN_LENGTH;
	}
	if (DEBUG_SANAME) DEBUG_IMM_OM("saNameMaxLen %u", maxLen);
	LEAVE_IMM_OM();
	return maxLen;
}

/**
 * Get the maximum length of the NTF name
 *
 * @return 		Maximum length of the NTF name
 */
unsigned saNameMaxLenNtf()
{
	ENTER_IMM_OM();
	unsigned maxLen = 0;
	if (!bExtendedNameEnabled) {
		maxLen = SA_MAX_UNEXTENDED_NAME_LENGTH+1;
	}
	else {
		maxLen = MAX_DN_LENGTH;
	}
	if (DEBUG_SANAME) DEBUG_IMM_OM("saNameMaxLenNtf %u", maxLen);
	LEAVE_IMM_OM();
	return maxLen;
}

/**
 * Convert a SaNameT to a char*
 * Note: the char* is allocated by new and must be deleted
 *
 * @param[in]	name - the name type
 * @return 		pointer to the name string
 */
char* makeCString(const SaNameT* saName) {
	ENTER_IMM_OM();
	unsigned len = saNameLen(saName);
	char* tmpStr = new char[len + 1];
	if (tmpStr != NULL) {
		memcpy(tmpStr, saNameGet(saName), len);
		tmpStr[len] = 0; // make a c-string!!
		if (DEBUG_SANAME) DEBUG_IMM_OM("makeCString %u [%s]", len, tmpStr);
	}
	else {
		ERR_IMM_OM("makeCString tmpStr == NULL");
	}
	LEAVE_IMM_OM();
	return tmpStr;
}

/**
 * Convert a char* to an SaNameT
 * Note: the SaNameT* is allocated by new and must be deleted
 *
 * @param[in]	cstr - the name string
 * @return 		pointer to the name type
 */
SaNameT* makeSaNameT(const char* cstr) {
	ENTER_IMM_OM();
	if (DEBUG_SANAME) { unsigned len = strlen(cstr); DEBUG_IMM_OM("makeSaNameT %u [%s]", len, cstr); }
	SaNameT* saname = new SaNameT;
	if (saname != NULL) {
		saNameSet(cstr, saname);
	}
	else {
		ERR_IMM_OM("makeSaNameT saname == NULL");
	}
	LEAVE_IMM_OM();
	return saname;
}


/**
 * Get the clear storage location
 *
 * @param[out]	strPath - the location of the clear storage
 * @return 		True, if the path is retrieved.
 */
bool getClearStorage(char* strPath)
{
#ifndef UNIT_TEST
	char* cmd = NULL;
	char* compName = Utils::convertToLowerCase((const char*)CC_NAME);
	asprintf(&cmd, "%s%s", compName, "sa_pso getPath clear"); // this command is used to get the clear location on the cluster
#else
	const char cmd[] = "pwd"; // this command is used to get the location for unit testing
#endif
	bool bSuccess = true;
	FILE *dirPath = NULL;
	char tempPath[MAX_PATH_DATA_LENGTH];

	dirPath = popen(cmd, "r");
	if(NULL == dirPath)
	{
		ERR_MWSA_REPLIST("getClearStorage(): ERROR: Could not execute '%s'", cmd);
		bSuccess = false;
	}
	else
	{
		DEBUG_MWSA_REPLIST("getClearStorage(): executed '%s' successfully", cmd);
		if(NULL == fgets(tempPath, MAX_PATH_DATA_LENGTH, dirPath))
		{
			ERR_MWSA_REPLIST("getClearStorage(): ERROR: fgets() returned NULL");
			bSuccess = false;
			pclose(dirPath);
		}
		else
		{
			DEBUG_MWSA_REPLIST("getClearStorage(): '%s' returned '%s'", cmd, tempPath);

			if(0 != pclose(dirPath))
			{
				ERR_MWSA_REPLIST("getClearStorage(): ERROR: pclose() failed");
				bSuccess = false;
			}
			else
			{
				DEBUG_MWSA_REPLIST("getClearStorage(): pclose() was successful");

				/* remove trailing space that comes as output of fgets */
				unsigned int i = 0;
				while(i < strlen(tempPath) - 1)
				{
					strPath[i] = tempPath[i];
					i++;
				}
				strPath[i] = '\0';
			}
		}
	}

#ifndef UNIT_TEST
	if (compName != NULL)
	{
		free(compName);
		compName = NULL;
	}

	if (cmd != NULL)
	{
		free(cmd);
		cmd = NULL;
	}
#endif
	return bSuccess;
}

void getThresholdInfoData(SaNtfThresholdInformationT *thresholdInformation, std::string&observedVal, std::string& thresholdVal){
	std::ostringstream obsVal,thresVal;
	if(NULL == thresholdInformation){
		DEBUG_PMTSA_EVENT("getThresholdInfoData: ThresholdInformation is null");
		return;
	}
	switch(thresholdInformation->thresholdValueType){
	case SA_NTF_VALUE_UINT8 :     /* 1 byte long - unsigned int */
		obsVal << thresholdInformation->observedValue.uint8Val;
		thresVal << thresholdInformation->thresholdValue.uint8Val;
		break;
	case SA_NTF_VALUE_INT8 :       /* 1 byte long - signed int */
		obsVal << thresholdInformation->observedValue.int8Val;
		thresVal << thresholdInformation->thresholdValue.int8Val;
		break;
	case SA_NTF_VALUE_UINT16 :    /* 2 bytes long - unsigned int */
		obsVal << thresholdInformation->observedValue.uint16Val;
		thresVal << thresholdInformation->thresholdValue.uint16Val;
		break;
	case SA_NTF_VALUE_INT16 :     /* 2 bytes long - signed int */
		obsVal << thresholdInformation->observedValue.int16Val;
		thresVal << thresholdInformation->thresholdValue.int16Val;
		break;
	case SA_NTF_VALUE_UINT32 :    /* 4 bytes long - unsigned int */
		obsVal << thresholdInformation->observedValue.uint32Val;
		thresVal << thresholdInformation->thresholdValue.uint32Val;
		break;
	case SA_NTF_VALUE_INT32 :     /* 4 bytes long - signed int */
		obsVal << thresholdInformation->observedValue.int32Val;
		thresVal << thresholdInformation->thresholdValue.int32Val;
		break;
	case SA_NTF_VALUE_FLOAT :     /* 4 bytes long - float */
		obsVal << thresholdInformation->observedValue.floatVal;
		thresVal << thresholdInformation->thresholdValue.floatVal;
		break;
	case SA_NTF_VALUE_UINT64 :    /* 8 bytes long - unsigned int */
		obsVal << thresholdInformation->observedValue.uint64Val;
		thresVal << thresholdInformation->thresholdValue.uint64Val;
		break;
	case SA_NTF_VALUE_INT64 :     /* 8 bytes long - signed int */
		obsVal << thresholdInformation->observedValue.int64Val;
		thresVal << thresholdInformation->thresholdValue.int64Val;
		break;
	case SA_NTF_VALUE_DOUBLE :    /* 8 bytes long - double */
		obsVal << thresholdInformation->observedValue.doubleVal;
		thresVal << thresholdInformation->thresholdValue.doubleVal;
		break;
	case SA_NTF_VALUE_LDAP_NAME : /* SaNameT type */
	case SA_NTF_VALUE_STRING :    /* '\0' terminated char array (UTF-8 encoded) */
	case SA_NTF_VALUE_IPADDRESS : /* IPv4 or IPv6 address as '\0' terminated char array */
	case SA_NTF_VALUE_BINARY:    /* Binary data stored in bytes - number of bytes stored separately */
//	{   //enable if coreMW sends the  ThresholdInformation observedValue and ThresholdValue with these types
//		void *dataPtr;
//		SaUint16T dataSize = 0;
//		SaNtfNotificationHandleT notificationHandle;
//		SaAisErrorT retVal = saNtfPtrValGet(notificationHandle, // [IN] Handle to identify the transaction
//				&(thresholdInformation->observedValue),						// [IN] offset and size of data buffer
//				&dataPtr,										// [OUT] Pointer to the buffer with the data
//				&dataSize);										// [OUT] Size of this particular data (Including null at the end of the string)
//		if (retVal == SA_AIS_OK)
//		{
//			std::string observedValue(saNameGet((SaNameT *)dataPtr), saNameLen((SaNameT *)dataPtr));
//			obsVal << observedVal;
//		}
//		void *dataPtr1;
//		retVal = saNtfPtrValGet(notificationHandle, // [IN] Handle to identify the transaction
//				&(thresholdInformation->thresholdValue),						// [IN] offset and size of data buffer
//				&dataPtr1,										// [OUT] Pointer to the buffer with the data
//				&dataSize);										// [OUT] Size of this particular data (Including null at the end of the string)
//		if (retVal == SA_AIS_OK)
//		{
//			std::string thresholdValue(saNameGet((SaNameT *)dataPtr), saNameLen((SaNameT *)dataPtr));
//			thresVal << thresholdValue;
//		}
//	}
//	break;

	case SA_NTF_VALUE_ARRAY :
	default:
		DEBUG_PMTSA_EVENT("Not supprted type for ThresholdInformation thresholdvalue or observed value");
		//Debug stmt
		break;
	}

	observedVal = obsVal.str();
	thresholdVal = thresVal.str();
	DEBUG_PMTSA_EVENT("getThresholdInfoData(): thresholdValueType = %d observedVal = %s, thresholdVal = %s", (int)thresholdInformation->thresholdValueType,observedVal.c_str(),thresholdVal.c_str());
	return;
}
SaStringT formatAdditionalText( SaNtfThresholdInformationT *thresholdInformation,
							    SaConstStringT measurementTypeDn,
								SaInt32T thresholdDirection,
								SaConstStringT moInstance )
{
	std::ostringstream addText;
	std::string observedValue, thresholdValue;
	getThresholdInfoData(thresholdInformation, observedValue, thresholdValue);

	addText << "Observed value: ";
	if(!observedValue.empty()){
		addText << observedValue.c_str();;
	}
	addText << "; ";

	addText << "Threshold level: ";
	if(!thresholdValue.empty()){
		addText << thresholdValue.c_str();
	}
	addText << "; ";

	addText << "MeasurementType: ";
	if(NULL != measurementTypeDn){
		addText << measurementTypeDn;
	}
	addText << "; ";

	addText << "Threshold Direction: ";
	if(thresholdDirection == 1){
		addText << "INCREASING";
	}else if(thresholdDirection == 2){
		addText << "DECREASING";
	}else{
		addText << "INCREASING";
	}
	addText << "; ";

	addText << "MO instance: ";

	if(NULL != moInstance){
		addText << moInstance;
	}
	std::string addTextStr = addText.str();
	char* additionalText = strdup(addTextStr.c_str());
	DEBUG_PMTSA_EVENT("FormatAdditionalText(): PM alarm additional text = %s", additionalText);
	return additionalText;
}
