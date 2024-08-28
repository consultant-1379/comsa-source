#ifndef MafOamSpiEvent_1_h__
#define MafOamSpiEvent_1_h__

#include <MafMgmtSpiCommon.h>
#include <MafMgmtSpiInterface_1.h>
#include <MafMgmtSpiComponent_1.h>
#include <stdint.h>

/**
 * Event producer handle Id
 */
typedef unsigned long MafOamSpiEventProducerHandleT;

/**
 * Event consumer handle Id
 */
typedef unsigned long MafOamSpiEventConsumerHandleT;


/**
 * Event interface
 *
 * @file MafOamSpiEvent_1.h
 * @ingroup MafOamSpi
 *
 * The MAF Event SPI consists of an event producer interface, an event
 * consumer interface and an event router interface.
 *
 * The Event SPI does not describe the type or the value of a notification.
 * The detailed content of a notification is described in the producer's
 * header file.
 *
 */


/**
 * Event producer interface.
 * The producer must define the events to produce, define what filters
 * can be used, and what type of value it can send in the event.
 * The event type must be unique within the total event type space.
 *
 * Event Filtering:
 * The filter expression syntax used is defined per eventType.
 * A consumer provides an array of name-value pairs, where each value holds a filter expression.
 * The name in this name-value pair is used for selecting filter syntax (if the particular eventType can be filtered by
 * more than one filter syntax). The criteria for forwarding an event is that at least one of the filter expressions matches the event.
 * If no filters are provided, represented by a null terminated array, all events will be forwarded.
 *
 * When a Consumer registers in the event router for an event type,
 * then the producer is called to add a filter for that event.
 * The consumer will only receive the notifications that exactly
 * matches the event type and the filter. If the consumer unregisters from
 * the event router, then the producer is called to remove all filters
 * registered by that consumer.
 *
 * The event type must be unique within the total event type space.
 * This allows one or more consumers to have one or more producers sending
 * the same simple events to them. For example "MAF_NBI_SESSION_ENDED".
 *
 * Event notifications are sent in a separate thread which will call doneWithValue
 * on the producer when all consumers have been notified. For this reason the producer
 * must able to handle outstanding events, from the unregistration time until the producer is stopped
 * by MAF. That is, doneWithValue may be called after a consumer has been unregistered and
 * the filters have been removed from the producer.
 */
typedef struct MafOamSpiEventProducer_1 {

    /**
     * addFilter is called by the event router whenever the producer needs
     * to be updated about what events are subscribed for.
     *
     * @param consumerId [in] identifier of the consumer
     *
     * @param eventType [in] the type of event.
     *
     * @param filter [in] the filter specifies in more detail what it is the
     *        consumer will be notified for. filter is a null terminated array and must always
     *        have a value even if it contains an empty array. The
     *        consumer must ensure that the instance of the filter
     *        exists during the time that the consumer is
     *        subscribing for the filtered events. See also above about Event Filtering.
     *
     * @return MafOk, or @n
     * MafInvalidArgument if a parameter is not valid, or @n
     * MafFailure if an internal error occurred.
     */
    MafReturnT (*addFilter)(MafOamSpiEventConsumerHandleT consumerId, const char * eventType, MafNameValuePairT ** filter);

    /**
     * removeFilter is called by the event service whenever the producer
     * needs to be updated about events that are no longer subscribed for
     *
     * @param consumerId [in] consumerId identifier of the consumer
     *
     * @param eventType [in] the type of event
     *
     * @param filter [in] the filter instance that was previously added.
     *
     * @return MafOk, or @n
     * MafInvalidArgument if a parameter is not valid, or @n
     * MafFailure if an internal error occurred.
     */
    MafReturnT (*removeFilter)(MafOamSpiEventConsumerHandleT consumerId, const char * eventType, MafNameValuePairT ** filter);

    /**
     * clearAll can be called by the Event Router to restart from a known state.
     * When called the producer will discard all filters.
     *
     * @return MafOk, or @n
     * MafInvalidArgument if a parameter is not valid, or @n
     * MafFailure if an internal error occurred.
     */
    MafReturnT (*clearAll)();

    /**
     * doneWithValue, Event router calls this function to notify the Producer that the
     * the value is not needed anymore. The producer will know how to delete the value.
     *
     * @param eventType [in] the type of event. It will help the producer how to cast the
     *                  value before deleting it.
     *
     * @param value [in] the value that is no longer needed.
     *
     * @return MafOk, or @n
     * MafInvalidArgument if a parameter is not valid, or @n
     * MafFailure if an internal error occurred.
     */
    MafReturnT (*doneWithValue)(const char * eventType, void * value);

} MafOamSpiEventProducer_1T;

