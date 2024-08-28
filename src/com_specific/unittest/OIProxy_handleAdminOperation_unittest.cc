
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
*   File:   OIProxy_handleAdminOperation_unittest.cc
*
*   Author: xnikvap
*
*   Date:   2012-12-13
*
*   Unit test for OIProxy_handleAdminOperation
*   (based on the unit test of the implementation of the OamSARegisterObjectImplementer)
*   Modified: xadaleg 2014-08-02  MR35347 - increase DN length
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
#include "saAis.h"
#include "saImm.h"
#include "saImmOm.h"
#include "saImmOi.h"
// COM
#include "MafMgmtSpiCommon.h"
#include "MafMgmtSpiComponent_1.h"
#include "MafMgmtSpiInterfacePortal_1.h"
#include "MafOamSpiModelRepository_1.h"
#include <MafOamSpiModelRepository_2.h>
#include "MafOamSpiManagedObject_3.h"
#include "MafOamSpiRegisterObjectImplementer_2.h"
// COM SA
#include "OamSATransactionRepository.h"
#include "OamSACache.h"
#include "TxContext.h"
#include "ImmCmd.h"
#include "OamSATranslator.h"
#include "ComSA.h"

// TESTS
#include "OamSAImmBridge.h"
#include "ParameterVerifier.h"
#include "StubIMM.h"
#include "MockProcessUtil.h"
#include "MockTransactionMasterIF2.h"
#include <OamSAOIProxy.h>
#include <tr1/memory>

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

ParameterVerifier* sParameterVerifier;

aaservice::MockProcessUtil *mymockProcessUtil;

extern bool saImmOmCcbGetErrorStrings_accessed;

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

extern MafReturnT registerClass(const char * componentName, const char * mocPath);

extern MafReturnT registerDn(const char * componentName, const char * mocPath);

extern MafReturnT unregisterDn(const char * componentName, const char * mocPath);

extern MafReturnT unregisterClass(const char * componentName, const char * mocPath);

/**
* MOCK thing we do not use in the code.
*/

MafOamSpiTransaction_1* MafOamSpiTransactionStruct_p = NULL;

extern "C" MafOamSpiTransactionalResource_2T* ExportOamSATransactionalResourceInterface_V2(void);

/* SDP1694 - support MAF SPI */
// FIXME : DEFINE THIS ONE LATER
extern MafOamSpiModelRepository_4T* maf_theModelRepo_v4_p;

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

using namespace std::tr1;

typedef struct _auto_clean
{
	std::list<void*> listItem;
	~_auto_clean()
	{
		std::list<void*>::iterator it = listItem.begin();
		for(; it != listItem.end(); it++)
		{
			if(*it)
			{
				delete *it;
				*it = NULL;
			}
		}
	}
} auto_clean;

typedef struct _auto_clean_free
{
	std::list<void*> listItem;
	~_auto_clean_free()
	{
		std::list<void*>::iterator it = listItem.begin();
		for(; it != listItem.end(); it++)
		{
			if(*it)
			{
				free(*it);
				*it = NULL;
			}
		}
	}
} auto_clean_free;

auto_clean autoCleanList;
auto_clean_free autoCleanFreeList;

/*
* MafOamSpiManagedObject
*/
extern std::string receivedData_setMoAttribute;
extern std::string receivedDN_setMoAttribute;
MafReturnT setMoAttribute(MafOamSpiTransactionHandleT txHandle, const char * dn, const char * attributeName, const MafMoAttributeValueContainer_3T * attributeValue) {

	receivedData_setMoAttribute=attributeValue->values->value.theString;
	receivedDN_setMoAttribute=dn;
	return MafOk;
}

static void dummyReleaseOne(MafMoAttributeValueContainer_3T *container)
{
}

static void dummyReleaseMany(MafMoAttributeValueContainer_3T **containers)
{
}

MafMoAttributeValueContainer_3T  * getMoAttributeresult;

MafReturnT getMoAttribute(MafOamSpiTransactionHandleT txHandle, const char * dn, const char * attributeName, MafMoAttributeValueResult_3T *result) {
	//Return prepared data structure
	result->container = getMoAttributeresult;
	result->release = &(dummyReleaseOne);
	return MafOk;
}

