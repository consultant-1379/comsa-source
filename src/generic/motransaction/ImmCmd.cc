/*
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
 *   File:   ImmCmd.cc
 *
 *   Author: efaiami & egorped
 *
 *   Date:   2010-05-21
 *
 *   This class acts as an interface to imm and imm communication.
 *
 *   Reviewed: efaiami 2010-06-10
 *
 *
 *   Reviewed: efaiami 2011-01-26  Com_SA Action
 *   Reviewed: efaiami 2011-01-23 for Unique Class name
 *   Modify:  efaiami 2011-02-22 for log and trace function
 *   Modify:  eozasaf 2011-07-08 for changing initialized IMM version
 *   Modify:  xduncao 2013-01-10 changed to use saImmOmAdminOperationInvoke_o2()
 *   Modify:  xjonbuc 2014-06-23  Implement MR-20275 support for explicit ccb validate() & abort()
 *   Modify:  xadaleg 2014-08-02 MR35347 - increase DN length
 *   Modify:  xanhqle 2014-12-03 TR HT28583 Ignore Try Again code in descrete classes
 *   Modify:  xnikvap 2015-08-02  MR42277 Remove old interfaces exposed to COM & CoreMW that are no longer supported for SLES12
 **************************************************************************************/

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "trace.h"
#include "ImmCmd.h"
#include "ComSA.h"
extern void PrintAttributeValues(SaImmAttrValuesT_2& AttrValues);

using namespace CM;

struct ImmAutoRetryProfile immAutoRetryProfileAdminSet = {1,5,100};

SaVersionT ImmCmd::mVersion = { immReleaseCode, immMajorVersion, immMinorVersion };
/*
 * ImmCmd, base class handles retries and standard error
 * handling
 */

ImmCmd::ImmCmd(TxContext * txContextIn, std::string cmd, int retries)
: mTxContextIn( txContextIn ), mCmd( cmd ), mRetries( retries ), isCCNameLM(false)
{
	ENTER_IMM_OM();
	if (0 == strcmp(_CC_NAME, "lm"))
	{
		isCCNameLM = true;
	}
	LEAVE_IMM_OM();
}

ImmCmd::~ImmCmd()
{
	ENTER_IMM_OM();
	int size = mTmpDns.size();
	for (int i=0; i<size; i++)
	{
		saNameDelete(mTmpDns.at(i), true);
	}
	LEAVE_IMM_OM();
}

SaAisErrorT
ImmCmd::execute()
{
	ENTER_IMM_OM();
	DEBUG_IMM_OM("%s",mCmd.c_str());
	SaAisErrorT err = SA_AIS_OK;
	int retry = 1;
	do {

		err = doExecute(); // call cmd implementor

		if (err == SA_AIS_ERR_TRY_AGAIN
		    || (isCCNameLM && (err == SA_AIS_ERR_FAILED_OPERATION)))
		{
			if ((retry % 10) == 0 && mRetries > 0) {
				DEBUG_IMM_OM("Try again(%i): %s", retry, mCmd.c_str());
			}
#ifndef UNIT_TEST
			usleep(500000);
#endif
		} else {
			DEBUG_IMM_OM("%s: %i", mCmd.c_str(), err);
		}
	}
	while ((err == SA_AIS_ERR_TRY_AGAIN || (isCCNameLM && (err == SA_AIS_ERR_FAILED_OPERATION)))
	       && ++retry <= mRetries);

	if (err != SA_AIS_OK) {
		if(err == SA_AIS_ERR_NOT_EXIST &&
			( mCmd == "saImmOmSearchNext" || mCmd == "saImmOmAccessorGet" || mCmd == "saImmOmSearchNext_2" || mCmd == "saImmOmAccessorGet_2")){
			//we get this every time the iterator ends
			DEBUG_IMM_OM("%s exited with IMM Error Code %i after %d tries",mCmd.c_str(), err, retry);
		}
		else {
			ERR_IMM_OM("%s failed with IMM Error Code %i after %d tries",mCmd.c_str(), err, retry);
		}
		if(err != SA_AIS_ERR_INVALID_PARAM &&
			err != SA_AIS_ERR_EXIST &&
			err != SA_AIS_ERR_NOT_EXIST){
		}
	}
	LEAVE_IMM_OM();
	return err;
}

