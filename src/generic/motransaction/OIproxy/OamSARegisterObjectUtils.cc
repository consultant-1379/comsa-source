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
 *   File:	OamSARegisterObjectData.cc
 *
 *   Author: uabjoy
 *
 *   Date: 2011-09-19
 *
 *   This file implements the Object Implementer inside ComSA which shall forward
 *   IMM callbacks to their mapped counterparts in the COM OIs
 *
 *   Modify: emilzor  2012-04-14
 *
 *   Reviewed: efaiami 2012-04-21
 *   Modified: uabjoy  2012-05-30 Changed so the method imm2MO_DN is replaced with Imm2MO_DNRoot
 *   Modified: xnikvap 2012-08-30 Support for COM MO SPI Ver.3 (Ver.1 is not supported any more)
 *   Modified: xjonbuc 2012-09-06 Implement SDP1694 - support MAF SPI
 *   Modified: uabjoy  2012-11-20 Changed so the method imm2MO_DN is used for translation
 *   Modified: eaparob 2012-11-22 Function "parsePartialImmDN" added
 *   Modified: xadaleg 2014-08-02 MR35347 - increase DN length
 *   Modified: xnikvap 2015-08-02 MR42277 Remove old interfaces exposed to COM & CoreMW that are no longer supported for SLES12
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <list>

#include "MafMgmtSpiCommon.h"
#include "MafMgmtSpiInterface_1.h"
#include "MafOamSpiModelRepository_1.h"
#include "MafOamSpiRegisterObjectImplementer_2.h"
#include "MafOamSpiServiceIdentities_1.h"
#include "MafMgmtSpiInterfacePortal_3.h"

#include "ComSA.h"
#include "OamSACache.h"
#include "OamSATranslator.h"
#include "trace.h"
#include "OamSARegisterObjectUtils.h"

extern MafMgmtSpiInterfacePortal_3T* InterfacePortal;
extern OamSATranslator theTranslator;
// Handle to the object access API
extern SaImmAccessorHandleT accessorHandleOI;

extern MafOamSpiModelRepository_1T* theModelRepo_v1_p;

std::map<std::string, RegisteredClassStructT *> registeredClass;								//	<className, mocPath>
std::map<std::string, std::vector<RegisteredCTClassStructT *> > registeredComplexTypeClass;		//	<immClassNameOfComplexType, vector<path:complexTypeAttribute> >
std::map<std::string, RegisteredDNStructT *> registeredDN;										// 	<DN, dnMoc>
std::map<SaImmOiCcbIdT, MafOamSpiTransactionHandleT> ccbTxMap;									//
std::map<SaImmOiCcbIdT, SaAisErrorT> ccbApplierError;


const char *COM_ATTR_TYPE[] = {
	"UNKNOWN",
	"INT8",
	"INT16",
	"INT32",
	"INT64",
	"UINT8",
	"UINT16",
	"UINT32",
	"UINT64",
	"STRING",
	"BOOL",
	"REFERENCE",
	"ENUM",
	"DERIVED",
	"STRUCT",
	"VOID"
};

void dumpAttribute(char *attrName, MafMoAttributeValueContainer_3T *cnt) {
	ENTER_OIPROXY();
	if(!attrName || !cnt) {
		DEBUG_OIPROXY("@@@\t\t\t\t\attrName(%p) or cnt(%p) are NULL", attrName, cnt);
		LEAVE_OIPROXY();
		return;
	}

	if(cnt->nrOfValues > 1) {
		for(unsigned int i=0; i<cnt->nrOfValues; i++) {
			if(cnt->type == MafOamSpiMoAttributeType_3_STRING)
				DEBUG_OIPROXY("@@@ DUMP @@@\t\t%s:%s[%u]: %s", attrName, COM_ATTR_TYPE[cnt->type], i, cnt->values[i].value.theString);
			else
				DEBUG_OIPROXY("@@@ DUMP @@@\t\t%s:%s[%u]: %ld", attrName, COM_ATTR_TYPE[cnt->type], i, cnt->values[i].value.i64);
		}
	} else {
		if(cnt->type == MafOamSpiMoAttributeType_3_STRING)
			DEBUG_OIPROXY("@@@ DUMP @@@\t\t%s:%s: %s", attrName, COM_ATTR_TYPE[cnt->type], cnt->values[0].value.theString);
		else
			DEBUG_OIPROXY("@@@ DUMP @@@\t\t%s:%s: %ld", attrName, COM_ATTR_TYPE[cnt->type], cnt->values[0].value.i64);
	}
	LEAVE_OIPROXY();
}

