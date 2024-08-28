/**
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
*   File:   mafOI.cc
*
*   Author: uabjoy
*
*   Date:   2011-09-21
*
*   Unit test of the implementation of the OIproxy functionality, or expressed as SDP763.
*
*   Modified: xnikvap 2012-08-30  support for COM MO SPI Ver.3 (Ver.1 is not supported any more)
*
*   Modified: xadaleg  014-08-02  MR35347 - increase DN length
*/

#define IMM_A_02_01

#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <list>
#include <vector>
#include <map>
#include <ctype.h>

#include <assert.h>
#include <gtest/gtest.h>

// CoreMW
#include <saAis.h>
#include <saImm.h>
#include <saImmOm.h>
#include <saImmOi.h>
// COM
#include <MafMgmtSpiCommon.h>
#include <MafMgmtSpiComponent_1.h>
#include <MafMgmtSpiInterfacePortal_1.h>
#include <MafOamSpiModelRepository_1.h>
#include <MafOamSpiManagedObject_3.h>
#include <MafOamSpiRegisterObjectImplementer_2.h>
// COMSA
#include "OamSATransactionRepository.h"
#include "OamSACache.h"
#include "TxContext.h"
#include "ImmCmd.h"
#include "OamSATranslator.h"

// TESTS
#include "OamSAImmBridge.h"
#include "ParameterVerifier.h"
#include "StubIMM.h"
#include "MockProcessUtil.h"
#include "MockTransactionMasterIF2.h"
#include "OamSAOIProxy.h"

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

MafOamSpiModelRepository_4T* theModelRepo_v4_p = NULL;

ParameterVerifier* sParameterVerifier;

aaservice::MockProcessUtil *mymockProcessUtil;

static bool saImmOmCcbGetErrorStrings_accessed=false;
SaAisErrorT saImmOmCcbGetErrorStrings(SaImmCcbHandleT ccbHandle, const SaStringT **errorStrings)
{

	SaAisErrorT rc=SA_AIS_OK;
	DEBUG("OamSAImmBridge_unittest::saImmOmCcbGetErrorStrings \n");

	SaStringT* eStrings = new SaStringT[128];

	static char sKalle[] = "kalle";
	eStrings[0] = sKalle;

	static char sStina1[] = "stina1";
	eStrings[1] = sStina1;

	eStrings[2] = NULL;
	*errorStrings=eStrings;

	saImmOmCcbGetErrorStrings_accessed=true;

	return rc;

}

extern "C" int implementor_init(void)
{
	return 0;
}

/* called from AMF thread */
extern "C" int implementor_stop(void)
{
	return 0;
}

extern "C" int implementor_terminate()
{
	return 0;
}

// This is what we test

extern MafReturnT registerClass(MafMgmtSpiInterface_1T managedObjectInterfaceId, MafMgmtSpiInterface_1T transactionalResourceId, const char * mocPath);

extern MafReturnT registerDn(MafMgmtSpiInterface_1T managedObjectInterfaceId, MafMgmtSpiInterface_1T transactionalResourceId, const char * dn);

extern MafReturnT unregisterDn(MafMgmtSpiInterface_1T managedObjectInterfaceId, MafMgmtSpiInterface_1T transactionalResourceId, const char * dn);

extern MafReturnT unregisterClass(MafMgmtSpiInterface_1T managedObjectInterfaceId, MafMgmtSpiInterface_1T transactionalResourceId, const char * mocPath);

/**
* MOCK thing we do not use in the code.
*/

SaAisErrorT saImmOiRtObjectDelete(SaImmOiHandleT immOiHandle, const SaNameT *objectName)
{
	return SA_AIS_OK;
}

SaAisErrorT saImmOiRtObjectCreate_2(SaImmOiHandleT immOiHandle, const SaImmClassNameT className, const SaNameT *parentName, const SaImmAttrValuesT_2 **attrValues)
{
	return SA_AIS_OK;
}

/* SDP1694 -support MAF SPI */

MafOamSpiModelRepository_2T* maf_theModelRepo_v2_p = NULL;



//////////////////////////////////////////////////////
// Utilities
//////////////////////////////////////////////////////



SaAisErrorT saImmOmClassDescriptionMemoryFree_2(SaImmHandleT immHandle,
SaImmAttrDefinitionT_2 **attrDefinitions) {
	return SA_AIS_OK;
}

///////////////////////////
//STUB IMPL IMM
///////////////////////////'

// Globale Memory tracker!
ImmMemoryTracker immMemoryTrackerG;

// Global IMM storage!!
ImmStorage immStorageG;

//////////////////////////////////////////////////////
// Setup model repository
//////////////////////////////////////////////////////

/// Builders

/* SDP1694 -support MAF SPI */

struct MafOamSpiMom* maf_makeMom(const char* name, struct MafOamSpiMoc* rootMoc) {
	struct MafOamSpiMom* mom = new struct MafOamSpiMom;
	mom->generalProperties.name = (char*) name;
	mom->next = NULL;
	mom->rootMoc = rootMoc;
	mom->version = (char*) "Unit testing 101";
	mom->release = (char*) "BETA";
	rootMoc->mom = mom;
	return mom;
}

