#ifndef MafOamSpiCmEvent_1_h_
#define MafOamSpiCmEvent_1_h_

#include <MafOamSpiEvent_1.h>
#include <MafMgmtSpiCommon.h>
#include <MafOamSpiManagedObject_3.h>


/**
 * @file
 * This file defines notifications for MIB updates. The
 * notifications can be received by event consumers that are
 * subscribing for the notification. When the consumer sets up a subscription, it provided zero to many
 * filters.
 * Filters are used to specify which from which MO instances the
 * notification shall be sent for.
 *
 * If any of the filters match, the consumer will get the notification. The exception is if no filters are provided,
 * the consumer will get all notifications.
 *
 * In this case, a filter is a regular expression. This will be compiled by the producer using no
 * extra compilation options and matched against all DNs that
 * operations are performed on. Therefore the regular expression
 * must be designed with efficiency of execution in mind.
 *
 * @b Example:
 * @code
 * MafNameValuePairT *myFilter[3];
 *
 * MafNameValuePairT filter1 = {MafOamSpiCmEvent_FilterTypeRegExp_1,"^ManagedElement=1,ApplX=1"};
 *
 * MafNameValuePairT filter2 = {MafOamSpiCmEvent_FilterTypeRegExp_1,"^ManagedElement=1,ApplX=[4-9]$"};
 *
 * myFilter[0] = &filter1;
 * myFilter[1] = &filter2;
 * myFilter[2] = 0;
 * eventService->registerConsumer(&myConsumerIf,
 *                                MafOamSpiCmEvent_Notification_1,
 *                                myFilter);
 * @endcode
 * filter1 will generate events for modifications to attributes on MOs
 * starting with the DN string "ManagedElement=1,ApplX=1".
 * @n
 * filter2 will generate events for modifications to attributes on
 * the MOs that have exactly the DN string "ManagedElement=1,ApplX=4" or
 * "ManagedElement=1,ApplX=5".
 */



/**
This type,indicates the source of the operation that led to the generation of this notification.
The values are aligned with 3GPP TS 32.662
It can have one of the following values:
<br><ol>
<li> ResourceOperation: The notification was generated in response to an internal operation of the resource; </li>
<li> ManagementOperation: The notification was generated in response to a management operation applied across the managed object boundary external to the managed object; </li>
<li> SonOperation: The notification was generated as result of a SON (Self Organising Network, used in Radio networks) process like self-configuration, self-optimization, self-healing etc.
      a system (MW) that has no support for SON will not use this value </li>
<li> Unknown: It is not possible to determine the source of the operation. </li>
</ol>
 */
typedef enum {
    MafOamSpiCmEvent_ResourceOperation_1 = 1,
    MafOamSpiCmEvent_ManagementOperation_1 = 2,
    MafOamSpiCmEvent_SonOperation_1 = 3,
    MafOamSpiCmEvent_Unknown_1 = 4
} MafOamSpiCmEvent_SourceIndicator_1T;


/**
This type,indicates an event type. It can have one of the following values:
<br><ol>
<li> MafOamSpiCmEvent_MoCreated_1: An MO is created. All MOs created should report
     create and delete notifications </li>
<li> MafOamSpiCmEvent_MoDeleted_1: An MO is deleted </li>
<li> MafOamSpiCmEvent_AttributeValueChange_1: One or more attributes have been updated in an existing MO.
          Note that attributes that are not marked as notifiable in the MOM should not be reported. </li>
<li> MafOamSpiCmEvent_Overflow_1: One or more notifications have been lost due to a flooding situation </li>
</ol>
 */
typedef enum {
    MafOamSpiCmEvent_MoCreated_1 = 1,
    MafOamSpiCmEvent_MoDeleted_1 = 2,
    MafOamSpiCmEvent_AttributeValueChange_1 = 3,
    MafOamSpiCmEvent_Overflow_1 = 4
} MafOamSpiCmEvent_EventType_1T;


/**
 * The filter type specifies that the filter
 * is a regular expression string. The filter is applied on the MO DN
 * (dn in MafOamSpiCmEvent_Notification_1T)
 */
#define MafOamSpiCmEvent_FilterTypeRegExp_1 "MafOamSpiCmEvent_FilterTypeRegExp_1"


/**
 * Requires a filter of type MafOamSpiCmEvent_FilterTypeRegExp_1.
 * The event value will contain a MafOamSpiCmEventNotification_1T
 */
#define MafOamSpiCmEvent_Notification_1 "MafOamSpiCmEvent_Notification_1"


/**
 * This struct conatins values relevant to the EventType
 *
 **/
typedef struct {

    /** MO distinguished name in 3GPP format */
    const char *dn;


    /** Event type */
    MafOamSpiCmEvent_EventType_1T eventType;


    /** A NULL terminated array of containers with the new attribute values. The field
     * shall be set to NULL if eventType is MafOamSpiCmEvent_MoDeleted. For a  MafOamSpiCmEvent_MoCreated,
     * the attribute values provided in the MO creation should be here */
    MafMoNamedAttributeValueContainer_3T ** attributes;

} MafOamSpiCmEvent_1T;

/**
 * the MIB changes done for one transaction
 */
typedef struct {

    /** Transaction handle, can be set to 0 if irrelevant
     * (which is the case if the change is not done in the scope of an NBI transaction)
     * The information is used to make it possible for the NBI client to discard notifications that are
     * a result of its own changes. The notifications are reported after the transaction is committed.
     */
    MafOamSpiTransactionHandleT txHandle;

    /** Timestamp when the changes were committed in nanoseconds (Epoch) */
    uint64_t eventTime;

    /** Indicates the source of the change */
    MafOamSpiCmEvent_SourceIndicator_1T sourceIndicator;

    /** A null terminated array with events. */
    MafOamSpiCmEvent_1T** events;

} MafOamSpiCmEvent_Notification_1T;

#endif
