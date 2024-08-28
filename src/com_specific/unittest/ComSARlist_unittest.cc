/*
 * ComSARlist_unittest.cc
 *
 *  Created on: Dec 11, 2014
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

extern bool replicateInIMM;

const SaVersionT imm_version = { immReleaseCode, immMajorVersion, immMinorVersion };

// MOCKED FUNCTIONS
xmlNode* coremw_find_config_item(char const* component_name, char const* node_name)
{
	return NULL;
}

char const* coremw_xmlnode_contents(xmlNode* cur)
{
	return NULL;
}

char const* coremw_xmlnode_get_attribute(xmlNode* n, char const* attr_name)
{
	if(strcmp(attr_name, "clearAlarmsOnClusterReboot") == 0){
		return (char const*)n->properties->children->content;
	}
	else {
		return "NO PARAMETER";
	}
}

unsigned long long getMillisecondsSinceEpochUnixTime()
{
	return 0;
}

// Declare extern variables for testing
extern SaAisErrorT saImmOmClassDescriptionGet_2_rc;
extern SaAisErrorT saImmOiRtObjectCreate_2_rc;
extern SaAisErrorT saImmOmAccessorGet_2_rc;
extern SaAisErrorT saImmOiRtObjectDelete_rc;
extern SaAisErrorT saImmOiRtObjectUpdate_2_rc;

// Declare external functions to be tested

// Replicated List Functions

extern "C" MafReturnT maf_comsa_listCreate(const MafMwSpiListNameT* listInstanceName, uint32_t dataBufferSize);
extern "C" MafReturnT maf_comsa_listDelete(const MafMwSpiListNameT* listInstanceName);
extern "C" MafReturnT maf_comsa_listClear(const MafMwSpiListNameT* listInstanceName);
extern "C" MafReturnT maf_comsa_listGetSize(const MafMwSpiListNameT* listInstanceName, uint32_t* listSize);
extern "C" MafReturnT maf_comsa_listIsEmpty(const MafMwSpiListNameT* listInstanceName, bool* listEmpty);
extern "C" MafReturnT maf_comsa_listPushBack(const MafMwSpiListNameT* listInstanceName, MafMwSpiListItemNameT* listItemName, void* newItemDataBuffer);
extern "C" MafReturnT maf_comsa_listPopBack(const MafMwSpiListNameT* listInstanceName);
extern "C" MafReturnT maf_comsa_listEraseItem(const MafMwSpiListNameT* listInstanceName, const MafMwSpiListItemNameT* listItemName);
extern "C" MafReturnT maf_comsa_listGetFrontRef(const MafMwSpiListNameT* listInstanceName, MafMwSpiListItemRefT* listInstanceFrontRef);
extern "C" MafReturnT maf_comsa_listGetFinalize(const MafMwSpiListNameT* listInstanceName, const MafMwSpiListItemRefT currentItemRef);
extern "C" MafReturnT maf_comsa_listGetNextItemFront(const MafMwSpiListNameT* listInstanceName, MafMwSpiListItemRefT* currentItemRef, MafMwSpiListItemNameT* listItemName, void* copyOfItemData);
extern "C" MafReturnT maf_comsa_listFindItem(const MafMwSpiListNameT* listInstanceName, const MafMwSpiListItemNameT* listItemName, void* copyOfItemData);
extern "C" MafReturnT maf_comsa_listReplaceItem(const MafMwSpiListNameT* listInstanceName, const MafMwSpiListItemNameT* listItemName, const void* replaceItemData);
extern "C" MafReturnT maf_comsa_listNumberOfListInstances(uint32_t* numberOfLinkListInstances);
extern "C" MafReturnT maf_comsa_listMemoryUsage(uint32_t* memoryUsed, uint32_t* totalMemoryAvailable);

extern "C" MafReturnT maf_comSAMwComponentStart(MafStateChangeReasonT reason);
extern "C" MafReturnT maf_comSAMwComponentStop(MafStateChangeReasonT reason);
extern "C" MafReturnT maf_comSAMwComponentInitialize(MafMgmtSpiInterfacePortal_3T* portal, xmlDocPtr config);
extern "C" void maf_comSAMwComponentFinalize(MafMgmtSpiInterfacePortal_3T* portal);

extern "C" unsigned long long readShmmaxValue();
extern "C" void set_rlist_maxsize(xmlNode* cfg);
extern "C" void set_clearAlarmsOnClusterReboot(xmlNode* cfg);
extern "C" void set_replicateInIMM();
extern "C" void classToObj(const char* className, SaNameT* rdnName);

// UNIT TEST
extern unsigned long long rlist_maxsize;
extern 	char strRlistLastId[] ;
extern char strRlistNumElements[] ;
extern SaImmAttrValuesT_2** attrValues_Returned;

extern SaImmAttrValuesT_2** createAttrValuesBlocksize(uint32_t);
extern void deleteAttrValuesBlocksize();

TEST (RlistTest, maf_comsa_listCreate)
{
// MafReturnT maf_comsa_listCreate(const MafMwSpiListNameT* listInstanceName, uint32_t dataBufferSize);
	MafReturnT res = MafFailure;
	MafMwSpiListNameT listInstanceName;
	strncpy((char*)listInstanceName.value, "ReplList", 9);
	listInstanceName.value[8] = '\0';
	listInstanceName.length = 8;
	rlist_maxsize = 200;
	char className[] = "TransferAlarm";
	char strRlistClassRdn[] = "comsaRlistLastId";
	unsigned int lastId = 0;
	unsigned int numberElement = 0;
	SaNameT rdnName;

	// Normal case
	saImmOmAccessorGet_2_rc = SA_AIS_ERR_NOT_EXIST;
	saImmOmClassDescriptionGet_2_rc = SA_AIS_ERR_NOT_EXIST;
	res = maf_comsa_listCreate(&listInstanceName, 100);
	EXPECT_EQ(res, MafOk);
	saImmOmClassDescriptionGet_2_rc = SA_AIS_OK;
	res = maf_comsa_listDelete(&listInstanceName);
	saImmOmAccessorGet_2_rc = SA_AIS_OK;
	EXPECT_EQ(res, MafOk);

}

TEST (RlistTest, maf_comsa_listPushBack)
{
// MafReturnT maf_comsa_listPushBack(const MafMwSpiListNameT* listInstanceName, MafMwSpiListItemNameT* listItemName, void* newItemDataBuffer)
	MafReturnT res = MafFailure;
	MafMwSpiListItemNameT listItemName;
	void* invalidDataBuffer = NULL;
	char validDataBuffer[] = "UTTesting";


	/* Rainy case: invalid listInstanceName */
	MafMwSpiListNameT invalidListInstanceName;
	strncpy((char*)invalidListInstanceName.value, "\0", 0);
	invalidListInstanceName.length = 0;
	res = maf_comsa_listPushBack(&invalidListInstanceName, &listItemName, invalidDataBuffer);
	EXPECT_EQ(res, MafNotExist);

	/* Rainy case: newItemDataBuffer is NULL */
	MafMwSpiListNameT listInstanceName;
	strncpy((char*)listInstanceName.value, "ReplList", 9);
	listInstanceName.length = 8;

	res = maf_comsa_listCreate(&listInstanceName, 100);
	EXPECT_EQ(res, MafOk);

	res = maf_comsa_listPushBack(&listInstanceName, &listItemName, invalidDataBuffer);
	EXPECT_EQ(res, MafNotExist);

	attrValues_Returned = createAttrValuesBlocksize(sizeof(validDataBuffer));
	/* Sunny case: valid listInstanceName and not NULL newItemDataBuffer */
	res = maf_comsa_listPushBack(&listInstanceName, &listItemName, (void*) validDataBuffer);
	EXPECT_EQ(res, MafOk);

	res = maf_comsa_listDelete(&listInstanceName);
	EXPECT_EQ(res, MafOk);

	deleteAttrValuesBlocksize();
}