struct MafOamSpiMoc* maf_makeMoc(const char* name, struct MafOamSpiMoAttribute* attrList) {
	struct MafOamSpiMoc* moc = new struct MafOamSpiMoc;
	moc->generalProperties.name = (char*) name;
	moc->moAttribute = attrList;
	moc->parentContainment = NULL;
	moc->childContainment = NULL;
	return moc;
}

struct MafOamSpiMoc* maf_makeMoc(const char* name,
		struct MafOamSpiMoAttribute* attrList,
		struct MafOamSpiContainment* childContainment,
		struct MafOamSpiContainment* parentContainment) {
	struct MafOamSpiMoc* moc = new struct MafOamSpiMoc;
	moc->generalProperties.name = (char*) name;
	moc->moAttribute = attrList;
	moc->parentContainment = childContainment;
	moc->childContainment = parentContainment;
	return moc;
}

struct MafOamSpiMoc* maf_makeMoc(const char* name, struct MafOamSpiMoAttribute* attrList, std::string &momName) {
	struct MafOamSpiMoc* moc = new struct MafOamSpiMoc;
	moc->generalProperties.name = (char*) name;
	moc->moAttribute = attrList;
	moc->parentContainment = NULL;
	moc->childContainment = NULL;
	/*
	* set the mom general properties
	*/
	struct MafOamSpiMom *mom=new struct MafOamSpiMom;

	// Allocate memory for string plus a finishing /0
	char *tempPointer=new (char[momName.size()+1]);
	strcpy(tempPointer, momName.c_str());
	mom->generalProperties.name=tempPointer;
	moc->mom=mom;
	return moc;
}

struct MafOamSpiContainment* maf_makeContainment(MafOamSpiMocT* parent, MafOamSpiMocT* child, MafOamSpiContainmentT* nextSameParent, MafOamSpiContainmentT* nextSameChild) {
	struct MafOamSpiContainment* cont = new struct MafOamSpiContainment;
	cont->parentMoc = parent;
	cont->childMoc = child;
	cont->nextContainmentSameParentMoc = nextSameParent;
	cont->nextContainmentSameChildMoc = nextSameChild;
	return cont;
}

struct MafOamSpiContainment* maf_makeContainment(struct MafOamSpiMoc* parent, struct MafOamSpiMoc* child) {
	struct MafOamSpiContainment* cont = new struct MafOamSpiContainment;
	cont->parentMoc = parent;
	cont->childMoc = child;
	cont->nextContainmentSameParentMoc = NULL;
	cont->nextContainmentSameChildMoc = NULL;

	// update child and parent
	parent->childContainment = cont;
	child->parentContainment = cont;
	//parent->mom = mom;
	//child->mom = mom;
	return cont;
}

struct MafOamSpiMoAttribute* maf_makeAttribute(char* name, MafOamSpiMoAttributeTypeT type, struct MafOamSpiMoAttribute* next) {
	struct MafOamSpiMoAttribute* attr = new struct MafOamSpiMoAttribute;
	attr->generalProperties.name = (char*) name;
	attr->type = type;
	//attr->moc = moc;
	attr->next = next;
	return attr;
}

struct MafOamSpiMoAttribute* maf_makeAttribute(char* name, struct MafOamSpiStruct* structData, struct MafOamSpiMoAttribute* next) {
	struct MafOamSpiMoAttribute* attr = new struct MafOamSpiMoAttribute;
	attr->generalProperties.name = (char*) name;
	attr->generalProperties.description = (char*) "This is an attribute in UnitTesting";
	attr->generalProperties.specification = (char*) "A specification";
	attr->generalProperties.status = MafOamSpiStatus_CURRENT;
	attr->generalProperties.hidden = (char*) "The hidden!!";

	attr->type = MafOamSpiMoAttributeType_STRUCT; // implicit
	attr->structDatatype = structData;
	//attr->moc = moc;
	attr->next = next;
	return attr;
}

struct MafOamSpiStruct* maf_makeStruct(char* name, struct MafOamSpiStructMember* memberList) {
	struct MafOamSpiStruct* s = new struct MafOamSpiStruct;
	s->generalProperties.name = name;
	s->members = memberList;
	return s;
}

struct MafOamSpiStructMember* maf_makeStructMember(char* name, MafOamSpiDatatype type, struct MafOamSpiStructMember* next) {
	struct MafOamSpiStructMember* m = new struct MafOamSpiStructMember;
	m->generalProperties.name = name;
	m->memberType.type = type;
	m->next = next;
	return m;
}

// Export the global MOM pointer
//
//
static struct MafOamSpiMom* maf_theGlobalMom;

void maf_setMom(struct MafOamSpiMom* theMom) {
	maf_theGlobalMom = theMom;
}

//
// Interface SPI
//
static MafReturnT maf_getMoms(const MafOamSpiMomT ** result) {
	*result = (MafOamSpiMomT*) maf_theGlobalMom;
	return MafOk;
}

static MafReturnT maf_getMoc(const char* momName, const char* momVersion, const char* mocName, MafOamSpiMocT** result)
{
	return MafOk;
}

MafReturnT maf_getTreeRoot(const MafOamSpiMocT **result)
{
	*result=maf_theGlobalMom->rootMoc;
	return MafOk;
}

static MafOamSpiModelRepository_1T maf_InterfaceStruct = { { "", "", "1" }, maf_getMoms, maf_getMoc, maf_getTreeRoot };
MafOamSpiModelRepository_1T* theModelRepo_v1_p = &maf_InterfaceStruct;