void dumpStructAttr(char *attrName, MafMoAttributeValueContainer_3T *cnt) {
	ENTER_OIPROXY();
	if(!attrName || !cnt)
	{
		LEAVE_OIPROXY();
		return;
	}

	DEBUG_OIPROXY("@@@ DUMP @@@\t%s[%d]", attrName, cnt->nrOfValues);
	MafMoAttributeValueStructMember_3T *structMember;
	for(unsigned int i=0; i<cnt->nrOfValues; i++) {
		structMember = cnt->values[i].value.structMember;
		if(!structMember) {
			DEBUG_OIPROXY("@@@\t\t\t\t\tstructMember is NULL");
		}

		while(structMember) {
			dumpAttribute(structMember->memberName, structMember->memberValue);
			structMember = structMember->next;
		}
	}
	LEAVE_OIPROXY();
}

SaAisErrorT getAllClassDNs(SaImmHandleT immHandle, const char *className, std::list<std::string> &immDnList) {
	ENTER_OIPROXY();
	SaImmSearchOptionsT options = SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_NO_ATTR;
	SaStringT classNames[] = { (char *)className, NULL };
	SaImmSearchParametersT_2 params = (SaImmSearchParametersT_2){ { (SaImmAttrNameT)"SaImmAttrClassName", SA_IMM_ATTR_SASTRINGT, (SaImmAttrValueT)classNames } };
	SaImmSearchHandleT searchHandle;
	SaAisErrorT err;
	if((err = saImmOmSearchInitialize_2(immHandle, NULL, SA_IMM_SUBTREE, options, &params, NULL, &searchHandle)) != SA_AIS_OK)
	{
		LEAVE_OIPROXY();
		return err;
	}

	SaNameT objectName;
	SaImmAttrValuesT_2 **attributes = NULL;
	std::string temp;
	while(saImmOmSearchNext_2(searchHandle, &objectName, &attributes) == SA_AIS_OK) {
		temp.clear();
		temp.append(saNameGet(&objectName), saNameLen(&objectName));
		immDnList.push_back(temp);
	}

	err = saImmOmSearchFinalize(searchHandle);
	LEAVE_OIPROXY();
	return err;
}

MafOamSpiModelRepository_1T *getModelRepository() {
	ENTER_OIPROXY();
	DEBUG_OIPROXY("getModelRepository: InterfacePortal = %p, theModelRepo_v1_p = %p", InterfacePortal, theModelRepo_v1_p);

	if(InterfacePortal && !theModelRepo_v1_p) {
			MafOamSpiModelRepository_1T* modelRepository = NULL;
			if(InterfacePortal->getInterface(MafOamSpiModelRepository_1Id, (MafMgmtSpiInterface_1T**)&modelRepository) == MafOk)
				theModelRepo_v1_p = modelRepository;
	}
	DEBUG_OIPROXY("getModelRepository: Returning theModelRepo_v1_p = %p", theModelRepo_v1_p);
	LEAVE_OIPROXY();
	return theModelRepo_v1_p;
}

MafOamSpiMocT *findComClass(MafOamSpiMocT *moc, const char *className) {
	ENTER_OIPROXY();
	if(!moc || !className) {
		LEAVE_OIPROXY();
		return NULL;
	}

	if(!strcmp(moc->generalProperties.name, className)) {
		LEAVE_OIPROXY();
		return moc;
	}

	MafOamSpiMocT *retMoc = NULL;
	MafOamSpiContainmentT *child = moc->childContainment;
	while(child) {
		if((retMoc = findComClass(child->childMoc, className)) != NULL) {
			LEAVE_OIPROXY();
			return retMoc;
		}

		child = child->nextContainmentSameParentMoc;
	}
	LEAVE_OIPROXY();
	return NULL;
}

