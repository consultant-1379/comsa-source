/******************************************************************************
*   Copyright (C) 2018 by Ericsson AB
*   S - 125 26  STOCKHOLM, SWEDEN
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
*   File:   Reencryptor_ImmUtil_dummy.cc
*
*   Author: xsrikpu
*
*   Date:   2018-10-26
*
*   This file implements the helper functions to use when contacting IMM.
*   The design is heavily influenced by this file's previous and larger version
*   which was implemented by CoreMw.
*
*   Modify: xsrikpu 2018-10-26  MR58438 - APG to CBA - Security enhancements - COM
*
***********************************************************************************/

#include "Reencryptor_CmwUtil_dummy.h"

#include "saAis.h"
#include "saImmOi.h"
#include "saImmOm.h"
#include <unistd.h>
#include <cstring>
#include <string>
#include <list>

std::list<GetNextCache> AccessorList;
typedef std::list<GetNextCache>::iterator AccessorListIterT;

void add2Accessor(std::string obj, std::string attr, SaImmAttrValuesT_2** vp, SaImmValueTypeT t)
{
	AccessorList.push_back(static_cast<GetNextCache>(std::string(obj + "_" + attr)));
	GetNextCache & addedItem = *(AccessorList.rbegin());
	addedItem.type = t;
	addedItem.value = vp;
}

std::list<SearchNextCache> SearchList;
typedef std::list<SearchNextCache>::iterator SearchListIterT;
std::string searchNextKey;

void add2Search(std::string root, std::string className, ObjectQueueT objs)
{
	SearchList.push_back(static_cast<SearchNextCache>(std::string(root + "_" + className)));
	SearchNextCache & addedItem = *(SearchList.rbegin());
	addedItem.objects = objs;
}

std::list<CcbCache> CcbCacheList;
typedef std::list<CcbCache>::iterator CcbCacheListIter;

void add2Ccb(std::string obj, std::string attr, int val)
{
	std::string key = obj + "_" + attr;
	CcbCacheListIter it = CcbCacheList.begin();
	while (it != CcbCacheList.end()) {
		if (key == it->obj_attr) {
			it->i = val;
			return;
		}
		it++;
	}

	CcbCacheList.push_back(static_cast<CcbCache>(key));
	CcbCache & addedItem = *(CcbCacheList.rbegin());
	addedItem.i = val;
}


void add2Ccb(std::string obj, std::string attr, std::string val)
{
	std::string key = obj + "_" + attr;
	CcbCacheListIter it = CcbCacheList.begin();
	while (it != CcbCacheList.end()) {
		if (key == it->obj_attr) {
			it->str = val;
			return;
		}
		it++;
	}

	CcbCacheList.push_back(static_cast<CcbCache>(key));
	CcbCache & addedItem = *(CcbCacheList.rbegin());
	addedItem.str = val;
}

bool assertCcbValue(std::string obj, std::string attr, std::string value)
{
	std::string key = obj + "_" + attr;
	CcbCacheListIter it = CcbCacheList.begin();
	while (it != CcbCacheList.end()) {
		if (key == it->obj_attr) {
		return (value == it->str);
		}
		it++;
	}
	return false;
}

bool assertCcbValue(std::string obj, std::string attr, int value)
{
	std::string key = obj + "_" + attr;
	CcbCacheListIter it = CcbCacheList.begin();
	while (it != CcbCacheList.end()) {
		if (key == it->obj_attr) {
		return (value == it->i);
		}
		it++;
	}
	return false;
}

void resetAllCaches()
{
	AccessorList.clear();
	SearchList.clear();
	searchNextKey.clear();
	CcbCacheList.clear();
}

static struct AutoCleanT
{
	char** errmsgs;

	~AutoCleanT()
	{
		if (errmsgs) {
			delete[] errmsgs;
			errmsgs = NULL;
		}
	}
} AutoClean;

SaAisErrorT saLogDispatch(SaLogHandleT logHandle, SaDispatchFlagsT dispatchFlags)
{
	return SA_AIS_OK;
}

SaAisErrorT	saLogStreamOpen_2(SaLogHandleT logHandle,
			   const SaNameT *logStreamName,
			   const SaLogFileCreateAttributesT_2 *logFileCreateAttributes,
			   SaLogStreamOpenFlagsT logStreamOpenFlags,
			   SaTimeT timeout, SaLogStreamHandleT *logStreamHandle){
	return SA_AIS_OK;
}

SaAisErrorT saLogInitialize(SaLogHandleT *logHandle, const SaLogCallbacksT *callbacks, SaVersionT *version){
	return SA_AIS_OK;
}