SaNameT *
ImmCmd::toSaNameT(std::string &dnIn)
{
	ENTER_IMM_OM();
	// temp SaNameTs are deleted in destructor
	SaNameT * dn = NULL;
	if (dnIn.size()>0) {
		dn = new SaNameT();
		mTmpDns.push_back(dn);
		//from the root
		saNameSet(dnIn.c_str(), dn);
	}
	LEAVE_IMM_OM();
	return dn;
}

bool ImmCmd::isReqVersion(const SaVersionT currentVersion, SaUint8T reqReleaseCode, SaUint8T reqMajorVersion, SaUint8T reqMinorVersion) {
	return (currentVersion.releaseCode > reqReleaseCode || currentVersion.majorVersion > reqMajorVersion ||
		(currentVersion.majorVersion == reqMajorVersion && currentVersion.minorVersion >= reqMinorVersion));
}

//------------------------ ImmCmdOmInit ----------------------------------------------------
ImmCmdOmInit::ImmCmdOmInit(TxContext * txContextIn)
: ImmCmd(txContextIn, "saImmOmInitialize")
{
	ENTER_IMM_OM();
	LEAVE_IMM_OM();
}

SaAisErrorT
ImmCmdOmInit::doExecute()
{
	ENTER_IMM_OM();
	// set ImmHandle and register callbacks
	SaAisErrorT err = saImmOmInitialize(&(mTxContextIn->mImmHandle)/*out*/,
							 NULL,
							 &mVersion);
	if(err != SA_AIS_OK && err != SA_AIS_ERR_TRY_AGAIN)
	{
		ERR_IMM_OM("Error code returned by saImmOmInitialize() in ImmCmdOmInit::doExecute(): %d",err);
	}
	DEBUG_IMM_OM("mVersion value after saImmOmInitialize() call in ImmCmdOmInit::doExecute(): %c.%u.%u",mVersion.releaseCode,mVersion.majorVersion,mVersion.minorVersion);
	LEAVE_IMM_OM();
	return err;
}

//------------------------ ImmCmdOmAccessorInit ----------------------------------------------------

ImmCmdOmAccessorInit::ImmCmdOmAccessorInit( TxContext * txContextIn)
: ImmCmd(txContextIn, "saImmOmAccessorInitialize")
{
	ENTER_IMM_OM();
	LEAVE_IMM_OM();
}

SaAisErrorT
ImmCmdOmAccessorInit::doExecute()
{
	ENTER_IMM_OM();
	// set ImmAccessorHandle
	SaAisErrorT rc;
	rc = saImmOmAccessorInitialize(mTxContextIn->mImmHandle, &(mTxContextIn->mAccessorHandle)/*out*/);
	LEAVE_IMM_OM();
	return rc;
}


//------------------------ ImmCmdOmAdminOwnerInit ----------------------------------------------------
ImmCmdOmAdminOwnerInit::ImmCmdOmAdminOwnerInit( TxContext * txContextIn,
												std::string immOwnerNameIn,
												SaBoolT releaseOnFinalizeIn)
: ImmCmd(txContextIn,"saImmOmAdminOwnerInitialize"),
mImmOwnerNameIn( immOwnerNameIn ),
mReleaseOnFinalizeIn( releaseOnFinalizeIn )
{
	ENTER_IMM_OM();
	LEAVE_IMM_OM();
}

SaAisErrorT
ImmCmdOmAdminOwnerInit::doExecute()
{
	ENTER_IMM_OM();
	SaAisErrorT rc;
	// set ImmOwnerHandle
	rc = saImmOmAdminOwnerInitialize(mTxContextIn->mImmHandle,
			(const SaImmAdminOwnerNameT)mImmOwnerNameIn.c_str(),
										mReleaseOnFinalizeIn,
										&(mTxContextIn->mAdminOwnerHandle)/*out*/);
	LEAVE_IMM_OM();
	return rc;
}


//------------------------ ImmCmdOmAdminOwnerSet ----------------------------------------------------
ImmCmdOmAdminOwnerSet::ImmCmdOmAdminOwnerSet(TxContext * txContextIn,
						  std::vector <std::string> *objectDns,
						  SaImmScopeT scope)
: ImmCmd(txContextIn, "saImmOmAdminOwnerSet"),
mObjectDns( objectDns ), mScope( scope )
{
	ENTER_IMM_OM();
	LEAVE_IMM_OM();
}