MafOamSpiMocT *findComMocPathList(MafOamSpiMocT *moc, const char *className, OamSACache::MocPathList &mocPathList) {
	ENTER_OIPROXY();
	if(!moc || !className) {
		return NULL;
	}

	if(!strcmp(moc->generalProperties.name, className)) {
		mocPathList.push_front(className);
		LEAVE_OIPROXY();
		return moc;
	}

	MafOamSpiMocT *retMoc = NULL;
	MafOamSpiContainmentT *child = moc->childContainment;
	DEBUG_OIPROXY("findComMocPathList: child = %p", child);
	while(child) {
		if(child->childMoc == moc) {
			//	I have this case in the test that moc == child ??? Is this correct ?
			DEBUG_OIPROXY("findComMocPathList: moc == child");
			LEAVE_OIPROXY();
			return NULL;
		}
		if((retMoc = findComMocPathList(child->childMoc, className, mocPathList)) != NULL) {
			mocPathList.push_front(moc->generalProperties.name);
			LEAVE_OIPROXY();
			return retMoc;
		}

		child = child->nextContainmentSameParentMoc;
		DEBUG_OIPROXY("findComMocPathList: next child = %p", child);
	}
	DEBUG_OIPROXY("findComMocPathList: after the while loop - returning NULL");
	LEAVE_OIPROXY();
	return NULL;
}

MafOamSpiMocT *findComMoc(MafOamSpiMocT *moc, const OamSACache::MocPathList &mocPathList) {
	ENTER_OIPROXY();
	if(!moc || mocPathList.size() == 0) {
		LEAVE_OIPROXY();
		return NULL;
	}

	OamSACache::MocPathList::const_iterator it = mocPathList.begin();
	if(it == mocPathList.end()) {
		LEAVE_OIPROXY();
		return moc;
	}

	if(strcmp(moc->generalProperties.name, (*it).c_str())) {
		LEAVE_OIPROXY();
		return NULL;
	}
	it++;
	bool found = true;
	MafOamSpiContainmentT *child;
	for(; it != mocPathList.end() && found; it++) {
		child = moc->childContainment;
		while(child) {
			if(child->childMoc) {
				if(!strcmp(child->childMoc->generalProperties.name, (*it).c_str())) {
					moc = child->childMoc;
					break;
				}
			} else {
				found = false;
				break;
			}
			child = child->nextContainmentSameParentMoc;
		}

		if(!child) {
			found = false;
			break;
		}
	}
	LEAVE_OIPROXY();
	return found ? moc : NULL;
}

void getMocComplexAttributes(MafOamSpiMocT *moc, std::vector<MafOamSpiMoAttributeT *> &attrList) {
	ENTER_OIPROXY();
	if(moc) {
		MafOamSpiMoAttributeT *attr = moc->moAttribute;
		while(attr) {
			if(attr->type == MafOamSpiMoAttributeType_STRUCT)
				attrList.push_back(attr);
			attr = attr->next;
		}
	}
	LEAVE_OIPROXY();
}

MafOamSpiMoAttributeT *getMocAttribute(const MafOamSpiMocT *moc, const char *attrName) {
	ENTER_OIPROXY();
	if(moc && attrName) {
		MafOamSpiMoAttributeT *attr = moc->moAttribute;
		while(attr) {
			if(!strcmp(attrName, attr->generalProperties.name)) {
				LEAVE_OIPROXY();
				return attr;
			}
			attr = attr->next;
		}
	}
	LEAVE_OIPROXY();
	return NULL;
}

const char *getComplexTypeImmClass(MafOamSpiMoAttributeT *attr) {
	ENTER_OIPROXY();
	if(!attr) {
		LEAVE_OIPROXY();
		return NULL;
	}
	LEAVE_OIPROXY();
	return (attr->type == MafOamSpiMoAttributeType_STRUCT) ? attr->structDatatype->generalProperties.name : NULL;
}

//	moDn
const char *getMoComplexTypeImmClass(char *moDn, char *ctAttrName) {
	ENTER_OIPROXY();
	if(!moDn || !ctAttrName) {
		LEAVE_OIPROXY();
		return NULL;
	}
	OamSACache::DNList moSplitDn;
	GlobalSplitDN(moDn, moSplitDn);
	MafOamSpiMocT *moc = theTranslator.GetComMoc(moSplitDn);
	MafOamSpiMoAttributeT *attr = getMocAttribute(moc, ctAttrName);
	LEAVE_OIPROXY();
	return attr ? attr->structDatatype->generalProperties.name : NULL;
}

bool getFragmentKey(std::string &fragment, std::string &key) {
	ENTER_OIPROXY();
	try {
		size_t pos = fragment.find_first_of('.');
		if(pos == fragment.npos)
		{
			pos = fragment.find('=');
		}

		if(pos == fragment.npos) {
			LEAVE_OIPROXY();
			return false;
		}
		key.clear();
		key.append(fragment.c_str(), pos);
	} catch (...)
	{
		ERR_OIPROXY("getFragmentKey received faulty IMM name");
	}
	LEAVE_OIPROXY();
	return true;
}


