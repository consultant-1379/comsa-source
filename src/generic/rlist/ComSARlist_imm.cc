/******************************************************************************
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
 *	 Author: DEK Beehive team (xadaleg, xnikvap, xanhqle, xanhdao, xthabui, xtronle)
 *
 *	 File:	ComSARlist_imm.cc
 *	 The ReplicatedList service in COM_SA.
 *
 *****************************************************************************/
/*
 *
 * Modify: xadaleg 2015-02-17  MR37637 - Adapt IMM for replicated list service instead of checkpoint
 * Modify: xnikvap 2015-04-22  Corection for TR TR HT65567: IMM Accessor handle leak and SPI return codes
 * Modify: xvintra 2015-08-10  Corection for TR HT94210 COMSA rplist not handling storage clear area being deleted during system restore.
 *
 *****************************************************************************/
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include <limits.h>
#include <sstream>
#include <string>
#include <vector>


#include "ComSARlist.h"
#include "ComSARlist_imm.h"

#define FM_STATUS_SECTION "FmStatusSection"
#define RPLIST_MAX_RETRIES 5
#define RPLIST_RETRY_SLEEP_SECOND 1

#define IMPLEMENTER_NAME     CC_NAME"SaRepListOI"
#define RLIST_CLASS_RDN      CC_NAME"saRlistClassRdn"
#define RLIST_BLOCK_SIZE     "saRlistBlockSize"
#define RLIST_LAST_ID        "saRlistLastId"
#define RLIST_NUM_ELEMENTS   "saRlistNumElements"

using namespace RlistUtil;

/*
 * These IMM handles are obtained once and kept active all the time.
 * The OI is also kept active all the time.
 * When some IMM operation fails with SA_AIS_ERR_BAD_HANDLE (9) then new handles must be obtained and
 * the OI activated again
 */
static SaImmHandleT rplistImmHandle         = 0; /* IMM Handles to be used for RPList */
static SaImmHandleT rplistImmOiHandle       = 0;
static SaImmAccessorHandleT immAccessHandle = 0;
static bool rplistOiSetFlag = false;
//To avoid recursive calls, retry variable for handling SA_AIS_ERR_NO_RESOURCES case
short retries = 0;

OamSARList::OamSARList()
{
	ENTER_MWSA_REPLIST();
	LEAVE_MWSA_REPLIST();
}

OamSARList::~OamSARList()
{

}

// Declare Object Implementer name for Runtime objects
static const char *implementerName = IMPLEMENTER_NAME;
#ifndef UNIT_TEST
static char pathToStorageDirectory[MAX_PATH_DATA_LENGTH] = "\0";
#else
char pathToStorageDirectory[MAX_PATH_DATA_LENGTH] = "\0";
#endif

// Constant Strings
#ifndef UNIT_TEST
static const char strRpListPath[] = "rplist/";
static const char strBin[] = ".bin";
static char strRlistClassRdn[] = RLIST_CLASS_RDN;
static char* strRlistBlockSize = NULL;
static char* strRlistLastId = NULL;
static char* strRlistNumElements = NULL;
static std::string sRuntimeIMMSuffix("RT");
static std::string sSaRlistRTdn("saRlistRTdn");
static std::string sData("data");
static std::string sEqualSign("=");
#else
extern const char strRpListPath[] = "rplist/";
const char strBin[] = ".bin";
char strRlistClassRdn[] = "ComsaRlistClassRdn";
char strRlistBlockSize[] = "comsaRlistBlockSize";
char strRlistLastId[] = "comsaRlistLastId";
char strRlistNumElements[] = "comsaRlistNumElements";
std::string sRuntimeIMMSuffix("RT");
std::string sSaRlistRTdn("saRlistRTdn");
std::string sData("data");
std::string sEqualSign("=");
#endif

#ifndef UNIT_TEST
static MafReturnT comsa_rplist_imm_init(SaImmHandleT *immHandle);
static void comsa_rplist_imm_finalize(SaImmHandleT *immHandle);
static MafReturnT comsa_rplist_immOi_init(SaImmHandleT *immHandle);
static void comsa_rplist_immOi_finalize(SaImmHandleT *immHandle);
static MafReturnT comsa_rplist_immAccess_init(const SaImmHandleT immHandle, SaImmAccessorHandleT *immAccessorHandle);
static void comsa_rplist_immAccess_finalize(SaImmAccessorHandleT immAccessorHandle);
static MafReturnT comsa_rplist_immOi_set(void);

// IMM Class Functions
static MafReturnT createclass(const char* className, uint32_t dataBufferSize, bool createRunTimeClass = false);
static MafReturnT deleteclass(const char* className);
static MafReturnT searchclass(const char* className);

// IMM Object Functions
static MafReturnT createObject(const char* className, const SaImmAttrValuesT_2 **attrValues);
static MafReturnT readObject(const SaNameT* immObject, SaImmAttrValuesT_2 ***attrValues);
static MafReturnT modifyObject(const SaNameT *objectName, const SaImmAttrModificationT_2 **attrValues);
static MafReturnT deleteObject(const SaNameT *objectName);
static MafReturnT setObjectUiAttribute(const char* className, const char* attrName, unsigned int attrValue);
static MafReturnT getObjectUiAttribute(const char* className, const char* attrName, unsigned int* uiAttrValPtr);
static MafReturnT addRlistRTObject(const std::string& pClassName, const unsigned int& lastId, const void* newItemDataBuffer);
static MafReturnT modifyRlistRTObject(const std::string& pClassName, const std::string& lastId, const void* newItemDataBuffer);
static MafReturnT getAllRTObjects(const char* className, std::vector<std::string>& immRTObjects);
static MafReturnT getRTObjectsCount(const char* className , uint32_t& rlistObjCnt);
static MafReturnT deleteRTObject(const std::string& listInstanceName, const std::string& listItemName);
static MafReturnT deleteAllRTObjects(const char* className);
// File Functions
static MafReturnT createDir(const char* fileName, const char* path);
static MafReturnT writeToFile(const char* fileName, const char* subDir, const void *data, const int dataSize);
static MafReturnT autoRetry_writeToFile(const MafMwSpiListNameT* listInstanceName, const MafMwSpiListItemNameT* listItemName, const void* replaceItemData);
static MafReturnT readFromFile(const char* fileName, const char* subDir, void *data, const int dataSize);
static MafReturnT autoRetry_readFromFile(const char* fileName, const char* subDir, void *data, const int dataSize);
static MafReturnT findFile(const char* fileName, const char* subDir);
static MafReturnT removeFile(const char* fileName, const char* subDir);
static MafReturnT getBaseRepository();
static MafReturnT detectClusterState(const MafMwSpiListNameT* listInstanceName, unsigned int* stateFlag);
static MafReturnT validateImmData(const MafMwSpiListNameT* listInstanceName, const char* dirName);
static MafReturnT restore_immData(const MafMwSpiListNameT* listInstanceName);
static MafReturnT comsa_rplist_imm_removeAllFiles(const char* subDir);
#endif

char* comsa_rplist_imm_getfullPath(const char* subDir);

/**
 * Remove the variable ptr from the environment
 */
void RlistUtil::unsetFromEnvironment(const char *ptr)
{
	LOG_MWSA_REPLIST("unsetFromEnvironment1(%s) is called", ptr);
	char *value = getenv(ptr);
	if( NULL != value )
	{
		LOG_MWSA_REPLIST("getenv(%s) returns (%s), hence trying to unsetenv", ptr,value);
		unsetenv(ptr);
		value = getenv(ptr);
		if( NULL != value )
		{
			LOG_MWSA_REPLIST("getenv(%s) returns (%s) after unsetenv, something is wrong here!!", ptr,value);
		}
		else
		{
			LOG_MWSA_REPLIST("getenv(%s) succesfully returns NULL after unsetenv", ptr);
		}
	}
}
// Function to clear and refresh immHandler whenever SA_AIS_ERR_BAD_HANDLE is received
void clearAndRefreshIMMHandler()
{
    SaAisErrorT err = autoRetry_saImmOiImplementerClear(rplistImmOiHandle);
    if (err != SA_AIS_OK)
    {
	    WARN_MWSA_REPLIST("saImmOiImplementerClear(): autoRetry_saImmOiImplementerClear Failed with rc: %s", getOpenSAFErrorString(err));
    }
    comsa_rplist_immOi_finalize(&rplistImmOiHandle);
    comsa_rplist_immAccess_finalize(immAccessHandle);
    comsa_rplist_imm_finalize(&rplistImmHandle);
}

#ifndef UNIT_TEST
static MafReturnT getRTObjectsCount(const char* className , uint32_t& rlistObjCnt)
#else
MafReturnT getRTObjectsCount(const char* className , uint32_t& rlistObjCnt)
#endif
{
	ENTER_MWSA_REPLIST();
	MafReturnT retVal = MafOk;
	std::vector<std::string> immRTObjects;
	retVal = getAllRTObjects(className, immRTObjects);
	rlistObjCnt = (uint32_t)immRTObjects.size();
	DEBUG_MWSA_REPLIST("getRTObjectsCount(%s): count = %u", className, (unsigned int)rlistObjCnt);
	immRTObjects.clear();
	LEAVE_MWSA_REPLIST();
	return retVal;
}
#ifndef UNIT_TEST
static  MafReturnT deleteAllRTObjects(const char* className)
#else
MafReturnT deleteAllRTObjects(const char* className)
#endif
{
	ENTER_MWSA_REPLIST();
	MafReturnT retVal = MafOk;
	std::vector<std::string> immRTObjects;

	retVal = getAllRTObjects(className, immRTObjects);
	for (std::vector<std::string>::iterator iter = immRTObjects.begin(); iter != immRTObjects.end(); ++iter){
		SaNameT *immObject = NULL;
		std::string objName= *iter;
		immObject = makeSaNameT(objName.c_str());
		retVal = deleteObject(immObject);
		saNameDelete(immObject, true);
		if (retVal != MafOk)
		{
			ERR_MWSA_REPLIST("deleteAllRTObjects(): ERROR: deleteRTObject %s  failed with error code: %d",  objName.c_str(), retVal);
			break;
		}
	}
	immRTObjects.clear();
	LEAVE_MWSA_REPLIST();
	return retVal;
}

#ifndef UNIT_TEST
static MafReturnT getAllRTObjects(const char* className, std::vector<std::string>& immRTObjects)
#else
MafReturnT getAllRTObjects(const char* className, std::vector<std::string>& immRTObjects)
#endif
{
	ENTER_MWSA_REPLIST();
	SaAisErrorT err = SA_AIS_OK;
	std::string classNameRT =  std::string(className) + sRuntimeIMMSuffix;
	char * pStringClassName = NULL;
	pStringClassName = (char*)classNameRT.c_str();
	SaImmSearchParametersT_2 searchParam;

	searchParam.searchOneAttr.attrName = (char*)SA_IMM_ATTR_CLASS_NAME;
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = (void*)&pStringClassName;
	SaImmSearchOptionsT searchOptions =  (SA_IMM_SEARCH_GET_NO_ATTR | SA_IMM_SEARCH_ONE_ATTR);
	SaImmSearchHandleT searchHandle;
	err = autoRetry_saImmOmSearchInitialize_2(rplistImmHandle, NULL, SA_IMM_SUBLEVEL, searchOptions,
	                                          &searchParam, NULL, &searchHandle);
	if (SA_AIS_OK != err) {
		ERR_MWSA_REPLIST("getAllRTObjects(%s) : autoRetry_saImmOmSearchInitialize_2 failed ", classNameRT.c_str());
		LEAVE_MWSA_REPLIST();
		return MafNotExist;
	}
	SaNameT objectName;
	SaImmAttrValuesT_2** attributes = NULL;
	while((err = autoRetry_saImmOmSearchNext_2(searchHandle, &objectName, (SaImmAttrValuesT_2 ***)&attributes)) == SA_AIS_OK)
	{
		std::string strObjectName = saAisNameBorrow(&objectName);
		immRTObjects.push_back(strObjectName);
	}

	DEBUG_MWSA_REPLIST("getAllRTObjects(%s) : autoRetry_saImmOmSearchNext_2 count =  %u", classNameRT.c_str(), (unsigned int)immRTObjects.size());

	err = autoRetry_saImmOmSearchFinalize(searchHandle);
	if(SA_AIS_OK != err){
		DEBUG_MWSA_REPLIST("getAllRTObjects(%s) : autoRetry_saImmOmSearchFinalize failed  %d ", classNameRT.c_str(), err);
	}
	LEAVE_MWSA_REPLIST();
	return MafOk;
}

// Use IMM and files to store Replicated Lists
MafReturnT OamSARList::listCreate(const MafMwSpiListNameT* listInstanceName, uint32_t dataBufferSize)
{
	ENTER_MWSA_REPLIST();
	MafReturnT maf_rc;
	unsigned int stateFlag = 0;
	const char* className = (char*) listInstanceName->value;

	if (!validateName(listInstanceName)) {
		LEAVE_MWSA_REPLIST();
		return MafNoResources;
	}

	if (dataBufferSize > rlist_maxsize) {
		ERR_MWSA_REPLIST("listCreate(%s): ERROR: dataBufferSize > rlist_maxsize (%u > %u)",
						className, (unsigned)dataBufferSize, (unsigned)rlist_maxsize);
		LEAVE_MWSA_REPLIST();
		return MafNoResources;
	}

	DEBUG_MWSA_REPLIST("listCreate() called with listInstanceName: %s, dataBufferSize: %u",
					(char*) listInstanceName->value, dataBufferSize);

	// ====================================================
	// Firstly, CREATE IMM class for the list
	// ====================================================
	DEBUG_MWSA_REPLIST("listCreate() calling createclass for the list: list name %s",
						(char*) listInstanceName->value);

	maf_rc = createclass((char*) listInstanceName->value, dataBufferSize);
	if (maf_rc != MafOk)
	{
		DEBUG_MWSA_REPLIST("listCreate(): createclass(): Fail when create Class in IMMM %s, return %d ",
				(char*) listInstanceName->value, maf_rc);
		LEAVE_MWSA_REPLIST();
		return MafNoResources; // The MAF RL SPI does not allow all return codes
	}

	if (replicateInIMM)
	{
		std::string className((const char*)listInstanceName->value);
		//1. Create corresponding RunTime Class for Replicated List Storage.
		className += sRuntimeIMMSuffix;
		//2. Create corresponding IMM Class
		maf_rc = createclass(className.c_str(), dataBufferSize, true);

		if (maf_rc != MafOk)
		{
			DEBUG_MWSA_REPLIST("listCreate(): createclass(): Fail when create Class in IMMM %s, return %d ", (char*) listInstanceName->value, maf_rc);
			LEAVE_MWSA_REPLIST();
			return MafNoResources; // The MAF RL SPI does not allow all return codes
		}
	}

	// ====================================================
	// Secondly, DETECT state of cluster
	// ====================================================
	maf_rc = detectClusterState(listInstanceName, &stateFlag);

	if(MafOk == maf_rc)
	{
		DEBUG_MWSA_REPLIST("listCreate(): Detected cluster status %u, doing nothing", stateFlag);
		if((unsigned int) CLUSTER_RT_OBJ_EXIST == stateFlag) return MafAlreadyExist;
	} else if(MafNotExist == maf_rc) {
		if(false == replicateInIMM){
			if(clearAlarmsOnClusterReboot){
				//Remove all files of sub-directory
				//Don't do it recursively for all sub-directories
				//because it will affect to another RT Obj and file system
				maf_rc = comsa_rplist_imm_removeAllFiles((char*) listInstanceName->value);
				if (MafOk == maf_rc)
				{
					DEBUG_MWSA_REPLIST("listCreate(): SUCCESS, deleted all files for list '%s'", (char*) listInstanceName->value);
				}
				else
				{
					ERR_MWSA_REPLIST("listCreate(): ERROR:failed to delete all files for list '%s', rc = %d",
							(char*) listInstanceName->value, (int) maf_rc);
				}
			}
		} else {
			maf_rc = deleteAllRTObjects((char*) listInstanceName->value);
			if (MafOk == maf_rc)
			{
				DEBUG_MWSA_REPLIST("listCreate(): SUCCESS, deleted all objects for list '%s'", (char*) listInstanceName->value);
			}
			else
			{
				ERR_MWSA_REPLIST("listCreate(): ERROR:failed to delete all objects for list '%s', rc = %d",
						(char*) listInstanceName->value, (int) maf_rc);
			}
		}
	}
	else
	{
		ERR_MWSA_REPLIST("listCreate(%s): ERROR: detectClusterState() FAILED return = %d", className, (int) maf_rc);
		//In future, a relevant action should be added here
	}
	if(false == replicateInIMM)
	{
		// =====================================================
		// Thirdly, CREATE the file system for Replicated Lists
		// =====================================================
		/* Try to create the base directory for all replicated lists under the cluster clear location */
		if(MafOk == createDir("rplist", "/"))
		{
			DEBUG_MWSA_REPLIST("listCreate(): Successfully created base directory for rplist");
		}
		else
		{
			ERR_MWSA_REPLIST("listCreate(): ERROR: Could not create base directory for rplist");
			LEAVE_MWSA_REPLIST();
			return MafNoResources;    // The MAF RL SPI does not allow all return codes
		}

		/* Try to create sub directory with the list name */
		if(MafOk == createDir(className, strRpListPath))
		{
			DEBUG_MWSA_REPLIST("listCreate(): Successfully created sub-directory %s under rplist", className);
		}
		else
		{
			ERR_MWSA_REPLIST("listCreate(): ERROR: Could not create sub-repository %s under rplist", className);
			LEAVE_MWSA_REPLIST();
			return MafNoResources;    // The MAF RL SPI does not allow all return codes
		}
	}

	// ====================================================
	// Finally, CREATE the OBJECT for the list
	// ====================================================

	// RDN
	SaNameT rdnName;
	classToObj(className, &rdnName);
	SaImmAttrValueT attrValue1 = &rdnName;
	SaImmAttrValuesT_2 ComsaRlistClassRdnValue = {
		strRlistClassRdn,				// name
		SA_IMM_ATTR_SANAMET,			// type
		1,								// number of values
		(SaImmAttrValueT*) &attrValue1	// pointer to the value
	};

	// Block size attribute value
	SaImmAttrValueT attrValue2 = &dataBufferSize;
	SaImmAttrValuesT_2 comsaRlistBlockSizeValue = {
		strRlistBlockSize,				// name
		SA_IMM_ATTR_SAUINT32T,			// type
		1,								// number of values
		(SaImmAttrValueT*) &attrValue2	// pointer to the value
	};

	// Last id
	SaUint32T comsaRlistLastId = 0;
	SaImmAttrValueT attrValue3 = &comsaRlistLastId;
	SaImmAttrValuesT_2 comsaRlistLastIdValue = {
		strRlistLastId,
		SA_IMM_ATTR_SAUINT32T,
		1,
		(SaImmAttrValueT*) &attrValue3
	};

	// this is the current number of elements in the list
	SaUint32T comsaRlistNumElementsVal = 0;
	SaImmAttrValueT attrValue5 = &comsaRlistNumElementsVal;
	SaImmAttrValuesT_2 comsaRlistNumElementsValue = {
		strRlistNumElements,
		SA_IMM_ATTR_SAUINT32T,
		1,
		(SaImmAttrValueT*) &attrValue5
	};

	// Combine all the attribute values in one NULL terminated array
	const SaImmAttrValuesT_2* attrValuesForObject[] = {&comsaRlistBlockSizeValue,
													&comsaRlistLastIdValue,
													&comsaRlistNumElementsValue,
													&ComsaRlistClassRdnValue,
													NULL};

	maf_rc = createObject(className, attrValuesForObject);
	if (maf_rc != MafOk && maf_rc != MafAlreadyExist)
	{
		ERR_MWSA_REPLIST("listCreate(): ERROR: createObject failed for class '%s' with error code: %d",
						className, maf_rc);
		LEAVE_MWSA_REPLIST();
		return MafNoResources;    // The MAF RL SPI does not allow all return codes
	}

	if (false == replicateInIMM) {
		if(!clearAlarmsOnClusterReboot) {
			maf_rc = restore_immData(listInstanceName);
			if(maf_rc == MafOk || maf_rc == MafAlreadyExist )
			{
				LEAVE_MWSA_REPLIST();
				return maf_rc;
			} else {
				ERR_MWSA_REPLIST("listCreate():ERROR: restore_immData failed for class '%s' with error code: %d",
						className, maf_rc);
				LEAVE_MWSA_REPLIST();
				return MafNoResources;    // The MAF RL SPI does not allow all return codes
			}
		}
	}
	LEAVE_MWSA_REPLIST();
	return MafOk;
}


