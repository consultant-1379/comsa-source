/*
* OIProxy_handleAdminOperation.cc
* Test environment for function OIProxy_handleAdminOperation() in OIProxy
*
*  Created on: Nov 17, 2011
*      Author: uabjoy
*/
#include <gtest/gtest.h>
//COM
#include <MafMgmtSpiCommon.h>
#include <MafMgmtSpiComponent_1.h>
#include <MafMgmtSpiInterfacePortal_1.h>
#include <MafMgmtSpiServiceIdentities_1.h>
#include <MafMgmtSpiThreadContext_2.h>
#include <MafOamSpiModelRepository_4.h>
#include <MafOamSpiServiceIdentities_1.h>
#include <MafOamSpiTransactionMaster_2.h>
//COMSA
#include "OamSACache.h"
#include "DxEtModelConstants.h"
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

extern void OIProxy_handleAdminOperation(SaImmOiHandleT immOiHandle, SaInvocationT invocation, const SaNameT *objectName, SaImmAdminOperationIdT operationId, const SaImmAdminOperationParamsT_2 **params);

MafOamSpiGeneralPropertiesT generalProperties;
MafOamSpiGeneralPropertiesT generalProperties_1;
MafOamSpiGeneralPropertiesT generalProperties_param;
MafOamSpiGeneralPropertiesT generalProperties_param_1;

MafOamSpiDatatypeContainerT returnType;
MafOamSpiDatatypeContainerT returnType_param;

MafOamSpiDomainExtensionT domain;
MafOamSpiDomainExtensionT domain_1;

MafOamSpiExtensionT extensions;
MafOamSpiExtensionT extensions_1;


// Test Mock
MafOamSpiMoActionT* GetComActionList(OamSACache::DNList& mo_name) {

	generalProperties.name="actionOne";
	generalProperties_1.name="actionOther";

	generalProperties_param.name="attributeOne";
	generalProperties_param_1.name="attributeTwo";
	returnType_param.type=MafOamSpiDatatype_BOOL;
	//parameters.next=NULL;
	//testMoAction.parameters=&parameters;
	domain.domain   = DOMAIN_NAME_COREMW;
	domain_1.domain = DOMAIN_NAME_COREMW;

	extensions.next  = NULL;
	extensions.value = "52";
	extensions.name  = DOMAIN_EXT_NAME_ADM_OP_ID;

	extensions_1.next  = NULL;
	extensions_1.value = "53";
	extensions_1.name  = DOMAIN_EXT_NAME_ADM_OP_ID;


	domain.extensions=&extensions;
	domain_1.extensions=&extensions_1;
	returnType.type=MafOamSpiDatatype_BOOL;
	returnType_param.type;
	//testMoAction.domainExtension=&domain;
	static MafOamSpiParameterT parameters_1={generalProperties_param_1,NULL,returnType_param,NULL};
	static MafOamSpiParameterT parameters={generalProperties_param,NULL,returnType_param,&parameters_1};
	static MafOamSpiMoActionT testMoAction={generalProperties,NULL,returnType,&parameters,NULL,&domain};

	static MafOamSpiMoActionT testMoAction_1={generalProperties_1,NULL,returnType,&parameters,&testMoAction,&domain_1};


	return &testMoAction_1;

}

bool RetrieveComAttributeType(MafOamSpiDatatypeT &paramType) {
	// First basic test case
	paramType=MafOamSpiDatatype_STRING;
	return true;
}


char TESTCASE_1[100]="PersonData=1";
char TESTCASE_2[100]="PersonData=2";
char TESTCASE_3[100]="Employee=1";
bool Imm2MO_DN(char *DNPath,char** n3gpp_name) {
	if (std::string(DNPath)=="personDataId=2") {
		*n3gpp_name=new char[100];
		strcpy(*n3gpp_name,TESTCASE_2);
		//*n3gpp_name=TESTCASE_2;
	}else if (std::string(DNPath)=="EmployeeId=1") {
		// This memory is freed internally in OIProxy
		*n3gpp_name=new char[100];
		strcpy(*n3gpp_name,TESTCASE_3);
	}else {
		// First test case
		*n3gpp_name=new char[100];
		strcpy(*n3gpp_name,TESTCASE_1);
	}
	return true;
}


// End of Test mock

// FIXME: this TC is not completed implemented, comment out
#if 0
TEST(OIProxy_handleAdminOperation,BasicCall )
{
	// Setup the MOM with a personDataId=1, I use a mock above instead.

	SaImmOiHandleT immOiHandle=47;
	SaInvocationT invocation=447;
	SaNameT objectName;
	strcpy((char*)objectName.value,"personDataId=1");
	objectName.length=strlen("personDataId=1");
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