TEST (RlistTest, maf_comsa_listDelete)
{
// MafReturnT maf_comsa_listDelete(const MafMwSpiListNameT* listInstanceName);

	MafReturnT res = MafFailure;
	MafMwSpiListNameT listInstanceName;
	strncpy((char*)listInstanceName.value, "ReplList", 9);
	listInstanceName.length = 8;
	res = maf_comsa_listCreate(&listInstanceName, 100);
	EXPECT_EQ(res, MafOk);
	res = maf_comsa_listDelete(&listInstanceName);
	EXPECT_EQ(res, MafOk);
}

TEST (RlistTest, maf_comsa_listClear)
{
	// MafReturnT maf_comsa_listClear(const MafMwSpiListNameT* listInstanceName);
	MafReturnT res = MafFailure;
	MafMwSpiListNameT listInstanceName;
	strncpy((char*)listInstanceName.value, "ReplList", 9);
	listInstanceName.length = 8;

	// Normal case
	saImmOmClassDescriptionGet_2_rc = SA_AIS_ERR_NOT_EXIST;
	res = maf_comsa_listCreate(&listInstanceName, 100);
	EXPECT_EQ(res, MafOk);
	attrValues_Returned = NULL;
	res = maf_comsa_listClear(&listInstanceName);
	EXPECT_EQ(res, MafOk);

	res = maf_comsa_listDelete(&listInstanceName);
	EXPECT_EQ(res, MafOk);
}