MafReturnT OamSARList::listDelete(const MafMwSpiListNameT* listInstanceName)
{
	ENTER_MWSA_REPLIST();
	MafReturnT rc = MafOk;

	if (!validateName(listInstanceName)) {
		LEAVE_MWSA_REPLIST();
		return MafNotExist;
	}

	DEBUG_MWSA_REPLIST("listDelete called with listInstanceName: %s", (char*) listInstanceName->value);

	// delete the IMM object for the list
	SaNameT rdnName;
	classToObj((const char*) listInstanceName->value, &rdnName);

	rc = deleteObject(&rdnName);
	if (MafOk != rc)
	{
		ERR_MWSA_REPLIST("listDelete: ERROR: deleteObject() failed for list '%s', rc = %d",
						(char*) listInstanceName->value, (int) rc);
		return MafNotExist;    // The MAF RL SPI does not allow all return codes
	}
	else
	{
		DEBUG_MWSA_REPLIST("listDelete: deleteObject() SUCCESS for list '%s'",
						(char*) listInstanceName->value);
	}

	// delete the IMM class
	rc = deleteclass((char*) listInstanceName->value);
	if (MafOk != rc)
	{
		ERR_MWSA_REPLIST("listDelete: ERROR: deleteclass() failed for list '%s', rc = %d",
						(char*) listInstanceName->value, (int) rc);
		return MafNotExist;    // The MAF RL SPI does not allow all return codes
	}
	else
	{
		DEBUG_MWSA_REPLIST("listDelete: deleteclass() SUCCESS for list '%s'",
						(char*) listInstanceName->value);
	}
	if( false == replicateInIMM ) {
		// delete all the files and the directory for this list in the file system
		rc = comsa_rplist_imm_removeAllFiles((char*) listInstanceName->value);
		if (MafOk != rc)
		{
			ERR_MWSA_REPLIST("listDelete: ERROR: removeFile() failed to delete all files for list '%s', rc = %d",
					(char*) listInstanceName->value, (int) rc);
			return MafNotExist;    // The MAF RL SPI does not allow all return codes
		}
		else
		{
			DEBUG_MWSA_REPLIST("listDelete: removeFile() SUCCESS, deleted all files for list '%s'",
					(char*) listInstanceName->value);
		}

		rc = comsa_rplist_imm_deleteDir((char*) listInstanceName->value);
		if (MafOk != rc)
		{
			ERR_MWSA_REPLIST("listDelete: ERROR: deleteDir() failed to delete directory '%s'",
					(char*) listInstanceName->value);
			return MafNotExist;    // The MAF RL SPI does not allow all return codes
		}
		else
		{
			DEBUG_MWSA_REPLIST("listDelete: removeFile() SUCCESS, deleted directory '%s'",
					(char*) listInstanceName->value);
		}
	} else {
		std::string  classNameRT =  std::string((char*)listInstanceName->value) + sRuntimeIMMSuffix;
		rc = deleteAllRTObjects((char*) listInstanceName->value);
		if (MafOk == rc)
		{
			DEBUG_MWSA_REPLIST("listDelete(): SUCCESS, deleted all objects for list '%s'", classNameRT.c_str());
		}
		else
		{
			ERR_MWSA_REPLIST("listDelete(): ERROR:failed to delete all objects for list '%s', rc = %d",
					classNameRT.c_str(), (int) rc);
			return MafNotExist;    // The MAF RL SPI does not allow all return codes
		}
		// delete the IMM class
		rc = deleteclass(classNameRT.c_str());
		if (MafOk != rc)
		{
			ERR_MWSA_REPLIST("listDelete: ERROR: deleteclass() failed for list '%s', rc = %d",
						classNameRT.c_str(), (int) rc);
			return MafNotExist;    // The MAF RL SPI does not allow all return codes
		}
		else
		{
			DEBUG_MWSA_REPLIST("listDelete: deleteclass() SUCCESS for list '%s'",
						classNameRT.c_str());
		}
	}

	LEAVE_MWSA_REPLIST();
	return MafOk;
}


MafReturnT OamSARList::listClear(const MafMwSpiListNameT* listInstanceName)
{
	ENTER_MWSA_REPLIST();
	MafReturnT rc = MafOk;

	if (!validateName(listInstanceName)) {
		LEAVE_MWSA_REPLIST();
		return MafNotExist;
	}

	DEBUG_MWSA_REPLIST("listClear called with listInstanceName: %s", (char*) listInstanceName->value);

	/*
	 * Update the IMM object for the number of elements in the list to 0.
	 */
	rc = setObjectUiAttribute((const char*)listInstanceName->value, strRlistNumElements, 0);
	if(rc != MafOk)
	{
		ERR_MWSA_REPLIST("listClear: ERROR: Could not update attribute of RT Obj: %s", (const char*)listInstanceName->value);
		LEAVE_MWSA_REPLIST();
		return MafNotExist;    // The MAF RL SPI does not allow all return codes
	}

	if( false == replicateInIMM ) {
		// delete all the files and the directory for this list in the file system
		rc = comsa_rplist_imm_removeAllFiles((char*) listInstanceName->value);
		if (MafOk == rc)
		{
			DEBUG_MWSA_REPLIST("listClear: removeFile() SUCCESS, deleted all files for list '%s'", (char*) listInstanceName->value);
		}
		else
		{
			ERR_MWSA_REPLIST("listClear: ERROR: removeFile() failed to delete all files for list '%s', rc = %d", (char*) listInstanceName->value, (int) rc);
			LEAVE_MWSA_REPLIST();
			return MafNotExist;    // The MAF RL SPI does not allow all return codes
		}
	}else{
		std::string  classNameRT =  std::string((char*)listInstanceName->value) + sRuntimeIMMSuffix;
		rc = deleteAllRTObjects((char*) listInstanceName->value);
		if (MafOk == rc)
		{
			DEBUG_MWSA_REPLIST("listClear(): SUCCESS, deleted all objects for list '%s'", classNameRT.c_str());
		}
		else
		{
			ERR_MWSA_REPLIST("listClear(): ERROR:failed to delete all objects for list '%s', rc = %d",
					classNameRT.c_str(), (int) rc);
			return MafNotExist;    // The MAF RL SPI does not allow all return codes
		}
	}

	LEAVE_MWSA_REPLIST();
	return MafOk;
}


MafReturnT OamSARList::listGetSize(const MafMwSpiListNameT* listInstanceName, uint32_t* listSize)
{
	ENTER_MWSA_REPLIST();
	MafReturnT maf_rc = MafOk;

	if (!validateName(listInstanceName)) {
		LEAVE_MWSA_REPLIST();
		return MafNotExist;
	}

	DEBUG_MWSA_REPLIST("listGetSize called with listInstanceName: %s", (char*) listInstanceName->value);

	unsigned int nListSize = 0;
	maf_rc = getObjectUiAttribute((char*) listInstanceName->value, strRlistNumElements, &nListSize);
	if (MafOk == maf_rc)
	{
		DEBUG_MWSA_REPLIST("listGetSize: getObjectUiAttribute returned nListSize: %u", nListSize);
		*listSize = nListSize;
		DEBUG_MWSA_REPLIST("listGetSize: returned listSize: %d", *listSize);
	}
	else
	{
		ERR_MWSA_REPLIST("listGetSize(%s): ERROR: getObjectUiAttribute failed with %d", (char*) listInstanceName->value, (int) maf_rc);
		LEAVE_MWSA_REPLIST();
		return MafNotExist;    // The MAF RL SPI does not allow all return codes
	}

	LEAVE_MWSA_REPLIST();
	return MafOk;
}


MafReturnT OamSARList::listIsEmpty(const MafMwSpiListNameT* listInstanceName, bool* listEmpty)
{
	ENTER_MWSA_REPLIST();
	MafReturnT maf_rc = MafOk;

	if (!validateName(listInstanceName)) {
		LEAVE_MWSA_REPLIST();
		return MafNotExist;
	}

	DEBUG_MWSA_REPLIST("listIsEmpty called with listInstanceName: %s", (char*) listInstanceName->value);

	unsigned int nNumElements = 0;
	maf_rc = getObjectUiAttribute((char*) listInstanceName->value, strRlistNumElements, &nNumElements);
	if (MafOk == maf_rc)
	{
		DEBUG_MWSA_REPLIST("listIsEmpty: getObjectUiAttribute returned nNumElements: %u", nNumElements);
		*listEmpty = (nNumElements == 0);
		DEBUG_MWSA_REPLIST("listIsEmpty: returned listEmpty: %d", *listEmpty);
	}
	else
	{
		if (MafNotExist == maf_rc)
		{
			DEBUG_MWSA_REPLIST("listIsEmpty: getObjectUiAttribute() return %d", (int) maf_rc);
		}
		else
		{
			ERR_MWSA_REPLIST("listIsEmpty: ERROR: getObjectUiAttribute failed with %d", (int) maf_rc);
		}
		LEAVE_MWSA_REPLIST();
		return MafNotExist;    // The MAF RL SPI does not allow all return codes
	}

#ifndef UNIT_TEST
	// HT70246: verify the file system
	if(!(*listEmpty))
	{
		if( false == replicateInIMM ) {
			DEBUG_MWSA_REPLIST("listIsEmpty: checking file system");
			char *fullPath = comsa_rplist_imm_getfullPath((char*) listInstanceName->value);
			if(fullPath)
			{
				struct dirent *pDirent;
				DIR *pDir;
				pDir = opendir(fullPath);
				if (pDir != NULL)
				{
					int len = 0;
					bool isFolderEmpty = true;
					while ((pDirent = readdir(pDir)) != NULL)
					{
						len = strlen (pDirent->d_name);
						if (len >= 4)
						{
							if (strcmp (strBin, &(pDirent->d_name[len - 4])) == 0)
							{
								// Only check for the file of Fm_Status_Section. It may be corrupted
								if (strncmp((char*)listInstanceName->value, FM_STATUS_SECTION, sizeof(FM_STATUS_SECTION)) == 0)
								{
									char *fullName = (char*)calloc(MAX_PATH_DATA_LENGTH,sizeof(char));
									strcpy(fullName,fullPath);
									if (fullName[strlen(fullName)-1] != '/') {
										strcat(fullName, "/");
									}
									strcat(fullName, pDirent->d_name);

									struct stat file_status;
									if( stat( fullName, &file_status ) != 0 ) {
										// File exists but cannot stat
										WARN_MWSA_REPLIST ("listIsEmpty: Couldn't stat file [%s]", fullName);
									} else {
										if (file_status.st_size == 0)
										{
											// File size is 0, remove it. Break the loop, isFolderEmpty is true
											WARN_MWSA_REPLIST("listIsEmpty: File size of [%s] is 0. Removing...",fullName);
											if(0 != remove(fullName))
											{
												WARN_MWSA_REPLIST("listIsEmpty(): WARN: Could not remove the file '%s' ", fullName);
											}
											else
											{
												DEBUG_MWSA_REPLIST("removeFile(): Successfully removed the file '%s' ", fullName);
											}
											free(fullName);
											break;
										}
									}
									free(fullName);
								}
								isFolderEmpty = false;
								break;
							}
						}
					}
					*listEmpty = isFolderEmpty;
					if(isFolderEmpty)
					{
						ERR_MWSA_REPLIST("listIsEmpty: ERROR: the directory %s is empty", fullPath);
					}
					closedir(pDir);
				}
				else
				{
					// Unexpected error, folder is empty while nNumElements != 0
					LOG_MWSA_REPLIST("listIsEmpty: the directory %s is not exist. Trying to re-create it", fullPath);
					*listEmpty = true;
					/* Storage clear area being deleted during system restore. Trying to re-create sub directory with the list name */
					if(MafOk != createDir((char*) listInstanceName->value, strRpListPath))
					{
						ERR_MWSA_REPLIST("listIsEmpty(): ERROR: Could not create sub-repository %s under rplist", (char*) listInstanceName->value);
						LEAVE_MWSA_REPLIST();
						return MafNoResources;
					}
				}
				// Data is corrupt, reset all attributes
				if(*listEmpty)
				{
					setObjectUiAttribute((char*) listInstanceName->value, strRlistNumElements, 0);
					setObjectUiAttribute((char*) listInstanceName->value, strRlistLastId, 0);
				}
				free(fullPath);
			}
		}else{
			if (strncmp((char*)listInstanceName->value, FM_STATUS_SECTION, sizeof(FM_STATUS_SECTION)) == 0) {
				std::string classNameRT = std::string((char*) listInstanceName->value) + sRuntimeIMMSuffix;
				SaImmAttrValuesT_2 **returnedAttrValues = 0;
				SaNameT objName;
				classToObj(classNameRT.c_str(), &objName);
				maf_rc = readObject(&objName, &returnedAttrValues);
				if( MafNotExist == maf_rc ) {// Data is corrupt, reset all attributes
					LOG_MWSA_REPLIST("listIsEmpty(): object %s not exist", saNameGet(&objName));
					setObjectUiAttribute((char*) listInstanceName->value, strRlistNumElements, 0);
					setObjectUiAttribute((char*) listInstanceName->value, strRlistLastId, 0);
				}
			}
		}
	}
#endif

	LEAVE_MWSA_REPLIST();
	return MafOk;
}


/*********************************************************************************************************
 *
 *	1). Read the list last_id from IMM, increment it to obtain the name for the new item being added.
 *	2). Create the file for the new list item
 *	3). If step (2) was successful then update the list object in IMM:
 *          comsaRlistLastId, comsaRlistNumElements
 */
MafReturnT OamSARList::listPushBack(const MafMwSpiListNameT* listInstanceName,
									MafMwSpiListItemNameT* listItemName, /* OUT parameter */
									void* newItemDataBuffer)
{
	ENTER_MWSA_REPLIST();
	MafReturnT maf_rc = MafOk;

	if (!validateName(listInstanceName)) {
		LEAVE_MWSA_REPLIST();
		return MafNotExist;
	}
	if (NULL == newItemDataBuffer) {
		ERR_MWSA_REPLIST("maf_comsa_imm_listPushBack(%s): ERROR: newItemDataBuffer is NULL", (char*) listInstanceName->value);
		LEAVE_MWSA_REPLIST();
		return MafNotExist;
	}

	DEBUG_MWSA_REPLIST("listPushBack called with listInstanceName: %s", (char*) listInstanceName->value);

#ifdef RPLIST_DEBUG_CODE_ENABLE
	uint32_t listSize = 0;

	DEBUG_MWSA_REPLIST("maf_comsa_imm_listPushBack calling listGetSize()...");
	MafReturnT rc_dbg = listGetSize(listInstanceName, &listSize);
	if (MafOk != rc_dbg)
	{
		ERR_MWSA_REPLIST("maf_comsa_imm_listPushBack(%s): ERROR: listGetSize() failed, rc = %d", (char*) listInstanceName->value, (int) rc_dbg);
	}
	DEBUG_MWSA_REPLIST("maf_comsa_imm_listPushBack: the list size before adding the item: %d", (int) listSize);
#endif /* RPLIST_DEBUG_CODE_ENABLE */

	unsigned int lastId = 0;
	unsigned int numItems = 0;
	bool pathExist = true;

	if(false == replicateInIMM) {
		char* fullPath = comsa_rplist_imm_getfullPath((char*) listInstanceName->value);

		if(fullPath)
		{
			DIR *pDir = opendir(fullPath);
			// Check if the path exist
			if (pDir != NULL)
			{
				closedir(pDir);
			}
			else
			{
				pathExist = false;
				LOG_MWSA_REPLIST("maf_comsa_imm_listPushBack: the directory %s is not exist. Trying to re-create it", fullPath);
				/* Storage clear area being deleted during system restore. Trying to re-create sub directory with the list name */
				if(MafOk != createDir((char*) listInstanceName->value, strRpListPath))
				{
					ERR_MWSA_REPLIST("maf_comsa_imm_listPushBack(): ERROR: Could not create sub-repository %s under rplist", (char*) listInstanceName->value);
					LEAVE_MWSA_REPLIST();
					return MafNoResources;
				}
				// Data is corrupt, reset all attributes
				setObjectUiAttribute((char*) listInstanceName->value, strRlistNumElements, 0);
				setObjectUiAttribute((char*) listInstanceName->value, strRlistLastId, 0);
			}
			free(fullPath);
		}
	}

	if (pathExist)
	{
		//DEBUG_MWSA_REPLIST("getObjectUiAttribute called with lastId: %d %p", lastId, &lastId);
		maf_rc = getObjectUiAttribute((char*) listInstanceName->value, strRlistLastId, &lastId);
		if (MafOk == maf_rc)
		{
			DEBUG_MWSA_REPLIST("maf_comsa_imm_listPushBack: getObjectUiAttribute returned lastId: %u", lastId);
		}
		else
		{
			ERR_MWSA_REPLIST("maf_comsa_imm_listPushBack(%s): ERROR: getObjectUiAttribute failed with %d", (char*) listInstanceName->value, (int) maf_rc);
			LEAVE_MWSA_REPLIST();
			return MafNotExist;    // The MAF RL SPI does not allow all return codes
		}

		maf_rc = getObjectUiAttribute((char*) listInstanceName->value, strRlistNumElements, &numItems);
		if (MafOk == maf_rc)
		{
			DEBUG_MWSA_REPLIST("maf_comsa_imm_listPushBack: getObjectUiAttribute returned numItems: %d", numItems);
		}
		else
		{
			ERR_MWSA_REPLIST("maf_comsa_imm_listPushBack(%s): ERROR: getObjectUiAttribute failed with %d", (char*) listInstanceName->value, (int) maf_rc);
			LEAVE_MWSA_REPLIST();
			return MafNotExist;    // The MAF RL SPI does not allow all return codes
		}
	}

	/*
	 * Create the name for the list item - convert (last_id + 1) from int to to string
	 */
	if (NULL == listItemName)
	{
		ERR_MWSA_REPLIST("maf_comsa_imm_listPushBack(%s): ERROR: called with listItemName = NULL", (char*) listInstanceName->value);
		LEAVE_MWSA_REPLIST();
		return MafInvalidArgument;
	}
	else
	{
		// Increment the last ID and check for reaching the limit ULONG_MAX = 4294967295
		lastId++;
		if (ULONG_MAX == lastId)
		{
			ERR_MWSA_REPLIST("maf_comsa_imm_listPushBack(%s): ERROR: lastId reached the ULONG_MAX limit: %u", (char*) listInstanceName->value, lastId);
			LEAVE_MWSA_REPLIST();
			return MafNoResources; // The MAF RL SPI does not allow all return codes
		}
		else
		{
			listItemName->length = snprintf((char*) listItemName->value, MAX_ITEM_NAME_LEN, "%llu", (unsigned long long) lastId);
		}
	}

	if (false == replicateInIMM) {

		/*
		 * Write the  binary data to file
		 */
		DEBUG_MWSA_REPLIST("maf_comsa_imm_listPushBack: calling autoRetry_writeToFile");
		maf_rc = autoRetry_writeToFile(listInstanceName,listItemName,
				newItemDataBuffer);

		if(MafOk == maf_rc) {
			DEBUG_MWSA_REPLIST("maf_comsa_imm_listPushBack: Successfully created a binary file for the RP list item: %s", listInstanceName->value);
		} else {
			ERR_MWSA_REPLIST("maf_comsa_imm_listPushBack: ERROR: Could not create a binary file: %s", listInstanceName->value);
			// HT70246: if anything goes wrong here, try to delete the (created) file since it's corrupted.
			// Okay, this case nothing is written to IMM yet, so no need to check for consistency between IMM and NFS
			removeFile((const char*) listItemName->value, (char*) listInstanceName->value);
			LEAVE_MWSA_REPLIST();
			return MafNoResources;    // The MAF RL SPI does not allow all return codes
		}
	} else {
		maf_rc = addRlistRTObject((const char*)listInstanceName->value, lastId, newItemDataBuffer);

		if ( MafAlreadyExist == maf_rc ) {
			DEBUG_MWSA_REPLIST("maf_comsa_imm_listPushBack(): object already exists. Modifying...");
			std::ostringstream ostr;
			ostr << lastId;
			maf_rc = modifyRlistRTObject((const char*)listInstanceName->value, ostr.str(), newItemDataBuffer);
		}

		if ( MafOk == maf_rc )
		{
			DEBUG_MWSA_REPLIST("maf_comsa_imm_listPushBack: Successfully added an object: %s, lastId=%d ", listInstanceName->value,lastId);
		} else {
			WARN_MWSA_REPLIST("maf_comsa_imm_listPushBack: ERROR: Could not add an object: %s", listInstanceName->value);
			LEAVE_MWSA_REPLIST();
			return MafNoResources;    // The MAF RL SPI does not allow all return codes
		}
	}

	/*
	 * Update the IMM object for the list
	 */
	if(MafOk == setObjectUiAttribute((const char*)listInstanceName->value, strRlistNumElements, numItems + 1))
	{
		DEBUG_MWSA_REPLIST("maf_comsa_imm_listPushBack: Successfully updated attribute of RT Obj: %s", (const char*)listInstanceName->value);
	}
	else
	{
		ERR_MWSA_REPLIST("maf_comsa_imm_listPushBack: ERROR: Could not update attribute of RT Obj: %s", (const char*)listInstanceName->value);
		LEAVE_MWSA_REPLIST();
		return MafNotExist;    // The MAF RL SPI does not allow all return codes
	}

#ifdef RPLIST_DEBUG_CODE_ENABLE
	maf_rc = getObjectUiAttribute((char*) listInstanceName->value, strRlistNumElements, &numItems);
	if (MafOk == maf_rc)
	{
		DEBUG_MWSA_REPLIST("maf_comsa_imm_listPushBack: getObjectUiAttribute returned numItems: %d", numItems);
	}
	else
	{
		ERR_MWSA_REPLIST("maf_comsa_imm_listPushBack(%s): ERROR: getObjectUiAttribute failed with %d", (char*) listInstanceName->value, (int) maf_rc);
		LEAVE_MWSA_REPLIST();
		return MafNotExist;    // The MAF RL SPI does not allow all return codes
	}
#endif /* RPLIST_DEBUG_CODE_ENABLE */

	maf_rc = setObjectUiAttribute((const char*)listInstanceName->value, strRlistLastId, lastId);
	if (maf_rc != MafOk)
	{
		ERR_MWSA_REPLIST("maf_comsa_imm_listPushBack: ERROR: Could not update attribute of RT Obj: %s", (const char*)listInstanceName->value);
		LEAVE_MWSA_REPLIST();
		return MafNotExist;    // The MAF RL SPI does not allow all return codes
	}

#ifdef RPLIST_DEBUG_CODE_ENABLE
	maf_rc = getObjectUiAttribute((char*) listInstanceName->value, strRlistLastId, &lastId);
	if (MafOk == maf_rc)
	{
		DEBUG_MWSA_REPLIST("maf_comsa_imm_listPushBack: getObjectUiAttribute returned numItems: %u", lastId);
	}
	else
	{
		ERR_MWSA_REPLIST("maf_comsa_imm_listPushBack(%s): ERROR: getObjectUiAttribute failed with %d", (char*) listInstanceName->value, (int) maf_rc);
		LEAVE_MWSA_REPLIST();
		return maf_rc;
	}

	rc_dbg = maf_comsa_imm_listGetSize(listInstanceName, &listSize);
	if (MafOk != rc_dbg)
	{
		ERR_MWSA_REPLIST("maf_comsa_imm_listPushBack(%s): ERROR: maf_comsa_imm_listGetSize() failed, rc = %d", (char*) listInstanceName->value, (int) rc_dbg);
	}
	DEBUG_MWSA_REPLIST("maf_comsa_imm_listPushBack: the list size after adding the item: %d", (int) listSize);
#endif /* RPLIST_DEBUG_CODE_ENABLE */

	LEAVE_MWSA_REPLIST();
	return MafOk;
}