bool parseImmDN(const char *immDn, std::string &moDn, std::string &ctAttr, int *index) {
	ENTER_OIPROXY();
	DEBUG_OIPROXY("parseImmDN: immDn = '%s', moDn = '%s', ctAttr = '%s'", immDn, moDn.c_str(), ctAttr.c_str());

	if(!immDn) {
		LEAVE_OIPROXY();
		return false;
	}
	ctAttr.clear();
	char *dn;
	if(strstr(immDn, "id=") == immDn) {
		DEBUG_OIPROXY("parseImmDN: complexType");
		dn = (char *)strchr(immDn, ',');
		if(!dn) {
			LEAVE_OIPROXY();
			return false;
		}
		dn++;
		char *t = (char *)immDn + 3;
		while(*t && *t != '_' && t < dn)
			t++;
		if(*t != '_') {	//	It is not of complex type format
			LEAVE_OIPROXY();
			return false;
		}

		ctAttr.append(immDn + 3, ((int)(t - immDn)) - 3);

		t++;
		if(*t < '0' || *t > '9') {	//	Not an index
			LEAVE_OIPROXY();
			return false;
		}

		if(index)
			*index = strtol(t, NULL, 10);
	} else {
		DEBUG_OIPROXY("parseImmDN: not a complexType");
		dn = (char *)immDn;
		if(index)
			*index = -1;
	}

	OamSACache::DNList immSplitDn;
	GlobalSplitDN(dn, immSplitDn);
	DEBUG_OIPROXY("parseImmDN: after the 1st call to 	GlobalSplitDN(dn, immSplitDn)");
	// In this call we do not want the full path back, but only the MO path from the root and downwards
	if(!theTranslator.Imm2MO_DN(0,immSplitDn, moDn)) {
		LEAVE_OIPROXY();
		return false;
	}

	DEBUG_OIPROXY("parseImmDN: after a call to theTranslator.Imm2MO_DNRoot(immSplitDn, moDn)");
	DEBUG_OIPROXY("parseImmDN: moDn = '%s', ctAttr = '%s'", moDn.c_str(), ctAttr.c_str());

	LEAVE_OIPROXY();
	return true;
}

bool parsePartialImmDN(const char *immDn, std::string &moDn)
{
	ENTER_OIPROXY();
	DEBUG_OIPROXY("parsePartialImmDN(): enter with immDn=(%s)", immDn);

	if(!immDn)
	{
		LEAVE_OIPROXY();
		return false;
	}

	OamSACache::DNList immSplitDn;
	GlobalSplitDN((char *)immDn, immSplitDn);

	// In this call we want the full DN back
	if(!theTranslator.Imm2MO_DN(0, immSplitDn, moDn))
	{
		DEBUG_OIPROXY("parsePartialImmDN(): return false");
		LEAVE_OIPROXY();
		return false;
	}
	DEBUG_OIPROXY("parsePartialImmDN(): return true with moDn=(%s)",moDn.c_str());
	LEAVE_OIPROXY();
	return true;
}

bool getCtAttrFromImmDN(const char *immDn, std::string &ctAttr, int *index)
{
	ENTER_OIPROXY();
	if(!immDn)
	{
		DEBUG_OIPROXY("getCtAttrFromImmDN return false: immDn missing");
		LEAVE_OIPROXY();
		return false;
	}

	ctAttr.clear();
	char *dn;

	if(strstr(immDn, "id=") == immDn)
	{
		dn = (char *)strchr(immDn, ',');
		if(dn == NULL)
		{
			DEBUG_OIPROXY("getCtAttrFromImmDN return false: dn missing");
			LEAVE_OIPROXY();
			return false;
		}
		dn++;
		char *t = (char *)immDn + 3;
		while(*t && *t != '_' && t < dn)
		{
			t++;
		}
		if(*t != '_')
		{	//	It is not of complex type format
			DEBUG_OIPROXY("getCtAttrFromImmDN return false: Not complex type format");
			LEAVE_OIPROXY();
			return false;
		}

		DEBUG_OIPROXY("getCtAttrFromImmDN: immDn=(%s) pos=(%d)",immDn+3, ((int)(t - immDn)) - 3);
		ctAttr.append(immDn + 3, ((int)(t - immDn)) - 3);

		t++;
		if(*t < '0' || *t > '9')
		{	//	Not an index
			DEBUG_OIPROXY("getCtAttrFromImmDN return false: Not an index");
			LEAVE_OIPROXY();
			return false;
		}

		if(index != NULL)
		{
			*index = strtol(t, NULL, 10);
		}
	}
	else
	{
		if(index != NULL)
		{
			*index = -1;
		}
	}
	DEBUG_OIPROXY("getCtAttrFromImmDN return true");
	LEAVE_OIPROXY();
	return true;
}