//	keyAttributeNameReceived.clear();
//	keyAttributevalueReceived.clear();
//	parentDnReceived.clear();


#ifdef REDIRECT_LOG
////////////////////////////////////3//////////////////
// LOG
//////////////////////////////////////////////////////
#ifdef  __cplusplus
extern "C" {
#endif
#define LOG_PREFIX " "
	static void coremw_vlog2(int priority, char const* fmt, va_list ap) {
		char buffer[256];
		int len = strlen(LOG_PREFIX);
		strcpy(buffer, LOG_PREFIX);
		buffer[len] = ' ';
		len++;
		vsnprintf(buffer + len, sizeof(buffer) - len, fmt, ap);
		printf("DEBUG: %s\n", buffer);
	}

	void coremw_log(int priority, const char* fmt, ...) {
		va_list ap;
		va_start(ap, fmt);
		coremw_vlog2(priority, fmt, ap);
		va_end(ap);
	}
	void coremw_debug_log(int priority, const char* fmt, ...) {
		va_list ap;
		va_start(ap, fmt);
		coremw_vlog2(priority, fmt, ap);
		va_end(ap);
	}

#ifdef  __cplusplus
}
#endif

#endif //REDIRECT_LOG

//////////////////////////////////////////////////////
// IMM API
//////////////////////////////////////////////////////
SaAisErrorT
saImmOmSearchInitialize_2(SaImmHandleT immHandle,
		const SaNameT *rootName,
		SaImmScopeT scope,
		SaImmSearchOptionsT searchOptions,
		const SaImmSearchParametersT_2 *searchParam,
		const SaImmAttrNameT *attributeNames, SaImmSearchHandleT *searchHandle) {
	printf("----> saImmOmSearchInitialize_2 \n");
	return SA_AIS_OK;
}

/* 4.5.2 saImmOmSearchNext() */
SaAisErrorT
saImmOmSearchNext_2(SaImmSearchHandleT searchHandle, SaNameT *objectName, SaImmAttrValuesT_2 ***attributes) {
	printf("----> saImmOmSearchNext_2 \n");
	return SA_AIS_OK;

}

std::string receivedData_saImmOiRtObjectUpdate_2;

SaAisErrorT saImmOiRtObjectUpdate_2(SaImmOiHandleT immOiHandle, const SaNameT *objectName, const SaImmAttrModificationT_2 **attrMods) {
	printf("----> saImmOiRtObjectUpdate_2 \n");
	receivedData_saImmOiRtObjectUpdate_2=*(char**)(attrMods[0]->modAttr.attrValues[0]);
	return SA_AIS_OK;
}


/* OI sets error string in CCB related upcall. See: http://devel.opensaf.org/ticket/1904 */

// Received error strings saved here
std::list<std::string>	ReceivedErrorStrings;

SaAisErrorT saImmOiCcbSetErrorString( SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId, const SaStringT errorString) {
	printf("----> saImmOiCcbSetErrorString \n");

	ReceivedErrorStrings.push_front(std::string(errorString));
	return SA_AIS_OK;
}

SaAisErrorT
saImmOiDispatch(SaImmOiHandleT immOiHandle, SaDispatchFlagsT dispatchFlags) {
	printf("----> saImmOiDispatch \n");
	return SA_AIS_OK;
}

/*
* Gets the callbacks here ...
*/


SaImmOiCallbacksT_2 immOiCalls;
SaImmOiHandleT immOiHandleTEST = 1;

bool saImmOiInitialize_2_return_OK=true;
bool saImmOiInitialize_2_accessed=false;

SaAisErrorT saImmOiInitialize_2(SaImmOiHandleT *immOiHandle, const SaImmOiCallbacksT_2 *immOiCallbacks, SaVersionT *version) {
	cout << "----> saImmOiInitialize_2 \n" << endl;
	cout << "Register the call back functions now from the OmOi we test" << endl;
	immOiCalls.saImmOiAdminOperationCallback = immOiCallbacks->saImmOiAdminOperationCallback;
	immOiCalls.saImmOiCcbAbortCallback = immOiCallbacks->saImmOiCcbAbortCallback;
	immOiCalls.saImmOiCcbApplyCallback = immOiCallbacks->saImmOiCcbApplyCallback;
	immOiCalls.saImmOiCcbCompletedCallback = immOiCallbacks->saImmOiCcbCompletedCallback;
	immOiCalls.saImmOiCcbObjectCreateCallback = immOiCallbacks->saImmOiCcbObjectCreateCallback;
	immOiCalls.saImmOiCcbObjectDeleteCallback = immOiCallbacks->saImmOiCcbObjectDeleteCallback;
	immOiCalls.saImmOiCcbObjectModifyCallback = immOiCallbacks->saImmOiCcbObjectModifyCallback;
	immOiCalls.saImmOiRtAttrUpdateCallback = immOiCallbacks->saImmOiRtAttrUpdateCallback;
	immOiHandle = &immOiHandleTEST;
	saImmOiInitialize_2_accessed=true;
	if (saImmOiInitialize_2_return_OK) return SA_AIS_OK;

	if (!saImmOiInitialize_2_return_OK) return  SA_AIS_ERR_EXIST;

}