MafReturnT OamSARList::listPopBack(const MafMwSpiListNameT* listInstanceName)
{
	ENTER_MWSA_REPLIST();
	MafReturnT rc = MafOk;

	if (!validateName(listInstanceName)) {
		LEAVE_MWSA_REPLIST();
		return MafNotExist;
	}

	DEBUG_MWSA_REPLIST("listPopBack called with listInstanceName: %s", (char*) listInstanceName->value);

	// Get the last ID from IMM
#ifndef UNIT_TEST
		unsigned int nLastId = 0;
#else
		/* nLastId is assigned to 1 because there is no IMM access */
		unsigned int nLastId = 1;
#endif
	rc = getObjectUiAttribute((char*) listInstanceName->value, strRlistLastId, &nLastId);
	if (MafOk == rc)
	{
		DEBUG_MWSA_REPLIST("listPopBack: getObjectUiAttribute returned nLastId: %u", nLastId);
	}
	else
	{
		ERR_MWSA_REPLIST("listPopBack(%s): ERROR: getObjectUiAttribute failed with %d", (char*) listInstanceName->value, (int) rc);
		LEAVE_MWSA_REPLIST();
		return MafNotExist;    // The MAF RL SPI does not allow all return codes
	}

	char listItemName[MAX_ITEM_NAME_LEN];
	sprintf(listItemName, "%u", nLastId);
	if (!replicateInIMM)
	{
		// Check if the item exists in the file storage. If not then return MafNotExist
		rc = findFile(listItemName, (const char*) listInstanceName->value);
		if (MafOk == rc)
		{
			DEBUG_MWSA_REPLIST("listPopBack: calling removeFile");
			// delete the file for this list in the file system
			rc = removeFile(listItemName, (char*) listInstanceName->value);
			if (MafOk != rc)
			{
				ERR_MWSA_REPLIST("listPopBack: ERROR: removeFile() failed to delete file %s for list '%s', rc = %d",
								listItemName, (char*) listInstanceName->value, (int) rc);
				LEAVE_MWSA_REPLIST();
				return MafNotExist;    // The MAF RL SPI does not allow all return codes
			}
			else
			{
				DEBUG_MWSA_REPLIST("listPopBack: removeFile() SUCCESS, deleted file '%s' for list '%s'",
								listItemName, (char*) listInstanceName->value);
			}
		}
		else
		{
			ERR_MWSA_REPLIST("listPopBack: ERROR: findFile() failed to find file %s for list '%s', rc = %d",
				listItemName, (char*) listInstanceName->value, (int) rc);
			return MafNotExist;
		}
	}
	else
	{
		rc = deleteRTObject(std::string((char*)(listInstanceName->value)), std::string(listItemName));
		if (rc != MafOk)
		{
			ERR_MWSA_REPLIST("listPopBack(): ERROR: deleteRTObject failed with error code: %d", rc);
			LEAVE_MWSA_REPLIST();
			return MafNotExist;
		}
		else
		{
			DEBUG_MWSA_REPLIST("listPopBack(): INFO : SUCCESS");
		}
	}

	// Update the number of items in the list
	unsigned int numItems = 0;
	rc = getObjectUiAttribute((char*) listInstanceName->value, strRlistNumElements, &numItems);
	if (MafOk == rc)
	{
		DEBUG_MWSA_REPLIST("listPopBack: getObjectUiAttribute returned numItems: %u", numItems);
	}
	else
	{
		ERR_MWSA_REPLIST("listPopBack(%s): ERROR: getObjectUiAttribute failed with %d", (char*) listInstanceName->value, (int) rc);
		LEAVE_MWSA_REPLIST();
		return MafNotExist;    // The MAF RL SPI does not allow all return codes
	}

	numItems--;
	rc = setObjectUiAttribute((char*) listInstanceName->value, strRlistNumElements, numItems);
	if (MafOk == rc)
	{
		DEBUG_MWSA_REPLIST("listPopBack: setObjectUiAttribute returned numItems: %u", numItems);
	}
	else
	{
		ERR_MWSA_REPLIST("listPopBack(%s): ERROR: setObjectUiAttribute failed with %d", (char*) listInstanceName->value, (int) rc);
		LEAVE_MWSA_REPLIST();
		return MafNotExist;    // The MAF RL SPI does not allow all return codes
	}

	// Update the last ID in the list
	rc = MafNotExist;
	while( MafOk != rc &&  0 < nLastId)
	{
		nLastId --;
		if (replicateInIMM) {
			SaImmAttrValuesT_2 **attrValues = NULL;
			SaNameT *immObject = NULL;
			std::ostringstream ostr;
			ostr << nLastId;
			rc = convertToRTObjectName(std::string((char*)(listInstanceName->value)), ostr.str(), &immObject);
			rc = readObject(immObject, &attrValues);
			saNameDelete(immObject, true);
		}
		else
		{
			sprintf(listItemName, "%u", nLastId);
			rc = findFile(listItemName, (const char*) listInstanceName->value);
		}
	}
	if ( 0 > nLastId )
	{
		ERR_MWSA_REPLIST("listPopBack(%s): ERROR:  nLastId < 0", (char*) listInstanceName->value);
		rc = MafNotExist;
		LEAVE_MWSA_REPLIST();
		return rc;
	}
	rc = setObjectUiAttribute((char*) listInstanceName->value, strRlistLastId, nLastId);
	if (MafOk == rc)
	{
		DEBUG_MWSA_REPLIST("listPopBack: setObjectUiAttribute returned nLastId: %u", nLastId);
	}
	else
	{
		ERR_MWSA_REPLIST("listPopBack(%s): ERROR: setObjectUiAttribute failed with %d", (char*) listInstanceName->value, (int) rc);
		LEAVE_MWSA_REPLIST();
		return MafNotExist;    // The MAF RL SPI does not allow all return codes
	}

	LEAVE_MWSA_REPLIST();
	return rc;
}


MafReturnT OamSARList::listEraseItem(const MafMwSpiListNameT* listInstanceName, const MafMwSpiListItemNameT* listItemName)
{
	ENTER_MWSA_REPLIST();
	MafReturnT rc = MafOk;

	if (!validateName(listInstanceName) ||
		!validateName(listItemName)) {
		LEAVE_MWSA_REPLIST();
		return MafNotExist;
	}
	DEBUG_MWSA_REPLIST("listEraseItem called with listInstanceName: %s and listitemName [%s]",
							(char*) listInstanceName->value, (char *)listItemName->value);

	if (!replicateInIMM)
	{
		// Check if the item exists in the file storage. If not then return MafNotExist
		rc = findFile((const char*) listItemName->value, (const char*) listInstanceName->value);
		if (MafOk == rc)
		{
			DEBUG_MWSA_REPLIST("listEraseItem: calling removeFile");
			// delete the file for this list in the file system
			rc = removeFile((const char*) listItemName->value, (char*) listInstanceName->value);
			if (MafOk != rc)
			{
				ERR_MWSA_REPLIST("listEraseItem: ERROR: removeFile() failed to delete file %s for list '%s', rc = %d",
						(char*) listItemName->value, (char*) listInstanceName->value, (int) rc);
				LEAVE_MWSA_REPLIST();
				return MafNotExist;    // The MAF RL SPI does not allow all return codes
			}
			else
			{
				DEBUG_MWSA_REPLIST("listEraseItem: removeFile() SUCCESS, deleted file '%s' for list '%s'",
						(char*) listItemName->value, (char*) listInstanceName->value);
			}

		}
		else
		{
			rc = MafNotExist;
			LEAVE_MWSA_REPLIST();
			return rc;
		}
	}
	else
	{
		rc = deleteRTObject(std::string((char*)(listInstanceName->value)), (std::string)(char*)listItemName->value);
		if (rc == MafNotExist)
		{
			ERR_MWSA_REPLIST("listEraseItem(): ERROR: deleteRTObject failed with error code: %d", rc);
			LEAVE_MWSA_REPLIST();
			return rc;
		}
		else
		{
			DEBUG_MWSA_REPLIST("listEraseItem(): INFO: deletion of Object SUCCESS");
		}
	}
	// Update the number of items in the list
	unsigned int numItems = 0;
	rc = getObjectUiAttribute((char*) listInstanceName->value, strRlistNumElements, &numItems);
	if (MafOk == rc)
	{
		DEBUG_MWSA_REPLIST("listEraseItem: getObjectUiAttribute returned numItems: %u", numItems);
	}
	else
	{
		ERR_MWSA_REPLIST("listEraseItem(%s): ERROR: getObjectUiAttribute failed with %d", (char*) listInstanceName->value, (int) rc);
		LEAVE_MWSA_REPLIST();
		return MafNotExist;    // The MAF RL SPI does not allow all return codes
	}

	numItems--;
	if(numItems >= 0)
	{
		rc = setObjectUiAttribute((char*) listInstanceName->value, strRlistNumElements, numItems);
		if (MafOk == rc)
		{
			DEBUG_MWSA_REPLIST("listEraseItem: setObjectUiAttribute returned numItems: %u", numItems);
		}
		else
		{
			ERR_MWSA_REPLIST("listEraseItem(%s): ERROR: setObjectUiAttribute failed with %d", (char*) listInstanceName->value, (int) rc);
			LEAVE_MWSA_REPLIST();
			return MafNotExist;    // The MAF RL SPI does not allow all return codes
		}
	}
	LEAVE_MWSA_REPLIST();
	return rc;
}


/* *********************************************************************************
 *	1). Create iterator structure instance of type RLIterator
 *	2). Set the 'iterator->index' to 1 (to point to the first list item)
 *	3). If the list is empty, set the listInstanceFrontRef to NULL
 *	4). Set the 'iterator-size' to the list item data size. (Note: In CKPT is used for something else)
 *	5). Pass a pointer to the structure instance back to COM
 */

MafReturnT OamSARList::listGetFrontRef(const MafMwSpiListNameT* listInstanceName, MafMwSpiListItemRefT* listInstanceFrontRef)
{
	ENTER_MWSA_REPLIST();
	MafReturnT rc = MafOk;
	struct RLIterator* iterator;

	DEBUG_MWSA_REPLIST("listGetFrontRef called.");

	if (!validateName(listInstanceName)) {
		LEAVE_MWSA_REPLIST();
		return MafNotExist;
	}

	if (listInstanceFrontRef == NULL) {
		WARN_MWSA_REPLIST("listGetFrontRef(%s): listInstanceFrontRef is invalid argument (NULL pointer)", (char*) listInstanceName->value);
		LEAVE_MWSA_REPLIST();
		return MafNotExist;
	}

	DEBUG_MWSA_REPLIST("listGetFrontRef called with listInstanceName: %s", (char*) listInstanceName->value);

	// Check if the list is empty, read the list size from IMM.
	uint32_t listSize = 0;

	DEBUG_MWSA_REPLIST("listGetFrontRef calling listGetSize()...");
	rc = listGetSize(listInstanceName, &listSize);
	if (MafOk != rc)
	{
		ERR_MWSA_REPLIST("listGetFrontRef(%s): ERROR: listGetSize() failed, rc = %d", (char*) listInstanceName->value, (int) rc);
		LEAVE_MWSA_REPLIST();
		return MafNotExist;    // The MAF RL SPI does not allow all return codes
	}

	if (0 == listSize)
	{
		// the list is empty
		DEBUG_MWSA_REPLIST("listGetFrontRef: the list is EMPTY!");
		*listInstanceFrontRef = NULL;
		LEAVE_MWSA_REPLIST();
		return MafOk;
	}
	else
	{
		DEBUG_MWSA_REPLIST("listGetFrontRef: the list size is: %d", (int) listSize);

		iterator = (struct RLIterator*)malloc(sizeof(struct RLIterator));
		if (iterator == NULL) {
			ERR_MWSA_REPLIST("listGetFrontRef(%s) ERROR: Failed to allocate memory for the list iterator", (char*) listInstanceName->value);
			LEAVE_MWSA_REPLIST();
			return MafNotExist;    // The MAF RL SPI does not allow all return codes
		}

		iterator->index = 0;
		iterator->id = 1;
		*listInstanceFrontRef = iterator;

		// Read the list item data size from IMM, iterator will be passed to listGetNextItemFront function later
		unsigned int itemDataSize = UINT_MAX;
		int nRetries = 0;
		MafReturnT rc = getObjectUiAttribute((char*) listInstanceName->value, strRlistBlockSize, &itemDataSize);
		// getObjectUiAttribute may return MafOk code but itemDataSize is not updated
		while (	itemDataSize == UINT_MAX
				&& nRetries++ < RPLIST_MAX_RETRIES
				&& MafOk == rc)
		{
			// Could not get itemDataSize from IMM. Retry...
			sleep(RPLIST_RETRY_SLEEP_SECOND);
			rc = getObjectUiAttribute((char*) listInstanceName->value, strRlistBlockSize, &itemDataSize);
		}

		if (MafOk == rc && itemDataSize != UINT_MAX)
		{
			if (itemDataSize == 0)
			{
				WARN_MWSA_REPLIST("listGetFrontRef: getObjectUiAttribute returned itemDataSize: 0");
			}
			else
			{
				DEBUG_MWSA_REPLIST("listGetFrontRef: getObjectUiAttribute returned itemDataSize: %u", itemDataSize);
			}
		}
		else
		{
			ERR_MWSA_REPLIST("listGetFrontRef(%s): ERROR: getObjectUiAttribute failed with %d - itemDataSize=%u", (char*) listInstanceName->value, (int) rc, itemDataSize);
			LEAVE_MWSA_REPLIST();
			return MafNotExist;    // The MAF RL SPI does not allow all return codes
		}

		iterator->data = NULL;           // Not used
		iterator->size = itemDataSize;
	}
	LEAVE_MWSA_REPLIST();
	return MafOk;
}


MafReturnT OamSARList::listGetFinalize(const MafMwSpiListNameT* listInstanceName, const MafMwSpiListItemRefT currentItemRef)
{
	ENTER_MWSA_REPLIST();
	struct RLIterator* iterator = NULL;

	DEBUG_MWSA_REPLIST("listGetFinalize entered.");

	if (!validateName(listInstanceName)) {
		LEAVE_MWSA_REPLIST();
		return MafNotExist;
	}

	if (currentItemRef == NULL) {
		DEBUG_MWSA_REPLIST("listGetFinalize called with currentItemRef = NULL");
		LEAVE_MWSA_REPLIST();
		return MafOk;
	}

	DEBUG_MWSA_REPLIST("listGetFinalize called with listInstanceName: %s", (char*) listInstanceName->value);
	iterator = (struct RLIterator*)currentItemRef;
	if (NULL != iterator)
	{
		free(iterator);
		iterator = NULL;
	}
	LEAVE_MWSA_REPLIST();
	return MafOk;
}


/**
 *	'currentItemRef' is a pointer to the iterator structure instance of type RLIterator. It was created in listGetFrontRef()
 *	Compare iterator->index to the number of list items in the IMM Object.
 *	Check if we have reached the end of the list, then set *currentItemRef to NULL, call listGetFinalize() and return MafOk to COM.
 *	If not end-of-list then read the item data, memcpy to the pointer provided by COM and return to COM
 *	increment 'iterator->index'. Compare with the number of list items in the IMM Object.
 */
MafReturnT OamSARList::listGetNextItemFront(const MafMwSpiListNameT* listInstanceName, /* In     */
											MafMwSpiListItemRefT* currentItemRef,      /* In/Out */
											MafMwSpiListItemNameT* listItemName,       /* Out    */
											void* copyOfItemData)                      /* Out    */
{
	ENTER_MWSA_REPLIST();
	MafReturnT rc = MafOk;

	if (!validateName(listInstanceName)) {
		LEAVE_MWSA_REPLIST();
		return MafNotExist;
	}
	DEBUG_MWSA_REPLIST("listGetNextItemFront called with listInstanceName: %s", (char*) listInstanceName->value);

	if (currentItemRef == NULL
		|| listItemName == NULL || copyOfItemData == NULL) {
		ERR_MWSA_REPLIST("listGetNextItemFront(%s): ERROR: Some input parameter is invalid argument", (char*) listInstanceName->value);
		LEAVE_MWSA_REPLIST();
		return MafInvalidArgument;
	}

	struct RLIterator* iterator = (struct RLIterator*) *currentItemRef;

	if (NULL == iterator) {
		ERR_MWSA_REPLIST("listGetNextItemFront(%s): ERROR: iterator = (*currentItemRef) is NULL", (char*) listInstanceName->value);
		listGetFinalize(listInstanceName, *currentItemRef);
		*currentItemRef = NULL;
		LEAVE_MWSA_REPLIST();
		return MafInvalidArgument;
	}

	DEBUG_MWSA_REPLIST("listGetNextItemFront() called with listInstanceName: %s, iterator-index: %d, iterator-id: %d",
					(char*) listInstanceName->value, iterator->index, iterator->id);

	// Get the list size from IMM
	uint32_t listSize = 0;
	char tmpLstItemName[MAX_ITEM_NAME_LEN];
	unsigned int intListItemName = 0;

	DEBUG_MWSA_REPLIST("listGetNextItemFront() calling listGetSize()...");
	rc = listGetSize(listInstanceName, &listSize);
	if (MafOk != rc)
	{
		ERR_MWSA_REPLIST("listGetNextItemFront(%s): ERROR: listGetSize() failed, rc = %d", (char*) listInstanceName->value, (int) rc);
		LEAVE_MWSA_REPLIST();
		return MafNotExist;    // The MAF RL SPI does not allow all return codes
	}

	// Check if we are at the end of the list (incremented iterator->index is equal to the current list size)
	if (iterator->index >= listSize)
	{
		// We have reached the end of the list, nothing to return ?
		DEBUG_MWSA_REPLIST("listGetNextItemFront() end-of-list reached at index %d", iterator->index);
		rc = listGetFinalize(listInstanceName, *currentItemRef);
		*currentItemRef = NULL;
	}
	else
	{
		SaImmAttrValuesT_2 **attrValues = NULL;
		// Get the list item data from the file
		// find the next list element (file) in the list directory.
		// This would be the file with filename that represents the smallest number which is equal or bigger than the 'iterator->index'.
		rc = findNextListItem(listInstanceName, iterator->id, tmpLstItemName, &intListItemName, &attrValues);
		if (MafOk != rc)
		{
			ERR_MWSA_REPLIST("listGetNextItemFront(%s): ERROR: findNextListItem failed.", (char*) listInstanceName->value);
			// HT70246: due to some unexpected error, the file is already deleted. To avoid further error
			// we reduce the number of item so that COM will never try to access a non-exist item anymore.
			unsigned int numItems = 0;
			rc = getObjectUiAttribute((char*) listInstanceName->value, strRlistNumElements, &numItems);
			if (MafOk != rc)
			{
				ERR_MWSA_REPLIST("listGetNextItemFront(%s): ERROR: getObjectUiAttribute failed with %d - numItems=%u", (char*) listInstanceName->value, (int) rc, numItems);
				LEAVE_MWSA_REPLIST();
				return MafNotExist;
			}
			numItems--;
			if(numItems >= 0)
			{
				rc = setObjectUiAttribute((char*) listInstanceName->value, strRlistNumElements, numItems);
				if (MafOk != rc)
				{
					DEBUG_MWSA_REPLIST("listGetNextItemFront(%s): ERROR: setObjectUiAttribute failed with %d - numItems=%u", (char*) listInstanceName->value, (int) rc, numItems);
					LEAVE_MWSA_REPLIST();
					return MafNotExist;
				}
			}
			// All the data in file system are unexpected lost, reset the lastID also
			// this will have COM create a new list next time it's started
			if(numItems == 0)
			{
				setObjectUiAttribute((char*) listInstanceName->value, strRlistLastId, 0);
			}
			LEAVE_MWSA_REPLIST();
			return MafNotExist;    // The MAF RL SPI does not allow all return codes
		}
		iterator->id = intListItemName + 1;

		if (replicateInIMM) {
			rc = RlistUtil::getVoidData((const SaImmAttrValuesT_2**)attrValues, &copyOfItemData, iterator->size);
		}
		else {
			/*
			 * Read the list item data from the file and set the iterator pointer to point to it
			 */
			rc = autoRetry_readFromFile((const char*) tmpLstItemName, (const char*) listInstanceName->value, copyOfItemData, (const int) iterator->size);
		}

		if (MafOk == rc)
		{
			DEBUG_MWSA_REPLIST("listGetNextItemFront: readFromFile returned data at copyOfItemData: %p", copyOfItemData);
		}
		else
		{
			ERR_MWSA_REPLIST("listGetNextItemFront(%s): ERROR: readFromFile failed with %d", (char*) listInstanceName->value, (int) rc);
			// HT70246: can't access resource, must delete all related information since it's very dangerous
			// COM will restart forever because it always try to access to corrupted listItem.
			if (replicateInIMM) {
				deleteRTObject(std::string((char*)(listInstanceName->value)), std::string(tmpLstItemName));
			}
			else {
				removeFile(tmpLstItemName, (char*) listInstanceName->value);
			}

			unsigned int numItems = 0;
			rc = getObjectUiAttribute((char*) listInstanceName->value, strRlistNumElements, &numItems);
			if (MafOk != rc)
			{
				return MafNotExist;
			}
			numItems--;
			if(numItems >= 0)
			{
				rc = setObjectUiAttribute((char*) listInstanceName->value, strRlistNumElements, numItems);
				if (MafOk != rc)
				{
					return MafNotExist;
				}
			}
			// All the data in file system are unexpected lost, reset the lastID also
			// this will have COM create a new list next time it's started
			if(numItems == 0)
			{
				setObjectUiAttribute((char*) listInstanceName->value, strRlistLastId, 0);
			}
			LEAVE_MWSA_REPLIST();
			return MafNotExist;    // The MAF RL SPI does not allow all return codes
		}
		// Return the list item name
		listItemName->length = snprintf((char*)listItemName->value, MAX_ITEM_NAME_LEN, "%llu", (unsigned long long) intListItemName);

		// Increment the iterator index
		iterator->index++;

		if (iterator->index >= listSize)
		{
			// We have reached the end of the list, nothing to return ?
			DEBUG_MWSA_REPLIST("listGetNextItemFront() end-of-list reached at index %d", iterator->index);
			rc = listGetFinalize(listInstanceName, *currentItemRef);
			*currentItemRef = NULL;
		}
	}

	LEAVE_MWSA_REPLIST();
	return MafOk;
}


