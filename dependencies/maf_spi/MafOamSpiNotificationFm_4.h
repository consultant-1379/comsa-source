#ifndef MafOamSpiNotificationFm_4_h__
#define MafOamSpiNotificationFm_4_h__

#include <MafOamSpiNotificationFm_1.h>

#include <stdint.h>


/**
 * @file MafOamSpiNotificationFm_4.h
 * @ingroup MafOamSpi
 *
 * Fault management specific notification description.
 */

/**
 * This event is sent when a new notification have been sent from the MW.
 */
#define MafOamSpiNotificationFmEventType_4 "MafOamSpiNotificationFmEventType_4"

/**
 * This filter type specifies that only notifications newer than the value
 * of this filter should be sent.
 *
 * If the value is NULL then Fault Management does not have any history of
 * any previous notifications and all notifications should be sent.
 */
#define MafOamSpiNotificationFmFilterTypeDateTime_4 "MafOamSpiNotificationFmFilterTypeDateTime_4"


/**
 * This event type shall be used for FM notifications originating from MAF Components.
 */
#define MafOamSpiNotificationFmEventComponent_4 "MafOamSpiNotificationFmEventComponent_4"


/**
 * Value of MafOamSpiNotificationFmFilterTypeDateTime_4 filter. Its time
 * represents nanoseconds since the Epoch.
 */
typedef uint64_t MafOamSpiNotificationFmFilterDateTimeValue_4T;


/**
 * A data structures that contains two items of information, an identifier and a problem description.
 */
typedef struct MafOamSpiNotificationFmAdditionalInfo {
    /**
     * Null-terminated string for the identifier.
     * The max allowed size is 256 characters. Any additional characters will be cut away.
     */
    char* name;
    /**
     * Null-terminated string for the value or description associated with the identifier.
     * The max allowed size is 256 characters. Any additional characters will be cut away.
     */
    char* value;

} MafOamSpiNotificationFmAdditionalInfoT;


/**
 * Container struct for additional info
 */
typedef struct MafOamSpiFmAdditionalInfoContainer {
    /**
     * Array of additionalInfo
     */
    MafOamSpiNotificationFmAdditionalInfoT* additionalInfoArr;
    /**
     * The length of the array of MafOamSpiNotificationFmAdditionalInfo.
     */
    uint32_t size;
} MafOamSpiFmAdditionalInfoContainerT;

/**
 * Structure to handle the alarm object.
 */
typedef struct MafOamSpiNotificationFmStruct_4 {
    /**
     * Alarming object in the 3GPP distinguished name format.
     * This attribute, together with the major and minor type,
     * identifies the alarm instance. It is mandatory.
     */
    char* dn;
    /**
     * Major type for an alarm.
     * The major and minor types together define the alarm type.
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
     * Event time for an alarm.
     * The value is the number of nanoseconds since the Epoch.
     */
    uint64_t eventTime;
    /**
     * Null-terminated string for additional information
     * sent with the alarm. It is optional.
     */
    char* additionalText;
    /**
     * Perceived severity level of the alarm.
     */
    MafOamSpiNotificationFmSeverityT severity;
    /**
     * Further information about the problem. It is optional.
     */
    MafOamSpiFmAdditionalInfoContainerT additionalInfo;

} MafOamSpiNotificationFmStruct_4T;


#endif




