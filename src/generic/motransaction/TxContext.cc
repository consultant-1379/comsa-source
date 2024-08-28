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
 *   File:   TxContext.cc
 *
 *   Author: efaiami & egorped
 *
 *   Date:   2010-05-21
 *
 *   This file holds some data needed during a tx.
 *
 *   Reviewed: efaiami 2010-06-14
 *
 *   Modify: efaiami 2011-02-23 for log and trace function
 *           ejonajo 2012-01-11 when printing pointer-values, we're using %p,
 *                              then the compiler doesn't complain on 64-bit.
 *
 *   Modify: uabjoy  2014-03-24 Adapt to support Trace CC.
 *   Modify: xdonngu 2014-10-31 Fix TR HT17443 - only create a ccb if it is not created
 *   Modify: xtronle 2014-12-05 Fix TR HT28574 - No ccb validation should be done by COM SA when ccb is empty
 *
 ***************************************************************************/

#include <ComSA.h>
#include <stdio.h>
#include <map>
#include <string>
#ifndef SaStringT
typedef char * SaStringT;
#endif
#include "trace.h"
#define IMM_A_02_01
#include "saImmOm.h"
#include "saAis.h"
#include "TxContext.h"


using namespace CM;

// need to be protected if this goes threaded
int TxContext::numInstances = 0;

TxContext::TxContext(bool isCmIf)
{
	DEBUG_OAMSA_TRANSLATIONS("Entered TxContext constructor");
	ENTER_OAMSA_TRANSLATIONS();

	/* TR HT17443 - only create a ccb if it is not created*/
	mImmCcbCreatedStatus = ImmCcbNotCreated;

	mAdminOwnerHandle = 0;
	mAccessorHandle = 0;
	mImmHandle = 0;
	mCcbHandle = 0;
	mFailed = false;
	isCm_if = isCmIf;
	numInstances++;
	isCcbEmpty = false; /* HT 28574 */
	LEAVE_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("Exited TxContext constructor");
}

TxContext::~TxContext()
{
	ENTER_OAMSA_TRANSLATIONS();

	DEBUG_OAMSA_TRANSLATIONS("TxContext destructor executing");
	SaAisErrorT err = SA_AIS_OK;

	if(mCcbHandle != 0)
	{
		if(SA_AIS_OK == (err = saImmOmCcbFinalize(mCcbHandle)))
		{
			DEBUG_OAMSA_TRANSLATIONS("TxContext::~TxContext() saImmOmCcbFinalize return code: SA_AIS_OK");
			mCcbHandle = 0;
		}
		else
		{
			DEBUG_OAMSA_TRANSLATIONS("TxContext::~TxContext() saImmOmCcbFinalize failed with error code: %d", err);
		}
	}

	std::map<BridgeImmIterator*, SaImmSearchHandleT >::iterator iter;
	for( iter = mSearchHandleMap.begin(); iter != mSearchHandleMap.end(); ++iter )
	{
		saImmOmSearchFinalize(iter->second);
	}

	if (mAccessorHandle)
	{
		if(SA_AIS_OK == (err = saImmOmAccessorFinalize(mAccessorHandle)))
		{
			DEBUG_OAMSA_TRANSLATIONS("TxContext::~TxContext() saImmOmAccessorFinalize return code: SA_AIS_OK");
			mAccessorHandle = 0;
		}
		else
		{
			DEBUG_OAMSA_TRANSLATIONS("TxContext::~TxContext() saImmOmAccessorFinalize failed with error code: %d", err);
		}
	}

	if (mAdminOwnerHandle)
	{
		if(SA_AIS_OK == (err = saImmOmAdminOwnerFinalize(mAdminOwnerHandle)))
		{
			DEBUG_OAMSA_TRANSLATIONS("TxContext::~TxContext() saImmOmAdminOwnerFinalize return code: SA_AIS_OK");
			mAdminOwnerHandle = 0;
		}
		else
		{
			DEBUG_OAMSA_TRANSLATIONS("TxContext::~TxContext() saImmOmAdminOwnerFinalize failed with error code: %d", err);
		}
	}

	// remove the above finalizes(?), only this one should be needed
	if(mImmHandle)
	{
		if(SA_AIS_OK == (err = saImmOmFinalize(mImmHandle)))
		{
			DEBUG_OAMSA_TRANSLATIONS("TxContext::~TxContext() saImmOmFinalize return code: SA_AIS_OK");
			mImmHandle = 0;
		}
		else
		{
			DEBUG_OAMSA_TRANSLATIONS("TxContext::~TxContext() saImmOmFinalize failed with error code: %d", err);
		}
	}

	gc();
	numInstances--;
	LEAVE_OAMSA_TRANSLATIONS();
}

