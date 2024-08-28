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
 *   File:   OamSAImmBridge.cc
 *
 *   Author: efaiami & egorped
 *
 *   Date:   2010-05-21
 *
 *   This class acts as an interface to imm and imm communication.
 *
 *   Reviewed: efaiami 2010-06-18
 *
 *   Reviewed: efaiami 2011-01-25   code reviewed for CM Action.
 *
 *   Modify: efaiami 2011-02-22  for log and trace function
 *           efaiami 2011-02-19  OamSAFinish(), OamSAAbort() and OamSACommit() fixed for TR: HO17038 OamSAFinish
 *   Modify: eozasaf 2011-07-08  getAndForwardErrorStrings() for forwarding error strings received from IMM OIs to COM
 *   Modify: efaiami 2011-07-18  comSAMgmtSpiThreadContInit() and getting the ThreadContext_2 interface in getAndForwardErrorStrings()
 *   Modify: eozasaf 2011-07-29  added parseAndCategorizeErrorString() in order to separate categorization of
 *								 error strings into a new function to make things easier for unit testing
 *   Modify: efaiami 2011-09-13  added createImmMO(), deleteImmMO(), setImmMoAttr(), createMo(), deleteMO(), setMOAttribute() for CCB
 *   Modify: efaiami 2011-11-19  action() modified for TR: HO93742
 *   Modify: efaiami 2011-11-24  remove  createImmMO(), deleteImmMO(), setMOAttribute()
 *   Modify: ejonajo 2012-01-11  fixed several compilation warnings on 64-bit linux and improved debug printouts
 *   Modify: uabjoy  2012-04-12  Adding support for prefixing of IMM classes and key attributes.
 *   Modify: uabjoy  2012-09-14  Corrected TR HQ23564, adding support for key attribute named anything.
 *   Modify: xnikvap, xngangu, xjonbuc, xduncao 2012-08-30  support for COM MO SPI Ver.3 (Ver.1 is not supported any more)
 *   Modify: xjonbuc 2012-09-06  Implement SDP1694 - support MAF SPI
 *   Modify: xjonbuc 2013-03-21  Fix HQ75768 - modify existsImmMo() to check IMM values
 *   Modify: xnikvap 2012-12-15  Implemented SDP975 - support for struct and multivalue parameters in action()
 *   Modify: xnikvap 2013-01-04  Implemented SDP875 - Error strings from IMM to COM in action()
 *   Modify: xduncao 2013-05-16  Implemented support for any return type from action()
 *   Modify: xadaleg 2013-08-20  MR26712 - Support option to not split IMM DN at EcimContribution
 *   Modify: uabjoy  2013-08-26  Correcting trouble report HR51069
 *   Modify: xdonngu 2013-10-11  Fix HR76134 - modify existsImmMo() to correct the behavior when input invalid dn string
 *   Modify: xdonngu 2013-10-15  Fix HR81490 - Fix parseAndCategorizeErrorString() regular expression pattern.
 *   Modify: xdonngu 2013-10-29  Fix HR82371 - Prevent segment fault caused by invalid handle.
 *   Modify: xanhdao 2013-10-15  MR24146 Supporting Floating Point
 *   Modify: xdonngu 2013-12-12  run cppcheck 1.62 and fix errors and warnings
 *   Modify: xdonngu 2014-05-29  using std::tr1::shared_ptr for TxContext
 *   Modify: xjonbuc 2014-06-23  Implement MR-20275 support for explicit ccb validate() & abort()
 *   Modify: xjonbuc 2014-09-26  Fix TR HS99358 to always do ccb abort() after a ccb handle is initialized.
 *   Modify: xadaleg 2014-08-02  MR35347 - increase DN length
 *   Modify: xjonbuc 2014-10-27  Fix TR HT17443 - only create a ccb if it is not created
 *   Modify: xtronle 2014-12-05  Fix TR HT28574 - No ccb validation should be done by COM SA when ccb is empty
 *   Modify: xvintra 2015-03-24  Fix TR HT45431 - ComSa return wrong error code if another process locked (set admin owner) object
 *   Modify: xnikvap 2015-08-02  MR42277 Remove old interfaces exposed to COM & CoreMW that are no longer supported for SLES12
 *
 * *****************************************************************************************************************/

#include <ComSA.h>
#include <string.h>
#include <set>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <cerrno>
#include <stdlib.h>
#include <regex.h>
#include <cctype>
#include <unistd.h>
#include "MafOamSpiManagedObject_3.h"
#include "MafOamSpiTransaction_2.h"
#include "MafMgmtSpiInterfacePortal_3.h"
#include "MafMgmtSpiServiceIdentities_1.h"
#include "OamSAImmBridge.h"
#include "OamSATranslator.h"
#include "OamSACache.h"
#include "OamSATransactionRepository.h"
#include "OamSATransactionalResource.h"
#include "ImmCmd.h"
#include "DxEtModelConstants.h"
#include "InternalTrace.h"
#include <algorithm>
#include <tr1/memory>

#ifndef UNIT_TEST
	#define IMM_ABORT_RETRIES_COUNT 60
#else
	#define IMM_ABORT_RETRIES_COUNT 2
#endif


using namespace CM;
using namespace std::tr1;
OamSATranslator	theTranslator;


extern MafOamSpiTransaction_2T* MafOamSpiTransactionStruct_p_v2;
extern bool lock_mo_for_config_change;

OamSAImmBridge* OamSAImmBridge::oamSAImmBridge;
OamSAImmBridge theImmBridge;
static MafMgmtSpiInterfacePortal_3T* _portal;
static MafMgmtSpiInterface_1T thContext = MafMgmtSpiThreadContext_2Id;
static MafMgmtSpiThreadContext_2T* threadContext;

/* Non class functions */

static void DebugDumpAttributeContainer(const MafMoAttributeValueContainer_3T * const ctp)
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("Dumping attribute container");
	if (NULL == ctp) {
		DEBUG_OAMSA_TRANSLATIONS("No attribute");
		return;
	}
	DEBUG_OAMSA_TRANSLATIONS("Number of values %d",ctp->nrOfValues);
	switch (ctp->type)
	{
	case MafOamSpiMoAttributeType_3_INT8:
		DEBUG_OAMSA_TRANSLATIONS("MafOamSpiMoAttributeType_3_INT8");
		break;
	case MafOamSpiMoAttributeType_3_INT16:
		DEBUG_OAMSA_TRANSLATIONS("MafOamSpiMoAttributeType_3_INT16");
		break;
	case MafOamSpiMoAttributeType_3_INT32:
		DEBUG_OAMSA_TRANSLATIONS("MafOamSpiMoAttributeType_3_INT32");
		break;
	case MafOamSpiMoAttributeType_3_INT64:
		DEBUG_OAMSA_TRANSLATIONS("MafOamSpiMoAttributeType_3_INT64");
		break;
	case MafOamSpiMoAttributeType_3_UINT8:
		DEBUG_OAMSA_TRANSLATIONS("MafOamSpiMoAttributeType_3_UINT8");
		break;
	case MafOamSpiMoAttributeType_3_UINT16:
		DEBUG_OAMSA_TRANSLATIONS("MafOamSpiMoAttributeType_3_UINT16");
		break;
	case MafOamSpiMoAttributeType_3_UINT32:
		DEBUG_OAMSA_TRANSLATIONS("MafOamSpiMoAttributeType_3_UINT32");
		break;
	case MafOamSpiMoAttributeType_3_UINT64:
		DEBUG_OAMSA_TRANSLATIONS("MafOamSpiMoAttributeType_3_UINT64");
		break;
	case MafOamSpiMoAttributeType_3_STRING:
		DEBUG_OAMSA_TRANSLATIONS("MafOamSpiMoAttributeType_3_STRING");
		break;
	case MafOamSpiMoAttributeType_3_BOOL:
		DEBUG_OAMSA_TRANSLATIONS("MafOamSpiMoAttributeType_3_BOOL");
		break;
	case MafOamSpiMoAttributeType_3_REFERENCE:
		DEBUG_OAMSA_TRANSLATIONS("MafOamSpiMoAttributeType_3_REFERENCE");
		break;
	case MafOamSpiMoAttributeType_3_ENUM:
		DEBUG_OAMSA_TRANSLATIONS("MafOamSpiMoAttributeType_3_ENUM");
		break;
	case MafOamSpiMoAttributeType_3_STRUCT:
		DEBUG_OAMSA_TRANSLATIONS("MafOamSpiMoAttributeType_3_STRUCT");
		break;
	case MafOamSpiMoAttributeType_3_DECIMAL64:
		DEBUG_OAMSA_TRANSLATIONS("MafOamSpiMoAttributeType_3_DECIMAL64");
		break;

	default:
		DEBUG_OAMSA_TRANSLATIONS("ERROR - unknown MafOamSpiMoAttributeType_3: %d", ctp->type);
		LEAVE_OAMSA_TRANSLATIONS();
		return;
	}

	for (unsigned int i = 0; i < ctp->nrOfValues; i++)
	{
		switch (ctp->type)
		{
		case MafOamSpiMoAttributeType_3_INT8:
			DEBUG_OAMSA_TRANSLATIONS("Value nr %u  = %d", i+1, ctp->values[i].value.i8);
			break;
		case MafOamSpiMoAttributeType_3_INT16:
			DEBUG_OAMSA_TRANSLATIONS("Value nr %u  = %d", i+1,ctp->values[i].value.i16);
			break;
		case MafOamSpiMoAttributeType_3_INT32:
			DEBUG_OAMSA_TRANSLATIONS("Value nr %u  = %d", i+1, ctp->values[i].value.i32);
			break;
		case MafOamSpiMoAttributeType_3_INT64:
			DEBUG_OAMSA_TRANSLATIONS("Value nr %u  = %ld", i+1, ctp->values[i].value.i64);
			break;
		case MafOamSpiMoAttributeType_3_UINT8:
			DEBUG_OAMSA_TRANSLATIONS("Value nr %u  = %u", i+1, ctp->values[i].value.u8);
			break;
		case MafOamSpiMoAttributeType_3_UINT16:
			DEBUG_OAMSA_TRANSLATIONS("Value nr %u  = %u", i+1, ctp->values[i].value.u16);
			break;
		case MafOamSpiMoAttributeType_3_UINT32:
			DEBUG_OAMSA_TRANSLATIONS("Value nr %u  = %u", i+1, ctp->values[i].value.u32);
			break;
		case MafOamSpiMoAttributeType_3_UINT64:
			DEBUG_OAMSA_TRANSLATIONS("Value nr %u  = %lu", i+1, ctp->values[i].value.u64);
			break;
		case MafOamSpiMoAttributeType_3_STRING:
			if (ctp->values[i].value.theString != NULL)
			{
				DEBUG_OAMSA_TRANSLATIONS("Value nr %u  = %s", i+1, ctp->values[i].value.theString);
			}
			else
			{
				DEBUG_OAMSA_TRANSLATIONS("Error String pointer is NULL!!");
			}
			break;
		case MafOamSpiMoAttributeType_3_BOOL:
			DEBUG_OAMSA_TRANSLATIONS("Value nr %u  = %d", i+1, ctp->values[i].value.theBool);
			break;
		case MafOamSpiMoAttributeType_3_REFERENCE:
			if (ctp->values[i].value.moRef != NULL)
			{
				DEBUG_OAMSA_TRANSLATIONS("Value nr %u  = %s", i+1, ctp->values[i].value.moRef);
			}
			else
			{
				DEBUG_OAMSA_TRANSLATIONS("Error mo reference is NULL");
			}
			break;

		case MafOamSpiMoAttributeType_3_ENUM:
			DEBUG_OAMSA_TRANSLATIONS("Value nr %u  = %u", i + 1, (unsigned int) (ctp->values[i].value.theEnum));
			break;
		case MafOamSpiMoAttributeType_3_STRUCT:
			DEBUG_OAMSA_TRANSLATIONS("Printing MafOamSpiMoAttributeType_3_STRUCT");
			MafMoAttributeValueStructMember_3 * structMember;
			structMember = ctp->values[i].value.structMember;
			while (structMember != NULL)
			{
				DEBUG_OAMSA_TRANSLATIONS("  Struct member name: %s", structMember->memberName);
				DebugDumpAttributeContainer(structMember->memberValue);
				structMember = structMember->next;
			}
			break;
		case MafOamSpiMoAttributeType_3_DECIMAL64:
			DEBUG_OAMSA_TRANSLATIONS("MafOamSpiMoAttributeType_3_DECIMAL64: %d", ctp->type);
			DEBUG_OAMSA_TRANSLATIONS("Value nr %u  = %f", i + 1, (double) (ctp->values[i].value.decimal64));
			break;
		default:
			DEBUG_OAMSA_TRANSLATIONS("ERROR - unknown MafOamSpiMoAttributeType_3: %d", ctp->type);
			LEAVE_OAMSA_TRANSLATIONS();
			return;
		}
	}
	LEAVE_OAMSA_TRANSLATIONS();
}

/**
 * Function is used to sort the numeric string values
 */
static bool CustomizedNumericStringSort(const std::string lhs, const std::string rhs) {
	if (lhs.size() == rhs.size()) {
		return lhs < rhs;
	}
	else {
		return lhs.size() < rhs.size();
	}
}


const char *OamSAImmBridge::strError(SaAisErrorT e){
	ENTER_OAMSA_TRANSLATIONS();
	switch(e){
	case SA_AIS_ERR_LIBRARY:
		LEAVE_OAMSA_TRANSLATIONS();
		return "IMM: SA_AIS_ERR_LIBRARY";
	case SA_AIS_ERR_VERSION:
		LEAVE_OAMSA_TRANSLATIONS();
		return "IMM: SA_AIS_ERR_VERSION";
	case SA_AIS_ERR_INIT:
		LEAVE_OAMSA_TRANSLATIONS();
		return "IMM: SA_AIS_ERR_INIT";
	case SA_AIS_ERR_TIMEOUT:
		LEAVE_OAMSA_TRANSLATIONS();
		return "IMM: SA_AIS_ERR_TIMEOUT";
	case SA_AIS_ERR_TRY_AGAIN:
		LEAVE_OAMSA_TRANSLATIONS();
		return "IMM: SA_AIS_ERR_TRY_AGAIN";
	case SA_AIS_ERR_INVALID_PARAM:
		LEAVE_OAMSA_TRANSLATIONS();
		return "IMM: SA_AIS_ERR_INVALID_PARAM";
	case SA_AIS_ERR_NO_MEMORY:
		LEAVE_OAMSA_TRANSLATIONS();
		return "IMM: SA_AIS_ERR_NO_MEMORY";
	case SA_AIS_ERR_BAD_HANDLE:
		LEAVE_OAMSA_TRANSLATIONS();
		return "IMM: SA_AIS_ERR_BAD_HANDLE";
	case SA_AIS_ERR_BUSY:
		LEAVE_OAMSA_TRANSLATIONS();
		return "IMM: SA_AIS_ERR_BUSY";
	case SA_AIS_ERR_ACCESS:
		LEAVE_OAMSA_TRANSLATIONS();
		return "IMM: SA_AIS_ERR_ACCESS";
	case SA_AIS_ERR_NOT_EXIST:
		LEAVE_OAMSA_TRANSLATIONS();
		return "IMM: SA_AIS_ERR_NOT_EXIST";
	case SA_AIS_ERR_NAME_TOO_LONG:
		LEAVE_OAMSA_TRANSLATIONS();
		return "IMM: SA_AIS_ERR_NAME_TOO_LONG";
	case SA_AIS_ERR_EXIST:
		LEAVE_OAMSA_TRANSLATIONS();
		return "IMM: SA_AIS_ERR_EXIST";
	case SA_AIS_ERR_NO_SPACE:
		LEAVE_OAMSA_TRANSLATIONS();
		return "IMM: SA_AIS_ERR_NO_SPACE";
	case SA_AIS_ERR_INTERRUPT:
		LEAVE_OAMSA_TRANSLATIONS();
		return "IMM: SA_AIS_ERR_INTERRUPT";
	case SA_AIS_ERR_NO_RESOURCES:
		LEAVE_OAMSA_TRANSLATIONS();
		return "IMM: SA_AIS_ERR_NO_RESOURCESS";
	case SA_AIS_ERR_NOT_SUPPORTED:
		LEAVE_OAMSA_TRANSLATIONS();
		return "IMM: SA_AIS_ERR_NOT_SUPPORTED";
	case SA_AIS_ERR_BAD_OPERATION:
		LEAVE_OAMSA_TRANSLATIONS();
		return "IMM: SA_AIS_ERR_BAD_OPERATION";
	case SA_AIS_ERR_FAILED_OPERATION:
		LEAVE_OAMSA_TRANSLATIONS();
		return "IMM: SA_AIS_ERR_FAILED_OPERATION";
	case SA_AIS_ERR_MESSAGE_ERROR:
		LEAVE_OAMSA_TRANSLATIONS();
		return "IMM: SA_AIS_ERR_MESSAGE_ERROR";
	case SA_AIS_ERR_QUEUE_FULL:
		LEAVE_OAMSA_TRANSLATIONS();
		return "IMM: SA_AIS_ERR_QUEUE_FULL";
	case SA_AIS_ERR_QUEUE_NOT_AVAILABLE:
		LEAVE_OAMSA_TRANSLATIONS();
		return "IMM: SA_AIS_ERR_QUEUE_NOT_AVAILABLE";
	case SA_AIS_ERR_BAD_FLAGS:
		LEAVE_OAMSA_TRANSLATIONS();
		return "IMM: SA_AIS_ERR_BAD_FLAGS";
	case SA_AIS_ERR_TOO_BIG:
		LEAVE_OAMSA_TRANSLATIONS();
		return "IMM: SA_AIS_ERR_TOO_BIG";
	case SA_AIS_ERR_NO_SECTIONS:
		LEAVE_OAMSA_TRANSLATIONS();
		return "IMM: SA_AIS_ERR_NO_SECTIONS";
	case SA_AIS_ERR_NO_OP:
		LEAVE_OAMSA_TRANSLATIONS();
		return "IMM: SA_AIS_ERR_NO_OP";
	case SA_AIS_ERR_REPAIR_PENDING:
		LEAVE_OAMSA_TRANSLATIONS();
		return "IMM: SA_AIS_ERR_REPAIR_PENDING";
	default:
		LEAVE_OAMSA_TRANSLATIONS();
		return "Unknown IMM error";
	}
}


static void DebugDumpAdminOpReturnParams(const SaImmAdminOperationParamsT_2 ** const aorp)
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("Dumping Admin Operation Return Parameters");
	if (NULL == aorp)
	{
		DEBUG_OAMSA_TRANSLATIONS("No parameters provided");
		LEAVE_OAMSA_TRANSLATIONS();
		return;
	}
	SaImmAdminOperationParamsT_2 **ppar = (SaImmAdminOperationParamsT_2**) aorp;
	int i = 1;
	while (NULL != *ppar)
	{
		SaImmAdminOperationParamsT_2 *nextPar = *ppar;
		if (nextPar->paramName != NULL)
		{
			DEBUG_OAMSA_TRANSLATIONS("parameter[%d]: paramName = %s", i, nextPar->paramName);
		}
		switch (nextPar->paramType)
		{
		case SA_IMM_ATTR_SAINT32T:
			DEBUG_OAMSA_TRANSLATIONS("paramType[%d] is SA_IMM_ATTR_SAINT32T, paramValue = %d", i, *((int*) nextPar->paramBuffer));
			break;
		case SA_IMM_ATTR_SAUINT32T:
			DEBUG_OAMSA_TRANSLATIONS("paramType[%d] is SA_IMM_ATTR_SAUINT32T, paramValue = %u", i, *((unsigned int*) nextPar->paramBuffer));
			break;
		case SA_IMM_ATTR_SAINT64T:
			DEBUG_OAMSA_TRANSLATIONS("paramType[%d] is SA_IMM_ATTR_SAINT64T, paramValue = %ld", i, *((long*) nextPar->paramBuffer));
			break;
		case SA_IMM_ATTR_SAUINT64T:
			DEBUG_OAMSA_TRANSLATIONS("paramType[%d] is SA_IMM_ATTR_SAUINT64T, paramValue = %lu", i, *((unsigned long*) nextPar->paramBuffer));
			break;
		case SA_IMM_ATTR_SATIMET:
			DEBUG_OAMSA_TRANSLATIONS("paramType[%d] is SA_IMM_ATTR_SATIMET, paramValue = %lu", i, *((unsigned long*) nextPar->paramBuffer));
			break;
		case SA_IMM_ATTR_SANAMET:
			DEBUG_OAMSA_TRANSLATIONS("paramType[%d] is SA_IMM_ATTR_SANAMET, paramValue = %s", i, saNameGet((SaNameT *)nextPar->paramBuffer));
			break;
		case SA_IMM_ATTR_SAFLOATT:
			DEBUG_OAMSA_TRANSLATIONS("paramType[%d] is SA_IMM_ATTR_SAFLOATT, paramValue = %f", i, *((float*) nextPar->paramBuffer));
			break;
		case SA_IMM_ATTR_SADOUBLET:
			DEBUG_OAMSA_TRANSLATIONS("paramType[%d] is SA_IMM_ATTR_SADOUBLET, paramValue = %f", i, *((double*) nextPar->paramBuffer));
			break;
		case SA_IMM_ATTR_SASTRINGT:
			DEBUG_OAMSA_TRANSLATIONS("paramType[%d] is SA_IMM_ATTR_SASTRINGT, paramValue = %s", i, *((char**) nextPar->paramBuffer));
			break;
		case SA_IMM_ATTR_SAANYT:
			DEBUG_OAMSA_TRANSLATIONS("paramType[%d] is SA_IMM_ATTR_SAANYT, paramValue = %p", i, nextPar->paramBuffer);
			break;
		default:
			ERR_OAMSA_TRANSLATIONS("DebugDumpAdminOpReturnParams(): ERROR: Unknown SA_IMM_ATTR paramType: %d", nextPar->paramType);
			break;
		}
		ppar++;
		i++;
	}
	LEAVE_OAMSA_TRANSLATIONS();
}