MafReturnT getMoAttributes(MafOamSpiTransactionHandleT txHandle, const char * dn, const char ** attributeNames, MafMoAttributeValuesResult_3T *result) {

	//Return prepared data structure for a single result
	MafMoAttributeValueContainer_3T** conv_result = (result->containers);
	*conv_result = getMoAttributeresult;
	conv_result++;
	*conv_result = NULL; // results terminator

	result->release = &(dummyReleaseMany);
	return MafOk;
}

MafReturnT newMoIterator(MafOamSpiTransactionHandleT txHandle, const char * dn, const char * className, MafOamSpiMoIteratorHandle_3T *result) {
	return MafOk;
}
MafReturnT nextMo(MafOamSpiMoIteratorHandle_3T itHandle, char **result) {
	return MafOk;
}

extern std::string parentDnReceived, classNameReceived, keyAttributeNameReceived, keyAttributevalueReceived;
extern std::string dnReceived;
extern MafReturnT ReturndeleteMo;

MafReturnT deleteMo(MafOamSpiTransactionHandleT txHandle, const char * dn) {
	dnReceived.clear();
	dnReceived = dn;
	return ReturndeleteMo;
}


extern MafMoAttributeValueContainer_3T parameters_received[10];
MafReturnT action(MafOamSpiTransactionHandleT txHandle, const char * dn, const char * name, MafMoNamedAttributeValueContainer_3T **parameters, MafMoAttributeValueResult_3T * result) {
	return MafOk;
}

static MafReturnT finalizeMoIterator(MafOamSpiMoIteratorHandle_3T itHandle)
{
	return MafOk;
}

static MafReturnT existsMo(MafOamSpiTransactionHandleT txHandle, const char * dn, bool * result)
{
	return MafOk;
}

static MafReturnT countMoChildren(MafOamSpiTransactionHandleT txHandle, const char * dn, const char * className, uint64_t * result)
{
	return MafOk;
}

extern MafOamSpiManagedObject_3 stubPortalManaged;

/* SDP1694 - support MAF SPI */

struct MafOamSpiMom* maf_makeMom(const char* name, struct MafOamSpiMoc* rootMoc) {
	struct MafOamSpiMom* mom = new struct MafOamSpiMom;
	autoCleanList.listItem.push_back(mom);
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
	autoCleanList.listItem.push_back(moc);
	moc->generalProperties.name = (char*) name;
	moc->moAttribute = attrList;
	moc->parentContainment = NULL;
	moc->childContainment = NULL;
	return moc;
}

struct MafOamSpiMoc* maf_makeMoc(const char* name, struct MafOamSpiMoAttribute* attrList, struct MafOamSpiContainment* childContainment, struct MafOamSpiContainment* parentContainment) {
	struct MafOamSpiMoc* moc = new struct MafOamSpiMoc;
	autoCleanList.listItem.push_back(moc);
	moc->generalProperties.name = (char*) name;
	moc->moAttribute = attrList;
	moc->parentContainment = childContainment;
	moc->childContainment = parentContainment;
	return moc;
}

struct MafOamSpiMoc* maf_makeMoc(const char* name, struct MafOamSpiMoAttribute* attrList, std::string &momName) {
	struct MafOamSpiMoc* moc = new struct MafOamSpiMoc;
	autoCleanList.listItem.push_back(moc);
	moc->generalProperties.name = (char*) name;
	moc->moAttribute = attrList;
	moc->parentContainment = NULL;
	moc->childContainment = NULL;
	/*
	* set the mom general properties
	*/
	struct MafOamSpiMom *mom = new struct MafOamSpiMom;
	autoCleanList.listItem.push_back(mom);

	// Allocate memory for string plus a finishing \0
	//char *tempPointer = new (char[momName.size()+1]);
	char *tempPointer = strndup(momName.c_str(), momName.size()+1);
	autoCleanFreeList.listItem.push_back(tempPointer);
	//strcpy(tempPointer, momName.c_str());
	mom->generalProperties.name = tempPointer;
	moc->mom = mom;
	return moc;
}