bool createMocPath(const char *moDn, std::string &mocPath) {
	ENTER_OIPROXY();
	if(!moDn) {
		LEAVE_OIPROXY();
		return false;
	}

	OamSACache::DNList moSplitDn;
	GlobalSplitDN(moDn, moSplitDn);

	mocPath.clear();

	OamSACache::DNList::iterator it = moSplitDn.begin();
	if(it == moSplitDn.end()) {
		LEAVE_OIPROXY();
		return false;
	}

	std::string classStr;
	if(!getFragmentKey(*it, classStr)) {
		LEAVE_OIPROXY();
		return false;
	}

	MafOamSpiMocT *mocRoot = NULL;
	DEBUG_OIPROXY("createMocPath: calling getModelRepository()");
	getModelRepository()->getTreeRoot(&mocRoot);

	OamSACache::MocPathList mocPathList;
	if(!findComMocPathList(mocRoot, classStr.c_str(), mocPathList)) {
		LEAVE_OIPROXY();
		return false;
	}

	char *t;
	OamSACache::MocPathList::iterator iter=mocPathList.begin();
	for(; iter != mocPathList.end();) {
		t = (char *)(*iter).c_str();
		if(++iter == mocPathList.end())
			break;
		mocPath.append("/").append(t);
	}
	// it++;
	size_t pos;
	for(OamSACache::DNList::iterator it = moSplitDn.begin(); it != moSplitDn.end(); it++) {
		pos = (*it).find('.', 0);
		if (pos == (*it).npos) {
			pos = (*it).find('=', 0);
		}
		if(pos == (*it).npos) {
			LEAVE_OIPROXY();
			return false;
		}
		mocPath.append("/").append((*it).c_str(), pos);
	}
	LEAVE_OIPROXY();
	return true;
}


MafReturnT addNewRegisteredClass(MafMgmtSpiInterface_1T managedObjectInterfaceId, MafMgmtSpiInterface_1T transactionalResourceId,
								const char *className, const char *mocPath) {
	ENTER_OIPROXY();
	if(!className) {
		LEAVE_OIPROXY();
		return MafFailure;
	}
	std::map<std::string, RegisteredClassStructT *>::iterator it = registeredClass.find(className);
	if(it == registeredClass.end())
	{
		RegisteredClassStructT *classStruct = (RegisteredClassStructT *)malloc(sizeof(RegisteredClassStructT));
		memset(classStruct, 0, sizeof(RegisteredClassStructT));

		classStruct->managedObjectInterfaceId.componentName = strdup(managedObjectInterfaceId.componentName);
		classStruct->managedObjectInterfaceId.interfaceName = strdup(managedObjectInterfaceId.interfaceName);
		classStruct->managedObjectInterfaceId.interfaceVersion = strdup(managedObjectInterfaceId.interfaceVersion);
		classStruct->transactionalResourceId.componentName = strdup(transactionalResourceId.componentName);
		classStruct->transactionalResourceId.interfaceName = strdup(transactionalResourceId.interfaceName);
		classStruct->transactionalResourceId.interfaceVersion = strdup(transactionalResourceId.interfaceVersion);
		classStruct->mocPath = strdup(mocPath);

		registeredClass[className] = classStruct;
	}
	else
	{
		LEAVE_OIPROXY();
		return MafAlreadyExist;
	}
	LEAVE_OIPROXY();
	return MafOk;
}

bool removeRegisteredClass(const char *className) {
	ENTER_OIPROXY();
	bool ret = true;
	if(className) {
		std::map<std::string, RegisteredClassStructT *>::iterator it = registeredClass.find(className);
		if(it != registeredClass.end()) {
			RegisteredClassStructT *classStruct = (*it).second;

			free((char *)classStruct->managedObjectInterfaceId.componentName);
			free((char *)classStruct->managedObjectInterfaceId.interfaceName);
			free((char *)classStruct->managedObjectInterfaceId.interfaceVersion);
			free((char *)classStruct->transactionalResourceId.componentName);
			free((char *)classStruct->transactionalResourceId.interfaceName);
			free((char *)classStruct->transactionalResourceId.interfaceVersion);
			free(classStruct->mocPath);
			free(classStruct);

			registeredClass.erase(className);
		}
		else
			ret = false;
	}
	else
		ret = false;

	LEAVE_OIPROXY();
	return ret;
}