bool saImmOiImplementerSet_return_OK=true;
bool saImmOiImplementerSet_accessed=false;
SaAisErrorT
saImmOiImplementerSet(SaImmOiHandleT immOiHandle, const SaImmOiImplementerNameT implementerName) {
	saImmOiImplementerSet_accessed=true;
	printf("----> saImmOiImplementerSet \n");
	if (saImmOiImplementerSet_return_OK) return SA_AIS_OK; else return SA_AIS_ERR_EXIST;
}

SaAisErrorT saImmOiClassImplementerSetChoice = SA_AIS_OK;
std::string saImmOiClassImplementerSet_receivedRegistration;
SaAisErrorT
saImmOiClassImplementerSet(SaImmOiHandleT immOiHandle, const SaImmClassNameT className) {
	printf("----> saImmOiClassImplementerSet \n");
	saImmOiClassImplementerSet_receivedRegistration.clear();
	saImmOiClassImplementerSet_receivedRegistration=className;
	return saImmOiClassImplementerSetChoice;
}

SaAisErrorT
saImmOiFinalize(SaImmOiHandleT immOiHandle) {
	printf("----> saImmOiFinalize \n");
	return SA_AIS_OK;
}

SaAisErrorT
saImmOiAdminOperationResult_o2(SaImmOiHandleT immOiHandle, SaInvocationT invocation, SaAisErrorT result, const SaImmAdminOperationParamsT_2** returnParams) {
	printf("----> saImmOiAdminOperationResult_o2 \n");
	return SA_AIS_OK;
}

SaAisErrorT
saImmOiImplementerClear(SaImmOiHandleT immOiHandle) {
	printf("----> saImmOiImplementerClear \n");
	return SA_AIS_OK;
}

extern bool saImmOiSelectionObjectGet_return_OK;
extern bool saImmOiSelectionObjectGet_accessed;
SaAisErrorT
saImmOiSelectionObjectGet(SaImmOiHandleT immOiHandle, SaSelectionObjectT *selectionObject) {
	printf("----> saImmOiSelectionObjectGet \n");
	saImmOiSelectionObjectGet_accessed=true;
	if (saImmOiSelectionObjectGet_return_OK) return SA_AIS_OK; else return SA_AIS_ERR_EXIST;
}


std::string recievedData_saImmOiClassImplementerRelease;
SaAisErrorT saImmOiClassImplementerRelease_returnvalue = SA_AIS_OK;
int	returnCounter = 20;
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
	printf("----> saImmOiObjectImplementerSet \n");
	char tempBuff[SA_MAX_NAME_LENGTH];
	strncpy(tempBuff,(char *)objectName->value,objectName->length);
	tempBuff[objectName->length]='\0';
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
	recievedData_saImmOiObjectImplementerRelease = (char *)objectName->value;
	if (saImmOiObjectImplementerRelease_returnvalue == SA_AIS_ERR_TRY_AGAIN) {
		--returnCounter1;
		if (returnCounter1 == 0) saImmOiObjectImplementerRelease_returnvalue = SA_AIS_OK;
	}
	return saImmOiObjectImplementerRelease_returnvalue;
}



SaAisErrorT saImmOmSearchFinalize(SaImmSearchHandleT searchHandle) {
	printf("----> saImmOmSearchFinalize \n");
	return SA_AIS_OK;
}

SaAisErrorT saImmOmCcbFinalize(SaImmCcbHandleT ccbHandle) {
	printf("----> saImmOmCcbFinalize \n");
	return SA_AIS_OK;
}

SaAisErrorT saImmOmAdminOwnerFinalize(SaImmAdminOwnerHandleT ownerHandle) {
	printf("----> saImmOmAdminOwnerFinalize \n");
	return SA_AIS_OK;
}

SaAisErrorT saImmOmAccessorGet_2(SaImmAccessorHandleT accessorHandle, const SaNameT *objectName, const SaImmAttrNameT *attributeNames, SaImmAttrValuesT_2 ***attributes) {
	// used directly by cache
	printf("----> saImmOmAccessorGet_2 \n");

	char* tmp = new char[objectName->length + 1];
	memcpy(tmp, objectName->value, objectName->length);
	tmp[objectName->length] = 0;

	std::string dn(tmp);
	std::string attrName(attributeNames[0]);

	printf("saImmOmAccessorGet_2 dn=%s attrName=%s \n", dn.c_str(), attrName.c_str());

	CM::ImmCmdOmAccessorGet immGet(NULL, dn, attrName, attributes); // no need to run execute() on command, we stubbed this command!

	delete[] tmp;
	return SA_AIS_OK;
}

SaAisErrorT saImmOmFinalize(SaImmHandleT immHandle) {
	printf("----> saImmOmFinalize \n");
	return SA_AIS_OK;
}

bool saImmOmInitialize_return_OK=true;
bool saImmOmInitialize_accessed=false;

SaAisErrorT saImmOmInitialize(SaImmHandleT *immHandle, const SaImmCallbacksT *immCallbacks, SaVersionT *version) {
	saImmOmInitialize_accessed=true;
	printf("----> saImmOmInitialize \n");
	(*immHandle) = (SaImmHandleT) 1; // set to value != 0 to inidcate success!
	if (saImmOmInitialize_return_OK) return SA_AIS_OK; else return SA_AIS_ERR_EXIST;
}

