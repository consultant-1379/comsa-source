
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
*   Unit test of the implementation of the OamSARegisterObjectImplementer.
*
*   Modified: xnikvap 2012-08-30  support for COM MO SPI Ver.3 (Ver.1 is not supported any more)
*   Modified: xadaleg 2014-07-04: Enable OI proxy files *
*   Modified: xadaleg 2014-08-02  MR35347 - increase DN length
*   Modified: xthabui 2015-08-20 MR36067 - Improved OI/SPI
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
#include "MafMgmtSpiComponent_2.h"
#include "MafMgmtSpiInterfacePortal_3.h"
#include "MafOamSpiModelRepository_1.h"
#include "MafOamSpiManagedObject_3.h"
#include "MafOamSpiRegisterObjectImplementer_3.h"
// COM SA
#include "ComSA.h"
#include "OamSATransactionRepository.h"
#include "OamSACache_dummy.h"
#include "TxContext.h"
#include "ImmCmd.h"
#include "OamSATranslator_dummy.h"
#include "OamSARegisterObjectUtils.h"

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

#ifdef __cplusplus
const MafMgmtSpiInterface_1T MafOamSpiManagedObject_3Id = {0, "MafOamSpiManagedObject", "3"};
#endif
#ifndef __cplusplus
#define MafOamSpiManagedObject_3Id (MafMgmtSpiInterface_1T){NULL, "MafOamSpiManagedObject", "3"};
#endif

#ifdef __cplusplus
const MafMgmtSpiInterface_1T MafOamSpiTransactionalResource_2Id = {0, "MafOamSpiTransactionalResource", "2"};
#endif
#ifndef __cplusplus
#define MafOamSpiTransactionalResource_2Id (MafMgmtSpiInterface_1T){NULL, "MafOamSpiTransactionalResource", "2"}
#endif

// FIXME : DEFINE THIS ONE LATER
extern MafOamSpiModelRepository_1T* theModelRepo_v1_p;
extern MafOamSpiModelRepository_4T* theModelRepo_v4_p;
extern MafOamSpiModelRepository_1T InterfaceStruct;
extern SaAisErrorT saImmOmSearchNext_2_returnValue;

extern aaservice::MockProcessUtil *mymockProcessUtil;