TEST (RlistTest, maf_comsa_listGetSize)
{
// MafReturnT maf_comsa_listGetSize(const MafMwSpiListNameT* listInstanceName, uint32_t* listSize);
	MafReturnT res = MafFailure;
	MafMwSpiListNameT listInstanceName;
	strncpy((char*)listInstanceName.value, "ReplList", 9);
	listInstanceName.length = 8;
	uint32_t listSize = 0;
	res = maf_comsa_listGetSize(&listInstanceName, &listSize);
	EXPECT_EQ(res, MafOk);
	EXPECT_EQ(listSize, 0);
}

TEST (RlistTest, maf_comsa_listIsEmpty)
{
// MafReturnT maf_comsa_listIsEmpty(const MafMwSpiListNameT* listInstanceName, bool* listEmpty);
	MafReturnT res = MafOk;
	bool listEmpty = false;
	MafMwSpiListNameT listInstanceName;
	strncpy((char*)listInstanceName.value, "isEmptyReplList", 16);
	listInstanceName.length = 15;


	// Normal Check listEmpty = true
	res = maf_comsa_listIsEmpty(&listInstanceName, &listEmpty);
	EXPECT_EQ(res, MafOk);
	EXPECT_EQ(listEmpty, true);

}


TEST (RlistTest, maf_comsa_listPopBack)
{
// MafReturnT maf_comsa_listPopBack(const MafMwSpiListNameT* listInstanceName);
	MafReturnT res = MafFailure;
	MafMwSpiListNameT listInstanceName;

	strncpy((char*)listInstanceName.value, "ReplList", 9);
	listInstanceName.length = 8;
	char newDataBuffer[] = "UTTesting";
	MafMwSpiListItemNameT listItemName;

	res = maf_comsa_listCreate(&listInstanceName, 100);
	EXPECT_EQ(res, MafOk);

	attrValues_Returned = createAttrValuesBlocksize(sizeof(newDataBuffer));
	res = maf_comsa_listPushBack(&listInstanceName, &listItemName, (void*) newDataBuffer);
	EXPECT_EQ(res, MafOk);

	res = maf_comsa_listPopBack(&listInstanceName);
	EXPECT_EQ(res, MafOk);

	res = maf_comsa_listDelete(&listInstanceName);
	EXPECT_EQ(res, MafOk);

	deleteAttrValuesBlocksize();
}