struct MafOamSpiContainment* maf_makeContainment(MafOamSpiMocT* parent, MafOamSpiMocT* child, MafOamSpiContainmentT* nextSameParent, MafOamSpiContainmentT* nextSameChild) {
	struct MafOamSpiContainment* cont = new struct MafOamSpiContainment;
	autoCleanList.listItem.push_back(cont);
	cont->parentMoc = parent;
	cont->childMoc = child;
	cont->nextContainmentSameParentMoc = nextSameParent;
	cont->nextContainmentSameChildMoc = nextSameChild;
	return cont;
}

struct MafOamSpiContainment* maf_makeContainment(struct MafOamSpiMoc* parent, struct MafOamSpiMoc* child) {
	struct MafOamSpiContainment* cont = new struct MafOamSpiContainment;
	autoCleanList.listItem.push_back(cont);
	cont->parentMoc = parent;
	cont->childMoc = child;
	cont->nextContainmentSameParentMoc = NULL;
	cont->nextContainmentSameChildMoc = NULL;

	// update child and parent
	parent->childContainment = cont;
	child->parentContainment = cont;
	return cont;
}

struct MafOamSpiMoAttribute* maf_makeAttribute(char* name, MafOamSpiMoAttributeTypeT type, struct MafOamSpiMoAttribute* next) {
	struct MafOamSpiMoAttribute* attr = new struct MafOamSpiMoAttribute;
	autoCleanList.listItem.push_back(attr);
	attr->generalProperties.name = (char*) name;
	attr->type = type;
	attr->next = next;
	return attr;
}

struct MafOamSpiMoAttribute* maf_makeAttribute(char* name, struct MafOamSpiStruct* structData, struct MafOamSpiMoAttribute* next) {
	struct MafOamSpiMoAttribute* attr = new struct MafOamSpiMoAttribute;
	autoCleanList.listItem.push_back(attr);
	attr->generalProperties.name = (char*) name;
	attr->generalProperties.description = (char*) "This is an attribute in UnitTesting";
	attr->generalProperties.specification = (char*) "A specification";
	attr->generalProperties.status = MafOamSpiStatus_CURRENT;
	attr->generalProperties.hidden = (char*) "The hidden!!";

	attr->type = MafOamSpiMoAttributeType_STRUCT; // implicit
	attr->structDatatype = structData;
	attr->next = next;
	return attr;
}

struct MafOamSpiStruct* maf_makeStruct(char* name, struct MafOamSpiStructMember* memberList) {
	struct MafOamSpiStruct* s = new struct MafOamSpiStruct;
	autoCleanList.listItem.push_back(s);
	s->generalProperties.name = name;
	s->members = memberList;
	return s;
}

