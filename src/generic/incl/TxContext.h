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
 *
 *   File:   TxContext.h
 *
 *   Author: efaiami & egorped
 *
 *   Date:   2010-05-21
 *
 *   This class declares the local context that is needed for transactions.
 *
 *   Reviewed: efaiami 2010-06-11
 *
 *  Modify: efaiami 2011-02-23 for log and trace function
 *  Modify: xdonngu 2014-10-31  Fix TR HT17443 - only create a ccb if it is not created
 *  Modify: xtronle 2014-12-05  Fix TR HT28574 - No ccb validation should be done by COM SA when ccb is empty
 *******************************************************************************/

#ifndef __OAMSA_TXCONTEXT_H
#define __OAMSA_TXCONTEXT_H

#include <map>
#include <string>
#include <list>
#include "saImmOm.h"
#include "OamSACache.h"
#include "trace.h"

class BridgeImmIterator;


typedef enum  {
	ImmCcbNotCreated = 0,
	ImmCcbCreated = 1,
} ImmCcbCreatedStatusT;

namespace CM {

class TxContext {
private:

	/**
	*   a sorted map to hold the imm search handle(s).
	*   Don't know if more than one search can be done in the same
	*   tx so be sure to handle this case (using a map). key is BridgeImmIterator
	*   pointer. This was changed from rDN since there might be more than one
	*   search operator per object running at the same time.
	*   <string, SaUint64T>
	*/
	std::map <BridgeImmIterator*, SaImmSearchHandleT> mSearchHandleMap;
	/**
	*   pointer to the cache section in the transaction.
	*   It will hold this TxContext for the duration of the Tx.
	*/

	void ** mtxOpaque;
	 /**
	 * This class holds the cache for the transaction, one cache for each.
	 * Here goes all changed values, added and deleted object and they are
	 * not writtern to IMM until committ
	 */
	 OamSACache mCache;

	 /* TR HT17443 - only create a ccb if it is not created*/
	 ImmCcbCreatedStatusT mImmCcbCreatedStatus;

	 /* HT28574 - check ccb is whether empty or not*/
	 bool isCcbEmpty;

public:

	/**
	*  the imm ccb handle ( SaUint64T)where changes are
	*  added(cached) until the ccb is applied. 0 means no ccb.
	*/
	SaImmCcbHandleT mCcbHandle;

	/**
	*   pointer to immhandle(SaUint64T), common for all tx, DON'T delete here
	*/
	SaImmHandleT mImmHandle;

	/**
	*    pointer to accessorhandle(SaUint64T) for each tx.
	*/
	SaImmAccessorHandleT  mAccessorHandle;

	/**
	*   pointer to adminOwnerHandle for each tx
	*/
	SaImmAdminOwnerHandleT mAdminOwnerHandle;

	bool mFailed;

	/**
	 * flag that says if the deleted objects still exixts in IMM or not.
	 * true if the object still exists in IMM
	 */

	bool ccbDeleteFailed;

	/**
	  * flag that says isf we are called from cm_if or not.
	  * true if it is a tx initiated from cm_if
	  */

	bool isCm_if;
	/**
	* singleton for keeping count if number of active instances
	* needed by immClient for adminowner clashes
	*/
	static int numInstances;

	TxContext(bool isCmIf = true);
	~TxContext();

	/*
	*    caches the dns that we know we are admin owners for
	*    use map here to do find in logarithmic time.
	*    The int is not used for anything
	*/
	std::map<std::string,int> mAdminOwners;

	/*
	*   returns true if the owner was added, else is already added
	*/
	bool addAdminOwnerFor(std::string &fdn, int scope);

	/*
	*    returns true if the fdn was added (did not already exist)
	*/
	bool isAdminOwnerFor(std::string &fdn, int scope);
	/*
	*   returns true if the owner was found and removed
	*/
	bool removeAdminOwnerFor(std::string &fdn, int scope);

	/*
	*   return false if failed
	*/
	bool addSearchHandle(BridgeImmIterator* iter_p, SaImmSearchHandleT handle);

	/*
	*   erase and delete search handle.
	*   return false if failed(doesn't exist)
	*/
	bool deleteSearchHandle( BridgeImmIterator* iter_p );

	/*
	*   return search handle or null if it doesn't exist
	*/
	SaImmSearchHandleT  getSearchHandle(BridgeImmIterator* iter_p);

	/*
	*   gc deletes temporary variables in txContext
	*/
	void gc();

	/*
	*   Data Area for temp strings, should be garbage collected
	*   (gc()) when not needed anymore
	*/
	std::list<std::string> tmpStrings;

	/*
	*   Get a reference to the cache
	*/
	OamSACache& GetCache(void) {ENTER_OAMSA_TRANSLATIONS(); LEAVE_OAMSA_TRANSLATIONS(); return mCache; }


	 /* TR HT17443 - only create a ccb if it is not created*/
	ImmCcbCreatedStatusT getImmCcbCreatedStatus()
	{
		return mImmCcbCreatedStatus;
	}

	 /* TR HT17443 - only create a ccb if it is not created*/
	void setImmCcbCreatedStatus(ImmCcbCreatedStatusT immCcbCreatedStatus)
	{
		mImmCcbCreatedStatus = immCcbCreatedStatus;
	}

	/* HT28574 - check ccb is whether empty or not*/
	bool getIsCcbEmpty()
	{
		return isCcbEmpty;
	}
	void setIsCcbEmpty(bool CcbEmpty)
	{
		isCcbEmpty = CcbEmpty;
	}
};
}

#endif