TEST (RlistTest, maf_comsa_listEraseItem)
{
// MafReturnT maf_comsa_listEraseItem(const MafMwSpiListNameT* listInstanceName, const MafMwSpiListItemNameT* listItemName);
	MafReturnT res = MafFailure;
	MafMwSpiListNameT listName;
	MafMwSpiListItemNameT itemName;

	strncpy((char*)itemName.value, "1", 2);
	itemName.length = 1;

	/* Rainy case: invalid listInstanceName */
	strncpy((char*)listName.value, "\0", 0);
	listName.length = 0;
	res = maf_comsa_listEraseItem(&listName, &itemName);
	EXPECT_EQ(res, MafNotExist);
}

extern MafMwSpiListItemRefT listInstanceFrontRef_UT ;
TEST (RlistTest, maf_comsa_listGetFrontRef)
{
// MafReturnT maf_comsa_listGetFrontRef(const MafMwSpiListNameT* listInstanceName, MafMwSpiListItemRefT* listInstanceFrontRef);
	MafReturnT res = MafFailure;
	MafMwSpiListNameT* listInstanceName = NULL;
	const char * listname = "ReplListFront";

	listInstanceName =  new MafMwSpiListNameT;
	strncpy((char *)&listInstanceName->value, listname, strlen(listname) +1);
	listInstanceName->length = strlen(listname);

	//case NULL
	res = maf_comsa_listGetFrontRef(listInstanceName, NULL);
	EXPECT_EQ(res, MafNotExist);

	//case not exist
	saImmOmAccessorGet_2_rc = SA_AIS_ERR_NOT_EXIST;
	res = maf_comsa_listGetFrontRef(listInstanceName, &listInstanceFrontRef_UT);
	EXPECT_EQ(res, MafNotExist);

	//case list EMPTY
	saImmOmAccessorGet_2_rc = SA_AIS_OK;
	res = maf_comsa_listGetFrontRef(listInstanceName, &listInstanceFrontRef_UT);
	EXPECT_EQ(res, MafOk);

	delete listInstanceName;
}

TEST (RlistTest, maf_comsa_listGetFinalize)
{
// MafReturnT maf_comsa_listGetFinalize(const MafMwSpiListNameT* listInstanceName, const MafMwSpiListItemRefT currentItemRef);
	MafReturnT res = MafFailure;
	MafMwSpiListNameT* listInstanceName = NULL;
	const char * listname = "ReplListFront";
	MafMwSpiListItemRefT currentItemRef;
	struct RLIterator* iterator = NULL;

	listInstanceName =  new MafMwSpiListNameT;
	strncpy((char *)&listInstanceName->value, listname, strlen(listname) +1);
	listInstanceName->length = strlen(listname);

	//Case NULL
	currentItemRef = NULL;
	res = maf_comsa_listGetFinalize(listInstanceName, currentItemRef);
	EXPECT_EQ(res, MafOk);


	//normal case
	res = maf_comsa_listGetFinalize(listInstanceName, listInstanceFrontRef_UT);
	EXPECT_EQ(res, MafOk);
	EXPECT_EQ(iterator, (RLIterator *)0);

	// Delete List
	res = maf_comsa_listDelete(listInstanceName);
	EXPECT_EQ(res, MafOk);

	delete listInstanceName;
	listInstanceName = NULL;
}

