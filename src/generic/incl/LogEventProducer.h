#ifndef SRC_GENERIC_INCL_LOGEVENTPRODUCER_H_
#define SRC_GENERIC_INCL_LOGEVENTPRODUCER_H_

#include "MafOamSpiLoggingEvent_1.h"
#include "MafMgmtSpiCommon.h"
#include "MafMgmtSpiInterfacePortal_3.h"
#include "LoggingEvent.h"
#ifdef  __cplusplus
extern "C" {
#endif

/*
 *  Creates the LogEventHandler object and starts it
 */
MafReturnT start_LogEventProducer(MafMgmtSpiInterfacePortal_3T *portal_MAF);
MafReturnT push_LogEventProducer(MafStreamLoggingLevelChangeValue_2T *mafCmNot);
MafReturnT stop_LogEventProducer(void);

#ifdef  __cplusplus
}
#endif


#endif /* SRC_GENERIC_INCL_LOGEVENTPRODUCER_H_ */