struct MafOamSpiStructMember* maf_makeStructMember(char* name, MafOamSpiDatatype type, struct MafOamSpiStructMember* next) {
	struct MafOamSpiStructMember* m = new struct MafOamSpiStructMember;
	autoCleanList.listItem.push_back(m);
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

static MafOamSpiModelRepository_1T maf_InterfaceStruct = { { "", "", "1" },
	maf_getMoms, maf_getMoc, maf_getTreeRoot };
MafOamSpiModelRepository_1T* theModelRepo_v1_p = &maf_InterfaceStruct;

// MOCK the InterfacePortal
MafReturnT maf_getInterface( MafMgmtSpiInterface_1T interfaceId, MafMgmtSpiInterface_1T **result) {
	*result = (MafMgmtSpiInterface_1T *)&maf_InterfaceStruct;
	return MafOk;
}

static MafMgmtSpiInterfacePortal_3T maf_thePortal = {{ "", "", "3" },
	maf_getInterface, 0, 0};

/*
* MafOamSpiManagedObject
*/
MafReturnT maf_setMoAttribute(MafOamSpiTransactionHandleT txHandle, const char * dn, const char * attributeName, const MafMoAttributeValueContainer_3T * attributeValue) {

	receivedData_setMoAttribute=attributeValue->values->value.theString;
	receivedDN_setMoAttribute=dn;
	return MafOk;
}

static void maf_dummyReleaseOne(MafMoAttributeValueContainer_3T *container)
{
}

static void maf_dummyReleaseMany(MafMoAttributeValueContainer_3T **containers)
{
}

MafMoAttributeValueContainer_3T  * maf_getMoAttributeresult;

MafReturnT maf_getMoAttribute(MafOamSpiTransactionHandleT txHandle, const char * dn, const char * attributeName, MafMoAttributeValueResult_3T *result) {
	//Return prepared data structure
	result->container = maf_getMoAttributeresult;
	result->release = &(maf_dummyReleaseOne);
	return MafOk;
}

MafReturnT maf_getMoAttributes(MafOamSpiTransactionHandleT txHandle, const char * dn, const char ** attributeNames, MafMoAttributeValuesResult_3T *result) {

	//Return prepared data structure for a single result
	MafMoAttributeValueContainer_3T** conv_result = (result->containers);
	*conv_result = maf_getMoAttributeresult;
	conv_result++;
	*conv_result = NULL; // results terminator

	result->release = &(maf_dummyReleaseMany);
	return MafOk;
}

MafReturnT maf_newMoIterator(MafOamSpiTransactionHandleT txHandle, const char * dn, const char * className,MafOamSpiMoIteratorHandle_3T *result) {
	return MafOk;
}
MafReturnT maf_nextMo(MafOamSpiMoIteratorHandle_3T itHandle, char **result) {
	return MafOk;
}


MafReturnT maf_createMo(MafOamSpiTransactionHandleT txHandle, const char * parentDn, const char * className, const char * keyAttributeName, const char * keyAttributevalue,MafMoNamedAttributeValueContainer_3T ** initialAttributes) {
	parentDnReceived.clear();
	classNameReceived.clear();
	keyAttributeNameReceived.clear();
	keyAttributevalueReceived.clear();
	parentDnReceived.clear();

	parentDnReceived = parentDn;
	classNameReceived = className;
	keyAttributeNameReceived = keyAttributeName;
	keyAttributevalueReceived = keyAttributevalue;

	return MafOk;
}

MafReturnT maf_ReturndeleteMo = MafOk;

MafReturnT maf_deleteMo(MafOamSpiTransactionHandleT txHandle, const char * dn) {
	dnReceived.clear();
	dnReceived = dn;
	return maf_ReturndeleteMo;
}


MafMoAttributeValueContainer_3T maf_parameters_received[10];
MafReturnT maf_action(MafOamSpiTransactionHandleT txHandle, const char * dn, const char * name, MafMoNamedAttributeValueContainer_3T **parameters, MafMoAttributeValueResult_3T * result) {
	return MafOk;
}
static MafReturnT maf_finalizeMoIterator(MafOamSpiMoIteratorHandle_3T itHandle)
{
	return MafOk;
}


static MafReturnT maf_existsMo(MafOamSpiTransactionHandleT txHandle, const char * dn, bool * result)
{
	return MafOk;
}


static MafReturnT maf_countMoChildren(MafOamSpiTransactionHandleT txHandle, const char * dn, const char * className, uint64_t * result)
{
	return MafOk;
}

MafOamSpiManagedObject_3T maf_stubPortalManaged = { { "", "", "" },
	maf_setMoAttribute,
	maf_getMoAttribute,
	maf_getMoAttributes,
	maf_newMoIterator,
	maf_nextMo,
	maf_createMo,
	maf_deleteMo,
	maf_action,
	maf_finalizeMoIterator,
	maf_existsMo,
	maf_countMoChildren};


/*
* END MafOamSpiManagedObject
*/

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


#ifdef __cplusplus
}
#endif

#endif //REDIRECT_LOG

//////////////////////////////////////////////////////
// IMM API
//////////////////////////////////////////////////////


std::string receivedData_saImmOiRtObjectUpdate_2;