MafReturnT OamSARList::listFindItem(const MafMwSpiListNameT* listInstanceName, const MafMwSpiListItemNameT* listItemName, void* copyOfItemData)
{
	ENTER_MWSA_REPLIST();
	MafReturnT rc = MafOk;
	DEBUG_MWSA_REPLIST("listFindItem");
	if (listInstanceName == NULL || listItemName == NULL || copyOfItemData == NULL) {
		LEAVE_MWSA_REPLIST();
		return MafInvalidArgument;
	}

	DEBUG_MWSA_REPLIST("listFindItem called with listInstanceName: %s, listItemName %s", (char*) listInstanceName->value, (char*) listItemName->value);

	if (!validateName(listInstanceName) ||
		!validateName(listItemName)) {
		LEAVE_MWSA_REPLIST();
		return MafNotExist;
	}

	SaImmAttrValuesT_2 **attrValues = NULL;
	if (replicateInIMM) {

		SaNameT *immObject = NULL;
		rc = convertToRTObjectName(std::string((char*)(listInstanceName->value)), std::string((char*)(listItemName->value)), &immObject);
		if (MafOk != rc) {
			DEBUG_MWSA_REPLIST("listFindItem(): Unable to convert to IMM object name");
			return rc;
		}

		rc = readObject(immObject, &attrValues);
		saNameDelete(immObject, true);
	}
	else {
		// Check if the item exists in the file storage. If not then return MafNotExist
		rc = findFile((const char*) listItemName->value, (const char*) listInstanceName->value);
	}

	if (MafOk == rc) {

		// Read the list item data size from IMM
		unsigned int itemDataSize = 0;
		rc = getObjectUiAttribute((char*) listInstanceName->value, strRlistBlockSize, &itemDataSize);
		if (MafOk == rc)
		{
			DEBUG_MWSA_REPLIST("listFindItem: getObjectUiAttribute returned itemDataSize: %u", itemDataSize);
		}
		else
		{
			ERR_MWSA_REPLIST("listFindItem(%s): ERROR: getObjectUiAttribute failed with %d", (char*) listInstanceName->value, (int) rc);
			LEAVE_MWSA_REPLIST();
			return MafNotExist;    // The MAF RL SPI does not allow all return codes
		}

		if (replicateInIMM) {

			rc = RlistUtil::getVoidData((const SaImmAttrValuesT_2**)attrValues, &copyOfItemData, itemDataSize);
			if (MafOk != rc) {
				deleteRTObject(std::string((char*)(listInstanceName->value)), std::string((char*)(listItemName->value)));
				LEAVE_MWSA_REPLIST();
				return MafNotExist;    // The MAF RL SPI does not allow all return codes
			}
		}
		else
		{
			// read the item data file with the data provided by COM, no change to the IMM Object
			DEBUG_MWSA_REPLIST("listFindItem: calling readFromFile");
			rc = readFromFile((const char*)listItemName->value,
					(const char*)listInstanceName->value,
					copyOfItemData,
					itemDataSize);
			if(MafOk == rc)
			{
				DEBUG_MWSA_REPLIST("listFindItem: Successfully read a binary file for the RP list item: %s", listInstanceName->value);
			}
			else
			{
				ERR_MWSA_REPLIST("listFindItem: ERROR: Could not read a binary file: %s", listInstanceName->value);
				// HT70246: delete this item, there's no meaning to keep corrupted item.
				listEraseItem(listInstanceName, listItemName);
				LEAVE_MWSA_REPLIST();
				return MafNotExist;    // The MAF RL SPI does not allow all return codes
			}
		}
	}
	else
	{
		rc = MafNotExist;
	}

	LEAVE_MWSA_REPLIST();
	return rc;
}


MafReturnT OamSARList::listReplaceItem(const MafMwSpiListNameT* listInstanceName, const MafMwSpiListItemNameT* listItemName, const void* replaceItemData)
{
	ENTER_MWSA_REPLIST();
	MafReturnT rc = MafOk;

	if (!validateName(listInstanceName) ||
		!validateName(listItemName)) {
		LEAVE_MWSA_REPLIST();
		return MafNotExist;
	}

	DEBUG_MWSA_REPLIST("listReplaceItem called with listInstanceName: %s and listitemName [%s]",
						(char*) listInstanceName->value, (char *)listItemName->value);

	if(false == replicateInIMM) {
	// Check if the item exists in the file storage. If not then return MafNotExist
	rc = findFile((const char*) listItemName->value, (const char*) listInstanceName->value);
	if (MafOk == rc)
	{
		// Read the list item data size from IMM
		unsigned int itemDataSize = 0;
		rc = getObjectUiAttribute((char*) listInstanceName->value, strRlistBlockSize, &itemDataSize);
		if (MafOk == rc)
		{
			DEBUG_MWSA_REPLIST("listReplaceItem: getObjectUiAttribute returned itemDataSize: %u", itemDataSize);
		}
		else
		{
			ERR_MWSA_REPLIST("listReplaceItem(%s): ERROR: getObjectUiAttribute failed with %d", (char*) listInstanceName->value, (int) rc);
			LEAVE_MWSA_REPLIST();
			return MafNotExist;    // The MAF RL SPI does not allow all return codes
		}

		// replace the item data file with the data provided by COM, no change to the IMM Object
		// do we need to delete the file ??? TBC

		DEBUG_MWSA_REPLIST("listReplaceItem: calling autoRetry_writeToFile");
		rc = autoRetry_writeToFile(listInstanceName,listItemName,replaceItemData);

		if(MafOk == rc)
		{
			DEBUG_MWSA_REPLIST("listReplaceItem: Successfully replaced a binary file for the RP list item: %s", listInstanceName->value);
		}
		else
		{
			ERR_MWSA_REPLIST("listReplaceItem: ERROR: Could not replace a binary file: %s", listInstanceName->value);
			// HT70246: consider
			// Do we need to delete the file here? to avoid further failure?
			LEAVE_MWSA_REPLIST();
			return MafNotExist;    // The MAF RL SPI does not allow all return codes
		}
	}
	else
	{
		rc = MafNotExist;
	}
	} else {

		rc = modifyRlistRTObject((const char*)listInstanceName->value,(const char*)listItemName->value,replaceItemData);

		if(MafOk == rc)
		{
			DEBUG_MWSA_REPLIST("listReplaceItem: Successfully modified the alarm data for: %s", listInstanceName->value);
		}
		else
		{
			ERR_MWSA_REPLIST("listReplaceItem: ERROR: Could not modify alarm data for: %s",listInstanceName->value);
			LEAVE_MWSA_REPLIST();
			return MafNotExist;    // The MAF RL SPI does not allow all return codes
		}
	}

	LEAVE_MWSA_REPLIST();
	return rc;
}


MafReturnT OamSARList::listNumberOfListInstances(uint32_t* numberOfLinkListInstances)
{
	ENTER_MWSA_REPLIST();
	DEBUG_MWSA_REPLIST("listNumberOfListInstances called...");
	if (numberOfLinkListInstances == NULL) {
		ERR_MWSA_REPLIST("listNumberOfListInstances: numberOfLinkListInstances is NULL");
		LEAVE_MWSA_REPLIST();
		return MafInvalidArgument;
	}
	WARN_MWSA_REPLIST("listNumberOfListInstances always return 1");
	*numberOfLinkListInstances = 1;
	LEAVE_MWSA_REPLIST();
	return MafOk;
}


MafReturnT OamSARList::listMemoryUsage(uint32_t* memoryUsed, uint32_t* totalMemoryAvailable)
{
	ENTER_MWSA_REPLIST();
	DEBUG_MWSA_REPLIST("listMemoryUsage called...");
	WARN_MWSA_REPLIST("listMemoryUsage always returns {10, rlist_maxsize}");
	if (memoryUsed == NULL || totalMemoryAvailable == NULL) {
		LEAVE_MWSA_REPLIST();
		return MafInvalidArgument;
	}
	*memoryUsed = 10;
	*totalMemoryAvailable = (uint32_t)rlist_maxsize;

	LEAVE_MWSA_REPLIST();
	return MafOk;
}


/* ---------------------------------------------------------------------------
 * Helper functions
 */

#ifndef UNIT_TEST
static MafReturnT comsa_rplist_imm_init(SaImmHandleT *rplistImmHandle)
#else
MafReturnT comsa_rplist_imm_init(SaImmHandleT *rplistImmHandle)
#endif
{
	ENTER_MWSA_REPLIST();

	SaAisErrorT error = SA_AIS_OK;
	MafReturnT maf_rc = MafOk;

	if((error = autoRetry_saImmOmInitialize(rplistImmHandle, NULL, &imm_version)) != SA_AIS_OK)
	{
		ERR_MWSA_REPLIST("comsa_rplist_imm_init(): ERROR: saImmOmInitialize failed %s", getOpenSAFErrorString(error));
		DEBUG_MWSA_REPLIST("comsa_rplist_imm_init(): calling comsa_rplist_imm_finalize() because of saImmOmInitialize() failure");
		comsa_rplist_imm_finalize(rplistImmHandle);
		maf_rc = MafFailure;
	}
	else
	{
		DEBUG_MWSA_REPLIST("comsa_rplist_imm_init(): autoRetry_saImmOmInitialize : SUCCESS, returned IMM handle: %lu",
						(unsigned long) *rplistImmHandle);
	}
	LEAVE_MWSA_REPLIST();
	return maf_rc;
}


#ifndef UNIT_TEST
static void comsa_rplist_imm_finalize(SaImmHandleT *rplistImmHandle)
#else
void comsa_rplist_imm_finalize(SaImmHandleT *rplistImmHandle)
#endif
{
	ENTER_MWSA_REPLIST();
	SaAisErrorT error = SA_AIS_OK;

	if ((error = autoRetry_saImmOmFinalize(*rplistImmHandle)) != SA_AIS_OK)
	{
		ERR_MWSA_REPLIST("comsa_rplist_imm_finalize(): ERROR: saImmOmFinalize FAILED: %s", getOpenSAFErrorString(error));
	}
	else
	{
		DEBUG_MWSA_REPLIST("comsa_rplist_imm_finalize(): Successfully finalized the rplistImmHandle");
		*rplistImmHandle = 0;
	}

	LEAVE_MWSA_REPLIST();
	return;
}


#ifndef UNIT_TEST
static MafReturnT comsa_rplist_immAccess_init(const SaImmHandleT rplistImmHandle, SaImmAccessorHandleT *immAccessorHandle)
#else
MafReturnT comsa_rplist_immAccess_init(const SaImmHandleT rplistImmHandle, SaImmAccessorHandleT *immAccessorHandle)
#endif
{
	ENTER_MWSA_REPLIST();

	SaAisErrorT error = SA_AIS_OK;
	MafReturnT maf_rc = MafOk;

	error = autoRetry_saImmOmAccessorInitialize(rplistImmHandle, immAccessorHandle);
	if (SA_AIS_OK != error)
	{
		ERR_MWSA_REPLIST("comsa_rplist_immAccess_init(): ERROR: saImmOmInitialize failed %s", getOpenSAFErrorString(error));
		DEBUG_MWSA_REPLIST("comsa_rplist_immAccess_init(): calling comsa_rplist_immAccess_finalize() because of autoRetry_saImmOmAccessorInitialize() failure");
		comsa_rplist_immAccess_finalize(*immAccessorHandle);
		maf_rc = MafFailure;
	}
	else
	{
		DEBUG_MWSA_REPLIST("comsa_rplist_immAccess_init(): autoRetry_saImmOmAccessorInitialize : SUCCESS, returned IMM Accessor handle: %lu",
						(unsigned long) *immAccessorHandle);
	}

	LEAVE_MWSA_REPLIST();
	return maf_rc;
}


#ifndef UNIT_TEST
static void comsa_rplist_immAccess_finalize(SaImmAccessorHandleT immAccessorHandle)
#else
void comsa_rplist_immAccess_finalize(SaImmAccessorHandleT immAccessorHandle)
#endif
{
	ENTER_MWSA_REPLIST();
	SaAisErrorT error = SA_AIS_OK;

	if ((error = autoRetry_saImmOmAccessorFinalize(immAccessorHandle)) != SA_AIS_OK)
	{
		ERR_MWSA_REPLIST("comsa_rplist_immAccess_finalize(): ERROR: autoRetry_saImmOmAccessorFinalize() FAILED: %s", getOpenSAFErrorString(error));
	}
	else
	{
		DEBUG_MWSA_REPLIST("comsa_rplist_immAccess_finalize(): Successfully finalized the immAccessorHandle");
		immAccessorHandle = 0;
	}

	LEAVE_MWSA_REPLIST();
	return;
}


#ifndef UNIT_TEST
static MafReturnT createclass(const char* pClassName, uint32_t dataBufferSize, bool createRunTimeClass)
#else
MafReturnT createclass(const char* pClassName, uint32_t dataBufferSize, bool createRunTimeClass)
#endif
{
	ENTER_MWSA_REPLIST();

	SaAisErrorT error = SA_AIS_OK;
	MafReturnT maf_rc = MafOk;

	std::string className(pClassName);

	DEBUG_MWSA_REPLIST("comsa_rplist_imm_createclass calling comsa_rplist_imm_searchclass for the list: list name %s",
			className.c_str());
	maf_rc = searchclass(className.c_str());
	if (MafNotExist == maf_rc)
	{
		SaImmAttrDefinitionT_2** attrDef = NULL;
		if (createRunTimeClass) {
			attrDef = RlistUtil::prepareRTImmAttrDefinitions();
		}
		else {
			attrDef = RlistUtil::prepareImmAttrDefinitions();
		}

		if (NULL == attrDef)
		{
			DEBUG_MWSA_REPLIST("comsa_rplist_imm_createclass(): prepareImmAttrDefinitions failed");
			return MafFailure;
		}

		DEBUG_MWSA_REPLIST("comsa_rplist_imm_createclass(): calling autoRetry_saImmOmClassCreate_2 with class name %s", className.c_str());
		error = autoRetry_saImmOmClassCreate_2(rplistImmHandle,
				(const SaImmClassNameT) className.c_str(),
				(SaImmClassCategoryT) SA_IMM_CLASS_RUNTIME, // class category
				(const SaImmAttrDefinitionT_2**) attrDef);
		if(SA_AIS_OK != error)
		{
			// If the call returns bad handle try to initialize all the handles, set the OI and try one more time
			if (SA_AIS_ERR_BAD_HANDLE == error)
			{
				WARN_MWSA_REPLIST("comsa_rplist_imm_createclass(%s): autoRetry_saImmOmClassCreate_2 returned SA_AIS_ERR_BAD_HANDLE, try to re-init", className.c_str());
				MafReturnT maf_rc1 = RlistUtil::reset_rplistImmOiHandle();
				if( MafOk != maf_rc1 ){
					return maf_rc1;
				}
				DEBUG_MWSA_REPLIST("comsa_rplist_imm_createclass(): calling again autoRetry_saImmOmClassCreate_2 with class name %s", className.c_str());
				error = autoRetry_saImmOmClassCreate_2(rplistImmHandle,
						(const SaImmClassNameT) className.c_str(),
						(SaImmClassCategoryT) SA_IMM_CLASS_RUNTIME, // class category
						(const SaImmAttrDefinitionT_2**) attrDef);
				if(SA_AIS_OK != error)
				{
					ERR_MWSA_REPLIST("comsa_rplist_imm_createclass(%s): ERROR: second call to autoRetry_saImmOmClassCreate_2 failed %s",
							className.c_str(), getOpenSAFErrorString(error));
					RlistUtil::freeImmAttrDefinitions(&attrDef);
					LEAVE_MWSA_REPLIST();
					return MafFailure;
				}
				RlistUtil::freeImmAttrDefinitions(&attrDef);
			}
			else
			{
				ERR_MWSA_REPLIST("comsa_rplist_imm_createclass(%s): ERROR: autoRetry_saImmOmClassCreate_2 failed %s", className.c_str(), getOpenSAFErrorString(error));
				RlistUtil::freeImmAttrDefinitions(&attrDef);
				LEAVE_MWSA_REPLIST();
				return MafFailure;
			}
		}
		else
		{
			DEBUG_MWSA_REPLIST("comsa_rplist_imm_createclass(): autoRetry_saImmOmClassCreate_2 : SUCCESS");
			RlistUtil::freeImmAttrDefinitions(&attrDef);
			LEAVE_MWSA_REPLIST();
			return MafOk;
		}
	}
	else
	{
		if (maf_rc != MafOk)
		{
			DEBUG_MWSA_REPLIST("comsa_rplist_imm_createclass(): comsa_rplist_imm_searchclass(): Failed when searching for  Class '%s' in IMM, returned %d ",
					className.c_str(), maf_rc);
			LEAVE_MWSA_REPLIST();
			return maf_rc;
		}
	}
	LEAVE_MWSA_REPLIST();
	return maf_rc;
}

#ifndef UNIT_TEST
static MafReturnT deleteclass(const char* className)
#else
MafReturnT deleteclass(const char* className)
#endif
{
	ENTER_MWSA_REPLIST();

	SaAisErrorT error = SA_AIS_OK;
	MafReturnT maf_rc = MafOk;

	error = autoRetry_saImmOmClassDelete(rplistImmHandle, (const SaImmClassNameT) className);
	if(SA_AIS_OK != error)
	{
		// If the call returns bad handle try to initialize all the handles, set the OI and try one more time
		if (SA_AIS_ERR_BAD_HANDLE == error)
		{
			WARN_MWSA_REPLIST("deleteclass(%s): autoRetry_saImmOmClassDelete() returned SA_AIS_ERR_BAD_HANDLE, try to re-init", className);
			// obtain IMM handles and set the OI
			MafReturnT maf_rc1 = RlistUtil::reset_rplistImmOiHandle();
			if( MafOk != maf_rc1 ){
				return maf_rc1;
			}
			DEBUG_MWSA_REPLIST("deleteclass(): calling again autoRetry_saImmOmClassDelete with class name %s", className);
			error = autoRetry_saImmOmClassDelete(rplistImmHandle, (const SaImmClassNameT) className);
			if(SA_AIS_OK != error)
			{
				ERR_MWSA_REPLIST("deleteclass(%s): ERROR: second call to autoRetry_saImmOmClassDelete failed %s", className, getOpenSAFErrorString(error));
				LEAVE_MWSA_REPLIST();
				return MafFailure;
			}
		}
		else
		{
			ERR_MWSA_REPLIST("deleteclass(%s): ERROR: autoRetry_saImmOmClassDelete failed %s", className, getOpenSAFErrorString(error));
			LEAVE_MWSA_REPLIST();
			return MafFailure;
		}
	}
	else
	{
		DEBUG_MWSA_REPLIST("deleteclass(): autoRetry_saImmOmClassDelete : SUCCESS");
	}

	LEAVE_MWSA_REPLIST();
	return maf_rc;
}


#ifndef UNIT_TEST
static MafReturnT searchclass(const char* className)
#else
MafReturnT searchclass(const char* className)
#endif
{
	ENTER_MWSA_REPLIST();

	SaAisErrorT error = SA_AIS_OK;
	SaImmClassCategoryT theCategory;
	SaImmAttrDefinitionT_2** theDefinitions = NULL;
	MafReturnT maf_rc = MafOk;

	DEBUG_MWSA_REPLIST("searchclass(): calling autoRetry_saImmOmClassDescriptionGet_2 with class name %s", className);
	error = autoRetry_saImmOmClassDescriptionGet_2(rplistImmHandle,
												(const SaImmClassNameT) className,
												&theCategory ,
												&theDefinitions);
	if(error == SA_AIS_ERR_NOT_EXIST)
	{
		DEBUG_MWSA_REPLIST("searchclass(): can not find in IMM class name %s, return %d", className, error);
		LEAVE_MWSA_REPLIST();
		return MafNotExist;
	}
	else
	{
		if (SA_AIS_OK != error)
		{
			// If the call returns bad handle try to initialize all the handles, set the OI and try one more time
			if (SA_AIS_ERR_BAD_HANDLE == error)
			{
				// obtain IMM handles and set the OI
				MafReturnT maf_rc1 = RlistUtil::reset_rplistImmOiHandle();
				if( MafOk != maf_rc1 ) {
					WARN_MWSA_REPLIST("searchclass(%s): autoRetry_saImmOmClassDescriptionGet_2 returned SA_AIS_ERR_BAD_HANDLE, try to re-init handles FAILED", className);
					return maf_rc1;
				}
				DEBUG_MWSA_REPLIST("searchclass(): calling again autoRetry_saImmOmClassDescriptionGet_2 with class name %s", className);
				error = autoRetry_saImmOmClassDescriptionGet_2(rplistImmHandle,
															(const SaImmClassNameT) className,
															&theCategory ,
															&theDefinitions);
				if(SA_AIS_OK != error)
				{
					ERR_MWSA_REPLIST("searchclass(%s): ERROR: second call to autoRetry_saImmOmClassDescriptionGet_2 failed %s", className, getOpenSAFErrorString(error));
					LEAVE_MWSA_REPLIST();
					return MafFailure;
				}
			}
			else
			{
				ERR_MWSA_REPLIST("searchclass(%s): ERROR: autoRetry_saImmOmClassDescriptionGet_2 failed %s", className, getOpenSAFErrorString(error));
				LEAVE_MWSA_REPLIST();
				return MafFailure;
			}

			DEBUG_MWSA_REPLIST("searchclass(): Failed when searching for Class in IMM with error: %d", error);
			maf_rc = maf_coremw_rc(error);
			LEAVE_MWSA_REPLIST();
			return maf_rc;
		}
		else
		{
			DEBUG_MWSA_REPLIST("searchclass(): The class is already existing in IMM");
			maf_rc = maf_coremw_rc(error);
		}
	}
	if (theDefinitions != NULL) {
		error = saImmOmClassDescriptionMemoryFree_2(rplistImmHandle, theDefinitions);
		if(SA_AIS_OK != error)
		{
			ERR_MWSA_REPLIST("searchclass(%s): ERROR: call to saImmOmClassDescriptionMemoryFree_2 failed %s", className, getOpenSAFErrorString(error));
		}
		theDefinitions = NULL;
	}
	LEAVE_MWSA_REPLIST();
	return maf_rc;
}


