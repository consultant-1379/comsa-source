#ifndef __OAMSA_TRANSACTIONREPOSITORY_H
#define __OAMSA_TRANSACTIONREPOSITORY_H
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
 *   File:   OamSATransactionRepository.h
 * 
 *   Author: egorped & efaiami
 * 
 *   Date:   2010-05-21
 *
 *   This file declares functions for the mapping between ComOamSpiTransactionHandleT and TxContext.
 * 
 *   Reviewed: efaiami 2010-06-22
 *
 *   Modified: xjonbuc 2012-08-30  support for COM MO SPI Ver.3 (Ver.1 is not supported any more)
 *   Modified: xjonbuc 2012-09-06  Implement SDP1694 - support MAF SPI
 *   Modified: xdonngu 2014-05-29  usingstd::shared_ptr for TxContext
 *   Modified: xdonngu 2014-05-29  correct indent and space/tab
 *   Modified: xnikvap 2015-08-02  MR42277 Remove old interfaces exposed to COM & CoreMW that are no longer supported for SLES12
 */

#include <map>
#include <pthread.h>
#include "MafMgmtSpiCommon.h"
#include "TxContext.h"
#include <tr1/memory>
using namespace CM;
using namespace std;
using namespace std::tr1;

class OamSATransactionRepository
{
public:
    OamSATransactionRepository();
    ~OamSATransactionRepository();

   std::tr1::shared_ptr<TxContext> newTxContext(const MafOamSpiTransactionHandleT txh);
   std::tr1::shared_ptr<TxContext> getTxContext(const MafOamSpiTransactionHandleT txh);

    bool       delTxContext(const MafOamSpiTransactionHandleT txh);
    const char *GetOwnerName();
    static OamSATransactionRepository *getOamSATransactionRepository();

    void ReleaseAttValContfromCache(MafMoAttributeValueContainer_3T *container);
    void ReleaseMultipleAttValContfromCache(MafMoAttributeValueContainer_3T **containers);
    void ClearContextMap(void);


private:
   typedef map<const MafOamSpiTransactionHandleT,std::tr1::shared_ptr<TxContext> > ContextMapType;
   typedef map<const MafOamSpiTransactionHandleT,std::tr1::shared_ptr<TxContext> >::iterator ContextMapIterator;
   static OamSATransactionRepository *oamSATransactionRepository;
   int            myOwnerIndex;
   ContextMapType theContextMap;

};
#endif