RegisteredClassStructT *getRegisteredClass(const char *className) {
	ENTER_OIPROXY();

	RegisteredClassStructT *classStruct = NULL;
	std::map<std::string, RegisteredClassStructT *>::iterator it = registeredClass.find(className);
	if(it != registeredClass.end())
		classStruct = (*it).second;

	LEAVE_OIPROXY();
	return classStruct;
}

const char *getRegisteredClassMocPath(const char *className) {
	ENTER_OIPROXY();
	const char *mocPath = NULL;
	std::map<std::string, RegisteredClassStructT *>::iterator it = registeredClass.find(className);
	if(it != registeredClass.end())
		mocPath = (*it).second->mocPath;

	LEAVE_OIPROXY();
	return mocPath;
}

bool existRegisteredClass(const char *className) {
	ENTER_OIPROXY();
	std::map<std::string, RegisteredClassStructT *>::iterator it = registeredClass.find(className);
	LEAVE_OIPROXY();
	return it != registeredClass.end();
}


//	path can be one of next cases:
//	path = <mocPath> ':' <attributeName>
//	path = <dn> ':' <attributeName>
MafReturnT addNewRegisteredCTClass(MafMgmtSpiInterface_1T managedObjectInterfaceId, MafMgmtSpiInterface_1T transactionalResourceId,
									const char *immClassName, const char *path) {
	ENTER_OIPROXY();
	if(!immClassName || !path) {
		LEAVE_OIPROXY();
		return MafFailure;
	}
	MafReturnT ret = MafOk;
	std::map<std::string, std::vector<RegisteredCTClassStructT *> >::iterator it = registeredComplexTypeClass.find(immClassName);
	if(it == registeredComplexTypeClass.end()) {
		RegisteredCTClassStructT *ctStruct = (RegisteredCTClassStructT *)malloc(sizeof(RegisteredCTClassStructT));
		memset(ctStruct, 0, sizeof(RegisteredCTClassStructT));

		ctStruct->managedObjectInterfaceId.componentName = strdup(managedObjectInterfaceId.componentName);
		ctStruct->managedObjectInterfaceId.interfaceName = strdup(managedObjectInterfaceId.interfaceName);
		ctStruct->managedObjectInterfaceId.interfaceVersion = strdup(managedObjectInterfaceId.interfaceVersion);
		ctStruct->transactionalResourceId.componentName = strdup(transactionalResourceId.componentName);
		ctStruct->transactionalResourceId.interfaceName = strdup(transactionalResourceId.interfaceName);
		ctStruct->transactionalResourceId.interfaceVersion = strdup(transactionalResourceId.interfaceVersion);
		ctStruct->path = strdup(path);

		registeredComplexTypeClass[immClassName].push_back(ctStruct);
	} else {
		for(size_t i=0; i<(*it).second.size(); i++) {
			if(!strcmp((*it).second[i]->path, path)) {
				ret = MafAlreadyExist;
				break;
			}
		}

		//	New registered complex type does not exists in the map with the same type, so it will be added to the map
		if(ret == MafOk) {
			RegisteredCTClassStructT *ctStruct = (RegisteredCTClassStructT *)malloc(sizeof(RegisteredCTClassStructT));
			memset(ctStruct, 0, sizeof(RegisteredCTClassStructT));

			ctStruct->managedObjectInterfaceId.componentName = strdup(managedObjectInterfaceId.componentName);
			ctStruct->managedObjectInterfaceId.interfaceName = strdup(managedObjectInterfaceId.interfaceName);
			ctStruct->managedObjectInterfaceId.interfaceVersion = strdup(managedObjectInterfaceId.interfaceVersion);
			ctStruct->transactionalResourceId.componentName = strdup(transactionalResourceId.componentName);
			ctStruct->transactionalResourceId.interfaceName = strdup(transactionalResourceId.interfaceName);
			ctStruct->transactionalResourceId.interfaceVersion = strdup(transactionalResourceId.interfaceVersion);
			ctStruct->path = strdup(path);

			registeredComplexTypeClass[immClassName].push_back(ctStruct);
		}
	}

	LEAVE_OIPROXY();
	return ret;
}