SaAisErrorT saImmOmAccessorInitialize(SaImmHandleT immHandle, SaImmAccessorHandleT *accessorHandle) {
	printf("----> saImmOmAccessorInitialize \n");
	(*accessorHandle) = (SaImmAccessorHandleT) 1; // set to value != 0 to show success
	return SA_AIS_OK;
}

SaAisErrorT saImmOmAccessorFinalize(SaImmAccessorHandleT accessorHandle) {
	printf("----> saImmOmAccessorFinalize \n");
	return SA_AIS_OK;
}

// Used by the registerClass test case, then set to true to enable
// definition of an key Attribute Name
bool KeyAttributeName = false;
std::string CheckKeyAttributeName;

SaAisErrorT saImmOmClassDescriptionGet_2(SaImmHandleT immHandle, const SaImmClassNameT className, SaImmClassCategoryT *classCategory, SaImmAttrDefinitionT_2 ***attrDefinitions) {
	printf("----> saImmOmClassDescriptionGet_2 \n");

	ImmClassDef immDef = immStorageG.getClassDef(className);

	SaImmAttrDefinitionT_2** defArray = new SaImmAttrDefinitionT_2*[immDef.attrDef.size() + 1];

	for (int i = 0; i < immDef.attrDef.size(); i++) {

		SaImmAttrDefinitionT_2* def = new SaImmAttrDefinitionT_2;

		def->attrName = (char *) immDef.attrDef[i].attrName.c_str();
		def->attrValueType = (SaImmValueTypeT) immDef.attrDef[i].attrType;
		if (KeyAttributeName && (std::string(def->attrName) == CheckKeyAttributeName)) {
			def->attrFlags = SA_IMM_ATTR_RDN;
		}else {
			def->attrFlags = 0;
		}
		def->attrDefaultValue = NULL;
		printf("   attrDef: name=%s type=%d\n", immDef.attrDef[i].attrName.c_str(), immDef.attrDef[i].attrType);
		defArray[i] = def;
	}
	defArray[immDef.attrDef.size()] = NULL;

	(*attrDefinitions) = defArray;
	immMemoryTrackerG.reg(defArray);
	return SA_AIS_OK;
}


///////////////////////////////////
// ImmCommands
///////////////////////////////////


SaVersionT CM::ImmCmd::mVersion = { immReleaseCode, immMajorVersion, immBaseMinorVer };

CM::ImmCmd::ImmCmd(TxContext * txContextIn, std::string cmd, int retries) :
	mTxContextIn(txContextIn), mCmd(cmd), mRetries(retries) {
	ENTER();
	LEAVE();
}

CM::ImmCmd::~ImmCmd() {
	//	ENTER();
	//	int size = mTmpDns.size();
	//	for (int i=0; i<size; i++)
	//	{
	//		delete mTmpDns.at(i);
	//	}
	//	LEAVE();
}

SaAisErrorT CM::ImmCmd::execute() {
	return doExecute();
}

SaNameT *
CM::ImmCmd::toSaNameT(std::string &dnIn) {
	ENTER();
	// temp SaNameTs are deleted in destructor
	//	SaNameT * dn = NULL;
	//  if (dnIn.size()>0) {
	//	 dn = new SaNameT();
	//	mTmpDns.push_back(dn);
	//from the root
	//	  dn->length = dnIn.length();
	//	  snprintf((char*)(dn->value), SA_MAX_NAME_LENGTH,"%s", dnIn.c_str());
	//  }
	//LEAVE();
	return NULL;
}

//------------------------ ImmCmdOmInit ----------------------------------------------------
CM::ImmCmdOmInit::ImmCmdOmInit(TxContext * txContextIn) :
	ImmCmd(txContextIn, "saImmOmInitialize") {
	ENTER();
	LEAVE();
}

SaAisErrorT CM::ImmCmdOmInit::doExecute() {
	return SA_AIS_OK;
}

//------------------------ ImmCmdOmAccessorInit ----------------------------------------------------

CM::ImmCmdOmAccessorInit::ImmCmdOmAccessorInit(TxContext * txContextIn) :
	ImmCmd(txContextIn, "saImmOmAccessorInitialize") {
	ENTER();
	LEAVE();
}

SaAisErrorT CM::ImmCmdOmAccessorInit::doExecute() {
	return SA_AIS_OK;
}

//------------------------ ImmCmdOmAdminOwnerInit ----------------------------------------------------
CM::ImmCmdOmAdminOwnerInit::ImmCmdOmAdminOwnerInit(TxContext * txContextIn,	std::string immOwnerNameIn, SaBoolT releaseOnFinalizeIn) :
	ImmCmd(txContextIn, "saImmOmAdminOwnerInitialize"), mImmOwnerNameIn(immOwnerNameIn), mReleaseOnFinalizeIn(releaseOnFinalizeIn) {
}

SaAisErrorT ImmCmdOmAdminOwnerInit::doExecute() {
	return SA_AIS_OK;
}

//------------------------ ImmCmdOmAdminOwnerSet ----------------------------------------------------
CM::ImmCmdOmAdminOwnerSet::ImmCmdOmAdminOwnerSet(TxContext * txContextIn, std::vector<std::string> *objectDns, SaImmScopeT scope) :
	ImmCmd(txContextIn, "saImmOmAdminOwnerSet"), mObjectDns(objectDns), mScope(scope) {
}