SaAisErrorT saImmOiRtObjectUpdate_2(SaImmOiHandleT immOiHandle, const SaNameT *objectName, const SaImmAttrModificationT_2 **attrMods) {
	printf("----> saImmOiRtObjectUpdate_2 \n");
	receivedData_saImmOiRtObjectUpdate_2=*(char**)(attrMods[0]->modAttr.attrValues[0]);
	return SA_AIS_OK;
}

/* OI sets error string in CCB related upcall. See: http://devel.opensaf.org/ticket/1904 */

// Received error strings saved here
extern std::list<std::string>	ReceivedErrorStrings;

/*
* Gets the callbacks here ...
*/

SaImmOiCallbacksT_2 immOiCalls;
SaImmOiHandleT immOiHandleTEST = 1;

extern bool saImmOiInitialize_2_return_OK;
extern bool saImmOiInitialize_2_accessed;

extern bool saImmOiImplementerSet_return_OK;
extern bool saImmOiImplementerSet_accessed;

extern SaAisErrorT saImmOiClassImplementerSetChoice;
extern std::string saImmOiClassImplementerSet_receivedRegistration;

extern bool saImmOiSelectionObjectGet_return_OK;
extern bool saImmOiSelectionObjectGet_accessed;
extern std::string recievedData_saImmOiClassImplementerRelease;
extern SaAisErrorT saImmOiClassImplementerRelease_returnvalue;
extern int returnCounter;

extern std::string recievedData_saImmOiObjectImplementerSet;
extern std::list<std::string> recievedData_saImmOiObjectImplementerSetList;
extern SaAisErrorT saImmOibjectImplementerSet_returnvalue;

extern std::string recievedData_saImmOiObjectImplementerRelease;
extern SaAisErrorT saImmOiObjectImplementerRelease_returnvalue;
extern int returnCounter1;

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

	char* tmp =  makeCString(objectName);

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

extern bool saImmOmInitialize_return_OK;
extern bool saImmOmInitialize_accessed;

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


extern struct MafOamSpiContainment* maf_makeContainment(struct MafOamSpiMoc* parent, struct MafOamSpiMoc* child);
///////////////////////////////////
// ImmCommands
///////////////////////////////////


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
CM::ImmCmdOmAdminOwnerInit::ImmCmdOmAdminOwnerInit(TxContext * txContextIn, std::string immOwnerNameIn, SaBoolT releaseOnFinalizeIn) :
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
CM::ImmCmdOmClassDescriptionGet::ImmCmdOmClassDescriptionGet(TxContext * txContextIn/*in*/, SaImmClassNameT className/*in*/, SaImmClassCategoryT *classCategory/*out*/, SaImmAttrDefinitionT_2 *** attrDef/*out*/) :
	ImmCmd(txContextIn, "saImmOmClassDescription_2"), mAttrDef(attrDef), mClassCategory(classCategory), mClassName(className) {
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
	ImmItem item = immStorageG.readFromImmStorage(dnIn.c_str(),
	attrNameIn.c_str());
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
				SaNameT* buf = new SaNameT;
				saNameSet(item.data[i].c_str(), buf);
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
				printf(
				"ERROR in CM::ImmCmdOmAccessorGet::ImmCmdOmAccessorGet, unsupported TYPE %d\n",
				res->attrValueType);
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
		CM::ImmCmdOmCcbObjectModify modCmd(txContextIn, path, modArray);
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
CM::ImmCmdOmCcbObjectDelete::ImmCmdOmCcbObjectDelete(TxContext* txContextIn,
		SaNameT* dnIn) :
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
	printf("-----> ImmCmdOmAdminOperationInvoke: dnIn=%s operationId=%i\n",
	dnIn.c_str(), operationId);
}

SaAisErrorT ImmCmdOmAdminOperationInvoke::doExecute() {
	if (sParameterVerifier != 0) {
		printf("-----> ImmCmdOmAdminOperationInvoke:doExecute()\n");
		sParameterVerifier->checkParameters(mParams);
	}
	return SA_AIS_OK;
}

/**
* Allocate c-string on heap
*/
extern char* allocCstr(const char* str);

/*
*
* UNIT TESTS
*
*/
extern OamSATranslator	theTranslator;
extern bool MafOamSpiTransactionMaster_1_available, MafOamSpiTransactionMaster_2_available;
extern bool MafOamSpiTransactionMaster_1_selected, MafOamSpiTransactionMaster_2_selected;