SaAisErrorT
ImmCmdOmAdminOwnerSet::doExecute()
{
	ENTER_IMM_OM();
	if (mObjectDns->size()==0) {
		return SA_AIS_ERR_INVALID_PARAM;
	}
	int size = mObjectDns->size();
	SaNameT * objDns[size+1];
	objDns[size] = 0;
	unsigned int nTries = 1;
	for(int i=0; i<size; i++)
	{
		objDns[i] = toSaNameT(mObjectDns->at(i));
	}

	SaAisErrorT err = saImmOmAdminOwnerSet(mTxContextIn->mAdminOwnerHandle,
											(const SaNameT**)objDns,
											mScope);
	while ((err == SA_AIS_ERR_EXIST) && (nTries <= immAutoRetryProfileAdminSet.nTries))
		{
			DEBUG_IMM_OM("saImmOmAdminOwnerSet() returned SA_AIS_ERR_EXIST retrying %d time", nTries);
			usleep(immAutoRetryProfileAdminSet.retryInterval*1000);
			err = saImmOmAdminOwnerSet(mTxContextIn->mAdminOwnerHandle,(const SaNameT**)objDns,mScope);
			nTries++;
		}
	if (err != SA_AIS_OK && immAutoRetryProfileAdminSet.errorsAreNonFatal)
	{
		WARN_IMM_OM("saImmOmAdminOwnerSet FAILED, after %d retries, err = %d", nTries, (int)err);
		for(int i = 0; i < size; i++)
		{
			DEBUG_IMM_OM("saImmOmAdminOwnerSet for object: %s", (mObjectDns->at(i)).c_str());
		}
	}
	LEAVE_IMM_OM();
	return err;
}


//------------------------ ImmCmdOmAdminOwnerRelease ----------------------------------------------------
ImmCmdOmAdminOwnerRelease::ImmCmdOmAdminOwnerRelease(TxContext * txContextIn,
						  std::vector <std::string> *objectDns,
						  SaImmScopeT scope)
: ImmCmd(txContextIn, "saImmOmAdminOwnerRelease"),
mObjectDns( objectDns ), mScope( scope )
{
	ENTER_IMM_OM();
	LEAVE_IMM_OM();
}

SaAisErrorT
ImmCmdOmAdminOwnerRelease::doExecute()
{
	ENTER_IMM_OM();
	if (mObjectDns->size()==0) {
		return SA_AIS_ERR_INVALID_PARAM;
	}
	int size = mObjectDns->size();
	SaNameT * objDns[size+1];
	objDns[size] = 0;
	for(int i=0; i<size; i++)
		objDns[i] = toSaNameT(mObjectDns->at(i));

	SaAisErrorT err = saImmOmAdminOwnerRelease(mTxContextIn->mAdminOwnerHandle,
											(const SaNameT**)objDns,
											mScope);

	LEAVE_IMM_OM();
	return err;
}

//------------------------ ImmCmdOmAdminOwnerClear ----------------------------------------------------
ImmCmdOmAdminOwnerClear::ImmCmdOmAdminOwnerClear(TxContext * txContextIn,
						  std::string dnIn,
						  SaImmScopeT scope)
: ImmCmd(txContextIn, "saImmOmAdminOwnerClear"),
mDnIn( dnIn ), mScope( scope )
{
	ENTER_IMM_OM();
	LEAVE_IMM_OM();
}

SaAisErrorT
ImmCmdOmAdminOwnerClear::doExecute()
{
	ENTER_IMM_OM();

	SaNameT * objDns[2];
	objDns[1] = 0;
	objDns[0] = toSaNameT(mDnIn);

	SaAisErrorT err = saImmOmAdminOwnerClear(mTxContextIn->mImmHandle,
											(const SaNameT**)objDns,
											mScope);

	LEAVE_IMM_OM();
	return err;
}

//------------------------ ImmCmdOmClassDescriptionGet ----------------------------------------------------
ImmCmdOmClassDescriptionGet::ImmCmdOmClassDescriptionGet(TxContext * txContextIn/*in*/,
								SaImmClassNameT className/*in*/,
								SaImmClassCategoryT *classCategory/*out*/,
								SaImmAttrDefinitionT_2 *** attrDef/*out*/)