/**
 * Event consumer interface.
 */
typedef  struct MafOamSpiEventConsumer_1 {

    /**
     * When a producer sends a notification for an event type, this function
     * is called on all registered subscribers that want this type of notification,
     * and that matches their filters. (Note that the filtering is done by the
     * producer).
     *
     * If the filters for consumers of this notification type is an null
     * terminated array, then all these consumers will be notified.
     * The value can only be expected to exist during the call, thus it must
     * be copied if kept.
     *
     * Notify function is called from the event router in a separate thread.
     * The information is just transferred to this call.
     *
     * @param handle [in] identifier of the consumer
     *
     * @param eventType [in] the type of notification is a unique string.
     *
     * @param filter [in] filter registered by the consumer
     *
     * @param value [in] The value that the is associated with the event, e.g. A DN or a MOC
     * The Subscriber must cast the value to the correct type as defined for this unique type.
     *
     * @return MafOk, or @n
     * MafFailure if an internal error occurred during the notification.
     */
    MafReturnT (*notify)(MafOamSpiEventConsumerHandleT handle, const char * eventType, MafNameValuePairT **filter, void * value);

} MafOamSpiEventConsumer_1T;


/**
 * Event router interface.
 *
 * The event router mediates event notifications asynchronously from one producer to
 * one or more consumers over the EventProducer and EventConsumer SPI.
 *
 * The Event Router can only handle notifications within the MAF process and
 * is optimized for this.
 *
 * The decision to send a notification is done by the producer. All filtering
 * for subscriber notifications are done by the producers. So in the general case:
 * If there are no consumers there will be no notifications sent by the producers.
 *
 * Allocated memory for events values is owned and managed by the producer.
 */