#ifndef UNIT_TEST
static MafReturnT createObject(const char* className, const SaImmAttrValuesT_2 **attrValues)
#else
MafReturnT createObject(const char* className, const SaImmAttrValuesT_2 **attrValues)
#endif
{
	ENTER_MWSA_REPLIST();
	SaAisErrorT err = SA_AIS_OK;
	MafReturnT maf_rc = MafOk;
	DEBUG_MWSA_REPLIST("createObject(): class name = %s", className);
	err = autoRetry_saImmOiRtObjectCreate_2(rplistImmOiHandle, (const SaImmClassNameT) className, NULL, attrValues);
	maf_rc = maf_coremw_rc(err);

	// If the object already exists this is fine, no need to create it.
	if (SA_AIS_ERR_EXIST == err)
	{
		DEBUG_MWSA_REPLIST("createObject(): saImmOiRtObjectCreate_2() Failed because Object already exists : %s", getOpenSAFErrorString(err));
		LEAVE_MWSA_REPLIST();
		return maf_rc;
	}
	else
	{
		if (SA_AIS_OK != err)
		{
			// If the call returns bad handle try to initialize all the handles, set the OI and try one more time
			if (SA_AIS_ERR_BAD_HANDLE == err)
			{
				WARN_MWSA_REPLIST("createObject(%s): saImmOiRtObjectCreate_2 returned SA_AIS_ERR_BAD_HANDLE, try to re-init", className);
				// obtain IMM handles and set the OI
				MafReturnT maf_rc1 = RlistUtil::reset_rplistImmOiHandle();
				if( MafOk != maf_rc1 ){
					return maf_rc1;
				}
				DEBUG_MWSA_REPLIST("createObject(): calling again saImmOiRtObjectCreate_2 with class name %s", className);
				err = autoRetry_saImmOiRtObjectCreate_2(rplistImmOiHandle, (const SaImmClassNameT) className, NULL, attrValues);
				maf_rc = maf_coremw_rc(err);
				if(SA_AIS_OK != err && SA_AIS_ERR_EXIST != err )
				{
					ERR_MWSA_REPLIST("createObject(%s): ERROR: second call to saImmOiRtObjectCreate_2 failed %s", className, getOpenSAFErrorString(err));
					LEAVE_MWSA_REPLIST();
					return MafFailure;
				}
			}
			else
			{
				DEBUG_MWSA_REPLIST("createObject(): ERROR: saImmOiRtObjectCreate_2 failed %s", getOpenSAFErrorString(err));
				LEAVE_MWSA_REPLIST();
				return MafFailure;
			}
		}
		else
		{
			DEBUG_MWSA_REPLIST("createObject(): saImmOiRtObjectCreate_2() className: %s : SUCCESS", className);
		}
	}
#ifndef UNIT_TEST
#ifdef RPLIST_DEBUG_CODE_ENABLE
	SaImmAttrValuesT_2 **returnedAttrValues = 0;
	SaNameT objName;
	classToObj(className, &objName);
	readObject(&objName, &returnedAttrValues);
	comsa_rplist_imm_debugPrintObject(returnedAttrValues);
#endif
#endif
	LEAVE_MWSA_REPLIST();
	return maf_rc;
}


#ifndef UNIT_TEST
static MafReturnT readObject(const SaNameT* immObject, SaImmAttrValuesT_2 ***attrValues)
#else
MafReturnT readObject(const SaNameT *immObject, SaImmAttrValuesT_2 ***attrValues)
#endif
{
	ENTER_MWSA_REPLIST();
	SaAisErrorT err = SA_AIS_OK;
	MafReturnT maf_rc = MafOk;

	err = autoRetry_saImmOmAccessorGet_2(immAccessHandle, immObject, NULL, attrValues);	 // All attributes
	if (SA_AIS_OK != err)
	{
		char* objectName = makeCString(immObject);
		// If the call returns bad handle try to initialize all the handles, set the OI and try one more time
		if (SA_AIS_ERR_BAD_HANDLE == err)
		{
			WARN_MWSA_REPLIST("readObject(%s): autoRetry_saImmOmAccessorInitialize returned SA_AIS_ERR_BAD_HANDLE, try to re-init", objectName);
			// obtain IMM handles and set the OI
			MafReturnT maf_rc1 = RlistUtil::reset_rplistImmOiHandle();
			if( MafOk != maf_rc1 ){
				return maf_rc1;
			}
			DEBUG_MWSA_REPLIST("readObject(%s): calling again autoRetry_saImmOmAccessorGet_2", objectName);
			err = autoRetry_saImmOmAccessorGet_2(immAccessHandle, immObject, NULL, attrValues);	 // All attributes
			if (SA_AIS_OK != err)
			{
				ERR_MWSA_REPLIST("readObject(%s): ERROR: second call to autoRetry_saImmOmAccessorGet_2 failed %s", objectName, getOpenSAFErrorString(err));
				delete objectName;
				LEAVE_MWSA_REPLIST();
				return MafFailure;
			}
			delete objectName;
		}
		else
		{
			if (SA_AIS_ERR_NOT_EXIST == err)
			{
				DEBUG_MWSA_REPLIST("readObject(%s): autoRetry_saImmOmAccessorGet_2() return SA_AIS_ERR_NOT_EXIST", objectName);
			}
			else
			{
				ERR_MWSA_REPLIST("readObject(%s): ERROR: autoRetry_saImmOmAccessorGet_2 failed %s", objectName, getOpenSAFErrorString(err));
			}
			delete objectName;
			LEAVE_MWSA_REPLIST();
			return MafNotExist;
		}
	}

	DEBUG_MWSA_REPLIST("readObject(): saImmOmAccessorGet_2() : SUCCESS");
	LEAVE_MWSA_REPLIST();
	return maf_rc;
}

#ifndef UNIT_TEST
static MafReturnT modifyObject(const SaNameT *objectName, const SaImmAttrModificationT_2 **attrValues)
#else
MafReturnT modifyObject(const SaNameT *objectName, const SaImmAttrModificationT_2 **attrValues)
#endif
{
	ENTER_MWSA_REPLIST();
	MafReturnT maf_rc = MafOk;

	const char *className = saNameGet(objectName);

	SaAisErrorT err = SA_AIS_OK;
	err = autoRetry_saImmOiRtObjectUpdate_2(rplistImmOiHandle, objectName, (const SaImmAttrModificationT_2 **)attrValues);
	if(SA_AIS_OK != err)
	{
		// If the call returns bad handle try to initialize all the handles, set the OI and try one more time
		if (SA_AIS_ERR_BAD_HANDLE == err)
		{
			WARN_MWSA_REPLIST("modifyObject(%s): autoRetry_saImmOiRtObjectUpdate_2 returned SA_AIS_ERR_BAD_HANDLE, try to re-init", className);
			// obtain IMM handles and set the OI
			MafReturnT maf_rc1 = RlistUtil::reset_rplistImmOiHandle();
			if( MafOk != maf_rc1 ){
				return maf_rc1;
			}
			DEBUG_MWSA_REPLIST("modifyObject(): calling again autoRetry_saImmOiRtObjectUpdate_2 with object name %s", className);
			err = autoRetry_saImmOiRtObjectUpdate_2(rplistImmOiHandle, objectName, (const SaImmAttrModificationT_2 **)attrValues);
			if(SA_AIS_OK != err)
			{
				if(SA_AIS_ERR_NOT_EXIST == err)
					maf_rc = MafNotExist;
				else
					maf_rc = MafFailure;

				ERR_MWSA_REPLIST("modifyObject(%s): ERROR: second call to autoRetry_saImmOiRtObjectUpdate_2 failed %s", className, getOpenSAFErrorString(err));
				LEAVE_MWSA_REPLIST();
				return maf_rc;
			}
		}
		else
		{
			ERR_MWSA_REPLIST("modifyObject(%s): ERROR: autoRetry_saImmOiRtObjectUpdate_2 failed %s", className, getOpenSAFErrorString(err));
			LEAVE_MWSA_REPLIST();
			return MafFailure;
		}
	}
	LEAVE_MWSA_REPLIST();
	return maf_rc;
}

#ifndef UNIT_TEST
static MafReturnT comsa_rplist_immOi_init(SaImmHandleT *rplistImmHandle)
#else
MafReturnT comsa_rplist_immOi_init(SaImmHandleT *rplistImmHandle)
#endif
{
	ENTER_MWSA_REPLIST();

	SaAisErrorT error = SA_AIS_OK;
	MafReturnT maf_rc = MafOk;

	DEBUG_MWSA_REPLIST("comsa_rplist_immOi_init(): calling autoRetry_saImmOiInitialize_2");
	if((error = autoRetry_saImmOiInitialize_2(rplistImmHandle, NULL, &imm_version)) != SA_AIS_OK)
	{
		ERR_MWSA_REPLIST("comsa_rplist_immOi_init(): ERROR: saImmOiInitialize_2 failed %s", getOpenSAFErrorString(error));
		maf_rc = MafFailure;
	}
	else
	{
		DEBUG_MWSA_REPLIST("comsa_rplist_immOi_init(): autoRetry_saImmOiInitialize_2 : SUCCESS");
	}
	LEAVE_MWSA_REPLIST();
	return maf_rc;
}


#ifndef UNIT_TEST
static void comsa_rplist_immOi_finalize(SaImmHandleT *rplistImmHandle)
#else
void comsa_rplist_immOi_finalize(SaImmHandleT *rplistImmHandle)
#endif
{
	ENTER_MWSA_REPLIST();

	SaAisErrorT error = SA_AIS_OK;

	if ( *rplistImmHandle == 0 )
	{
		LEAVE_MWSA_REPLIST();
		return;
	}
	if((error = autoRetry_saImmOiFinalize(*rplistImmHandle)) != SA_AIS_OK)
	{
		ERR_MWSA_REPLIST("comsa_rplist_immOi_finalize(): ERROR: saImmOiFinalize failed %s", getOpenSAFErrorString(error));
	}
	else
	{
		DEBUG_MWSA_REPLIST("comsa_rplist_immOi_finalize(): autoRetry_saImmOiFinalize : SUCCESS");
		*rplistImmHandle = 0;
	}
	LEAVE_MWSA_REPLIST();
	return;
}


#ifndef UNIT_TEST
static MafReturnT deleteObject(const SaNameT *objectName)
#else
MafReturnT deleteObject(const SaNameT *objectName)
#endif
{
	ENTER_MWSA_REPLIST();
	MafReturnT maf_rc = MafOk;
	SaAisErrorT err = SA_AIS_OK;
	err = autoRetry_saImmOiRtObjectDelete(rplistImmOiHandle, objectName);
	if(SA_AIS_OK != err)
	{
		// If the call returns bad handle try to initialize all the handles, set the OI and try one more time
		if (SA_AIS_ERR_BAD_HANDLE == err)
		{
			WARN_MWSA_REPLIST("deleteObject(%s): autoRetry_saImmOiRtObjectDelete returned SA_AIS_ERR_BAD_HANDLE, try to re-init", saNameGet(objectName));
			// obtain IMM handles and set the OI
			MafReturnT maf_rc1 = RlistUtil::reset_rplistImmOiHandle();
			if( MafOk != maf_rc1 ){
				return maf_rc1;
			}
			DEBUG_MWSA_REPLIST("deleteObject(): calling again autoRetry_saImmOiRtObjectDelete");
			err = autoRetry_saImmOiRtObjectDelete(rplistImmOiHandle, objectName);
			if(SA_AIS_OK != err)
			{
				ERR_MWSA_REPLIST("deleteObject(%s): ERROR: second call to autoRetry_saImmOiRtObjectDelete failed %s", saNameGet(objectName), getOpenSAFErrorString(err));
				LEAVE_MWSA_REPLIST();
				return MafFailure;
			}
		}
		else
		{
			ERR_MWSA_REPLIST("deleteObject(%s): ERROR: autoRetry_saImmOiRtObjectDelete failed %s", saNameGet(objectName), getOpenSAFErrorString(err));
			LEAVE_MWSA_REPLIST();
			return MafFailure;
		}
	}
	else
	{
		DEBUG_MWSA_REPLIST("deleteObject(): SUCCESS");
		maf_rc = MafOk;
	}
	LEAVE_MWSA_REPLIST();
	return maf_rc;
}

/*
 * This supporting function is used to create directory in the file system
 */
#ifndef UNIT_TEST
static MafReturnT createDir(const char* dirName, const char* subDir)
#else
MafReturnT createDir(const char* dirName, const char* subDir)
#endif
{
	ENTER_MWSA_REPLIST();
	struct stat st;
	MafReturnT maf_rc = MafOk;
	FILE *result = NULL;
	char *dir = NULL;

	/* update the path to the base repository before doing anything */
	if(MafOk != getBaseRepository())
	{
		ERR_MWSA_REPLIST("createDir(): ERROR: Could not obtain path for the base replicated list directory");
		maf_rc = MafFailure;
	}
	else
	{
		DEBUG_MWSA_REPLIST("createDir(): Obtained path for the replicated list base directory");

		char strMkDir[] = "mkdir -p ";
		const char *pathArray[4];
		pathArray[0] = strMkDir;
		pathArray[1] = pathToStorageDirectory;
		pathArray[2] = COMSA_FOR_COREMW_DIR;
		pathArray[3] = subDir;
		pathArray[4] = dirName;
		dir = createPath(pathArray, 5, NULL);
		if (NULL == dir)
		{
			ERR_MWSA_REPLIST("createDir(): ERROR: Failed to allocate memory");
			LEAVE_MWSA_REPLIST();
			return MafFailure;
		}

		if(0 == stat(dir, &st))
		{
			WARN_MWSA_REPLIST("createDir(): %s already exists", dirName);
			maf_rc = MafOk;
		}
		else
		{
			DEBUG_MWSA_REPLIST("createDir(): %s doesn't exists then create it", dir);
			result = popen(dir, "r");
			if(NULL == result)
			{
				ERR_MWSA_REPLIST("createDir(): ERROR: Could not create %s directory, popen() returned NULL", dir);
				maf_rc = MafFailure;
			}
			else
			{
				DEBUG_MWSA_REPLIST("createDir(): Created %s directory successfully", dir);

				if (-1 == pclose(result))
				{
					ERR_MWSA_REPLIST("createDir(): %s ERROR: pclose() failed with errno = '%s'", dir, strerror(errno));
					// maf_rc = MafFailure;	 // seems this always fails for some reason, maybe the process has terminated already ?!?
				}
				else
				{
					DEBUG_MWSA_REPLIST("createDir(): pclose executed successfully");
				}
			}
		}
	}

	if(NULL != dir)
	{
		free(dir);
		dir = NULL;
	}

	LEAVE_MWSA_REPLIST();
	return maf_rc;
}

char* comsa_rplist_imm_getfullfilename(const char* fileName, const char* subDir)
{
	char *fullName = NULL;
	/* update path to base repository before do anything */
	if(MafOk != getBaseRepository())
	{
		ERR_MWSA_REPLIST("comsa_rplist_imm_getfullfilename(): ERROR: Could not obtain the path of the base repository");
	}
	else
	{
		DEBUG_MWSA_REPLIST("comsa_rplist_imm_getfullfilename(): Obtained the path of the base repository");

		const char *pathArray[4];
		pathArray[0] = pathToStorageDirectory;
		pathArray[1] = COMSA_FOR_COREMW_DIR;
		pathArray[2] = strRpListPath;
		pathArray[3] = subDir;
		char fullFileName[MAX_PATH_DATA_LENGTH];
		strcpy(fullFileName, fileName);
		strcat(fullFileName, strBin);
		fullName = createPath(pathArray, 4, fullFileName);
		if (NULL == fullName)
		{
			ERR_MWSA_REPLIST("removeFile(): ERROR: Failed to allocate memory");
			LEAVE_MWSA_REPLIST();
			return NULL;
		}
	}

	LEAVE_MWSA_REPLIST();
	return fullName;
}

char* comsa_rplist_imm_getfullPath(const char* subDir)
{
	char *fullPath = NULL;
	/* update path to base repository before do anything */
	if(MafOk != getBaseRepository())
	{
		ERR_MWSA_REPLIST("comsa_rplist_imm_getfullPath(): ERROR: Could not obtain the path of the base repository");
	}
	else
	{
		DEBUG_MWSA_REPLIST("comsa_rplist_imm_getfullPath(): Obtained the path of the base repository");
		fullPath = (char *)calloc(MAX_PATH_DATA_LENGTH, 1);
		if (NULL == fullPath)
		{
			ERR_MWSA_REPLIST("comsa_rplist_imm_getfullPath(): ERROR: Failed to allocate memory");
			LEAVE_MWSA_REPLIST();
			return NULL;
		}
		strcpy(fullPath, pathToStorageDirectory);
		strcat(fullPath, "/");
		strcat(fullPath, COMSA_FOR_COREMW_DIR);
		strcat(fullPath, "/");
		strcat(fullPath, strRpListPath); // already has "/"
		//strcat(fullPath, "/");
		strcat(fullPath, subDir);
	}

	LEAVE_MWSA_REPLIST();
	return fullPath;
}

/*
 * This supporting function is used to create and write out binary data from COM into a file
 * /<cluster clear location>/rplist/<subDir>/
 */
#ifndef UNIT_TEST
static MafReturnT writeToFile(const char* fileName, const char* subDir, const void *data, const int dataSize)
#else
MafReturnT writeToFile(const char* fileName, const char* subDir, const void *data, const int dataSize)
#endif
{
	ENTER_MWSA_REPLIST();
	MafReturnT maf_rc = MafOk;

	char* fullName = comsa_rplist_imm_getfullfilename(fileName, subDir);

	FILE *fp = fopen(fullName, "wb");
	if(NULL == fp)
	{
		ERR_MWSA_REPLIST("writeToFile(): ERROR: Cannot open file %s", fullName);
		maf_rc = MafFailure;
	}
	else
	{
		DEBUG_MWSA_REPLIST("writeToFile(): Successfully opened the file %s to write data", fullName);
		int size_rc = (int) fwrite((const void*)data, 1, dataSize, fp);
		if(dataSize != size_rc)
		{
			ERR_MWSA_REPLIST("writeToFile(%s): ERROR: fwrite failed: wrote [%d], requested [%d]", fullName, dataSize, size_rc);
			maf_rc = MafNoResources;
		}
		if(0 != fclose(fp))
		{
			ERR_MWSA_REPLIST("writeToFile(): ERROR: fclose failed for file %s", fullName);
			maf_rc = MafFailure;
		}
	}
	if(NULL != fullName)
	{
		free(fullName);
		fullName = NULL;
	}
	LEAVE_MWSA_REPLIST();
	return maf_rc;
}


#ifndef UNIT_TEST
static MafReturnT autoRetry_writeToFile(const MafMwSpiListNameT* listInstanceName,const MafMwSpiListItemNameT* listItemName, const void* replaceItemData)
#else
MafReturnT autoRetry_writeToFile(const MafMwSpiListNameT* listInstanceName,const MafMwSpiListItemNameT* listItemName, const void* replaceItemData)
#endif
{
	// Read the list item data size from IMM
	unsigned int itemDataSize = UINT_MAX;
	int nRetries = 0;
	MafReturnT rc = getObjectUiAttribute((char*) listInstanceName->value, strRlistBlockSize, &itemDataSize);
	// getObjectUiAttribute may return MafOk code but itemDataSize is not updated
	while (	itemDataSize == UINT_MAX
			&& nRetries++ < RPLIST_MAX_RETRIES
			&& MafOk == rc)
	{
		sleep(RPLIST_RETRY_SLEEP_SECOND);
		rc = getObjectUiAttribute((char*) listInstanceName->value, strRlistBlockSize, &itemDataSize);
	}

	if (MafOk == rc && itemDataSize != UINT_MAX)
	{
		if (itemDataSize == 0)
		{
			// The actual value of blocksize in IMM is 0
			WARN_MWSA_REPLIST("autoRetry_writeToFile: getObjectUiAttribute returned itemDataSize: 0");
		}
		else
		{
			DEBUG_MWSA_REPLIST("autoRetry_writeToFile: getObjectUiAttribute returned itemDataSize: %u", itemDataSize);
		}
	}
	else
	{
		ERR_MWSA_REPLIST("autoRetry_writeToFile(%s): ERROR: getObjectUiAttribute failed with %d - itemDataSize=%u", (char*) listInstanceName->value, (int) rc, itemDataSize);
		LEAVE_MWSA_REPLIST();
		return MafNotExist;    // The MAF RL SPI does not allow all return codes
	}

	DEBUG_MWSA_REPLIST("autoRetry_writeToFile: calling writeToFile");
	nRetries = 0;
	rc = writeToFile((const char*)listItemName->value,
									(const char*)listInstanceName->value,
									replaceItemData,
									itemDataSize);
	while (nRetries++ < RPLIST_MAX_RETRIES && MafOk != rc)
	{
		// write data fail. Retry...
		sleep(RPLIST_RETRY_SLEEP_SECOND);
		rc = writeToFile((const char*)listItemName->value,
									(const char*)listInstanceName->value,
									replaceItemData,
									itemDataSize);
	}

	return rc;
}

/*
 * Allocate memory for the list item data,
 * read the list item data from file to this buffer and return a pointer to it
 */
#ifndef UNIT_TEST
static MafReturnT readFromFile(const char* fileName, const char* subDir, void *data, const int dataSize)
#else
MafReturnT readFromFile(const char* fileName, const char* subDir, void *data, const int dataSize)
#endif
{
	ENTER_MWSA_REPLIST();
	MafReturnT maf_rc = MafOk;
	char* fullName = comsa_rplist_imm_getfullfilename(fileName, subDir);

	FILE *fp = fopen(fullName, "rb");
	if(NULL == fp)
	{
		ERR_MWSA_REPLIST("readFromFile(): ERROR: Cannot open file %s", fullName);
		maf_rc = MafFailure;
	}
	else
	{
		DEBUG_MWSA_REPLIST("readFromFile(): Successfully opened the file %s to read data from", fullName);
		size_t actualSize = fread(data, 1, dataSize, fp);
		if (int(actualSize) != dataSize)
		{
			ERR_MWSA_REPLIST("readFromFile(%s): ERROR: fread() failed, actualSize: %d, requested size: %d", fullName, (int) actualSize, dataSize);
			maf_rc = MafFailure;
		}
		else
		{
			DEBUG_MWSA_REPLIST("readFromFile(): Successfully read the item data of size %d to buffer adderess %p ", (int) actualSize, data);
		}
		if(0 != fclose(fp))
		{
			ERR_MWSA_REPLIST("readFromFile(): ERROR: fclose() failed for file %s", fullName);
			maf_rc = MafFailure;
		}
	}

	if(NULL != fullName)
	{
		free(fullName);
		fullName = NULL;
	}
	LEAVE_MWSA_REPLIST();
	return maf_rc;
}


#ifndef UNIT_TEST
static MafReturnT autoRetry_readFromFile(const char* fileName, const char* subDir, void *data, const int dataSize)
#else
MafReturnT autoRetry_readFromFile(const char* fileName, const char* subDir, void *data, const int dataSize)
#endif
{
	DEBUG_MWSA_REPLIST("autoRetry_readFromFile: calling readFromFile");
	int nRetries = 0;
	MafReturnT rc = readFromFile(fileName, subDir, data, dataSize);
	while (nRetries++ < RPLIST_MAX_RETRIES && MafFailure == rc)
	{
		// readFromFile failed with code MafFailure. Retry...
		sleep(RPLIST_RETRY_SLEEP_SECOND);
		rc = readFromFile(fileName, subDir, data, dataSize);
	}

	return rc;
}