void TxContext::gc(){
	ENTER_OAMSA_TRANSLATIONS();
	tmpStrings.erase(tmpStrings.begin(), tmpStrings.end());
	LEAVE_OAMSA_TRANSLATIONS();
}

bool
TxContext::addAdminOwnerFor(std::string &fdn, int scope){
	ENTER_OAMSA_TRANSLATIONS();
	// false if already exist(=not added again)
	if (mAdminOwners.insert(std::make_pair(fdn, scope)).second)
	{
	LEAVE_OAMSA_TRANSLATIONS();
		return true;
	}

	std::map<std::string,int>::iterator iter = mAdminOwners.find(fdn);

	if(iter == mAdminOwners.end()){
		ERR_OAMSA_TRANSLATIONS("TxContext::addAdminOwnerFor internal error fdn(%s), scope(%d)",fdn.c_str(),scope);
	LEAVE_OAMSA_TRANSLATIONS();
		return false;
	}
	iter->second = scope;
	LEAVE_OAMSA_TRANSLATIONS();
	return true;
}

bool
TxContext::isAdminOwnerFor(std::string &fdn, int scope){
	ENTER_OAMSA_TRANSLATIONS();
	//return true if we are owner
	std::map<std::string,int>::iterator iter = mAdminOwners.find(fdn);
	//SA_IMM_ONE=1, SA_IMM_SUBLEVEL=2,SA_IMM_SUBTREE=3 according to Spec.
	LEAVE_OAMSA_TRANSLATIONS();
	return (iter != mAdminOwners.end() && scope <= iter->second);
}

bool
TxContext::removeAdminOwnerFor(std::string &fdn, int scope)
{
	ENTER_OAMSA_TRANSLATIONS();
	bool RetVal = false;
	if (isAdminOwnerFor(fdn, scope))
	{
		mAdminOwners.erase(fdn);
		RetVal = true;
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return RetVal;
}

bool
TxContext::addSearchHandle( BridgeImmIterator* iter_p, SaImmSearchHandleT handle)
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("TxContext::addSearchHandle(%p)",iter_p);
	LEAVE_OAMSA_TRANSLATIONS();
	return mSearchHandleMap.insert(std::make_pair(iter_p, handle)).second;
}

// erase and delete search handle.
bool
TxContext::deleteSearchHandle( BridgeImmIterator* iter_p )
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("TxContext::deleteSearchHandle(%p)",iter_p);
	std::map<BridgeImmIterator*, SaImmSearchHandleT >::iterator iter = mSearchHandleMap.find(iter_p);
	if (iter != mSearchHandleMap.end())
	{
		saImmOmSearchFinalize(iter->second);
		mSearchHandleMap.erase(iter);
		LEAVE_OAMSA_TRANSLATIONS();
		return true;
	}

	LEAVE_OAMSA_TRANSLATIONS();
	return false;
}

// return search handle or null if it doesn't exist
SaImmSearchHandleT
TxContext::getSearchHandle(BridgeImmIterator* iter_p)
{
	ENTER_OAMSA_TRANSLATIONS();
	std::map<BridgeImmIterator*, SaImmSearchHandleT >::iterator iter = mSearchHandleMap.find(iter_p);
	if ((iter != mSearchHandleMap.end()))
	{
		LEAVE_OAMSA_TRANSLATIONS();
		return  iter->second;
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return 0;
}