/**
 *  BridgeImmIterator constructor
 */
BridgeImmIterator::BridgeImmIterator(const char *rdn, const char *rootClass) :
		myRootDN( rdn ),
		myRootClass( rootClass ),
		myLastCacheObj("")
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
}

BridgeImmIterator::BridgeImmIterator(const char *rdn, const char *rootClass, const char *theImmName) :
		myRootDN( rdn ),
		myRootClass( rootClass ),
		myLastCacheObj( "" ),
		myImmName( theImmName )
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
}

/**
 *  Default constructor
 */
OamSAImmBridge::OamSAImmBridge()
{
	ENTER_OAMSA_TRANSLATIONS();
	oamSAImmBridge = this;
	LEAVE_OAMSA_TRANSLATIONS();
}

/**
 * 	Default destructor
 */
OamSAImmBridge::~OamSAImmBridge()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSAImmBridge *OamSAImmBridge::getOamSAImmBridge()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return oamSAImmBridge;
}

MafReturnT OamSAImmBridge::getImmMoAttributeSimple(MafOamSpiTransactionHandleT txHandle,
								const char *dn, const char *attributeName, MafOamSpiMoAttributeType_3T attrType,
								MafMoAttributeValueContainer_3T **result, bool isKeyAttribute)



{
	ENTER_OAMSA_TRANSLATIONS();
	MafReturnT 				retVal = MafOk;
	SaImmAttrValuesT_2		**AttrValue;
	std::string				immDn;
	std::tr1::shared_ptr<TxContext>	txContextIn;
	SaAisErrorT				SaError;
	OamSACache::DNList		theDNList;
	MafLockPolicyT	thePolicy=(MafLockPolicyT)0;
	*result = NULL;

	txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->getTxContext(txHandle);
	if (NULL == txContextIn.get())
	{
		DEBUG_OAMSA_TRANSLATIONS("getImmMoAttributeSimple the context associated with handle %d was not found in the repository", (int)txHandle);
		LEAVE_OAMSA_TRANSLATIONS();
		return MafNotExist;
	}
	std::string theDN(dn);
	txContextIn->GetCache().SplitDN(theDN,theDNList);

	if (!theTranslator.IsDnListValid(theDNList, attributeName))
	{
		invalidModelDetected("OamSAImmBridge::getImmMoAttributeSimple", dn, attributeName);
		LEAVE_OAMSA_TRANSLATIONS();
		return MafFailure;
	}

	theTranslator.MO2Imm_DN(theDNList, immDn);
	DEBUG_OAMSA_TRANSLATIONS("getImmMoAttributeSimple DN = \"%s\" IMM name = \"%s\" Attribute = \"%s\" ",
										theDN.c_str(), immDn.c_str(), attributeName);
	DEBUG_OAMSA_TRANSLATIONS("getImmMoAttributeSimple immDn = %s", immDn.c_str());
	if (txContextIn->GetCache().IsDeleted(theDNList))
	{
		DEBUG_OAMSA_TRANSLATIONS("getImmMoAttributeSimple :: object %s is in deleted list...",dn);
		return MafNotExist;
	}

	// First look in the cache to see if what we are looking for is there
	// OK, here we need to handle the possibility that the attribute we search in the cache for is prefixed
	// by it's MOM. If this is the case then the attribute name in the MOM needs to have the MOM name
	// added before the key name.
	std::string strAttrName(attributeName);
	OamSACache::ObjectState objres = txContextIn->GetCache().GetAttribute(theDNList, strAttrName, result);
	DEBUG_OAMSA_TRANSLATIONS("getImmMoAttributeSimple after looking in cache objres = %d", objres);
	if (objres == OamSACache::eObjectSuccess)
	{
		if (isKeyAttribute && ((*result)->nrOfValues == 1))
		{
			// OK, we need to remove the name of the key attribute from the value
			// So we have something like this : CmwPMIDIDIequal=1 and it shall be converted to 1
			std::string keyAttributeValue = (*result)->values[0].value.theString;
			size_t equalSingPosition = keyAttributeValue.find_first_of("=");
			if (equalSingPosition!=std::string::npos)
			{
				keyAttributeValue = keyAttributeValue.substr(equalSingPosition+1);
				// Change the value in the key attribute
				delete [](*result)->values[0].value.theString;
				(*result)->values[0].value.theString = new char[keyAttributeValue.length() + 1];
				strncpy((char *)(*result)->values[0].value.theString, keyAttributeValue.c_str(), keyAttributeValue.length() + 1);
			}
		}
		// Object found, let's register this allocated return value in the transaction context
		// in order to be able to delete it when the transaction is removed
		txContextIn->GetCache().RegisterAttributeValueContainer(*result);
		return MafOk;
	}

	// No cache hit, need to read directly from IMM
	// Check the lock policy for this transaction to see if it's necessary to lock also for a read operation.
	if (MafOamSpiTransactionStruct_p_v2 != NULL)
	{
		MafOamSpiTransactionStruct_p_v2->getLockPolicy(txHandle, &thePolicy);
	}

	SaAisErrorT immErr = SA_AIS_OK;
	{
		if (immErr == SA_AIS_OK)
		{
			if (thePolicy == MafLockReadWrite)
			{
				immErr = checkSetAdminOwner(txContextIn.get(), immDn, SA_IMM_ONE);
				if(immErr != SA_AIS_OK)
				{
					if (isAdminOwnerLockedByAnotherTx(txContextIn.get(), immErr))
					{
						// We probably should send some kind of an error message or log here
						// indicating that the object is locked by someone else.
						DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::getImmMoAttributeSimple \"%s\" already locked by other transaction",dn);
						retVal = MafObjectLocked;
						goto end_exit;
					}
					else if (immErr == SA_AIS_ERR_EXIST)
					{
						// For the case object is locked by another process differs than ComSA
						DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::getImmMoAttributeSimple \"%s\" already locked by another process",dn);
						retVal = MafObjectLocked;
						goto end_exit;
					}
					else
					{
						retVal = MafFailure;
						goto end_exit;
					}

				}
			}
			SaError = getImmAttribute(txContextIn.get(), immDn.c_str(), (char*)attributeName, &AttrValue);
			DEBUG_OAMSA_TRANSLATIONS("getImmMoAttributeSimple after getImmAttribute imm return value = %d", SaError);
			// If locked, unlock immediately do not wait until abort or commit
			if (thePolicy == MafLockReadWrite)
			{
				immErr = releaseAdminOwner(txContextIn.get(), immDn, SA_IMM_ONE);
			}

			if (SA_AIS_OK == SaError)
			{
				// Translate the value returned from IMM into something that is understandable by MO
				MafMoAttributeValueContainer_3T *ctp = new MafMoAttributeValueContainer_3T;
				if (ctp != NULL)
				{
					// Since ctp is a struct and not a class we need to see to that it gets
					// initialized as it has no constructor
					memset(ctp,0,sizeof(MafMoAttributeValueContainer_3T));
					txContextIn->GetCache().RegisterAttributeValueContainer(ctp);
					bool tempResult = theTranslator.Imm2MO_AttrValue(txHandle, *AttrValue, attrType, &ctp);
					if (tempResult)
					{
						if (isKeyAttribute && (ctp->nrOfValues == 1))
						{
							// OK, we need to remove the name of the key attribute from the value
							// So we have something like this : CmwPMIDIDIequal=1 and it shall be converted to 1
							std::string keyAttributeValue = ctp->values[0].value.theString;
							size_t equalSingPosition = keyAttributeValue.find_first_of("=");
							if (equalSingPosition!=std::string::npos)
							{
								keyAttributeValue = keyAttributeValue.substr(equalSingPosition+1);
								// Change the value in the key attribute
								delete [] ctp->values[0].value.theString;
								ctp->values[0].value.theString = new char[keyAttributeValue.length() + 1];
								strncpy((char *)ctp->values[0].value.theString, keyAttributeValue.c_str(), keyAttributeValue.length() + 1);
							}
						}
						DebugDumpAttributeContainer(ctp);
						*result = ctp;
					}
					else
					{
						retVal = MafFailure;
					}
				}
				else
				{
					retVal = MafNoResources;
				}
			}
			else
			{
				if (SA_AIS_ERR_NOT_EXIST == SaError)
				{
					// Maybe the object is created (has not been committed) and attribute is not set. In this case we need to check if the object is stored in cache first
					// Then check if the attribute is valid -> set nrOfValues = 0 and return MafOk
					// Otherwise return MafNotExist
					if (txContextIn->GetCache().IsInCache(theDNList))
					{
						// Get the className
						std::string className;
						std::string parentClassName;
						retrieveLastElementFromDn(theDNList, className, parentClassName, true);

						// Get decorated className
						className = theTranslator.TransformImmClassName(theDNList, className);

						// Check if the attribute is valid
						SaImmClassCategoryT		theCategory;
						SaImmValueTypeT			theValueType;
						SaImmAttrFlagsT			theFlags;

						bool ImmAttributeFound = txContextIn->GetCache().GetImmAttributeData((char*)className.c_str(),
																					(char*)attributeName,
																					theCategory,
																					theValueType,
																					theFlags);
						if (ImmAttributeFound)
						{
							// Translate the value returned from IMM into something that is understandable by MO
							MafMoAttributeValueContainer_3T *ctp = new MafMoAttributeValueContainer_3T;
							if (ctp != NULL)
							{
								// Since ctp is a struct and not a class we need to see to that it gets
								// initialized as it has no constructor
								memset(ctp,0,sizeof(MafMoAttributeValueContainer_3T));

								txContextIn->GetCache().RegisterAttributeValueContainer(ctp);

								ctp->nrOfValues = 0;
								ctp->type = attrType;

								DebugDumpAttributeContainer(ctp);
								*result = ctp;
							}
							else
							{
								retVal = MafNoResources;
							}
						}
						else
						{
							retVal = MafNotExist;
						}
					}
					else
					{
						retVal = MafNotExist;
					}
				}
				else
				{
					retVal = MafFailure;
				}
			}
		}
	}
	end_exit:
	DEBUG_OAMSA_TRANSLATIONS("getImmMoAttributeSimple leaving return value = %d", retVal);
	LEAVE_OAMSA_TRANSLATIONS();
	return retVal;
}


/**
 * Read a structure from IMM
 * txHandle: is the current transaction
 * dn: the ECIM path
 * attributeName: the name of the struct attribute
 */
MafReturnT OamSAImmBridge::readStructFromImm(MafOamSpiTransactionHandleT txHandle,
											const char *dn,
											const char *attributeName,
											MafMoAttributeValueContainer_3T **result)
{
	ENTER_OAMSA_TRANSLATIONS();
	std::tr1::shared_ptr<TxContext> txContextIn;
	std::string theImmClassName;
	std::string theComClassName;
	std::string prefixMom="";
	OamSACache::DNList		theDNList;
	*result = NULL;
	MafReturnT ret = MafOk;

	txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->getTxContext(txHandle);
	if (NULL == txContextIn.get())
	{
		DEBUG_OAMSA_TRANSLATIONS("readStructFromImm the context associated with handle %d was not found in the repository", (int)txHandle);
		LEAVE_OAMSA_TRANSLATIONS();
		return MafNotExist;
	}
	std::string theDN(dn);
	txContextIn->GetCache().SplitDN(theDN,theDNList);

	// Get prefix MOM
	std::string parentClassName;
	retrieveLastElementFromDn(theDNList, theComClassName, parentClassName, true);
	// Get decorated className
	theImmClassName = theTranslator.TransformImmClassName(theDNList, theComClassName);
	std::size_t foundString  = theImmClassName.rfind(theComClassName);
	prefixMom = theImmClassName.substr(0,foundString);
	MafMoAttributeValueContainer_3T *structRef = NULL;

	// NOTE: read this value as if it were a "STRING"
	ret = getImmMoAttributeSimple(txHandle,dn,attributeName,MafOamSpiMoAttributeType_3_REFERENCE,&structRef);
	if(ret != MafOk || structRef == NULL)
	{
		if(ret == MafNotExist)
		{
			*result = new MafMoAttributeValueContainer_3T;
			// Register new memory container in cache to be deleted after transaction ended
			txContextIn->GetCache().RegisterAttributeValueContainer(*result);
			(*result)->type = MafOamSpiMoAttributeType_3_STRUCT;
			(*result)->nrOfValues = 0;
			(*result)->values = NULL;
			return MafOk;
		}
		return MafFailure;
	}
	// the struct ref can contain multiple values each pointing to a struct instance
	// for each struct instance we need to collect the data and pack into a AttributeValueContainer
	MafMoAttributeValueContainer_3T *ctp = new MafMoAttributeValueContainer_3T;
	ctp->type = MafOamSpiMoAttributeType_3_STRUCT;
	ctp->nrOfValues = structRef->nrOfValues;

	if (structRef->nrOfValues == 0)
	{
		ctp->values = NULL;
	}
	else
	{
		ctp->values = new MafMoAttributeValue_3T[structRef->nrOfValues];
	}

	std::string structAttributeName(attributeName);
	DEBUG_OAMSA_TRANSLATIONS("readStructFromImm DN: %s AttributeName: %s", dn, attributeName);
	DEBUG_OAMSA_TRANSLATIONS("readStructFromImm Preparing to read struct, instance count: %d", structRef->nrOfValues);

	unsigned int ix;
	for(ix = 0; ix < structRef->nrOfValues; ix++) {
		// this is an ECIM address because we cannot reverse the IMM DN
		// and we need the original ECIM DN, hence when we create a struct we will store the ECIM DN and not the IMM DN.
		std::string structDn(structRef->values[ix].value.moRef);
		OamSACache::DNList structDnList;
		std::string structImmDn;
		std::string structImmClassName;
		std::string structComClassName;
		std::string prefixMomstructComClassName;

		std::list<string> memberNames = theTranslator.getStructMembers(theDNList,attributeName);

		DEBUG_OAMSA_TRANSLATIONS("readStructFromImm structDn: %s", structDn.c_str());
		DEBUG_OAMSA_TRANSLATIONS("readStructFromImm memberCount: %d", (int)memberNames.size());
		// Incase, the IMM DN do no exist in IMM.
		txContextIn->GetCache().SplitDN(structDn,structDnList);
		theTranslator.MO2Imm_DN(structDnList, structImmDn);
		if (!txContextIn->GetCache().GetClassNameFromImm(structImmDn, structImmClassName)) {
			if(!txContextIn->GetCache().IsInCache(structDnList)){
				DEBUG_OAMSA_TRANSLATIONS("readStructFromImm ERROR GetClassNameFromImm false, structImmDn = %s is not exist", structImmDn.c_str());
				for (unsigned int i = ix; i < structRef->nrOfValues; i++)
					ctp->values[i].value.structMember = NULL;
				txContextIn->GetCache().RegisterAttributeValueContainer(ctp);
				txContextIn->GetCache().ReleaseAttributeValueContainer(ctp);
				*result = new MafMoAttributeValueContainer_3T;
				// Register new memory container in cache to be deleted after transaction ended
				txContextIn->GetCache().RegisterAttributeValueContainer(*result);
				(*result)->type = MafOamSpiMoAttributeType_3_STRUCT;
				(*result)->nrOfValues = 0;
				(*result)->values = NULL;
				return MafOk;
			}
		}
		else
		{	//Incase, the memberName is not match with structDn.
			for (MafOamSpiMoAttributeT* ap = theTranslator.GetComAttributeList(theDNList); ap != NULL; ap = ap->next)
			{
				if (!strcmp(ap->generalProperties.name, attributeName))
				{
					if(ap->type == MafOamSpiMoAttributeType_STRUCT && ap->structDatatype != NULL){
						structComClassName = ap->structDatatype->generalProperties.name;
					}
				}
			}
			if (prefixMom != ""){
				DEBUG_OAMSA_TRANSLATIONS("readStructFromImm prefixMom = %s", prefixMom.c_str());
				prefixMomstructComClassName = prefixMom + structComClassName;
			}
			else{
				prefixMomstructComClassName = structComClassName;
			}
			if (strcmp(structImmClassName.c_str(), prefixMomstructComClassName.c_str()))
			{
				ERR_OAMSA_TRANSLATIONS("readStructFromImm theImmClassName[%s] does not match with prefixMomstructComClassName[%s]", structImmClassName.c_str(), prefixMomstructComClassName.c_str());
				ret = MafFailure;
				for (unsigned int i = ix; i < structRef->nrOfValues; i++)
					ctp->values[i].value.structMember = NULL;
				break;
			}
		}
		// step through all struct members in reverse order and build single linked list from the back.
		MafMoAttributeValueStructMember_3T *prevStructMember = NULL;
		std::list<std::string>::reverse_iterator it;
		for( it = memberNames.rbegin(); it != memberNames.rend(); ++it) {
			MafMoAttributeValueStructMember_3T *structMember = new MafMoAttributeValueStructMember_3T;
			int len = it->length()+1; // extra char for terminating '\0'
			structMember->memberName = new char[len];
			strcpy(structMember->memberName, it->c_str());	// Changed from memcpy to get the terminating nul-char too
			// find out the ECIM type of the member so that we can read it from IMM and convert.
			std::string structMemberName(*it);
			MafOamSpiMoAttributeType_3T memberType;
			if( ! theTranslator.getStructMemberComDatatype(theDNList,structAttributeName,structMemberName,&memberType)) {
				// failed to find a members type
				DEBUG_OAMSA_TRANSLATIONS("readStructFromImm ERROR getStructMemberComDatatype, memberName: %s", structMemberName.c_str());
				ret = MafFailure;
				delete[] structMember->memberName;
				delete structMember;
				break;
			}
			ret = getImmMoAttributeSimple(txHandle,structDn.c_str(),it->c_str(),memberType,&(structMember->memberValue));
			if(ret != MafOk || structMember->memberValue == NULL){
				DEBUG_OAMSA_TRANSLATIONS("readStructFromImm ERROR getImmMoAttributeSimple, memberName: %s structDn: %s", structMemberName.c_str(), structDn.c_str());

				delete[] structMember->memberName;
				delete structMember;
				if (ret != MafNotExist)
				{
					ret = MafFailure;
					break;
				}
				continue;
			}
			DEBUG_OAMSA_TRANSLATIONS("readStructFromImm SUCCESS, memberName %s", structMemberName.c_str());
			structMember->next = prevStructMember;
			prevStructMember = structMember;
		} // for

		// attach struct members list to array of values in the container
		ctp->values[ix].value.structMember = prevStructMember;
		if (ret == MafFailure) {
			for (unsigned int i = ix + 1; i < structRef->nrOfValues; i++)
				ctp->values[i].value.structMember = NULL;
			break;
		}
	} //for

	if ((ret != MafOk) && (ret != MafNotExist)) {
		txContextIn->GetCache().RegisterAttributeValueContainer(ctp);
		txContextIn->GetCache().ReleaseAttributeValueContainer(ctp);
		LEAVE_OAMSA_TRANSLATIONS();
		return MafFailure;
	}

	DEBUG_OAMSA_TRANSLATIONS("readStructFromImm RETURN SUCCESS");
	txContextIn->GetCache().RegisterAttributeValueContainer(ctp);
	*result = ctp; // write in result value!
	LEAVE_OAMSA_TRANSLATIONS();
	return MafOk;
}


/**
 * Write a structure attribute to the IMM
 * TODO optimize code since it was developed for functionality and not speed, and perform code review.
 *
 * dn: path to class containing the structure attribute
 * attributeName: the name of the structure attribute
 * AttributeValue: the data container to write to IMM.
 */