SaAisErrorT CM::ImmCmdOmAdminOwnerSet::doExecute() {
	return SA_AIS_OK;
}

//------------------------ ImmCmdOmAdminOwnerClear ----------------------------------------------------
CM::ImmCmdOmAdminOwnerClear::ImmCmdOmAdminOwnerClear(TxContext * txContextIn, std::string dnIn, SaImmScopeT scope) :
	ImmCmd(txContextIn, "saImmOmAdminOwnerClear"), mDnIn(dnIn), mScope(scope) {
}

SaAisErrorT CM::ImmCmdOmAdminOwnerClear::doExecute() {
	return SA_AIS_OK;
}

//------------------------ ImmCmdOmAdminOwnerRelease ----------------------------------------------------
ImmCmdOmAdminOwnerRelease::ImmCmdOmAdminOwnerRelease(TxContext * txContextIn, std::vector<std::string> *objectDns, SaImmScopeT scope) :
	ImmCmd(txContextIn, "saImmOmAdminOwnerRelease"), mObjectDns(objectDns), mScope(scope) {
}

SaAisErrorT ImmCmdOmAdminOwnerRelease::doExecute() {
	return SA_AIS_OK;
}

//------------------------ ImmCmdOmClassDescriptionGet ----------------------------------------------------
CM::ImmCmdOmClassDescriptionGet::ImmCmdOmClassDescriptionGet(
		TxContext * txContextIn/*in*/, SaImmClassNameT className/*in*/,
		SaImmClassCategoryT *classCategory/*out*/,
		SaImmAttrDefinitionT_2 *** attrDef/*out*/) :
	ImmCmd(txContextIn, "saImmOmClassDescription_2"), mAttrDef(attrDef),
		mClassCategory(classCategory), mClassName(className) {
}

SaAisErrorT CM::ImmCmdOmClassDescriptionGet::doExecute() {
	return SA_AIS_OK;
}

//------------------------ ImmCmdOmKeySearchInit ----------------------------------------------------
CM::ImmCmdOmKeySearchInit::ImmCmdOmKeySearchInit(TxContext *txContextIn, std::string dnIn, std::string classNameIn, BridgeImmIterator* biter) :
	ImmCmd(txContextIn, "saImmOmSearchInitialize_2"), mDnIn(dnIn), mClassNameIn(classNameIn) {
	printf("-----> ImmCmdOmKeySearchInit: dn=%s classname=%s\n", dnIn.c_str(), classNameIn.c_str());
}

SaAisErrorT CM::ImmCmdOmKeySearchInit::doExecute() {
	return SA_AIS_OK;
}

// result string for CM::ImmCmdOmSearchNext::ImmCmdOmSearchNext
std::string GLOBAL_immCmdOmSearchNextDnOut;

//------------------------ ImmCmdOmKeySearchNext ----------------------------------------------------
CM::ImmCmdOmSearchNext::ImmCmdOmSearchNext(TxContext *txContextIn, std::string dnIn, BridgeImmIterator* biter, std::string *dnOut/*out*/, SaImmAttrValuesT_2*** attrValsOut/*out*/) :
	ImmCmd(txContextIn, "saImmOmSearchNext_2"), mDnIn(dnIn), mDnOut(dnOut), mAttrValsOut(attrValsOut) {
	printf("-----> ImmCmdOmSearchNext: dn=%s dnOut=%s\n", dnIn.c_str(), GLOBAL_immCmdOmSearchNextDnOut.c_str());
	// returnt the response
	// note that the ImmBridge does not seem to care about attrvalsOut at all, only the dnOut.
	(*dnOut) = GLOBAL_immCmdOmSearchNextDnOut;

}

SaAisErrorT CM::ImmCmdOmSearchNext::doExecute() {
	return SA_AIS_OK;
}

//------------------------ ImmCmAccessorGet ----------------------------------------------------


CM::ImmCmdOmAccessorGet::ImmCmdOmAccessorGet(TxContext *txContextIn, std::string dnIn, std::string attrNameIn, SaImmAttrValuesT_2*** attrValsOut /*out*/) :
	ImmCmd(txContextIn, "saImmOmAccessorGet_2"), mDnIn(dnIn), mAttrNameIn(attrNameIn), mAttrValsOut(attrValsOut) {
	ImmItem item = immStorageG.readFromImmStorage(dnIn.c_str(), attrNameIn.c_str());
	SaImmAttrValuesT_2* res = new SaImmAttrValuesT_2; // allocate value space
	SaImmAttrValuesT_2** resPtr = new SaImmAttrValuesT_2*[2]; // allocate pointer array
	resPtr[0] = res; // add our attribute to the array
	resPtr[1] = NULL; // null terminate array of attributes (no counter!)

	res->attrName = new char[attrNameIn.size()];
	memcpy(res->attrName, attrNameIn.c_str(), attrNameIn.size());

	int nrofValues = item.data.size();
	res->attrValueType = (SaImmValueTypeT) item.type;
	res->attrValuesNumber = nrofValues;
	if (nrofValues == 0) {
		// exit if there are no values!
		res->attrValues = NULL;
		(*attrValsOut) = resPtr;
		return;
	}
	res->attrValues = new void*[nrofValues]; // allocate array of void*
	for (int i = 0; i < nrofValues; i++) {

		switch (res->attrValueType) {
		case SA_IMM_ATTR_SANAMET: {

				// make a SaNameT
				int size = item.data[i].size();
				if (size > SA_MAX_NAME_LENGTH) {
					size = SA_MAX_NAME_LENGTH;
				}
				SaNameT* buf = new SaNameT;
				//char* buf = new char[size+1];
				//SaNameT** bufPtr = new SaNameT*;
				//*bufPtr = buf;
				buf->length = size;
				memcpy(buf->value, item.data[i].c_str(), size);
				res->attrValues[i] = (void*) buf;
				break;
			}
		case SA_IMM_ATTR_SASTRINGT: {
				// make a char**
				int size = item.data[i].size();
				char* buf = new char[size + 1];
				char** bufPtr = new char*;
				*bufPtr = buf;
				strcpy(buf, item.data[i].c_str());
				res->attrValues[i] = (void*) bufPtr;
				break;
			}
		default: {
				printf("ERROR in CM::ImmCmdOmAccessorGet::ImmCmdOmAccessorGet, unsupported TYPE %d\n", res->attrValueType);
			}
		}
	}
	(*attrValsOut) = resPtr;
	immMemoryTrackerG.reg(resPtr);
}