: ImmCmd(txContextIn,"saImmOmClassDescription_2"), mAttrDef( attrDef ),
mClassCategory( classCategory ), mClassName( className )
{
	ENTER_IMM_OM();
	LEAVE_IMM_OM();
}

SaAisErrorT
ImmCmdOmClassDescriptionGet::doExecute()
{
	ENTER_IMM_OM();
	LEAVE_IMM_OM();
	return saImmOmClassDescriptionGet_2(mTxContextIn->mImmHandle/*in*/, mClassName/*in*/,
									mClassCategory/*out*/,mAttrDef/*out*/);
}

//------------------------ ImmCmdOmKeySearchInit ----------------------------------------------------
ImmCmdOmKeySearchInit::ImmCmdOmKeySearchInit(TxContext *txContextIn,
											 std::string dnIn,
											 std::string classNameIn,
											 BridgeImmIterator* Iter_p)
: ImmCmd(txContextIn, "saImmOmSearchInitialize_2"),
mDnIn( dnIn ), mClassNameIn( classNameIn ), myIter_p( Iter_p )
{
	ENTER_IMM_OM();
	LEAVE_IMM_OM();
}

SaAisErrorT
ImmCmdOmKeySearchInit::doExecute()
{
	ENTER_IMM_OM();
	/*
		if dn = "" we start from root.
		if classname is set we know what class type we want.
	*/
	SaImmSearchHandleT searchHandle;
	SaNameT * dn = toSaNameT(mDnIn);
	SaImmSearchOptionsT searchOptions = SA_IMM_SEARCH_GET_NO_ATTR;
	SaImmSearchParametersT_2 * searchParameters = NULL;
	SaImmSearchParametersT_2  theSearchParams;
	char * pStringClassName = NULL;
	if(mClassNameIn.length() > 0) {
		pStringClassName = (char*)mClassNameIn.c_str();
		searchParameters=&theSearchParams;
		searchParameters->searchOneAttr.attrName = (char*)SA_IMM_ATTR_CLASS_NAME;
		searchParameters->searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
		searchParameters->searchOneAttr.attrValue = (void*)&pStringClassName;
		searchOptions =  (SA_IMM_SEARCH_GET_NO_ATTR | SA_IMM_SEARCH_ONE_ATTR);
	}

	SaAisErrorT err = saImmOmSearchInitialize_2(mTxContextIn->mImmHandle,
									dn ,  // if NULL, means from the root
									SA_IMM_SUBLEVEL,
									searchOptions,
									searchParameters,
									NULL,
									&searchHandle/*out*/);
	if (err!=SA_AIS_OK) {
		DEBUG_IMM_OM("ImmCmdOmKeySearchInit::doExecute fail err = %d", err);
		LEAVE_IMM_OM();
		return err;
	}
	if(!mTxContextIn->addSearchHandle(myIter_p,searchHandle)) {
		DEBUG_IMM_OM("ImmCmdOmKeySearchInit: addSearchHandle:search handle already exist");
	}

	LEAVE_IMM_OM();
	return SA_AIS_OK;
}

//------------------------ ImmCmdOmKeySearchNext ----------------------------------------------------
ImmCmdOmSearchNext::ImmCmdOmSearchNext(TxContext *txContextIn,
										std::string dnIn,
										BridgeImmIterator* Iter_p,
										std::string *dnOut/*out*/,
										SaImmAttrValuesT_2*** attrValsOut/*out*/)
: ImmCmd(txContextIn, "saImmOmSearchNext_2"),
mDnIn( dnIn ), mDnOut( dnOut ), mAttrValsOut( attrValsOut ), myIter_p(Iter_p)
{
	ENTER_IMM_OM();
	LEAVE_IMM_OM();
}

SaAisErrorT
ImmCmdOmSearchNext::doExecute()
{
	ENTER_IMM_OM();
	SaImmSearchHandleT searchHandle = mTxContextIn->getSearchHandle(myIter_p);
	if (searchHandle == 0)
		DEBUG_IMM_OM("Exiting, ImmCmdOmKeySearchNext failed to get searchHandle");
	SaNameT tmpDn;
	SaAisErrorT err = saImmOmSearchNext_2(searchHandle, &tmpDn, mAttrValsOut);
	if (err!=SA_AIS_OK)
	{
		DEBUG_IMM_OM("ImmCmdOmSearchNext::doExecute fail err = %d", err);
		LEAVE_IMM_OM();
		return err;
	}
	std::string x(saNameGet(&tmpDn), saNameLen(&tmpDn));
	*mDnOut = x;
	LEAVE_IMM_OM();
	return SA_AIS_OK;
}

