#ifndef COMSARLIST_IMM_H
#define COMSARLIST_IMM_H
/*******************************************************************************
* Copyright (C) 2015 by Ericsson AB
* S - 125 26  STOCKHOLM
* SWEDEN, tel int + 46 10 719 0000
*
* The copyright to the computer program herein is the property of
* Ericsson AB. The program may be used and/or copied only with the
* written permission from Ericsson AB, or in accordance with the terms
* and conditions stipulated in the agreement/contract under which the
* program has been supplied..
*
* All rights reserved.
*
* Author: xadaleg
*
* Date:   2015-02-17
*
* This file declares the required functions for implementing the replicated list
* in IMM and files.
*
* Modify: xnikvap 2015-04-22  Corection for TR TR HT65567: IMM Accessor handle leak and SPI return codes
*
*******************************************************************************/
#include "ComSA.h"
#include "imm_utils.h"

// use to enable some debug code. TO BE COMMENTED OUT FOR PRODUCTION !!!
//#define RPLIST_DEBUG_CODE_ENABLE

extern bool replicateInIMM;

#define CLUSTER_RESTORED 1
#define CLUSTER_REBOOTED_ONLY 2
#define CLUSTER_RT_OBJ_EXIST 3

#ifdef __cplusplus
extern "C" {

namespace RlistUtil {

	/* Utilities */
	bool validateName(const MafMwSpiListNameT* name);
	void classToObj(const char* className, SaNameT* rdnName);
	MafReturnT findNextListItem(const MafMwSpiListNameT* listInstanceName, const int currentItemName, char* nextListItemName, unsigned int* intItemName, SaImmAttrValuesT_2*** attrValues);
	void comsa_rplist_imm_debugPrintObject(SaImmAttrValuesT_2 **returnedAttrValues);
	MafReturnT comsa_rplist_imm_deleteDir(const char* subDir);
	char* createPath(const char** pathArray, const int nNumStrings, const char *fileName);
	void unsetFromEnvironment(const char *ptr);
	SaImmAttrDefinitionT_2** prepareImmAttrDefinitions();
	SaImmAttrDefinitionT_2** prepareRTImmAttrDefinitions();
	void freeImmAttrDefinitions(SaImmAttrDefinitionT_2*** attrDef);
	MafReturnT convertToRTObjectName(const std::string& listInstanceName, const std::string& lastId, SaNameT** immObject);
	MafReturnT getVoidData(const SaImmAttrValuesT_2** attrVal, void** data, unsigned int dataLen);
	MafReturnT reset_rplistImmOiHandle();

}
	/* IMM Wrapper Functions */
#ifdef UNIT_TEST
	MafReturnT comsa_rplist_imm_init(SaImmHandleT *immHandle);
	void comsa_rplist_imm_finalize(SaImmHandleT *immHandle);
	MafReturnT comsa_rplist_immOi_init(SaImmHandleT *immHandle);
	void comsa_rplist_immOi_finalize(SaImmHandleT *immHandle);
	MafReturnT comsa_rplist_immAccess_init(const SaImmHandleT immHandle, SaImmAccessorHandleT *immAccessorHandle);
	void comsa_rplist_immAccess_finalize(SaImmAccessorHandleT immAccessorHandle);
	MafReturnT comsa_rplist_immOi_set(void);

	// IMM Class Functions
	MafReturnT createclass(const char* className, uint32_t dataBufferSize, bool createRunTimeClass = false);
	MafReturnT deleteclass(const char* className);
	MafReturnT searchclass(const char* className);

	// IMM Object Functions
	MafReturnT createObject(const char* className, const SaImmAttrValuesT_2 **attrValues);
	MafReturnT readObject(const SaNameT* immObject, SaImmAttrValuesT_2 ***attrValues);
	MafReturnT modifyObject(const SaNameT *objectName, const SaImmAttrModificationT_2 **attrValues);
	MafReturnT deleteObject(const SaNameT *objectName);
	MafReturnT setObjectUiAttribute(const char* className, const char* attrName, unsigned int attrValue);
	MafReturnT getObjectUiAttribute(const char* className, const char* attrName, unsigned int* uiAttrValPtr);
	MafReturnT addRlistRTObject(const std::string& pClassName, const unsigned int& lastId, const void* newItemDataBuffer);
	MafReturnT modifyRlistRTObject(const std::string& pClassName, const std::string& lastId, const void* newItemDataBuffer);
	MafReturnT getAllRTObjects(const char* className, std::vector<std::string>& immRTObjects);
	MafReturnT getRTObjectsCount(const char* className , uint32_t& rlistObjCnt);
	MafReturnT deleteRTObject(const std::string& listInstanceName, const std::string& listItemName);
	MafReturnT deleteAllRTObjects(const char* className);
	// File Functions
	MafReturnT createDir(const char* fileName, const char* path);
	MafReturnT writeToFile(const char* fileName, const char* subDir, const void *data, const int dataSize);
	MafReturnT readFromFile(const char* fileName, const char* subDir, void *data, const int dataSize);
	MafReturnT findFile(const char* fileName, const char* subDir);
	MafReturnT removeFile(const char* fileName, const char* subDir);
	MafReturnT getBaseRepository();
	MafReturnT detectClusterState(const MafMwSpiListNameT* listInstanceName, unsigned int* stateFlag);
	MafReturnT validateImmData(const MafMwSpiListNameT* listInstanceName, const char* dirName);
	MafReturnT restore_immData(const MafMwSpiListNameT* listInstanceName);
	MafReturnT autoRetry_writeToFile(const MafMwSpiListNameT* listInstanceName, const MafMwSpiListItemNameT* listItemName, const void* replaceItemData);
	MafReturnT autoRetry_readFromFile(const char* fileName, const char* subDir, void *data, const int dataSize);
	MafReturnT comsa_rplist_imm_removeAllFiles(const char* subDir);
#endif

class OamSARList
{

	OamSARList();
public:

	~OamSARList();
	static OamSARList& instance()
	{
		static OamSARList s_instance;
		return s_instance;
	}

	/* Startup */
	MafReturnT maf_comsa_imm_comSAMwComponentStart(void);
	MafReturnT maf_comsa_imm_comSAMwComponentStop(void);

	MafReturnT listCreate(const MafMwSpiListNameT* listInstanceName, uint32_t dataBufferSize);
	MafReturnT listPushBack(const MafMwSpiListNameT* listInstanceName, MafMwSpiListItemNameT* listItemName, /* OUT parameter */ void* newItemDataBuffer);
	MafReturnT listDelete(const MafMwSpiListNameT* listInstanceName);
	MafReturnT listClear(const MafMwSpiListNameT* listInstanceName);
	MafReturnT listGetSize(const MafMwSpiListNameT* listInstanceName, uint32_t* listSize);
	MafReturnT listIsEmpty(const MafMwSpiListNameT* listInstanceName, bool* listEmpty);
	MafReturnT listPopBack(const MafMwSpiListNameT* listInstanceName);
	MafReturnT listEraseItem(const MafMwSpiListNameT* listInstanceName, const MafMwSpiListItemNameT* listItemName);
	MafReturnT listGetFrontRef(const MafMwSpiListNameT* listInstanceName, MafMwSpiListItemRefT* listInstanceFrontRef);
	MafReturnT listGetFinalize(const MafMwSpiListNameT* listInstanceName, const MafMwSpiListItemRefT currentItemRef);
	MafReturnT listGetNextItemFront(const MafMwSpiListNameT* listInstanceName, MafMwSpiListItemRefT* currentItemRef, MafMwSpiListItemNameT* listItemName, void* copyOfItemData);
	MafReturnT listFindItem(const MafMwSpiListNameT* listInstanceName, const MafMwSpiListItemNameT* listItemName, void* copyOfItemData);
	MafReturnT listReplaceItem(const MafMwSpiListNameT* listInstanceName, const MafMwSpiListItemNameT* listItemName, const void* replaceItemData);
	MafReturnT listNumberOfListInstances(uint32_t* numberOfLinkListInstances);
	MafReturnT listMemoryUsage(uint32_t* memoryUsed, uint32_t* totalMemoryAvailable);

};

}
#endif