SaAisErrorT CM::ImmCmdOmAccessorGet::doExecute() {
	return SA_AIS_OK;
}

//------------------------ ImmCmdOmCcbInitialize ----------------------------------------------------
CM::ImmCmdOmCcbInit::ImmCmdOmCcbInit(TxContext *txContextIn) :
	ImmCmd(txContextIn, "saImmOmCcbInitialize") {
}

SaAisErrorT CM::ImmCmdOmCcbInit::doExecute() {
	return SA_AIS_OK;
}

//------------------------ ImmCmdOmCcbFinalize ----------------------------------------------------
CM::ImmCmdOmCcbFinalize::ImmCmdOmCcbFinalize(TxContext *txContextIn) :
	ImmCmd(txContextIn, "saImmOmCcbFinalize") {
}

SaAisErrorT CM::ImmCmdOmCcbFinalize::doExecute() {
	return SA_AIS_OK;
}

//------------------------ ImmCmdOmCcbObjectModify ----------------------------------------------------
CM::ImmCmdOmCcbObjectModify::ImmCmdOmCcbObjectModify(TxContext *txContextIn, SaNameT* dnIn, SaImmAttrModificationT_2 **attrModsIn) :
	ImmCmd(txContextIn, "saImmOmCcbObjectModify_2"), mDnIn(dnIn), mAttrModsIn(attrModsIn) {
	//	char* dn = new char[dnIn->length+1];
	//	memcpy(dn,dnIn->value,dnIn->length);
	//	dn[dnIn->length]=0; // make a c-string!!

	char* dn = makeCString(dnIn);

	SaImmAttrModificationT_2** modArray = attrModsIn;

	int i = 0;
	while (modArray[i] != NULL) {
		SaImmAttrModificationT_2* mod = modArray[i];
		switch (mod->modType) {

		case SA_IMM_ATTR_VALUES_ADD:
		case SA_IMM_ATTR_VALUES_REPLACE: {
				std::vector<std::string> values;
				for (int a = 0; a < mod->modAttr.attrValuesNumber; a++) {
					// assume string type!!
					switch (mod->modAttr.attrValueType) {
					case SA_IMM_ATTR_SASTRINGT: {
							std::string strval(*((char**) mod->modAttr.attrValues[a]));
							values.push_back(strval);
							printf(
							"ModifyImm: dn=%s modtype=%d attr=%s attrtype=%d attval=%s\n",
							dn, mod->modType, mod->modAttr.attrName,
							mod->modAttr.attrValueType, strval.c_str());
							break;
						}
					case SA_IMM_ATTR_SANAMET: {
							std::string strval(((char*) mod->modAttr.attrValues[a]));
							values.push_back(strval);
							printf(
							"ModifyImm: dn=%s modtype=%d attr=%s attrtype=%d attval=%s\n",
							dn, mod->modType, mod->modAttr.attrName,
							mod->modAttr.attrValueType, strval.c_str());
							break;
						}
					default: {
							printf(
							"ERROR: Unsupported datatype in CM::ImmCmdOmCcbObjectModify::ImmCmdOmCcbObjectModify, currently IMM stub backend only support SA_STRING and SA_NAME\n");
						}

					}
				}
				immStorageG.addToImmStorage(dn, mod->modAttr.attrName,
				mod->modAttr.attrValueType, values);
				break;
			}
		case SA_IMM_ATTR_VALUES_DELETE: {
				for (int a = 0; a < mod->modAttr.attrValuesNumber; a++) {
					immStorageG.deleteFromImmStorage(dn, mod->modAttr.attrName);
				}

				break;
			}

		}// switch

		i = i + 1;
	}// while
	delete[] dn; // remove tmp


}

SaAisErrorT CM::ImmCmdOmCcbObjectModify::doExecute() {
	return SA_AIS_OK;
}

//------------------------ ImmCmdOmCcbApply ----------------------------------------------------
CM::ImmCmdOmCcbApply::ImmCmdOmCcbApply(TxContext *txContextIn) :
	ImmCmd(txContextIn, "saImmOmCcbApply") {
}

SaAisErrorT ImmCmdOmCcbApply::doExecute() {
	return SA_AIS_OK;
}

