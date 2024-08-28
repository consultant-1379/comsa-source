#ifndef Com_Oam_Spi_Fm_Alarm_Suppress_Event_1_h_
#define Com_Oam_Spi_Fm_Alarm_Suppress_Event_1_h_


/**
 *@file
 * The ComOamSpiFmAlarmSuppressEvent is used by the SA to enable/disable alarm suppression in COM.
 * The SA is the event producer and COM the event consumer using the ComOamSpiEvent (the event router).
 * Alarm suppression is disabled by default. A disable event or a COM restart/switch
 * will disable alarm suppression.
 * To avoid registration order dependencies, the producer must always send the event with the
 * current alarm suppression:
 *  - to all existing consumers when the producer is registered
 *  - to new consumers registered
 * No filtering is supported. When starting a subscription, an empty filter must be provided;
 * @b Example:
 * @code
 *
 * static ComNameValuePairT* emptyFilter[] = {NULL};
 * eventService->registerConsumer(&myConsumerIf,
 *                                ComOamSpiFmAlarmSuppressEventType_1,
 *                                emptyFilter);
 * @endcode
 *
 */


#define ComOamSpiFmAlarmSuppressEventType_1 "ComOamSpiFmAlarmSuppressEventType_1"


typedef enum ComOamSpiFmAlarmSuppressType_1 {
    ComOamSpiFmAlarmSuppressType_1_OFF = 0, /* disable */
    ComOamSpiFmAlarmSuppressType_1_ON = 1   /* enable  */
} ComOamSpiFmAlarmSuppressType_1T;

/**
 * Data structure for the event value.
 */
typedef struct ComOamSpiFmAlarmSuppressEventValue_1 {

    /**
    * Specifies if alarm suppression shall be enabled or disabled
    */
    ComOamSpiFmAlarmSuppressType_1T value;


} ComOamSpiFmAlarmSuppressEventValue_1T;


#endif /* Com_Oam_Spi_Fm_Alarm_Suppress_Event_1_h_ */