/*
 * This supporting function helps to guarantee that the path of the base repository is updated
 * Reason: after rebooting, the path to base repository is lost.
 */
#ifndef UNIT_TEST
static MafReturnT getBaseRepository()
#else
MafReturnT getBaseRepository()
#endif
{
	ENTER_MWSA_REPLIST();
	MafReturnT maf_rc = MafOk;

	/* If path to base repository is already updated, return MafOk immediately to avoid faults from IO stream */
	if('\0' != pathToStorageDirectory[0])
	{
		DEBUG_MWSA_REPLIST("getBaseRepository(): base repository: %s", pathToStorageDirectory);
		return MafOk;
	}

	getClearStorage(pathToStorageDirectory);

	LEAVE_MWSA_REPLIST();
	return maf_rc;
}


/*
 * This supporting function is used to check the existence of a file
 */
#ifndef UNIT_TEST
static MafReturnT findFile(const char* fileName, const char* subDir)
#else
MafReturnT findFile(const char* fileName, const char* subDir)
#endif
{
	ENTER_MWSA_REPLIST();
	MafReturnT maf_rc = MafOk;
	struct stat st;

	if ( NULL == fileName )
	{
		maf_rc =MafFailure;
		LEAVE_MWSA_REPLIST();
		return maf_rc;
	}
	char *fullName = comsa_rplist_imm_getfullfilename(fileName, subDir);

	if(0 == stat(fullName, &st))
	{
		DEBUG_MWSA_REPLIST("comsa_rplist_findFile(): The file '%s' exists", fullName);
		maf_rc = MafOk;
	}
	else
	{
		DEBUG_MWSA_REPLIST("comsa_rplist_findFile(): The file '%s' does not exist", fullName);
		maf_rc = MafNotExist;
	}

	if(NULL != fullName)
	{
		free(fullName);
		fullName = NULL;
	}

	LEAVE_MWSA_REPLIST();
	return maf_rc;
}


/*
 * This supporting function is used to remove a file
 * when listEraseItem is fetched
 */
#ifndef UNIT_TEST
static MafReturnT removeFile(const char* fileName, const char* subDir)
#else
MafReturnT removeFile(const char* fileName, const char* subDir)
#endif
{
	ENTER_MWSA_REPLIST();
	MafReturnT maf_rc = MafOk;

	char* fullName = comsa_rplist_imm_getfullfilename(fileName, subDir);

	DEBUG_MWSA_REPLIST("removeFile(): Removing the file '%s' ... ", fullName);

	if(0 != remove(fullName))
	{
		ERR_MWSA_REPLIST("removeFile(): ERROR: Could not remove the file '%s' ", fullName);
		LEAVE_MWSA_REPLIST();
		return MafFailure;
	}
	else
	{
		DEBUG_MWSA_REPLIST("removeFile(): Successfully removed the file '%s' ", fullName);
	}

	if(NULL != fullName)
	{
		free(fullName);
		fullName = NULL;
	}

	LEAVE_MWSA_REPLIST();
	return maf_rc;
}

#ifndef UNIT_TEST
static MafReturnT comsa_rplist_imm_removeAllFiles(const char* subDir)
#else
MafReturnT comsa_rplist_imm_removeAllFiles(const char* subDir)
#endif
{
	ENTER_MWSA_REPLIST();
	MafReturnT maf_rc = MafOk;
	struct dirent *pDirent = NULL;
	DIR *pDir = NULL;
	char *dirName = NULL;
	char *fileName = NULL;
	struct stat st;

	/* update the path to the base repository before doing anything */
	if(MafOk != getBaseRepository()) {
		ERR_MWSA_REPLIST("comsa_rplist_imm_removeAllFiles(): ERROR: Could not obtain the path of the base repository");
		maf_rc = MafFailure;
	}
	else {
		DEBUG_MWSA_REPLIST("comsa_rplist_imm_removeAllFiles(): Obtained the path of the base repository");
		// Build the path to the files
		const char *pathArray[4];
		pathArray[0] = pathToStorageDirectory;
		pathArray[1] = COMSA_FOR_COREMW_DIR;
		pathArray[2] = strRpListPath;
		pathArray[3] = subDir;
		dirName = createPath(pathArray, 4, NULL);

		if (NULL == dirName) {
			ERR_MWSA_REPLIST("comsa_rplist_imm_removeAllFiles(): ERROR: Failed to allocate memory for dirName");
			maf_rc = MafFailure;
		}
		else {
			if(0 != stat(dirName, &st))
			{
				// If the directory does not exist, it means there is no need further actions. Return MafOk
				WARN_MWSA_REPLIST("comsa_rplist_imm_removeAllFiles(): %s doesn't exists", dirName);
				maf_rc = MafOk;
			}
			else {
				// If the directory exists, keep doing
				DEBUG_MWSA_REPLIST("comsa_rplist_imm_removeAllFiles(): Removing files from '%s' ... ", dirName);
				pDir = opendir (dirName);

				if (NULL == pDir) {
					ERR_MWSA_REPLIST("comsa_rplist_imm_removeAllFiles(): Unable to open dir '%s' ... ", dirName);
					maf_rc = MafFailure;
				}
				else {
					// Delete all the files in the directory
					while (NULL != (pDirent = readdir(pDir))) {
						const char *pathArray[1];
						pathArray[0] = dirName;
						fileName = createPath(pathArray, 1, pDirent->d_name);
						if (NULL == fileName) {
							ERR_MWSA_REPLIST("comsa_rplist_imm_removeAllFiles(%s): ERROR: Failed to allocate memory for fileName", dirName);
							maf_rc = MafFailure;
							break;
						}
						else {
							if (0 != (strcmp(pDirent->d_name, ".") & strcmp(pDirent->d_name, "..")))
							{
								int res_rm = remove(fileName);
								if (0 == res_rm)
								{
									DEBUG_MWSA_REPLIST("comsa_rplist_imm_removeAllFiles(): removed [%s] [%s]", pDirent->d_name, fileName);
								}
								else
								{
									ERR_MWSA_REPLIST("comsa_rplist_imm_removeAllFiles(): ERROR: remove(%s) returned %d, failed with errno = '%s'",
													fileName, res_rm, strerror(errno));
									maf_rc = MafFailure;
									free(fileName);
									fileName = NULL;
									break;
								}
							}
							free(fileName);
							fileName = NULL;
						}
					}
					closedir (pDir);
				}
			}
		}
	}

	if(dirName != NULL)
	{
		free(dirName);
		dirName = NULL;
	}

	LEAVE_MWSA_REPLIST();
	return maf_rc;
}


#ifndef UNIT_TEST
static MafReturnT setObjectUiAttribute(const char* className, const char* attrName, unsigned int attrValue)
#else
MafReturnT setObjectUiAttribute(const char* className, const char* attrName, unsigned int attrValue)
#endif
{
	ENTER_MWSA_REPLIST();
	MafReturnT maf_rc = MafOk;
	DEBUG_MWSA_REPLIST("setObjectUiAttribute(): className %s attrName %s attrValue %u", className, attrName, attrValue);

	SaAisErrorT err = SA_AIS_OK;
	SaNameT objectName;
	classToObj(className, &objectName);
	char* an = (char*) attrName;

	// Attribute to be updated
	SaUint32T av = attrValue;
	SaImmAttrValueT avPtr = &av;
	SaImmAttrValuesT_2 comsaRlistLastIdValue = {
		an,
		SA_IMM_ATTR_SAUINT32T,
		1,
		(SaImmAttrValueT*) &avPtr
	};

	SaImmAttrModificationTypeT modType = SA_IMM_ATTR_VALUES_REPLACE;
	SaImmAttrModificationT_2 attrMod1 = { modType, comsaRlistLastIdValue };
	const SaImmAttrModificationT_2 * attrVal[] = { &attrMod1, NULL };
	DEBUG_MWSA_REPLIST("setObjectUiAttribute(): calling saImmOiRtObjectUpdate_2()");

	err = autoRetry_saImmOiRtObjectUpdate_2(rplistImmOiHandle, &objectName, (const SaImmAttrModificationT_2 **)attrVal);
	if(SA_AIS_OK != err)
	{
		// If the call returns bad handle try to initialize all the handles, set the OI and try one more time
		if (SA_AIS_ERR_BAD_HANDLE == err)
		{
			WARN_MWSA_REPLIST("setObjectUiAttribute(%s): autoRetry_saImmOiRtObjectUpdate_2 returned SA_AIS_ERR_BAD_HANDLE, try to re-init", className);
			// obtain IMM handles and set the OI
			MafReturnT maf_rc1 = RlistUtil::reset_rplistImmOiHandle();
			if( MafOk != maf_rc1 ){
				return maf_rc1;
			}
			DEBUG_MWSA_REPLIST("setObjectUiAttribute(): calling again autoRetry_saImmOiRtObjectUpdate_2 with class name %s", className);
			err = autoRetry_saImmOiRtObjectUpdate_2(rplistImmOiHandle, &objectName, (const SaImmAttrModificationT_2 **)attrVal);
			if(SA_AIS_OK != err)
			{
				ERR_MWSA_REPLIST("setObjectUiAttribute(%s): ERROR: second call to autoRetry_saImmOiRtObjectUpdate_2 failed %s", className, getOpenSAFErrorString(err));
				LEAVE_MWSA_REPLIST();
				return MafFailure;
			}
		}
		else
		{
			ERR_MWSA_REPLIST("setObjectUiAttribute(%s): ERROR: autoRetry_saImmOiRtObjectUpdate_2 failed %s", className, getOpenSAFErrorString(err));
			LEAVE_MWSA_REPLIST();
			return MafFailure;
		}
	}
	DEBUG_MWSA_REPLIST("setObjectUiAttribute(): saImmOiRtObjectUpdate_2() : SUCCESS");
	LEAVE_MWSA_REPLIST();
	return maf_rc;
}


#ifndef UNIT_TEST
static MafReturnT getObjectUiAttribute(const char* className, const char* attrName, unsigned int* uiAttrValPtr)
#else
MafReturnT getObjectUiAttribute(const char* className, const char* attrName, unsigned int* uiAttrValPtr)
#endif
{
	ENTER_MWSA_REPLIST();
	MafReturnT maf_rc = MafOk;

	unsigned int myUiAttrValue = 0;
	SaImmAttrValuesT_2 **returnedAttrValues = 0;
	int nTries = 0;
	bool found = false;
	while((++nTries <= 3)&&(found == false)){
		SaNameT objName;
		classToObj(className, &objName);
		maf_rc = readObject(&objName, &returnedAttrValues);
		if (MafOk != maf_rc)
		{
			if (MafNotExist == maf_rc)
			{
				DEBUG_MWSA_REPLIST("getObjectUiAttribute(): getObjectUiAttribute() return %d", maf_rc);
			}
			else
			{
				ERR_MWSA_REPLIST("getObjectUiAttribute(): ERROR: getObjectUiAttribute() Failed with error : %d", maf_rc);
			}
			LEAVE_MWSA_REPLIST();
			return maf_rc;
		}
		if(returnedAttrValues == 0)
		{
			LOG_MWSA_REPLIST("getObjectUiAttribute(): returnedAttrValues is 0");
		}
		else if(*returnedAttrValues == 0)
		{
			LOG_MWSA_REPLIST("getObjectUiAttribute(): *returnedAttrValues is 0");
		}
		else if (returnedAttrValues[0] == 0)
		{
			LOG_MWSA_REPLIST("getObjectUiAttribute(): returnedAttrValues[0] is 0");
		}
		else if (returnedAttrValues[0]->attrValuesNumber == 0)
		{
			//Consider this as IMMND restart.Hence needs to restart OI Handlers.
			MafReturnT maf_rc1 = RlistUtil::reset_rplistImmOiHandle();
			if( MafOk != maf_rc1 ){
				return maf_rc1;
			}
		}
		else
		{
			int i = 0;
			found = true;
			while(returnedAttrValues[i] != NULL)
			{
				char* myAtrName = (char*)returnedAttrValues[i]->attrName;
				//DEBUG_MWSA_REPLIST("getObjectUiAttribute(): attribute name: '%s'", myAtrName);
				if (strcmp(myAtrName, attrName) == 0)
				{
					DEBUG_MWSA_REPLIST("getObjectUiAttribute(): attribute name: '%s'", myAtrName);
					DEBUG_MWSA_REPLIST("getObjectUiAttribute(): attribute type: '%d'", returnedAttrValues[i]->attrValueType);
					if (NULL == returnedAttrValues[i]->attrValues)
					{
						DEBUG_MWSA_REPLIST("getObjectUiAttribute(): attribute value: <Empty value (NULL ptr)>");
					}
					else
					{
						switch (returnedAttrValues[i]->attrValueType)
						{
						case SA_IMM_ATTR_SAUINT32T:
							myUiAttrValue = *((SaUint32T *)((returnedAttrValues[i])->attrValues[0]));
							*uiAttrValPtr = myUiAttrValue;
							DEBUG_MWSA_REPLIST("getObjectUiAttribute(): attribute value: '%u'", *((SaUint32T *)((returnedAttrValues[i])->attrValues[0])));
							break;

						case SA_IMM_ATTR_SASTRINGT:
							DEBUG_MWSA_REPLIST("getObjectUiAttribute(): attribute value: '%s'", *(char**)(returnedAttrValues[i]->attrValues[0]));
							break;

						case SA_IMM_ATTR_SANAMET:
							do {
								SaConstStringT myStr = saNameGet((SaNameT*) returnedAttrValues[i]->attrValues[0]);
								if (NULL == myStr)
								{
									DEBUG_MWSA_REPLIST("getObjectUiAttribute(): attribute value: saNameGet returned NULL");
								}
								else
								{
									DEBUG_MWSA_REPLIST("getObjectUiAttribute(): attribute value: '%s'", myStr);
								}
							}while(0);
						break;

						default:
							DEBUG_MWSA_REPLIST("getObjectUiAttribute(): attribute value: <not printable at this time>");
							break;
						}
					}
					break;
				}
				else
				{
					i++;
				}
			}
		}
	}

	LEAVE_MWSA_REPLIST();
	return maf_rc;
}

std::string getRTObject(const char* listInstanceName, const unsigned int lastId){
	ENTER_MWSA_REPLIST();
	std::string immObject;
	std::ostringstream ostr;

	ostr << listInstanceName << sRuntimeIMMSuffix <<  sEqualSign << lastId;
	immObject  = ostr.str();
	DEBUG_MWSA_REPLIST("getRTObject(): MAF ObjectName: %s",immObject.c_str());

	LEAVE_MWSA_REPLIST();
	return immObject;
}

// freeAttrValue
void freeAttrValue(SaImmValueTypeT attrValueType, SaImmAttrValueT attrValue)
{
	if (attrValueType == SA_IMM_ATTR_SANAMET)
	{
		saNameDelete((SaNameT*)attrValue, false);
	}
	if(attrValueType == SA_IMM_ATTR_SAANYT){
		free(((SaAnyT*)attrValue)->bufferAddr);
	}
	free(attrValue);
}

void freeAttrValues(SaImmAttrValuesT_2* attrValues)
{
	free(attrValues->attrName);
	for (unsigned int i = 0; i < attrValues->attrValuesNumber; i++)
	{
		freeAttrValue(attrValues->attrValueType, attrValues->attrValues[i]);
	}
	free(attrValues->attrValues);
	free(attrValues);
}

SaImmAttrValueT* allocateSaAnyAttrValueArray(const void* value, unsigned int& len)
{
	SaImmAttrValueT* attrValues        = (void**) malloc(1 * sizeof(void*));
	attrValues[0]                      =  (void *) malloc(sizeof(SaAnyT));
	((SaAnyT*)attrValues[0])->bufferAddr = (SaUint8T*) malloc(sizeof(SaUint8T) * len);
	((SaAnyT*)attrValues[0])->bufferSize = len;
	memcpy(((SaAnyT*)attrValues[0])->bufferAddr, value, len);

	return attrValues;
}

//allocate attribute value for SAANY
SaImmAttrValuesT_2* allocateSaAnyAttrValues(const char* attrName, const void* value, unsigned int& len)
{
	SaImmAttrValuesT_2* attrValues = (SaImmAttrValuesT_2*) malloc(sizeof(SaImmAttrValuesT_2));
	attrValues->attrName                = strdup(attrName);
	attrValues->attrValueType           = SA_IMM_ATTR_SAANYT;
	attrValues->attrValuesNumber        = 1;
	attrValues->attrValues              = allocateSaAnyAttrValueArray(value,len);
	return attrValues;
}

SaImmAttrValueT* allocateNameAttrValueArray(const char* value)
{
	SaImmAttrValueT* attrValues        = (void**) malloc(1 * sizeof(SaImmAttrValueT));
	attrValues[0]                      =  (void *) malloc(sizeof(SaNameT));
	saNameSet(value, (SaNameT*)attrValues[0]);
	return attrValues;
}

// allocateNameAttrValues
SaImmAttrValuesT_2* allocateNameAttrValues(const char* attrName, const char* value)
{
	SaImmAttrValuesT_2* attrValues = (SaImmAttrValuesT_2*) malloc(sizeof(SaImmAttrValuesT_2));
	attrValues->attrName                = strdup(attrName);
	attrValues->attrValueType           = SA_IMM_ATTR_SANAMET;
	attrValues->attrValuesNumber        = 1;
	attrValues->attrValues              = allocateNameAttrValueArray(value);
	return attrValues;
}

#ifndef UNIT_TEST
static MafReturnT addRlistRTObject(const std::string& pClassName, const unsigned int& lastId, const void* newItemDataBuffer)
#else
MafReturnT addRlistRTObject(const std::string& pClassName, const unsigned int& lastId, const void* newItemDataBuffer)
#endif
{
	ENTER_MWSA_REPLIST();
	MafReturnT retVal = MafOk;
	// Read the list item data size from IMM
	unsigned int dataBufLen = UINT_MAX;
	if( pClassName == FM_STATUS_SECTION ){
		int nRetries = 0;
		do {
			retVal = getObjectUiAttribute(pClassName.c_str(), strRlistBlockSize, &dataBufLen);
			//getObjectUiAttribute may return MafOk code but dataBufLen is not updated
		} while (UINT_MAX == dataBufLen
				&& nRetries++ < RPLIST_MAX_RETRIES
				&& MafOk == retVal
				&& (0 == sleep(RPLIST_RETRY_SLEEP_SECOND)));

		if (MafOk == retVal && dataBufLen != UINT_MAX)
		{
			if (dataBufLen == 0)
			{
				// The actual value of blocksize in IMM is 0
				WARN_MWSA_REPLIST("addRlistRTObject: getObjectUiAttribute returned dataBufLen: 0");
			} else {
				DEBUG_MWSA_REPLIST("addRlistRTObject : getObjectUiAttribute returned dataBufLen: %u", dataBufLen);
			}
		}
		else
		{
			ERR_MWSA_REPLIST("addRlistRTObject(%s): ERROR: getObjectUiAttribute failed with %d - dataBufLen=%u", pClassName.c_str(), (int)retVal, dataBufLen);
			LEAVE_MWSA_REPLIST();
			return MafNotExist;
		}
	}else{
		char* buffer = (char*)newItemDataBuffer;
		dataBufLen = strlen(buffer);
	}
	std::string immObject= getRTObject(pClassName.c_str(), lastId);
	DEBUG_MWSA_REPLIST("addRlistRTObject(): object rdn: %s , size = %u", immObject.c_str(), (unsigned int)dataBufLen);

	SaImmAttrValuesT_2* alarmRdn = allocateNameAttrValues(sSaRlistRTdn.c_str(), immObject.c_str());
	SaImmAttrValuesT_2* alarmData = allocateSaAnyAttrValues(sData.c_str(), newItemDataBuffer, dataBufLen);

	// Combine all the attribute values in one NULL terminated array
	const SaImmAttrValuesT_2* attrValuesForObject[] = {alarmRdn, alarmData, NULL};
	std::string className(pClassName + sRuntimeIMMSuffix);
	retVal = createObject(className.c_str(), attrValuesForObject);

	freeAttrValues(alarmRdn);
	freeAttrValues(alarmData);

	if( retVal == MafAlreadyExist){
		DEBUG_MWSA_REPLIST("addRlistRTObject(): object %s already Existed", immObject.c_str());
	}else if (retVal != MafOk)
	{
		ERR_MWSA_REPLIST("addRlistRTObject(): ERROR: createObject failed for class '%s' with error code: %d", className.c_str(), retVal);
	}

	LEAVE_MWSA_REPLIST();
	return retVal;
}

SaImmAttrModificationT_2* allocateSaAnyAttrMod(const char* attrName, const void* newItemDataBuffer, unsigned int& dataBufLen) {

	SaImmAttrModificationT_2* attrMod = (SaImmAttrModificationT_2*) malloc(sizeof(SaImmAttrModificationT_2));

	attrMod->modType = SA_IMM_ATTR_VALUES_REPLACE;
	attrMod->modAttr.attrName 		=  strdup(attrName);
	attrMod->modAttr.attrValueType     	= SA_IMM_ATTR_SAANYT;
	attrMod->modAttr.attrValuesNumber  	= 1;
	attrMod->modAttr.attrValues        	= allocateSaAnyAttrValueArray(newItemDataBuffer, dataBufLen);
	return attrMod;
}

void freeModAttrValues(SaImmAttrModificationT_2 *modAttrValues) {

	free(modAttrValues->modAttr.attrName);

	for (unsigned int i = 0; i < modAttrValues->modAttr.attrValuesNumber; i++)
	{
		freeAttrValue(modAttrValues->modAttr.attrValueType, modAttrValues->modAttr.attrValues[i]);
	}
	free(modAttrValues->modAttr.attrValues);
	free(modAttrValues);
}

