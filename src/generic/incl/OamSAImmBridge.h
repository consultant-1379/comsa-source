#ifndef __OAMSA_IMMBRIDGE_H
#define __OAMSA_IMMBRIDGE_H
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
 *   File:   OamSAImmBridge.h
 *
 *   Author: efaiami & egorped
 *
 *   Date:   2010-05-21
 *
 *   This file declares functions for communcation with IMM.
 *
 *   Reviewed: efaiami 2010-06-17
 *
 *   Reviewed: efaiami 2011-01-25   code reviewed for CM Action.
 *
 *   Modify: efaiami 2011-02-22  for log and trace function
 *   Modify: eozasaf 2011-07-08  for adding getAndForwardErrorStrings function
 *   Modify: efaiami 2011-07-18  add createMo(), deleteMO(), setMOAttribute() for CCB operation
 *   Modify: efaiami 2011-09-13  added createImmMO(), deleteImmMO(), setImmMoAttr() for CCB operation
 *   Modify: efaiami 2011-11-24  remove  createImmMO(), deleteImmMO(), setImmMoAttr()
 *   Modify: xnikvap, xngangu, xjonbuc, xduncao 2012-08-30  support for COM MO SPI Ver.3 (Ver.1 is not supported any more)
 *   Modify: xduncao 2013-01-20  support for SDP875
 *   Modify: xadaleg 2013-08-23  MR26712 - Support option to not split IMM DN at EcimContribution
 *   Modify: uabjoy  2013-08-26 Correcting trouble report HR51069
 *   Modify: xjonbuc 2014-06-23  Implement MR-20275 support for explicit ccb validate() & abort()
 *   Modify: xnikvap 2015-08-02  MR42277 Remove old interfaces exposed to COM & CoreMW that are no longer supported for SLES12
 *
 ******************************************************************************************************/

#include <string>
#include <list>
#include "saImm.h"
#include "ImmCmd.h"
#include "TxContext.h"
#include "trace.h"
#include "MafMgmtSpiThreadContext_2.h"
#include "OamSAManagedObjects.h"

using namespace CM;
// These defines how a struct is handled in IMM
#define STRUCT_CLASS_INDEX_SEPARATOR "_"
#define STRUCT_CLASS_KEY_ATTRIBUTE   "id"
#define MICROS_PER_SEC 1000000
class BridgeImmIterator
{
public:
	BridgeImmIterator(): myImmSearchSet(false) { ENTER_OAMSA_TRANSLATIONS(); LEAVE_OAMSA_TRANSLATIONS();};
	~BridgeImmIterator() {ENTER_OAMSA_TRANSLATIONS(); LEAVE_OAMSA_TRANSLATIONS();};
	BridgeImmIterator(const char *rdn, const char *rootClass);
	BridgeImmIterator(const char *rdn, const char *rootClass, const char *immName);
	std::string &GetRoot() { ENTER_OAMSA_TRANSLATIONS(); LEAVE_OAMSA_TRANSLATIONS(); return myRootDN; }
	void SetRoot(const std::string& theRoot) { ENTER_OAMSA_TRANSLATIONS(); LEAVE_OAMSA_TRANSLATIONS(); myRootDN = theRoot; }
	std::string &GetClass() { ENTER_OAMSA_TRANSLATIONS(); LEAVE_OAMSA_TRANSLATIONS(); return myRootClass; }
	void SetClass(const std::string &theClass) {ENTER_OAMSA_TRANSLATIONS(); myRootClass = theClass; LEAVE_OAMSA_TRANSLATIONS(); }
	std::string &GetLastCacheObj() { ENTER_OAMSA_TRANSLATIONS(); LEAVE_OAMSA_TRANSLATIONS(); return myLastCacheObj;}
	void SetLastCacheObj(const std::string &theCacheObj) { ENTER_OAMSA_TRANSLATIONS(); myLastCacheObj = theCacheObj; LEAVE_OAMSA_TRANSLATIONS();}
	std::string &GetImmName() { ENTER_OAMSA_TRANSLATIONS(); LEAVE_OAMSA_TRANSLATIONS(); return myImmName; }
	void SetImmName(const std::string &theImmName) {ENTER_OAMSA_TRANSLATIONS(); myImmName = theImmName; LEAVE_OAMSA_TRANSLATIONS(); }
	MafOamSpiTransactionHandleT GetTxContextH() {ENTER_OAMSA_TRANSLATIONS(); LEAVE_OAMSA_TRANSLATIONS(); return myTxH; }
	void SetTxContextH(MafOamSpiTransactionHandleT theContext) { ENTER_OAMSA_TRANSLATIONS(); myTxH = theContext; LEAVE_OAMSA_TRANSLATIONS(); }

