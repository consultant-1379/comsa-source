/*
 * LoggingEvent.h
 *
 *  Created on: Nov 17, 2019
 *      Author: xharami
 */

#ifndef SRC_GENERIC_INCL_LOGGINGEVENT_H_
#define SRC_GENERIC_INCL_LOGGINGEVENT_H_

#include <stdint.h>

#define MafStreamLoggingLevelChangeEventType_2 "MafComStreamLoggingLevelChangeEventType_2"

typedef struct MafStreamLoggingLevelChangeValue_2 {
        uint32_t  streamType;
        uint32_t  newLevel;
    } MafStreamLoggingLevelChangeValue_2T;


#endif /* SRC_GENERIC_INCL_LOGGINGEVENT_H_ */
