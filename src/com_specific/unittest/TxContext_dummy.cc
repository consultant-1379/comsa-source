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
}

TxContext::~TxContext()
{
	SaAisErrorT err = SA_AIS_OK;
	mCcbHandle = 0;
	mAccessorHandle = 0;
	mAdminOwnerHandle = 0;
	mImmHandle = 0;
	gc();
}

void TxContext::gc(){
}

bool
TxContext::addSearchHandle( BridgeImmIterator* iter_p, SaImmSearchHandleT handle)
{
	return true;
}

// return search handle or null if it doesn't exist
SaImmSearchHandleT
TxContext::getSearchHandle(BridgeImmIterator* iter_p)
{
	return 0;
}