	std::list<std::string>& GetRootChildList()	{ ENTER_OAMSA_TRANSLATIONS(); LEAVE_OAMSA_TRANSLATIONS(); return myRootChildList; }

	bool  PopFirstRootChild(std::string& theStr);

	void SetImmSearchInitialized() { ENTER_OAMSA_TRANSLATIONS(); myImmSearchSet = true; LEAVE_OAMSA_TRANSLATIONS(); }
	void ResetImmSearchInitialized() {ENTER_OAMSA_TRANSLATIONS(); myImmSearchSet = false; LEAVE_OAMSA_TRANSLATIONS(); }
	bool IsImmSearchInitialized() {ENTER_OAMSA_TRANSLATIONS(); LEAVE_OAMSA_TRANSLATIONS(); return	myImmSearchSet; }

private:
	bool 	myImmSearchSet;
	std::string myRootDN;
	std::string myRootClass;
	std::string myLastCacheObj;
	std::string myImmName;
	std::list<std::string> myRootChildList;
	MafOamSpiTransactionHandleT myTxH;
};


inline bool BridgeImmIterator::PopFirstRootChild(std::string& theStr)
{
	ENTER_OAMSA_TRANSLATIONS();
	bool RetVal = false;
	if (!myRootChildList.empty())
	{
		theStr = myRootChildList.front();
		myRootChildList.pop_front();
		RetVal = true;
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return RetVal;
}
class OamSAImmBridge
{
public:
	OamSAImmBridge();
	~OamSAImmBridge();

	MafReturnT getImmMoAttribute(MafOamSpiTransactionHandleT txHandle, const char *dn,
							const char *attributeName, MafMoAttributeValueContainer_3T  **result);

	MafReturnT setImmMoAttribute(MafOamSpiTransactionHandleT txHandle, const char *dn,
							const char *attributeName, MafMoAttributeValueContainer_3T  *AttributeValue);

	MafReturnT createImmMo(MafOamSpiTransactionHandleT txHandle, const char *parentDn,
							const char *className, const char *keyAttributeName, const char *keyAttributevalue,
							MafMoNamedAttributeValueContainer_3T **initialAttributes);

	MafReturnT deleteImmMo(MafOamSpiTransactionHandleT txHandle, const char *dn);

	MafReturnT existsImmMo(MafOamSpiTransactionHandleT txHandle, const char * dn, bool * result);

	MafReturnT countImmMoChildren(MafOamSpiTransactionHandleT txHandle, const char * dn, const char * className, uint64_t * result);

	MafReturnT action(MafOamSpiTransactionHandleT txHandle, const char * dn, const char * name,
								MafMoAttributeValueContainer_3T **parameters, MafMoAttributeValueContainer_3T **result);

	MafReturnT getImmMoIterator(MafOamSpiTransactionHandleT txHandle, const char *dn,
								const char *className, MafOamSpiMoIteratorHandle_3T *ith);

	MafReturnT finalizeImmMoIterator(MafOamSpiMoIteratorHandle_3T itHandle);

	MafReturnT getImmNextMo (MafOamSpiMoIteratorHandle_3T itHandle, char **result);

	MafReturnT OamSAJoin(MafOamSpiTransactionHandleT txHandle);

	MafReturnT OamSAPrepare(MafOamSpiTransactionHandleT txHandle);

	MafReturnT OamSACommit(MafOamSpiTransactionHandleT txHandle);

	MafReturnT OamSAAbort(MafOamSpiTransactionHandleT txHandle);

	MafReturnT OamSAFinish(MafOamSpiTransactionHandleT txHandle);

	MafReturnT OamSAValidate(MafOamSpiTransactionHandleT txHandle, bool *result);

	static const char *strError(SaAisErrorT e);

	static OamSAImmBridge *getOamSAImmBridge();

	MafReturnT releaseAdminOwnerForDn(MafOamSpiTransactionHandleT txhandle);

private:
	// Support method that retrieves the two last element in a 3Gpp dn to identify the parent and class name of these.
	void retrieveLastElementFromDn(OamSACache::DNList theDNList,
								std::string& className,
								std::string& parentClassName,
								bool onlyClassName = false);

	MafReturnT getImmMoAttributeSimple(MafOamSpiTransactionHandleT txHandle, const char *dn,
								const char *attributeName, MafOamSpiMoAttributeType_3T attrType,
								MafMoAttributeValueContainer_3T  **result, bool isKeyAttribute = false);

	MafReturnT readStructFromImm(MafOamSpiTransactionHandleT txHandle,
								const char *dn, const char *attributeName, MafMoAttributeValueContainer_3T **result);

	MafReturnT writeStructToImm(MafOamSpiTransactionHandleT txHandle,
								const char *dn,	const char *attributeName, MafMoAttributeValueContainer_3T *AttributeValue);

	std::string translateClassName(TxContext *txContext, const std::string& parentName, const std::string& className);

	std::string translateStructName(TxContext *txContext, const std::string& ObjectName, const std::string& structAttrName);

	MafReturnT lockDn(TxContext *txContext, std::string &immDn, SaImmScopeT scope);
	MafReturnT unLockDnOnFailure(TxContext *txContext, std::string &immDn, SaImmScopeT scope);

	SaAisErrorT checkSetAdminOwner(TxContext *txContext, std::string &dn, SaImmScopeT scope);
	SaAisErrorT releaseAdminOwner(TxContext *txContext, std::string &dn, SaImmScopeT scope);
	bool isAdminOwnerLockedByAnotherTx(TxContext *txContext, SaAisErrorT err);
	SaAisErrorT getImmAttribute(TxContext *txContextIn, const char *dn, const char *attributeName, SaImmAttrValuesT_2 ***AttrValue);
	SaAisErrorT getAdminOwner(TxContext *txContext, std::string &dnIn, std::string &adminOwner);
	SaAisErrorT ccbApply(TxContext *txContext);
	SaAisErrorT ccbValidate(TxContext *txContext);
	SaAisErrorT ccbAbort(TxContext *txContext);
	SaAisErrorT ccbInit(TxContext *txContext);
	SaAisErrorT ccbFinalize(TxContext *txContext);
	SaAisErrorT ccbObjectCreate(TxContext *txContext, SaNameT *parent, SaImmClassNameT className, SaImmAttrValuesT_2** attrValsIn);
	SaAisErrorT ccbObjectDelete(TxContext *txContext, SaNameT* dnIn);
	SaAisErrorT ccbModify(TxContext *txContext, SaNameT *dn, SaImmAttrModificationT_2 **attrMods);
	static 	OamSAImmBridge *oamSAImmBridge;

	bool handleCcbOnFailedOperation(MafOamSpiTransactionHandleT txHandle);
	MafReturnT handlePrepareOnImmAbort(MafOamSpiTransactionHandleT txHandle, bool fwdErrStrings = true);

	std::string removeEqualAndDot(std::string DnFragment);

	void invalidModelDetected(const std::string &strFunction, const std::string &dn, const std::string &className);

protected:
	const char* parseAndCategorizeErrorString(const char* errorString, ThreadContextMsgCategory_2 &category);
	MafReturnT getAndForwardErrorStrings(TxContext *txContext, bool& immResourceAbort, bool checkImmResourceAbort = false);
	//sdp875: this function is used for get and forward error string invoke by admin operation
	MafReturnT getAndForwardErrorStrings_2(TxContext *txContextIn, SaImmAdminOperationParamsT_2 **returnParams);
};

#endif