SaAisErrorT saLogSelectionObjectGet(SaLogHandleT logHandle, SaSelectionObjectT *selectionObject){
	return SA_AIS_OK;
}

SaAisErrorT saLogStreamClose(SaLogStreamHandleT logStreamHandle){
	return SA_AIS_OK;
}

SaAisErrorT saLogFinalize(SaLogHandleT logHandle){
	return SA_AIS_OK;
}

SaAisErrorT saLogWriteLogAsync(SaLogStreamHandleT logStreamHandle, SaInvocationT invocation, SaLogAckFlagsT ackFlags, const SaLogRecordT *logRecord){
	return SA_AIS_OK;
}

/* ----------------------------------------------------------------------
* IMM call wrappers; This wrapper interface off loads the burden to
* handle return values and retries for each and every IMM-call. It
* makes the code cleaner.
*/

SaAisErrorT saImmOiInitialize_rc = SA_AIS_OK;
SaImmOiCallbacksT_2* sImmOiApplierCallbacks;
SaAisErrorT saImmOiInitialize_2(SaImmOiHandleT *immOiHandle,
	const SaImmOiCallbacksT_2 *immOiCallbacks,
	SaVersionT *version)
{
	*immOiHandle = 69;
	sImmOiApplierCallbacks = const_cast<SaImmOiCallbacksT_2*>(immOiCallbacks);
	return saImmOiInitialize_rc;
}


SaAisErrorT saImmOiSelectionObjectGet(SaImmOiHandleT immOiHandle, SaSelectionObjectT *selectionObject)
{
	*selectionObject = 69;
	return SA_AIS_OK;
}

SaAisErrorT saImmOiClassImplementerSet(SaImmOiHandleT immOiHandle, const SaImmClassNameT className)
{
	return SA_AIS_OK;
}

SaAisErrorT saImmOiClassImplementerRelease(SaImmOiHandleT immOiHandle, const SaImmClassNameT className)
{
	return SA_AIS_OK;
}

SaAisErrorT saImmOiImplementerSet(SaImmOiHandleT immOiHandle, const SaImmOiImplementerNameT implementerName)
{
	return SA_AIS_OK;
}

SaAisErrorT saImmOiImplementerClear(SaImmOiHandleT immOiHandle)
{
	return SA_AIS_OK;
}

SaAisErrorT saImmOiFinalize_rc = SA_AIS_OK;
SaAisErrorT saImmOiFinalize(SaImmOiHandleT immOiHandle)
{
	return saImmOiFinalize_rc;
}

SaAisErrorT saImmOmInitialize_rc = SA_AIS_OK;
SaAisErrorT saImmOmInitialize(SaImmHandleT *immHandle,
		const SaImmCallbacksT *immCallbacks, SaVersionT *version)
{
	*immHandle = 69;
	return saImmOmInitialize_rc;
}

SaAisErrorT saImmOmSearchInitialize_2_rc = SA_AIS_ERR_FAILED_OPERATION;
SaAisErrorT saImmOmSearchInitialize_2(SaImmHandleT immHandle,
		const SaNameT *rootName,
		SaImmScopeT scope,
		SaImmSearchOptionsT searchOptions,
		const SaImmSearchParametersT_2 *searchParam,
		const SaImmAttrNameT *attributeNames, SaImmSearchHandleT *searchHandle)
{
	std::string parentObj, className;
	if (rootName) {
		const TheSaNameT* objName = reinterpret_cast<const TheSaNameT*>(rootName);
		parentObj = std::string((char*)objName->value, static_cast<size_t>(objName->length));
	}
	className = std::string(*(char**)(searchParam->searchOneAttr.attrValue));
	TRACE("ENTER saImmOmSearchInitialize_2 parentObj[%s], className[%s]\n", parentObj.c_str(), className.c_str());

	std::string key = parentObj + "_" + className;
	SearchListIterT it = SearchList.begin();
	while (it != SearchList.end()) {
		TRACE("saImmOmSearchInitialize_2 Iterating over [%s]\n", it->root_class.c_str());
		if (it->root_class == key) {
			searchNextKey = it->root_class;
			TRACE("saImmOmSearchInitialize_2 Search key found [%s]\n", searchNextKey.c_str());
			break;
		}
		it++;
	}

	if (it != SearchList.end()) {
		*searchHandle = 69;
		return SA_AIS_OK;
	}

	return saImmOmSearchInitialize_2_rc;
}