/**
* Helper to find the key attribute matching the classname!
*/
std::string findInstanceKey(char* className, SaImmAttrValuesT_2** attrValsIn) {

	std::string keyName(className);
	keyName[0] = tolower(keyName[0]);
	keyName.append("Id");

	int i = 0;
	while (attrValsIn[i] != NULL) {
		if (0 == strcmp(attrValsIn[i]->attrName, keyName.c_str())
				&& attrValsIn[i]->attrValueType == 9) {
			// found key now make instance key!
			std::string instance;
			char* attr = *((char**) attrValsIn[i]->attrValues[0]);
			instance.append(attr);
			return instance;
		}
		i = i + 1;
	}
	return "";
}

//------------------------ ImmCmdOmCcbCreate ----------------------------------------------------
CM::ImmCmdOmCcbObjectCreate::ImmCmdOmCcbObjectCreate(TxContext* txContextIn, SaNameT *parentIn, SaImmClassNameT classNameIn, SaImmAttrValuesT_2** attrValsIn) :
	ImmCmd(txContextIn, "saImmOmCcbObjectCreate_2"), mParentIn(parentIn), mClassNameIn(classNameIn), mAttrValsIn(attrValsIn) {
	char* parentDn = makeCString(parentIn);
	printf("ImmCmdOmCcbObjectCreate parent=%s class=%s\n", parentDn, classNameIn);

	// build complete path
	std::string instance = findInstanceKey((char*) classNameIn, attrValsIn);
	std::string dn(instance);
	dn.append(",");
	dn.append(parentDn);

	SaNameT* path = makeSaNameT(dn.c_str());

	SaImmAttrModificationT_2 mod;
	mod.modType = SA_IMM_ATTR_VALUES_REPLACE;
	mod.modAttr.attrValuesNumber = 1;

	SaImmAttrModificationT_2** modArray = new SaImmAttrModificationT_2*[2];
	modArray[0] = &mod;
	modArray[1] = NULL;
	int i = 0;
	while (attrValsIn[i] != NULL) {
		mod.modAttr.attrValueType = attrValsIn[i]->attrValueType;
		mod.modAttr.attrName = attrValsIn[i]->attrName;
		mod.modAttr.attrValuesNumber = attrValsIn[i]->attrValuesNumber;
		mod.modAttr.attrValues = attrValsIn[i]->attrValues;
		CM::ImmCmdOmCcbObjectModify::ImmCmdOmCcbObjectModify modCmd(txContextIn, path, modArray);
		i = i + 1;
	}

	modArray[0] = NULL;
	delete[] modArray;
	delete path;
	delete[] parentDn;

}

SaAisErrorT CM::ImmCmdOmCcbObjectCreate::doExecute() {
	return SA_AIS_OK;
}

//------------------------ ImmCmdOmCcbDelete ----------------------------------------------------
CM::ImmCmdOmCcbObjectDelete::ImmCmdOmCcbObjectDelete(TxContext* txContextIn, SaNameT* dnIn) :
	ImmCmd(txContextIn, "saImmOmCcbObjectDelete"), mDnIn(dnIn) {
}

SaAisErrorT CM::ImmCmdOmCcbObjectDelete::doExecute() {
	return SA_AIS_OK;
}

//------------------------ ImmCmdOmAdminOperationInvoke ----------------------------------------------------

ImmCmdOmAdminOperationInvoke::ImmCmdOmAdminOperationInvoke(
		TxContext *txContextIn, std::string dnIn,
		SaImmAdminOperationIdT operationId,
		SaImmAdminOperationParamsT_2 **params,
		SaAisErrorT *operationReturnValue /*out*/,
		SaImmAdminOperationParamsT_2 ***returnParams /*out*/) :
	ImmCmd(txContextIn, "saImmOmAdminOperationInvoke_2"), mDnIn(dnIn),
		mOperationId(operationId), mParams(params), mOperationReturnValue(
		operationReturnValue) {
	printf("-----> ImmCmdOmAdminOperationInvoke: dnIn=%s operationId=%i\n", dnIn.c_str(), operationId);
}

SaAisErrorT ImmCmdOmAdminOperationInvoke::doExecute() {
	if (sParameterVerifier != 0) {
		printf("-----> ImmCmdOmAdminOperationInvoke:doExecute()\n");
		sParameterVerifier->checkParameters(mParams);
	}
	return SA_AIS_OK;
}

//------------------------ ImmCmdOmAdminOperationMemoryFree ----------------------------------------------------

CM::ImmCmdOmAdminOperationMemoryFree::ImmCmdOmAdminOperationMemoryFree(TxContext *txContextIn, SaImmAdminOperationParamsT_2 **returnParams) :
	ImmCmd(txContextIn, "saImmOmAdminOperationMemoryFree"), mReturnParams(returnParams) {
	printf("-----> ImmCmdOmAdminOperationMemoryFree\n");
}

SaAisErrorT CM::ImmCmdOmAdminOperationMemoryFree::doExecute() {
	printf("-----> ImmCmdOmAdminOperationMemoryFree:doExecute()\n");
	return SA_AIS_OK;
}

/**
* Allocate c-string on heap
*/
char* allocCstr(const char* str) {
	unsigned int len = strlen(str);
	char* tmp = new char[len + 1]; // the '\0' terminator
	strcpy(tmp, str);
	return tmp;
}
