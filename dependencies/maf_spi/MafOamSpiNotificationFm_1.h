#ifndef MafOamSpiNotificationFm_1_h__
#define MafOamSpiNotificationFm_1_h__

#include <stdint.h>


/**
 * @file MafOamSpiNotificationFm_1.h
 * @ingroup MafOamSpi
 *
 * Fault management specific notification description.
 */

/**
 * This event is sent when a new notification have been sent from the MW.
 *
 * @deprecated replaced by MafOamSpiNotificationFmEventType_2
 */
#define MafOamSpiNotificationFmEventType_1 "MafOamSpiNotificationFmEventType_1"

/**
 * This filter type specifies that only notifications newer than the value
 * of this filter should be sent.
 *
 * If the value is NULL then Fault Management does not have any history of
 * any previous notifications and all notifications should be sent.
 *
 * @deprecated replaced by MafOamSpiNotificationFmFilterTypeDateTime_2
 */
#define MafOamSpiNotificationFmFilterTypeDateTime_1 "MafOamSpiNotificationFmFilterTypeDateTime_1"

/**
 * Severity levels used in the notifications.
 */
typedef enum {
    /**
     * Indicates that the problem has disappeared.
     * Only the stateful alarms can be cleared.
     * The stateless alarms (alerts) can not be cleared.
     */
    MafOamSpiNotificationFmSeverityCleared,
    /**
     * If the severity level is not specified, this level
     * will be used by default.
     */
    MafOamSpiNotificationFmSeverityIndeterminate,
    /**
     * Indicates that there is a problem.
     * The system may work without serious problems,
     * but the problem must be solved.
     */
    MafOamSpiNotificationFmSeverityWarning,
    /**
     * Indicates that there is a problem.
     * It must be solved.
     */
    MafOamSpiNotificationFmSeverityMinor,
    /**
     * Indicates that there is a problem.
     * It can lead to a partial or a complete
     * unfunctional system.
     */
    MafOamSpiNotificationFmSeverityMajor,
    /**
     * Indicates that there is a serious problem.
     * Immediate actions must be taken by the operator,
     * otherwise the system will be completely unfunctional.
     */
    MafOamSpiNotificationFmSeverityCritical
} MafOamSpiNotificationFmSeverityT;

/**
 * Value of MafOamSpiNotificationFmFilterTypeDateTime_1 filter. Its time
 * represents nanoseconds since the Epoch.
 *
 * @deprecated replaced by MafOamSpiNotificationFmFilterDateTimeValue_2T
 */
typedef uint64_t MafOamSpiNotificationFmFilterDateTimeValue_1T;

/**
 * Structure to handle the alarm object.
 *
 * @deprecated replaced by MafOamSpiNotificationFmStruct_2T
 */
typedef struct MafOamSpiNotificationFmStruct {
    /**
     * Alarming object in the 3GPP distinguished name format.
     * This attribute, together with the major and minor type,
     * identifies the alarm instance. It is mandatory.
     */
    char* dn;
    /**
     * Major type for an alarm.
     * The major and minor type together define the alarm type.
     * The alarm information is looked up with help of
     * these two numbers. It is mandatory to supply.
     *
     * Major type is a vendor specific integer defined by
     * IANA as SMI Network Management Private Enterprise Codes.
     * For example @n
     * 193 is for Ericsson,
     * 18568 is for Service Availability Forum,
     * 32993 is for OpenSAF Foundation
     */
    uint32_t majorType;
    /**
     * Minor type for an alarm.
     * The major and minor type together define the alarm type.
     * The alarm information is looked up with help of
     * these two numbers. It is mandatory to supply.
     *
     * Minor type consist of two 16-bit positive integer.
     * The most significant 16-bit part identifies the subsystem or area and must be registered with COM.
     * The less significant 16-bit part is defined inside the subsystem or area.
     */
    uint32_t minorType;
    /**
     * Null-terminated string for additional information
     * sent with the alarm. It is optional.
     */
    char* additionalText;
    /**
     * Percieved severity level of the alarm.
     */
    MafOamSpiNotificationFmSeverityT severity;
} MafOamSpiNotificationFmStructT;

#endif