TEST (RlistTest, maf_comsa_listGetNextItemFront)
{
// MafReturnT maf_comsa_listGetNextItemFront(const MafMwSpiListNameT* listInstanceName, MafMwSpiListItemRefT* currentItemRef, MafMwSpiListItemNameT* listItemName, void* copyOfItemData);
	MafReturnT res = MafFailure;
	MafMwSpiListNameT* listInstanceName = NULL;
	const char * listname = "ReplListNextFront";
	MafMwSpiListItemRefT currentItemRef;
	MafMwSpiListItemNameT listItemName ;
	MafMwSpiListItemRefT listInstanceFrontRef;
	struct RLIterator* iterator;
	void* copyOfItemData;

	listInstanceName =  new MafMwSpiListNameT;
	strncpy((char *)&listInstanceName->value, listname, strlen(listname) +1);
	listInstanceName->length = strlen(listname);


	//case NULL
	res = maf_comsa_listGetNextItemFront(listInstanceName, NULL, &listItemName, copyOfItemData);
	EXPECT_EQ(res, MafInvalidArgument);

/*
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
	res = maf_comsa_listCreate(listInstanceName, 100);
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

	attrValues_Returned = new SaImmAttrValuesT_2*[3];
	attrValues_Returned[0] = &ComsaRlistLastIdValue;
	attrValues_Returned[1] = &ComsaRlistNumberElementValue;
	attrValues_Returned[2] = NULL;
	saImmOmAccessorGet_2_rc = SA_AIS_OK;

	//call pushback --> add new file
	char newDataBuffer1[] = "UTTesting1";
	saImmOmAccessorGet_2_rc = SA_AIS_OK;
	res = maf_comsa_listPushBack(listInstanceName, &listItemName, (void*) newDataBuffer1);
	EXPECT_EQ(res, MafOk);

	res = maf_comsa_listGetFrontRef(listInstanceName, &listInstanceFrontRef);  //return the list size is: 1
	EXPECT_EQ(res, MafOk);

	res = maf_comsa_listGetNextItemFront(listInstanceName, &listInstanceFrontRef, &listItemName, copyOfItemData);
	EXPECT_EQ(res, MafOk);

	// Delete List
	res = maf_comsa_listDelete(listInstanceName);
	EXPECT_EQ(res, MafOk);

	delete listInstanceName;
	listInstanceName = NULL;
	delete attrValues_Returned;
	attrValues_Returned = NULL;
*/
	delete listInstanceName;
}

TEST (RlistTest, maf_comsa_listFindItem)
{
// MafReturnT maf_comsa_listFindItem(const MafMwSpiListNameT* listInstanceName, const MafMwSpiListItemNameT* listItemName, void* copyOfItemData);
	MafReturnT res = MafFailure;

	char listName[] = "ReplList";
	unsigned int listLen = strlen(listName);
	MafMwSpiListNameT listInstanceNameUT;
	listInstanceNameUT.length = listLen;
	strncpy((char*)listInstanceNameUT.value, listName, listLen);
	listInstanceNameUT.value[listLen] = '\0';
	MafMwSpiListItemNameT listItemName;
	char itemName[] = "1";
	unsigned int itemLen = strlen(itemName);
	strncpy((char*)listItemName.value, itemName, itemLen);
	listItemName.value[itemLen] = '\0';
	listItemName.length = itemLen;
	char origItemData[] = "ORIG";

	res = maf_comsa_listCreate(&listInstanceNameUT, 100);
	EXPECT_EQ(res, MafOk);

	saImmOiRtObjectUpdate_2_rc = SA_AIS_OK;
	attrValues_Returned = createAttrValuesBlocksize(sizeof(origItemData));
	res = maf_comsa_listPushBack(&listInstanceNameUT, &listItemName, (void*)origItemData);
	EXPECT_EQ(res, MafOk);

	// Success
	res = maf_comsa_listFindItem(&listInstanceNameUT, &listItemName, (void*) origItemData);
	EXPECT_EQ(res, MafOk);

	res = maf_comsa_listDelete(&listInstanceNameUT);
	EXPECT_EQ(res, MafOk);

	deleteAttrValuesBlocksize();
}

TEST (RlistTest, maf_comsa_listReplaceItem)
{
// MafReturnT maf_comsa_listReplaceItem(const MafMwSpiListNameT* listInstanceName, const MafMwSpiListItemNameT* listItemName, const void* replaceItemData);
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

	res = maf_comsa_listCreate(&listInstanceName, 100);
	EXPECT_EQ(res, MafOk);

	saImmOmAccessorGet_2_rc = SA_AIS_OK;
	attrValues_Returned = createAttrValuesBlocksize(sizeof(origItemData));
	res = maf_comsa_listPushBack(&listInstanceName, &listItemName, (void*)origItemData);
	EXPECT_EQ(res, MafOk);

	/* Sunny case: valid parameters */
	saImmOmAccessorGet_2_rc = SA_AIS_OK;
	res = maf_comsa_listReplaceItem(&listInstanceName, &listItemName, (void*)replaceItemData);
	EXPECT_EQ(res, MafOk);

	res = maf_comsa_listDelete(&listInstanceName);
	EXPECT_EQ(res, MafOk);

	deleteAttrValuesBlocksize();
}

