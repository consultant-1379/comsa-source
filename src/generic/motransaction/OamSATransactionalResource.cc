/**
 * Include files   Copyright (C) 2010 by Ericsson AB
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
 *   File:   OamSATransactionalResource.cc
 *
 *   Author: egorped & efaiami
 *
 *   Date:   2010-05-21
 *
 *   This file declares the need functions for Transaction services.
 *
 *  Reviewed: efaiami 2010-06-21
 *
 *  Modify: efaiami 2011-02-23  for log and trace function
 *  Modify: eozasaf 2011-07-08  for changing initialized IMM version in saImmOmInitialize()
 *  Modify: xjonbuc 2012-09-06  Implement SDP1694 - support MAF SPI
 *  Modify: uabjoy  2014-03-24  Adding support for Trace CC.
 *  Modify: xjonbuc 2014-06-24  Implementing Transaction SPI v2
 *  Modify: xnikvap 2015-08-02  MR42277 Remove old interfaces exposed to COM & CoreMW that are no longer supported for SLES12
 *
 ***********************************************************************/

#include <ComSA.h>
#include <MafOamSpiServiceIdentities_1.h>
#include "OamSATransactionalResource.h"
#include "OamSAImmBridge.h"
#include "trace.h"
#include "saImm.h"
#include "saImmOm.h"

/**
 *  Forward declaration of functions
 */

static MafReturnT join(MafOamSpiTransactionHandleT txHandle);

static MafReturnT prepare(MafOamSpiTransactionHandleT txHandle);

static MafReturnT commit(MafOamSpiTransactionHandleT txHandle);

static MafReturnT abort(MafOamSpiTransactionHandleT  txHandle);

static MafReturnT finish(MafOamSpiTransactionHandleT txHandle);

static MafReturnT validate(MafOamSpiTransactionHandleT txHandle, bool *result);

static MafOamSpiTransactionalResource_2T InterfaceStruct_V2 = {
	MafOamSpiTransactionalResource_2Id,
	join,
	prepare,
	commit,
	abort,
	finish,
	validate};

/**
 *	Global interface
 */
MafOamSpiTransactionalResource_2T* ExportOamSATransactionalResourceInterface_V2(void)
{

	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return &InterfaceStruct_V2;
}


static SaImmHandleT ImmHandle;
static SaVersionT  mVersion = (SaVersionT) { immReleaseCode, immMajorVersion, immMinorVersion };


/**
 *  Transactional Resource Open
 */
MafReturnT OamSATransactionalResourceOpen(void)
{
	ENTER_OAMSA_TRANSLATIONS();

	// NB!!!! set This is a work around to get rid of the IMM error code 2 (SA_AIS_ERR_LIBRARY)
	// by keeping an open handle for the entire life time of the COM SA library...
	SaAisErrorT err = saImmOmInitialize(&ImmHandle /* out */, NULL, &mVersion);

	DEBUG_OAMSA_TRANSLATIONS("mVersion value after saImmOmInitialize() call in OamSATransactionalResourceOpen(void): %c.%u.%u",
							 mVersion.releaseCode,mVersion.majorVersion,mVersion.minorVersion);
	if (SA_AIS_OK != err)
	{
		DEBUG_OAMSA_TRANSLATIONS("OamSATransactionalResourceOpen initializing IMM failed error code = %d", err);
		LEAVE_OAMSA_TRANSLATIONS();
		return MafFailure;
	}

	DEBUG_OAMSA_TRANSLATIONS("OamSATransactionalResourceOpen() - exit with MafOk");
	LEAVE_OAMSA_TRANSLATIONS();
	return MafOk;
}

/**
 *  ComLogServiceClose
 */
MafReturnT OamSATransactionalResourceClose(void)
{

	ENTER_OAMSA_TRANSLATIONS();
	  // NB!!!! set This is a work around to get ri of the IMM error code 2 (SA_AIS_ERR_LIBRARY)
	// by keeping an open handle for the entire life time of the COM SA library...
	if(ImmHandle){
		saImmOmFinalize(ImmHandle);
		ImmHandle = 0;
	}

	LEAVE_OAMSA_TRANSLATIONS();
	return MafOk;
}

/**
 *  Routines called via the pointer interface
 *
 */

/**
 *  join
 */
static MafReturnT join(MafOamSpiTransactionHandleT txHandle)
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("OamSATransactionalResource called...txhandle %lu", txHandle);
	MafReturnT	myRetVal = OamSAImmBridge::getOamSAImmBridge()->OamSAJoin(txHandle);
	DEBUG_OAMSA_TRANSLATIONS("OamSATransactionalResource return from OamSAImmBridge()->OamSAJoin...return value %d", myRetVal);
	LEAVE_OAMSA_TRANSLATIONS();
	return myRetVal;
}
 /**
  *  prepare
  */
static MafReturnT prepare(MafOamSpiTransactionHandleT txHandle)
{
	ENTER_OAMSA_TRANSLATIONS();
	MafReturnT	myRetVal = OamSAImmBridge::getOamSAImmBridge()->OamSAPrepare(txHandle);
	LEAVE_OAMSA_TRANSLATIONS();
	return myRetVal;
}

/**
 *  commit
 */
static MafReturnT commit(MafOamSpiTransactionHandleT txHandle)
{
	ENTER_OAMSA_TRANSLATIONS();
	MafReturnT	myRetVal = OamSAImmBridge::getOamSAImmBridge()->OamSACommit(txHandle);
	LEAVE_OAMSA_TRANSLATIONS();
	return myRetVal;
}

/**
 *  abort
 */
static MafReturnT abort(MafOamSpiTransactionHandleT txHandle)
{
	ENTER_OAMSA_TRANSLATIONS();
	MafReturnT	myRetVal = OamSAImmBridge::getOamSAImmBridge()->OamSAAbort(txHandle);
	LEAVE_OAMSA_TRANSLATIONS();
	return myRetVal;
}

/**
 *  finish
 */
static MafReturnT finish(MafOamSpiTransactionHandleT txHandle)
{
	ENTER_OAMSA_TRANSLATIONS();
	MafReturnT	myRetVal = OamSAImmBridge::getOamSAImmBridge()->OamSAFinish(txHandle);
	LEAVE_OAMSA_TRANSLATIONS();
	return myRetVal;
}

/**
 *  validate
 */
static MafReturnT validate(MafOamSpiTransactionHandleT txHandle, bool *result)
{
	ENTER_OAMSA_TRANSLATIONS();
	MafReturnT	myRetVal = OamSAImmBridge::getOamSAImmBridge()->OamSAValidate(txHandle, result);
	LEAVE_OAMSA_TRANSLATIONS();
	return myRetVal;
}

