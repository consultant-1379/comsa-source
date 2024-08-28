#ifndef MafOamSpiLoggingEvent_1_h__
#define MafOamSpiLoggingEvent_1_h__

#include <stdint.h>


/**
 * @file MafOamSpiLoggingEvent_1.h
 * @ingroup MafOamSpi
 *
 * COM/MAF logging level change event description.
 */

/**
 * This event is sent when a new logging level has been sent from the MW.
 */
#define MafLoggingLevelChangeEventType_1 "MafComLoggingLevelChangeEventType_1"

typedef struct MafLoggingLevelChangeEventValue_1 {
    uint32_t  newLevel;
} MafLoggingLevelChangeEventValue_1T;

#endif