typedef struct MafOamSpiEventRouter_1 {
    /**
     * common interface description.
     * The "base class" for this interface contains the
     * component name, interface name, and interface version.
     */
    MafMgmtSpiInterface_1T base;

    /**
     * notify function called by the producer when an event has happened.
     *
     * @param producerId [in] The identifier of the producer that sends the notification.
     *
     * @param consumerId [in] The identifier of the consumer that will receive the notification.
     *
     * @param eventType [in] defines the type of notification e.g. TransactionCommitted, XXX_WRITE_OP
     *                 etc as defined by the producer include file (SPI if it is a public event)
     *                 "CM_2_1_MoAttributeSet". It is used by the router to send the event to
     *                 the correct consumers.
     *
     * @param filter [in] is a pointer to the filter instance used by the producer. This must be the same
     * pointer that was provided by the consumer when calling addSubscription
     *
     * @param value [in] defined by the producer, e.g. The MoClass, Dn, or whatever is relevant.
     * The type of value is defined in the producer include or SPI file.
     * The producer must keep this value until the Event Service has called the
     * EventProducer::doneWithValue().
     *
     * @return MafOk, or @n
     * MafInvalidArgument if producerId is not registered or a parameter is null, or @n
     * MafFailure if an internal error occurred.
     **/

    MafReturnT (*notify)(MafOamSpiEventProducerHandleT producerId,
                         MafOamSpiEventConsumerHandleT consumerId,
                         const char * eventType,
                         MafNameValuePairT **filter,
                         void * value);


    /**
     * registerConsumer function called by the consumers to register their callbacks.
     *
     * @param interface [in] the interface to call back on when an event. It must be available
     *		  until the consumer decides to unregister itself.
     *
     * @param handle [out] At success, an identifier for this consumer, is stored at the location referenced by handle.
     *
     * @return MafOk, or @n
     * MafInvalidArgument if a parameter is null, or @n
     * MafFailure if an internal error occurred.
     */
    MafReturnT (*registerConsumer)(MafOamSpiEventConsumer_1T * interface,
                                   MafOamSpiEventConsumerHandleT * handle);

    /**
     * Add a new subscription. This function is called by the consumers. It will forward the
     * eventType and the filter to producers of the eventType. A consumer can register a
     * subscription several times with different event types or filter.
     *
     * @param handle [in] identifier of the consumer. This value is returned when the consumer
     *		  registers its interface on the eventRouter.
     *
     * @param eventType [in] the type that the subscriber is interested in.
     *        E.g. It could be CM_ROUTER_WRITE_OP, for MO operations.
     *        It must be a unique string.
     *
     * @param filter [in] A null terminated array.
     *        The producer will use the filter to decide which events to send.
     *        The producer will specify how these filters are constructed.
     *        E.g. It could be a regular expression that can be matched
     *        to a DN or a MOC. The filter must be kept by the consumer
     *        until it unregisters itself.
     *        The consumer is allowed to register at any point in time between the call of
     *        start and stop of its component. The filter must always have a value
     *        even if it contains an empty array. The filter
     *        instance must exist until the consumer has
     *        unregistered itself or removed the subscription for
     *        the event.
     *
     * @return MafOk, or @n
     * MafInvalidArgument if handle is not registered or a parameter is null, or @n
     * MafFailure if subscription could not be added.
     */
    MafReturnT (*addSubscription) (MafOamSpiEventConsumerHandleT handle, const char * eventType, MafNameValuePairT **filter);

    /**
     * Remove a subscription that it was registered in the event Router. This function is called by
     * the consumers. It will remove the eventType and the filter forwarded to producers of the eventType.
     *
     * @param handle [in] identifier of the consumer. This value is returned when the consumer
     *					  registers its interface on the eventRouter.
     *
     * @param eventType [in] the same event type that was used by the addSubscription call.
     *
     * @param filter [in] the same filter instance taht was used by the addSubscription call.
     *
     * @return MafOk, or @n
     * MafInvalidArgument if handle is not registered or a parameter is null, or @n
     * MafFailure if subscription could not be removed.
     */
    MafReturnT (*removeSubscription) (MafOamSpiEventConsumerHandleT handle, const char * eventType, MafNameValuePairT **filter);


    /**
     * unregisterConsumer function called by the consumers to unregister their callbacks.
     *
     * @param handle [in] identifier of the consumer. This value is returned when the consumer
     *					  registers its interface in the eventRouter.
     *
     * @param interface [in] the same interface instance that was used by the registerConsumer call.
     *
     * @return MafOk, or @n
     * MafInvalidArgument if handle is not registered or interface is null.
     */

    MafReturnT (*unregisterConsumer)(MafOamSpiEventConsumerHandleT handle,
                                     MafOamSpiEventConsumer_1T * interface);

    /**
     * registerProducer function called by the producer to register their callbacks.
     *
     * @param interface [in] The EventService will call the instance of the interface associated with the type
     *        that a subscriber is interested in.
     *        It's recommended that the producer registers when the component is started.
     *        However, there is nothing hindering the registering or unregistering at
     *        any time between the call of the start and stop of the producer component.
     *        An interface can be registered several times with different eventTypes. The interface must be available
     *        until the producer decides to unregister itself.
     *
     * @param handle [out] At success, an identifier for this producer, is stored at the location referenced by handle.
     *
     * @return MafOk, or @n
     * MafInvalidArgument if a parameter is null, or @n
     * MafFailure if producer could not be registered.
     */
    MafReturnT (*registerProducer)(MafOamSpiEventProducer_1T * interface, MafOamSpiEventProducerHandleT * handle);

    /**
     * Register a new event type that will be produced by the producer.
     *
     * @param handle [in] identifier of the producer. This value is returned when the producer
     *					  register its interface in the Event Router.
     *
     * @param eventType [in] the type of event that will be producer.
     *
     * @return MafOk, or @n
     * MafInvalidArgument if handle is not registered or eventType is null, or @n
     * MafFailure if producer event could not be added.
     */
    MafReturnT (*addProducerEvent) (MafOamSpiEventProducerHandleT handle, const char * eventType);

    /**
     * Remove a event type registered in the Event Router by the producer.
     *
     * @param handle [in] identifier of the producer. This value is returned when the producer
     *					  register its interface in the Event Router.
     *
     * @param eventType [in] the same type of event that was used by the addProducerEvent call.
     *
     * @return MafOk, or @n
     * MafInvalidArgument if handle is not registered or eventType is null, or @n
     * MafFailure if producer event could not be removed.
     */
    MafReturnT (*removeProducerEvent) (MafOamSpiEventProducerHandleT handle, const char * eventType);


    /**
      * unregisterProducer function called by the producer
      *
      * @param handle [in] identifier of the producer. This value is returned when the producer
      *					  register its interface in the Event Router.
      *
      * @param interface [in] the instance of the interface that was supplied in the registration.
      *
      * @return MafOk, or @n
      * MafInvalidArgument if handle is not registered or interface is null
      */
    MafReturnT (*unregisterProducer)(MafOamSpiEventProducerHandleT handle, MafOamSpiEventProducer_1T * interface);

} MafOamSpiEventRouter_1T;
#endif


