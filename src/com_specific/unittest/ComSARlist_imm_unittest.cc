/*
 * ComSARlist_imm_unittest.cc
 *
 *  Created on: Feb 26, 2015
 *      Author: xadaleg
 *
 */

// Library includes
#include <list>
#include <map>
#include <string>
#include <iostream>
#include <assert.h>
#include <gtest/gtest.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <poll.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>

// CoreMW includes
#include <saImmOi.h>
// COM includes
#include <MafMwSpiReplicatedList_1.h>
// COMSA includes
#include "ComSA.h"
#include "imm_utils.h"
#include "debug_log.h"
#include "ComSARlist.h"

// To make this output DEBUG printouts to std out. Change gLog level variable to LOG_LEVEL_DEBUG and remove the comment below
#define REDIRECT_LOG
#ifdef REDIRECT_LOG
#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG printf
#else
#include "trace.h"
#endif

// Declare extern variables for testing
extern SaAisErrorT saImmOmClassDescriptionGet_2_rc;
extern SaAisErrorT saImmOiRtObjectCreate_2_rc;
extern SaAisErrorT saImmOmAccessorGet_2_rc;
extern SaAisErrorT saImmOiRtObjectDelete_rc;
extern SaAisErrorT saImmOiRtObjectUpdate_2_rc;
extern SaAisErrorT saImmOmSearchNext_2_rc;
extern SaAisErrorT saImmOmSearchInitialize_2_rc;
extern bool replicateInIMM;

#define CLUSTER_RESTORED 1
#define CLUSTER_REBOOTED_ONLY 2
#define CLUSTER_RT_OBJ_EXIST 3
/* this variable is used to keep the "clear storage location" where the data files for
 * the replicated list are stored
 *
 */
extern char pathToStorageDirectory[MAX_PATH_DATA_LENGTH];
extern char strRpListPath[];
extern SaImmAttrValuesT_2** attrValues_Returned;
extern char strRlistLastId[] ;
extern char strRlistNumElements[] ;
extern char strRlistBlockSize[];
// MOCKED FUNCTIONS
SaAisErrorT saImmOmAdminOwnerInitialize(SaImmHandleT immHandle, const SaImmAdminOwnerNameT adminOwnerName, SaBoolT releaseOwnershipOnFinalize, SaImmAdminOwnerHandleT *ownerHandle)
{
	return SA_AIS_OK;
}

SaAisErrorT saImmOmCcbInitialize(SaImmAdminOwnerHandleT ownerHandle, SaImmCcbFlagsT ccbFlags, SaImmCcbHandleT *ccbHandle)
{
	return SA_AIS_OK;
}

SaAisErrorT saImmOmCcbObjectCreate_2(SaImmCcbHandleT ccbHandle, const SaImmClassNameT className, const SaNameT *parentName, const SaImmAttrValuesT_2 **attrValues)
{
	return SA_AIS_OK;
}

SaAisErrorT saImmOmCcbApply(SaImmCcbHandleT ccbHandle)
{
	return SA_AIS_OK;
}

SaAisErrorT saImmOmCcbFinalize(SaImmCcbHandleT ccbHandle)
{
	return SA_AIS_OK;
}

SaAisErrorT saImmOmAdminOwnerFinalize(SaImmAdminOwnerHandleT ownerHandle)
{
	return SA_AIS_OK;
}

SaAisErrorT saImmOiClassImplementerSetChoice = SA_AIS_OK;
std::string saImmOiClassImplementerSet_receivedRegistration;
SaAisErrorT
saImmOiClassImplementerSet(SaImmOiHandleT immOiHandle, const SaImmClassNameT className) {
	printf("----> saImmOiClassImplementerSet className(%s)\n", className);
	saImmOiClassImplementerSet_receivedRegistration.clear();
	saImmOiClassImplementerSet_receivedRegistration=className;
	return saImmOiClassImplementerSetChoice;
}

std::string recievedData_saImmOiClassImplementerRelease;
SaAisErrorT saImmOiClassImplementerRelease_returnvalue = SA_AIS_OK;
int returnCounter = 5;
SaAisErrorT
saImmOiClassImplementerRelease(SaImmOiHandleT immOiHandle, const SaImmClassNameT className) {
	printf("----> saImmOiClassImplementerRelease \n");
	recievedData_saImmOiClassImplementerRelease = className;
	if (saImmOiClassImplementerRelease_returnvalue == SA_AIS_ERR_TRY_AGAIN) {
		--returnCounter;
		if (returnCounter == 0) saImmOiClassImplementerRelease_returnvalue = SA_AIS_OK;
	}
	return saImmOiClassImplementerRelease_returnvalue;
}

std::string recievedData_saImmOiObjectImplementerSet;
std::list<std::string> recievedData_saImmOiObjectImplementerSetList;
SaAisErrorT saImmOibjectImplementerSet_returnvalue = SA_AIS_OK;
SaAisErrorT
saImmOiObjectImplementerSet(SaImmOiHandleT immOiHandle, const SaNameT *objectName, SaImmScopeT scope) {
	printf("----> saImmOiObjectImplementerSet objectName->value(%s)\n", saNameGet(objectName));
	char tempBuff[saNameMaxLen()];
	strncpy(tempBuff, saNameGet(objectName), saNameLen(objectName)+1);
	tempBuff[saNameLen(objectName)]='\0';
	recievedData_saImmOiObjectImplementerSet = tempBuff;
	recievedData_saImmOiObjectImplementerSetList.push_front(recievedData_saImmOiObjectImplementerSet);
	return saImmOibjectImplementerSet_returnvalue;
}

std::string recievedData_saImmOiObjectImplementerRelease;
SaAisErrorT saImmOiObjectImplementerRelease_returnvalue = SA_AIS_OK;
int returnCounter1 =20;
SaAisErrorT
saImmOiObjectImplementerRelease(SaImmOiHandleT immOiHandle, const SaNameT *objectName, SaImmScopeT scope) {
	printf("----> saImmOiObjectImplementerRelease \n");
	recievedData_saImmOiObjectImplementerRelease = saNameGet(objectName);
	if (saImmOiObjectImplementerRelease_returnvalue == SA_AIS_ERR_TRY_AGAIN) {
		--returnCounter1;
		if (returnCounter1 == 0) saImmOiObjectImplementerRelease_returnvalue = SA_AIS_OK;
	}
	return saImmOiObjectImplementerRelease_returnvalue;
}

SaAisErrorT saImmOmClassDescriptionMemoryFree_2(SaImmHandleT immHandle, SaImmAttrDefinitionT_2 **attrDefinitions) {
	return SA_AIS_OK;
}

SaImmAttrValuesT_2** createAttrValuesBlocksize(uint32_t blockSize = 0) {
	// Block size attribute value
	uint32_t* dataBufferSize = new uint32_t();
	SaImmAttrValueT* attrValueP = new SaImmAttrValueT();
	*dataBufferSize = blockSize;
	*attrValueP = (SaImmAttrValueT)dataBufferSize;

	SaImmAttrValuesT_2* ComsaRlistBlockSizeValue = new SaImmAttrValuesT_2();
	ComsaRlistBlockSizeValue->attrName = strRlistBlockSize;
	ComsaRlistBlockSizeValue->attrValueType = SA_IMM_ATTR_SAUINT32T;
	ComsaRlistBlockSizeValue->attrValuesNumber = 1;
	ComsaRlistBlockSizeValue->attrValues = attrValueP;

	SaImmAttrValuesT_2** attrValues_Returned = new SaImmAttrValuesT_2*[2];
	attrValues_Returned[0] = ComsaRlistBlockSizeValue;
	attrValues_Returned[1] = NULL;
	return attrValues_Returned;
}

void deleteAttrValuesBlocksize() {
	delete attrValues_Returned[0]->attrValues[0];
	delete attrValues_Returned[0]->attrValues;
	delete attrValues_Returned[0];
	delete[] attrValues_Returned;
	attrValues_Returned = NULL;
}

// Declare external functions to be tested

// IMM Functions

extern "C" MafReturnT comsa_rplist_imm_init(SaImmHandleT *immHandle);
extern "C" void comsa_rplist_imm_finalize(SaImmHandleT *immHandle);
extern "C" MafReturnT comsa_rplist_immOi_init(SaImmHandleT *immHandle);
extern "C" void comsa_rplist_immOi_finalize(SaImmHandleT *immHandle);
extern "C"  MafReturnT comsa_rplist_immAccess_init(const SaImmHandleT immHandle, SaImmAccessorHandleT *immAccessorHandle);
extern "C"  void comsa_rplist_immAccess_finalize(SaImmAccessorHandleT immAccessorHandle);

// IMM Class Functions
extern "C" MafReturnT createclass(const char* className, uint32_t dataBufferSize, bool createRunTClass = false);
extern "C" MafReturnT deleteclass(const char* className);
extern "C" MafReturnT searchclass(const char* className);
// IMM Object Functions
extern "C" MafReturnT createObject(const char* className, const SaImmAttrValuesT_2 **attrValues);
extern "C" MafReturnT readObject(const SaNameT* immObject, SaImmAttrValuesT_2 ***attrValues);
extern "C" MafReturnT deleteObject(const SaNameT *objectName);
extern "C" MafReturnT getObjectUiAttribute(const char* className, const char* attrName, unsigned int *attrValue);
extern "C" MafReturnT setObjectUiAttribute(const char* className, const char* attrName, unsigned int attrValue);
extern "C" MafReturnT restore_immData (const MafMwSpiListNameT* listInstanceName);
extern "C" MafReturnT addRlistRTObject(const std::string& pClassName, const unsigned int& lastId, const void* newItemDataBuffer);
extern "C" MafReturnT modifyRlistRTObject(const std::string& pClassName, const std::string& lastId, const void* newItemDataBuffer);
extern "C" MafReturnT deleteRTObject(const std::string& listInstanceName, const std::string& listItemName);
extern "C" MafReturnT getAllRTObjects(const char* className, std::vector<std::string>& immRTObjects);
extern "C" MafReturnT getRTObjectsCount(const char* className , uint32_t& rlistObjCnt);
extern "C" MafReturnT deleteAllRTObjects(const char* className);

// File Functions
extern "C" MafReturnT createDir(const char* fileName, const char* path);
extern "C" MafReturnT writeToFile(const char* fileName, const char* subDir, const void *data, const int dataSize);
extern "C" MafReturnT readFromFile(const char* fileName, const char* subDir, void *data, const int dataSize);
extern "C" MafReturnT findFile(const char* fileName, const char* subDir);
extern "C" MafReturnT removeFile(const char* fileName, const char* subDir);
extern "C" MafReturnT getBaseRepository();
extern "C" MafReturnT comsa_rplist_imm_deleteDir(const char* subDir);