#ifndef UNIT_TEST
static MafReturnT modifyRlistRTObject(const std::string& pClassName, const std::string& lastId, const void* newItemDataBuffer)
#else
MafReturnT modifyRlistRTObject(const std::string& pClassName, const std::string& lastId, const void* newItemDataBuffer)
#endif
{
	ENTER_MWSA_REPLIST();
	MafReturnT retVal = MafOk;

	// Read the list item data size from IMM
	unsigned int dataBufLen = UINT_MAX;
	if( pClassName == FM_STATUS_SECTION ){
		int nRetries = 0;
		do {
			retVal = getObjectUiAttribute(pClassName.c_str(), strRlistBlockSize, &dataBufLen);

			//getObjectUiAttribute may return MafOk code but dataBufLen is not updated
		} while (UINT_MAX == dataBufLen
				&& nRetries++ < RPLIST_MAX_RETRIES
				&& MafOk == retVal
				&& (0 == sleep(RPLIST_RETRY_SLEEP_SECOND)));

		if (MafOk == retVal && dataBufLen != UINT_MAX)
		{
			if (dataBufLen == 0)
			{
				// The actual value of blocksize in IMM is 0
				WARN_MWSA_REPLIST("modifyRlistRTObject(): getObjectUiAttribute returned dataBufLen: 0");
			} else {
				DEBUG_MWSA_REPLIST("modifyRlistRTObject(): getObjectUiAttribute returned dataBufLen: %u", dataBufLen);
			}
		}
		else
		{
			ERR_MWSA_REPLIST("modifyRlistRTObject(%s): ERROR: getObjectUiAttribute failed with %d - dataBufLen=%u", pClassName.c_str(), (int)retVal, dataBufLen);
			LEAVE_MWSA_REPLIST();
			return MafNotExist;
		}
	} else {
		char* buffer = (char*)newItemDataBuffer;
		dataBufLen = strlen(buffer);
	}

	// Alarm Data of the AAL object.
	SaNameT *immObject = NULL;
	retVal = convertToRTObjectName(pClassName.c_str(), lastId, &immObject);

	if (retVal != MafOk)
	{
		ERR_MWSA_REPLIST("modifyRlistRTObject(): ERROR: convertToRTObjectName failed for class '%s' with error code: %d", pClassName.c_str(), retVal);
		LEAVE_MWSA_REPLIST();
		return retVal;
	}

	SaImmAttrModificationT_2 *attrValuesForModifyObject = allocateSaAnyAttrMod(sData.c_str(), newItemDataBuffer, dataBufLen);
	SaImmAttrModificationT_2* pSaImmAttrModification[2] = {attrValuesForModifyObject, NULL};

	retVal = modifyObject(immObject, (const SaImmAttrModificationT_2**)(pSaImmAttrModification));

	if (retVal != MafOk)
	{
		ERR_MWSA_REPLIST("modifyRlistRTObject(): ERROR: modifyObject failed for class '%s' with error code: %d",
				saNameGet(immObject), retVal);
	}

	freeModAttrValues(attrValuesForModifyObject);
	saNameDelete( (SaNameT*) immObject , true);
	LEAVE_MWSA_REPLIST();
	return retVal;
}

bool RlistUtil::validateName(const MafMwSpiListNameT* name)
{
	bool ret = true;
	if ((name == NULL) ||
		(name->length == 0) ||
		(name->length > MW_SPI_MAX_NAME_LENGTH))
	{
		WARN_MWSA("Check validateName is invalid argument");
		ret = false;
	}
	return ret;
}

// Use the class name to create RDN as SaNameT for the object
void RlistUtil::classToObj(const char* className, SaNameT* rdnName)
{
	ENTER_MWSA_REPLIST();
	char * charName = (char*)malloc(strlen(className) + strlen("=1") + 1);
	if (NULL == charName)
	{
		ERR_MWSA_REPLIST("classToObj(): ERROR: Failed to allocate memory %s %s", className, saNameGet(rdnName));
	}
	else
	{
		strcpy(charName, className);
		strcat(charName, "=1");
		saNameSet(charName, rdnName);
	}

	if (NULL != charName)
	{
		free(charName);
		charName = NULL;
	}
	LEAVE_MWSA_REPLIST();
}


// Find the next list item name, take the current name as unsigned int number
// Copy the name to the char* provided and return error code (success/failure)
MafReturnT RlistUtil::findNextListItem(const MafMwSpiListNameT* listInstanceName,
                                       const int currentItemName,
                                       char* nextListItemName,
                                       unsigned int* intNextListItemName,
                                       SaImmAttrValuesT_2*** attrValues)
{
	ENTER_MWSA_REPLIST();
	MafReturnT maf_rc = MafOk;
	unsigned int theListNextItem = currentItemName;

	/*
	 * What is the maximum number of list elements allowed in any list?
	 * We can safely assume that a list item can not be bigger than the list lastId.
	 */

	// Get the list lastId from IMM
	unsigned int lastId = 0;
	maf_rc = getObjectUiAttribute((char*) listInstanceName->value, strRlistLastId, &lastId);
	if (MafOk == maf_rc)
	{
		DEBUG_MWSA_REPLIST("findNextListItem(): getObjectUiAttribute returned lastId: %u", lastId);
	}
	else
	{
		ERR_MWSA_REPLIST("findNextListItem(%s): ERROR: getObjectUiAttribute failed with %d", (char*) listInstanceName->value, (int) maf_rc);
		LEAVE_MWSA_REPLIST();
		return maf_rc;
	}

	bool itemFound = false;

	while (lastId >= theListNextItem)
	{
		MafReturnT rc_file = MafFailure;
		// Check if the item exists in the file storage. If not then return MafNotExist
		snprintf(nextListItemName , MAX_ITEM_NAME_LEN, "%llu", (unsigned long long) theListNextItem);
		if (replicateInIMM) {
			SaNameT *immObject = NULL;
			rc_file = convertToRTObjectName(std::string((char*)(listInstanceName->value)), std::string(nextListItemName), &immObject);
			if (MafOk != rc_file) {
				DEBUG_MWSA_REPLIST("listFindItem(): Unable to convert to IMM object name");
				return rc_file;
			}
			rc_file = readObject(immObject, attrValues);
			saNameDelete(immObject, true);
		}
		else {
			rc_file = findFile((const char*) nextListItemName, (const char*) listInstanceName->value);
		}
		if (MafOk == rc_file)
		{
			itemFound = true;
			break;
		}
		theListNextItem++;
	}

	if (itemFound)
	{
		*intNextListItemName = theListNextItem;
		DEBUG_MWSA_REPLIST("findNextListItem(): Next List item found is '%s', [%d]", nextListItemName, *intNextListItemName);
		maf_rc = MafOk;
	}
	else
	{
		ERR_MWSA_REPLIST("findNextListItem(%s): ERROR: Next List item not found", (char*) listInstanceName->value);
		maf_rc = MafFailure;
	}

	LEAVE_MWSA_REPLIST();
	return maf_rc;
}


#ifndef UNIT_TEST
// Debug print object
void RlistUtil::comsa_rplist_imm_debugPrintObject(SaImmAttrValuesT_2 **returnedAttrValues)
{
	ENTER_MWSA_REPLIST();


	if((returnedAttrValues) == 0)
	{
		DEBUG_MWSA_REPLIST("comsa_rplist_imm_debugPrintObject(): returnedAttrValues is 0");
	}
	else if((*returnedAttrValues) == 0)
	{
		DEBUG_MWSA_REPLIST("comsa_rplist_imm_debugPrintObject(): *returnedAttrValues is 0");
	}
	else if (returnedAttrValues[0] == 0)
	{
		DEBUG_MWSA_REPLIST("comsa_rplist_imm_debugPrintObject(): returnedAttrValues[0] is 0");
	}
	else if (returnedAttrValues[0]->attrValuesNumber == 0)
	{
		DEBUG_MWSA_REPLIST("comsa_rplist_imm_debugPrintObject(): returnedAttrValues[0]->attrValuesNumber is 0");
	}
	else
	{
		int i = 0;
		while(returnedAttrValues[i] != NULL)
		{
			char* myAtrName = (char*)returnedAttrValues[i]->attrName;

			DEBUG_MWSA_REPLIST("comsa_rplist_imm_debugPrintObject(): attribute name: '%s'", myAtrName);
			DEBUG_MWSA_REPLIST("comsa_rplist_imm_debugPrintObject(): attribute type: '%d'", returnedAttrValues[i]->attrValueType);
			if (NULL == returnedAttrValues[i]->attrValues)
			{
				DEBUG_MWSA_REPLIST("comsa_rplist_imm_debugPrintObject(): attribute value: <Empty value (NULL ptr)>");
			}
			else
			{
				switch (returnedAttrValues[i]->attrValueType)
				{
				case SA_IMM_ATTR_SAUINT32T:
					DEBUG_MWSA_REPLIST("comsa_rplist_imm_debugPrintObject(): attribute value: '%d'", *((SaUint32T *)((returnedAttrValues[i])->attrValues[0])));
					break;

				case SA_IMM_ATTR_SASTRINGT:
					DEBUG_MWSA_REPLIST("comsa_rplist_imm_debugPrintObject(): attribute value: '%s'", *(char**)(returnedAttrValues[i]->attrValues[0]));
					break;

				case SA_IMM_ATTR_SANAMET:
					{
						SaConstStringT myStr = saNameGet((SaNameT*) returnedAttrValues[i]->attrValues[0]);
						if (NULL == myStr)
						{
							DEBUG_MWSA_REPLIST("comsa_rplist_imm_debugPrintObject(): attribute value: saNameGet returned NULL");
						}
						else
						{
							DEBUG_MWSA_REPLIST("comsa_rplist_imm_debugPrintObject(): attribute value: '%s'", myStr);
						}
					}
					break;

				default:
					DEBUG_MWSA_REPLIST("comsa_rplist_imm_debugPrintObject(): attribute value: <not printable at this time>");
					//break;
				}
			}
			i++;
		}
	}
	LEAVE_MWSA_REPLIST();
}
#endif  // ifndef UNIT_TEST


MafReturnT RlistUtil::comsa_rplist_imm_deleteDir(const char* subDir)
{
	ENTER_MWSA_REPLIST();
	MafReturnT maf_rc = MafOk;
	struct stat st;

	char *dirName = NULL;
	const char *pathArray[4];
	pathArray[0] = pathToStorageDirectory;
	pathArray[1] = COMSA_FOR_COREMW_DIR;
	pathArray[2] = strRpListPath;
	pathArray[3] = subDir;
	dirName = createPath(pathArray, 4, NULL);
	if (dirName == NULL) {
		ERR_MWSA_REPLIST("comsa_rplist_imm_deleteDir(): ERROR: Failed to allocate memory for dirName %s", subDir);
		maf_rc = MafFailure;
	}
	else {
		if(0 != stat(dirName, &st))
		{
			// If the directory does not exist, it means there is no need further actions. Return MafOk
			WARN_MWSA_REPLIST("comsa_rplist_imm_deleteDir(): %s doesn't exists", dirName);
			maf_rc = MafOk;
		}
		else
		{
			if(0 == rmdir(dirName))
			{
				DEBUG_MWSA_REPLIST("comsa_rplist_imm_deleteDir(): Deleted directory [%s]", dirName);
			}
			else
			{
				ERR_MWSA_REPLIST("comsa_rplist_imm_deleteDir(): ERROR: Failed to delete directory [%s]", dirName);
				maf_rc = MafFailure;
			}
		}
	}

	if(dirName != NULL)
	{
		free(dirName);
		dirName = NULL;
	}

	LEAVE_MWSA_REPLIST();
	return maf_rc;
}


// Create a path to a file
char* RlistUtil::createPath(const char** pathArray, const int nNumStrings, const char *fileName)
{
	ENTER_MWSA_REPLIST();
	char *filePath = NULL;
	int nString = 0;

	if (pathArray == NULL) {
		ERR_MWSA_REPLIST("createPath(): ERROR: pathArray == NULL");
	}
	else {
		if (nNumStrings == 0) {
			ERR_MWSA_REPLIST("createPath(): ERROR: nNumStrings == 0");
		}
		else {
			// Start joining the strings
			filePath = (char*)malloc(MAX_PATH_DATA_LENGTH);
			if (pathArray[nString] == NULL) {
				ERR_MWSA_REPLIST("createPath(): ERROR: pathArray[0] == NULL");
			}
			else {
				strcpy(filePath, pathArray[nString]);
				for (nString = 1; nString < nNumStrings; nString++)
				{
					if (filePath[strlen(filePath)-1] != '/') {
						strcat(filePath, "/");
					}
					if (pathArray[nString] == NULL) {
						ERR_MWSA_REPLIST("createPath(): ERROR: pathArray[%d] == NULL", nString);
					}
					else {
						strcat(filePath, pathArray[nString]);
					}
				}

				if (fileName != NULL)
				{
					if (filePath[strlen(filePath)-1] != '/') {
						strcat(filePath, "/");
					}
					strcat(filePath, fileName);
				}
			}
		}
	}
	if (filePath != NULL) DEBUG_MWSA_REPLIST("createPath(): %s", filePath);
	LEAVE_MWSA_REPLIST();
	return filePath;
}

SaImmAttrDefinitionT_2** RlistUtil::prepareRTImmAttrDefinitions()
{
	ENTER_MWSA_REPLIST();
	/*	typedef struct {
	 *		SaImmAttrNameT attrName;
	 *		SaImmValueTypeT attrValueType;
	 *		SaImmAttrFlagsT attrFlags;
	 *		SaImmAttrValueT attrDefaultValue;
	 *	} SaImmAttrDefinitionT_2;
	 *
	 * We can possibly allocate	memory fot these but it is done only once.
	 * And will need to work out where and when to free this allocated memory
	 *
	 **********************************************************************
	 * Create IMM class for the Replicated List
	 *
	 * Class Attributes:
	 *
	 *	   saRlistRTdn    SA_NAME_T
	 *	   data           SA_ANY_T
	 */

	// Rdn of AAL object in IMM
	SaImmAttrDefinitionT_2* rdn = (SaImmAttrDefinitionT_2*)malloc(sizeof(SaImmAttrDefinitionT_2));
	rdn->attrName = strdup(sSaRlistRTdn.c_str());
	rdn->attrValueType = SA_IMM_ATTR_SANAMET;
	rdn->attrFlags = SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_RDN | SA_IMM_ATTR_CACHED;
	rdn->attrDefaultValue = NULL;

	// Alarm Data(Content) of the AAL object.
	SaImmAttrDefinitionT_2* data = (SaImmAttrDefinitionT_2*)malloc(sizeof(SaImmAttrDefinitionT_2));
	data->attrName = strdup(sData.c_str());
	data->attrValueType = SA_IMM_ATTR_SAANYT;
	data->attrFlags = SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_CACHED;
	data->attrDefaultValue = NULL;

	// this is the NULL terminated array of attributes for the Replicated List IMM class
	SaImmAttrDefinitionT_2** tAttrDef = (SaImmAttrDefinitionT_2**)malloc(3 * sizeof(SaImmAttrDefinitionT_2*));
	tAttrDef[0] = rdn;
	tAttrDef[1] = data;
	tAttrDef[2] = NULL;

	LEAVE_MWSA_REPLIST();
	return tAttrDef;
}

SaImmAttrDefinitionT_2** RlistUtil::prepareImmAttrDefinitions()
{
	ENTER_MWSA_REPLIST();
	/*      typedef struct {
	 *              SaImmAttrNameT attrName;
	 *              SaImmValueTypeT attrValueType;
	 *              SaImmAttrFlagsT attrFlags;
	 *              SaImmAttrValueT attrDefaultValue;
	 *      } SaImmAttrDefinitionT_2;
	 *
	 * We can possibly allocate     memory fot these but it is done only once.
	 * And will need to work out where and when to free this allocated memory
	 *
	 **********************************************************************
	 * Create IMM class for the Replicated List
	 *
	 * Class Attributes:
	 *
	 *	   listRdn
	 *	   comsaRlistBlockSize
	 *	   comsaRlistLastId
	 *	   comsaRlistNumElements
	 */

	// this is the list name provided by COM
	SaImmAttrDefinitionT_2* listRdn = (SaImmAttrDefinitionT_2*)malloc(sizeof(SaImmAttrDefinitionT_2));
	listRdn->attrName = strdup(strRlistClassRdn);
	listRdn->attrValueType = SA_IMM_ATTR_SANAMET;
	listRdn->attrFlags = SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_RDN | SA_IMM_ATTR_CACHED;
	listRdn->attrDefaultValue = NULL;

	// this is the list elements fixed block size provided by COM
	SaImmAttrDefinitionT_2* comsaRlistBlockSize = (SaImmAttrDefinitionT_2*)malloc(sizeof(SaImmAttrDefinitionT_2));
	comsaRlistBlockSize->attrName = strdup(strRlistBlockSize);
	comsaRlistBlockSize->attrValueType = SA_IMM_ATTR_SAUINT32T;
	comsaRlistBlockSize->attrFlags = SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_CACHED;
	comsaRlistBlockSize->attrDefaultValue = NULL;

	// this is the name of the last element in the list.
	// The name is created by COM SA as a sequence number starting from '1' for the first element added
	// Stored as UINT32 in IMM. To be converted to ASCII text when given to COM and the other way around
	SaImmAttrDefinitionT_2* comsaRlistLastId = (SaImmAttrDefinitionT_2*)malloc(sizeof(SaImmAttrDefinitionT_2));
	comsaRlistLastId->attrName = strdup(strRlistLastId);
	comsaRlistLastId->attrValueType = SA_IMM_ATTR_SAUINT32T;
	comsaRlistLastId->attrFlags = SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_CACHED;
	comsaRlistLastId->attrDefaultValue = NULL;

	// this is the current number of elements in the list
	SaImmAttrDefinitionT_2* comsaRlistNumElements = (SaImmAttrDefinitionT_2*)malloc(sizeof(SaImmAttrDefinitionT_2));
	comsaRlistNumElements->attrName = strdup(strRlistNumElements);
	comsaRlistNumElements->attrValueType = SA_IMM_ATTR_SAUINT32T;
	comsaRlistNumElements->attrFlags = SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_CACHED;
	comsaRlistNumElements->attrDefaultValue = NULL;

	// this is the NULL terminated array of attributes for the Replicated List IMM class
	SaImmAttrDefinitionT_2** tAttrDef = (SaImmAttrDefinitionT_2**)malloc(5 * sizeof(SaImmAttrDefinitionT_2*));
	tAttrDef[0] = listRdn;
	tAttrDef[1] = comsaRlistBlockSize;
	tAttrDef[2] = comsaRlistLastId;
	tAttrDef[3] = comsaRlistNumElements;
	tAttrDef[4] = NULL;

	LEAVE_MWSA_REPLIST();
	return tAttrDef;
}

void RlistUtil::freeImmAttrDefinitions(SaImmAttrDefinitionT_2*** tAttrDef)
{
	ENTER_MWSA_REPLIST();
	if (*tAttrDef) {
		SaImmAttrDefinitionT_2** attrDef = *tAttrDef;
		for(int i = 0; attrDef[i] != NULL; i++)
		{
			if(attrDef[i]->attrName) {
				free(attrDef[i]->attrName);
				attrDef[i]->attrName = NULL;
			}

			free(attrDef[i]);
			attrDef[i] = NULL;
		}
		free(*tAttrDef);
		*tAttrDef = NULL;
	}
	LEAVE_MWSA_REPLIST();
}

#ifndef UNIT_TEST
static MafReturnT comsa_rplist_immOi_set()
#else
MafReturnT comsa_rplist_immOi_set()
#endif
{
	ENTER_MWSA_REPLIST();
	MafReturnT maf_rc = MafOk;
	SaAisErrorT err = SA_AIS_OK;

	// Obtain IMM handle to be kept and used for all IMM operations
	maf_rc = comsa_rplist_imm_init(&rplistImmHandle);
	if (MafOk != maf_rc)
	{
		ERR_MWSA_REPLIST("comsa_rplist_immOi_set(): ERROR: Failed to obtain IMM handle with error: %d", (int) maf_rc);
		LEAVE_MWSA_REPLIST();
		return maf_rc;
	}

	DEBUG_MWSA_REPLIST("comsa_rplist_immOi_set(): comsa_rplist_imm_init : SUCCESS, returned IMM handle: %lu", (unsigned long) rplistImmHandle);
	DEBUG_MWSA_REPLIST("comsa_rplist_immOi_set(): calling comsa_rplist_immOi_init");

	// Obtain IMM OI handle to be kept and used for all IMM operations
	maf_rc = comsa_rplist_immOi_init(&rplistImmOiHandle);
	if (MafOk != maf_rc)
	{
		ERR_MWSA_REPLIST("comsa_rplist_immOi_set(): ERROR: Failed to obtain IMM OI handle with error: %d", (int) maf_rc);
		LEAVE_MWSA_REPLIST();
		return maf_rc;
	}

	DEBUG_MWSA_REPLIST("comsa_rplist_immOi_set(): comsa_rplist_immOi_init : SUCCESS, returned IMM OI handle: %lu", (unsigned long) rplistImmOiHandle);

	// Set implementer for the object
	DEBUG_MWSA_REPLIST("comsa_rplist_immOi_set(): calling autoRetry_saImmOiImplementerSet");

	// Declare an Implementer name for Runtime object
	err = autoRetry_saImmOiImplementerSet(rplistImmOiHandle, (char*) implementerName);

	if ( SA_AIS_OK != err )
	{
		if( ( err == SA_AIS_ERR_NO_RESOURCES ) && retries++<3 )
		{
			LOG_MWSA_REPLIST("autoRetry_saImmOiImplementerSet receives SA_AIS_ERR_NO_RESOURCES, hence re-initializing handlers at attempt (%d)", retries);
			comsa_rplist_immOi_finalize(&rplistImmOiHandle);
			RlistUtil::unsetFromEnvironment("IMMA_OI_CALLBACK_TIMEOUT");
			comsa_rplist_immOi_set();
		}
		else
		{
			ERR_MWSA_REPLIST("comsa_rplist_immOi_set(): ERROR: saImmOiObjectImplementerSet() Failed with error : %s", getOpenSAFErrorString(err));
			LEAVE_MWSA_REPLIST();
			return MafFailure;
		}
	}
	else
	{
		DEBUG_MWSA_REPLIST("comsa_rplist_immOi_set(): saImmOiObjectImplementerSet() : SUCCESS");
		retries=0;
	}
	// Obtain IMM OAccessor  handle to be kept and used for all IMM operations
	maf_rc = comsa_rplist_immAccess_init(rplistImmHandle, &immAccessHandle);

	LEAVE_MWSA_REPLIST();
	return maf_rc;
}


/*
 * This function is called by listCreate
 * It is used to detect if the cluster is rebooted only or rebooted after restoring a backup
 */