SaAisErrorT saImmOmSearchNext_2(SaImmSearchHandleT searchHandle, SaNameT *objectName, SaImmAttrValuesT_2 ***attributes)
{
	TRACE("ENTER saImmOmSearchNext_2\n");

	std::string retStr;
	SearchListIterT it = SearchList.begin();
	while (it != SearchList.end()) {
		if (it->root_class == searchNextKey) {
			if (it->objects.empty()) {
				return SA_AIS_ERR_NOT_EXIST;
			}
			else {
				retStr = it->objects.front();
				TRACE("saImmOmSearchNext_2 Found obj[%s]\n", retStr.c_str());
				it->objects.pop();
			}
			break;
		}
		it++;
	}

	if (it == SearchList.end()) {
		return SA_AIS_ERR_NOT_EXIST;
	}

	TheSaNameT* tempSaName = reinterpret_cast<TheSaNameT*>(objectName);
	memset(tempSaName, 0, sizeof(TheSaNameT));
	tempSaName->length = retStr.length();
	memcpy(tempSaName->value, retStr.c_str(), static_cast<unsigned long int>(tempSaName->length));

	searchHandle = 69;
	*attributes = NULL;
	return SA_AIS_OK;
}

SaAisErrorT saImmOmSearchFinalize(SaImmSearchHandleT searchHandle)
{
	TRACE("ENTER saImmOmSearchFinalize\n");
	searchNextKey.clear();
	return SA_AIS_OK;
}

SaAisErrorT saImmOmFinalize_rc = SA_AIS_OK;
SaAisErrorT saImmOmFinalize(SaImmHandleT immHandle)
{
	return saImmOmFinalize_rc;
}


SaAisErrorT saImmAccessFinalize_rc = SA_AIS_OK;
/*
* Finalize the Object Access API towards the IMM database.
*
* @return	A SaAisErrorT value that in turn is from the IMM, see the IMM documentation for details.
*/
SaAisErrorT saImmOmAccessorFinalize(SaImmAccessorHandleT accessorHandle)
{
	return saImmAccessFinalize_rc;
}

/*
* Initialize the Object Access API towards the IMM database.
*
* @return	A SaAisErrorT value that in turn is from the IMM, see the IMM documentation for details.
*/
SaAisErrorT saImmAccessInitialize_rc = SA_AIS_OK;
SaAisErrorT saImmOmAccessorInitialize(SaImmHandleT immHandle,
		SaImmAccessorHandleT *accessorHandle)
{
	if (0 == immHandle) {
		return SA_AIS_ERR_BAD_HANDLE;
	}
	*accessorHandle = 69;
	return saImmAccessInitialize_rc;
}

/*
* Request the IMM for a value of a specific attribute of a special object.
*
* @return	A SaAisErrorT enum that in turn is from the IMM, see the IMM documentation for details.
*/
SaAisErrorT saImmOmAccessorGet_2_rc = SA_AIS_OK;
SaImmAttrValuesT_2** attrValues_Returned = NULL;
SaAisErrorT saImmOmAccessorGet_2(SaImmAccessorHandleT accessorHandle,
		const SaNameT *objectName,
		const SaImmAttrNameT *attributeNames,
		SaImmAttrValuesT_2 ***attributes)
{
	if (0 == accessorHandle) {
		return SA_AIS_ERR_BAD_HANDLE;
	}

	const TheSaNameT* objName = reinterpret_cast<const TheSaNameT*>(objectName);
	std::string obj((char*)objName->value, static_cast<size_t>(objName->length));
	std::string attr((char*)attributeNames[0]);
	TRACE("ENTER saImmOmAccessorGet_2 Obj[%s], Attribute[%s]\n", obj.c_str(), attr.c_str());

	std::string key = std::string(obj + "_" + attr);
	AccessorListIterT it = AccessorList.begin();
	while (it != AccessorList.end()) {

		if (it->obj_attr == key) {
			TRACE("FOUND %s, type %d\n", it->obj_attr.c_str(), it->type);
			break;
		}
		it++;
	}

	if (it == AccessorList.end()) {
		return SA_AIS_ERR_NOT_EXIST;
	}

	*attributes = it->value;

	return saImmOmAccessorGet_2_rc;

}

SaAisErrorT saImmOmAdminOwnerInitialize(SaImmHandleT immHandle,
				     const SaImmAdminOwnerNameT adminOwnerName,
				     SaBoolT releaseOwnershipOnFinalize, SaImmAdminOwnerHandleT *ownerHandle)
{
	*ownerHandle = 69;
	return SA_AIS_OK;
}

