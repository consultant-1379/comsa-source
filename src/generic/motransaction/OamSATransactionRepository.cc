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
 *   File:   OamSATransactionRepository.cc
 *
 *   Author: egorped & efaiami
 *
 *   Date:   2010-05-22
 *
 *   This file implements functions for the mapping between MafOamSpiTransactionHandleT and TxContext.
 *
 *   Reviewed: efaiami 2010-06-22
 *
 *   Modify: efaiami 2011-02-23  for log and trace function
 *   Modify: xjonbuc 2012-08-30  support for COM MO SPI Ver.3 (Ver.1 is not supported any more)
 *   Modify: xjonbuc 2012-09-06  Implement SDP1694 - support MAF SPI
 *   Modify: uabjoy  2014-03-24  Adding support for Trace CC.
 *   Modify: xdonngu 2014-05-29  using std::tr1::shared_ptr for TxContext
 *   Modify: xdonngu 2014-05-29  correct indent and space/tab
 *   Modify: xnikvap 2015-08-02  MR42277 Remove old interfaces exposed to COM & CoreMW that are no longer supported for SLES12
 *
 ****************************************************************************************************/

#include <ComSA.h>
#include "OamSATransactionRepository.h"
#include "OamSAImmBridge.h"
#include "ImmCmd.h"
#include "trace.h"
#include <sstream>

static const char *myOwnerName = "OAMSA";

OamSATransactionRepository *OamSATransactionRepository::oamSATransactionRepository;

static OamSATransactionRepository    myTransactionRepository;
static pthread_mutex_t contextMapMutex = PTHREAD_MUTEX_INITIALIZER;

OamSATransactionRepository::OamSATransactionRepository() : myOwnerIndex(0)
{
    ENTER_OAMSA_TRANSLATIONS();

    oamSATransactionRepository = this;
    DEBUG_OAMSA_TRANSLATIONS("OamSATransactionRepository::defaultcontructor...oamSATransactionRepository  %lu", (unsigned long)oamSATransactionRepository );

    LEAVE_OAMSA_TRANSLATIONS();
}

OamSATransactionRepository::~OamSATransactionRepository()
{
    ENTER_OAMSA_TRANSLATIONS();
    LEAVE_OAMSA_TRANSLATIONS();
}

const char *OamSATransactionRepository::GetOwnerName()
{
    ENTER_OAMSA_TRANSLATIONS();
    LEAVE_OAMSA_TRANSLATIONS();
    return myOwnerName;
}

OamSATransactionRepository *OamSATransactionRepository::getOamSATransactionRepository()
{
    ENTER_OAMSA_TRANSLATIONS();
    LEAVE_OAMSA_TRANSLATIONS();
    return oamSATransactionRepository;
}

std::tr1::shared_ptr<TxContext> OamSATransactionRepository::newTxContext(const MafOamSpiTransactionHandleT txh)
{
    ENTER_OAMSA_TRANSLATIONS();
    DEBUG_OAMSA_TRANSLATIONS("OamSATransactionRepository::newTxContext...txhandle %lu", txh);
    std::tr1::shared_ptr<TxContext> txContext(new TxContext(false));
    DEBUG_OAMSA_TRANSLATIONS("OamSATransactionRepository:::newTxContext TxContext %lu", (unsigned long)txContext.get());
    //set immhandle
    ImmCmdOmInit immCmdOmInit(txContext.get());
    DEBUG_OAMSA_TRANSLATIONS("OamSATransactionRepository::newTxContext...after immCmdOmInit constructor");
    SaAisErrorT err = immCmdOmInit.execute();
    DEBUG_OAMSA_TRANSLATIONS("OamSATransactionRepository::newTxContext...after immCmdOmInit.execute");
    if (err != SA_AIS_OK)
    {
        DEBUG_OAMSA_TRANSLATIONS("OamSATransactionRepository::newTxContext.error after immCmdOmInit.execute errorcode %d", err);
        //delete txContext;
        LEAVE_OAMSA_TRANSLATIONS();
        std::tr1::shared_ptr<TxContext> ret;
        return ret;
    }
    // sett accessor for this tx
    ImmCmdOmAccessorInit immCmdOmAccessorInit(txContext.get()/*out accessor*/);
    err = immCmdOmAccessorInit.execute();
    if (err != SA_AIS_OK)
    {
        DEBUG_OAMSA_TRANSLATIONS("OamSATransactionRepository::newTxContext.error after immCmdOmAccessorInit.execute() errorcode %d", err);
        //delete txContext;
        LEAVE_OAMSA_TRANSLATIONS();
        std::tr1::shared_ptr<TxContext> ret;
        return ret;
    }
    // make a new owner name for each tx.
    // int ix should be enough for a couple of hundred years of continous operation

    std::string immAdminOwnerName=myOwnerName;
    std::ostringstream ss;
    ss << myOwnerIndex++;
    immAdminOwnerName += ss.str();

    // set admin onwer
    ImmCmdOmAdminOwnerInit immCmdOmAdminOwnerInit(txContext.get()/*sets adminowner in tx*/,
            immAdminOwnerName,
            SA_TRUE);
    err = immCmdOmAdminOwnerInit.execute();
    if (err != SA_AIS_OK)
    {
        DEBUG_OAMSA_TRANSLATIONS("OamSATransactionRepository::newTxContext.error after immCmdOmAdminOwnerInit.execute() errorcode %d", err);
        //delete txContext;
        LEAVE_OAMSA_TRANSLATIONS();
        std::tr1::shared_ptr<TxContext> ret;
        return ret;
    }

    pthread_mutex_lock( &contextMapMutex );
    theContextMap[txh] = txContext;
    pthread_mutex_unlock( &contextMapMutex );

    DEBUG_OAMSA_TRANSLATIONS("OamSATransactionRepository::newTxContext...exiting txContext %lu", (unsigned long)txContext.get());
    LEAVE_OAMSA_TRANSLATIONS();
    return txContext;
}