#ifndef UNIT_TEST
static MafReturnT detectClusterState(const MafMwSpiListNameT* listInstanceName, unsigned int* stateFlag)
#else
MafReturnT detectClusterState(const MafMwSpiListNameT* listInstanceName, unsigned int* stateFlag)
#endif
{
	ENTER_MWSA_REPLIST();
	MafReturnT maf_rc = MafOk;
	SaImmAttrValuesT_2 **returnedAttrValues = 0;

	if(false == replicateInIMM){
		struct stat st;
		const char* subDir = (char*)listInstanceName->value;
		char *dirName = NULL;
		if(MafOk != getBaseRepository())
		{
			ERR_MWSA_REPLIST("detectClusterState(%s): ERROR: Could not obtain path for the base replicated list directory", subDir);
			maf_rc = MafFailure;
		}
		else
		{
			const char *pathArray[4];
			pathArray[0] = pathToStorageDirectory;
			pathArray[1] = COMSA_FOR_COREMW_DIR;
			pathArray[2] = strRpListPath;
			pathArray[3] = subDir;
			dirName = createPath(pathArray, 4, NULL);

			if(0 != stat(dirName, &st))
			{
				DEBUG_MWSA_REPLIST("detectClusterState(): '%s' doesn't exist. CLUSTER IS CLEAN", (char*) listInstanceName->value);
				*stateFlag = (unsigned int) CLUSTER_RESTORED;
				maf_rc = MafOk;
			}
			else
			{
				SaNameT objName;
				classToObj((char*) listInstanceName->value, &objName);
				maf_rc = readObject(&objName, &returnedAttrValues);
				if(MafNotExist == maf_rc)
				{
					DEBUG_MWSA_REPLIST("detectClusterState(): '%s' exist but the corresponding RT OBJECT doesn't exist. CLUSTER IS REBOOTED", (char*) listInstanceName->value);
					*stateFlag = (unsigned int) CLUSTER_REBOOTED_ONLY;
				}
				else
				{
					if(MafOk == maf_rc)
					{
						/* In this case, COM requests that COMSA will create a list for new ReplList
						 * But if an object of ReplList already exists, it could be a failure of system
						 * in future COMSA should have a relevant action in this case if needed
						 */
						DEBUG_MWSA_REPLIST("detectClusterState(): '%s' exist and the corresponding RT OBJECT exist", (char*) listInstanceName->value);
						maf_rc = validateImmData(listInstanceName, dirName);
						*stateFlag = (unsigned int) CLUSTER_RT_OBJ_EXIST;
					}
					else
					{
						ERR_MWSA_REPLIST("detectClusterState(%s): ERROR: couldn't read IMM Object", subDir);
					}
				}

			}
		}

		if(NULL != dirName)
		{
			free(dirName);
			dirName = NULL;
		}
	} else {
		SaNameT objName;
		classToObj((char*) listInstanceName->value, &objName);
		maf_rc = readObject(&objName, &returnedAttrValues);
		if(MafNotExist == maf_rc)
		{
			DEBUG_MWSA_REPLIST("detectClusterState(): '%s' exist but the corresponding RT OBJECT doesn't exist. CLUSTER IS REBOOTED", (char*) listInstanceName->value);
			*stateFlag = (unsigned int) CLUSTER_REBOOTED_ONLY;
		} else {
			if(MafOk == maf_rc)
			{
				/* In this case, COM requests that COMSA will create a list for new ReplList
				 * But if an object of ReplList already exists, it could be a failure of system
				 * in future COMSA should have a relevant action in this case if needed
				 */
				DEBUG_MWSA_REPLIST("detectClusterState(): '%s' exist and the corresponding RT OBJECT exist", (char*) listInstanceName->value);
				maf_rc = validateImmData(listInstanceName, NULL);
				*stateFlag = (unsigned int) CLUSTER_RT_OBJ_EXIST;
			}
			else
			{
				ERR_MWSA_REPLIST("detectClusterState(%s): ERROR: couldn't read IMM Object", (char*)listInstanceName->value);
			}
		}
	}

	LEAVE_MWSA_REPLIST();
	return maf_rc;
}

/*
 * This function is called by listCreate
 * It is used to get the details of AAL and StatusSection using NFS, and set the values in IMM
  */
#ifndef UNIT_TEST
static MafReturnT restore_immData (const MafMwSpiListNameT* listInstanceName)
#else
MafReturnT restore_immData (const MafMwSpiListNameT* listInstanceName)
#endif
{
	MafReturnT maf_rc = MafOk;
	DIR *pDir;
	char *dirName = NULL;
	const char* subDir = (char*)listInstanceName->value;
	const char *pathArray[4];
	uint32_t actualNumOfFiles = 0;
	uint32_t lastFile = 0;
	pathArray[0] = pathToStorageDirectory;
	pathArray[1] = COMSA_FOR_COREMW_DIR;
	pathArray[2] = strRpListPath;
	pathArray[3] = subDir;
	dirName = createPath(pathArray, 4, NULL);

	DEBUG_MWSA_REPLIST("restore_immData className %s", subDir);
	pDir = opendir(dirName);
	if (NULL == pDir) {
		ERR_MWSA_REPLIST("restore_immData(): Unable to open dir '%s' ... ", dirName);
		maf_rc = MafFailure;
	} else {
		size_t len = 0;
		struct dirent *pDirent;
		uint32_t fileNum;
		char *fileName;
		char buff[MAX_PATH_DATA_LENGTH];
		// Count the actual number of NFS file
		while (NULL != (pDirent = readdir(pDir))) {
			len = strlen(pDirent->d_name);
			if (len >= 4)
			{
				if (strcmp(strBin, &(pDirent->d_name[len - 4])) == 0)
				{
					actualNumOfFiles++;
					strcpy(buff, pDirent->d_name);
					if ((fileName = strtok(buff, ".")) != NULL) {
						fileNum = (uint32_t)atol(fileName);
						if(fileNum > lastFile) {
							lastFile = fileNum;
						}
					} else {
						ERR_MWSA_REPLIST("restore_immData() unmatched file: %s", buff);
					}
					memset(buff, '\0', len);
				}
			}
		}
		closedir(pDir);
	}

	DEBUG_MWSA_REPLIST("restore_immData no of files = %u, lastFile= %u", actualNumOfFiles, lastFile);

	if(actualNumOfFiles > 0)
	{
		setObjectUiAttribute((char*) listInstanceName->value, strRlistNumElements, actualNumOfFiles);
		setObjectUiAttribute((char*) listInstanceName->value, strRlistLastId, lastFile);
		//As the FmStatusSection list is not empty, needs to return MafAlreadyExists to inform the
		//applications(which uses rplist interface) use the status info which is stored in the list of files.
		maf_rc = MafAlreadyExist;
	}

	return maf_rc;
}

/*
 * This function is called by detectClusterState
 * It is used to validate the IMM data is matching with related rplist NFS or not
 * If data is mismatch, reset IMM data and clear rplist NFS
 */
#ifndef UNIT_TEST
static MafReturnT validateImmData(const MafMwSpiListNameT* listInstanceName, const char* dirName)
#else
MafReturnT validateImmData(const MafMwSpiListNameT* listInstanceName, const char* dirName)
#endif
{
	MafReturnT maf_rc = MafOk;

	uint32_t listSize = 0;
	DEBUG_MWSA_REPLIST("validateImmData calling listGetSize()...");
	maf_rc = OamSARList::instance().listGetSize(listInstanceName, &listSize);
	if (MafOk != maf_rc)
	{
		ERR_MWSA_REPLIST("validateImmData(%s): ERROR: listGetSize() failed, rc = %d", (char*) listInstanceName->value, (int) maf_rc);
		LEAVE_MWSA_REPLIST();
		return MafNotExist;    // The MAF RL SPI does not allow all return codes
	}

	// Count the actual number of NFS file or number of objects in IMM
	uint32_t actualDataSize = 0;
	if(false == replicateInIMM){
		char cmd[MAX_PATH_DATA_LENGTH];
		char buff[MAX_PATH_DATA_LENGTH];
		sprintf (cmd, "ls %s | grep -c -i %s", dirName,strBin);
		unsigned int nTries = RPLIST_MAX_RETRIES;
		FILE *fpipe = (FILE*)popen(cmd, "r");

		while(NULL == fpipe && nTries > 0)
		{
			sleep(RPLIST_RETRY_SLEEP_SECOND);
			fpipe = (FILE*)popen(cmd, "r");
			nTries--;
		}

		if (fpipe)
		{
			DEBUG_MWSA_REPLIST("validateImmData(): command '%s' was successful", cmd);
		}
		else
		{
			ERR_MWSA_REPLIST("validateImmData(): ERROR: command '%s' failed", cmd);
			return MafNotExist;
		}

		while(fgets(buff, sizeof(buff), fpipe)!=NULL){
			break;
		}

		if ((pclose(fpipe)) == -1)
		{
			ERR_MWSA_REPLIST("validateImmData(): ERROR: cannot close fpipe.");
			return MafNotExist;
		}
		actualDataSize = (uint32_t)atoi(buff);
	}else{
		maf_rc = getRTObjectsCount((char*) listInstanceName->value, actualDataSize);
		if( MafOk != maf_rc ) {
			return  MafNotExist;
		}
		DEBUG_MWSA_REPLIST("validateImmData():  Count of RT objects = %u ret = %d", actualDataSize, (int)maf_rc );
	}
	if (actualDataSize != listSize)
	{
		WARN_MWSA_REPLIST("validateImmData(%s): Mismatch IMM database and NFS on [%u != %u]",(char*) listInstanceName->value, listSize, actualDataSize);
		// Data is corrupted. Reset IMM attribute
		setObjectUiAttribute((char*) listInstanceName->value, strRlistNumElements, 0);
		setObjectUiAttribute((char*) listInstanceName->value, strRlistLastId, 0);
		maf_rc = MafNotExist;
	}
	return maf_rc;
}

/**
 * convertToRTObjectName Builds IMM Object name
 *
 * @param[in]  listInstanceName Pointer to MafMwSpiListNameT.
 *
 * @param[in]  lastId Instance name of the object.
 *
 * @param[out] immObject Pointer to SaNameT*
 *             Memory is allocated through makeSaNameT().
 *             The caller should deallocate the memory
 *             using saNameDelete() as/when needed.
 *
 * @return MafOk,
 *         MafInvalidArgument if the @param[in] not valid,
 *         MafFailure         if the conversion failed,
 *         MafNoResources     if no memory is available and there is a reasonable
 *                            chance that COM can continue execution.
 *
 */
MafReturnT RlistUtil::convertToRTObjectName(const std::string& listInstanceName, const std::string& lastId, SaNameT** immObject)
{
	ENTER_MWSA_REPLIST();

	if (lastId == "0") {
		LEAVE_MWSA_REPLIST();
		return MafInvalidArgument;
	}

	std::string tempStr = listInstanceName + sRuntimeIMMSuffix + sEqualSign + lastId;

	DEBUG_MWSA_REPLIST("RlistUtil::convertToRTObjectName(): MAF ObjectName: %s",tempStr.c_str());

	(*immObject) = makeSaNameT(tempStr.c_str());

	if (*immObject == NULL) {
		ERR_MWSA_REPLIST("RlistUtil::convertToRTObjectName():makeSaNameT() conversion failed.");
		LEAVE_MWSA_REPLIST();
		return MafFailure;
	}

	LEAVE_MWSA_REPLIST();

	return MafOk;
}

/**
 * getVoidData Copies SaAnyT->bufferAddr to data.
 *
 * @param[in]  attrVal Pointer to SaImmAttrValuesT_2* data.
 *
 * @param[out] data Pointer to void*
 *             Memory is allocated to *data with malloc().
 *             The caller should deallocate the memory
 *             using free() as/when needed.
 *
 * @param[out] dataLen is length of void data.
 *
 * @param[out] lastId reference to Instance name of the object.
 *
 * @return MafOk,
 *         MafInvalidArgument if the @param[in] not valid,
 *         MafNoResources     if no memory is available and there is a reasonable
 *                            chance that COM can continue execution.
 */
MafReturnT RlistUtil::getVoidData(const SaImmAttrValuesT_2** attrVal, void** data, unsigned int dataLen) {

	ENTER_MWSA_REPLIST();

	if (attrVal == NULL) {
		LEAVE_MWSA_REPLIST();
		return MafInvalidArgument;
	}

	for (int i = 0; attrVal[i] != NULL; i++) {

		if (0 == strcmp(attrVal[i]->attrName, sData.c_str())) {

			SaAnyT* saAnyValue = reinterpret_cast<SaAnyT*>(attrVal[i]->attrValues[0]);
			DEBUG_MWSA_REPLIST("getVoidData():saAnyValue.bufferSize: %llu", saAnyValue->bufferSize);

			if(saAnyValue->bufferSize < UINT_MAX) {
				memset(*data, 0, dataLen);
				memcpy((*data), saAnyValue->bufferAddr, saAnyValue->bufferSize);
				LEAVE_MWSA_REPLIST();
				return MafOk;
			}
			break;
		}
	}
	LEAVE_MWSA_REPLIST();
	return MafInvalidArgument;
}

#ifndef UNIT_TEST
static MafReturnT deleteRTObject(const std::string& listInstanceName, const std::string& listItemName)
#else
MafReturnT deleteRTObject(const std::string& listInstanceName, const std::string& listItemName)
#endif
{
	ENTER_MWSA_REPLIST();

	SaNameT *immObject = NULL;
	MafReturnT rc = MafNotExist;
	rc = convertToRTObjectName(listInstanceName, listItemName, &immObject);
	if (rc != MafOk)
	{
		ERR_MWSA_REPLIST("deleteRTObjecct(): ERROR: convertToRTObjectName failed for class '%s' with error code: %d",
							listInstanceName.c_str(), rc);
		LEAVE_MWSA_REPLIST();
		return MafNoResources;
	}

	rc= deleteObject(immObject);
	if (rc != MafOk)
	{
		ERR_MWSA_REPLIST("deleteRTObject(): ERROR: deleteObject failed for '%s'  with error code: %d",
							saNameGet(immObject), rc);
		LEAVE_MWSA_REPLIST();
		rc = MafNotExist;
	}
	else
	{
		DEBUG_MWSA_REPLIST("deleteRTObject(): INFO: deleteObject(%s) was successful",
				saNameGet(immObject));
	}

	saNameDelete(immObject, true);

	LEAVE_MWSA_REPLIST();
	return rc;
}

MafReturnT RlistUtil::reset_rplistImmOiHandle()
{
	ENTER_MWSA_REPLIST();
	rplistOiSetFlag = false;
	// obtain IMM handles and set the OI
	if (!rplistOiSetFlag)
	{
		clearAndRefreshIMMHandler();
		MafReturnT rc = comsa_rplist_immOi_set();
		if (MafOk != rc)
		{
			DEBUG_MWSA_REPLIST("reset_rplistImmOiHandle failed. %d", (int)rc);
			LEAVE_MWSA_REPLIST();
			return MafFailure;
		}
		rplistOiSetFlag = true;
	}
	LEAVE_MWSA_REPLIST();
	return MafOk;
}

/* ----------------------------------------------------------------------
 * Startup;
 */

MafReturnT OamSARList::maf_comsa_imm_comSAMwComponentStart()
{
	ENTER_MWSA();
	DEBUG_MWSA("maf_comsa_imm_comSAMwComponentStart called...");
	MafReturnT rc = MafOk;

	// Obtain IMM handles and start the OI for the RP List
	if (!rplistOiSetFlag)
	{
		MafReturnT rc = comsa_rplist_immOi_set();
		if (MafOk != rc)
		{
			ERR_MWSA("maf_comsa_imm_comSAMwComponentStart(): ERROR: comsa_rplist_immOi_set() failed.");
			LEAVE_MWSA();
			return MafFailure;
		}
		DEBUG_MWSA("maf_comsa_imm_comSAMwComponentStart(): comsa_rplist_immOi_set() SUCCESS.");
		rplistOiSetFlag = true;
	}

	#ifndef UNIT_TEST
		asprintf(&strRlistBlockSize, "%s%s", _CC_NAME, RLIST_BLOCK_SIZE);
		asprintf(&strRlistLastId, "%s%s",  _CC_NAME, RLIST_LAST_ID);
		asprintf(&strRlistNumElements, "%s%s", _CC_NAME, RLIST_NUM_ELEMENTS);
	#endif

	LEAVE_MWSA();
	return rc;
}

MafReturnT OamSARList::maf_comsa_imm_comSAMwComponentStop()
{
	ENTER_MWSA();
	DEBUG_MWSA("maf_comsa_imm_comSAMwComponentStop called...");
	MafReturnT rc = MafOk;

	#ifndef UNIT_TEST
		if(strRlistBlockSize)
		{
			free(strRlistBlockSize);
			strRlistBlockSize = NULL;
		}
		if(strRlistLastId)
		{
			free(strRlistLastId);
			strRlistLastId = NULL;
		}
		if(strRlistNumElements)
		{
			free(strRlistNumElements);
			strRlistNumElements = NULL;
		}
	#endif

	// Release the IMM handles and stop the OI for the RP List
	SaAisErrorT err = autoRetry_saImmOiImplementerClear(rplistImmOiHandle);
	if (err != SA_AIS_OK)
	{
		ERR_MWSA("maf_comsa_imm_comSAMwComponentStop(): ERROR: saImmOiImplementerClear() Failed with error : %s", getOpenSAFErrorString(err));
		if (0 == strcmp(_CC_NAME, "lm")) {
			rc = MafOk;
		}
		else {
			rc = MafFailure;
		}
	}

	comsa_rplist_immOi_finalize(&rplistImmOiHandle);
	comsa_rplist_immAccess_finalize(immAccessHandle);
	comsa_rplist_imm_finalize(&rplistImmHandle);
	DEBUG_MWSA("maf_comsa_imm_comSAMwComponentStart(): finalized IMM handles and autoRetry_saImmOiImplementerClear() SUCCESS.");

	LEAVE_MWSA();
	return rc;
}

MafReturnT maf_comsa_imm_comSAMwComponentStart(void)
{
	MafReturnT retval = OamSARList::instance().maf_comsa_imm_comSAMwComponentStart();
	return retval;
}

MafReturnT maf_comsa_imm_comSAMwComponentStop(void)
{
	MafReturnT retval = OamSARList::instance().maf_comsa_imm_comSAMwComponentStop();
	return retval;
}

MafReturnT maf_comsa_imm_listCreate(const MafMwSpiListNameT* listInstanceName, uint32_t dataBufferSize)
{
	MafReturnT retval = OamSARList::instance().listCreate(listInstanceName, dataBufferSize);
	return retval;
}

MafReturnT maf_comsa_imm_listPushBack(const MafMwSpiListNameT* listInstanceName, MafMwSpiListItemNameT* listItemName, /* OUT parameter */ void* newItemDataBuffer)
{
	MafReturnT retval = OamSARList::instance().listPushBack(listInstanceName, listItemName, newItemDataBuffer);
	return retval;
}


MafReturnT maf_comsa_imm_listDelete(const MafMwSpiListNameT* listInstanceName)
{
	MafReturnT retval = OamSARList::instance().listDelete(listInstanceName);
	return retval;
}

MafReturnT maf_comsa_imm_listClear(const MafMwSpiListNameT* listInstanceName)
{
	MafReturnT retval = OamSARList::instance().listClear(listInstanceName);
	return retval;
}

MafReturnT maf_comsa_imm_listGetSize(const MafMwSpiListNameT* listInstanceName, uint32_t* listSize)
{
	MafReturnT retval = OamSARList::instance().listGetSize(listInstanceName, listSize);
	return retval;
}

MafReturnT maf_comsa_imm_listIsEmpty(const MafMwSpiListNameT* listInstanceName, bool* listEmpty)
{
	MafReturnT retval = OamSARList::instance().listIsEmpty(listInstanceName, listEmpty);
	return retval;
}

MafReturnT maf_comsa_imm_listPopBack(const MafMwSpiListNameT* listInstanceName)
{
	MafReturnT retval = OamSARList::instance().listPopBack(listInstanceName);
	return retval;
}

MafReturnT maf_comsa_imm_listEraseItem(const MafMwSpiListNameT* listInstanceName, const MafMwSpiListItemNameT* listItemName)
{
	MafReturnT retval = OamSARList::instance().listEraseItem(listInstanceName, listItemName);
	return retval;
}

MafReturnT maf_comsa_imm_listGetFrontRef(const MafMwSpiListNameT* listInstanceName, MafMwSpiListItemRefT* listInstanceFrontRef)
{
	MafReturnT retval = OamSARList::instance().listGetFrontRef(listInstanceName, listInstanceFrontRef);
	return retval;
}

MafReturnT maf_comsa_imm_listGetFinalize(const MafMwSpiListNameT* listInstanceName, const MafMwSpiListItemRefT currentItemRef)
{
	MafReturnT retval = OamSARList::instance().listGetFinalize(listInstanceName, currentItemRef);
	return retval;
}

MafReturnT maf_comsa_imm_listGetNextItemFront(const MafMwSpiListNameT* listInstanceName, MafMwSpiListItemRefT* currentItemRef, MafMwSpiListItemNameT* listItemName, void* copyOfItemData)
{
	MafReturnT retval = OamSARList::instance().listGetNextItemFront(listInstanceName, currentItemRef, listItemName, copyOfItemData);
	return retval;
}

MafReturnT maf_comsa_imm_listFindItem(const MafMwSpiListNameT* listInstanceName, const MafMwSpiListItemNameT* listItemName, void* copyOfItemData)
{
	MafReturnT retval = OamSARList::instance().listFindItem(listInstanceName, listItemName, copyOfItemData);
	return retval;
}

MafReturnT maf_comsa_imm_listReplaceItem(const MafMwSpiListNameT* listInstanceName, const MafMwSpiListItemNameT* listItemName, const void* replaceItemData)
{
	MafReturnT retval = OamSARList::instance().listReplaceItem(listInstanceName, listItemName, replaceItemData);
	return retval;
}

MafReturnT maf_comsa_imm_listNumberOfListInstances(uint32_t* numberOfLinkListInstances)
{
	MafReturnT retval = OamSARList::instance().listNumberOfListInstances(numberOfLinkListInstances);
	return retval;
}

MafReturnT maf_comsa_imm_listMemoryUsage(uint32_t* memoryUsed, uint32_t* totalMemoryAvailable)
{
	MafReturnT retval = OamSARList::instance().listMemoryUsage(memoryUsed, totalMemoryAvailable);
	return retval;
}