extern "C" MafReturnT maf_comsa_imm_listCreate(const MafMwSpiListNameT* listInstanceName, uint32_t dataBufferSize);
extern "C" MafReturnT maf_comsa_imm_listPushBack(const MafMwSpiListNameT* listInstanceName, MafMwSpiListItemNameT* listItemName, /* OUT parameter */ void* newItemDataBuffer);
extern "C" MafReturnT maf_comsa_imm_listDelete(const MafMwSpiListNameT* listInstanceName);
extern "C" MafReturnT maf_comsa_imm_listClear(const MafMwSpiListNameT* listInstanceName);
extern "C" MafReturnT maf_comsa_imm_listGetSize(const MafMwSpiListNameT* listInstanceName, uint32_t* listSize);
extern "C" MafReturnT maf_comsa_imm_listIsEmpty(const MafMwSpiListNameT* listInstanceName, bool* listEmpty);
extern "C" MafReturnT maf_comsa_imm_listPopBack(const MafMwSpiListNameT* listInstanceName);
extern "C" MafReturnT maf_comsa_imm_listEraseItem(const MafMwSpiListNameT* listInstanceName, const MafMwSpiListItemNameT* listItemName);
extern "C" MafReturnT maf_comsa_imm_listGetFrontRef(const MafMwSpiListNameT* listInstanceName, MafMwSpiListItemRefT* listInstanceFrontRef);
extern "C" MafReturnT maf_comsa_imm_listGetFinalize(const MafMwSpiListNameT* listInstanceName, const MafMwSpiListItemRefT currentItemRef);
extern "C" MafReturnT maf_comsa_imm_listGetNextItemFront(const MafMwSpiListNameT* listInstanceName, MafMwSpiListItemRefT* currentItemRef, MafMwSpiListItemNameT* listItemName, void* copyOfItemData);
extern "C" MafReturnT maf_comsa_imm_listFindItem(const MafMwSpiListNameT* listInstanceName, const MafMwSpiListItemNameT* listItemName, void* copyOfItemData);
extern "C" MafReturnT maf_comsa_imm_listReplaceItem(const MafMwSpiListNameT* listInstanceName, const MafMwSpiListItemNameT* listItemName, const void* replaceItemData);
extern "C" MafReturnT maf_comsa_imm_listNumberOfListInstances(uint32_t* numberOfLinkListInstances);
extern "C" MafReturnT maf_comsa_imm_listMemoryUsage(uint32_t* memoryUsed, uint32_t* totalMemoryAvailable);

/* Utilities */
extern "C" void classToObj(const char* className, SaNameT* rdnName);
extern "C" MafReturnT findNextListItem(const MafMwSpiListNameT* listInstanceName, const int currentItemName, char* nextListItemName, unsigned int* intNextListItemName, SaImmAttrValuesT_2*** attrValues);

extern "C" void comsa_rplist_imm_debugPrintObject(SaImmAttrValuesT_2 **returnedAttrValues);
extern "C" MafReturnT detectClusterState(const MafMwSpiListNameT* listInstanceName, unsigned int* stateFlag);
extern "C" char* createPath(const char** pathArray, const int nNumStrings, const char *fileName);
extern "C" SaImmAttrDefinitionT_2** prepareImmAttrDefinitions();
extern "C" SaImmAttrDefinitionT_2** prepareRTImmAttrDefinitions();

extern "C" void freeImmAttrDefinitions(SaImmAttrDefinitionT_2*** attrDef);
extern "C" MafReturnT convertToRTObjectName(const std::string& listInstanceName, const std::string& lastId, SaNameT** immObject);
extern "C" MafReturnT getVoidData(const SaImmAttrValuesT_2** attrVal, void** data, unsigned int dataLen);

// UNIT TEST

// IMM Unit Testing

extern SaAisErrorT saImmOmInitialize_rc;
TEST (RlistTest_imm, comsa_rplist_imm_init)
{
/* MafReturnT comsa_rplist_imm_init(SaImmHandleT *immHandle); */
	MafReturnT res = MafFailure;
	SaImmHandleT immHandle = 1;
	// Success
	saImmOmInitialize_rc = SA_AIS_OK;
	res = comsa_rplist_imm_init(&immHandle);
	EXPECT_EQ(res, MafOk);
	// Failure
	saImmOmInitialize_rc = SA_AIS_ERR_NOT_EXIST;
	res = comsa_rplist_imm_init(&immHandle);
	EXPECT_EQ(res, MafFailure);
	saImmOmInitialize_rc = SA_AIS_OK;
}

extern SaAisErrorT saImmOmFinalize_rc;
TEST (RlistTest_imm, comsa_rplist_imm_finalize)
{
/*  void comsa_rplist_imm_finalize(SaImmHandleT rplistImmHandle) */
	SaImmHandleT rplistImmHandle = 1;
	// Success
	saImmOmFinalize_rc = SA_AIS_OK;
	comsa_rplist_imm_finalize(&rplistImmHandle);
	EXPECT_EQ(rplistImmHandle, 0);
	// Error
	rplistImmHandle = 1;
	saImmOmFinalize_rc = SA_AIS_ERR_NOT_EXIST;
	comsa_rplist_imm_finalize(&rplistImmHandle);
	EXPECT_EQ(rplistImmHandle, 1);
	saImmOmFinalize_rc = SA_AIS_OK;
}

extern SaAisErrorT saImmOiInitialize_rc;
TEST (RlistTest_imm, comsa_rplist_immOi_init)
{
	MafReturnT res = MafFailure;
	SaImmHandleT immHandle = 0;
	// Success
	saImmOiInitialize_rc = SA_AIS_OK;
	res = comsa_rplist_immOi_init(&immHandle);
	EXPECT_EQ(res, MafOk);
	// Failure
	saImmOiInitialize_rc = SA_AIS_ERR_NOT_EXIST;
	res = comsa_rplist_immOi_init(&immHandle);
	EXPECT_EQ(res, MafFailure);
	saImmOiInitialize_rc = SA_AIS_OK;
}

extern SaAisErrorT saImmOiFinalize_rc;
TEST (RlistTest_imm, comsa_rplist_immOi_finalize)
{
	SaImmHandleT rplistImmHandle = 1;
	// Success
	saImmOiFinalize_rc = SA_AIS_OK;
	comsa_rplist_immOi_finalize(&rplistImmHandle);
	EXPECT_EQ(rplistImmHandle, 0);
	// Error
	rplistImmHandle = 1;
	saImmOiFinalize_rc = SA_AIS_ERR_NOT_EXIST;
	comsa_rplist_immOi_finalize(&rplistImmHandle);
	EXPECT_EQ(rplistImmHandle, 1);
	saImmOiFinalize_rc = SA_AIS_OK;
}

extern SaAisErrorT saImmAccessInitialize_rc;
TEST (RlistTest_imm, comsa_rplist_immAccess_init)
{
	MafReturnT res = MafFailure;
	SaImmHandleT immHandle = 0;
	SaImmAccessorHandleT immAccessHandle = 0;
	// Success
	saImmAccessInitialize_rc = SA_AIS_OK;
	res = comsa_rplist_immAccess_init(immHandle, &immAccessHandle);
	EXPECT_EQ(res, MafOk);
	// Failure
	saImmAccessInitialize_rc = SA_AIS_ERR_NOT_EXIST;
	res = comsa_rplist_immAccess_init(immHandle, &immAccessHandle);
	EXPECT_EQ(res, MafFailure);
	saImmAccessInitialize_rc = SA_AIS_OK;
}

extern SaAisErrorT saImmAccessFinalize_rc;
TEST (RlistTest_imm, comsa_rplist_immAccess_finalize)
{
	SaImmAccessorHandleT immAccessHandle = 0;
	// Success
	saImmAccessFinalize_rc = SA_AIS_OK;
	comsa_rplist_immAccess_finalize(immAccessHandle);
	EXPECT_EQ(immAccessHandle, 0);
	// Error
	immAccessHandle = 1;
	saImmAccessFinalize_rc = SA_AIS_ERR_NOT_EXIST;
	comsa_rplist_immAccess_finalize(immAccessHandle);
	EXPECT_EQ(immAccessHandle, 1);
	saImmOiFinalize_rc = SA_AIS_OK;
}

extern SaAisErrorT saImmOmClassCreate_2_rc;
TEST (RlistTest_imm, createclass)
{
/* MafReturnT createclass(const char* className, uint32_t dataBufferSize) */
	MafReturnT res = MafFailure;
	char className[] = "ReplList";
	// Success
	saImmOmClassDescriptionGet_2_rc = SA_AIS_ERR_NOT_EXIST;
	saImmOmClassCreate_2_rc = SA_AIS_OK;
	res = createclass(className, 100);
	EXPECT_EQ(res, MafOk);
	// Failure
	saImmOmClassDescriptionGet_2_rc = SA_AIS_ERR_BAD_HANDLE;
	saImmOmClassCreate_2_rc = SA_AIS_ERR_BAD_HANDLE;
	res = createclass(className, 100);
	EXPECT_EQ(res,MafFailure);
	saImmOmClassCreate_2_rc = SA_AIS_OK;
	saImmOmClassDescriptionGet_2_rc = SA_AIS_OK;

}

//MafReturnT deleteclass(const char* className);
extern SaAisErrorT saImmOmClassDelete_rc;
TEST (RlistTest_imm, deleteclass)
{
/* MafReturnT deleteclass(SaImmHandleT immHandle, const char* className) */
	MafReturnT res = MafFailure;
	SaImmHandleT immHandle = 1;
	char className[] = "ReplList";
	// Success
	saImmOmClassDelete_rc = SA_AIS_OK;
	res = deleteclass(className);
	EXPECT_EQ(res, MafOk);
	// Error
	saImmOmClassDelete_rc = SA_AIS_ERR_NOT_EXIST;
	res = deleteclass(className);
	EXPECT_EQ(res, MafFailure);
	saImmOmClassDelete_rc = SA_AIS_OK;
}

TEST (RlistTest_imm, searchclass)
{
	MafReturnT res = MafFailure;
	SaImmHandleT immHandle = 1;
	char className[] = "ReplList";
	// Success
	saImmOmClassDescriptionGet_2_rc = SA_AIS_OK;
	res = searchclass(className);
	EXPECT_EQ(res, MafOk);
	// Error
	saImmOmClassDescriptionGet_2_rc = SA_AIS_ERR_NOT_EXIST;
	res = searchclass(className);
	EXPECT_EQ(res, MafNotExist);
	saImmOmClassDescriptionGet_2_rc = SA_AIS_OK;
}