//------------------------ ImmCmAccessorGet ----------------------------------------------------

/*
	attrNameIn: the value of the attr to get, "" gets all attrs
	TODO: change so that several named attrs can be fetched if needed
*/
ImmCmdOmAccessorGet::ImmCmdOmAccessorGet(TxContext *txContextIn,
										 std::string  dnIn,
										 std::string attrNameIn,
										 SaImmAttrValuesT_2*** attrValsOut /*out*/)
: ImmCmd(txContextIn,"saImmOmAccessorGet_2"),
mDnIn( dnIn ), mAttrNameIn( attrNameIn ), mAttrValsOut( attrValsOut )
{
	ENTER_IMM_OM();
	LEAVE_IMM_OM();
}

SaAisErrorT
ImmCmdOmAccessorGet::doExecute()
{
	ENTER_IMM_OM();
	SaNameT * dn = toSaNameT(mDnIn);
	SaImmAttrNameT attrNames[2];
	attrNames[0] = NULL;
	if(mAttrNameIn.length() > 0)
	{
		attrNames[0] = (char*)mAttrNameIn.c_str();
	}
	attrNames[1] = NULL;
	SaAisErrorT err = saImmOmAccessorGet_2( mTxContextIn->mAccessorHandle, dn, attrNames, mAttrValsOut/*out*/);

	LEAVE_IMM_OM();
	return err;
}

//------------------------ ImmCmdOmAdminOperationInvoke ----------------------------------------------------

ImmCmdOmAdminOperationInvoke::ImmCmdOmAdminOperationInvoke(TxContext *txContextIn,
								std::string  dnIn,
								SaImmAdminOperationIdT operationId,
								SaImmAdminOperationParamsT_2 **params,
								SaAisErrorT *operationReturnValue /*out*/,
								SaImmAdminOperationParamsT_2 ***returnParams /*out*/)
: ImmCmd(txContextIn, "saImmOmAdminOperationInvoke_o2"),
  mDnIn(dnIn), mOperationId(operationId), mParams(params), mOperationReturnValue(operationReturnValue), mReturnParams(returnParams)
{
	ENTER_IMM_OM();
	LEAVE_IMM_OM();
}

SaAisErrorT
ImmCmdOmAdminOperationInvoke::doExecute()
{
	ENTER_IMM_OM();

	SaNameT *objectName = toSaNameT(mDnIn);
	SaAisErrorT err = saImmOmAdminOperationInvoke_o2(mTxContextIn->mAdminOwnerHandle,
													 objectName,
													 0,  /* continuationId */
													 mOperationId,
													 (const SaImmAdminOperationParamsT_2 **) mParams,
													 mOperationReturnValue,
													 SA_TIME_MAX,  /* timeout */
													 mReturnParams);
	LEAVE_IMM_OM();
	return err;
}

//------------------------ ImmCmdOmAdminOperationMemoryFree ----------------------------------------------------
ImmCmdOmAdminOperationMemoryFree::ImmCmdOmAdminOperationMemoryFree(TxContext *txContextIn,
								SaImmAdminOperationParamsT_2 **returnParams)
: ImmCmd(txContextIn, "saImmOmAdminOperationMemoryFree"),
  mReturnParams(returnParams)
{
	ENTER_IMM_OM();
	LEAVE_IMM_OM();
}

SaAisErrorT
ImmCmdOmAdminOperationMemoryFree::doExecute()
{
	ENTER_IMM_OM();

	SaAisErrorT err = saImmOmAdminOperationMemoryFree(mTxContextIn->mAdminOwnerHandle,
														mReturnParams);
	LEAVE_IMM_OM();
	return err;
}

//------------------------ ImmCmdOmCcbInitialize ----------------------------------------------------
ImmCmdOmCcbInit::ImmCmdOmCcbInit(TxContext *txContextIn)
: ImmCmd(txContextIn, "saImmOmCcbInitialize")
{
	ENTER_IMM_OM();
	LEAVE_IMM_OM();
}

