#ifndef ComOamSpiNotificationFm_1_h__
#define ComOamSpiNotificationFm_1_h__

#include <stdint.h>

/**
 * @file ComOamSpiNotificationFm_1.h
 *
 * Fault management specific notification description.
 */

/**
 * This event is sent when a new notification have been sent from the MW.
 *
 * @deprecated replaced by ComOamSpiNotificationFmEventType_2
 */
#define ComOamSpiNotificationFmEventType_1 "ComOamSpiNotificationFmEventType_1"

/**
 * This filter type specifies that only notifications newer than the value
 * of this filter should be sent.
 *
 * If the value is NULL then Fault Management does not have any history of
 * any previous notifications and all notifications should be sent.
 *
 * @deprecated replaced by ComOamSpiNotificationFmFilterTypeDateTime_2
 */
#define ComOamSpiNotificationFmFilterTypeDateTime_1 "ComOamSpiNotificationFmFilterTypeDateTime_1"

/**
 * Value of ComOamSpiNotificationFmFilterTypeDateTime_1 filter. Its time
 * represents nanoseconds since the Epoch.
 *
 * @deprecated replaced by ComOamSpiNotificationFmFilterDateTimeValue_2T
 */
typedef uint64_t ComOamSpiNotificationFmFilterDateTimeValue_1T;

/**
 * Severity levels used in the notifications.
 */
typedef enum {
    /**
     * <p>Indicates that the problem has disappeared.
     * Only the stateful alarms can be cleared.
     * The stateless alarms (alerts) can not be cleared.</p>
     */
    ComOamSpiNotificationFmSeverityCleared,
    /**
     * <p>If the severity level is not specified, this level
     * will be used by default.</p>
     */
    ComOamSpiNotificationFmSeverityIndeterminate,
    /**
     * <p>Indicates that there is a problem.
     * The system may work without serious problems,
     * but the problem must be solved.</p>
     */
    ComOamSpiNotificationFmSeverityWarning,
    /**
     * <p>Indicates that there is a problem.
     * It must be solved.</p>
     */
    ComOamSpiNotificationFmSeverityMinor,
    /**
     * <p>Indicates that there is a problem.
     * It can lead to a partial or a complete
     * unfunctional system.</p>
     */
    ComOamSpiNotificationFmSeverityMajor,
    /**
     * <p>Indicates that there is a serious problem.
     * Immediate actions must be taken by the operator,
     * otherwise the system will be completely unfunctional.</p>
     */
    ComOamSpiNotificationFmSeverityCritical
} ComOamSpiNotificationFmSeverityT;

/**
 * Structure to handle the alarm object.
 *
 * @deprecated replaced by ComOamSpiNotificationFmStruct_2T
 */
typedef struct ComOamSpiNotificationFmStruct {
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
     * The major and minor types together define the alarm type.
     * The alarm information is looked up with help of
     * these two numbers. It is mandatory to supply.
     *
     * Minor type consists of two 16-bit positive integer.
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
     * Perceived severity level of the alarm.
     */
    ComOamSpiNotificationFmSeverityT severity;
} ComOamSpiNotificationFmStructT;

#endif