TEST (RlistTest, maf_comsa_listNumberOfListInstances)
{
// MafReturnT maf_comsa_listNumberOfListInstances(uint32_t* numberOfLinkListInstances);
	MafReturnT res = MafFailure;
	uint32_t numberOfLinkListInstances = 0;
	// Success
	res = maf_comsa_listNumberOfListInstances(&numberOfLinkListInstances);
	EXPECT_EQ(res, MafOk);
	EXPECT_EQ(numberOfLinkListInstances, 1);
	// Error
	res = maf_comsa_listNumberOfListInstances(NULL);
	EXPECT_EQ(res, MafInvalidArgument);
}

TEST (RlistTest, maf_comSAMwComponentStart)
{
	MafReturnT res = MafFailure;

	// return OK
	res = maf_comSAMwComponentStart(MafActivating);
	EXPECT_EQ(res, MafOk);

	// return FAILURE
	res = maf_comSAMwComponentStart(MafActivating);
	EXPECT_EQ(res, MafOk);
	// return FAILURE
	res = maf_comSAMwComponentStart(MafActivating);
	EXPECT_EQ(res, MafOk);

}

TEST (RlistTest, maf_comSAMwComponentStop)
{
	// return OK
	MafReturnT res = MafFailure;
	res = maf_comSAMwComponentStop(MafDeactivating);
	EXPECT_EQ(res, MafOk);

	// return Failure
	res = maf_comSAMwComponentStop(MafDeactivating);
	EXPECT_EQ(res, MafOk);
}

// Declare functions for the MafMgmtSpiInterfacePortal_3T
MafReturnT rListGetInterface( MafMgmtSpiInterface_1T interfaceId, MafMgmtSpiInterface_1T **result)
{
	return MafOk;
}

MafReturnT rListGetInterfaceArray(const char *interface, const char * version, MafMgmtSpiInterface_1T ***result)
{
	return MafOk;
}

MafReturnT rListRegisterComponent(MafMgmtSpiComponent_2T *component)
{
	return MafOk;
}

MafReturnT rListUnregisterComponent(MafMgmtSpiComponent_2T *component)
{
	return MafOk;
}

TEST (RlistTest, maf_comSAMwComponentInitialize)
{
	MafReturnT res = MafFailure;
	xmlDocPtr config;
	MafMgmtSpiInterfacePortal_3T portal;
	portal.getInterface = &rListGetInterface;
	portal.getInterfaceArray = &rListGetInterfaceArray;
	portal.registerComponent = &rListRegisterComponent;
	portal.unregisterComponent = &rListUnregisterComponent;
	setenv("LDA_DEPLOYMENT","1",1);
	res = maf_comSAMwComponentInitialize(&portal, config);
	EXPECT_EQ(res, MafOk);
}

TEST (RlistTest, maf_comSAMwComponentFinalize)
{
	MafReturnT res = MafOk;
	MafMgmtSpiInterfacePortal_3T portal;
	portal.getInterface = &rListGetInterface;
	portal.getInterfaceArray = &rListGetInterfaceArray;
	portal.registerComponent = &rListRegisterComponent;
	portal.unregisterComponent = &rListUnregisterComponent;

	maf_comSAMwComponentFinalize(&portal);
	EXPECT_EQ(res, MafOk);
}