SaAisErrorT
ImmCmdOmCcbInit::doExecute()
{
	ENTER_IMM_OM();
	SaAisErrorT err = saImmOmCcbInitialize(mTxContextIn->mAdminOwnerHandle,
							   SA_IMM_CCB_REGISTERED_OI | SA_IMM_CCB_ALLOW_NULL_OI,
							   &(mTxContextIn->mCcbHandle)/*out*/);
	LEAVE_IMM_OM();
	return err;
}


//------------------------ ImmCmdOmCcbFinalize ----------------------------------------------------
ImmCmdOmCcbFinalize::ImmCmdOmCcbFinalize(TxContext *txContextIn)
: ImmCmd(txContextIn, "saImmOmCcbFinalize")
{
	ENTER_IMM_OM();
	LEAVE_IMM_OM();
}

SaAisErrorT
ImmCmdOmCcbFinalize::doExecute()
{
	ENTER_IMM_OM();
	SaAisErrorT immErr = saImmOmCcbFinalize(mTxContextIn->mCcbHandle);
	mTxContextIn->mCcbHandle = 0;
	LEAVE_IMM_OM();
	return immErr;

}
//------------------------ ImmCmdOmCcbObjectModify ----------------------------------------------------
ImmCmdOmCcbObjectModify::ImmCmdOmCcbObjectModify(TxContext *txContextIn,
												 SaNameT* dnIn,
												 SaImmAttrModificationT_2 **attrModsIn)
: ImmCmd(txContextIn, "saImmOmCcbObjectModify_2"),
mDnIn( dnIn ), mAttrModsIn( attrModsIn )
{
	ENTER_IMM_OM();
	LEAVE_IMM_OM();
}

SaAisErrorT
ImmCmdOmCcbObjectModify::doExecute()
{
	ENTER_IMM_OM();
	SaAisErrorT err = saImmOmCcbObjectModify_2(mTxContextIn->mCcbHandle,mDnIn,
								  (const SaImmAttrModificationT_2**)mAttrModsIn);
	LEAVE_IMM_OM();
	return err;
}

//------------------------ ImmCmdOmCcbValidate ----------------------------------------------------
ImmCmdOmCcbValidate::ImmCmdOmCcbValidate(TxContext* txContextIn)
: ImmCmd(txContextIn, "saImmOmCcbValidate")
{
	ENTER_IMM_OM();
	LEAVE_IMM_OM();
}

SaAisErrorT
ImmCmdOmCcbValidate::doExecute()
{
	ENTER_IMM_OM();
	SaAisErrorT	err = saImmOmCcbValidate(mTxContextIn->mCcbHandle);
	if (SA_AIS_OK != err && err != SA_AIS_ERR_TRY_AGAIN)
	{
		ERR_IMM_OM("ImmCmdOmCcbValidate::ImmCmdOmCcbValidate fail err = %d", err);
	}
	LEAVE_IMM_OM();
	return err;
}


//------------------------ ImmCmdOmCcbAbort----------------------------------------------------
ImmCmdOmCcbAbort::ImmCmdOmCcbAbort(TxContext* txContextIn)
: ImmCmd(txContextIn, "saImmOmCcbAbort")
{
	ENTER_IMM_OM();
	LEAVE_IMM_OM();
}

SaAisErrorT
ImmCmdOmCcbAbort::doExecute()
{
	ENTER_IMM_OM();
	SaAisErrorT	err = saImmOmCcbAbort(mTxContextIn->mCcbHandle);
	if (SA_AIS_OK != err && err != SA_AIS_ERR_TRY_AGAIN)
	{
		ERR_IMM_OM("ImmCmdOmCcbAbort::ImmCmdOmCcbAbortfail err = %d", err);
	}
	LEAVE_IMM_OM();
	return err;
}


//------------------------ ImmCmdOmCcbApply----------------------------------------------------
ImmCmdOmCcbApply::ImmCmdOmCcbApply(TxContext* txContextIn)
: ImmCmd(txContextIn, "saImmOmCcbApply")
{
	ENTER_IMM_OM();
	LEAVE_IMM_OM();
}


SaAisErrorT
ImmCmdOmCcbApply::doExecute()
{
	ENTER_IMM_OM();
	SaAisErrorT err = saImmOmCcbApply(mTxContextIn->mCcbHandle);
	if (SA_AIS_OK != err && err != SA_AIS_ERR_TRY_AGAIN)
	{
		ERR_IMM_OM("ImmCmdOmCcbApply::ImmCmdOmCcbApply fail err = %d", err);
	}
	LEAVE_IMM_OM();
	return err;
}