SaAisErrorT saImmOmAdminOwnerFinalize(SaImmAdminOwnerHandleT ownerHandle)
{
	return SA_AIS_OK;
}

SaAisErrorT saImmOmAdminOwnerSet(SaImmAdminOwnerHandleT ownerHandle, const SaNameT **objectNames, SaImmScopeT scope)
{
	return SA_AIS_OK;
}

SaAisErrorT saImmOmAdminOwnerRelease(SaImmAdminOwnerHandleT ownerHandle, const SaNameT **objectNames, SaImmScopeT scope)
{
	return SA_AIS_OK;
}

SaAisErrorT saImmOmCcbFinalize(SaImmCcbHandleT ccbHandle)
{
	return SA_AIS_OK;
}

SaAisErrorT saImmOmCcbInitialize(SaImmAdminOwnerHandleT ownerHandle, SaImmCcbFlagsT ccbFlags, SaImmCcbHandleT *ccbHandle)
{
	*ccbHandle = 69;
	return SA_AIS_OK;
}

SaAisErrorT saImmOiDispatch(SaImmOiHandleT immOiHandle, SaDispatchFlagsT dispatchFlags) {
	return SA_AIS_OK;
}

bool receivedObjectCreate = false;
SaAisErrorT saImmOmCcbObjectCreate_2(SaImmCcbHandleT ccbHandle,
				  const SaImmClassNameT className,
				  const SaNameT *parentName, const SaImmAttrValuesT_2 **attrValues)
{
	TRACE("ENTER saImmOmCcbObjectCreate_2\n");
	receivedObjectCreate = true;
	return SA_AIS_OK;
}

SaAisErrorT ccbModifyRetVal = SA_AIS_OK;
std::string ccbModifyObj;
bool receivedCcbModify = false;
SaAisErrorT saImmOmCcbObjectModify_2(SaImmCcbHandleT ccbHandle, const SaNameT *objectName, const SaImmAttrModificationT_2 **attrMods)
{
	receivedCcbModify = true;
	const TheSaNameT* objName = reinterpret_cast<const TheSaNameT*>(objectName);
	std::string obj((char*)objName->value, static_cast<size_t>(objName->length));

	if ((SA_AIS_ERR_FAILED_OPERATION == ccbModifyRetVal)
		&& (ccbModifyObj == obj)) {
		TRACE ("saImmOmCcbObjectModify_2(): returning failed operation for %s\n", obj.c_str());
		ccbModifyRetVal = SA_AIS_OK;
		return SA_AIS_ERR_FAILED_OPERATION;
	}

	TRACE("ENTER saImmOmCcbObjectModify_2 [%s]\n", obj.c_str());

	if (attrMods) {

		for (int i = 0; attrMods[i]; i++) {

			std::string attr((char*)attrMods[i]->modAttr.attrName);
			TRACE("ENTER saImmOmCcbObjectModify_2 attr [%s]\n", attr.c_str());

			if (attrMods[i]->modAttr.attrValuesNumber) {

				SaImmAttrValuesT_2 modAttr = attrMods[i]->modAttr;
				SaImmAttrValueT attrValues = modAttr.attrValues[0];
				if (attrValues) {

					if (SA_IMM_ATTR_SASTRINGT == modAttr.attrValueType) {
						add2Ccb(obj, attr, std::string(*(char**)(attrValues)));
					}
					else if (SA_IMM_ATTR_SAINT32T == modAttr.attrValueType) {
						add2Ccb(obj, attr, static_cast<int>(*((SaUint32T *)(attrValues))));
					}
				}
			}
		}
	}
	return SA_AIS_OK;
}

SaAisErrorT saImmOmCcbAbort(SaImmCcbHandleT ccbHandle)
{
	return SA_AIS_OK;
}

SaAisErrorT saImmOmCcbValidate(SaImmCcbHandleT ccbHandle)
{
	return SA_AIS_OK;
}

SaAisErrorT saImmOmCcbApply(SaImmCcbHandleT ccbHandle)
{
	return SA_AIS_OK;
}

SaAisErrorT saImmOmCcbGetErrorStrings(SaImmCcbHandleT ccbHandle, const SaStringT **errorStrings)
{
	static std::string theError = "I'm Batman.";
	AutoClean.errmsgs = new char*[2];
	AutoClean.errmsgs[0] = const_cast<char*>(theError.c_str());
	AutoClean.errmsgs[1] = NULL;
	*errorStrings = AutoClean.errmsgs;
	return SA_AIS_OK;
}
