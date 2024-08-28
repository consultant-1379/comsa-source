#ifndef ComOamSpiCmRouterEvent_1_h__
#define ComOamSpiCmRouterEvent_1_h__

#include <ComOamSpiEvent_1.h>
#include <ComMgmtSpiCommon.h>
#include <ComOamSpiManagedObject_1.h>

/**
 * @file
 * The CmRouter Event Producer will produce events for MO
 * operations create MO, delete MO, and modify attribute. The
 * events can be received by event consumers that have
 * registered for the event and provided a filter. A filter is
 * used by the consumer to select the DNs that an event
 * notification shall be sent for.
 *
 * Events are only sent for MOs that match the filter that the
 * client provided at its registration in the Event Service
 * The CmRouterEventProducer only supports one filter type,
 * CmRouterFilterTypeRegExp. Several filter values can be
 * specified in the filter in the form ComNameValuePairT, where
 * the name is ComCmRouterFilterRegExp and the value is a
 * Perl-compatible regular expression in string form. The
 * regular expressions will be compiled by the producer using no
 * extra compilation options and matched against all DNs that
 * operations are performed on. Therefore the regular expression
 * must be designed with efficiency of execution in mind.
 *
 * @b Example:
 * @code
 * ComNameValuePairT *myFilter[3];
 *
 * ComNameValuePairT filter1 = {ComCmRouterFilterTypeRegExp,"^ManagedElement=1,ApplX=1"};
 *
 * ComNameValuePairT filter2 = {ComCmRouterFilterTypeRegExp,"^ManagedElement=1,ApplX=[4-9]$"};
 *
 * myFilter[0] = &filter1;
 * myFilter[1] = &filter2;
 * myFilter[2] = 0;
 * eventService->registerConsumer(&myConsumerIf,
 *                                ComCmRouterEventTypeModifyMoAttr,
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
 * The event is sent if an attribute has been modified
 * and successfully committed.
 * The filter and transaction handle
 * are set in the value struct.
 * Requires a filter of type ComCmRouterFilterTypeRegExp.
 * Only one event will be sent per transaction.
 */
#define ComCmRouterEventTypeModifyMoAttr "ComCmRouterEventTypeModifyMoAttr"
/**
 * The event is sent when an MO has been created
 * and successfully committed.
 * The filter and transaction handle are set in the value
 * struct. Requires a filter of type
 * ComCmRouterFilterTypeRegExp. Only one event will be sent per
 * transaction.
 */
#define ComCmRouterEventTypeCreateMo "ComCmRouterEventTypeCreateMo"
/**
 * The event is sent when an MO has been deleted
 * and successfully committed.
 * The filter and transaction handle are set in the value
 * struct. Requires a filter of type
 * ComCmRouterFilterTypeRegExp. Only one event will be sent per
 * transaction.
 */
#define ComCmRouterEventTypeDeleteMo "ComCmRouterEventTypeDeleteMo"
/**
 * The event is sent each time a transaction is used.
 * The transaction handle is set in the value struct.
 * Requires an empty filter.
 * This event is mainly for internal use by the COM Basic
 * functionality.
 */
#define ComCmRouterEventTypeTransactionUsed "ComCmRouterEventTypeTransactionUsed"
/**
 * The filter type specifies that the filter
 * is a regular expression string.
 */
#define ComCmRouterFilterTypeRegExp "ComCmRouterFilterTypeRegExp"


/**
 * The struct contains values relevant to the event.
 * The struct is provided to the consumer as a parameter
 * in the notify call
 */
typedef struct ComOamSpiCmRouterEventValue_1 {

    /**
     * The transaction handle. Set if relevant, otherwise set to 0
     */
    ComOamSpiTransactionHandleT txHandle;

} ComOamSpiCmRouterEventValue_1T;


/**
 * The event is sent when a tx is committed.
 * The filter and transaction handle are set in the value
 * struct. Requires a filter of type ComCmRouterFilterTypeRegExp.
 * The event value will contain a list of ComOamSpiCmRouterTxOperation_1T with
 * CM operations.
 */
#define ComCmRouterEventTypeTxCommitted "ComCmRouterEventTypeTxCommitted"

/**
 * The struct contains values relevant to the EventType
 * TxCommitted.
 **/
typedef struct ComOamSpiCmRouterTxOperation_1 {

    /* MO distinguished name in 3GPP format */
    const char *dn;

    /* Transaction handle */
    ComOamSpiTransactionHandleT txHandle;

    /* Event type */
    const char *eventType;

    /* Name of the changed attribute. */
    const char *attributeName;

    /* Container with the new attribute value. */
    ComMoAttributeValueContainerT *attributeValueContainer;

} ComOamSpiCmRouterTxOperation_1T;


typedef struct ComOamSpiCmRouterTxCommittedValue_1 {
    /* Timestamp when the operations were committed in nanoseconds */
    uint64_t eventTime;

    /* A null terminated array with tx operations. */
    ComOamSpiCmRouterTxOperation_1T **txChange;
} ComOamSpiCmRouterTxCommittedValue_1T;

#endif