TEST (RlistTest_imm, createObject)
{
	MafReturnT res = MafFailure;
	SaImmHandleT immHandle = 1;
	char objectName[] = "ReplList";
	char strRlistClassRdn[] = "ComsaRlistClassRdn";
	char strRlistBlockSize[] = "comsaRlistBlockSize";
	char strRlistLastId[] = "comsaRlistLastId";
	char strRlistNumElements[] = "comsaRlistNumElements";
	uint32_t dataBufferSize = 1000;
	//=================================================================
	SaNameT rdnName;
	classToObj(objectName, &rdnName);
	SaImmAttrValueT attrValue1 = &rdnName;
	SaImmAttrValuesT_2 ComsaRlistClassRdnValue = {
			strRlistClassRdn,                       // name
			SA_IMM_ATTR_SANAMET,                    // type
			1,                                      // number of values
			(SaImmAttrValueT*) &attrValue1          // pointer to the value
	};

	// Block size attribute value
	SaImmAttrValueT attrValue2 = &dataBufferSize;
	SaImmAttrValuesT_2 comsaRlistBlockSizeValue = {
			strRlistBlockSize,                      // name
			SA_IMM_ATTR_SAUINT32T,                  // type
			1,                                      // number of values
			(SaImmAttrValueT*) &attrValue2          // pointer to the value
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
	//=================================================================

	// Success
	saImmOiRtObjectCreate_2_rc = SA_AIS_OK;
	res = createObject(objectName, attrValuesForObject);
	EXPECT_EQ(res, MafOk);

	// Error
	saImmOiRtObjectCreate_2_rc = SA_AIS_ERR_NOT_EXIST;
	res = createObject(objectName, attrValuesForObject);
	EXPECT_EQ(res, MafFailure);
	saImmOiRtObjectCreate_2_rc = SA_AIS_OK;

}

TEST (RlistTest_imm, readObject)
{
	MafReturnT res = MafOk;
	SaImmHandleT immHandle = 0;
	char objectName[] = "ReplList";
	SaNameT objName;
	classToObj(objectName, &objName);
	// Success
	saImmOmAccessorGet_2_rc = SA_AIS_OK;
	SaImmAttrValuesT_2 **returnedAttrValues = 0;

	res = readObject(&objName, &returnedAttrValues);
	EXPECT_EQ(res, MafOk);

	// Error
	saImmOmAccessorGet_2_rc = SA_AIS_ERR_NOT_EXIST;
	res = readObject(&objName, &returnedAttrValues);
	EXPECT_EQ(res, MafNotExist);
	saImmOmAccessorGet_2_rc = SA_AIS_OK;
}

TEST (RlistTest_imm, deleteObject)
{
	MafReturnT res = MafFailure;
	SaImmHandleT immHandle = 1;
	SaNameT objectName;
	char className[] = "ReplList";
	classToObj(className, &objectName);
	// Success
	saImmOiRtObjectDelete_rc = SA_AIS_OK;
	res = deleteObject(&objectName);
	EXPECT_EQ(res, MafOk);
	// Error
	saImmOiRtObjectDelete_rc = SA_AIS_ERR_NOT_EXIST;
	res = deleteObject(&objectName);
	EXPECT_EQ(res, MafFailure);
	saImmOiRtObjectDelete_rc = SA_AIS_OK;
}

TEST (RlistTest_imm, getObjectUiAttribute)
{
	MafReturnT res = MafFailure;
	SaImmHandleT immHandle = 1;
	SaNameT objectName;
	char className[] = "ReplList";
	unsigned int lastId = 0;
	unsigned int numberElement = 0;
	// ====================================================
	// CREATE the OBJECT for the list
	// ====================================================

	// RDN
	char strRlistClassRdn[] = "ComsaRlistClassRdn";
	unsigned int attrvalue = 0;
	SaNameT rdnName;
	classToObj(className, &rdnName);
	SaImmAttrValueT attrValue1 = &rdnName;
	SaImmAttrValuesT_2 ComsaRlistClassRdnValue = {
		strRlistClassRdn,				// name
		SA_IMM_ATTR_SANAMET,			// type
		1,								// number of values
		(SaImmAttrValueT*) &attrValue1	// pointer to the value
	};

	SaImmAttrValueT attrValue2 = &lastId;
	SaImmAttrValuesT_2 ComsaRlistLastIdValue = {
			strRlistLastId,				// name
			SA_IMM_ATTR_SAUINT32T,			// type
			1,								// number of values
			(SaImmAttrValueT*) &attrValue2	// pointer to the value
	};

	SaImmAttrValueT attrValue3 = &numberElement;
	SaImmAttrValuesT_2 ComsaRlistNumberElementValue = {
			strRlistNumElements,				// name
			SA_IMM_ATTR_SAUINT32T,			// type
			1,								// number of values
			(SaImmAttrValueT*) &attrValue3	// pointer to the value
	};

	// Combine all the attribute values in one NULL terminated array
	attrValues_Returned = new SaImmAttrValuesT_2*[4];
	attrValues_Returned[0] = &ComsaRlistClassRdnValue;
	attrValues_Returned[1] = &ComsaRlistLastIdValue;
	attrValues_Returned[2] = &ComsaRlistNumberElementValue;
	attrValues_Returned[3] = NULL;

	// Create Object with attribute for testing
	createObject(className, (const SaImmAttrValuesT_2**)&attrValues_Returned);

	// test negative case for
	saImmOmAccessorGet_2_rc = SA_AIS_ERR_NOT_EXIST;
	res = getObjectUiAttribute(className, strRlistClassRdn, &attrvalue);
	EXPECT_EQ(res, MafNotExist);

	// test negative case for invalid attrName
	res = getObjectUiAttribute(className, "invalidName", &attrvalue);
	EXPECT_EQ(res, MafNotExist);

	// Success lastId = 2235;
	saImmOmAccessorGet_2_rc = SA_AIS_OK;
	lastId = 2235;
	res = getObjectUiAttribute(className, strRlistLastId, &attrvalue);
	EXPECT_EQ(res, MafOk);
	EXPECT_EQ(attrvalue, lastId);

	// Success numberElement = 149995;
	saImmOmAccessorGet_2_rc = SA_AIS_OK;
	numberElement = 149995;
	res = getObjectUiAttribute(className, strRlistNumElements, &attrvalue);
	EXPECT_EQ(res, MafOk);
	EXPECT_EQ(attrvalue, numberElement);

	delete []attrValues_Returned;
	attrValues_Returned = NULL;
}
TEST (RlistTest_imm, setObjectUiAttribute)
{
	MafReturnT res = MafOk;
	SaImmHandleT immHandle = 1;
	SaNameT objectName;
	unsigned int lastId = 0;
	unsigned int numberElement = 0;
	char className[] = "ReplList";
	// ====================================================
	// CREATE the OBJECT for the list
	// ====================================================

	// RDN
	char strRlistClassRdn[] = "ComsaRlistClassRdn";
	unsigned int attrvalue = 0;
	SaNameT rdnName;
	classToObj(className, &rdnName);
	SaImmAttrValueT attrValue1 = &rdnName;
	SaImmAttrValuesT_2 ComsaRlistClassRdnValue = {
		strRlistClassRdn,				// name
		SA_IMM_ATTR_SANAMET,			// type
		1,								// number of values
		(SaImmAttrValueT*) &attrValue1	// pointer to the value
	};
	SaImmAttrValueT attrValue2 = &lastId;
	SaImmAttrValuesT_2 ComsaRlistLastIdValue = {
			strRlistLastId,				// name
			SA_IMM_ATTR_SAUINT32T,			// type
			1,								// number of values
			(SaImmAttrValueT*) &attrValue2	// pointer to the value
	};

	SaImmAttrValueT attrValue3 = &numberElement;
	SaImmAttrValuesT_2 ComsaRlistNumberElementValue = {
			strRlistNumElements,				// name
			SA_IMM_ATTR_SAUINT32T,			// type
			1,								// number of values
			(SaImmAttrValueT*) &attrValue3	// pointer to the value
	};

	// Combine all the attribute values in one NULL terminated array
	attrValues_Returned = new SaImmAttrValuesT_2*[4];
	attrValues_Returned[0] = &ComsaRlistClassRdnValue;
	attrValues_Returned[1] = &ComsaRlistLastIdValue;
	attrValues_Returned[2] = &ComsaRlistNumberElementValue;
	attrValues_Returned[3] = NULL;

	// Create Object with attribute for testing
	createObject(className, (const SaImmAttrValuesT_2**) attrValues_Returned);
	// Error
	saImmOiRtObjectUpdate_2_rc = SA_AIS_ERR_NOT_EXIST;
	res = setObjectUiAttribute(className, strRlistClassRdn, 0);
	EXPECT_EQ(res, MafFailure);

	// Success update lastid
	saImmOiRtObjectUpdate_2_rc = SA_AIS_OK;
	res = setObjectUiAttribute(className, strRlistLastId, 34567);
	EXPECT_EQ(res, MafOk);
	EXPECT_EQ(lastId, 34567);

	// Success update numberElement
	saImmOiRtObjectUpdate_2_rc = SA_AIS_OK;
	res = setObjectUiAttribute(className, strRlistNumElements, 2267);
	EXPECT_EQ(res, MafOk);
	EXPECT_EQ(numberElement, 2267);

	delete []attrValues_Returned;

	attrValues_Returned = NULL;
}

TEST (RlistTest_imm, restore_immData)
{
	MafReturnT res = MafOk;
	char subDir[] = "FmActiveAlarmList";
	char fileName1[] = "1";
	char fileName2[] = "21";
	char data[] = "alarmData";
	rlist_maxsize = 200;
	clearAlarmsOnClusterReboot=true;

	MafMwSpiListNameT listInstanceName;
	strncpy((char*)listInstanceName.value, subDir, strlen(subDir)+1);
	listInstanceName.length = strlen(subDir);

	//No alarms case
	saImmOmClassDescriptionGet_2_rc = SA_AIS_ERR_NOT_EXIST;
	res = maf_comsa_imm_listCreate(&listInstanceName, 100);
	EXPECT_EQ(res, MafOk);

	res = restore_immData(&listInstanceName);
	EXPECT_EQ(res, MafOk);

	//More than one alarm case
	res = writeToFile(fileName1, subDir, data , sizeof(data));
	EXPECT_EQ(res, MafOk);

	res = writeToFile(fileName2, subDir, data , sizeof(data));
	EXPECT_EQ(res, MafOk);

	res = restore_immData(&listInstanceName);
	EXPECT_EQ(res, MafAlreadyExist);

	//Tear down
	removeFile(fileName1, subDir);
	removeFile(fileName2, subDir);
	res = maf_comsa_imm_listDelete(&listInstanceName);
	EXPECT_EQ(res, MafOk);
}

TEST (RlistTest_imm, getBaseRepository)
{

	MafReturnT res = MafFailure;
	char pathUT[MAX_PATH_DATA_LENGTH] = "\0";
	char tempPath[MAX_PATH_DATA_LENGTH];
	const char command[] = "pwd";

	FILE *dirPath = NULL;

	dirPath = popen(command, "r");
	fgets(tempPath, MAX_PATH_DATA_LENGTH, dirPath);
	pclose(dirPath);

	int i = 0;
	while(i < strlen(tempPath) - 1)
	{
			pathUT[i] = tempPath[i];
			i++;
	}

	res = getBaseRepository();
	if(0 == strcmp(pathToStorageDirectory, pathUT))
	{
			EXPECT_EQ(res, MafOk);
	}
	else
	{
			EXPECT_EQ(res, MafFailure);
	}

}

TEST (RlistTest_imm, createDir)
{
	MafReturnT res = MafOk;
	char * subDir = "rplist";
	res = createDir(subDir, "/");
	EXPECT_EQ(res, MafOk);
}

TEST (RlistTest_imm, writeToFile)
{
	MafReturnT res = MafOk;
	// fileName is NULL
	char *fName = NULL;
	unsigned int size = 8;
	char *data = "testing";

	fName = "unittest";
	res = writeToFile((const char*)fName, (const char*)"", (const void*)data, (const int)size);
	EXPECT_EQ(res, MafOk);
}

TEST (RlistTest_imm, readFromFile)
{
	MafReturnT res = MafOk;
	char *fName = "unittest";
	long unsigned int size = 8;
	char *dir = "";
	void *datar = malloc(size);
	res = readFromFile((const char*)fName, (const char*)dir, datar, (const int)size);
	free(datar);
	EXPECT_EQ(res, MafOk);
}

TEST (RlistTest_imm, findFile)
{
	char invalidFileName[] = "invalidFileName";
	char validFileName[] = "valideFileName";
	char invalidSubdir[] = "invalidSubdir";
	char validSubdir[] = "validSubdir";
	char data[] = "testData";
	int dataSize = strlen(data);
	rlist_maxsize = 200;
	clearAlarmsOnClusterReboot=true;
	MafReturnT res = MafOk;

	MafMwSpiListNameT listInstanceName;
	strncpy((char*)listInstanceName.value, validSubdir, strlen(validSubdir)+1);
	listInstanceName.length = strlen(validSubdir);

	//setup file
	saImmOmClassDescriptionGet_2_rc = SA_AIS_ERR_NOT_EXIST;
	res = maf_comsa_imm_listCreate(&listInstanceName, 100);
	EXPECT_EQ(res, MafOk);
	res = writeToFile(validFileName, validSubdir, data , sizeof(data));
	EXPECT_EQ(res, MafOk);

	//NULL fileName;
	res = findFile(NULL, validSubdir);
	EXPECT_EQ(res, MafFailure);
	//NULL subDir
	res = findFile(validFileName, NULL);
	EXPECT_EQ(res, MafNotExist);
	//Invalid fileName
	res = findFile(invalidFileName, validSubdir);
	EXPECT_EQ(res, MafNotExist);
	//Invalid subdir
	res = findFile(validFileName, invalidSubdir);
	EXPECT_EQ(res, MafNotExist);
	//Normal case
	res = findFile(validFileName, validSubdir);
	EXPECT_EQ(res, MafOk);
	//tearDown
	removeFile(validFileName, validSubdir);
	comsa_rplist_imm_deleteDir(validSubdir);

}

TEST (RlistTest_imm, removeFile)
{
	MafReturnT res = MafOk;
	char *fName = "unittest";
	char *dir = "";
	res = removeFile((const char*)fName, (const char*)dir);
	EXPECT_EQ(res, MafOk);
}

TEST (RlistTest_imm, detectClusterState)
{
	MafReturnT res = MafFailure;
	MafMwSpiListNameT listInstanceName;
	strncpy((char*)listInstanceName.value, "ReplList", 9);
	listInstanceName.value[8] = '\0';
	listInstanceName.length = 8;
	unsigned int stateFlag = 0;
	struct stat st;
	int return_c = 0;

	res = getBaseRepository();
	EXPECT_EQ(res, MafOk);

	// Cluster is restored
	res = detectClusterState(&listInstanceName, &stateFlag);
	EXPECT_EQ(res, MafOk);
	EXPECT_EQ(stateFlag, 1);

	res = createDir("rplist", "/");
	EXPECT_EQ(res, MafOk);

	res = createDir("ReplList", strRpListPath);
	EXPECT_EQ(res, MafOk);

	//Cluster is rebooted only
	saImmOmAccessorGet_2_rc = SA_AIS_ERR_NOT_EXIST;
	res = detectClusterState(&listInstanceName, &stateFlag);
	EXPECT_EQ(res, MafNotExist);
	EXPECT_EQ(stateFlag, 2);

	saImmOmAccessorGet_2_rc = SA_AIS_OK;
	res = detectClusterState(&listInstanceName, &stateFlag);
	EXPECT_EQ(res, MafOk);
	EXPECT_EQ(stateFlag, 3);

	//Tear down
	const char *pathArray[4];
	pathArray[0] = pathToStorageDirectory;
	pathArray[1] = COMSA_FOR_COREMW_DIR;
	pathArray[2] = strRpListPath;
	pathArray[3] = "ReplList";
	char* dir = createPath(pathArray, 4, NULL);
	if(0 == stat(dir, &st))
	{
		DEBUG_MWSA_REPLIST("detectClusterState():dir [%s] exists. Deleting", dir);
		return_c = rmdir(dir);
		EXPECT_EQ(return_c, 0);
	}
	else
	{
		DEBUG_MWSA_REPLIST("detectClusterState():dir [%s] doesn't exists", dir);
	}
	if(NULL != dir)
		free(dir);
}

TEST (RlistTest_imm, prepareImmAttrDefinitions_true)
{
	SaImmAttrDefinitionT_2** attrDef = prepareRTImmAttrDefinitions();

	int i = 0;
	for(; attrDef[i] != NULL; i++)
	{
		if(0 == strcmp(attrDef[i]->attrName, "saRlistRTdn"))
		{
			DEBUG_MWSA_REPLIST("prepareImmAttrDefinitions(): [%s]", "saRlistRTdn");
			EXPECT_EQ(attrDef[i]->attrValueType, SA_IMM_ATTR_SANAMET);
			EXPECT_EQ(attrDef[i]->attrFlags, SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_RDN | SA_IMM_ATTR_CACHED);
			EXPECT_EQ(true, attrDef[i]->attrDefaultValue == NULL);
		}
		if(0 == strcmp(attrDef[i]->attrName, "data"))
		{
			DEBUG_MWSA_REPLIST("prepareImmAttrDefinitions(): [%s]", "data");
			EXPECT_EQ(attrDef[i]->attrValueType, SA_IMM_ATTR_SAANYT);
			EXPECT_EQ(attrDef[i]->attrFlags, SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_CACHED);
			EXPECT_EQ(true, attrDef[i]->attrDefaultValue == NULL);
		}
	}
	DEBUG_MWSA_REPLIST("prepareImmAttrDefinitions(): [%i]", i);

	EXPECT_EQ(i, 2);

	freeImmAttrDefinitions(&attrDef);
	EXPECT_EQ(true, attrDef == NULL);
}

TEST (RlistTest_imm, prepareImmAttrDefinitions_false)
{
	SaImmAttrDefinitionT_2** attrDef = prepareImmAttrDefinitions();
	int i = 0;
	for(; attrDef[i] != NULL; i++)
	{
		if(0 == strcmp(attrDef[i]->attrName, "ComsaRlistClassRdn"))
		{
			DEBUG_MWSA_REPLIST("prepareImmAttrDefinitions(): [%s]", "ComsaRlistClassRdn");
			EXPECT_EQ(attrDef[i]->attrValueType, SA_IMM_ATTR_SANAMET);
			EXPECT_EQ(attrDef[i]->attrFlags, SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_RDN | SA_IMM_ATTR_CACHED);
			EXPECT_EQ(true, attrDef[i]->attrDefaultValue == NULL);
		}
		else if(0 == strcmp(attrDef[i]->attrName, "comsaRlistBlockSize"))
		{
			DEBUG_MWSA_REPLIST("prepareImmAttrDefinitions(): [%s]", "comsaRlistBlockSize");
			EXPECT_EQ(attrDef[i]->attrValueType, SA_IMM_ATTR_SAUINT32T);
			EXPECT_EQ(attrDef[i]->attrFlags, SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_CACHED);
			EXPECT_EQ(true, attrDef[i]->attrDefaultValue == NULL);
		}
		else if(0 == strcmp(attrDef[i]->attrName, "comsaRlistLastId"))
		{
			DEBUG_MWSA_REPLIST("prepareImmAttrDefinitions(): [%s]", "comsaRlistLastId");
			EXPECT_EQ(attrDef[i]->attrValueType, SA_IMM_ATTR_SAUINT32T);
			EXPECT_EQ(attrDef[i]->attrFlags, SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_CACHED);
			EXPECT_EQ(true, attrDef[i]->attrDefaultValue == NULL);
		}
		else if(0 == strcmp(attrDef[i]->attrName, "comsaRlistNumElements"))
		{
			DEBUG_MWSA_REPLIST("prepareImmAttrDefinitions(): [%s]", "comsaRlistNumElements");
			EXPECT_EQ(attrDef[i]->attrValueType, SA_IMM_ATTR_SAUINT32T);
			EXPECT_EQ(attrDef[i]->attrFlags, SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_CACHED);
			EXPECT_EQ(true, attrDef[i]->attrDefaultValue == NULL);
		}
	}
	EXPECT_EQ(i, 4);

	freeImmAttrDefinitions(&attrDef);
	EXPECT_EQ(true, attrDef == NULL);
}

extern unsigned long long rlist_maxsize;
TEST (RlistTest_imm, maf_comsa_imm_listCreate)
{
// MafReturnT maf_comsa_imm_listCreate(const MafMwSpiListNameT* listInstanceName, uint32_t dataBufferSize);
	MafReturnT res = MafFailure;
	MafMwSpiListNameT listInstanceName;
	strncpy((char*)listInstanceName.value, "ReplList", 9);
	listInstanceName.value[8] = '\0';
	listInstanceName.length = 8;
	rlist_maxsize = 200;
	clearAlarmsOnClusterReboot=true;

	// Normal case
	saImmOmClassDescriptionGet_2_rc = SA_AIS_ERR_NOT_EXIST;
	res = maf_comsa_imm_listCreate(&listInstanceName, 100);
	EXPECT_EQ(res, MafOk);
	saImmOmClassDescriptionGet_2_rc = SA_AIS_OK;

	// Invalid listInstanceName
	res = maf_comsa_imm_listCreate(NULL, 100);
	EXPECT_EQ(res, MafNoResources);

	// dataBufferSize > rlist_maxsize
	res = maf_comsa_imm_listCreate(&listInstanceName, rlist_maxsize+1);
	EXPECT_EQ(res, MafNoResources);

	res = maf_comsa_imm_listDelete(&listInstanceName);
	EXPECT_EQ(res, MafOk);

	//clearAlarmsOnClusterReboot is set to "false"
	clearAlarmsOnClusterReboot=false;
	//No alarms case
	saImmOmClassDescriptionGet_2_rc = SA_AIS_ERR_NOT_EXIST;
	res = maf_comsa_imm_listCreate(&listInstanceName, 100);
	EXPECT_EQ(res, MafOk);

	res = maf_comsa_imm_listDelete(&listInstanceName);
	EXPECT_EQ(res, MafOk);

	//More than one alarm case
	char fileName1[] = "1";
	char fileName2[] = "21";
	char data[] = "alarmData";
	char subDir[] = "ReplList";

	res = createDir(subDir, strRpListPath);
	EXPECT_EQ(res, MafOk);

	res = writeToFile(fileName1, subDir, data , sizeof(data));
	EXPECT_EQ(res, MafOk);

	res = writeToFile(fileName2, subDir, data , sizeof(data));
	EXPECT_EQ(res, MafOk);

	saImmOmClassDescriptionGet_2_rc = SA_AIS_ERR_NOT_EXIST;
	res = maf_comsa_imm_listCreate(&listInstanceName, 100);
	EXPECT_EQ(res, MafAlreadyExist);

	// Teardown
	removeFile(fileName1, subDir);
	removeFile(fileName2, subDir);
	res = maf_comsa_imm_listDelete(&listInstanceName);
	EXPECT_EQ(res, MafOk);
}

TEST (RlistTest_imm, maf_comsa_imm_listPushBack)
{
//MafReturnT maf_comsa_imm_listPushBack(const MafMwSpiListNameT* listInstanceName, MafMwSpiListItemNameT* listItemName, /* OUT parameter */ void* newItemDataBuffer);
	MafReturnT res = MafFailure;
	MafMwSpiListItemNameT listItemName;
	void* invalidDataBuffer = NULL;
	char validDataBuffer[] = "UTTesting";

	/* Rainy case: invalid listInstanceName */
	MafMwSpiListNameT invalidListInstanceName;
	strncpy((char*)invalidListInstanceName.value, "\0", 0);
	invalidListInstanceName.length = 0;
	res = maf_comsa_imm_listPushBack(&invalidListInstanceName, &listItemName, invalidDataBuffer);
	EXPECT_EQ(res, MafNotExist);

	/* Rainy case: newItemDataBuffer is NULL */
	MafMwSpiListNameT listInstanceName;
	strncpy((char*)listInstanceName.value, "ReplList", 9);
	listInstanceName.length = 8;
	res = maf_comsa_imm_listPushBack(&listInstanceName, &listItemName, invalidDataBuffer);
	EXPECT_EQ(res, MafNotExist);

	/* Sunny case: valid listInstanceName and not NULL newItemDataBuffer */
	// Create list
	saImmOmClassDescriptionGet_2_rc = SA_AIS_ERR_NOT_EXIST;
	res = maf_comsa_imm_listCreate(&listInstanceName, 100);
	EXPECT_EQ(res, MafOk);
	saImmOmClassDescriptionGet_2_rc = SA_AIS_OK;

	attrValues_Returned = createAttrValuesBlocksize(sizeof(validDataBuffer));
	res = maf_comsa_imm_listPushBack(&listInstanceName, &listItemName, (void*) validDataBuffer);
	EXPECT_EQ(res, MafOk);

	// Teardown
	res = maf_comsa_imm_listDelete(&listInstanceName);
	EXPECT_EQ(res, MafOk);

	deleteAttrValuesBlocksize();
}

TEST (RlistTest_imm, maf_comsa_imm_listDelete)
{
//MafReturnT maf_comsa_imm_listDelete(const MafMwSpiListNameT* listInstanceName);
	MafReturnT res = MafFailure;
	MafMwSpiListNameT listInstanceName;
	strncpy((char*)listInstanceName.value, "ReplList", 9);
	listInstanceName.length = 8;

	// Create list
	saImmOmClassDescriptionGet_2_rc = SA_AIS_ERR_NOT_EXIST;
	res = maf_comsa_imm_listCreate(&listInstanceName, 100);
	EXPECT_EQ(res, MafOk);
	saImmOmClassDescriptionGet_2_rc = SA_AIS_OK;

	// Rainy case: invalid argument
	res = maf_comsa_imm_listDelete(NULL);
	EXPECT_EQ(res, MafNotExist);

	// Normal case
	saImmOiRtObjectDelete_rc = SA_AIS_OK;
	saImmOmClassDelete_rc = SA_AIS_OK;
	res = maf_comsa_imm_listDelete(&listInstanceName);
	EXPECT_EQ(res, MafOk);
}

TEST (RlistTest_imm, maf_comsa_imm_listClear)
{
	MafReturnT res = MafFailure;
	MafMwSpiListNameT listInstanceName;
	strncpy((char*)listInstanceName.value, "ReplList", 9);
	listInstanceName.length = 8;

	// Normal case
	saImmOmClassDescriptionGet_2_rc = SA_AIS_ERR_NOT_EXIST;
	res = maf_comsa_imm_listCreate(&listInstanceName, 100);
	EXPECT_EQ(res, MafOk);

	// Create Imm class for testing
	char className[] = "ReplList";
	char strRlistNumElements[] = "comsaRlistNumElements";
	SaImmHandleT immHandle = 1;
	// this is the current number of elements in the list
	SaImmAttrDefinitionT_2 comsaRlistNumElements = {
				strRlistNumElements,
				SA_IMM_ATTR_SAUINT32T,
				SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_CACHED,
				NULL
	};
	// this is the NULL terminated array of attributes for the Replicated List IMM class
	const SaImmAttrDefinitionT_2* attrDef[] = {&comsaRlistNumElements,
													   NULL};
	DEBUG_MWSA_REPLIST("createclass(): calling autoRetry_saImmOmClassCreate_2 with class name %s", className);
	autoRetry_saImmOmClassCreate_2(immHandle,(const SaImmClassNameT) className,(SaImmClassCategoryT) SA_IMM_CLASS_RUNTIME, attrDef);
	// Success, create file in /ReplList/ so the removeFile() shall work
	char *fName = "unittest";
	unsigned int size = 8;
	char *data = "testing";

	unsigned int numberElement = 1;
	SaImmAttrValueT attrValue = &numberElement;
	SaImmAttrValuesT_2 ComsaRlistNumberElementValue = {
			strRlistNumElements,				// name
			SA_IMM_ATTR_SAUINT32T,			// type
			1,								// number of values
			(SaImmAttrValueT*) &attrValue	// pointer to the value
	};
	attrValues_Returned = new SaImmAttrValuesT_2*[2];
	attrValues_Returned[0] = &ComsaRlistNumberElementValue;
	attrValues_Returned[1] = NULL;

	char *dir = "ReplList";
	writeToFile((const char*)fName, (const char*)dir, (const void*)data, (const int)size);
	res = maf_comsa_imm_listClear(&listInstanceName);
	EXPECT_EQ(res, MafOk);

	delete[] attrValues_Returned;
	attrValues_Returned = NULL;
}

TEST (RlistTest_imm, listClear_replicateInIMM)
{
	MafReturnT res = MafFailure;
	MafMwSpiListNameT listInstanceName;
	MafMwSpiListItemNameT listItemName;
	strncpy((char*)listInstanceName.value, "ReplList", 9);
	listInstanceName.length = 8;
	replicateInIMM = true;

	// Normal case
        saImmOmSearchNext_2_rc = SA_AIS_ERR_NOT_EXIST;
	saImmOmAccessorGet_2_rc = SA_AIS_ERR_NOT_EXIST;
	res = maf_comsa_imm_listCreate(&listInstanceName, 100);
	EXPECT_EQ(res, MafOk);
	saImmOmAccessorGet_2_rc = SA_AIS_OK;

	//TODO: Create RT objects and verify the existence after listClear
        char newDataBuffer[] = "UTTesting1";
        attrValues_Returned = createAttrValuesBlocksize(sizeof(newDataBuffer));
        res = maf_comsa_imm_listPushBack(&listInstanceName, &listItemName, (void*) newDataBuffer);
        EXPECT_EQ(res, MafOk);

	res = maf_comsa_imm_listClear(&listInstanceName);
	EXPECT_EQ(res, MafOk);

	replicateInIMM = false;
        saImmOmSearchNext_2_rc = SA_AIS_OK;
	delete[] attrValues_Returned;
	attrValues_Returned = NULL;
}

TEST (RlistTest_imm, maf_comsa_imm_listGetSize)
{
// MafReturnT maf_comsa_imm_listGetSize(const MafMwSpiListNameT* listInstanceName, uint32_t* listSize);
	MafReturnT res = MafFailure;
	MafMwSpiListNameT listInstanceName;
	strncpy((char*)listInstanceName.value, "ReplList", 9);
	listInstanceName.length = 8;
	uint32_t listSize = 0;
	unsigned int numOfElement = 0;

	// Create list
	saImmOmClassDescriptionGet_2_rc = SA_AIS_ERR_NOT_EXIST;
	res = maf_comsa_imm_listCreate(&listInstanceName, 100);
	EXPECT_EQ(res, MafAlreadyExist);
	saImmOmClassDescriptionGet_2_rc = SA_AIS_OK;

	// Rainy case: invalid argument
	res = maf_comsa_imm_listGetSize(NULL, &listSize);
	EXPECT_EQ(res, MafNotExist);

	saImmOmAccessorGet_2_rc = SA_AIS_OK;
	res = maf_comsa_imm_listGetSize(&listInstanceName, &listSize);
	EXPECT_EQ(res, MafOk);
	EXPECT_EQ(listSize, 0);

	//Tear down
	res = maf_comsa_imm_listDelete(&listInstanceName);
	EXPECT_EQ(res, MafOk);
}

TEST (RlistTest_imm, maf_comsa_imm_listIsEmpty)
{
	MafReturnT res = MafOk;
	bool listEmpty = -1;
	SaImmHandleT immHandle = 1;
	SaNameT objectName;
	unsigned int lastId = 0;
	unsigned int numberElement = 0;
	char className[] = "isEmptyReplList";
	MafMwSpiListNameT listInstanceName;

	strncpy((char*)listInstanceName.value, "isEmptyReplList", 16);
	listInstanceName.length = 15;

	// ====================================================
	// CREATE the OBJECT for the list
	// ====================================================

	// RDN
	char strRlistClassRdn[] = "ComsaRlistClassRdn";
	unsigned int attrvalue = 0;
	SaNameT rdnName;
	classToObj(className, &rdnName);
	SaImmAttrValueT attrValue1 = &rdnName;
	SaImmAttrValuesT_2 ComsaRlistClassRdnValue = {
		strRlistClassRdn,				// name
		SA_IMM_ATTR_SANAMET,			// type
		1,								// number of values
		(SaImmAttrValueT*) &attrValue1	// pointer to the value
	};
	SaImmAttrValueT attrValue2 = &lastId;
	SaImmAttrValuesT_2 ComsaRlistLastIdValue = {
			strRlistLastId,				// name
			SA_IMM_ATTR_SAUINT32T,			// type
			1,								// number of values
			(SaImmAttrValueT*) &attrValue2	// pointer to the value
	};

	SaImmAttrValueT attrValue3 = &numberElement;
	SaImmAttrValuesT_2 ComsaRlistNumberElementValue = {
			strRlistNumElements,				// name
			SA_IMM_ATTR_SAUINT32T,			// type
			1,								// number of values
			(SaImmAttrValueT*) &attrValue3	// pointer to the value
	};

	// Combine all the attribute values in one NULL terminated array
	attrValues_Returned = new SaImmAttrValuesT_2*[4];
	attrValues_Returned[0] = &ComsaRlistClassRdnValue;
	attrValues_Returned[1] = &ComsaRlistLastIdValue;
	attrValues_Returned[2] = &ComsaRlistNumberElementValue;
	attrValues_Returned[3] = NULL;
	// Invalid list name
	saImmOmAccessorGet_2_rc = SA_AIS_ERR_NOT_EXIST;
	res = maf_comsa_imm_listIsEmpty(NULL, &listEmpty);
	EXPECT_EQ(res, MafNotExist);
	EXPECT_EQ(listEmpty, true);

	// listEmpty == NULL
	saImmOmAccessorGet_2_rc = SA_AIS_ERR_NOT_EXIST;
	res = maf_comsa_imm_listIsEmpty(&listInstanceName, NULL);
	EXPECT_EQ(res, MafNotExist);
	EXPECT_EQ(listEmpty, true);

	// Normal Check listEmpty = false
	numberElement = 0;
	saImmOmAccessorGet_2_rc = SA_AIS_OK;
	res = maf_comsa_imm_listIsEmpty(&listInstanceName, &listEmpty);
	EXPECT_EQ(res, MafOk);
	EXPECT_EQ(listEmpty, true);

	// Normal Check listEmpty = true
	numberElement = 2;
	res = maf_comsa_imm_listIsEmpty(&listInstanceName, &listEmpty);
	EXPECT_EQ(res, MafOk);
	EXPECT_EQ(listEmpty, false);

	delete []attrValues_Returned;
	attrValues_Returned = NULL;
}

TEST (RlistTest_imm, maf_comsa_imm_listPopBack)
{
//MafReturnT maf_comsa_imm_listPopBack(const MafMwSpiListNameT* listInstanceName);
	MafReturnT res = MafFailure;
	MafMwSpiListNameT listInstanceName;

	/* Rainy case: invalid listInstanceName */
	strncpy((char*)listInstanceName.value, "\0", 0);
	listInstanceName.length = 0;
	res = maf_comsa_imm_listPopBack(&listInstanceName);
	EXPECT_EQ(res, MafNotExist);

	/* sunny case: valid listInstanceName */
	strncpy((char*)listInstanceName.value, "ReplList", 9);
	listInstanceName.length = 8;
	char newDataBuffer[] = "UTTesting";
	MafMwSpiListItemNameT listItemName;

	res = maf_comsa_imm_listCreate(&listInstanceName, 100);
	EXPECT_EQ(res, MafOk);

	attrValues_Returned = createAttrValuesBlocksize(sizeof(newDataBuffer));

	res = maf_comsa_imm_listPushBack(&listInstanceName, &listItemName, (void*) newDataBuffer);
	EXPECT_EQ(res, MafOk);

	res = maf_comsa_imm_listPopBack(&listInstanceName);
	EXPECT_EQ(res, MafOk);

	res = maf_comsa_imm_listDelete(&listInstanceName);
	EXPECT_EQ(res, MafOk);

	deleteAttrValuesBlocksize();
}

TEST (RlistTest_imm, maf_comsa_imm_listEraseItem)
{
//MafReturnT maf_comsa_imm_listEraseItem(const MafMwSpiListNameT* listInstanceName, const MafMwSpiListItemNameT* listItemName);
	MafReturnT res = MafFailure;
	MafMwSpiListNameT listName;
	MafMwSpiListItemNameT listItemName;
	MafMwSpiListItemNameT invalidItemName;
	MafMwSpiListItemNameT itemName;
	MafMwSpiListItemNameT notExistItemName;

	strncpy((char*)itemName.value, "1", 2);
	itemName.length = 1;

	strncpy((char*)notExistItemName.value, "5", 2);
	notExistItemName.length = 1;

	strncpy((char*)invalidItemName.value, "\0", 0);
	invalidItemName.length = 0;

	/* Rainy case: invalid listInstanceName */
	strncpy((char*)listName.value, "\0", 0);
	listName.length = 0;
	res = maf_comsa_imm_listEraseItem(&listName, &itemName);
	EXPECT_EQ(res, MafNotExist);

	/* Rainy case: invalid invalidItemName */
	strncpy((char*)listName.value, "ReplList", 9);
	listName.length = 8;

	res = maf_comsa_imm_listCreate(&listName, 100);
	EXPECT_EQ(res, MafOk);

	res = maf_comsa_imm_listEraseItem(&listName, &invalidItemName);
	EXPECT_EQ(res, MafNotExist);

	/* Rainy case: valid parameters but not exist in file system */
	res = maf_comsa_imm_listEraseItem(&listName, &notExistItemName);
	EXPECT_EQ(res, MafNotExist);

	/* Sunny case: valid parameters */
	res = maf_comsa_imm_listCreate(&listName, 100);
	EXPECT_EQ(res, MafAlreadyExist);

	char newDataBuffer[] = "UTTesting";
	attrValues_Returned = createAttrValuesBlocksize(sizeof(newDataBuffer));
	res = maf_comsa_imm_listPushBack(&listName, &listItemName, (void*) newDataBuffer);
	EXPECT_EQ(res, MafOk);

	res = maf_comsa_imm_listEraseItem(&listName, &itemName);
	EXPECT_EQ(res, MafOk);

	res = maf_comsa_imm_listDelete(&listName);
	EXPECT_EQ(res, MafOk);

	deleteAttrValuesBlocksize();
}

TEST (RlistTest_imm, listEraseItem_replicateInIMM)
{
//MafReturnT maf_comsa_imm_listEraseItem(const MafMwSpiListNameT* listInstanceName, const MafMwSpiListItemNameT* listItemName);
        MafReturnT res = MafFailure;
        MafMwSpiListNameT listName;
        MafMwSpiListItemNameT listItemName;
        MafMwSpiListItemNameT invalidItemName;
        MafMwSpiListItemNameT itemName;
        MafMwSpiListItemNameT notExistItemName;

        strncpy((char*)itemName.value, "1", 2);
        itemName.length = 1;

        strncpy((char*)listName.value, "ReplList", 9);
        listName.length = 8;
        replicateInIMM = true;
        saImmOmSearchNext_2_rc = SA_AIS_ERR_NOT_EXIST;

        /* Sunny case: valid parameters */
	saImmOmAccessorGet_2_rc = SA_AIS_ERR_NOT_EXIST;
        res = maf_comsa_imm_listCreate(&listName, 100);
        EXPECT_EQ(res, MafOk);
	saImmOmAccessorGet_2_rc = SA_AIS_OK;

        char newDataBuffer[] = "UTTesting1";
        attrValues_Returned = createAttrValuesBlocksize(sizeof(newDataBuffer));
        res = maf_comsa_imm_listPushBack(&listName, &listItemName, (void*) newDataBuffer);
        EXPECT_EQ(res, MafOk);

        res = maf_comsa_imm_listEraseItem(&listName, &itemName);
        EXPECT_EQ(res, MafOk);
        res = maf_comsa_imm_listDelete(&listName);
        EXPECT_EQ(res, MafOk);
        replicateInIMM = false;
        saImmOmSearchNext_2_rc = SA_AIS_OK;
        deleteAttrValuesBlocksize();

}

extern SaImmAttrValuesT_2** attrValues_Returned;
MafMwSpiListItemRefT listInstanceFrontRef_UT ;
TEST (RlistTest_imm, maf_comsa_imm_listGetFrontRef)
{
	MafReturnT res = MafFailure;
	MafMwSpiListNameT* listInstanceName = NULL;
	const char * listname = "ReplListFront";

	listInstanceName =  new MafMwSpiListNameT;
	strncpy((char *)&listInstanceName->value, listname, strlen(listname) +1);
	listInstanceName->length = strlen(listname);


	//case NULL
	res = maf_comsa_imm_listGetFrontRef(listInstanceName, NULL);
	EXPECT_EQ(res, MafNotExist);

	//case not exist
	saImmOmAccessorGet_2_rc = SA_AIS_ERR_NOT_EXIST;
	res = maf_comsa_imm_listGetFrontRef(listInstanceName, &listInstanceFrontRef_UT);
	EXPECT_EQ(res, MafNotExist);

	//case list EMPTY
	saImmOmAccessorGet_2_rc = SA_AIS_OK;
	res = maf_comsa_imm_listGetFrontRef(listInstanceName, &listInstanceFrontRef_UT);
	EXPECT_EQ(res, MafOk);

	//normal case
	char className[] = "ReplListFront";
	char strRlistNumElements[] = "comsaRlistNumElements";
	SaNameT objectName;
	unsigned int attrvalue = 0;

	//Create
	saImmOmClassDescriptionGet_2_rc = SA_AIS_ERR_NOT_EXIST;
	res = maf_comsa_imm_listCreate(listInstanceName, 100);
	EXPECT_EQ(res, MafOk);


	classToObj(className, &objectName);
	SaImmAttrValueT attrValue1 = &attrvalue;
	SaImmAttrValuesT_2 ComsaRlistClassRdnValue = {
					strRlistNumElements,            // name
					SA_IMM_ATTR_SAUINT32T,          // type
					1,                              // number of values
					(SaImmAttrValueT*) &attrValue1  // pointer to the value
	};

	// Block size attribute value
	uint32_t dataBufferSize = 0;
	SaImmAttrValueT attrValue2 = &dataBufferSize;
	SaImmAttrValuesT_2 ComsaRlistBlockSizeValue = {
		strRlistBlockSize,				// name
		SA_IMM_ATTR_SAUINT32T,			// type
		1,								// number of values
		(SaImmAttrValueT*) &attrValue2	// pointer to the value
	};

	attrValues_Returned = new SaImmAttrValuesT_2*[3];
	attrValues_Returned[0] = &ComsaRlistClassRdnValue;
	attrValues_Returned[1] = &ComsaRlistBlockSizeValue;
	attrValues_Returned[2] = NULL;
	saImmOmAccessorGet_2_rc = SA_AIS_OK;

	attrvalue = 2;
	res =  maf_comsa_imm_listGetFrontRef(listInstanceName, &listInstanceFrontRef_UT);
	EXPECT_EQ(((RLIterator*) listInstanceFrontRef_UT)->index, 0);
	EXPECT_EQ(((RLIterator*) listInstanceFrontRef_UT)->id, 1);
	EXPECT_EQ(res, MafOk);

	delete []attrValues_Returned;
	delete listInstanceName;
	attrValues_Returned = NULL;
	listInstanceName = NULL;

	//iterator will be free in TC maf_comsa_imm_listGetFinalize
}

TEST (RlistTest_imm, maf_comsa_imm_listGetFinalize)
{
	MafReturnT res = MafFailure;
	MafMwSpiListNameT* listInstanceName = NULL;
	const char * listname = "ReplListFront";
	MafMwSpiListItemRefT currentItemRef;

	listInstanceName =  new MafMwSpiListNameT;
	strncpy((char *)&listInstanceName->value, listname, strlen(listname) +1);
	listInstanceName->length = strlen(listname);

	//Case NULL
	currentItemRef = NULL;
	res = maf_comsa_imm_listGetFinalize(listInstanceName, currentItemRef);
	EXPECT_EQ(res, MafOk);


	//normal case
	res = maf_comsa_imm_listGetFinalize(listInstanceName, listInstanceFrontRef_UT);
	EXPECT_EQ(res, MafOk);

	// Delete List
	res = maf_comsa_imm_listDelete(listInstanceName);
	EXPECT_EQ(res, MafOk);

	delete listInstanceName;
	listInstanceName = NULL;
}

TEST (RlistTest_imm, maf_comsa_imm_listGetNextItemFront)
{
	MafReturnT res = MafFailure;
	MafMwSpiListNameT* listInstanceName = NULL;
	const char * listname = "ReplListNextFront";
	MafMwSpiListItemRefT currentItemRef;
	MafMwSpiListItemNameT listItemName ;
	MafMwSpiListItemRefT listInstanceFrontRef;
	struct RLIterator* iterator;
	char buffer[100] = {0};
	void* copyOfItemData = (void*) buffer;

	listInstanceName =  new MafMwSpiListNameT;
	strncpy((char *)&listInstanceName->value, listname, strlen(listname) +1);
	listInstanceName->length = strlen(listname);


	//case NULL
	res = maf_comsa_imm_listGetNextItemFront(listInstanceName, NULL, &listItemName, copyOfItemData);
	EXPECT_EQ(res, MafInvalidArgument);


	//normal case
	char className[] = "ReplListNextFront";
	char strRlistNumElements[] = "comsaRlistNumElements";
	char strRlistLastId[] = "comsaRlistLastId";
	SaNameT objectName;
	unsigned int numberElement = 0;
	unsigned int lastId = 0;

	int currentItemName =0;
	char nextListItemName[MAX_ITEM_NAME_LEN];
	unsigned int intNextListItemName = 0;

	//Create
	saImmOmClassDescriptionGet_2_rc = SA_AIS_ERR_NOT_EXIST;
	res = maf_comsa_imm_listCreate(listInstanceName, 100);
	EXPECT_EQ(res, MafOk);

	classToObj(className, &objectName);
	SaImmAttrValueT attrValue1 = &lastId;
	SaImmAttrValuesT_2 ComsaRlistLastIdValue = {
			strRlistLastId,				// name
			SA_IMM_ATTR_SAUINT32T,			// type
			1,								// number of values
			(SaImmAttrValueT*) &attrValue1	// pointer to the value
	};

	// this is the current number of elements in the list
	SaImmAttrValueT attrValue2 = &numberElement;
	SaImmAttrValuesT_2 ComsaRlistNumberElementValue = {
					strRlistNumElements,            // name
					SA_IMM_ATTR_SAUINT32T,          // type
					1,                              // number of values
					(SaImmAttrValueT*) &attrValue2 // pointer to the value
	};

	// Block size attribute value
	char newDataBuffer1[] = "UTTesting1";
	uint32_t dataBufferSize = 0;
	SaImmAttrValueT attrValue3 = &dataBufferSize;
	SaImmAttrValuesT_2 ComsaRlistBlockSizeValue = {
		strRlistBlockSize,				// name
		SA_IMM_ATTR_SAUINT32T,			// type
		1,								// number of values
		(SaImmAttrValueT*) &attrValue3	// pointer to the value
	};

	attrValues_Returned = new SaImmAttrValuesT_2*[4];
	attrValues_Returned[0] = &ComsaRlistLastIdValue;
	attrValues_Returned[1] = &ComsaRlistNumberElementValue;
	attrValues_Returned[2] = &ComsaRlistBlockSizeValue;
	attrValues_Returned[3] = NULL;
	saImmOmAccessorGet_2_rc = SA_AIS_OK;

	//call pushback --> add new file
	saImmOmAccessorGet_2_rc = SA_AIS_OK;
	res = maf_comsa_imm_listPushBack(listInstanceName, &listItemName, (void*) newDataBuffer1);
	EXPECT_EQ(res, MafOk);

	res = maf_comsa_imm_listGetFrontRef(listInstanceName, &listInstanceFrontRef);  //return the list size is: 1
	EXPECT_EQ(res, MafOk);

	res = maf_comsa_imm_listGetNextItemFront(listInstanceName, &listInstanceFrontRef, &listItemName, copyOfItemData);
	EXPECT_EQ(res, MafOk);

	// Delete List
	res = maf_comsa_imm_listDelete(listInstanceName);
	EXPECT_EQ(res, MafOk);

	delete listInstanceName;
	listInstanceName = NULL;
	delete []attrValues_Returned;
	attrValues_Returned = NULL;
}

TEST (RlistTest_imm, maf_comsa_imm_listFindItem_error_validate)
{
	MafReturnT res = MafFailure;

	MafMwSpiListNameT listInstanceNameUT;
	listInstanceNameUT.value[0] = '0';
	listInstanceNameUT.value[1] = 0;
	listInstanceNameUT.length = 1;
	MafMwSpiListItemNameT listItemName;
	listItemName.value[0] = '1';
	listItemName.value[1] = 0;
	listItemName.length = 1;
	char origItemData[] = "ORIG";

	// Error with listInstanceName is not validate
	res = maf_comsa_imm_listFindItem(&listInstanceNameUT, &listItemName, origItemData);
	EXPECT_EQ(res, MafNotExist);

	//Error with listItemName is not validate
	char instanceName[] = "ReplList";
	unsigned int instanceLen = strlen(instanceName);
	strncpy((char*)listInstanceNameUT.value, instanceName, instanceLen);
	listInstanceNameUT.value[instanceLen] = '\0';
	listInstanceNameUT.length = instanceLen;
	MafMwSpiListItemNameT listItemNameUT;
	listItemNameUT.value[0] = '0';
	listItemNameUT.value[1] = 0;
	listItemNameUT.length = 1;
	res = maf_comsa_imm_listFindItem(&listInstanceNameUT, &listItemNameUT, origItemData);
	EXPECT_EQ(res, MafNotExist);
}
TEST (RlistTest_imm, maf_comsa_imm_listFindItem)
{
		MafReturnT res = MafFailure;

		MafMwSpiListNameT listInstanceNameUT;
		listInstanceNameUT.length = 0;
		listInstanceNameUT.value[0] = NULL;
		MafMwSpiListItemNameT listItemName;
		char itemName[] = "1";
		unsigned int itemLen = strlen(itemName);
		strncpy((char*)listItemName.value, itemName, itemLen);
		listItemName.value[itemLen] = '\0';
		listItemName.length = itemLen;
		char origItemData[] = "ORIG";

		// Error with listInstanceName = NULL
		res = maf_comsa_imm_listFindItem(&listInstanceNameUT, &listItemName, (void*) origItemData);
		EXPECT_EQ(res, MafNotExist);

		// Error with listItemName = NULL
		listItemName.length = 0;
		listItemName.value[0] = NULL;
		char instanceName[] = "ReplList";
		unsigned int instanceLen = strlen(instanceName);
		strncpy((char*)listInstanceNameUT.value, instanceName, instanceLen);
		listInstanceNameUT.value[instanceLen] = '\0';
		listInstanceNameUT.length = instanceLen;
		res = maf_comsa_imm_listFindItem(&listInstanceNameUT, &listItemName, (void*) origItemData);
		EXPECT_EQ(res, MafNotExist);

		// Error with copyOfItemData = NULL
		strncpy((char*)listItemName.value, itemName, itemLen);
		listItemName.value[itemLen] = '\0';
		listItemName.length = itemLen;
		void *origItemDataUT = NULL;
		res = maf_comsa_imm_listFindItem(&listInstanceNameUT, &listItemName, origItemDataUT);
		EXPECT_EQ(res, MafInvalidArgument);

		res = maf_comsa_imm_listCreate(&listInstanceNameUT, 100);
		EXPECT_EQ(res, MafOk);

		saImmOiRtObjectUpdate_2_rc = SA_AIS_OK;
		attrValues_Returned = createAttrValuesBlocksize(sizeof(origItemData));
		res = maf_comsa_imm_listPushBack(&listInstanceNameUT, &listItemName, (void*)origItemData);
		EXPECT_EQ(res, MafOk);

		// Success
		res = maf_comsa_imm_listFindItem(&listInstanceNameUT, &listItemName, (void*) origItemData);
		EXPECT_EQ(res, MafOk);

		// Error with getObjectUiAttribute returns not OK
		saImmOmAccessorGet_2_rc = SA_AIS_ERR_NOT_EXIST;
		res = maf_comsa_imm_listFindItem(&listInstanceNameUT, &listItemName, (void*) origItemData);
		EXPECT_EQ(res, MafNotExist);

		res = maf_comsa_imm_listDelete(&listInstanceNameUT);
		EXPECT_EQ(res, MafOk);

		deleteAttrValuesBlocksize();
}


TEST (RlistTest_imm, maf_comsa_imm_listReplaceItem)
{
		MafReturnT res = MafFailure;

		MafMwSpiListNameT invalidListName;
		strncpy((char*)invalidListName.value, "\0", 0);
		invalidListName.length = 0;

		MafMwSpiListItemNameT invalidItemName;
		strncpy((char*)invalidItemName.value, "\0", 0);
		invalidItemName.length = 0;

		MafMwSpiListItemNameT notExistItemName;
		strncpy((char*)notExistItemName.value, "5", 2);
		notExistItemName.length = 1;

		MafMwSpiListNameT listInstanceName;
		char instanceName[] = "ReplList";
		unsigned int instanceLen = strlen(instanceName);
		strncpy((char*)listInstanceName.value, instanceName, instanceLen+1);
		listInstanceName.value[instanceLen] = '\0';
		listInstanceName.length = instanceLen;

		MafMwSpiListItemNameT listItemName;
		char itemName[] = "1";
		unsigned int itemLen = strlen(itemName);
		strncpy((char*)listItemName.value, itemName, itemLen+1);
		listItemName.value[itemLen] = '\0';
		listItemName.length = itemLen;

		char origItemData[] = "ORIG";
		char replaceItemData[] = "DATA";

		res = maf_comsa_imm_listCreate(&listInstanceName, 100);
		EXPECT_EQ(res, MafOk);

		saImmOmAccessorGet_2_rc = SA_AIS_OK;
		attrValues_Returned = createAttrValuesBlocksize(sizeof(origItemData));
		res = maf_comsa_imm_listPushBack(&listInstanceName, &listItemName, (void*)origItemData);
		EXPECT_EQ(res, MafOk);

		/* Rainy case: invalid invalidListName */
		res = maf_comsa_imm_listReplaceItem(&invalidListName, &listItemName, (void*)replaceItemData);
		EXPECT_EQ(res, MafNotExist);

		/* Rainy case: invalid invalidItemName */
		res = maf_comsa_imm_listReplaceItem(&listInstanceName, &invalidItemName, (void*)replaceItemData);
		EXPECT_EQ(res, MafNotExist);

		/* Rainy case: not exist notExistItemName */
		res = maf_comsa_imm_listReplaceItem(&listInstanceName, &notExistItemName, (void*)replaceItemData);
		EXPECT_EQ(res, MafNotExist);

		/* Sunny case: valid parameters */
		saImmOmAccessorGet_2_rc = SA_AIS_OK;
		res = maf_comsa_imm_listReplaceItem(&listInstanceName, &listItemName, (void*)replaceItemData);
		EXPECT_EQ(res, MafOk);

		/* Fix me:
		 * Currently, there is still no available simulation of getObjectUiAttribute
		 * so maf_comsa_imm_listReplaceItem cannot write the correct data to file.
		 * Then we cannot assess the content of replacing data.
		 * In future, it should be assessed
		 *
		int datasize = strlen(replaceItemData);
		void *data = NULL;
		readFromFile((char*)listItemName.value, (char*)listInstanceName.value, data, datasize);
		//EXPECT_EQ(replaceItemData, char* data);
		if(0 == strcmp((char*) data, replaceItemData))
		{
			EXPECT_EQ(res, MafOk);
		}
		else
		{
			// If replaceItemData is not matched, here should be a FAIL case
			EXPECT_EQ(res, MafFailure);
		}
		*/

		res = maf_comsa_imm_listDelete(&listInstanceName);
		EXPECT_EQ(res, MafOk);

		deleteAttrValuesBlocksize();
}

TEST (RlistTest_imm, maf_comsa_imm_listNumberOfListInstances)
{
// MafReturnT maf_comsa_imm_listNumberOfListInstances(uint32_t* numberOfLinkListInstances);
	MafReturnT res = MafFailure;
	uint32_t numberOfLinkListInstances = 0;
	// Success
	res = maf_comsa_imm_listNumberOfListInstances(&numberOfLinkListInstances);
	EXPECT_EQ(res, MafOk);
	EXPECT_EQ(numberOfLinkListInstances, 1);
	// Error
	res = maf_comsa_imm_listNumberOfListInstances(NULL);
	EXPECT_EQ(res, MafInvalidArgument);
}

TEST (RlistTest_imm, maf_comsa_imm_listMemoryUsage)
{
// MafReturnT maf_comsa_imm_listMemoryUsage(uint32_t* memoryUsed, uint32_t* totalMemoryAvailable);
	MafReturnT res = MafFailure;
	uint32_t memoryUsed = 0;
	uint32_t totalMemoryAvailable = 0;
	// Success
	res = maf_comsa_imm_listMemoryUsage(&memoryUsed, &totalMemoryAvailable);
	EXPECT_EQ(res, MafOk);
	EXPECT_EQ(memoryUsed, 10);
// TODO	EXPECT_EQ(totalMemoryAvailable, 2097152);
	// Error
	res = maf_comsa_imm_listMemoryUsage(NULL, NULL);
	EXPECT_EQ(res, MafInvalidArgument);
}

TEST (RlistTest_imm, classToObj)
{
//void classToObj(const char* className, SaNameT* rdnName);
	MafReturnT res = MafFailure;
	const char className[] = "ReplList";
	const char rdnUT[] = "ReplList=1";
	SaNameT rdnName;

	classToObj(className, &rdnName);
	char* returnRdn = makeCString(&rdnName);
	DEBUG_MWSA_REPLIST("classToObj():returnRdn is [%s]", returnRdn);

	if(0 == strcmp(rdnUT, returnRdn))
	{
		res = MafOk;
	}
	EXPECT_EQ(res, MafOk);
	delete []returnRdn;
	//returnRdn = NULL;
	//rdnName = NULL;
}
extern SaImmAttrValuesT_2** attrValues_Returned;
TEST (RlistTest_imm, findNextListItem)
{
	MafReturnT res = MafFailure;
	MafMwSpiListNameT* listInstanceName = NULL;

	const char * listname = "FMRplistName";
	int currentItemName = 1;
	char nextListItemName[MAX_ITEM_NAME_LEN];
	unsigned int intNextListItemName = 0;
	char * fName1 = "1";
	char * fName3 = "3";
	char * fName6 = "6";

	listInstanceName =  new MafMwSpiListNameT;
	strncpy((char *)&listInstanceName->value, listname, strlen(listname) +1);
	listInstanceName->length = strlen(listname);

	// Case 1: all NULL parameter
	saImmOmAccessorGet_2_rc = SA_AIS_ERR_NOT_EXIST;
	res = findNextListItem(listInstanceName, currentItemName,(char *) &nextListItemName, &intNextListItemName, NULL);
	EXPECT_EQ(res, MafNotExist);

	// Case 2: 0 member
	SaNameT objectName;
	char className[] = "FMRplistName";
	char strRlistClassRdn[] = "comsaRlistLastId";
	unsigned int attrvalue = 0;
	SaNameT rdnName;

	// Normal case
	saImmOmClassDescriptionGet_2_rc = SA_AIS_ERR_NOT_EXIST;
	res = maf_comsa_imm_listCreate(listInstanceName, 100);
	EXPECT_EQ(res, MafOk);

	classToObj(className, &rdnName);
	SaImmAttrValueT attrValue1 = &attrvalue;
	SaImmAttrValuesT_2 ComsaRlistClassRdnValue = {
		strRlistClassRdn,				// name
		SA_IMM_ATTR_SAUINT32T,			// type
		1,								// number of values
		(SaImmAttrValueT*) &attrValue1	// pointer to the value
	};
	// Combine all the attribute values in one NULL terminated array
	attrValues_Returned = new SaImmAttrValuesT_2*[2];
	attrValues_Returned[0] = &ComsaRlistClassRdnValue;
	attrValues_Returned[1] = NULL;
	saImmOmAccessorGet_2_rc = SA_AIS_OK;

	attrvalue = 0;
	currentItemName = 0;
	res = findNextListItem(listInstanceName, currentItemName,(char *) &nextListItemName, &intNextListItemName, NULL);
	EXPECT_EQ(res, MafFailure);

	// Case 3: 1 member

	res = writeToFile((const char*)fName1, (const char*)listname, (const void*)"", (const int)0);

	attrvalue = 1;
	currentItemName = 0;
	res = findNextListItem(listInstanceName, currentItemName,(char *) &nextListItemName, &intNextListItemName, NULL);
	EXPECT_EQ(res, MafOk);
	EXPECT_EQ(intNextListItemName, 1);

	currentItemName = 2;
	res = findNextListItem(listInstanceName, currentItemName,(char *) &nextListItemName, &intNextListItemName, NULL);
	EXPECT_EQ(res, MafFailure);

	// Case 4: many member
	res = writeToFile((const char*)fName3, (const char*)listname, (const void*)"", (const int)0);
	res = writeToFile((const char*)fName6, (const char*)listname, (const void*)"", (const int)0);
	attrvalue = 3;
	currentItemName = 0;
	res = findNextListItem(listInstanceName, currentItemName,(char *) &nextListItemName, &intNextListItemName, NULL);
	EXPECT_EQ(res, MafOk);
	EXPECT_EQ(intNextListItemName, 1);

	currentItemName = 2;
	res = findNextListItem(listInstanceName, currentItemName,(char *) &nextListItemName, &intNextListItemName, NULL);
	EXPECT_EQ(res, MafOk);
	EXPECT_EQ(intNextListItemName, 3);

	attrvalue = 7;
	currentItemName = 4;
	res = findNextListItem(listInstanceName, currentItemName,(char *) &nextListItemName, &intNextListItemName, NULL);
	EXPECT_EQ(res, MafOk);
	EXPECT_EQ(intNextListItemName, 6);

	currentItemName = 7;
	res = findNextListItem(listInstanceName, currentItemName,(char *) &nextListItemName, &intNextListItemName, NULL);
	EXPECT_EQ(res, MafFailure);

	res = maf_comsa_imm_listDelete(listInstanceName);
	EXPECT_EQ(res, MafOk);

	delete listInstanceName;
	delete []attrValues_Returned;
	listInstanceName = NULL;
	attrValues_Returned = NULL;

}

TEST (RlistTest_imm, CLEAR_SYSTEM)
{
	// This is the LATEST testcase in RlistTest_imm. Just for clearing all created directory.
	// Each TC need to clear the /rplist/"subfolder" itself. This TC remove "rplist" folder only.
	int return_c = 0;
	extern char strRpListPath[];
	struct stat st;

	const char *pathArray[4];
	pathArray[0] = pathToStorageDirectory;
	pathArray[1] = COMSA_FOR_COREMW_DIR;
	pathArray[2] = strRpListPath;
	pathArray[3] = "ReplList";
	char* dir = createPath(pathArray, 4, NULL);
	DEBUG_MWSA_REPLIST("CLEAR_SYSTEM():dir is  %s", dir);
	if(0 == stat(dir, &st))
	{
		DEBUG_MWSA_REPLIST("CLEAR_SYSTEM():dir [%s] exists. Deleting", dir);
		return_c = rmdir(dir);
		EXPECT_EQ(return_c, 0);
	}
	else
	{
		DEBUG_MWSA_REPLIST("CLEAR_SYSTEM():dir [%s] doesn't exists", dir);
	}
	if(NULL != dir)
		free(dir);

	dir = createPath(pathArray, 3, NULL);
	DEBUG_MWSA_REPLIST("CLEAR_SYSTEM():dir is  %s", dir);
	if(0 == stat(dir, &st))
	{
		DEBUG_MWSA_REPLIST("CLEAR_SYSTEM():dir [%s] exists. Deleting", dir);
		return_c = rmdir(dir);
		EXPECT_EQ(return_c, 0);
	}
	else
	{
		DEBUG_MWSA_REPLIST("CLEAR_SYSTEM():dir [%s] doesn't exists", dir);
	}
	if(NULL != dir)
		free(dir);

	dir = createPath(pathArray, 2, NULL);
		DEBUG_MWSA_REPLIST("CLEAR_SYSTEM():dir is  %s", dir);
		if(0 == stat(dir, &st))
		{
			DEBUG_MWSA_REPLIST("CLEAR_SYSTEM():dir [%s] exists. Deleting", dir);
			return_c = rmdir(dir);
			EXPECT_EQ(return_c, 0);
		}
		else
		{
			DEBUG_MWSA_REPLIST("CLEAR_SYSTEM():dir [%s] doesn't exists", dir);
		}
		if(NULL != dir)
			free(dir);
}

TEST (RlistTest_imm, RTObjectName) {

	MafReturnT res = MafOk;
	MafMwSpiListNameT* listInstanceName = (MafMwSpiListNameT*) malloc(sizeof(MafMwSpiListNameT));
	std::string lastId("9");
	const char *val = "FmActiveAlarmList2";
	memcpy(listInstanceName->value, val, strlen(val) + 1);
	listInstanceName->length = strlen("FmActiveAlarmList2") + 1;
	SaNameT *immObject = NULL;

	const char outVal[] = "FmActiveAlarmList2RT=9";

	res = convertToRTObjectName(std::string((char*)listInstanceName->value), lastId, &immObject);

	EXPECT_EQ(res, MafOk);

	char* endVal = makeCString(immObject);

	DEBUG_MWSA_REPLIST("RunTObjectName():immObject is [%s]", endVal);

	if (strcmp(outVal, endVal) != 0) {
		res = MafFailure;
	}

	EXPECT_EQ(res, MafOk);

	free(listInstanceName);
	delete immObject;
}

TEST (RlistTest_imm, getVoidData)
{
	MafReturnT res = MafOk;

	unsigned int dataLen = 3000;

	typedef struct {
		int f;
		int g;
		int h;
	} myStruct;

	myStruct ms = { 69, 79, 89 };

	DEBUG_MWSA_REPLIST("getVoidData():size of ms :%d ", sizeof(ms));

	SaAnyT anyVal;
	anyVal.bufferSize = sizeof(ms);
	anyVal.bufferAddr = (SaUint8T*) malloc(anyVal.bufferSize);

	memcpy(anyVal.bufferAddr, &ms, anyVal.bufferSize);

	void *arr1[] = { &anyVal };

	char* attrName = "data";

	SaImmAttrValuesT_2 attrVal = {
			(char*)attrName,
			SA_IMM_ATTR_SAANYT,
			1,
			arr1 };

	SaImmAttrValuesT_2* attrValues[] = {&attrVal, NULL};

	void* data = malloc(3000);

	res = getVoidData((const SaImmAttrValuesT_2**)attrValues, &data, dataLen);

	EXPECT_EQ(res, MafOk);

	myStruct ms2;

	memcpy(&ms2, data, sizeof(ms2));

	DEBUG_MWSA_REPLIST("getVoidData():attrValues[values] [%d %d %d]", ms2.f, ms2.g, ms2.h);

	if ((ms.f == ms2.f) && (ms.g == ms2.g) && (ms.h == ms2.h)) {
		res = MafOk;
	} else {
		res = MafFailure;
	}
	EXPECT_EQ(res, MafOk);
	free(anyVal.bufferAddr);
	free(data);
}

TEST(RlistTest_imm, rlistRTObjects) {
	MafReturnT res = MafOk;
	unsigned int dataLen = 0;
	char newDataBuffer[] = "UTTesting";
	const char *className = "FmActiveAlarmList2";
	const char *ItemName = "1";

	typedef struct {
		int f;
		int g;
		int h;
	} myStruct;

	myStruct ms = { 10, 20, 30 };
	DEBUG_MWSA_REPLIST("rlistRTObjects():size of ms: %d", sizeof(ms));
	dataLen = sizeof(ms);

	void *data = malloc(dataLen);
	memcpy(data, &ms, dataLen);

	attrValues_Returned = createAttrValuesBlocksize(sizeof(newDataBuffer));

	//Error
	saImmOiRtObjectCreate_2_rc = SA_AIS_ERR_NOT_EXIST;
	res = addRlistRTObject(className, atoi(ItemName), data);
	EXPECT_EQ(res, MafFailure);

	//success
	saImmOiRtObjectCreate_2_rc = SA_AIS_OK;
	res = addRlistRTObject(className, atoi(ItemName), data);
	EXPECT_EQ(res, MafOk);

	//Error
	saImmOiRtObjectUpdate_2_rc = SA_AIS_ERR_NOT_EXIST;
	res = modifyRlistRTObject(className, ItemName, data);
	EXPECT_EQ(res, MafFailure);

	//success
	saImmOiRtObjectUpdate_2_rc = SA_AIS_OK;
	res = modifyRlistRTObject(className, ItemName, data);
	EXPECT_EQ(res, MafOk);

	//Error
	saImmOiRtObjectDelete_rc = SA_AIS_ERR_NOT_EXIST;
	res = deleteRTObject(className, ItemName);
	EXPECT_EQ(res, MafNotExist);

	//success
	saImmOiRtObjectDelete_rc = SA_AIS_OK;
	res = deleteRTObject(className, ItemName);
	EXPECT_EQ(res, MafOk);

	free(data);
	deleteAttrValuesBlocksize();
}

TEST(RlistTest_imm, RlistAllRTObjects) {
	MafReturnT res = MafOk;
	unsigned int dataLen = 0;
	char newDataBuffer[] = "UTTesting";
	const char *className = "FmActiveAlarmList2";
	const char *ItemName1 = "1";
	const char *ItemName2 = "2";
	std::vector<std::string> immRTObjects;
	uint32_t actualDataSize = 0;

	typedef struct {
		int f;
		int g;
		int h;
	} myStruct;

	myStruct ms = { 10, 20, 30 };
	DEBUG_MWSA_REPLIST("ImmAttrValuesT():size of ms1: %d", sizeof(ms));

	dataLen = sizeof(ms);

	void *data = malloc(dataLen);
	memcpy(data, &ms, dataLen);

	attrValues_Returned = createAttrValuesBlocksize(sizeof(newDataBuffer));

	//Add obj1
	saImmOiRtObjectCreate_2_rc = SA_AIS_OK;
	res = addRlistRTObject(className, atoi(ItemName1), data);
	EXPECT_EQ(res, MafOk);

	//Add obj2
	saImmOiRtObjectCreate_2_rc = SA_AIS_OK;
	res = addRlistRTObject(className, atoi(ItemName2), data);
	EXPECT_EQ(res, MafOk);

	//success
	std::vector<std::string> test;
	saImmOmSearchNext_2_rc = SA_AIS_ERR_NOT_EXIST;
	res = getAllRTObjects(className,test);
	EXPECT_EQ(res, MafOk);

	//Error
	saImmOmSearchInitialize_2_rc = SA_AIS_ERR_NOT_EXIST;
	res = getAllRTObjects(className,test);
	EXPECT_EQ(res, MafNotExist);
	saImmOmSearchInitialize_2_rc = SA_AIS_OK;

	uint32_t count = 0;
	res = getRTObjectsCount(className,actualDataSize);
	EXPECT_EQ(actualDataSize, count);

	res = deleteAllRTObjects(className);
	EXPECT_EQ(res, MafOk);

	free(data);
	deleteAttrValuesBlocksize();
}