bool saImmOmCcbGetErrorStrings_accessed = false;
SaAisErrorT saImmOmCcbGetErrorStrings(SaImmCcbHandleT ccbHandle, const SaStringT **errorStrings)
{

	SaAisErrorT rc = SA_AIS_OK;
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

extern "C" int implementor_init(void);

/* called from AMF thread */
extern "C" int implementor_stop(void);

extern "C" int implementor_terminate();

/* SDP1694 - support MAF SPI */
// FIXME : DEFINE THIS ONE LATER
MafOamSpiModelRepository_4T* maf_theModelRepo_v4_p = NULL;
// This is what we test

extern MafReturnT registerClass(const char * componentName, const char * mocPath);

extern MafReturnT registerDn(const char * componentName, const char * mocPath);

extern MafReturnT unregisterDn(const char * componentName, const char * mocPath);

extern MafReturnT unregisterClass(const char * componentName, const char * mocPath);

const char componentName[] = "TestComponent";
const char Not_componentName[] = "Not_TestComponent";

extern "C" MafOamSpiTransactionalResource_2T* maf_ExportOamSATransactionalResourceInterface_V2(void);

//////////////////////////////////////////////////////
// Utilities
//////////////////////////////////////////////////////

///////////////////////////
//STUB IMPL IMM
///////////////////////////'

// Global IMM storage!!
extern ImmStorage immStorageG;

//////////////////////////////////////////////////////
// Setup model repository
//////////////////////////////////////////////////////

/// Builders

struct MafOamSpiMom* maf_makeMom(const char* name, struct MafOamSpiMoc* rootMoc);

struct MafOamSpiMoc* maf_makeMoc(const char* name, struct MafOamSpiMoAttribute* attrList);

struct MafOamSpiMoc* maf_makeMoc(const char* name, struct MafOamSpiMoAttribute* attrList, struct MafOamSpiContainment* childContainment, struct MafOamSpiContainment* parentContainment);

struct MafOamSpiMoc* maf_makeMoc(const char* name, struct MafOamSpiMoAttribute* attrList, std::string &momName);

struct MafOamSpiContainment* maf_makeContainment(MafOamSpiMocT* parent, MafOamSpiMocT* child, MafOamSpiContainmentT* nextSameParent, MafOamSpiContainmentT* nextSameChild);

struct MafOamSpiContainment* maf_makeContainment(struct MafOamSpiMoc* parent, struct MafOamSpiMoc* child);

struct MafOamSpiMoAttribute* maf_makeAttribute(char* name, MafOamSpiMoAttributeTypeT type, struct MafOamSpiMoAttribute* next);

struct MafOamSpiMoAttribute* maf_makeAttribute(char* name, struct MafOamSpiStruct* structData, struct MafOamSpiMoAttribute* next);

struct MafOamSpiStruct* maf_makeStruct(char* name, struct MafOamSpiStructMember* memberList);

struct MafOamSpiStructMember* maf_makeStructMember(char* name, MafOamSpiDatatype type, struct MafOamSpiStructMember* next);

extern void maf_setMom(struct MafOamSpiMom* theMom);
//
// Interface SPI
//
extern MafReturnT getInterface( MafMgmtSpiInterface_1T interfaceId, MafMgmtSpiInterface_1T **result);
extern MafReturnT getComponentInterfaceArray(const char * componentName,
															const char * interface,
															MafMgmtSpiInterface_1T ***result);

static MafMgmtSpiInterfacePortal_3T thePortal3 = {{ "", "", "3" },
	getInterface, 0, 0};

static MafMgmtSpiInterfacePortal_3_1T thePortal_3_1 = {{ "", "", "3_1" },
		getComponentInterfaceArray};

MafMgmtSpiInterfacePortal_3_1T* portal_3_1 = (MafMgmtSpiInterfacePortal_3_1T *)&thePortal_3_1;

/*
* MafOamSpiManagedObject
*/
static void dummyReleaseOne(MafMoAttributeValueContainer_3T *container)
{
}

static void dummyReleaseMany(MafMoAttributeValueContainer_3T **containers)
{
}

extern std::string parentDnReceived, classNameReceived, keyAttributeNameReceived, keyAttributevalueReceived;
MafReturnT createMo(MafOamSpiTransactionHandleT txHandle,
		const char * parentDn,
		const char * className,
		const char * keyAttributeName,
		const char * keyAttributevalue,
		MafMoNamedAttributeValueContainer_3T ** initialAttributes) {
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

/* SDP1694 - support MAF SPI */

// Export the global MOM pointer
//
//
static struct MafOamSpiMom* maf_theGlobalMom;

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

// MOCK the InterfacePortal
MafReturnT maf_getInterface( MafMgmtSpiInterface_1T interfaceId, MafMgmtSpiInterface_1T **result);

//static MafMgmtSpiInterfacePortal_3T maf_thePortal = {{ "", "", "3" }, maf_getInterface, 0, 0};

/*
* MafOamSpiManagedObject
*/
static void maf_dummyReleaseOne(MafMoAttributeValueContainer_3T *container)
{
}

static void maf_dummyReleaseMany(MafMoAttributeValueContainer_3T **containers)
{
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

/*
* END MafOamSpiManagedObject
*/


#ifdef REDIRECT_LOG
////////////////////////////////////3//////////////////
// LOG
//////////////////////////////////////////////////////
#ifdef __cplusplus
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

	void coremw_log(int priority, const char* fmt, ...);
	void coremw_debug_log(int priority, const char* fmt, ...);


#ifdef __cplusplus
}
#endif

#endif //REDIRECT_LOG

//////////////////////////////////////////////////////
// IMM API
//////////////////////////////////////////////////////
SaAisErrorT saImmOmSearchInitialize_2(SaImmHandleT immHandle,
		const SaNameT *rootName,
		SaImmScopeT scope,
		SaImmSearchOptionsT searchOptions,
		const SaImmSearchParametersT_2 *searchParam,
		const SaImmAttrNameT *attributeNames, SaImmSearchHandleT *searchHandle);

/* 4.5.2 saImmOmSearchNext() */
SaAisErrorT
saImmOmSearchNext_2(SaImmSearchHandleT searchHandle, SaNameT *objectName, SaImmAttrValuesT_2 ***attributes);

extern std::string receivedData_saImmOiRtObjectUpdate_2;

/* OI sets error string in CCB related upcall. See: http://devel.opensaf.org/ticket/1904 */

/*
* Gets the callbacks here ...
*/


extern SaImmOiCallbacksT_2 immOiCalls;
extern SaImmOiHandleT immOiHandleTEST;

extern bool saImmOiInitialize_2_return_OK;
extern bool saImmOiInitialize_2_accessed;

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

	if (!saImmOiInitialize_2_return_OK) return SA_AIS_ERR_EXIST;

}

extern bool saImmOiImplementerSet_return_OK;
extern bool saImmOiImplementerSet_accessed;
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
	printf("----> saImmOiClassImplementerSet className(%s)\n", className);
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
	printf("----> saImmOiObjectImplementerSet objectName->value(%s)\n", saNameGet(objectName));
	char tempBuff[saNameMaxLen()];
	strncpy(tempBuff, saNameGet(objectName), saNameLen(objectName));
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

// Used by the registerClass test case, then set to true to enable
// definition of an key Attribute Name
extern bool KeyAttributeName;
extern std::string CheckKeyAttributeName;

///////////////////////////////////
// ImmCommands
///////////////////////////////////


// result string for CM::ImmCmdOmSearchNext::ImmCmdOmSearchNext

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
char* allocCstr(const char* str);

/*
*
* UNIT TESTS
*
*/
extern OamSATranslator	theTranslator;
extern std::map<std::string, std::vector<RegisteredCTClassStructT *> > registeredComplexTypeClass;		//	<immClassNameOfComplexType,


TEST(OamSARegisterObjectInterface, RegisterOneSimpleClass)
{
	// Test to register a class named FileM in IMM.

	// Clear the Model Repository
	theTranslator.ResetMOMRoot();
	// setup ModelRepository
	struct MafOamSpiMoAttribute* employeeAttrs = maf_makeAttribute("title",MafOamSpiMoAttributeType_STRING,NULL);
	struct MafOamSpiMoc* FileMMoc = maf_makeMoc("FileM", employeeAttrs);
	struct MafOamSpiMom* model = maf_makeMom("FileM",FileMMoc);
	saImmOmSearchNext_2_returnValue = SA_AIS_ERR_NOT_EXIST;
	maf_setMom(model);

	{
		// add the class name attribute for Employee with two attributes "title" and "person"
		std::vector<std::string> className;
		className.push_back("FileM");
		unsigned int IMM_STRING_TYPE=9;
		unsigned int IMM_REF_TYPE = 6;
		immStorageG.addToImmStorage("employeeId=2,meId=1","SaImmAttrClassName",IMM_STRING_TYPE,className);
		// add attribute definitions to class
		immStorageG.addImmClassAttributeDef(std::string("FileM"), std::string("title"), IMM_STRING_TYPE);
	}


	// setUp the MOCK for the ProcessUtil class.

	std::string Output = "";
	mymockProcessUtil = new aaservice::MockProcessUtil(Output,0);
	std::auto_ptr<aaservice::MockProcessUtil> x(mymockProcessUtil);

	// Initialize the maf_oamSAOiProxyInitialization internal Interface pointer
	// This is a stubbed version only providing the MafOamSpiModelRepository_1T
	MafMgmtSpiInterfacePortal_3T* _portal = (MafMgmtSpiInterfacePortal_3T *)&thePortal3;

	maf_oamSAOiProxyInitialization(_portal);

	const char * mocPath = "/FileM";
	// Set a flag to enable the SA_IMM_ATTR_RDN flag on the attributes so that the
	// registration of the class below finds a keyAttribute name!

	extern bool KeyAttributeName;
	extern std::string CheckKeyAttributeName;
	KeyAttributeName = true;
	CheckKeyAttributeName.clear();
	CheckKeyAttributeName = "title";

	MafReturnT returnValue = registerClass(componentName, mocPath);

	bool TestStatus = true;
	if (returnValue != MafOk) TestStatus = false;
	EXPECT_EQ ( true, TestStatus );

	returnValue = unregisterClass(componentName, mocPath);

	if (returnValue != MafOk) TestStatus = false;
	EXPECT_EQ ( true, TestStatus );

	//delete employeeAttrs;
	//delete FileMMoc;
	//delete model;
	saImmOmSearchNext_2_returnValue = SA_AIS_OK;
}

TEST(OamSARegisterObjectInterface, RegisterOneSimpleClassNotExistInModel)
{
	// Test to register a class named FileM in IMM.

	// Clear the Model Repository
	theTranslator.ResetMOMRoot();
	// setup ModelRepository
	struct MafOamSpiMoAttribute* employeeAttrs = maf_makeAttribute("title",MafOamSpiMoAttributeType_STRING,NULL);
	struct MafOamSpiMoc* FileMMoc = maf_makeMoc("FileM", employeeAttrs);
	struct MafOamSpiMoAttribute* employeeAttrs_2 = maf_makeAttribute("title_2",MafOamSpiMoAttributeType_STRING,NULL);
	struct MafOamSpiMoc* FileMMoc_2 = maf_makeMoc("FileM_2", employeeAttrs_2);
	struct MafOamSpiContainment* cont_1 = maf_makeContainment(FileMMoc_2, FileMMoc, NULL, NULL);
	FileMMoc_2->childContainment = cont_1;
	struct MafOamSpiMoc* FileMMoc_3 = maf_makeMoc("FileM_3", NULL);
	struct MafOamSpiContainment* cont = maf_makeContainment(FileMMoc_3, FileMMoc_2, NULL, cont_1);
	FileMMoc_3->childContainment = cont;

	struct MafOamSpiMom* model = maf_makeMom("FileM_3",FileMMoc_3);

	maf_setMom(model);

	{
		std::vector<std::string> className;
		className.push_back("FileMMoc");
		unsigned int IMM_STRING_TYPE=9;
		unsigned int IMM_REF_TYPE = 6;
		immStorageG.addToImmStorage("FileMMocId=2,FileMMoc_2Id=2,FileMMoc_3Id=2","SaImmAttrClassName",IMM_STRING_TYPE,className);
		// add attribute defintions to class
		immStorageG.addImmClassAttributeDef(std::string("FileMMoc"), std::string("title"), IMM_STRING_TYPE);
	}

	// setUp the MOCK for the ProcessUtil class.

	std::string Output = "";
	mymockProcessUtil = new aaservice::MockProcessUtil(Output,0);
	std::auto_ptr<aaservice::MockProcessUtil> x(mymockProcessUtil);

	const char * mocPath = "/FileMNOTEXIST";
	// Set a flag to enable the SA_IMM_ATTR_RDN flag on the attributes so that the
	// registration of the class below finds a keyAttribute name!

	extern bool KeyAttributeName;
	extern std::string CheckKeyAttributeName;
	KeyAttributeName = true;
	CheckKeyAttributeName.clear();
	CheckKeyAttributeName = "title";

	saImmOiClassImplementerSetChoice = SA_AIS_ERR_NOT_EXIST;

	MafReturnT returnValue = registerClass(componentName, mocPath);

	bool TestStatus = true;
	if (returnValue != MafFailure) TestStatus = false;
	EXPECT_EQ ( true, TestStatus );

	//delete cont;
	//delete FileMMoc_3;
	//delete cont_1;
	//delete FileMMoc_2;
//	delete employeeAttrs_2;
//	delete FileMMoc;
//	delete employeeAttrs;
//	delete model;
}

TEST(OamSARegisterObjectInterface, RegisterSameClassTwice)
{
	// Test to register a class named FileM in IMM.

	// Clear the Model Repository
	theTranslator.ResetMOMRoot();
	// setup ModelRepository
	struct MafOamSpiMoAttribute* employeeAttrs = maf_makeAttribute("title",MafOamSpiMoAttributeType_STRING,NULL);
	struct MafOamSpiMoc* FileMMoc = maf_makeMoc("FileMTwo", employeeAttrs);
	struct MafOamSpiMom* model = maf_makeMom("FileMTwo",FileMMoc);
	maf_setMom(model);
	saImmOmSearchNext_2_returnValue = SA_AIS_ERR_NOT_EXIST;

	{
		// add the class name attribute for Employee with two attributes "title" and "person"
		std::vector<std::string> className;
		className.push_back("FileMTwo");
		unsigned int IMM_STRING_TYPE=9;
		unsigned int IMM_REF_TYPE = 6;
		immStorageG.addToImmStorage("employeeId=2,meId=1","SaImmAttrClassName",IMM_STRING_TYPE,className);
		// add attribute defintions to class
		immStorageG.addImmClassAttributeDef(std::string("FileMTwo"), std::string("title"), IMM_STRING_TYPE);
	}

	// setUp the MOCK for the ProcessUtil class.
	std::string Output = "";
	mymockProcessUtil = new aaservice::MockProcessUtil(Output,0);
	std::auto_ptr<aaservice::MockProcessUtil> x(mymockProcessUtil);

	MafMgmtSpiInterface_1T managedObjectInterfaceId = {"FirstComponent",
		"OamSARegisterObjectInterface",
		"1"};
	MafMgmtSpiInterface_1T transactionalResourceId = {"SecondComponent",
		"OamTransactionalInterface",
		"1"};
	const char * mocPath = "/FileMTwo";
	bool TestStatus = true;

	// Set a flag to enable the SA_IMM_ATTR_RDN flag on the attributes so that the
	// registration of the class below finds a keyAttribute name!

	extern bool KeyAttributeName;
	extern std::string CheckKeyAttributeName;
	KeyAttributeName = true;
	CheckKeyAttributeName.clear();
	CheckKeyAttributeName = "title";

	// Setup IMM registration to return SA_AIS_OK
	saImmOiClassImplementerSetChoice = SA_AIS_OK;

	MafReturnT returnValue = registerClass(componentName, mocPath);

	MafReturnT returnValue1 = registerClass(componentName, mocPath);

	if (returnValue != MafOk && returnValue1 != MafAlreadyExist) TestStatus = false;
	EXPECT_EQ ( true, TestStatus );

	returnValue = unregisterClass(componentName, mocPath);
	if (returnValue != MafOk) TestStatus = false;
	EXPECT_EQ ( true, TestStatus );

//	delete employeeAttrs;
//	delete FileMMoc;
//	delete model;
	saImmOmSearchNext_2_returnValue = SA_AIS_OK;
}

TEST(OamSARegisterObjectInterface, saImmOiClassImplementerSetFailure)
{
	// Test to register a class named FileM in IMM. But call saImmOiClassImplementerSet
	// Fails
	// Returns SA_AIS_OK
	saImmOiClassImplementerSetChoice = SA_AIS_ERR_FAILED_OPERATION;

	// Clear the Model Repository
	theTranslator.ResetMOMRoot();
	// setup ModelRepository
	struct MafOamSpiMoc* FileMMoc = maf_makeMoc("FileMOne", NULL);
	struct MafOamSpiMom* model = maf_makeMom("FileMOne",FileMMoc);
	maf_setMom(model);
	saImmOmSearchNext_2_returnValue = SA_AIS_ERR_NOT_EXIST;

	// setUp the MOCK for the ProcessUtil class.

	std::string Output = "";
	mymockProcessUtil = new aaservice::MockProcessUtil(Output,0);
	std::auto_ptr<aaservice::MockProcessUtil> x(mymockProcessUtil);

	const char * mocPath = "/FileMOne";
	bool TestStatus = true;
	MafReturnT returnValue = registerClass(componentName, mocPath);


	if (returnValue != MafNotExist) TestStatus = false;
	EXPECT_EQ ( true, TestStatus );

//	delete FileMMoc;
//	delete model;
	saImmOmSearchNext_2_returnValue = SA_AIS_OK;
}

TEST(OamSARegisterObjectInterface, TestregisterDn)
{
	// Call Register Dn for a simple structure like this "Me=1,Employee=1"
	// Check that this is returned as MafOk

	/*
	* SetUp the prerequisites
	*/
	// Clear the Model Repository
	theTranslator.ResetMOMRoot();

	// Setup ModelRepository with Me base class and Sub class Employee containing a struct type PersonData

	//	Me<<EcimClass>,

	//	 Employee<<EcimClass>>
	//			title:string
	struct MafOamSpiMoAttribute* employeeAttrs = maf_makeAttribute("title",MafOamSpiMoAttributeType_STRING,NULL);
	struct MafOamSpiMoAttribute* employeeAttrs1 = maf_makeAttribute("title1",MafOamSpiMoAttributeType_STRING,employeeAttrs);
	struct MafOamSpiMoAttribute* employeeAttrs2 = maf_makeAttribute("title2",MafOamSpiMoAttributeType_STRING,employeeAttrs1);
	struct MafOamSpiMoAttribute* employeeAttrs3 = maf_makeAttribute("title3",MafOamSpiMoAttributeType_STRING,employeeAttrs2);
	struct MafOamSpiMoAttribute* employeeAttrs4 = maf_makeAttribute("title4",MafOamSpiMoAttributeType_STRING,employeeAttrs3);

	struct MafOamSpiMoc* meMoc = maf_makeMoc("Me", NULL);
	std::string momName="MOMNAMETEST";
	struct MafOamSpiMoc* employeeMoc = maf_makeMoc("Employee",employeeAttrs4,momName);
	struct MafOamSpiContainment* cont = maf_makeContainment(meMoc, employeeMoc);
	struct MafOamSpiMom* mom = maf_makeMom("Me",meMoc);
	theTranslator.utMoc = meMoc;
	theTranslator.imm_name_update = "employeeId=1,meId=1";

	maf_setMom(mom);

	// setUp the MOCK for the ProcessUtil class.

	std::string Output = "";
	mymockProcessUtil = new aaservice::MockProcessUtil(Output,0);
	std::auto_ptr<aaservice::MockProcessUtil> x(mymockProcessUtil);

	// Set up IMM
	saImmOiClassImplementerSetChoice = SA_AIS_OK;

	// Define the COM OI interface names.
	MafMgmtSpiInterface_1T managedObjectInterfaceId = {"FirstComponent",
		"OamSARegisterObjectInterface",
		"1"};
	MafMgmtSpiInterface_1T transactionalResourceId = {"SecondComponent",
		"OamTransactionalInterface",
		"1"};

	const char * dn = "Me=1,Employee=1";
	bool TestStatus = true;



	extern bool KeyAttributeName;
	extern std::string CheckKeyAttributeName;
	KeyAttributeName = true;
	CheckKeyAttributeName.clear();
	CheckKeyAttributeName = "title";


	MafReturnT returnValue = registerDn(componentName, dn);

	// Check that we received the correct data in IMM.
	if (recievedData_saImmOiObjectImplementerSet != std::string("employeeId=1,meId=1")) TestStatus = false;
	if (returnValue != MafOk) TestStatus = false;
	// Check result
	EXPECT_EQ ( true, TestStatus );
	// Finally check that if
	// saImmOibjectImplementerSet_returnvalue returns SA_AIS_ERR_FAILED_OPERATION;
	// we get the return value MafNotExist

	saImmOibjectImplementerSet_returnvalue = SA_AIS_ERR_FAILED_OPERATION;
	returnValue = registerDn(componentName, dn);

	if (returnValue != MafAlreadyExist) TestStatus = false;
	EXPECT_EQ ( true, TestStatus );

	returnValue = unregisterDn(componentName, dn);
	if (returnValue != MafOk) TestStatus = false;
	EXPECT_EQ ( true, TestStatus );

	// deallocate resources.
//	delete employeeAttrs;
//	delete employeeAttrs1;
//	delete employeeAttrs2;
//	delete employeeAttrs3;
//	delete employeeAttrs4;
//	delete meMoc;
//	delete employeeMoc->mom;
//	delete employeeMoc;
//	delete cont;
//	delete mom;

}


TEST(OamSARegisterObjectInterface, Register_ECIMStructs)
{
	//
	// OK, in this testcase we try to register a DN to a object that contains an struct attribute
	// This will end up in a IMM registration to both the DN to the Object requested, but also register
	// callbacks for all object implementing the struct values.
	//

	//
	// SetUp the prerequisites
	//
	// Clear the Model Repository
	theTranslator.ResetMOMRoot();

	// Setup ModelRepository with Me base class and Sub class Employee containing a struct type PersonData

	//	Me<<EcimClass>,
	//	 Employee<<EcimClass>>
	//			title:string
	//			person:PersonData
	//
	//	PersonData<<EcimStruct>>{
	//				firstname:string;
	//				lastname:string;
	//		}

	struct MafOamSpiStructMember* mLastname = maf_makeStructMember("lastname", MafOamSpiDatatype_STRING,NULL);
	struct MafOamSpiStructMember* mFirstname = maf_makeStructMember("firstname", MafOamSpiDatatype_STRING, mLastname);
	struct MafOamSpiStruct* personData = maf_makeStruct("PersonData", mFirstname);
	struct MafOamSpiMoAttribute* attrPerson = maf_makeAttribute("person",personData, NULL);
	struct MafOamSpiMoAttribute* employeeAttrs = maf_makeAttribute("title",MafOamSpiMoAttributeType_STRING,attrPerson);
	struct MafOamSpiMoc* meMoc = maf_makeMoc("Me", NULL);
	std::string momName="MOMNAMETEST";
	struct MafOamSpiMoc* employeeMoc = maf_makeMoc("Employee",employeeAttrs,momName);
	struct MafOamSpiContainment* cont = maf_makeContainment(meMoc, employeeMoc);
	// OK, set the containment in place in the meMoc memory
	meMoc->childContainment=cont;
	struct MafOamSpiMom* mom = maf_makeMom("Me", meMoc);
	theTranslator.utMoc = employeeMoc;
	maf_setMom(mom);
	theTranslator.imm_name_update = "employeeId=2,meId=1";

	// setup IMM storage
	immStorageG.reset();
	{
		// add Employee instance 1 attribute: title
		std::vector<std::string> values;
		values.push_back(std::string("President"));
		immStorageG.addToImmStorage("employeeId=2,meId=1","title",9,values);
	}
	{
		// add Employee instance 1 attribute: person (a reference to the struct instance!)
		std::vector<std::string> values;
		values.push_back(std::string("id=person_1,employeeId=2,meId=1"));
		unsigned int IMM_SA_NAME=6;
		immStorageG.addToImmStorage("employeeId=2,meId=1","person",IMM_SA_NAME,values);
	}
	{
		// add personData instance person_1 attribute: firstname
		std::vector<std::string> values;
		values.push_back(std::string("Bob"));
		immStorageG.addToImmStorage("id=person_1,employeeId=2,meId=1","firstname",9,values);
	}
	{
		// add personData instance person_1 attribute: lastname
		std::vector<std::string> values;
		values.push_back(std::string("Dole"));
		immStorageG.addToImmStorage("id=person_1,employeeId=2,meId=1","lastname",9,values);
	}

	{
		// add the class name attribute for Employee with two attributes "title" and "person"
		std::vector<std::string> className;
		className.push_back("Employee");
		unsigned int IMM_STRING_TYPE=9;
		unsigned int IMM_REF_TYPE = 6;
		immStorageG.addToImmStorage("employeeId=2,meId=1","SaImmAttrClassName",IMM_STRING_TYPE,className);
		// add attribute defintions to class
		immStorageG.addImmClassAttributeDef(std::string("Employee"), std::string("title"), IMM_STRING_TYPE);
		immStorageG.addImmClassAttributeDef(std::string("Employee"), std::string("person"), IMM_REF_TYPE);
	}
	{
		// add the class name attribute for PersonData with two attributes "firstname" and "lastname"
		std::vector<std::string> className;
		className.push_back("PersonData");
		unsigned int IMM_STRING_TYPE=9;
		immStorageG.addToImmStorage("id=person_1,employeeId=2,meId=1","SaImmAttrClassName",IMM_STRING_TYPE,className);
		// add attribute defintions to class
		immStorageG.addImmClassAttributeDef(std::string("PersonData"), std::string("firstname"), IMM_STRING_TYPE);
		immStorageG.addImmClassAttributeDef(std::string("PersonData"), std::string("lastname"), IMM_STRING_TYPE);
	}
	// add the class name attribute for Me
	{
		std::vector<std::string> className;
		className.push_back("Me");
		unsigned int IMM_STRING_TYPE=9;
		immStorageG.addToImmStorage("meId=1","SaImmAttrClassName",IMM_STRING_TYPE,className);
	}

	// setUp the MOCK for the ProcessUtil class.
	std::string Output = "";
	mymockProcessUtil = new aaservice::MockProcessUtil(Output,0);
	std::auto_ptr<aaservice::MockProcessUtil> x(mymockProcessUtil);

	// Set up IMM
	saImmOiObjectImplementerRelease_returnvalue = SA_AIS_OK;
	saImmOibjectImplementerSet_returnvalue = SA_AIS_OK;


	// Define the COM OI interface names.
	MafMgmtSpiInterface_1T managedObjectInterfaceId = {"FirstComponent",
		"OamSARegisterObjectInterface",
		"1"};
	MafMgmtSpiInterface_1T transactionalResourceId = {"SecondComponent",
		"OamTransactionalInterface",
		"1"};
	char * dn = "Me=1,Employee=2";
	bool TestStatus = true;

	//extern bool KeyAttributeName;
	//extern std::string CheckKeyAttributeName;
	KeyAttributeName = true;
	CheckKeyAttributeName.clear();
	CheckKeyAttributeName = "title";

	recievedData_saImmOiObjectImplementerSetList.clear();

	//
	// NOW REGISTER
	//

	MafReturnT returnValue = registerDn(componentName, dn);


	std::list<std::string>::iterator iterator=recievedData_saImmOiObjectImplementerSetList.begin();
	if ((*iterator).size()!=0) {
		ASSERT_TRUE ((std::string)"employeeId=2,meId=1" == (*iterator) );
	}else {
		ASSERT_TRUE(true==false);
	}


	// OK; check that the struct class is registered.
	ASSERT_TRUE( saImmOiClassImplementerSet_receivedRegistration == "PersonData" );


	saImmOiClassImplementerSet_receivedRegistration.clear();

	// Test to register the class
	char * mocPath = "/Me/Employee";
	// Set a flag to enable the SA_IMM_ATTR_RDN flag on the attributes so that the
	// registration of the class below finds a keyAttribute name!

	saImmOiClassImplementerSetChoice=SA_AIS_OK;
	theTranslator.utMoc = meMoc;
	saImmOmSearchNext_2_returnValue = SA_AIS_ERR_NOT_EXIST;

	returnValue = registerClass(componentName, mocPath);

	// OK; check that the struct class is registered.
	ASSERT_TRUE( saImmOiClassImplementerSet_receivedRegistration == "Employee" );

	// OK, now try to unregister the DN and the class.

	returnValue = unregisterDn(componentName, dn);

	ASSERT_TRUE( recievedData_saImmOiObjectImplementerRelease == "employeeId=2,meId=1" );

	returnValue = unregisterClass(componentName, mocPath);

	ASSERT_TRUE( recievedData_saImmOiClassImplementerRelease == "PersonData");

//	delete mLastname;
//	delete mFirstname;
//	delete personData;
//	delete attrPerson;
//	delete employeeAttrs;
//	delete meMoc;
//	delete employeeMoc->mom;
//	delete employeeMoc;
//	delete cont;
//	delete mom;
	saImmOmSearchNext_2_returnValue = SA_AIS_OK;
}


TEST(OamSARegisterObjectInterface, TestregisterDn_complex_multivalue)
{

	//
	// OK, in this testcase we try to register a DN to a object that contains an struct attribute
	// This will end up in a IMM registration to both the DN to the Object requested, but also register
	// callbacks for all object implementing the struct values.
	//

	//
	// SetUp the prerequisites
	//
	// Clear the Model Repository
	theTranslator.ResetMOMRoot();

	// Setup ModelRepository with Me base class and Sub class Employee containing a struct type PersonData

	//	Me1<<EcimClass>,
	//		Employee1<<EcimClass>>
	//			title:string
	//			person:PersonData
	//
	//	PersonData<<EcimStruct>>{
	//				firstname:string;
	//				lastname:string;
	//		}
	// But now we will define in the IMM that we have multiple values of the SANAMET attribute referencing to the
	// objects implementing the structs values.

	struct MafOamSpiStructMember* mLastname = maf_makeStructMember("lastname", MafOamSpiDatatype_STRING,NULL);
	struct MafOamSpiStructMember* mFirstname = maf_makeStructMember("firstname", MafOamSpiDatatype_STRING, mLastname);
	struct MafOamSpiStruct* personData = maf_makeStruct("PersonData", mFirstname);
	struct MafOamSpiMoAttribute* attrPerson = maf_makeAttribute("person",personData, NULL);
	struct MafOamSpiMoAttribute* employeeAttrs = maf_makeAttribute("title",MafOamSpiMoAttributeType_STRING,attrPerson);
	struct MafOamSpiMoc* meMoc = maf_makeMoc("Me", NULL);
	std::string momName="MOMNAMETEST";
	struct MafOamSpiMoc* employeeMoc = maf_makeMoc("Employee",employeeAttrs,momName);
	struct MafOamSpiContainment* cont =maf_makeContainment(meMoc, employeeMoc);
	struct MafOamSpiMom* mom = maf_makeMom("Me",meMoc);
	maf_setMom(mom);
	theTranslator.utMoc = employeeMoc;
	theTranslator.imm_name_update = "employeeId=3,meId=1";

	// setup IMM storage
	immStorageG.reset();
	{
		// add Employee instance 1 attribute: title
		std::vector<std::string> values;
		values.push_back(std::string("President"));
		immStorageG.addToImmStorage("employeeId=3,meId=1","title",9,values);
	}
	{
		// add Employee instance 1 attribute: person (a reference to the struct instance!)
		std::vector<std::string> values;
		values.push_back(std::string("id=person_1,employeeId=3,meId=1"));
		values.push_back(std::string("id=person_2,employeeId=3,meId=1"));
		unsigned int IMM_SA_NAME=6;
		immStorageG.addToImmStorage("employeeId=3,meId=1","person",IMM_SA_NAME,values);
	}
	{
		// add personData instance person_1 attribute: firstname
		std::vector<std::string> values;
		values.push_back(std::string("Bob"));
		immStorageG.addToImmStorage("id=person_1,employeeId=3,meId=1","firstname",9,values);
		immStorageG.addToImmStorage("id=person_2,employeeId=3,meId=1","firstname",9,values);
	}
	{
		// add personData instance person_1 attribute: lastname
		std::vector<std::string> values;
		values.push_back(std::string("Dole"));
		immStorageG.addToImmStorage("id=person_1,employeeId=3,meId=1","lastname",9,values);
		immStorageG.addToImmStorage("id=person_2,employeeId=3,meId=1","lastname",9,values);
	}

	{
		// add the class name attribute for Employee with two attributes "title" and "person"
		std::vector<std::string> className;
		className.push_back("Employee");
		unsigned int IMM_STRING_TYPE=9;
		unsigned int IMM_REF_TYPE = 6;
		immStorageG.addToImmStorage("employeeId=3,meId=1","SaImmAttrClassName",IMM_STRING_TYPE,className);
		// add attribute definitions to class
		immStorageG.addImmClassAttributeDef(std::string("Employee"), std::string("title"), IMM_STRING_TYPE);
		immStorageG.addImmClassAttributeDef(std::string("Employee"), std::string("person"), IMM_REF_TYPE);
	}
	{
		// add the class name attribute for PersonData with two attributes "firstname" and "lastname"
		std::vector<std::string> className;
		className.push_back("PersonData");
		unsigned int IMM_STRING_TYPE=9;
		immStorageG.addToImmStorage("id=person_1,employeeId=3,meId=1","SaImmAttrClassName",IMM_STRING_TYPE,className);
		immStorageG.addToImmStorage("id=person_2,employeeId=3,meId=1","SaImmAttrClassName",IMM_STRING_TYPE,className);
		// add attribute definitions to class
		immStorageG.addImmClassAttributeDef(std::string("PersonData"), std::string("firstname"), IMM_STRING_TYPE);
		immStorageG.addImmClassAttributeDef(std::string("PersonData"), std::string("lastname"), IMM_STRING_TYPE);
	}
	// add the class name attribute for Me
	{
		std::vector<std::string> className;
		className.push_back("Me");
		unsigned int IMM_STRING_TYPE=9;
		immStorageG.addToImmStorage("meId=1","SaImmAttrClassName",IMM_STRING_TYPE,className);
	}

	// setUp the MOCK for the ProcessUtil class.
	std::string Output = "";
	mymockProcessUtil = new aaservice::MockProcessUtil(Output,0);
	std::auto_ptr<aaservice::MockProcessUtil> x(mymockProcessUtil);

	// Set up IMM
	saImmOiObjectImplementerRelease_returnvalue = SA_AIS_OK;
	saImmOibjectImplementerSet_returnvalue = SA_AIS_OK;


	// Define the COM OI interface names.
	MafMgmtSpiInterface_1T managedObjectInterfaceId = {"FirstComponent",
		"OamSARegisterObjectInterface",
		"1"};
	MafMgmtSpiInterface_1T transactionalResourceId = {"SecondComponent",
		"OamTransactionalInterface",
		"1"};
	const char * dn = "Me=1,Employee=3";


	extern bool KeyAttributeName;
	extern std::string CheckKeyAttributeName;
	KeyAttributeName = true;
	CheckKeyAttributeName.clear();
	CheckKeyAttributeName = "title";

	recievedData_saImmOiObjectImplementerSetList.clear();
	saImmOiClassImplementerSet_receivedRegistration.clear();

	//
	// NOW REGISTER
	//
	MafReturnT returnValue = registerDn(componentName, dn);

	bool TestStatus = true;

	std::list<std::string>::iterator iterator=recievedData_saImmOiObjectImplementerSetList.begin();

	ASSERT_TRUE ((std::string)"employeeId=3,meId=1" == (*iterator) );

	ASSERT_TRUE( saImmOiClassImplementerSet_receivedRegistration == "PersonData" );

	returnValue = unregisterDn(componentName, dn);
	if (returnValue != MafOk) TestStatus = false;
	EXPECT_EQ ( true, TestStatus );

//	delete mLastname;
//	delete mFirstname;
//	delete personData;
//	delete attrPerson;
//	delete employeeAttrs;
//	delete meMoc;
//	delete employeeMoc->mom;
//	delete employeeMoc;
//	delete cont;
//	delete mom;
}


TEST(OamSARegisterObjectInterface, TestunregisterDn)
{
	//
	// THIS TEST ASSUMES THAT A REGISTRATION HAS OCCURED SEE BELOW!
	//

	//
	// SetUp the prerequisites
	//
	// Clear the Model Repository
	theTranslator.ResetMOMRoot();

	// Setup ModelRepository with Me base class and Sub class Employee containing a struct type PersonData

	//	Me1<<EcimClass>,
	//	 Employee1<<EcimClass>>
	//			title:string
	//			person:PersonData
	//
	//	PersonData<<EcimStruct>>{
	//				firstname:string;
	//				lastname:string;
	//		}

	struct MafOamSpiStructMember* mLastname = maf_makeStructMember("lastname", MafOamSpiDatatype_STRING,NULL);
	struct MafOamSpiStructMember* mFirstname = maf_makeStructMember("firstname", MafOamSpiDatatype_STRING, mLastname);
	struct MafOamSpiStruct* personData = maf_makeStruct("PersonData", mFirstname);
	struct MafOamSpiMoAttribute* attrPerson = maf_makeAttribute("person",personData, NULL);
	struct MafOamSpiMoAttribute* employeeAttrs = maf_makeAttribute("title",MafOamSpiMoAttributeType_STRING,attrPerson);
	struct MafOamSpiMoc* meMoc = maf_makeMoc("Me1", NULL);
	std::string momName="MOMNAMETEST";
	struct MafOamSpiMoc* employeeMoc = maf_makeMoc("Employee1",employeeAttrs,momName);
	struct MafOamSpiContainment* cont =maf_makeContainment(meMoc, employeeMoc);
	struct MafOamSpiMom* mom = maf_makeMom("Me1",meMoc);
	theTranslator.imm_name_update = "employee1Id=1,me1Id=1";
	theTranslator.utMoc = meMoc;
	maf_setMom(mom);

	// setUp the MOCK for the ProcessUtil class.
	std::string Output = "";
	mymockProcessUtil = new aaservice::MockProcessUtil(Output,0);
	std::auto_ptr<aaservice::MockProcessUtil> x(mymockProcessUtil);

	// Set up IMM
	saImmOiObjectImplementerRelease_returnvalue = SA_AIS_OK;
	saImmOibjectImplementerSet_returnvalue = SA_AIS_OK;

	const char * dn = "Me1=1,Employee1=1";
	bool TestStatus = true;

	extern bool KeyAttributeName;
	extern std::string CheckKeyAttributeName;
	KeyAttributeName = true;
	CheckKeyAttributeName.clear();
	CheckKeyAttributeName = "title";

	//
	// NOW REGISTER
	//

	MafReturnT returnValue = registerDn(componentName, dn);

	// Test to unregister a Dn that is registered by another component,
	// Expect MafNotExist as result

	TestStatus = true;
	recievedData_saImmOiObjectImplementerRelease.clear();
	returnValue = unregisterDn(Not_componentName, dn);

	// Check that we received the correct data in IMM.
	if (recievedData_saImmOiObjectImplementerRelease.size() != 0) TestStatus = false;
	if (returnValue != MafFailure) TestStatus = false;
	// Check result
	EXPECT_EQ ( true, TestStatus );

	// Test To unregister a Dn that has been registered before.
	// Expect MafOk as result

	returnValue = unregisterDn(componentName, dn);
	// Check that we received the correct data in IMM.
	if (recievedData_saImmOiObjectImplementerRelease != std::string("employee1Id=1,me1Id=1")) TestStatus = false;
	if (returnValue != MafOk) TestStatus = false;
	// Check result
	EXPECT_EQ ( true, TestStatus );
	//
	// Register Again
	//
	returnValue = registerDn(componentName, dn);

	// Test To unregister a Dn and receive SA_AIS_ERR_TRY_AGAIN from saImmOiObjectImplementerRelease
	// 20 times, then receive SA_AIS_OK
	// Expect MafOk as result
	// Set up IMM
	saImmOiObjectImplementerRelease_returnvalue = SA_AIS_ERR_TRY_AGAIN;
	TestStatus = true;
	recievedData_saImmOiObjectImplementerRelease.clear();
	returnValue = unregisterDn(componentName, dn);
	// Check that we received the correct data in IMM.
	if (returnValue != MafOk) TestStatus = false;
	if (recievedData_saImmOiObjectImplementerRelease != std::string("employee1Id=1,me1Id=1")) {
		TestStatus = false;
	}
	// Check result
	EXPECT_EQ ( true, TestStatus );

//	delete mLastname;
//	delete mFirstname;
//	delete personData;
//	delete attrPerson;
//	delete employeeAttrs;
//	delete meMoc;
//	delete employeeMoc->mom;
//	delete employeeMoc;
//	delete cont;
//	delete mom;
}

TEST(OamSARegisterObjectInterface, TestunregisterClass)
{
	//
	// Test To:
	// 1. Register a Class.
	// 	 * 2. unregister the class.
	// 3. unregister the same class again.
	//

	// Test to register a class named FileM in IMM.
	// Clear the Model Repository
	theTranslator.ResetMOMRoot();
	// setup ModelRepository
	struct MafOamSpiMoAttribute* employeeAttrs = maf_makeAttribute("title",MafOamSpiMoAttributeType_STRING,NULL);
	struct MafOamSpiMoc* FileMMoc = maf_makeMoc("FileMUN", employeeAttrs);
	struct MafOamSpiMom* model = maf_makeMom("FileMUN",FileMMoc);
	maf_setMom(model);

	{
		// add the class name attribute for Employee with two attributes "title" and "person"
		std::vector<std::string> className;
		className.push_back("FileMUN");
		unsigned int IMM_STRING_TYPE=9;
		unsigned int IMM_REF_TYPE = 6;
		immStorageG.addToImmStorage("fileMUNId=2","SaImmAttrClassName",IMM_STRING_TYPE,className);
		// add attribute defintions to class
		immStorageG.addImmClassAttributeDef(std::string("FileMUN"), std::string("title"), IMM_STRING_TYPE);
	}

	// setUp the MOCK for the ProcessUtil class.
	std::string Output = "";
	mymockProcessUtil = new aaservice::MockProcessUtil(Output,0);
	std::auto_ptr<aaservice::MockProcessUtil> x(mymockProcessUtil);
	const char * mocPath = "/FileMUN";

	// Set a flag to enable the SA_IMM_ATTR_RDN flag on the attributes so that the
	// registration of the class below finds a keyAttribute name!

	extern bool KeyAttributeName;
	extern std::string CheckKeyAttributeName;
	KeyAttributeName = true;
	CheckKeyAttributeName.clear();
	CheckKeyAttributeName = "title";
	saImmOmSearchNext_2_returnValue = SA_AIS_ERR_NOT_EXIST;

	MafReturnT returnValue = registerClass(componentName, mocPath);
	bool TestStatus = true;
	if (returnValue != MafOk) TestStatus = false;
	EXPECT_EQ ( true, TestStatus );

	returnValue = unregisterClass(componentName, mocPath);
	if (returnValue != MafOk) TestStatus = false;
	EXPECT_EQ ( true, TestStatus );

	returnValue = unregisterClass(componentName, mocPath);
	if (returnValue != MafNotExist) TestStatus = false;
	EXPECT_EQ ( true, TestStatus );

	//
	// Test To:
	// 1. Register a Class
	// 2. unregister the class but the imm returns SA_AIS_ERR_NAME_NOT_FOUND
	// 3. register the class again and fails because it is still registered
	//
	returnValue = registerClass(componentName, mocPath);
	TestStatus = true;
	if (returnValue != MafOk) TestStatus = false;
	EXPECT_EQ ( true, TestStatus );

	saImmOiClassImplementerRelease_returnvalue = SA_AIS_ERR_NAME_NOT_FOUND;
	returnValue = unregisterClass(componentName, mocPath);

	if (returnValue != MafFailure) TestStatus = false;
	EXPECT_EQ ( true, TestStatus );

	returnValue = registerClass(componentName, mocPath);
	TestStatus = true;
	if (returnValue != MafAlreadyExist) TestStatus = false;
	EXPECT_EQ ( true, TestStatus );
	//
	// Test To:
	// 1. Register a class. This is true from previous test case.
	// 2. unregister the class but return SA_AIS_ERR_TRY_AGAIN from saImmOiClassImplementerRelease
	// 3. Change to SA_AIS_OK after 20 times.
	//
	saImmOiClassImplementerRelease_returnvalue = SA_AIS_ERR_TRY_AGAIN;
	returnValue = unregisterClass(componentName, mocPath);
	if (returnValue != MafOk) TestStatus = false;
	EXPECT_EQ ( true, TestStatus );

//	delete employeeAttrs;
//	delete FileMMoc;
//	delete model;
	saImmOmSearchNext_2_returnValue = SA_AIS_OK;
}

MafReturnT getComponentInterfaceArray(const char * componentName,
															const char * interface,
															MafMgmtSpiInterface_1T ***result)
{
	printf("DEBUG:	getComponentInterfaceArray_unittest IN \n");
	/* Validate the componentName */
	if(0 == strcmp(componentName, "\0"))
	{
		printf("ERROR: componenName is invalid");
		printf("DEBUG:	getComponentInterfaceArray_unittest OUT \n");
		return MafFailure;
	}

	MafMgmtSpiInterface_1T  base_MO = {componentName,
														MafOamSpiManagedObject_3Id.interfaceName,
														MafOamSpiManagedObject_3Id.interfaceVersion};
	MafMgmtSpiInterface_1T* p_baseMO = &base_MO;

	MafMgmtSpiInterface_1T  base_TxR = {componentName,
														MafOamSpiTransactionalResource_2Id.interfaceName,
														MafOamSpiTransactionalResource_2Id.interfaceVersion};
	MafMgmtSpiInterface_1T* p_baseTxR = &base_TxR;

	if(0 == strcmp("MafOamSpiManagedObject", interface))
	{
		MafMgmtSpiInterface_1T **array = (MafMgmtSpiInterface_1T **)malloc(sizeof(MafMgmtSpiInterface_1T *) * 2);
		if(array)
		{
			memset(array, 0, sizeof(MafMgmtSpiInterface_1T *) * 2);
			array[0] = p_baseMO;
			array[1] = NULL;
			*result = array;
		}
		else
		{
			return MafFailure;
		}
	}
	else if(0 == strcmp("MafOamSpiTransactionalResource", interface))
	{
		MafMgmtSpiInterface_1T **array = (MafMgmtSpiInterface_1T **)malloc(sizeof(MafMgmtSpiInterface_1T *) * 2);
		if(array)
		{
			memset(array, 0, sizeof(MafMgmtSpiInterface_1T *) * 2);
			array[0] = p_baseMO;
			array[1] = NULL;
			*result = array;
		}
		else
		{
			return MafFailure;
		}
	}
	// In case interface is not specified, the returned array should contains all registered interfaces of component
	else
	{
		MafMgmtSpiInterface_1T **array = (MafMgmtSpiInterface_1T **)malloc(sizeof(MafMgmtSpiInterface_1T *) * 3);
		if(array)
		{
			memset(array, 0, sizeof(MafMgmtSpiInterface_1T *) * 3);
			array[0] = p_baseMO;
			array[1] = p_baseTxR;
			array[2] = NULL;
			*result = array;
		}
		else
		{
			return MafFailure;
		}
	}
	printf("DEBUG:	getComponentInterfaceArray_unittest OUT \n");
	return MafOk;
}