TEST (RlistTest, maf_comsa_listMemoryUsage)
{
// MafReturnT maf_comsa_listMemoryUsage(uint32_t* memoryUsed, uint32_t* totalMemoryAvailable)
	MafReturnT res = MafFailure;
	uint32_t memoryUsed = 0;
	uint32_t totalMemoryAvailable = 0;
	res = maf_comsa_listMemoryUsage(&memoryUsed, &totalMemoryAvailable);
	EXPECT_EQ(res, MafOk);
	EXPECT_EQ(memoryUsed, 10);
	EXPECT_EQ(totalMemoryAvailable, 200);
}

TEST (RlistTest, readShmmaxValue)
{
/* unsigned long long readShmmaxValue(); */
	unsigned long long ret = 0;
	FILE* pConfigFile = NULL;

	if ((pConfigFile = fopen(shmmaxFile,"r")) != NULL)
	{
		DEBUG_MWSA_REPLIST("file opened: (%s)",shmmaxFile);
		char buf[SA_MAX_UNEXTENDED_NAME_LENGTH];
		if (fgets(buf,sizeof(buf),pConfigFile) != NULL)
		{
			// convert the value. If value is greater then ULLONG_MAX then ULLONG_MAX will be the return value
			ret = strtoull(buf, NULL, 0);
			DEBUG_MWSA_REPLIST("config value: (%llu)",ret);
		}
		fclose(pConfigFile);
		DEBUG_MWSA_REPLIST("file closed: (%s)",shmmaxFile);
	}
	else
	{
		DEBUG_MWSA_REPLIST("no such file: (%s)",shmmaxFile);
	}


	unsigned long long res = readShmmaxValue();
	EXPECT_EQ(res, ret);
}

TEST (RlistTest, set_rlist_maxsize)
{
	xmlNode cfg;
	set_rlist_maxsize(&cfg);
}

TEST (RlistTest, set_clearAlarmsOnClusterReboot)
{
	xmlNode cfg;
	xmlAttr *a = (xmlAttr *)malloc(sizeof(xmlAttr));
	xmlNode *child = new xmlNode;
	char *value = "true";

	a->children = child;
	cfg.properties = a;

	//case true
	a->children->content = (unsigned char*)value;
	set_clearAlarmsOnClusterReboot(&cfg);
	EXPECT_TRUE(clearAlarmsOnClusterReboot);

	//case false
	value = "false";
	a->children->content = (unsigned char*)value;
	set_clearAlarmsOnClusterReboot(&cfg);
	EXPECT_FALSE(clearAlarmsOnClusterReboot);

	delete(child);
	free(a);
}

TEST (RlistTest, set_replicateInIMM)
{
	//case true
	setenv("LDA_DEPLOYMENT","1",1);
	set_replicateInIMM();
	EXPECT_TRUE(replicateInIMM);

	// resetting replicateInIMM variable to false
	replicateInIMM = false;

	//case false
	setenv("LDA_DEPLOYMENT","0",1);
	set_replicateInIMM();
	EXPECT_FALSE(replicateInIMM);
}

extern "C" char* createPath(const char** pathArray, const int nNumStrings, const char *fileName);
extern char pathToStorageDirectory[MAX_PATH_DATA_LENGTH];
TEST (RlistTest, CLEAR_SYSTEM)
{
	// This is the LATEST testcase in RlistTest_imm. Just for clearing all created directory.
	// Each TC need to clear the /rplist/"subfolder" itself. This TC remove "rplist" folder only.
	int return_c = 0;
	extern char strRpListPath[];

	const char *pathArray[4];
	pathArray[0] = pathToStorageDirectory;
	pathArray[1] = COMSA_FOR_COREMW_DIR;
	pathArray[2] = strRpListPath;
	char* dir = createPath(pathArray, 3, NULL);
	DEBUG_MWSA_REPLIST("CLEAR_SYSTEM():dir is  %s", dir);
	return_c = rmdir(dir);
	EXPECT_EQ(return_c, 0);
	free(dir);

	dir = createPath(pathArray, 2, NULL);
	DEBUG_MWSA_REPLIST("CLEAR_SYSTEM():dir is  %s", dir);
	return_c = rmdir(dir);
	EXPECT_EQ(return_c, 0);
	free(dir);
}