#ifdef __cplusplus
extern "C" {
#endif


/* Startup */
MafReturnT maf_comsa_imm_comSAMwComponentStart(void);
MafReturnT maf_comsa_imm_comSAMwComponentStop(void);

MafReturnT maf_comsa_imm_listCreate(const MafMwSpiListNameT* listInstanceName, uint32_t dataBufferSize);
MafReturnT maf_comsa_imm_listPushBack(const MafMwSpiListNameT* listInstanceName, MafMwSpiListItemNameT* listItemName, /* OUT parameter */ void* newItemDataBuffer);
MafReturnT maf_comsa_imm_listDelete(const MafMwSpiListNameT* listInstanceName);
MafReturnT maf_comsa_imm_listClear(const MafMwSpiListNameT* listInstanceName);
MafReturnT maf_comsa_imm_listGetSize(const MafMwSpiListNameT* listInstanceName, uint32_t* listSize);
MafReturnT maf_comsa_imm_listIsEmpty(const MafMwSpiListNameT* listInstanceName, bool* listEmpty);
MafReturnT maf_comsa_imm_listPopBack(const MafMwSpiListNameT* listInstanceName);
MafReturnT maf_comsa_imm_listEraseItem(const MafMwSpiListNameT* listInstanceName, const MafMwSpiListItemNameT* listItemName);
MafReturnT maf_comsa_imm_listGetFrontRef(const MafMwSpiListNameT* listInstanceName, MafMwSpiListItemRefT* listInstanceFrontRef);
MafReturnT maf_comsa_imm_listGetFinalize(const MafMwSpiListNameT* listInstanceName, const MafMwSpiListItemRefT currentItemRef);
MafReturnT maf_comsa_imm_listGetNextItemFront(const MafMwSpiListNameT* listInstanceName, MafMwSpiListItemRefT* currentItemRef, MafMwSpiListItemNameT* listItemName, void* copyOfItemData);
MafReturnT maf_comsa_imm_listFindItem(const MafMwSpiListNameT* listInstanceName, const MafMwSpiListItemNameT* listItemName, void* copyOfItemData);
MafReturnT maf_comsa_imm_listReplaceItem(const MafMwSpiListNameT* listInstanceName, const MafMwSpiListItemNameT* listItemName, const void* replaceItemData);
MafReturnT maf_comsa_imm_listNumberOfListInstances(uint32_t* numberOfLinkListInstances);
MafReturnT maf_comsa_imm_listMemoryUsage(uint32_t* memoryUsed, uint32_t* totalMemoryAvailable);


#ifdef __cplusplus
}
#endif

#endif /* not defined COMSARLIST_IMM_H */