extern void OIProxy_handleAdminOperation(SaImmOiHandleT immOiHandle, SaInvocationT invocation, const SaNameT *objectName, SaImmAdminOperationIdT operationId, const SaImmAdminOperationParamsT_2 **params);


// FIXME: this TC is not completed implemented, comment out
#if 0
TEST(OIProxy_handleAdminOperation, BasicCall )
{
	// Test to register a class named FileM in IMM.

	// Clear the Model Repository
	theTranslator.ResetMOMRoot();

	// setup ModelRepository
	struct MafOamSpiMoAttribute* employeeAttrs = makeAttribute("title", MafOamSpiMoAttributeType_STRING, NULL);
	struct MafOamSpiMoc* FileMMoc = makeMoc("FileM", NULL);
	//	struct MafOamSpiMoc* FileMMoc = makeMoc("FileM", employeeAttrs);
	struct MafOamSpiMoc* DataMMoc = makeMoc("DataM", employeeAttrs);
	struct MafOamSpiContainment* cont = makeContainment(FileMMoc, DataMMoc);
	struct MafOamSpiMom* model = makeMom("FileM", FileMMoc);
	setMom(model);

	{
		// add the class name attribute for Employee with two attributes "title" and "person"
		std::vector<std::string> className;
		className.push_back("FileM");
		className.push_back("DataM");
		unsigned int IMM_STRING_TYPE=9;
		unsigned int IMM_REF_TYPE = 6;
		immStorageG.addToImmStorage("employeeId=2,meId=1","SaImmAttrClassName",IMM_STRING_TYPE,className);

		// add attribute defintions to class
		immStorageG.addImmClassAttributeDef(std::string("FileM"), std::string("title"), IMM_STRING_TYPE);
		immStorageG.addImmClassAttributeDef(std::string("DataM"), std::string("person"), IMM_STRING_TYPE);
	}

	// setUp the MOCK for the ProcessUtil class.

	std::string Output = "";
	mymockProcessUtil = new aaservice::MockProcessUtil(Output,0);
	std::auto_ptr<aaservice::MockProcessUtil> x(mymockProcessUtil);


	// Initialize the maf_oamSAOiProxyInitialization internal Interface pointer
	// This is a stubbed version only providing the MafOamSpiModelRepository_1T
	MafMgmtSpiInterfacePortal_3T* _portal = (MafMgmtSpiInterfacePortal_3T *)&maf_thePortal;

	maf_oamSAOiProxyInitialization( _portal);

	// Setup the MOM with a personDataId=1, I use a mock above instead.

	char* myObjName = "employeeId=2,meId=1";

	SaImmOiHandleT immOiHandle=47;
	SaInvocationT invocation=447;
	SaNameT objectName;

	saNameSet(myObjName, &objectName);

	SaImmAdminOperationIdT operationId=52;

	SaImmAdminOperationParamsT_2 params[2];

	//	typedef struct {
	//		SaStringT paramName;
	//		SaImmValueTypeT paramType;
	//		SaImmAttrValueT paramBuffer;
	//	} SaImmAdminOperationParamsT_2;

	char *paraMValue="attributeOneValue";
	char *ParaMValue_1="attributeTwoValue";
	params[0].paramName="attributeOne";
	params[0].paramType=SA_IMM_ATTR_SASTRINGT;
	params[0].paramBuffer=(void*)&paraMValue;

	params[1].paramName="attributeTwo";
	params[1].paramType=SA_IMM_ATTR_SASTRINGT;
	params[1].paramBuffer=(void*)&ParaMValue_1;


	SaImmAdminOperationParamsT_2 *pNull=NULL;
	const SaImmAdminOperationParamsT_2 *pparams[3]={&params[0],&params[1],pNull};

	OIProxy_handleAdminOperation(immOiHandle, invocation, (const SaNameT*)&objectName, operationId, pparams);

	// FIXME: Check that this test case passed.
	EXPECT_TRUE(true);
}
#endif