//------------------------ ImmCmdOmCcbCreate ----------------------------------------------------
ImmCmdOmCcbObjectCreate::ImmCmdOmCcbObjectCreate(TxContext* txContextIn,
												 SaNameT *parentIn,
												 SaImmClassNameT classNameIn,
												 SaImmAttrValuesT_2** attrValsIn)
: ImmCmd(txContextIn, "saImmOmCcbObjectCreate_2"),
mParentIn( parentIn ), mClassNameIn( classNameIn ), mAttrValsIn( attrValsIn )
{
	ENTER_IMM_OM();
	LEAVE_IMM_OM();
}

SaAisErrorT
ImmCmdOmCcbObjectCreate::doExecute()
{
	ENTER_IMM_OM();
	DEBUG_IMM_OM("ImmCmdOmCcbObjectCreate::doExecute() called");
	for (int i = 0; mAttrValsIn[i] != NULL ; i++)
	{
		PrintAttributeValues(*mAttrValsIn[i]);
	}

	// Get the class descripion and find the attributed


	SaImmClassCategoryT theCategory;
	SaImmAttrDefinitionT_2** theDefinitions = NULL;
	SaAisErrorT ErrRet = SA_AIS_OK;
	DEBUG_IMM_OM("ImmCmdOmCcbObjectCreate::doExecute() mClassNameIn = %s",mClassNameIn);

	ErrRet = saImmOmClassDescriptionGet_2(mTxContextIn->mImmHandle,
													  mClassNameIn,
													  &theCategory,
													  &theDefinitions);

	if (SA_AIS_ERR_NOT_EXIST == ErrRet)
	{
		// Give this output to make it easy to trouble shoot the system
		ERR_IMM_OM("after saImmOmClassDescriptionGet_2 ErrRet = %d\n", ErrRet);
	}
	else if (SA_AIS_OK != ErrRet && ErrRet != SA_AIS_ERR_TRY_AGAIN)
	{
		// Give this output to make it easy to trouble shoot the system
		ERR_IMM_OM("after saImmOmClassDescriptionGet_2 ErrRet = %d\n", ErrRet);
		saImmOmClassDescriptionMemoryFree_2(mTxContextIn->mImmHandle,
											theDefinitions);
	}
	else
	{
		saImmOmClassDescriptionMemoryFree_2(mTxContextIn->mImmHandle,
											theDefinitions);
	}

	if (saNameLen(mParentIn) != 0)
	{
		DEBUG_IMM_OM("Calling saImmOmCcbObjectCreate_2 with parent name length > 0");
		ErrRet =  saImmOmCcbObjectCreate_2(mTxContextIn->mCcbHandle,
											mClassNameIn,mParentIn,
											(const SaImmAttrValuesT_2**)mAttrValsIn);
	}
	else
	{
		DEBUG_IMM_OM("Calling saImmOmCcbObjectCreate_2 with parent name length == 0");
		ErrRet = saImmOmCcbObjectCreate_2(mTxContextIn->mCcbHandle,
											mClassNameIn,NULL,
											(const SaImmAttrValuesT_2**)mAttrValsIn);
	}
	if(ErrRet != SA_AIS_OK && ErrRet != SA_AIS_ERR_TRY_AGAIN)
	{
		ERR_IMM_OM("Exiting ImmCmdOmCcbObjectCreate::doExecute() return value for object create %d\n", ErrRet);
	}
	LEAVE_IMM_OM();
	return ErrRet;
}

//------------------------ ImmCmdOmCcbDelete ----------------------------------------------------
ImmCmdOmCcbObjectDelete::ImmCmdOmCcbObjectDelete(TxContext* txContextIn,
												 SaNameT* dnIn)
: ImmCmd(txContextIn, "saImmOmCcbObjectDelete"),
mDnIn( dnIn )
{
	ENTER_IMM_OM();
	LEAVE_IMM_OM();
}

SaAisErrorT
ImmCmdOmCcbObjectDelete::doExecute()
{
	ENTER_IMM_OM();
	SaAisErrorT err = saImmOmCcbObjectDelete(mTxContextIn->mCcbHandle,mDnIn);
	LEAVE_IMM_OM();
	return err;
}