MafReturnT removeRegisteredCTClass(const char *immClassName, const char *path) {
	ENTER_OIPROXY();

	if(!immClassName || !path) {
		LEAVE_OIPROXY();
		return MafFailure;
	}

	RegisteredCTClassStructT *ctStruct;
	MafReturnT ret = MafNotExist;
	std::map<std::string, std::vector<RegisteredCTClassStructT *> >::iterator it = registeredComplexTypeClass.find(immClassName);
	if(it != registeredComplexTypeClass.end()) {
		for(size_t i=0; i<(*it).second.size(); i++) {
			ctStruct = (*it).second[i];
			if(!strcmp(ctStruct->path, path)) {
				free((char *)ctStruct->managedObjectInterfaceId.componentName);
				free((char *)ctStruct->managedObjectInterfaceId.interfaceName);
				free((char *)ctStruct->managedObjectInterfaceId.interfaceVersion);
				free((char *)ctStruct->transactionalResourceId.componentName);
				free((char *)ctStruct->transactionalResourceId.interfaceName);
				free((char *)ctStruct->transactionalResourceId.interfaceVersion);
				free(ctStruct->path);
				free(ctStruct);
				registeredComplexTypeClass[immClassName].erase(registeredComplexTypeClass[immClassName].begin() + i);
				break;
			}
		}

		if((*it).second.size() == 0)
			registeredComplexTypeClass.erase(immClassName);
	}

	LEAVE_OIPROXY();
	return ret;
}

RegisteredCTClassStructT *getRegisteredCTClass(const char *immClassName, const char *path) {
	ENTER_OIPROXY();
	if(!immClassName || !path) {
		LEAVE_OIPROXY();
		return NULL;
	}

	RegisteredCTClassStructT *ctStruct = NULL;
	std::map<std::string, std::vector<RegisteredCTClassStructT *> >::iterator it = registeredComplexTypeClass.find(immClassName);
	if(it != registeredComplexTypeClass.end()) {
		for(size_t i=0; i<(*it).second.size(); i++) {
			ctStruct = (*it).second[i];
			if(!strcmp((*it).second[i]->path, path)) {
				ctStruct = (*it).second[i];
				break;
			}
		}
	}
	LEAVE_OIPROXY();
	return ctStruct;
}

bool existRegisteredCTClass(const char *immClassName) {
	ENTER_OIPROXY();
	std::map<std::string, std::vector<RegisteredCTClassStructT *> >::iterator it = registeredComplexTypeClass.find(immClassName);
	LEAVE_OIPROXY();
	return it != registeredComplexTypeClass.end();
}

bool existRegisteredCTClassAttr(const char *immClassName, const char *path) {
	ENTER_OIPROXY();
	if(!immClassName || !path) {
		LEAVE_OIPROXY();
		return false;
	}

	bool ret = false;

	std::map<std::string, std::vector<RegisteredCTClassStructT *> >::iterator it = registeredComplexTypeClass.find(immClassName);
	if(it != registeredComplexTypeClass.end()) {
		for(size_t i=0; i<(*it).second.size(); i++) {
			//	Pointers should be same, since both pointers are coming from the same model repository
			if(!strcmp((*it).second[i]->path, path)) {
				ret = true;
				break;
			}
		}
	}
	LEAVE_OIPROXY();
	return ret;
}

MafReturnT addNewRegisteredDN(MafMgmtSpiInterface_1T managedObjectInterfaceId, MafMgmtSpiInterface_1T transactionalResourceId,
								const char *dn, MafOamSpiMocT *moc) {
	ENTER_OIPROXY();
	if(!dn || !moc) {
		LEAVE_OIPROXY();
		return MafFailure;
	}
	MafReturnT ret;
	std::map<std::string, RegisteredDNStructT *>::iterator it = registeredDN.find(dn);
	if(it == registeredDN.end())
	{
		RegisteredDNStructT *dnStruct = (RegisteredDNStructT *)malloc(sizeof(RegisteredDNStructT));
		memset(dnStruct, 0, sizeof(RegisteredDNStructT));

		dnStruct->managedObjectInterfaceId.componentName = strdup(managedObjectInterfaceId.componentName);
		dnStruct->managedObjectInterfaceId.interfaceName = strdup(managedObjectInterfaceId.interfaceName);
		dnStruct->managedObjectInterfaceId.interfaceVersion = strdup(managedObjectInterfaceId.interfaceVersion);
		dnStruct->transactionalResourceId.componentName = strdup(transactionalResourceId.componentName);
		dnStruct->transactionalResourceId.interfaceName = strdup(transactionalResourceId.interfaceName);
		dnStruct->transactionalResourceId.interfaceVersion = strdup(transactionalResourceId.interfaceVersion);
		dnStruct->dnMoc = moc;

		registeredDN[dn] = dnStruct;
		ret = MafOk;
	}
	else
		ret = MafNotExist;
	LEAVE_OIPROXY();
	return ret;
}