/**
 *   getTxContext
 */
std::tr1::shared_ptr<TxContext> OamSATransactionRepository::getTxContext(const MafOamSpiTransactionHandleT txh)
{
    ENTER_OAMSA_TRANSLATIONS();
    std::tr1::shared_ptr<TxContext> ret;

    pthread_mutex_lock( &contextMapMutex );
    ContextMapIterator iter = theContextMap.find(txh);
    if (iter != theContextMap.end())
    {
        ret = (*iter).second;
    }
    pthread_mutex_unlock( &contextMapMutex );
    LEAVE_OAMSA_TRANSLATIONS();
    return ret;
}

bool OamSATransactionRepository::delTxContext(const MafOamSpiTransactionHandleT txh)
{
    ENTER_OAMSA_TRANSLATIONS();
    DEBUG_OAMSA_TRANSLATIONS("OamSATransactionRepository::delTxContext...txhandle %lu", txh);

    pthread_mutex_lock( &contextMapMutex );
    bool ret = false;
    ContextMapIterator iter = theContextMap.find(txh);
    if (iter != theContextMap.end())
    {
        DEBUG_OAMSA_TRANSLATIONS("OamSATransactionRepository::delTxContext...handle found, calling delete %lu", txh);
        ret = true;
        //delete (*iter).second;
        theContextMap.erase(iter);
    }
    pthread_mutex_unlock( &contextMapMutex );

    LEAVE_OAMSA_TRANSLATIONS();
    return ret;
}


void OamSATransactionRepository::ReleaseAttValContfromCache(MafMoAttributeValueContainer_3T *container)
{
    ENTER_OAMSA_TRANSLATIONS();
    bool foundInCache = false;
    std::tr1::shared_ptr<TxContext> txContextIn;

    ContextMapIterator iter;
    pthread_mutex_lock( &contextMapMutex );
    for ( iter=theContextMap.begin() ;(iter != theContextMap.end()) && (foundInCache == false);iter++ )
    {
        txContextIn= (*iter).second;
        foundInCache = txContextIn->GetCache().ReleaseAttributeValueContainer(container);
    }
    pthread_mutex_unlock( &contextMapMutex );


    LEAVE_OAMSA_TRANSLATIONS();
}

void OamSATransactionRepository::ReleaseMultipleAttValContfromCache(MafMoAttributeValueContainer_3T **containers)
{
    ENTER_OAMSA_TRANSLATIONS();
    bool foundInCache = false;
    std::tr1::shared_ptr<TxContext> txContextIn;
    int i;
    ContextMapIterator iter;

    pthread_mutex_lock( &contextMapMutex );
    for (i=0; containers[i] !=NULL; i++)
    {
        foundInCache=false;
        for ( iter=theContextMap.begin() ;(iter != theContextMap.end()) && (foundInCache == false);iter++ )
        {
            txContextIn= (*iter).second;
            foundInCache = txContextIn->GetCache().ReleaseAttributeValueContainer(containers[i]);
        }
    }
    // For HS96956 - COMSA will de-allocate memory for MafMoAttributeValueContainer_3T pointer
    delete[] containers;
    // End HS96956

    pthread_mutex_unlock( &contextMapMutex );

    LEAVE_OAMSA_TRANSLATIONS();
}

void OamSATransactionRepository::ClearContextMap(void)
{
    ENTER_OAMSA_TRANSLATIONS();
    pthread_mutex_lock( &contextMapMutex );
    theContextMap.clear();
    pthread_mutex_unlock( &contextMapMutex );
    LEAVE_OAMSA_TRANSLATIONS();
}