MafReturnT OamSAImmBridge::writeStructToImm(MafOamSpiTransactionHandleT txHandle,
											const char *dn,
											const char *attributeName,
											MafMoAttributeValueContainer_3T *AttributeValue)
{
	ENTER_OAMSA_TRANSLATIONS();
	std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->getTxContext(txHandle);
	if (NULL == txContextIn.get())
	{
		DEBUG_OAMSA_TRANSLATIONS("writeStructToImm the context associated with handle %d was not found in the repository", (int)txHandle);
		LEAVE_OAMSA_TRANSLATIONS();
		return MafNotExist;
	}
	// find the ECIM class for the attribute and create the IMM path to use
	std::string ecimDn(dn);
	std::string ecimAttrName(attributeName);
	std::string immDn;
	OamSACache::DNList	ecimDnList;
	txContextIn->GetCache().SplitDN(ecimDn,ecimDnList);
	theTranslator.MO2Imm_DN(ecimDnList, immDn);    // the original DN in IMM format

	std::string attributeTypeName = theTranslator.findAttributeTypeName(ecimDnList,ecimAttrName);

	// Get the existing structures in the class
	std::set<std::string> myStructNameSet;
	std::set<std::string>::iterator myStructNameSetIter;

	std::string StringAttrName(attributeName);

	MafMoAttributeValueContainer_3T *structRef = NULL;
	MafReturnT ret = getImmMoAttributeSimple(txHandle, dn, attributeName, MafOamSpiMoAttributeType_3_STRING, &structRef);
	if(ret != MafOk && ret != MafNotExist)
	{
		LEAVE_OAMSA_TRANSLATIONS();
		return ret;
	}
	DEBUG_OAMSA_TRANSLATIONS("writeStructToImm after reading existing structRefs: retVal = %d (MafOk == 0)", ret);
	if (0 == AttributeValue->nrOfValues)
	{
		// Empty parameter list, delete all existing structure objects and erase the parameter list
		if(ret == MafOk)
		{
			DEBUG_OAMSA_TRANSLATIONS("writeStructToImm. found %d objects to delete",structRef->nrOfValues);
			for(unsigned int ix = 0; ix < structRef->nrOfValues; ix++)
			{
				DEBUG_OAMSA_TRANSLATIONS(" writeStructToImm Deleteing %s ix %d",structRef->values[ix].value.moRef,ix);
				MafReturnT retVal = deleteImmMo(txHandle, structRef->values[ix].value.moRef);
				if (retVal != MafOk)
				{
					ERR_OAMSA_TRANSLATIONS(" writeStructToImm error after deleteImmMo retVal=%d", retVal);
					LEAVE_OAMSA_TRANSLATIONS();
					return retVal;
				}
			}
		}
		OamSACache::ObjectState objstate = 	txContextIn->GetCache().UpdateObjectAttribute(ecimDnList,immDn, StringAttrName, AttributeValue);
		if (objstate != OamSACache::eObjectSuccess)
		{
			ERR_OAMSA_TRANSLATIONS("writeStructToImm Failed to update structure parameter to no values objstate = %d", objstate);
			LEAVE_OAMSA_TRANSLATIONS();
			return MafFailure;
		}
	}
	else
	{
		std::vector<std::string> myStructNameVector;
		if(ret == MafOk)
		{
			// insert new values
			for(unsigned int ix = 0; ix < structRef->nrOfValues; ix++)
			{
				// Put the names into a set to be able to find them later to avoid
				// creating already existing things......
				DEBUG_OAMSA_TRANSLATIONS(" writeStructToImm structRef->values[ix].value.moRef %s ix %d",structRef->values[ix].value.moRef,ix);
				std::string structDn(structRef->values[ix].value.moRef);
				myStructNameVector.push_back(structDn);
				size_t equalsignpos = structDn.find_last_of('=');
				structDn = structDn.substr(equalsignpos+1);
				DEBUG_OAMSA_TRANSLATIONS("writeStructToImm putting in %s as reference in set",structDn.c_str());
				myStructNameSet.insert(structDn);
			}
			std::sort(myStructNameVector.begin(),myStructNameVector.end(),CustomizedNumericStringSort);
		}
		//delete a necessary element of struct
		if(ret == MafOk)
		{
			if (AttributeValue->nrOfValues < structRef->nrOfValues)
			{
				int iy;
				iy = structRef->nrOfValues - AttributeValue->nrOfValues;
				DEBUG_OAMSA_TRANSLATIONS("writeStructToImm. found %d objects to delete", iy);
				while (iy > 0)
				{
					DEBUG_OAMSA_TRANSLATIONS(" writeStructToImm Deleteing %s iy %d",myStructNameVector[structRef->nrOfValues - iy].c_str(), structRef->nrOfValues - iy);
					MafReturnT retVal = deleteImmMo(txHandle, myStructNameVector[structRef->nrOfValues - iy].c_str());
					std::string structDn(myStructNameVector[structRef->nrOfValues - iy]);
					size_t equalsignpos = structDn.find_last_of('=');
					structDn = structDn.substr(equalsignpos+1);
					myStructNameSet.erase(structDn);
					if (retVal != MafOk)
					{
						ERR_OAMSA_TRANSLATIONS(" writeStructToImm error after deleteImmMo retVal=%d", retVal);
						LEAVE_OAMSA_TRANSLATIONS();
						return retVal;
					}
				iy--;
				}
			}
			//delete an element when it is existing and is an EcimPassword
			else if (structRef->nrOfValues == 1)
			{
				std::string structDn(structRef->values[0].value.moRef);
				if(structDn.find("EcimPassword")!= std::string::npos)
					{
						MafReturnT retVal = deleteImmMo(txHandle, myStructNameVector[0].c_str());
						std::string structDn(myStructNameVector[0]);
						size_t equalsignpos = structDn.find_last_of('=');
						structDn = structDn.substr(equalsignpos+1);
						DEBUG_OAMSA_TRANSLATIONS("writeStructToImm Deleting %s ",structDn.c_str());
						myStructNameSet.erase(structDn);
						if (retVal != MafOk)
						{
						ERR_OAMSA_TRANSLATIONS(" writeStructToImm error after deleteImmMo retVal=%d", retVal);
						LEAVE_OAMSA_TRANSLATIONS();
						return retVal;
						}
					}
			}
			//clearing the vector once the operation is done
			myStructNameVector.clear();
		}
		// store all created struct class instances
		std::list<std::string> immStructDnReferenceList;
		DEBUG_OAMSA_TRANSLATIONS("writeStructToImm dn: %s, attributeName: %s, attrNum: %d ", dn, attributeName, AttributeValue->nrOfValues);
		// We need to handle multiple value attributes (eg. arrays)
		for(unsigned int ix=0; ix<AttributeValue->nrOfValues; ix++){

			// build a unique IMM DN for the struct class
			// first we generate the instance id <attributeName>_<index>
			std::string instanceId(attributeName);
			instanceId.append(STRUCT_CLASS_INDEX_SEPARATOR);
			// convert number to string...lovely, just lovely :-P
			std::ostringstream os;
			os << ix;
			instanceId.append(os.str());

			// then we make the unique DN <StructClassName>.id=<instanceId>
			std::string translatedAttributeTypeName(attributeTypeName);
			std::string uniqueId(translatedAttributeTypeName);

			uniqueId.append(".");
			uniqueId.append(STRUCT_CLASS_KEY_ATTRIBUTE);
			uniqueId.append("=");
			uniqueId.append(instanceId);

			// we need the ECIM DN to write into the reference, this is because we want to use Classname.id as RDN!!
			// if we use the IMM DN then we cannot convert it back to ECIM DN. We need ECIM DN's to be able to work towards
			// the session cache.
			std::string ecimStructDn(dn);
			ecimStructDn.append(",");
			ecimStructDn.append(uniqueId);

			// store the ECIM struct dn so we later can write it into the struct reference in parent class
			immStructDnReferenceList.push_back(ecimStructDn);

			// create each struct class instance separately
			std::string keyAttribute = theTranslator.Convert3GPPNameFragmentToImmNameFragment(uniqueId);
			size_t equalSignPos = keyAttribute.find("=");
			if(equalSignPos != std::string::npos){
				keyAttribute = keyAttribute.substr(0, equalSignPos);
			}

			std::string keyValue(instanceId);
			if (myStructNameSet.find(instanceId) == myStructNameSet.end())
			{
				DEBUG_OAMSA_TRANSLATIONS("writeStructToImm %s not existing, creating the object",ecimStructDn.c_str());
				// See to that the class name always start with a capital letter
				translatedAttributeTypeName[0] = toupper(translatedAttributeTypeName[0]);
				// initial attribute values not used here, so sett it to NULL
				MafReturnT res = createImmMo(txHandle,dn,translatedAttributeTypeName.c_str(),keyAttribute.c_str(),keyValue.c_str(), NULL);
				DEBUG_OAMSA_TRANSLATIONS("writeStructToImm after createImmMo: attributeTypeName: %s keyAttribute: %s, keyValue: %s, NULL",
						translatedAttributeTypeName.c_str(),keyAttribute.c_str(),keyValue.c_str());
				if(res != MafOk && res != MafAlreadyExist){
					// error could not create struct class instance, what now?
					ERR_OAMSA_TRANSLATIONS("writeStructToImm RETURN FAILURE");
					LEAVE_OAMSA_TRANSLATIONS();
					return MafFailure;
				}
			}
			else
			{
				DEBUG_OAMSA_TRANSLATIONS("writeStructToImm %s existing, NOT creating the object",ecimStructDn.c_str());
			}
			// set the values for the instance created
			// we need to loop through all members in the struct and convert to non-complex attributes
			// using the memberName as attribute name and the memberValues as values
			MafMoAttributeValueStructMember_3T *currentStructMember = AttributeValue->values[ix].value.structMember;
			while(currentStructMember != NULL){
				// convert struct member to non-complex attribute value
				MafReturnT setRes = setImmMoAttribute(txHandle,ecimStructDn.c_str(),currentStructMember->memberName, currentStructMember->memberValue);
				if (MafOk != setRes)
				{
					ERR_OAMSA_TRANSLATIONS("writeStructToImm setImmMoattribute() failed.");
					LEAVE_OAMSA_TRANSLATIONS();
					return setRes;
				}
				currentStructMember = currentStructMember->next;
			}
		}
		// now we need to write the struct references into the original attribute
		// this means we use the original COM DN with the original attribute name

		// First we need an attribute value container with possibly multiple references
		MafMoAttributeValueContainer_3T *valueRef = new MafMoAttributeValueContainer_3T;
		valueRef->type = MafOamSpiMoAttributeType_3_REFERENCE;
		valueRef->nrOfValues = immStructDnReferenceList.size();
		unsigned int i = 0;
		valueRef->values = new MafMoAttributeValue_3T[valueRef->nrOfValues];
		for(std::list<std::string>::iterator it=immStructDnReferenceList.begin(); it!=immStructDnReferenceList.end(); ++it ){
			unsigned int length = (*it).length();
			const char* src = (*it).c_str();
			char* tmp = new char[length+1];
			strcpy(tmp,src);
			valueRef->values[i].value.moRef = tmp;
			DEBUG_OAMSA_TRANSLATIONS("writeStructToImm immStructRefList: %s", tmp);
			i = i+1;
		}
		// let the cache delete the container when session is terminated
		txContextIn->GetCache().RegisterAttributeValueContainer(valueRef);

		// now we need to write to the _original_ DN and _original_ attribute all the references to the struct classes
		// both the ECIM DN and the corresponding IMM DN need to be the originals
		OamSACache::ObjectState objstate;
		objstate = 	txContextIn->GetCache().UpdateObjectAttribute(ecimDnList,immDn, StringAttrName, valueRef);
		if (objstate != OamSACache::eObjectSuccess)
		{
			ERR_OAMSA_TRANSLATIONS("writeStructToImm RETURN FAILURE");
			LEAVE_OAMSA_TRANSLATIONS();
			return MafFailure;
		}
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return MafOk;
}


/**
 *  Front end for GetImmMoAttribute that delegates to original method
 */
MafReturnT OamSAImmBridge::getImmMoAttribute(MafOamSpiTransactionHandleT txHandle,
								const char *dn, const char *attributeName, MafMoAttributeValueContainer_3T **result)
{
	ENTER_OAMSA_TRANSLATIONS();
	MafReturnT 			retVal = MafOk;
	std::tr1::shared_ptr<TxContext> txContextIn;
	OamSACache::DNList	theDNList;
	*result = NULL;
	std::string strAttrName(attributeName);
	txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->getTxContext(txHandle);
	if (NULL == txContextIn.get())
	{
		DEBUG_OAMSA_TRANSLATIONS("getImmMoAttribute the context associated with handle %d was not found in the repository", (int)txHandle);
		LEAVE_OAMSA_TRANSLATIONS();
		return MafNotExist;
	}
	std::string theDN(dn);
	txContextIn->GetCache().SplitDN(theDN,theDNList);

	if (!theTranslator.IsDnListValid(theDNList, attributeName))
	{
		invalidModelDetected("OamSAImmBridge::getImmMoAttribute", theDN, attributeName);
		LEAVE_OAMSA_TRANSLATIONS();
		return MafFailure;
	}

	// Check if the this is a key attribute in a decorated MOM.
	std::string className;
	std::string parentClassName;
	retrieveLastElementFromDn(theDNList, className, parentClassName, false);
	// Check if this is the a decorated key attribute, if so change it.
	bool isKeyAttribute = false;
	if (theTranslator.isImmKeyAttribute(parentClassName.c_str(), className.c_str(),strAttrName.c_str()))
	{
		// Ok, needs to be decorated, do it.
		isKeyAttribute = true;
		strAttrName = theTranslator.getImmKeyAttribute(parentClassName.c_str(),
													className.c_str(),
													strAttrName.c_str());
	}
	DEBUG_OAMSA_TRANSLATIONS("getImmMoAttribute:  dn=%s, attrName=%s", dn, attributeName);

	// Handle Complex struct data type by reading the parts separately.
	// When retrieving the type of the attribute the MOM name must be used....The IMM name do not work in this method
	std::string attributeNameString(attributeName);
	if (theTranslator.isStructAttribute(theDNList, attributeNameString))
	{
		DEBUG_OAMSA_TRANSLATIONS("getImmMoAttribute: attribute <%s> is a struct", strAttrName.c_str());
		retVal = readStructFromImm(txHandle, dn, strAttrName.c_str(), result);
	}
	else
	{
		// normal reading of attributes, use simple method
		// When retrieving the type of the attribute the MOM name must be used....The IMM name do not work in this method
		MafOamSpiMoAttributeType_3 attrType = theTranslator.GetComAttributeType(theDNList, attributeNameString);
		retVal = getImmMoAttributeSimple(txHandle,dn,strAttrName.c_str(),attrType,result, isKeyAttribute);
		DEBUG_OAMSA_TRANSLATIONS("getImmMoAttribute: attribute <%s> is of 'simple' type %d", strAttrName.c_str(), (int)attrType);
	}

	if (*result)
	{
		if ((*result)->type == MafOamSpiMoAttributeType_3_STRING)
		{
			/*
			* Show time! Go figure...
			*/
			for (unsigned int i=0; i<(*result)->nrOfValues; i++)
			{
				if (strstr((*result)->values[i].value.theString, attributeName) ==
						(*result)->values[i].value.theString &&
						*((*result)->values[i].value.theString + strlen(attributeName)) == '=')
				{
					memmove(const_cast<char*>((*result)->values[i].value.theString), (*result)->values[i].value.theString + strlen(attributeName) +1, strlen((*result)->values[i].value.theString + strlen(attributeName) +1) +1);
				}
			}
		}
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return retVal;
}

/**
 * invokeImmMoAction
 */
MafReturnT OamSAImmBridge::action(MafOamSpiTransactionHandleT txHandle, const char * dn, const char * name,
				MafMoAttributeValueContainer_3T **parameters, MafMoAttributeValueContainer_3T **result)
{
	ENTER_OAMSA_TRANSLATIONS();
	MafReturnT 			retVal = MafOk;
	std::string			immDn;
	std::tr1::shared_ptr<TxContext> txContextIn;
	OamSACache::DNList	theDNList;


	txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->getTxContext(txHandle);
	if (NULL == txContextIn.get())
	{
		DEBUG_OAMSA_TRANSLATIONS("action the context associated with handle %d was not found in the repository", (int)txHandle);
		LEAVE_OAMSA_TRANSLATIONS();
		return MafNotExist;
	}
	std::string theDN(dn);
	DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::action() txContextIn = %p, dn = %s", txContextIn.get(), theDN.c_str() );
	txContextIn->GetCache().SplitDN(theDN,theDNList);
	if (!theTranslator.IsDnListValid(theDNList, name))
	{
		invalidModelDetected("OamSAImmBridge::action", theDN, name);
		LEAVE_OAMSA_TRANSLATIONS();
		return MafFailure;
	}
	theTranslator.MO2Imm_DN(theDNList, immDn);
	// Check if the object has been modified
	if(txContextIn->isAdminOwnerFor(immDn, SA_IMM_ONE)) {
		ERR_OAMSA_TRANSLATIONS("OamSAImmBridge: Object has already been modified and cannot be used for invocation");
		LEAVE_OAMSA_TRANSLATIONS();
		return MafFailure;
	}

	// Allocate result, cleaned at cache destructor.
	struct MafMoAttributeValueContainer_3 *tmpResult = new MafMoAttributeValueContainer_3;
	tmpResult->nrOfValues = 0;
	tmpResult->type = MafOamSpiMoAttributeType_3_BOOL;
	tmpResult->values = NULL;

	txContextIn->GetCache().RegisterAttributeValueContainer(tmpResult);
	*result = tmpResult;

	//
	// Get OperationId
	//
	SaImmAdminOperationIdT operationId = 0;
	bool ok = false;
	std::string actionName (name);
	DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::action() txContextIn = %p, actionName = %s", txContextIn.get(), actionName.c_str() );
	MafOamSpiMoActionT* moAction = theTranslator.GetComAction(theDNList, actionName);
	if (moAction != 0)  {
		DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::action() Action = %s found in COM MO", actionName.c_str());
		MafOamSpiDomainExtensionT* dp = moAction->domainExtension;
		if (dp != 0 && dp->domain != 0 && !strcmp(dp->domain, DOMAIN_NAME_COREMW)) {

			bool admOpFound = false;
			for (MafOamSpiExtensionT* ep = dp->extensions; ep != 0; ep=ep->next) {
				if (ep->name != 0 && !strcmp(ep->name, DOMAIN_EXT_NAME_ADM_OP_ID)) {
					admOpFound = true;
					if (ep->value != 0) {
						operationId = strtoull (ep->value,0,10);
						if (errno != ERANGE || errno == 0) {
							ok = true;
						} else {
							ERR_OAMSA_TRANSLATIONS("OamSAImmBridge::action() Erroneous operationId on (dn=%s, name=%s, operationId=%s)", dn, name, ep->value);
							retVal = MafFailure; // Erroneous operationId
						}
					} else {
						ERR_OAMSA_TRANSLATIONS("OamSAImmBridge::action() No operationId specified on (dn=%s, name=%s)", dn, name);
						retVal = MafFailure; // No value specified
					}
				} // if admOpId
			} // for

			if (!admOpFound) {
				ERR_OAMSA_TRANSLATIONS("OamSAImmBridge::action() No admOpId specified on (dn=%s, name=%s)", dn, name);
				retVal = MafFailure; // No admOpId extension found
			}
		} else {
			ERR_OAMSA_TRANSLATIONS("OamSAImmBridge::action() No domain extension specified on (dn=%s, name=%s)", dn, name);
			retVal = MafFailure; // No domain extension
		}
	} else {
		ERR_OAMSA_TRANSLATIONS("OamSAImmBridge::action() No moAction specified on (dn=%s, name=%s)", dn, name);
		retVal = MafFailure; // No moaction
	}
	//
	// Set admin owner before a call to admin operation
	//
	if (ok) {
		DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::action() checkSetAdminOwner for immDN = %s",immDn.c_str());
		SaAisErrorT checkSetErr = checkSetAdminOwner(txContextIn.get(), immDn, SA_IMM_ONE);
		if(checkSetErr != SA_AIS_OK)
		{
			ERR_OAMSA_TRANSLATIONS("OamSAImmBridge::action() checkSetAdminOwner for immDN = %s if failed",immDn.c_str());
			ok = false;
			retVal = MafFailure;
			if (isAdminOwnerLockedByAnotherTx(txContextIn.get(), checkSetErr) || ( checkSetErr != SA_AIS_ERR_NOT_EXIST) )
			{
				// Some error logging should go in here, someone else holds a lock to the parent
				ERR_OAMSA_TRANSLATIONS("Trying to take ownership of the object failed, someone else has already locked");
				ERR_OAMSA_TRANSLATIONS("or some other error but not found occurred Error code %d", checkSetErr);
				retVal = MafObjectLocked;
			}
		}
	}

	// DN is removed from the cache, since we don't keep anymore DNs of actions
	txContextIn->removeAdminOwnerFor(immDn, SA_IMM_ONE);

	//
	// Call Admin operation
	//
	SaImmAdminOperationParamsT_2 ** returnParams = NULL;
	SaImmAdminOperationParamsT_2 ** returnParams_2 = NULL;	// use for error handling when parameter name error or other error
	returnParams_2 = new SaImmAdminOperationParamsT_2*[2];
	char *errorText = new char[ERR_STR_BUF_SIZE + 1];
	returnParams_2[0] = new SaImmAdminOperationParamsT_2;
	char errStr[] = "errorString";
	returnParams_2[0]->paramName = errStr;
	returnParams_2[0]->paramType = SA_IMM_ATTR_SASTRINGT;
	returnParams_2[0]->paramBuffer =  &errorText;
	returnParams_2[1] = NULL;

	SaAisErrorT operationReturnValue_adminOp = SA_AIS_OK;
	if (ok) {
		SaImmAdminOperationParamsT_2 **	opPP = NULL;
		SaAisErrorT operationReturnValue = SA_AIS_OK;

		// support actions with no input parameters, too.
		if (*parameters != NULL)
		{
			MafMoAttributeValueContainer_3T **pP = parameters;
			while (*pP) {
				DebugDumpAttributeContainer(*pP);
				pP++;
			}

			// Convert parameters sent to us into a format underestood by the object implementer
			// before passing them on

			retVal = theTranslator.ConvertToAdminOpParameters(moAction, parameters, &opPP);
		}
		else
		{
			DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::action() with no input parameters");
			SaImmAdminOperationParamsT_2** 	AdmOpParam_p = new SaImmAdminOperationParamsT_2*[1];
			AdmOpParam_p[0] = NULL;  // the proper way to signal 'no parameters' to IMM
			opPP = AdmOpParam_p;
		}

		if (MafOk == retVal)
		{
			DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::action() immDn = %s, operationId=%llu", immDn.c_str(), operationId);

			ImmCmdOmAdminOperationInvoke immCmdOmAdminOperationInvoke(txContextIn.get(), immDn, operationId,
											opPP,
											&operationReturnValue, &returnParams);
			SaAisErrorT saError = immCmdOmAdminOperationInvoke.execute();
			operationReturnValue_adminOp = operationReturnValue;

			if (saError != SA_AIS_OK)
			{
				ERR_OAMSA_TRANSLATIONS("OamSAImmBridge::action() immCmdOmAdminOperationInvoke_o2() failed saError = %d", saError);
				operationReturnValue = saError;
				//  (moved at the end of this function)
				//	getAndForwardErrorStrings(txContextIn.get());   // In case of error forward any error strings to COM (SDP875)
			}
                        else
                        {
                                saError = operationReturnValue;
                        }

			DebugDumpAdminOpReturnParams((const SaImmAdminOperationParamsT_2**)returnParams);

			/* SDP872: For backward compatibility in case there are no return parameters provided by the admin operation
			* then we return BOOL determined by the 'operationReturnValue' if the model specifies BOOL or VOID.
			*/
			if (NULL == returnParams || !(ImmCmd::mVersion.releaseCode >= immReleaseCode && ImmCmd::mVersion.majorVersion >= immMajorVersion && ImmCmd::mVersion.minorVersion >= immMinorVersion) || (operationReturnValue_adminOp != SA_AIS_OK))
			{
				// Set return value based on the 'operationReturnValue'
				DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::action() no return params from the admin op -> returning BOOL based on operationReturnValue");
				if (moAction->returnType.type == MafOamSpiDatatype_BOOL)
				{
					tmpResult->values = new MafMoAttributeValue_3[1];
					tmpResult->nrOfValues = 1;
					tmpResult->values->value.theBool = (operationReturnValue == SA_AIS_OK ? true : false );
				}
				else if (moAction->returnType.type == MafOamSpiDatatype_VOID)
				{
					WARN_OAMSA_TRANSLATIONS("OamSAImmBridge::action() VOID is not supported by MAF MO SPI Ver.3, returning nothing");
					tmpResult->values = NULL;
					// VOID is not supported by MAF MO SPI Ver.3
					tmpResult->nrOfValues = 0;
					tmpResult->type = MafOamSpiMoAttributeType_3_BOOL;
				}
			}
			else
			{
				//TBD: check IMM version
				if (moAction->returnType.type == MafOamSpiDatatype_STRUCT)
				{
					// handle struct
					retVal = theTranslator.HandleStructToMoAttribute(txHandle, (MafOamSpiDatatypeContainerT*)&moAction->returnType,returnParams, &tmpResult, errorText);
				}
				else
				{
					// simple (single or multivalue)
					retVal = theTranslator.ConvertSimpleToMoAttribute(txHandle, (MafOamSpiDatatypeContainerT*)&moAction->returnType, returnParams, &tmpResult, errorText);
				}
				if (MafOk != retVal)
				{
					getAndForwardErrorStrings_2(txContextIn.get(), returnParams_2);
				}
			}

			DebugDumpAttributeContainer(tmpResult);


			switch(saError)
			{
			case SA_AIS_OK:
				break;  // retVal is already ComOK
			case SA_AIS_ERR_LIBRARY:
			case SA_AIS_ERR_INIT:
			case SA_AIS_ERR_BAD_OPERATION:
			case SA_AIS_ERR_FAILED_OPERATION:
				retVal = MafFailure;
				break;
			case SA_AIS_ERR_TIMEOUT:
				retVal = MafTimeOut;
				break;
			case SA_AIS_ERR_TRY_AGAIN:
				retVal = MafTryAgain;
				break;
			case SA_AIS_ERR_BAD_HANDLE:
			case SA_AIS_ERR_INVALID_PARAM:
				retVal = MafInvalidArgument;
				break;
			case SA_AIS_ERR_NO_MEMORY:
			case SA_AIS_ERR_NO_RESOURCES:
			case SA_AIS_ERR_UNAVAILABLE:
				retVal = MafNoResources;
				break;
			case SA_AIS_ERR_NOT_EXIST:
				retVal = MafNotExist;
				break;
			case SA_AIS_ERR_BUSY:
				retVal = MafObjectLocked;
				break;
			default:
				retVal = MafFailure;
				break;
			}

		}

		// Release an ownership
		std::vector<std::string> objectNames;
		objectNames.push_back(immDn);
		ImmCmdOmAdminOwnerRelease immCmdOmAdminOwnerRelease(txContextIn.get(), &objectNames, SA_IMM_ONE);
		operationReturnValue = immCmdOmAdminOwnerRelease.execute();
		if(operationReturnValue != SA_AIS_OK)
		{
			ERR_OAMSA_TRANSLATIONS("OamSAImmBridge::action() immCmdOmAdminOwnerRelease() failed  operationReturnValue = %d", operationReturnValue);
			retVal = MafObjectLocked;
		}
		else
		{
			DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::action() immCmdOmAdminOwnerRelease() ok  operationReturnValue = %d", operationReturnValue);
		}

		// Need to walk through the parameter list to free what is enterred there since
		// these are C-struct and not C++ classes, hence no destructors to do the job

		int i;
		for (i = 0; opPP != NULL && opPP[i] != NULL;i++)
		{
			delete [] opPP[i]->paramName;
			switch (opPP[i]->paramType)
			{
			case SA_IMM_ATTR_SAINT32T:
				delete (SaInt32T*) opPP[i]->paramBuffer;
				break;
			case SA_IMM_ATTR_SAINT64T:
				delete (SaInt64T*) opPP[i]->paramBuffer;
				break;
			case SA_IMM_ATTR_SAUINT32T:
				delete (SaUint32T*) opPP[i]->paramBuffer;
				break;
			case SA_IMM_ATTR_SAUINT64T:
				delete (SaUint64T*) opPP[i]->paramBuffer;
				break;
			case SA_IMM_ATTR_SASTRINGT:
			{
				char* cptr = *(char**)(opPP[i]->paramBuffer);
				delete [] cptr;
				delete (char*)opPP[i]->paramBuffer;
			}
				break;
			case SA_IMM_ATTR_SAFLOATT:
				delete (SaFloatT*) opPP[i]->paramBuffer;
				break;
			case SA_IMM_ATTR_SADOUBLET:
				delete (SaDoubleT*) opPP[i]->paramBuffer;
				break;
			case SA_IMM_ATTR_SANAMET:
				saNameDelete((SaNameT*) opPP[i]->paramBuffer, true);
				break;
			default:
				// Should never end up here.
				ERR_OAMSA_TRANSLATIONS("OamSAImmBridge::action(): ERROR: Unknown SA_IMM_ATTR_ paramType: %d", opPP[i]->paramType);
				break;
			}
			if (opPP[i]) delete opPP[i], opPP[i]=0;
		}
		if (opPP) delete [] opPP, opPP=0;
	}

	// In case of error forward any error strings to COM (SDP875)
	if (operationReturnValue_adminOp != SA_AIS_OK)
	{
		getAndForwardErrorStrings_2(txContextIn.get(), returnParams);
		//getAndForwardErrorStrings(txContextIn.get());
	}
	ImmCmdOmAdminOperationMemoryFree immCmdOmAdminOperationMemoryFree(txContextIn.get(), returnParams);
	SaAisErrorT saError = immCmdOmAdminOperationMemoryFree.execute();
	if (saError != SA_AIS_OK)
	{
		ERR_OAMSA_TRANSLATIONS("OamSAImmBridge::action() immCmdOmAdminOperationMemoryFree() failed saError = %d", saError);
	}

	// free the allocated memory
	delete returnParams_2[0]; //	returnParams_2[0] = new SaImmAdminOperationParamsT_2;
	delete [] returnParams_2; //  	returnParams_2 = new SaImmAdminOperationParamsT_2*[2];
	delete [] errorText;      //  	char *errorText = new char[ERR_STR_BUF_SIZE + 1];

	LEAVE_OAMSA_TRANSLATIONS();
	return retVal;
}

/**
 * lockDn
 * @Description : Used to check if the immDn passed is locked under the transaction context
 *		  and lock the dn if it is not locked.
 */

MafReturnT OamSAImmBridge::lockDn( TxContext *txContextIn, std::string &theImmParentName, SaImmScopeT scope){
	ENTER_OAMSA_TRANSLATIONS();
	MafReturnT retVal = MafOk;
	SaAisErrorT immErr = SA_AIS_OK;
	immErr = checkSetAdminOwner(txContextIn, theImmParentName , scope);
	DEBUG_OAMSA_TRANSLATIONS("SetAdminOwner for dn: %s, rc = %d", theImmParentName.c_str(), immErr);
	// We assume that SA_AIS_ERR_NOT_EXIST is OK, since the parent might be elsewhere in the ccb waiting to be created
	if (immErr != SA_AIS_OK && immErr != SA_AIS_ERR_NOT_EXIST)
	{
		if (isAdminOwnerLockedByAnotherTx(txContextIn, immErr))
		{
			DEBUG_OAMSA_TRANSLATIONS("Administrative ownership of the parent object is locked by another transaction, Error code %d", immErr);
			retVal = MafObjectLocked;
		}
		else if (immErr == SA_AIS_ERR_EXIST)
		{
			// For the case object is locked by another process differs than ComSA
			DEBUG_OAMSA_TRANSLATIONS("Administrative ownership of the parent object is locked by another process, Error code %d", immErr);
			retVal = MafObjectLocked;
		}
		else
		{
			DEBUG_OAMSA_TRANSLATIONS("Taking ownership of object failed for other reason than already locked, Error code %d", immErr);
			retVal = MafFailure;
		}
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return retVal;
}

/**
 * unLockDnOnFailure
 * @Description : Used to release the lock on immDn under the transaction context.
 * If lock is not released warning is returned.
 */
MafReturnT OamSAImmBridge::unLockDnOnFailure( TxContext *txContextIn, std::string &immDn, SaImmScopeT scope){
	ENTER_OAMSA_TRANSLATIONS();
	MafReturnT retVal = MafOk;
	SaAisErrorT immErr = SA_AIS_OK;
	immErr = releaseAdminOwner(txContextIn, immDn, scope);
	if(immErr != SA_AIS_OK)
	{
		WARN_OAMSA_TRANSLATIONS("Failed to unlock immDn = %s scope = %d", immDn.c_str(), scope);
		retVal = MafObjectLocked;
	}else{
		DEBUG_OAMSA_TRANSLATIONS(" ReleaseAdminOwner is successful immDn = %s scope = %d ", immDn.c_str(), scope);
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return retVal;
}

/**
 * setImmMoAttribute
 */
MafReturnT OamSAImmBridge::setImmMoAttribute(MafOamSpiTransactionHandleT txHandle, const char *dn,
							const char *attributeName, MafMoAttributeValueContainer_3T *AttributeValue)
{
	ENTER_OAMSA_TRANSLATIONS();
	MafReturnT 		retVal = MafOk;
	std::string		immDn;
	std::tr1::shared_ptr<TxContext> txContextIn;
	OamSACache::DNList	theDNList;
	bool 				isAdminOwner = false;

	txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->getTxContext(txHandle);
	if (NULL == txContextIn.get())
	{
		DEBUG_OAMSA_TRANSLATIONS("setImmMoAttribute the context associated with handle %d was not found in the repository", (int)txHandle);
		LEAVE_OAMSA_TRANSLATIONS();
		return MafNotExist;
	}
	DEBUG_OAMSA_TRANSLATIONS("setImmMoAttribute DN = %s  attr name = %s",dn, attributeName);
	std::string theDN(dn);
	txContextIn->GetCache().SplitDN(theDN,theDNList);

	if (!theTranslator.IsDnListValid(theDNList, attributeName))
	{
		invalidModelDetected("OamSAImmBridge::setImmMoAttribute", theDN, attributeName);
		LEAVE_OAMSA_TRANSLATIONS();
		return MafFailure;
	}
	theTranslator.MO2Imm_DN(theDNList, immDn);

	if(lock_mo_for_config_change){
		isAdminOwner = txContextIn ->isAdminOwnerFor(immDn, SA_IMM_ONE);
		if(!isAdminOwner){
			DEBUG_OAMSA_TRANSLATIONS("setImmMoAttribute : Locking immDn=%s", immDn.c_str());
			retVal = lockDn(txContextIn.get(), immDn , SA_IMM_ONE);
			if(retVal != MafOk){
				DEBUG_OAMSA_TRANSLATIONS("setImmMoAttribute : Failed to lock immDn=%s", immDn.c_str());
				return retVal;
			}
		}
	}
	// Now check so that the referenced object isn't deleted
	if (!txContextIn->GetCache().IsDeleted(theDNList))
	{
		DEBUG_OAMSA_TRANSLATIONS("setImmMoAttribute()  referenced object isn't deleted ");
		// Handle complex attributes
		// TODO surely COM performs a check to see that the user is not allowed to write a struct to an non struct value
		//      hence the type value should be a safe way to detect the writing of a struct?
		//      If not then we have to perform a lookup of the attribute type.
		if(AttributeValue->type == MafOamSpiMoAttributeType_3_STRUCT){
			DEBUG_OAMSA_TRANSLATIONS("setImmMoAttribute RETURN writeStructToImm");
			LEAVE_OAMSA_TRANSLATIONS();
			retVal = writeStructToImm(txHandle, dn, attributeName, AttributeValue);
		}
		else{
			// handle simple attributes
			OamSACache::ObjectState objstate;
			std::string StringAttrName(attributeName);
			DEBUG_OAMSA_TRANSLATIONS("setImmMoAttribute : insert immDn=%s ", immDn.c_str());
			objstate = 	txContextIn->GetCache().UpdateObjectAttribute(theDNList,immDn, StringAttrName, AttributeValue);
			if (objstate != OamSACache::eObjectSuccess)
			{
				retVal = MafFailure;
			}
			DEBUG_OAMSA_TRANSLATIONS("setImmMoAttribute retVal: %d", retVal);
		}
	}
	else
	{
		retVal = MafFailure;
	}

	// Setting of attribute failed and also the lock on parent MO is set because of this operation,
	// so release the lock as it is no longer valid to hold the lock.
	if(retVal!= MafOk && lock_mo_for_config_change && !isAdminOwner){
		unLockDnOnFailure(txContextIn.get(), immDn, SA_IMM_ONE);
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return retVal;
}

/**
 *  createImmMo
 */
MafReturnT OamSAImmBridge::createImmMo(MafOamSpiTransactionHandleT txHandle,
										const char *parentDn,
										const char *className,
										const char *keyAttributeName,
										const char *keyAttributeValue,
										MafMoNamedAttributeValueContainer_3T **initialAttributes)
{
	ENTER_OAMSA_TRANSLATIONS();
	MafReturnT			retVal = MafOk;
	std::string 		theImmParentDn;
	std::string         theImmParentName;
	std::tr1::shared_ptr<TxContext> txContextIn;
	OamSACache::DNList	theParentDNList;
	std::string			theParentStringName(parentDn);
	bool 				isAdminOwner = false;

	// Check if every character in keyAttributeValue has graphical representation
	// This is required by IMM
	if (keyAttributeValue != NULL)
	{
		int result = 0;
		int len = strlen(keyAttributeValue);

		for (int pos = 0; pos < len ; pos++)
		{
			unsigned char chr = keyAttributeValue[pos];
			/* The characters with graphical representation are all those characters than
				* can be printed (as determined by isprint) except the espace character (' ').
				* isgraph() will return non-zero value if the character has graphical representation
				*/
			if (isgraph(chr) == 0)
			{
					result = -1;
					break;
			}
		}

		if (result != 0)
		{
			LOG_OAMSA_TRANSLATIONS("createImmMo IMM name contains character that doesn't have graphical representation");
			LEAVE_OAMSA_TRANSLATIONS();
			return MafInvalidArgument;
		}
	}

	DEBUG_OAMSA_TRANSLATIONS("createImmMo entry");
	DEBUG_OAMSA_TRANSLATIONS("createImmMo parent \"%s\" ", parentDn);
	DEBUG_OAMSA_TRANSLATIONS("createImmMo class  \"%s\" key attr name \"%s\" key attr value\"%s\" ", className, keyAttributeName, keyAttributeValue );
	txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->getTxContext(txHandle);
	if (NULL == txContextIn.get())
	{
		DEBUG_OAMSA_TRANSLATIONS("createImmMo the context associated with handle %d was not found in the repository", (int)txHandle);
		LEAVE_OAMSA_TRANSLATIONS();
		return MafNotExist;
	}
	txContextIn->GetCache().SplitDN(theParentStringName, theParentDNList);

	if (!theTranslator.IsDnListValid(theParentDNList, className))
	{
		invalidModelDetected("OamSAImmBridge::createImmMo", theParentStringName, className);
		LEAVE_OAMSA_TRANSLATIONS();
		return MafFailure;
	}

	if(lock_mo_for_config_change){
		theTranslator.MO2Imm_DN(theParentDNList, theImmParentName);
		isAdminOwner = txContextIn ->isAdminOwnerFor(theImmParentName, SA_IMM_ONE);
		if(!isAdminOwner){
			DEBUG_OAMSA_TRANSLATIONS("createImmMo : Setting lock for immDn=%s", theImmParentName.c_str());
			retVal = lockDn(txContextIn.get(), theImmParentName , SA_IMM_ONE);
			if(retVal != MafOk){
				DEBUG_OAMSA_TRANSLATIONS("createImmMo : Failed to lock immDn=%s", theImmParentName.c_str());
				return retVal;
			}
		}
	}

	std::string dummyString;
	std::string parentClassName;
	retrieveLastElementFromDn(theParentDNList,
									parentClassName,
									dummyString,
									true);
	std::string keyAttributeString = theTranslator.getImmKeyAttribute(parentClassName.c_str(),
																	className,
																	keyAttributeName);

	std::string theObjectName;
	std::string theClassName = translateClassName(txContextIn.get(), parentDn, className);
	std::string theClassNameMOMStyle(className);

	// A list to be used later to initialize the attributes from the input array initialAttributes
	std::list<std::string> attrList;

	DEBUG_OAMSA_TRANSLATIONS("createImmMo parentDn=%s, className=%s, keyAttrName=%s, keyAttrValue=%s",parentDn,className, keyAttributeString.c_str(), keyAttributeValue);
	theTranslator.BuildObjectName(parentDn, theClassNameMOMStyle, keyAttributeName, keyAttributeValue, theObjectName);
	OamSACache::DNList	theDNList;
	txContextIn->GetCache().SplitDN(theObjectName, theDNList);
	if (!txContextIn->GetCache().IsDeleted(theDNList))
	{
		DEBUG_OAMSA_TRANSLATIONS("Printf the object %s was not found in deleted list, inserting in created list", theObjectName.c_str());
		if (txContextIn->GetCache().InsertCreatedObject(theObjectName, theClassName, parentDn, theImmParentDn, keyAttributeString.c_str(), keyAttributeValue) != OamSACache::eObjectSuccess)
		{
			retVal = MafFailure;
		}
	}
	txContextIn->GetCache().RemoveDeletedObject(theObjectName);

	if (MafOk != retVal) {
		DEBUG_OAMSA_TRANSLATIONS("createImmMo Failed to insert created object in the Cache. retVal:%d", retVal);
		if(lock_mo_for_config_change && !isAdminOwner){
			unLockDnOnFailure(txContextIn.get(), theImmParentName, SA_IMM_ONE);
		}
		LEAVE_OAMSA_TRANSLATIONS();
		return retVal;
	}

	if( NULL == initialAttributes || NULL == *initialAttributes)
	{
		DEBUG_OAMSA_TRANSLATIONS("createImmMo initialAttributes container is not provided for createImmMo.");
		LEAVE_OAMSA_TRANSLATIONS();
		return retVal;
	}

	// Now we have to spin through the attributes in order to find those, if any, which are structs
	OamSACache::DNList	theObjectDNList;
	// In the code below there is usage of search functions working with the MOM representation of the object to be created.
	// However then we can not use the IMM decorated name for this, it will not give any match in the MOM.
	txContextIn->GetCache().SplitDN(theObjectName, theObjectDNList);
	for (MafOamSpiMoAttributeT* ap = theTranslator.GetComAttributeList(theObjectDNList); ap != NULL; ap = ap->next)
	{
		attrList.push_back(ap->generalProperties.name);

		if((ap->type != (MafOamSpiMoAttributeTypeT) MafOamSpiMoAttributeType_3_STRUCT) || (ap->structDatatype == NULL)) {
			continue;
		}

		bool structInInitAttr = false;
		std::string attr(ap->generalProperties.name);
		MafMoNamedAttributeValueContainer_3T** attrlist = initialAttributes;
		while (*attrlist) {
			std::string inattr((**attrlist).name);
			if (attr == inattr) {
				DEBUG_OAMSA_TRANSLATIONS("createImmMo structs captured in initattr:%s",inattr.c_str());
				structInInitAttr = true;
				break;
			}
		        ++attrlist;
		}

		if (structInInitAttr)
		{
			std::string instanceId(ap->generalProperties.name);
			instanceId.append(STRUCT_CLASS_INDEX_SEPARATOR);
			instanceId.append("0");
			std::string uniqueId(ap->structDatatype->generalProperties.name);
			uniqueId.append(".");
			uniqueId.append(STRUCT_CLASS_KEY_ATTRIBUTE);
			uniqueId.append("=");
			uniqueId.append(instanceId);
			std::string keyAttribute = theTranslator.Convert3GPPNameFragmentToImmNameFragment(uniqueId);
			size_t equalSignPos = keyAttribute.find("=");
			if(equalSignPos != std::string::npos)
			{
				keyAttribute = keyAttribute.substr(0, equalSignPos);
			}
			// Inside this creation we translate the class name separately
			// The initial attribute values are not used here, so sett it to NULL
			// the createImmMo should detect this
			DEBUG_OAMSA_TRANSLATIONS("createImmMo RECURSIVE CALL");
			retVal = createImmMo(txHandle, theObjectName.c_str(), ap->structDatatype->generalProperties.name, keyAttribute.c_str(), instanceId.c_str(), NULL);
			// if correct type then traverse members and build a list!
			if (MafOk != retVal)
			{
				ERR_OAMSA_TRANSLATIONS("createImmMo retVal = %d", retVal);
				break;
			}
			std::string AttrObjName;
			std::string theStructClassName = ap->structDatatype->generalProperties.name;
			//TODO code to be refactored to avoid BuildObjectName call 2 times as do it
			//     as part of createImmMo recursive call as well.
			theTranslator.BuildObjectName(theObjectName.c_str(),
												theStructClassName,
										keyAttribute.c_str(),
										instanceId.c_str(), AttrObjName);
			// Write the reference to the parent struct attribute reference
			MafMoAttributeValueContainer_3T *valueRef = new MafMoAttributeValueContainer_3T;
			valueRef->type = MafOamSpiMoAttributeType_3_REFERENCE;
			valueRef->nrOfValues = 1;
			valueRef->values = new MafMoAttributeValue_3T[1];
			char* tmp = new char[AttrObjName.size()+1];
			strcpy(tmp,AttrObjName.c_str());
			valueRef->values[0].value.moRef = tmp;
			txContextIn->GetCache().RegisterAttributeValueContainer(valueRef);
			OamSACache::ObjectState objstate;
			std::string theStructAttrName(ap->generalProperties.name);
			DEBUG_OAMSA_TRANSLATIONS("createImmMo UpdateObjectAttribute parentDN= \"%s\" AttrName= \"%s\" Reference \"%s\" ",theObjectName.c_str(), theStructAttrName.c_str(),AttrObjName.c_str());
			objstate = txContextIn->GetCache().UpdateObjectAttribute(theObjectDNList,theImmParentDn, theStructAttrName, valueRef);
			if (objstate != OamSACache::eObjectSuccess)
			{
				ERR_OAMSA_TRANSLATIONS("OamSAImmBridge::createImmMo RETURN FAILURE, UpdateObjectAttribute FAILED");
				retVal = MafFailure;
				break;
			}
		}
	}

	// set the MO attributes to the provided initial values
	if (MafOk == retVal)
	{
		// use a separate return variable here as we may decide to continue through the
		// array if error happens, TBC
		MafReturnT retValInitAttr = MafOk;
		MafMoNamedAttributeValueContainer_3T** pit = initialAttributes;
		//std::string AttributeValue(keyAttributeValue); // Unused variable
		while (*pit) // the named container array is NULL terminated
		{
			DEBUG_OAMSA_TRANSLATIONS("createImmMo initialAttributes: Container Name = %s", (**pit).name);
			MafMoAttributeValueContainer_3T* p_theValue = &((**pit).value);
			MafMoAttributeValueContainer_3T** pV = &p_theValue;
			DebugDumpAttributeContainer(*pV);
			std::list<std::string>::iterator attrListIter;
			DEBUG_OAMSA_TRANSLATIONS("createImmMo MoAttributeList has %u items", (unsigned int) attrList.size());
			// try to find the attribute name in the list. Not fatal if not found
#ifdef _TRACE_FLAG
			int i = 1;
#endif
			std::string attrInitName((**pit).name);
			attrListIter = std::find(attrList.begin(), attrList.end(), attrInitName);
			if (attrListIter != attrList.end()) {
				DEBUG_OAMSA_TRANSLATIONS("createImmMo MoAttribute: (%s) match found, initializing...", attrInitName.c_str());
				retValInitAttr = setImmMoAttribute(txHandle, theObjectName.c_str(), attrInitName.c_str(), *pV);
				//TODO Code to be refactored instead of continuation incase of not MafOK.
				if (MafOk != retValInitAttr)
				{
					ERR_OAMSA_TRANSLATIONS("createImmMo initialAttributes: Attribute %s initialization failed", attrInitName.c_str());
					retVal = retValInitAttr;
				}
			}
			else {
				DEBUG_OAMSA_TRANSLATIONS("createImmMo MoAttribute: (%s) not found", attrInitName.c_str());
			}
			//*pit++; // ++ (post incrementor) operator is performed before the * (dereference) operator, so dereference is meaningless.
			pit++;
		}
	}

	// Creation of ChildMo failed and also the lock on parent MO is set because of this operation,
	// so release the lock as it is no longer valid to hold the lock.
	if(retVal!= MafOk && lock_mo_for_config_change && !isAdminOwner){
		unLockDnOnFailure(txContextIn.get(), theImmParentName, SA_IMM_ONE);
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return retVal;
}

/**
 *  deleteImmMo
 */
MafReturnT OamSAImmBridge::deleteImmMo(MafOamSpiTransactionHandleT txHandle, const char *dn)
{
	ENTER_OAMSA_TRANSLATIONS();
	MafReturnT			retVal = MafOk;
	std::string 		theImmDn;
	std::tr1::shared_ptr<TxContext> txContextIn;
	OamSACache::DNList	theDNList;
	std::string			theStringName(dn);
	bool 				isAdminOwnerForSubtree =false;
	bool 				isAdminOwnerForDn = false;
	txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->getTxContext(txHandle);
	if (NULL == txContextIn.get())
	{
		DEBUG_OAMSA_TRANSLATIONS("deleteImmMo the context associated with handle %d was not found in the repository", (int)txHandle);
		LEAVE_OAMSA_TRANSLATIONS();
		return MafNotExist;
	}
	else
	{
		DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::deleteImmMo dn = %s, txContextIn = %d ", theStringName.c_str(), (int)txHandle );
		// Start by translating the name of the parent into the IMM format
		txContextIn->GetCache().SplitDN(theStringName, theDNList);

		if (!theTranslator.IsDnListValid(theDNList, ""))
		{
			invalidModelDetected("OamSAImmBridge::deleteImmMo", theStringName, "");
			LEAVE_OAMSA_TRANSLATIONS();
			return MafFailure;
		}

		theTranslator.MO2Imm_DN(theDNList, theImmDn);
		if(lock_mo_for_config_change){
			isAdminOwnerForSubtree = txContextIn ->isAdminOwnerFor(theImmDn, SA_IMM_SUBTREE);
			if(!isAdminOwnerForSubtree){
				isAdminOwnerForDn = txContextIn ->isAdminOwnerFor(theImmDn, SA_IMM_ONE);
				retVal = lockDn(txContextIn.get(), theImmDn , SA_IMM_SUBTREE);
				if(retVal != MafOk){
					DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::deleteImmMo : Failed to lock immDn=%s", theImmDn.c_str());
					return retVal;
				}
			}
		}
		DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::deleteImmMo theImmDn = %s, txContextIn = %d ", theImmDn.c_str(), (int)txHandle );
		// Moved locking (getting administrative ownership) to prepare phase in order to keep the lock for as short time
		// as possible.
		// Remove it from the created list if there. This also nullifies the delete which will not be put in the cache
		if (!txContextIn->GetCache().RemoveCreatedObject(theDNList))
		{
			DEBUG_OAMSA_TRANSLATIONS("Printf the object %s was not found in created list, insering in deleted list", theStringName.c_str());
			if (txContextIn->GetCache().InsertDeletedObject(theDNList,theImmDn) != OamSACache::eObjectSuccess)
			{
				ERR_OAMSA_TRANSLATIONS("deleteImmMo failed inserting in deleted list.");
				retVal = MafFailure;
			}
		}
		else
		{
			DEBUG_OAMSA_TRANSLATIONS("The object %s was found in created list, NOT inserted in deleted list", theStringName.c_str());
		}
	}

	if( retVal!=MafOk && lock_mo_for_config_change && !isAdminOwnerForSubtree){
		DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::deleteImmMo Unlocking immDn = %s, scope = %d", theImmDn.c_str(), SA_IMM_SUBTREE);
		unLockDnOnFailure(txContextIn.get(), theImmDn, SA_IMM_SUBTREE);
		// If the MO is locked earlier we need to restore the lock on MO
		if(isAdminOwnerForDn){
			MafReturnT retVal1 = MafOk;
			retVal1 = lockDn(txContextIn.get(), theImmDn , SA_IMM_ONE);
			if(retVal1 != MafOk){
				DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::deleteImmMo : Failed to restore lock immDn = %s scope = %d", theImmDn.c_str(), SA_IMM_ONE);
			}
		}
	}
	DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::deleteImmMo retVal = %d", retVal );
	LEAVE_OAMSA_TRANSLATIONS();
	return retVal;
}

/**
 *  existsImmMo
 */
MafReturnT OamSAImmBridge::existsImmMo(MafOamSpiTransactionHandleT txHandle, const char * dn, bool * result)
{
	ENTER_OAMSA_TRANSLATIONS();
	MafReturnT retVal = MafOk;
	std::string	theStringName(dn);
	OamSACache::DNList	theDNList;
	std::tr1::shared_ptr<TxContext> txContextIn;

	*result = false;
	txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->getTxContext(txHandle);
	if (NULL == txContextIn.get())
	{
		DEBUG_OAMSA_TRANSLATIONS("existImmMo:  the context associated with handle %d was not found in the repository", (int)txHandle);
		retVal = MafNotExist;
	}
	else
	{
		DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::existImmMo: dn = %s, txContextIn = %d ", theStringName.c_str(), (int)txHandle );
		txContextIn->GetCache().SplitDN(theStringName, theDNList);

		if (!theTranslator.IsDnListValid(theDNList, ""))
		{
			invalidModelDetected("OamSAImmBridge::existsImmMo", theStringName, "");
			LEAVE_OAMSA_TRANSLATIONS();
			return MafFailure;
		}

		/* 1. Check if object exists in cache */
		// 1a. First check in deleted object list. If true return false, because after committing this object will be deleted in IMM
		if (txContextIn->GetCache().IsDeleted(theDNList))
		{
			*result = false;
			goto end_exit;
		}
		// 1b. Second check in created and modified object list.
		//     If true return true, because with creating new object, after committing this object will be created in IMM.
		//     With modifying object, the existence of this object is not change after committing
		if (txContextIn->GetCache().IsInCache(theDNList))
		{
			*result = true;
			goto end_exit;
		}

		/* 2. If 1 false, Else check directly in IMM */
		std::string theIMMName;
                theTranslator.MO2Imm_DN(theDNList, theIMMName);
                MafReturnT ret = txContextIn->GetCache().checkObjectExistInImm(theIMMName);
                if (MafOk != ret) {
                        if (MafNotExist == ret) {
                                DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::existsImmMo: the object %s is not existed in IMM err = %d", theStringName.c_str(), (int) ret);
                        }
                        else {
                                ERR_OAMSA_TRANSLATIONS("OamSAImmBridge::existsImmMo: FAILED err = %d", (int) ret);
                                retVal = MafFailure;
                        }
                        *result = false;
                }
                else {
                        DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::existsImmMo: the object %s is existed in IMM", theStringName.c_str());
                        *result = true;
                }
        }
end_exit:
        DEBUG_OAMSA_TRANSLATIONS("existImmMo: result is %s", (*result)?"true":"false");
        LEAVE_OAMSA_TRANSLATIONS();
        return retVal;

}

/**
 *  countImmMoChildren
 */
MafReturnT OamSAImmBridge::countImmMoChildren(MafOamSpiTransactionHandleT txHandle, const char * dn, const char * className, uint64_t * result)
{
	ENTER_OAMSA_TRANSLATIONS();
	std::string dn_str(dn);
	std::string className_str(className);
	DEBUG_OAMSA_TRANSLATIONS("countImmMoChildren: dn = %s , className = %s", dn_str.c_str(), className_str.c_str());

	/* 1.check in IMM */
	MafOamSpiMoIteratorHandle_3T it;
	MafReturnT retVal = OamSAImmBridge::getImmMoIterator(txHandle, dn_str.c_str(), className_str.c_str(),&it);
	if (MafOk == retVal) {
		char *key = 0;
		MafReturnT ret = MafOk;
		*result = 0;
		while(MafOk == ret)
		{
			ret = OamSAImmBridge::getImmNextMo(it, &key);
			DEBUG_OAMSA_TRANSLATIONS("countImmMoChildren: key is %s", key);
			if (key == 0)
				break;
			else
				(*result)++;
		}
		/* 2.check in cache */
		//2a. check in modify object list, count number of create object. No need because getImmNextMo has already checked it
		//2b. check in delete object list. No need because getImmNextMo has already checked it

		DEBUG_OAMSA_TRANSLATIONS("countImmMoChildren: number of children is %lu", (*result));
		retVal = OamSAImmBridge::finalizeImmMoIterator(it);
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return retVal;
}

/**
 *  getImmMoIterator
 */
MafReturnT OamSAImmBridge::getImmMoIterator(MafOamSpiTransactionHandleT txHandle,
											const char *dn,const char *className, MafOamSpiMoIteratorHandle_3T *ith)
{
	ENTER_OAMSA_TRANSLATIONS();
	MafReturnT retVal = MafOk;
	std::tr1::shared_ptr<TxContext> txContextIn;
	txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->getTxContext(txHandle);
	if (NULL == txContextIn.get()) {
		DEBUG_OAMSA_TRANSLATIONS("getImmMoIterator the context associated with handle %d was not found in the repository", (int)txHandle);
		retVal = MafNotExist;
	}else {
		OamSACache::DNList	DNNameList;
		std::string	theName(dn);
		std::string theImmDn;
		//std::string ParentName; // Unused variable
		txContextIn->GetCache().SplitDN(theName,DNNameList);
		theTranslator.MO2Imm_DN(DNNameList, theImmDn);
		std::string myClassName = translateClassName(txContextIn.get(), theName, className);

		// MR26712 changes: now need to pass IsImmRoot() the undecorated class name and parent dn to determine unique class names in IMM.
		std::string undecoratedClassName(className);
		DEBUG_OAMSA_TRANSLATIONS("getImmMoIterator myClassName \"%s\" undecorated className \"%s\" dn \"%s\"", myClassName.c_str(), undecoratedClassName.c_str(), theName.c_str());
		if (theTranslator.IsImmRoot(myClassName, className, theName))
		{
			// If the Imm Name is root, truncate
			theImmDn = "";
		}

		DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::getImmMoIterator DN \"%s\" IMM name \"%s\" class \"%s\"", theName.c_str(), theImmDn.c_str(), myClassName.c_str());
		BridgeImmIterator *biip = new BridgeImmIterator(dn, myClassName.c_str(), theImmDn.c_str());
		biip->SetTxContextH(txHandle);
		txContextIn->GetCache().RegisterBridgeImmIter(biip);
		//
		// If this iterator is set on anything else but "" (.i.e top level) the look to see if there is any
		// objects which are below in the Ecim model, but might be a root in IMM
		//
		*ith = (MafOamSpiMoIteratorHandle_3T)biip; // in COM SPI Ver.3 the handle type has changed from void* to uint64_t
		DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::getImmMoIterator : serachkeys %s %s", (biip->GetImmName()).c_str(), (biip->GetClass()).c_str());

		ImmCmdOmKeySearchInit SintiCmd(txContextIn.get(), biip->GetImmName(), biip->GetClass(), biip);
		SaAisErrorT immErr = SintiCmd.doExecute();
		DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::getImmMoIterator after call to SintiCmd.doExecute() immErr (SA_AIS_OK == 1) %d ",immErr);
		if (immErr != SA_AIS_OK)
		{
			biip->ResetImmSearchInitialized();
			if (SA_AIS_ERR_NOT_EXIST == immErr)
			{
				if (!txContextIn->GetCache().IsInCache(DNNameList))
				{
					retVal = MafNotExist;
				}
			}
			else
			{
				retVal = MafFailure;
			}
		}
		else
		{
			biip->SetImmSearchInitialized();
		}
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return retVal;
}


/**
 *  finalizeImmMoIterator
 */
MafReturnT OamSAImmBridge::finalizeImmMoIterator(MafOamSpiMoIteratorHandle_3T itHandle)
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("finalizeImmMoIterator: itHandle = %lu", itHandle);
	MafReturnT retVal = MafOk;

	// within each cache transaction context search within the bridgeImmIterList for this
	// itrator handle id. If found then release the memory and exit
	std::tr1::shared_ptr<TxContext> txContextIn;
	BridgeImmIterator *biip;
	biip = (BridgeImmIterator*) itHandle; // in COM SPI Ver.3 the handle type has changed from void* to uint64_t
	txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->getTxContext(biip->GetTxContextH());
	if (NULL == txContextIn.get())
	{
		DEBUG_OAMSA_TRANSLATIONS("finalizeImmMoIterator the context associated with handle %d was not found in the repository", (int)biip->GetTxContextH());
		LEAVE_OAMSA_TRANSLATIONS();
		return MafNotExist;
	}

	DEBUG_OAMSA_TRANSLATIONS("finalizeImmMoIterator: txContextIn = %lu ...", (unsigned long) txContextIn.get());
	// iterate over the iterator list in the cache
	bool foundIterInCache = false;
	std::list<BridgeImmIterator*>::iterator iter    = txContextIn->GetCache().GetBridgeImmIter().begin();
	std::list<BridgeImmIterator*>::iterator iterEnd = txContextIn->GetCache().GetBridgeImmIter().end();

	for (; iter != iterEnd; ++iter)
	{
		DEBUG_OAMSA_TRANSLATIONS("finalizeImmMoIterator: iter = %lu", (unsigned long) *iter);
		if ((uint64_t) *iter == itHandle)
		{
			DEBUG_OAMSA_TRANSLATIONS("finalizeImmMoIterator: deleting itHandle from the search handles map");
			txContextIn->deleteSearchHandle(*iter);
			DEBUG_OAMSA_TRANSLATIONS("finalizeImmMoIterator: found match, deleting itHandle = %lu ...", (unsigned long) itHandle);
			// Delete the iterator and its allocated memory
			delete (*iter);
			txContextIn->GetCache().GetBridgeImmIter().erase(iter);
			foundIterInCache = true;
			break;
		}
	}

	if (!foundIterInCache)
	{
		retVal = MafFailure;
		ERR_OAMSA_TRANSLATIONS("finalizeImmMoIterator: Iterator handle %lu not found in the cache list", (unsigned long) itHandle);
	}

	std::map<BridgeImmIterator*, std::list<char*> >& itMap = txContextIn->GetCache().GetTheBridgeImmIterMap();

	// check if there is an entry for this iterator in the map
	if (itMap.find((BridgeImmIterator*)itHandle) == itMap.end())
	{
		// It is possible that the iterator was created but never used, then not finding it in the map
		// is not an error. Also it may have been used, but if nextImmMo did not find anything again
		// there will be no entry in the map. So, only log a debug message.
		// retVal = MafFailure;
		DEBUG_OAMSA_TRANSLATIONS("finalizeImmMoIterator: Iterator handle %lu not found in the cache map", (unsigned long) itHandle);
	}

	// iterate over the char string list for this iterator in the cache iterator map
	std::list<char*>::iterator it    = itMap[(BridgeImmIterator*)itHandle].begin();
	std::list<char*>::iterator itEnd = itMap[(BridgeImmIterator*)itHandle].end();

	for (; it != itEnd; ++it)
	{
		DEBUG_OAMSA_TRANSLATIONS("finalizeImmMoIterator: deleting char string = %p for iterator %lu", *it, (unsigned long) itHandle);
		// this erase() seems to make the list iterator invalid and causing segmentation fault ... old C++ STL?
		// So. we call clear() after the loop - does the same thing after all.
		// itMap[(BridgeImmIterator*)itHandle].erase(it);
		delete [] *it;
	}

	itMap[(BridgeImmIterator*)itHandle].clear();  // erase the whole char string list
	itMap.erase((BridgeImmIterator*)itHandle);    // erase the key of the map

	LEAVE_OAMSA_TRANSLATIONS();
	return retVal;
}


/**
 *  getImmNextMo
 */
MafReturnT OamSAImmBridge::getImmNextMo (MafOamSpiMoIteratorHandle_3T itHandle, char **result)
{
	ENTER_OAMSA_TRANSLATIONS();
	MafReturnT retVal = MafOk;
	std::tr1::shared_ptr<TxContext> txContextIn;
	BridgeImmIterator *biip;
	biip = (BridgeImmIterator*) itHandle; // in COM SPI Ver.3 the handle type has changed from void* to uint64_t
	txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->getTxContext(biip->GetTxContextH());
	//To avoid recursive calls, retCnt variable for handling SA_AIS_ERR_BAD_HANDLE case
	short nRetries = 0;
	DEBUG_OAMSA_TRANSLATIONS("getImmNextMo txContext %16.16lx", (unsigned long)txContextIn.get());
	if (NULL == txContextIn.get())
	{
		DEBUG_OAMSA_TRANSLATIONS("getImmNextMo the context associated with handle %d was not found in the repository", (int)biip->GetTxContextH());
		retVal = MafNotExist;
	}
	else
	{
		OamSACache::DNList theObjectList;
		txContextIn->GetCache().SplitDN(biip->GetRoot(), theObjectList);
		std::string theSibling = txContextIn->GetCache().GetNextChildObject(theObjectList, biip->GetClass(), biip->GetLastCacheObj());
		DEBUG_OAMSA_TRANSLATIONS("getImmNextMo after getting sibling from cache theSibling = \"%s\" ", theSibling.c_str());
		if (!theSibling.empty())
		{
			// The next sibling was found in cache, so return it. No need to look further in IMM

			// Need to extract the last attribute value from the name. Now we look from the rear
			// since 3Gpp format is the opposite from IMM format

			size_t equalpos = theSibling.rfind('=');

			std::string attrValue = theSibling.substr(equalpos+1);

			char *namep = new char[attrValue.length() + 1];
			strcpy(namep, attrValue.c_str());
			*result = namep;
			DEBUG_OAMSA_TRANSLATIONS("getImmNextMo: allocated char* %p, itHandle %lu", namep, (unsigned long) itHandle);
			// Register the string to be able to delete it together with the context
			txContextIn->GetCache().RegisterCString(biip, namep);
			biip->SetLastCacheObj(theSibling);
		}
		else
		{
			if (biip->IsImmSearchInitialized())
			{
				std::string OutputName;
				SaImmAttrValuesT_2 **theAttributeValues;
				SaAisErrorT immErr;
				DEBUG_OAMSA_TRANSLATIONS("getImmNextMo looking for theSibling from IMM parent = \"%s\" ", biip->GetImmName().c_str());
				for(;;)
				{
					// Look for the next sibling in IMM which hasn't been deleted. Need to look in the cache to determine that.

					ImmCmdOmSearchNext immCmdOmSearchNext(txContextIn.get(), biip->GetImmName(), biip,&OutputName, &theAttributeValues);
					immErr = immCmdOmSearchNext.doExecute();
					DEBUG_OAMSA_TRANSLATIONS("getImmNextMo after the call to IMM object name = \"%s\" imm status  (SA_AIS_OK == 1) %d", OutputName.c_str(), immErr);
					if (SA_AIS_OK != immErr)
					{
						if ((SA_AIS_ERR_BAD_HANDLE == immErr) && (nRetries++ < 3)) {
							//delete the handle
							txContextIn.get()->deleteSearchHandle(biip);
							biip->ResetImmSearchInitialized();
							// initialize the handle again
							ImmCmdOmKeySearchInit SintiCmd(txContextIn.get(), biip->GetImmName(), biip->GetClass(), biip);
							immErr = SintiCmd.doExecute();
							DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::getImmNextMo after call to SintiCmd.doExecute() immErr (SA_AIS_OK == 1) %d ",immErr);
							if (immErr != SA_AIS_OK)
							{
								biip->ResetImmSearchInitialized();
							}
							else
							{
								biip->SetImmSearchInitialized();
								continue;
							}
						}
						break;
					}
					OamSACache::DNList theTempObjectList = theObjectList;
					std::string theOutputStr = OutputName.substr(0,OutputName.find(','));

					theTempObjectList.push_back(theTranslator.ConvertImmNameFragmentTo3GPPNameFragmentWoMomPre(biip->GetTxContextH(),
																						theOutputStr,
																						OutputName));

					//theTempObjectList.push_back(theTranslator.ConvertImmNameFragmentTo3GPPNameFragment(theOutputStr));
					if (!txContextIn->GetCache().IsDeleted(theTempObjectList))
					{
						break;
					}
				}

				if (SA_AIS_OK == immErr)
				{
					// We should only return the VALUE of the naming attribute. The quickest way to
					// get a grip on that is to parse the object name looking for whatever is between
					// the first '=' from the left (remember this is an IMM name, with the nodes in the
					// reverse order compared to 3Gpp) and the first ','

					size_t equalpos = OutputName.find('=');
					size_t commapos = OutputName.find(',');

					// There might not be a comma, for instance if the object is a top level object, in that case take the entire string

					if (std::string::npos == commapos)
					{
						OutputName = OutputName.substr(equalpos+1);
					}
					else
					{
						OutputName = OutputName.substr(equalpos+1, commapos - (equalpos+1));
					}
					char *namep = new char[OutputName.length() + 1];
					strcpy(namep, OutputName.c_str());
					*result = namep;
					DEBUG_OAMSA_TRANSLATIONS("getImmNextMo: allocated char* %p, itHandle %lu", namep, (unsigned long) itHandle);
					txContextIn->GetCache().RegisterCString(biip, namep);
				}
				else if (SA_AIS_ERR_NOT_EXIST == immErr)
				{
					retVal = MafOk;
					*result = NULL;
				}
				else
				{
					retVal = MafFailure;
				}
			}
			else
			{
				// if not found in cache and no search in IMM it's not there
				retVal = MafOk;
				*result = NULL;
			}
		}
	}
	DEBUG_OAMSA_TRANSLATIONS("getImmNextMo returning retVal = %d", retVal);
	LEAVE_OAMSA_TRANSLATIONS();
	return retVal;
}

/**
 *  OamSAJoin
 */
MafReturnT OamSAImmBridge::OamSAJoin(MafOamSpiTransactionHandleT txHandle)
{
	ENTER_OAMSA_TRANSLATIONS();
	MafReturnT retVal = MafOk;
	DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::OamSAJoin...txhandle %lu", txHandle);
	if (NULL != OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txHandle))
	{
		// Register participant
		if (NULL != MafOamSpiTransactionStruct_p_v2)
		{
			MafOamSpiTransactionStruct_p_v2->registerParticipant(txHandle, ExportOamSATransactionalResourceInterface_V2());
		}
		else
		{
			retVal = MafFailure;
		}
	}
	else
	{
		retVal =  MafFailure;
	}
	DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::OamSAJoin...leaving retVal %d", retVal);
	LEAVE_OAMSA_TRANSLATIONS();
	return retVal;
}

/*
 * OamSAPrepare
 */
MafReturnT OamSAImmBridge::OamSAPrepare(MafOamSpiTransactionHandleT txHandle)
{
	ENTER_OAMSA_TRANSLATIONS();
	unsigned int noTx;
	unsigned int noTxfor;
	unsigned int checkCcb = 3;
	std::tr1::shared_ptr<TxContext> txContextIn;
	MafReturnT      retVal = MafOk;
	SaAisErrorT immErr = SA_AIS_OK;
	bool immResAbort = false;


	DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::OamSAPrepare...txhandle %lu", txHandle);

	txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->getTxContext(txHandle);
	if (NULL == txContextIn.get())
	{
		DEBUG_OAMSA_TRANSLATIONS("OamSAPrepare the context associated with handle %d was not found in the repository", (int)txHandle);
		LEAVE_OAMSA_TRANSLATIONS();
		return MafNotExist;
	}
	//Clear modified objects list from the temporary cache
	txContextIn->GetCache().clearFinalModifiedObjectsList();
	txContextIn->setIsCcbEmpty(false);
	/* TR HT17443 - only create a ccb if it is not created*/
	if (txContextIn->getImmCcbCreatedStatus() == ImmCcbNotCreated)
	{
		OamSAImmBridge *imm =  OamSAImmBridge::getOamSAImmBridge();
		immErr = imm->ccbInit(txContextIn.get());
		if(immErr != SA_AIS_OK){
			ERR_OAMSA_TRANSLATIONS("OamSAImmBridge::OamSAPrepare() Failed to init ccb,error(%s)",imm->strError(immErr));
			return MafFailure;
		}
		else
		{
			/* TR HT17443 - only create a ccb if it is not created*/
			txContextIn->setImmCcbCreatedStatus(ImmCcbCreated);
		}

		// CreatedObjects
		OamSACacheCreatedObjectsList *creObj = txContextIn->GetCache().GetListOfCreatedObjects();
		noTx = creObj->NoOfObjects();  // number of object to be created in Imm.
		if (noTx > 0) {
			txContextIn->GetCache().prepareListOfModifiedObjects(*creObj);
		}
		DEBUG_OAMSA_TRANSLATIONS("OamSAPrepare() Got list of created objects, number of objects = %d", noTx);
		/* HT28574 - check ccb is whether empty or not*/
		if(noTx == 0)
		{
			//ccb empty
			checkCcb --;
		}
		for(noTxfor = noTx; noTxfor > 0; noTxfor--)
		{
			std::string ParentName(saNameGet((*creObj)[noTxfor-1].ParentName()), saNameLen((*creObj)[noTxfor-1].ParentName()));
			DEBUG_OAMSA_TRANSLATIONS("Trying to create object with parent \"%s\" and class \"%s\" ", ParentName.c_str(),  (*creObj)[noTxfor-1].ClassName());
			if (!lock_mo_for_config_change){
				retVal = lockDn(txContextIn.get(), ParentName , SA_IMM_ONE);
				if(retVal != MafOk){
					delete  creObj;
					LEAVE_OAMSA_TRANSLATIONS();
					return retVal;
				}
			}
			DEBUG_OAMSA_TRANSLATIONS("OamPrepare call create with : %s // %s", ParentName.c_str(), (*creObj)[noTxfor-1].ClassName());
			immErr = imm->ccbObjectCreate(txContextIn.get(), (*creObj)[noTxfor-1].ParentName(),(*creObj)[noTxfor-1].ClassName(), (*creObj)[noTxfor-1].AttrValues());
			if(immErr == SA_AIS_ERR_FAILED_OPERATION){
				getAndForwardErrorStrings(txContextIn.get(),immResAbort, true);
				DEBUG_OAMSA_TRANSLATIONS("OamSAPrepare() CreateObject: failed to create DN(%s),className(%s),error(%s)",
						ParentName.c_str(),(*creObj)[noTxfor-1].ClassName() ,imm->strError(immErr));
				delete  creObj;
				LEAVE_OAMSA_TRANSLATIONS();
				if (immResAbort){
					return MafCommitFailed;
				}
				return MafFailure;
			}else if(immErr != SA_AIS_OK){
				getAndForwardErrorStrings(txContextIn.get(), immResAbort);
				ERR_OAMSA_TRANSLATIONS("OamSAPrepare() CreateObject: failed to create DN(%s),className(%s),error(%s)",
						ParentName.c_str(),(*creObj)[noTxfor-1].ClassName() ,imm->strError(immErr));
				delete  creObj;
				// imm->ccbFinalize(txContextIn.get());
				LEAVE_OAMSA_TRANSLATIONS();
				return MafFailure;
			}
		}
		delete  creObj;

		// DeletedObjects
		txContextIn->ccbDeleteFailed = false;
		OamSACacheDeletedObjectsList *delObj = txContextIn->GetCache().GetListOfDeletedObjects();
		noTx = delObj->NoOfObjects();  // number of object to be delete in Imm.
		DEBUG_OAMSA_TRANSLATIONS("Got list of deleted objects, number of objects = %d", noTx);
		/* HT28574 - check ccb is whether empty or not*/
		if(noTx == 0)
		{
			//ccb empty
			checkCcb --;
		}
		for(noTxfor = 0;noTxfor < noTx; noTxfor++)
		{
			std::string ObjectName(saNameGet((*delObj)[noTxfor].ObjectName()), saNameLen((*delObj)[noTxfor].ObjectName()));
			if (!lock_mo_for_config_change){
				retVal = lockDn(txContextIn.get(), ObjectName , SA_IMM_SUBTREE);
				if(retVal != MafOk){
					delete  delObj;
					LEAVE_OAMSA_TRANSLATIONS();
					return retVal;
				}
			}
			DEBUG_OAMSA_TRANSLATIONS("Calling delete for \"%s\" ", ObjectName.c_str());
			immErr = imm->ccbObjectDelete(txContextIn.get(), (*delObj)[noTxfor].ObjectName());

			if(immErr == SA_AIS_ERR_FAILED_OPERATION){
				getAndForwardErrorStrings(txContextIn.get(),immResAbort, true);
				txContextIn->ccbDeleteFailed = true;
				DEBUG_OAMSA_TRANSLATIONS("OamSAPrepare DeleteObject: failed to Delete DN(%s),error(%s)",
						ObjectName.c_str(),imm->strError(immErr));
				LEAVE_OAMSA_TRANSLATIONS();
				delete  delObj;
				if (immResAbort){
					return MafCommitFailed;
				}
				return MafFailure;
			}else if(immErr != SA_AIS_OK){
				getAndForwardErrorStrings(txContextIn.get(),immResAbort);
				txContextIn->ccbDeleteFailed = true;
				ERR_OAMSA_TRANSLATIONS("OamSAPrepare DeleteObject: failed to Delete DN(%s),error(%s)",
										ObjectName.c_str(),imm->strError(immErr));
				delete  delObj;
				LEAVE_OAMSA_TRANSLATIONS();
				return MafFailure;
			}
		}
		delete  delObj;

		// ModifiedObjects
		OamSACacheModifiedObjectsList modObj = txContextIn->GetCache().GetListOfModifiedObjects();
		noTx = modObj.NoOfObjects();  // number of object to be set in Imm.
		DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::OamSAPrepare : Number of Modified Objects %u", noTx);
		/* HT28574 - check ccb is whether empty or not*/
		if(noTx == 0)
		{
			//ccb empty
			checkCcb --;
		}
		for(noTxfor = 0;noTxfor < noTx; noTxfor++)
		{
			// Take ownership of the object if that's not already done
			std::string ObjName(saNameGet((modObj)[noTxfor].ObjectName()), saNameLen((modObj)[noTxfor].ObjectName()));
			DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::OamSAPrepare : modify object  %s", ObjName.c_str());
			if (!lock_mo_for_config_change){
				retVal = lockDn(txContextIn.get(), ObjName , SA_IMM_ONE);
				if(retVal != MafOk){
					LEAVE_OAMSA_TRANSLATIONS();
					return retVal;
				}
			}
			immErr = imm->ccbModify(txContextIn.get(), (modObj)[noTxfor].ObjectName(),(modObj)[noTxfor].AttrValues());
			bool immResAbort = false;
			if(immErr == SA_AIS_ERR_FAILED_OPERATION){
				DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::OamSAPrepare() ModifiedObjects: failed to Modify DN(%s),error(%s)",ObjName.c_str(),imm->strError(immErr));
				LEAVE_OAMSA_TRANSLATIONS();
				getAndForwardErrorStrings(txContextIn.get(),immResAbort, true);
				if (immResAbort){
					return MafCommitFailed;
				}
				return MafFailure;
			}else if(immErr != SA_AIS_OK){
				getAndForwardErrorStrings(txContextIn.get(),immResAbort);
				ERR_OAMSA_TRANSLATIONS("OamSAImmBridge::OamSAPrepare() ModifiedObjects: failed to Modify DN(%s),error(%s)",
							ObjName.c_str(),imm->strError(immErr));
				//imm->ccbFinalize(txContextIn.get());
				LEAVE_OAMSA_TRANSLATIONS();
				return MafFailure;
			}
		}
		//HT28574
		if(checkCcb == 0)
		{
			txContextIn->setIsCcbEmpty(true);
		}
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return retVal;
}

/**
 *  OamSACommit
 */
MafReturnT OamSAImmBridge::OamSACommit(MafOamSpiTransactionHandleT txHandle)
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::OamSACommit...txhandle %lu", txHandle);
	std::tr1::shared_ptr<TxContext> txContextIn;
	txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->getTxContext(txHandle);
	if (NULL == txContextIn.get())
	{
		DEBUG_OAMSA_TRANSLATIONS("OamSACommit the context associated with handle %d was not found in the repository", (int)txHandle);
		LEAVE_OAMSA_TRANSLATIONS();
		return MafNotExist;
	}

	/* TR HT17443 - only create a ccb if it is not created*/
	/* This existing ccb is now applied, no need to falsely trigger an abort on the next Validate() */
	/* No need to falsely avoid creating a ccb on the next Prepare () */
	SaAisErrorT immErr = SA_AIS_OK;
	txContextIn->setImmCcbCreatedStatus(ImmCcbNotCreated);

	OamSAImmBridge *imm =  OamSAImmBridge::getOamSAImmBridge();
	immErr = imm->ccbApply(txContextIn.get());
		int retCnt = 0;
		while ( immErr == SA_AIS_ERR_FAILED_OPERATION && retCnt < IMM_ABORT_RETRIES_COUNT ){
			//As MW aborts the CCB when SA_AIS_ERR_FAILED_OPERATION returns, no need to explicitly
			//invoke ccbAbort. But needs to handle ccbFinalize and release admin owner handle
			bool retVal = handleCcbOnFailedOperation(txHandle);
			DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::OamSACommit() txhandle= %lu retCnt= %d, Failed to apply CCB", txHandle,retCnt);
			if(!retVal)
				return MafCommitFailed;

			usleep(MICROS_PER_SEC);
			bool isValid ;
			MafReturnT ret = OamSAValidate(txHandle, &isValid);
			if(ret == MafOk && isValid == true){
				DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::OamSACommit() txhandle= %lu retCnt= %d, OamSAValidate() passed, error= %d ", txHandle, retCnt, ret);
				txContextIn->setImmCcbCreatedStatus(ImmCcbNotCreated);
				immErr = imm->ccbApply(txContextIn.get());
			}else{
				DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::OamSACommit() txhandle= %lu retCnt= %d, Failed to apply CCB, OamSAValidate() failed, error= %d ", txHandle, retCnt, ret);
				break;
			}
			++retCnt;
		}
	if(immErr != SA_AIS_OK){
		bool temp = false;
		getAndForwardErrorStrings(txContextIn.get(),temp);
		ERR_OAMSA_TRANSLATIONS("OamSAImmBridge::OamSACommit(), Failed to apply ccb,error(%s)",imm->strError(immErr));
		// Release admin owner for all dn if hold after ccb apply failure
		releaseAdminOwnerForDn(txHandle);
		LEAVE_OAMSA_TRANSLATIONS();
		return MafCommitFailed;
	}
	// Release admin owner for all dn if hold after ccb apply
	if (releaseAdminOwnerForDn(txHandle) == MafNotExist) {
		LEAVE_OAMSA_TRANSLATIONS();
		return MafNotExist;
	}
	DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::OamSACommit() txhandle = %lu", txHandle);
	LEAVE_OAMSA_TRANSLATIONS();
	return MafOk;
}

/**
 *  OamSAAbort
 */
MafReturnT OamSAImmBridge::OamSAAbort(MafOamSpiTransactionHandleT txHandle)
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::OamSAAbort...txhandle %lu", txHandle);
	LEAVE_OAMSA_TRANSLATIONS();
	return MafOk;
}

/**
 *  OamSAFinish
 */
MafReturnT OamSAImmBridge::OamSAFinish(MafOamSpiTransactionHandleT txHandle)
{
	ENTER_OAMSA_TRANSLATIONS();
	MafReturnT retVal = MafOk;
	SaAisErrorT immErr = SA_AIS_OK;
	std::tr1::shared_ptr<TxContext> txContextIn;
	OamSAImmBridge *imm =  OamSAImmBridge::getOamSAImmBridge();
	txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->getTxContext(txHandle);
	if (NULL == txContextIn.get())
	{
		DEBUG_OAMSA_TRANSLATIONS("OamSAFinish the context associated with handle %d was not found in the repository", (int)txHandle);
		LEAVE_OAMSA_TRANSLATIONS();
		return MafNotExist;
	}
	DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::OamSAFinish...txhandle %lu", txHandle);
	if(txContextIn->mCcbHandle != 0)
	{
		immErr = imm->ccbFinalize(txContextIn.get());
	}

	if(immErr != SA_AIS_OK)
	{
		ERR_OAMSA_TRANSLATIONS("OamSAImmBridge::OamSAFinish(), Failed to Finalize ccb,error(%s)",imm->strError(immErr));
		LEAVE_OAMSA_TRANSLATIONS();
		retVal = MafFailure;
	}

	/* clean up all allocated stuff in this tx and cache*/
	if(OamSATransactionRepository::getOamSATransactionRepository()->delTxContext(txHandle))
	{
		DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::OamSAFinish");
		retVal = MafOk;
	}
	else
	{
		ERR_OAMSA_TRANSLATIONS("OamSAImmBridge::OamSAFinish(), Internal error,Failed to erase transaction from map");
		retVal = MafFailure;
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return retVal;
}


/*
 * OamSAValidate
 * Perform the same activities as OamSAPrepare, followed by explicit ccb validate and ccb abort.
 */
#ifdef UNIT_TEST
//extern int CheckSupportedTxResourceSPIVersion_forUT(void);
extern int CheckSupportedTxResourceSPIVersion_forUT(void);
extern MafReturnT OamSAPrepareTest(MafOamSpiTransactionHandleT txHandle);
#endif

MafReturnT OamSAImmBridge::OamSAValidate(MafOamSpiTransactionHandleT txHandle, bool *result)
{
	ENTER_OAMSA_TRANSLATIONS();
	MafReturnT retVal = MafOk;
	SaAisErrorT immErr = SA_AIS_OK;
	std::tr1::shared_ptr<TxContext> txContextIn;

	DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::OamSAValidate...txhandle %lu", txHandle);

	//Assume validation result is true, only return false if IMM returns error from ccb validation.
	*result=true;

	OamSAImmBridge *imm =  OamSAImmBridge::getOamSAImmBridge();
	txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->getTxContext(txHandle);
	if (NULL == txContextIn.get())
	{
		DEBUG_OAMSA_TRANSLATIONS("OamSAValidate the context associated with handle %d was not found in the repository", (int)txHandle);
		LEAVE_OAMSA_TRANSLATIONS();
		return MafNotExist;
	}

	/* TR HT17443 - only create a ccb if it is not created */
	/* Must abort previous ccb before creating a new ccb. */
#ifndef UNIT_TEST
	if (txContextIn->getImmCcbCreatedStatus() == ImmCcbCreated)
#endif
	{
		txContextIn->setImmCcbCreatedStatus(ImmCcbNotCreated);
		immErr = imm->ccbAbort(txContextIn.get());
		if(immErr != SA_AIS_OK)
		{
			bool temp = false;
			getAndForwardErrorStrings(txContextIn.get(),temp);
			ERR_OAMSA_TRANSLATIONS("OamSAImmBridge::OamSAValidate(), Failed to abort ccb,error(%s)",imm->strError(immErr));

			/* Return code when an abort is done for a non-existent ccb should be ignored, continue with validate as normal */
			if(immErr != SA_AIS_ERR_BAD_HANDLE)
			{
				LEAVE_OAMSA_TRANSLATIONS();
				return MafFailure;
			}
		}
	}

#ifndef UNIT_TEST
	retVal = OamSAPrepare(txHandle);
#else
	retVal = OamSAPrepareTest(txHandle);
#endif


	if(retVal == MafCommitFailed){
		retVal = handlePrepareOnImmAbort(txHandle);
	}
	if(retVal != MafOk){
		DEBUG_OAMSA_TRANSLATIONS("OamSAValidate failed to prepare ccb, OamSAPrepare returns %d", (int)retVal);
		/* TR HS99358 - always abort CCB after getting ccb Handle */
		immErr = imm->ccbAbort(txContextIn.get());
		if(immErr != SA_AIS_OK)
		{
			bool temp = false;
			getAndForwardErrorStrings(txContextIn.get(), temp);
			ERR_OAMSA_TRANSLATIONS("OamSAImmBridge::OamSAValidate(), Failed to abort ccb,error(%s)",imm->strError(immErr));
		}
		// Release admin owner for all dn if hold after ccb abort
		releaseAdminOwnerForDn(txHandle);
		LEAVE_OAMSA_TRANSLATIONS();
		return retVal;
	}

	if(txContextIn->getIsCcbEmpty() == false){
		// Perform ccb Validate followed by ccb Abort.
		immErr = imm->ccbValidate(txContextIn.get());
		int count = 0;
		//Retry ccbValidation only incase of IMM resource abort
		while ( immErr == SA_AIS_ERR_FAILED_OPERATION && count < IMM_ABORT_RETRIES_COUNT ){
			bool ret = handleCcbOnFailedOperation(txHandle);
			DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::OamSAValidate() txhandle= %lu retCnt= %d, Failed to apply CCB", txHandle,count);
			if(!ret) {
				*result = false;
				LEAVE_OAMSA_TRANSLATIONS();
				return MafOk;
			}
			txContextIn->setImmCcbCreatedStatus(ImmCcbNotCreated);
			usleep(MICROS_PER_SEC);

			#ifndef UNIT_TEST
				retVal = OamSAPrepare(txHandle );
			#else
				retVal = OamSAPrepareTest(txHandle);
			#endif

			if(retVal == MafCommitFailed) {
				retVal = handlePrepareOnImmAbort(txHandle, false);
			}
			if(retVal == MafOk){
				DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::OamSAValidate() txhandle= %lu count= %d, OamSAPrepare() passed, error= %d ", txHandle, count, retVal);
				immErr = imm->ccbValidate(txContextIn.get());
			}else{
				DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::OamSAValidate() txhandle= %lu count= %d, Failed to apply CCB, OamSAPrepare() failed, error= %d ", txHandle, count, retVal);
				break;
			}
			++count;
		}
		if (retVal != MafOk){
			*result = false;
			bool temp = false;
			getAndForwardErrorStrings(txContextIn.get(), temp);
			DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::OamSAValidate(), Failed to prepare ccb, OamSAPrepare returns %d", (int)retVal);
			/* TR HS99358 - always abort CCB after getting ccb Handle */
			immErr = imm->ccbAbort(txContextIn.get());
			if(immErr != SA_AIS_OK)
			{
				bool temp = false;
				getAndForwardErrorStrings(txContextIn.get(), temp);
				ERR_OAMSA_TRANSLATIONS("OamSAImmBridge::OamSAValidate(), Failed to abort ccb,error(%s)",imm->strError(immErr));
			}
			// Release admin owner for all dn if hold after ccb abort
			releaseAdminOwnerForDn(txHandle);
			LEAVE_OAMSA_TRANSLATIONS();
			return retVal;

		}
		if(immErr != SA_AIS_OK)
		{
			*result = false;
			bool temp = false;
			getAndForwardErrorStrings(txContextIn.get(), temp);
			ERR_OAMSA_TRANSLATIONS("OamSAImmBridge::OamSAValidate(), Validation of ccb failed, error(%s)",imm->strError(immErr));
			releaseAdminOwnerForDn(txHandle);
		}
	}
	else
	{
		*result = true;
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return retVal;
}


MafReturnT OamSAImmBridge::releaseAdminOwnerForDn(MafOamSpiTransactionHandleT txHandle)
{
	ENTER_OAMSA_TRANSLATIONS();
	unsigned int noTx;
	unsigned int noTxfor;
	SaAisErrorT immErr;
	DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::releaseAdminOwnerForDn...txhandle %lu", txHandle);
	std::tr1::shared_ptr<TxContext> txContextIn;
	txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->getTxContext(txHandle);
	if (NULL == txContextIn.get())
	{
		DEBUG_OAMSA_TRANSLATIONS("Release the context associated with handle %d was not found in the repository", (int)txHandle);
		LEAVE_OAMSA_TRANSLATIONS();
		return MafNotExist;
	}

	// CreatedObjects
	OamSACacheCreatedObjectsList *creObj = txContextIn->GetCache().GetListOfCreatedObjects();
	noTx = creObj->NoOfObjects();  // number of objects to be Released.
	for(noTxfor = noTx; noTxfor > 0; noTxfor--)
	{
		std::string ParentName(saNameGet((*creObj)[noTxfor-1].ParentName()), saNameLen((*creObj)[noTxfor-1].ParentName()));
		immErr = releaseAdminOwner(txContextIn.get(), ParentName, SA_IMM_ONE);
		if(immErr != SA_AIS_OK)
		{
			DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::releaseAdminOwnerForDn(): created object: releaseAdminOwner failed with err code: %d dn: %s", immErr, ParentName.c_str());
		}
	}
	delete creObj;

	// DeletedObjects
	// Remove admin owner in COMSA resource only if the dn is already deleted.
	// We have to only clear the internal cache  if the object is already deleted, if not saImmOmAdminOwnerRelease should be called to delete it at IMM level
	OamSACacheDeletedObjectsList *delObj = txContextIn->GetCache().GetListOfDeletedObjects();
	noTx = delObj->NoOfObjects();  // number of object to be delete in Imm.
	for(noTxfor = 0;noTxfor < noTx; noTxfor++)
	{
		std::string ObjectName(saNameGet((*delObj)[noTxfor].ObjectName()), saNameLen((*delObj)[noTxfor].ObjectName()));
		if (!ObjectName.empty())
		{
			if(txContextIn->ccbDeleteFailed)
			{
				DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::releaseAdminOwnerForDn(): releasing admin owner");
				immErr = releaseAdminOwner(txContextIn.get(), ObjectName, SA_IMM_SUBTREE);
				if(immErr != SA_AIS_OK)
				{
					ERR_OAMSA_TRANSLATIONS("OamSAImmBridge::releaseAdminOwnerForDn(): deleted object: releaseAdminOwner failed with err code: %d dn: %s", immErr, ObjectName.c_str());
				}
			}
			else
			{
				if (txContextIn->isAdminOwnerFor(ObjectName,SA_IMM_SUBTREE))
				{

					DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::releaseAdminOwnerForDn(): release admin owner in cache");
					txContextIn->removeAdminOwnerFor(ObjectName,SA_IMM_SUBTREE);
				}
			}
		}
	}
	delete delObj;


	// ModifiedObjects
	OamSACacheModifiedObjectsList modObj = txContextIn->GetCache().getFinalModifiedObjectsList();
	noTx = modObj.NoOfObjects();  // number of object to be set in Imm.
	for(noTxfor = 0;noTxfor < noTx; noTxfor++)
	{
		// Take ownership of the object if that's not already done
		std::string ObjName(saNameGet((modObj)[noTxfor].ObjectName()), saNameLen((modObj)[noTxfor].ObjectName()));
		immErr = releaseAdminOwner(txContextIn.get(), ObjName, SA_IMM_ONE);
		if(immErr != SA_AIS_OK)
		{
			DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::ReleaseAdminOwnerForDn(): modified object: releaseAdminOwner failed with err code: %d dn: %s", immErr, ObjName.c_str());
		}
	}
	txContextIn->GetCache().clearFinalModifiedObjectsList();
	LEAVE_OAMSA_TRANSLATIONS();
	return MafOk;
}


/**
 * getImmAttribute
 */
SaAisErrorT OamSAImmBridge::getImmAttribute(TxContext *txContextIn,
											const char *dn, const char *attributeName, SaImmAttrValuesT_2 ***AttrValue)
{
	ENTER_OAMSA_TRANSLATIONS();
	ImmCmdOmAccessorGet immCmdOmAccessorGet(txContextIn, dn, attributeName, AttrValue);
	SaAisErrorT Errt = immCmdOmAccessorGet.execute();
	LEAVE_OAMSA_TRANSLATIONS();
	return Errt;
}
/*
 *  removeEqualAndDot
 *
 *  Removes everything after '=' and '.' in the end of a string
 *
 *
 * Input : A 3GPP DN string
 *
 * Output : 3GPP string without any value and key attribute part.
 */
std::string OamSAImmBridge::removeEqualAndDot(std::string DnFragment)
{
	ENTER_OAMSA_TRANSLATIONS();
	try {
		// Remove everything after . and =
		if (DnFragment.find_first_of('=') != std::string::npos)
		{
			DnFragment = DnFragment.substr(0,DnFragment.find_first_of('='));
		}
		if (DnFragment.find_first_of('.') != std::string::npos)
		{
			DnFragment = DnFragment.substr(0,DnFragment.find_first_of('.'));
		}
	}catch (...) {
		// Only to not core dump
		DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::removeEqualAndDot failed to clean string");
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return DnFragment;
	}
/*
 *  retrieveLastElementFromDn
 *
 *  This method retrieves the last element and the second last element (if it exist) and returns these two
 *  as parentClass and className
 *
 *
 * Input : A 3GPP DN string
 *
 * Output the same string object but trimmed to contain the last class name in the DN instance chain.
 */
void OamSAImmBridge::retrieveLastElementFromDn(OamSACache::DNList theDNList,
												std::string& className,
												std::string& parentClassName,
												bool onlyClassName)
{
	ENTER_OAMSA_TRANSLATIONS();
	OamSACache::DNListReverseIterator theIter;
	OamSACache::DNListReverseIterator theIterOneBefore;
	// Check so that we actually get a value when called
	theIter = theDNList.rbegin();
	if(!(theIter == theDNList.rend()))
	{
	if (onlyClassName)
	{
		className = removeEqualAndDot(*theIter);
		parentClassName.clear();
	}
	else
	{
		theIterOneBefore = theDNList.rbegin();
		++theIterOneBefore;
		if (theIterOneBefore != theDNList.rend())
		{
			className = removeEqualAndDot(*theIter);
			parentClassName = removeEqualAndDot(*theIterOneBefore);
		}
		else
		{
			// Must be ComTop=1
			className = removeEqualAndDot(*theIter);
			parentClassName.clear();
		}
	}
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return ;
}


/*
 *  TranslateClassName
 *
 *  To keep IMM class names unique, the class names should be prefixed with the name of the MOM where the class
 *  is defined. An exception is made if the unaltered class name is found in IMM, when it is assumed that it is
 *  an existing class which for backwards compatibility should be left as is.
 *
 *  THis method needs to check if the class belongs to a MOM that is to be translated.
 *
 */
std::string OamSAImmBridge::translateClassName(TxContext *txContext, const std::string& parentName, const std::string& className)
{
	ENTER_OAMSA_TRANSLATIONS();
	// First search for the root class in IMM for this class.
	// Check what MOM we are a part of and then ...
	DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::translateClassName : Called with %s // %s ", parentName.c_str(), className.c_str());
	OamSACache::DNList splitName;
	// In the analyze below we need to start to search from DNList + ClassName, so that we do not miss
	// the case where the class name actually is a root itself. So we add the class name to the DNList created
	if (parentName != "") {
		std::string classNameDN = "," + className + "=1";
		std::string extendedDn = parentName + classNameDN;
		DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::translateClassName : Calls SplitDN with %s", extendedDn.c_str());
		txContext->GetCache().SplitDN(extendedDn, splitName);
	}else {
		// OK, we have the top here, it is not decorated, only return the original class name
		DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::translateClassName : transformed name %s", className.c_str());
		LEAVE_OAMSA_TRANSLATIONS();
		return className;
	}
	OamSACache::DNListReverseIterator theIter = splitName.rbegin();
	DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::translateClassName : calls with DNList");
	for (theIter = splitName.rbegin(); theIter != splitName.rend(); ++theIter) {
		DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::translateClassName : %s", (*theIter).c_str());
	}

	LEAVE_OAMSA_TRANSLATIONS();
	return theTranslator.TransformImmClassName(splitName, className);
}


/*
 *  TranslateStructName
 *
 *  To keep IMM struct names unique, the struct should have the
 *
 */
std::string OamSAImmBridge::translateStructName(TxContext *txContext, const std::string& ObjectName, const std::string& structAttrName)
{
	ENTER_OAMSA_TRANSLATIONS();
	OamSACache::DNList splitObjectName;
	txContext->GetCache().SplitDN(ObjectName, splitObjectName);
	DEBUG_OAMSA_TRANSLATIONS("Entering OamSAImmBridge::translateStructName class for (%s) struct attr(%s)",ObjectName.c_str(),structAttrName.c_str());
	// Get the class name for the class containing the struct.
	MafOamSpiMocT* theMoc = theTranslator.GetComMoc(splitObjectName);
	if (theMoc != NULL)
	{
		DEBUG_OAMSA_TRANSLATIONS("Entering OamSAImmBridge::translateStructName class for (%s) is (%s)",ObjectName.c_str(),theMoc->generalProperties.name);
		SaImmClassCategoryT		classCategory;
		SaImmAttrDefinitionT_2 **	attrDef = NULL;
		ImmCmdOmClassDescriptionGet  immCmdOmClassDescriptionGet(txContext, theMoc->generalProperties.name, &classCategory, &attrDef);
		SaAisErrorT Errt = immCmdOmClassDescriptionGet.execute();
		if (SA_AIS_OK == Errt)
		{
		DEBUG_OAMSA_TRANSLATIONS("Entering OamSAImmBridge::translateStructName class already existing in IMM");
		// Class in existance, no name transformation. release info
		Errt = saImmOmClassDescriptionMemoryFree_2(txContext->mImmHandle, attrDef);
		LEAVE_OAMSA_TRANSLATIONS();
		return structAttrName;
		}
		// If not then get the MOM of the containing class and prepend it to the class name (structAttrName)

		std::string thePrefixMom("");
		MafOamSpiMomT* theMom = theTranslator.GetComMom(splitObjectName);
		if (theMom != NULL)
		{
		DEBUG_OAMSA_TRANSLATIONS("Entering OamSAImmBridge::translateStructName MOM for (%s) is (%s)",ObjectName.c_str(),theMom->generalProperties.name);
			thePrefixMom = theMom->generalProperties.name;
		}
		LEAVE_OAMSA_TRANSLATIONS();
		return thePrefixMom + structAttrName;

	}
	else
	{
		ERR_OAMSA_TRANSLATIONS("OamSAImmBridge::translateStructName, failed to get class for (%s)",ObjectName.c_str());
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return structAttrName;
}

/**
 * set portal for MafMgmtSpiInterfacePortal_3T
 */
MafReturnT maf_comSAMgmtSpiThreadContInit(MafMgmtSpiInterfacePortal_3T* portal)
{
	ENTER_OAMSA_TRANSLATIONS();
	MafReturnT com_rc = MafOk;
	_portal = portal;
	if (_portal == NULL) {
		ERR_OAMSA_TRANSLATIONS("No portal");
		com_rc = MafFailure;
	}
	else
	{
		DEBUG_OAMSA_TRANSLATIONS("comSAMgmtSpiThreadContInit _portal = %p", _portal);
	}

	LEAVE_OAMSA_TRANSLATIONS();
	return com_rc;
}

char *regexp (const char *string, const char *patrn, int* len) {
	char *word = NULL;
	regex_t rgT;
	regmatch_t match;
	regcomp(&rgT,patrn,REG_EXTENDED);
	if ((regexec(&rgT,string,1,&match,0)) == 0) {
		int w=0;
		int begin = (int)match.rm_so;
		int end = (int)match.rm_eo;
		*len = end-begin;
		word = new char[*len+1];
		for (int i=begin; i<end; i++) {
			word[w] = string[i];
			w++; }
		word[w]=0;
	}
	regfree(&rgT);
	return word;
}

const char*  OamSAImmBridge::parseAndCategorizeErrorString(const char* errorString, ThreadContextMsgCategory_2 &category)
{
	ENTER_OAMSA_TRANSLATIONS();
	int len=0;
	char *match=regexp(errorString,"@[^@]*@",&len);
	if(match!=NULL) //a control sequence exists in the string
	{
		for(int i=0; i < len; ++i)
			match[i] = std::tolower(match[i]); //make all characters lowercase

		if(strcmp(match,"@comlog@") == 0)
			category=ThreadContextMsgLog_2;
		else if(strcmp(match,"@comnbi@") == 0)
			category=ThreadContextMsgNbi_2;
		else category=ThreadContextMsgLog_2; //any other control sequence

		delete[] match;
		LEAVE_OAMSA_TRANSLATIONS();
		return errorString+len; //in order to have the part without the control sequence
	}
	else {
		category=ThreadContextMsgLog_2;
		delete[] match;
		LEAVE_OAMSA_TRANSLATIONS();
		return errorString;
	}
}

MafReturnT OamSAImmBridge::getAndForwardErrorStrings(TxContext *txContextIn , bool& immResourceAbort, bool checkImmResAbort)
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::getAndForwardErrorStrings()");
	MafReturnT com_rc = _portal->getInterface(thContext, (MafMgmtSpiInterface_1T**)&threadContext);
	if(MafOk != com_rc)
	{
		ERR_OAMSA_TRANSLATIONS("Failed to get MafMgmtSpiThreadContext_2");
		return com_rc;
	}
	else
	{
		DEBUG_OAMSA_TRANSLATIONS("_portal->getInterface() return OK");
	}
	DEBUG_OAMSA_TRANSLATIONS("CCB Handle: %llu", txContextIn->mCcbHandle);
	const SaStringT* errorStrings;
	SaAisErrorT errorCode = saImmOmCcbGetErrorStrings(txContextIn->mCcbHandle, &errorStrings);
	//TODO: Error codes that are returned by this function are not defined yet according to the OpenSAF ticket.
	if ((SA_AIS_OK == errorCode) && (0 != errorStrings))
	{
		LOG_OAMSA_TRANSLATIONS("Looping over the available error strings..");
		for(int i=0;i<ImmAdmOpNoReturnParams && errorStrings[i]!=(char*)'\0';i++)
		{
			const char *errorString=errorStrings[i];
			LOG_OAMSA_TRANSLATIONS("Error string number %d: %s",i,errorString);
			ThreadContextMsgCategory_2 category;
			errorString=parseAndCategorizeErrorString(errorString, category);
			if(checkImmResAbort){
				std::string tmpStr = errorString;
				if(tmpStr.find("IMM: Resource abort")!= std::string::npos){
					immResourceAbort = true;
					DEBUG_OAMSA_TRANSLATIONS("IMM resource abort returned");
					return MafOk;
				}
			}
			com_rc=threadContext->addMessage(category,errorString);
			DEBUG_OAMSA_TRANSLATIONS("Return code set by ThreadContext->addMessage(): %d",com_rc);
			if(MafOk != com_rc)
			{
				ERR_OAMSA_TRANSLATIONS("Error occurred during ThreadContext->addMessage()");
			}
		}
		DEBUG_OAMSA_TRANSLATIONS("All error strings were forwarded to COM");
		LEAVE_OAMSA_TRANSLATIONS();
		return MafOk;
	}
	else if(SA_AIS_OK != errorCode)
	{
		ERR_OAMSA_TRANSLATIONS("saImmOmCcbGetErrorStrings() function returned an error = %d", errorCode);
		ERR_OAMSA_TRANSLATIONS("Error during forwarding error string to COM");
		LEAVE_OAMSA_TRANSLATIONS();
		return MafFailure;
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return MafFailure;
}


/* SDP875: this function is used for get and forward error string invoke by admin operation */
MafReturnT OamSAImmBridge::getAndForwardErrorStrings_2(TxContext *txContextIn, SaImmAdminOperationParamsT_2 **returnParams)
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::getAndForwardErrorStrings_2()");
	MafReturnT com_rc = _portal->getInterface(thContext, (MafMgmtSpiInterface_1T**)&threadContext);
	if(MafOk != com_rc)
	{
		ERR_OAMSA_TRANSLATIONS("Failed to get MafMgmtSpiThreadContext_2");
		return com_rc;
	}
	else
	{
		DEBUG_OAMSA_TRANSLATIONS("_portal->getInterface() return OK");
	}

	LOG_OAMSA_TRANSLATIONS("Looping over the available error strings..");
	if (returnParams != NULL)
	{
		int i = 0;
		while (returnParams[i] != NULL)
		{
			if (SA_IMM_ATTR_SASTRINGT == returnParams[i]->paramType)
			{
				const char *errorString = *((char **) returnParams[i]->paramBuffer);
				LOG_OAMSA_TRANSLATIONS("Error string number %d: %s", i, errorString);
				ThreadContextMsgCategory_2 category;
				errorString = parseAndCategorizeErrorString(errorString, category);
				LOG_OAMSA_TRANSLATIONS("Error string after parse: %s", errorString);
				com_rc = threadContext->addMessage(category, errorString);
				DEBUG_OAMSA_TRANSLATIONS("Return code set by ThreadContext->addMessage(): %d", com_rc);
				if(com_rc != MafOk)
				{
					ERR_OAMSA_TRANSLATIONS("Error occurred during ThreadContext->addMessage()");
				}
			}
			i++;
		}
	}

	DEBUG_OAMSA_TRANSLATIONS("All error strings were forwarded to COM");
	LEAVE_OAMSA_TRANSLATIONS();
	return MafOk;
}


/**
 * isAdminOwnerLockedByAnotherTx
 */
bool OamSAImmBridge::isAdminOwnerLockedByAnotherTx(TxContext *txContext, SaAisErrorT err)
{
	ENTER_OAMSA_TRANSLATIONS();
	// put this here, there are no good places to put it anyway
	LEAVE_OAMSA_TRANSLATIONS();
	return (err == SA_AIS_ERR_EXIST &&
					(txContext->numInstances != 1 || txContext->isCm_if));
}

/**
 * checkSetAdminOwner
 */

/**
 *  This is for caching the fdns that we have set admin owner on
 *  to avoid setting admin owner multiple times(which is allowed).
 *  The ownership will be released automatically at adminOwnerFinalize
 *  Normally the scope is at the actual object, but it can also be subtree.
 *  Subtree is only used for delete, so we don't have to remember anything
 *  about dns under dn.
 */
SaAisErrorT OamSAImmBridge::checkSetAdminOwner(TxContext *txContext, std::string &dn, SaImmScopeT scope)
{
	ENTER_OAMSA_TRANSLATIONS();
	unsigned int nTries = 0;
	SaAisErrorT err = SA_AIS_OK;
	if(dn.length() > 0){
		if(!txContext->isAdminOwnerFor(dn,scope)){
			// new dn, set admin owner
			std::vector<std::string> newOwners;
			newOwners.push_back(dn);
			ImmCmdOmAdminOwnerSet immCmdOmAdminOwnerSet(txContext,&newOwners,scope);
			err = immCmdOmAdminOwnerSet.execute();
			if (err == SA_AIS_ERR_EXIST){
				if(isAdminOwnerLockedByAnotherTx(txContext, err)) {
					WARN_OAMSA_TRANSLATIONS("OamSAImmBridge::checkSetAdminOwner, another transaction is locking adminOwner for (%s)", dn.c_str());
				}
				else{
					// Only one tx alive, so we know that we should be the only admin owner.
					// handle error the situation where the adminowner was not cleared at a prevoius
					// tx because of some problem in imm (e.g. readonly at sync) or something else.
					WARN_OAMSA_TRANSLATIONS("OamSAImmBridge::checkSetAdminOwner, admin owner already set for (%s)", dn.c_str());
					// Adminowner already set, check if its by a previous tx that has already terminated
					// if so, clear it.
					std::string adminOwner;
					SaAisErrorT err2 = getAdminOwner(txContext, dn, adminOwner);
					if(err2 != SA_AIS_OK){
						ERR_OAMSA_TRANSLATIONS("OamSAImmBridge::checkSetAdminOwner, failed to get admin owner for (%s)",dn.c_str());
					}
					else{
						// is it set by CM
						if(adminOwner.length() > 0 && strncmp(adminOwner.c_str(),
																OamSATransactionRepository::getOamSATransactionRepository()->GetOwnerName()
																, strlen(OamSATransactionRepository::getOamSATransactionRepository()->GetOwnerName())) == 0){

							// TODO: This behaviour is not thread friendly. It may also unlock when reading from CM_IF.
							// but ok for now, proceed to clear admin owner
							WARN_OAMSA_TRANSLATIONS("OamSAImmBridge::checkSetAdminOwner, clearing admin owner(%s) for (%s)", adminOwner.c_str(), dn.c_str());
							ImmCmdOmAdminOwnerClear immCmdOmAdminOwnerClear(txContext, dn, scope);
							err = immCmdOmAdminOwnerClear.execute();
							if(err != SA_AIS_OK){
								ERR_OAMSA_TRANSLATIONS("OamSAImmBridge::checkSetAdminOwner, failed to clear admin owner(%s) for (%s)", adminOwner.c_str(),dn.c_str());
							}
							else{
								err = immCmdOmAdminOwnerSet.execute();
							}
						}
						else{
							ERR_OAMSA_TRANSLATIONS("OamSAImmBridge::checkSetAdminOwner, cannot clear admin owner(%s), %s was never owner of (%s)",
							adminOwner.c_str(),OamSATransactionRepository::getOamSATransactionRepository()->GetOwnerName(),dn.c_str());
							err = immCmdOmAdminOwnerSet.execute();
							if (err == SA_AIS_OK) {
								LOG_OAMSA_TRANSLATIONS("OamSAImmBridge::checkSetAdminOwner, immCmdOmAdminOwnerSet succeeded for (%s) after %d tries", dn.c_str(), nTries);
							} else {
								ERR_OAMSA_TRANSLATIONS("OamSAImmBridge::checkSetAdminOwner, immCmdOmAdminOwnerSet failed for (%s) with error code %i after  %d tries", dn.c_str(), err, nTries);
							}
						}
					}
				}
			}
			if(err == SA_AIS_OK){
				if(txContext->addAdminOwnerFor(dn,scope)) {
					DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridget::checkSetAdminOwner, added admin owner for %s",dn.c_str());
				}
			}
		}
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return err;
}

SaAisErrorT OamSAImmBridge::getAdminOwner(TxContext *txContext, std::string &dnIn, std::string &adminOwner)
{
	ENTER_OAMSA_TRANSLATIONS();
	//std::string attrName;
	SaImmAttrValuesT_2 **attrValsOut = 0;
	SaAisErrorT err = getImmAttribute(txContext, dnIn.c_str(), SA_IMM_ATTR_ADMIN_OWNER_NAME, &attrValsOut/*out*/);
	if(err != SA_AIS_OK)
	{
		LEAVE_OAMSA_TRANSLATIONS();
		return err;
	}
	if(attrValsOut == 0 || attrValsOut[0] == 0 || attrValsOut[0]->attrValuesNumber == 0)
	{
		LEAVE_OAMSA_TRANSLATIONS();
		return SA_AIS_ERR_NOT_EXIST;
	}
	if(attrValsOut[0]->attrValueType != SA_IMM_ATTR_SASTRINGT)
	{
		LEAVE_OAMSA_TRANSLATIONS();
		return SA_AIS_ERR_NOT_SUPPORTED;
	}
	adminOwner = *(char**)attrValsOut[0]->attrValues[0];
	LEAVE_OAMSA_TRANSLATIONS();
	return SA_AIS_OK;
}
/*
 *  Release ownership of a single object or a tree of objects
 */

SaAisErrorT OamSAImmBridge::releaseAdminOwner(TxContext *txContext, std::string &dn, SaImmScopeT scope)
{
	ENTER_OAMSA_TRANSLATIONS();
	SaAisErrorT err = SA_AIS_OK;

	// Sanity check(s)
	if (!dn.empty())
	{
		if (txContext->isAdminOwnerFor(dn,scope))
		{
			std::vector<std::string> Owners;
			Owners.push_back(dn);
			ImmCmdOmAdminOwnerRelease immCmdOmAdminOwnerRelease(txContext, &Owners, scope);
			err = immCmdOmAdminOwnerRelease.execute();
			if (err == SA_AIS_OK)
			{
				txContext->removeAdminOwnerFor(dn,scope);
			}
		}
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return err;
}

/**
 * ccbApply
 */
SaAisErrorT OamSAImmBridge::ccbApply(TxContext* txContext)
{
	ENTER_IMM_OM();
	ImmCmdOmCcbApply immCmdOmCcbApply(txContext);
	LEAVE_IMM_OM();
	return immCmdOmCcbApply.execute();
}

/**
 * ccbValidate
 */
SaAisErrorT OamSAImmBridge::ccbValidate(TxContext* txContext)
{
	ENTER_IMM_OM();
	ImmCmdOmCcbValidate immCmdOmCcbValidate(txContext);
	LEAVE_IMM_OM();
	return immCmdOmCcbValidate.execute();
}

/**
 * ccbAbort
 */
SaAisErrorT OamSAImmBridge::ccbAbort(TxContext* txContext)
{
	ENTER_IMM_OM();
	ImmCmdOmCcbAbort immCmdOmCcbAbort(txContext);
	LEAVE_IMM_OM();
	return immCmdOmCcbAbort.execute();
}


/**
 * ccbInit
 */
SaAisErrorT OamSAImmBridge::ccbInit(TxContext* txContext){
	ENTER_IMM_OM();
	SaAisErrorT err;
	ImmCmdOmCcbInit immCmdOmCcbInit(txContext);
	err = immCmdOmCcbInit.execute();
	LEAVE_IMM_OM();
	return err;
}
/**
 * ccbFinalize
 */
SaAisErrorT OamSAImmBridge::ccbFinalize(TxContext* txContext){
	ENTER_IMM_OM();
	SaAisErrorT err;
	ImmCmdOmCcbFinalize immCmdOmCcbFinalize(txContext);
	err = immCmdOmCcbFinalize.execute();
	LEAVE_IMM_OM();
	return err;
}

/**
 * Create ImmData
 */
SaAisErrorT OamSAImmBridge::ccbObjectCreate(TxContext *txContext, SaNameT *parent, SaImmClassNameT className, SaImmAttrValuesT_2 **attrValsIn)
{
	ENTER_IMM_OM();
	SaAisErrorT err;
	ImmCmdOmCcbObjectCreate immCmdOmCcbObjectCreate(txContext, parent,className,attrValsIn);
	err = immCmdOmCcbObjectCreate.execute();
	LEAVE_IMM_OM();
	return err;
}

/**
 * Delete ImmData
 */
SaAisErrorT OamSAImmBridge::ccbObjectDelete(TxContext *txContext, SaNameT *dnIn)
{
	ENTER_IMM_OM();
	SaAisErrorT err;
	ImmCmdOmCcbObjectDelete immCmdOmCcbObjectDelete(txContext, dnIn);
	err = immCmdOmCcbObjectDelete.execute();
	LEAVE_IMM_OM();
	return err;
}

/**
 * Set ImmData
 */
SaAisErrorT OamSAImmBridge::ccbModify(TxContext *txContext, SaNameT *dn, SaImmAttrModificationT_2 **attrMods)
{
	ENTER_IMM_OM();
	SaAisErrorT err;
	ImmCmdOmCcbObjectModify immCmdOmCcbObjectModify(txContext, dn, attrMods);
	err = immCmdOmCcbObjectModify.execute();
	LEAVE_IMM_OM();
	return err;
}


void ClearOamContextTransactionMap()
{
	ENTER_OAMSA_TRANSLATIONS();
	OamSATransactionRepository::getOamSATransactionRepository()->ClearContextMap();
	LEAVE_OAMSA_TRANSLATIONS();
}

/**
 * invalidModelDetected
 */
void OamSAImmBridge::invalidModelDetected(const std::string &strFunction, const std::string &dn, const std::string &className)
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("%s dn=%s className=%s", strFunction.c_str(), dn.c_str(), className.c_str());
	char errorText[ERR_STR_BUF_SIZE];
	snprintf (errorText, ERR_STR_BUF_SIZE, "Invalid model detected for class %s ", className.c_str());
	ERR_OAMSA_TRANSLATIONS("%s - %s", strFunction.c_str(), errorText);
	MafReturnT com_rc = _portal->getInterface(thContext, (MafMgmtSpiInterface_1T**)&threadContext);
	if (com_rc != MafOk)
		ERR_OAMSA_TRANSLATIONS("OamSAImmBridge::invalidModelDetected _portal->getInterface failed=%d", com_rc);
	com_rc = threadContext->addMessage(ThreadContextMsgNbi_2, errorText);
	if (com_rc != MafOk)
		ERR_OAMSA_TRANSLATIONS("OamSAImmBridge::invalidModelDetected addMessage(ThreadContextMsgNbi_2) failed=%d", com_rc);
	com_rc = threadContext->addMessage(ThreadContextMsgLog_2, errorText);
	if (com_rc != MafOk)
			ERR_OAMSA_TRANSLATIONS("OamSAImmBridge::invalidModelDetected addMessage(ThreadContextMsgLog_2) com_rc=%d", com_rc);
	LEAVE_OAMSA_TRANSLATIONS();
}

bool OamSAImmBridge::handleCcbOnFailedOperation(MafOamSpiTransactionHandleT txHandle)
{
	std::tr1::shared_ptr<TxContext> txContextIn;
	txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->getTxContext(txHandle);
	OamSAImmBridge *imm =  OamSAImmBridge::getOamSAImmBridge();
	bool immResAbort = false;
	getAndForwardErrorStrings(txContextIn.get(),immResAbort, true);
	if(!immResAbort){
		ERR_OAMSA_TRANSLATIONS("OamSAImmBridge::handleCcbOnFailedOperation, Failed to apply ccb,error SA_AIS_ERR_FAILED_OPERATION");
		// Release admin owner for all dn if hold after ccb apply failure
		releaseAdminOwnerForDn(txHandle);
		LEAVE_OAMSA_TRANSLATIONS();
		return false;
	}
	imm->ccbFinalize(txContextIn.get());
	releaseAdminOwnerForDn(txHandle);
	return true;
}

MafReturnT OamSAImmBridge::handlePrepareOnImmAbort(MafOamSpiTransactionHandleT txHandle, bool fwdErrStrings){
	MafReturnT retVal = MafCommitFailed;
	int retCnt = 0;
	std::tr1::shared_ptr<TxContext> txContextIn;
	OamSAImmBridge *imm =  OamSAImmBridge::getOamSAImmBridge();
	txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->getTxContext(txHandle);
	if (NULL == txContextIn.get())
	{
		DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::handlePrepareOnImmAbort() the context associated with handle %d was not found in the repository", (int)txHandle);
		LEAVE_OAMSA_TRANSLATIONS();
		return MafNotExist;
	}
	while (retVal == MafCommitFailed && retCnt < IMM_ABORT_RETRIES_COUNT){
		//As MW aborts the CCB when SA_AIS_ERR_FAILED_OPERATION returns, no need to explicitly
		//invoke ccbAbort but handle ccbFinalize/relase adminowner handles
		imm->ccbFinalize(txContextIn.get());
		releaseAdminOwnerForDn(txHandle);
		txContextIn->setImmCcbCreatedStatus(ImmCcbNotCreated);
		DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::handlePrepareOnImmAbort() txhandle= %lu retCnt= %d, CCB Prepare Failed", txHandle, retCnt);
		// re-init the whole CCB and try again until the IMM sync is finished
		usleep(MICROS_PER_SEC);

		#ifndef UNIT_TEST
			retVal = OamSAPrepare(txHandle);
		#else
			retVal = OamSAPrepareTest(txHandle);
		#endif

		if(retVal == MafOk){
			DEBUG_OAMSA_TRANSLATIONS("OamSAImmBridge::handlePrepareOnImmAbort() txhandle= %lu retCnt= %d, OamSAPrepare() passed, error= %d ", txHandle, retCnt, retVal);
		}
		++retCnt;
	}

	if(retVal == MafCommitFailed){
		// If Imm sync is not yet finished after defined timeout and retries
		if(fwdErrStrings) {
			bool temp = false;
			getAndForwardErrorStrings(txContextIn.get(), temp);
		}
		retVal = MafFailure;
	}
	return retVal;
}