MafReturnT removeRegisteredDN(const char *dn) {
	ENTER_OIPROXY();
	if(!dn) {
		LEAVE_OIPROXY();
		return MafFailure;
	}

	MafReturnT ret;
	std::map<std::string, RegisteredDNStructT *>::iterator it = registeredDN.find(dn);
	if(it != registeredDN.end())
	{
		RegisteredDNStructT *dnStruct = (*it).second;

		free((char *)dnStruct->managedObjectInterfaceId.componentName);
		free((char *)dnStruct->managedObjectInterfaceId.interfaceName);
		free((char *)dnStruct->managedObjectInterfaceId.interfaceVersion);
		free((char *)dnStruct->transactionalResourceId.componentName);
		free((char *)dnStruct->transactionalResourceId.interfaceName);
		free((char *)dnStruct->transactionalResourceId.interfaceVersion);
		free(dnStruct);

		registeredDN.erase(dn);
		ret = MafOk;
	}
	else
		ret = MafNotExist;

	LEAVE_OIPROXY();
	return ret;
}

RegisteredDNStructT *getRegisteredDN(const char *dn) {
	ENTER_OIPROXY();
	RegisteredDNStructT *moc;
	std::map<std::string, RegisteredDNStructT *>::iterator it = registeredDN.find(dn);
	if(it != registeredDN.end())
		moc = (*it).second;
	else
		moc = NULL;

	LEAVE_OIPROXY();
	return moc;
}

bool existRegisteredDN(const char *dn) {
	ENTER_OIPROXY();
	std::map<std::string, RegisteredDNStructT *>::iterator it = registeredDN.find(dn);
	LEAVE_OIPROXY();
	return it != registeredDN.end();
}

MafReturnT addNewCcbTransaction(SaImmOiCcbIdT ccbId, MafOamSpiTransactionHandleT tx) {
	ENTER_OIPROXY();
	MafReturnT ret;
	std::map<SaImmOiCcbIdT, MafOamSpiTransactionHandleT>::iterator it = ccbTxMap.find(ccbId);
	if(it == ccbTxMap.end()) {
		ccbTxMap[ccbId] = tx;
		ret = MafOk;
	} else
		ret = MafAlreadyExist;
	LEAVE_OIPROXY();
	return ret;
}

MafReturnT removeCcbTransaction(SaImmOiCcbIdT ccbId) {
	ENTER_OIPROXY();
	LEAVE_OIPROXY();
	return (ccbTxMap.erase(ccbId) > 0) ? MafOk : MafNotExist;
}

bool getCcbTransaction(SaImmOiCcbIdT ccbId, MafOamSpiTransactionHandleT *txHandle) {
	ENTER_OIPROXY();
	std::map<SaImmOiCcbIdT, MafOamSpiTransactionHandleT>::iterator it = ccbTxMap.find(ccbId);
	bool ret;
	if(it != ccbTxMap.end()) {
		*txHandle = (*it).second;
		ret = true;
	} else
		ret = false;
	LEAVE_OIPROXY();
	return ret;
}

bool existCcbTransaction(SaImmOiCcbIdT ccbId) {
	ENTER_OIPROXY();
	std::map<SaImmOiCcbIdT, MafOamSpiTransactionHandleT>::iterator it = ccbTxMap.find(ccbId);
	LEAVE_OIPROXY();
	return it != ccbTxMap.end();
}

void setCcbApplierError(SaImmOiCcbIdT ccbId, SaAisErrorT err) {
	ENTER_OIPROXY();
	ccbApplierError[ccbId] = err;
	LEAVE_OIPROXY();
}

SaAisErrorT getCcbApplierError(SaImmOiCcbIdT ccbId) {
	ENTER_OIPROXY();
	std::map<SaImmOiCcbIdT, SaAisErrorT>::iterator it = ccbApplierError.find(ccbId);
	LEAVE_OIPROXY();
	return (it != ccbApplierError.end()) ? (*it).second : SA_AIS_OK;
}

void removeCcbApplierError(SaImmOiCcbIdT ccbId) {
	ENTER_OIPROXY();
	ccbApplierError.erase(ccbId);
	LEAVE_OIPROXY();
}
