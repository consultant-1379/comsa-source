/*
* MockTransactionMasterIF2.h
*
*  Created on: Sep 30, 2011
*      Author: uabjoy
*
*   Modified: xnikvap 2012-08-30  support for COM MO SPI Ver.3 (Ver.1 is not supported any more)
*/
#include "MafMgmtSpiCommon.h"
#include "MafMgmtSpiComponent_2.h"
#include "MafMgmtSpiInterfacePortal_3.h"
#include "MafOamSpiModelRepository_1.h"
#include "MafOamSpiManagedObject_3.h"
#include "MafOamSpiRegisterObjectImplementer_3.h"
#include "MafOamSpiTransactionMaster_2.h"
#include "MafMgmtSpiThreadContext_2.h"

#include "OamSATransactionRepository.h"

#ifndef MOCKTRANSACTIONMASTERIF2_H_
#define MOCKTRANSACTIONMASTERIF2_H_

class MockTransactionMasterIF_2 {
public:
	MockTransactionMasterIF_2();
	virtual ~MockTransactionMasterIF_2();
};


#endif /* MOCKTRANSACTIONMASTERIF2_H_ */
