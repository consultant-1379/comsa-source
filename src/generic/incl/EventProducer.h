#ifndef ComEventProducer__
#define ComEventProducer__

#include <stdlib.h>
#include <MafOamSpiEvent_1.h>
#include <MafMgmtSpiInterfacePortal_3.h>
#include <string>
#include <stdlib.h>
#include <pthread.h>
#include <list>
#include <assert.h>
#include <MafOamSpiServiceIdentities_1.h>
#include <MafMgmtSpiCommon.h>
#include <algorithm>
#include "ComSA.h"
#include "trace.h"

namespace Com
{

/**
 * base class for a filter used for matching an event
 */
template <class EventT>
class FilterMatcher
{
public:

    /**
     * ctor
     */
    FilterMatcher(MafNameValuePairT** filters) :  _filters(filters) {/*DEBUG("FilterMatcher::FilterMatcher(): constructor called");*/}
    virtual ~FilterMatcher() {}

    /**
     * returns true if the event matches the filter
     */
    virtual bool isMatch(const EventT& filter) const = 0;

    /**
     * returns true if all the filters in the filter array are valid
     */
    virtual bool isValid(void) const = 0;

    /**
     *  returns the filter as represented in MAF
     */
    MafNameValuePairT** getFilter() const {
        return _filters;
    }

protected:
    MafNameValuePairT** _filters;

};

/**
 * A filter that matches everything
 */
template <class EventT>
class MatchAllFilter : public FilterMatcher<EventT>
{
public:

    MatchAllFilter(MafNameValuePairT** filters) : FilterMatcher<EventT>(filters) {}

    /**
     * match whatever
     */
    virtual bool isMatch(const EventT& filter) const {
        return true;
    }
    // check if the filters are valid
    virtual bool isValid(void) const
    {
        return true;
    }
};

namespace eventproducerInternal
{
/**
 * local private class only used by the EventProducer
 */
template <class EventT>
class Consumer
{
public:
    Consumer(MafOamSpiEventConsumerHandleT consumerHandle, const char* eventType, const FilterMatcher<EventT>* filter)
        : _consumerHandle (consumerHandle)
        , _eventType(eventType)
        , _filter(filter) {
    }

    Consumer(const Consumer& a) :
        _consumerHandle(a._consumerHandle),
        _eventType(a._eventType),
        _filter(a._filter) {
    }

    ~Consumer() {}

    bool operator==(const Consumer& a) const {
        return _consumerHandle == a._consumerHandle && _eventType == a._eventType;
    }

    bool operator!=(const Consumer& a) const {
        return !operator==(a);
    }

    const std::string& getEventType() const {
        return _eventType;
    }

    const MafOamSpiEventConsumerHandleT& getConsumerHandle() const {
        return _consumerHandle;
    }


    const FilterMatcher<EventT>* getFilterMatcher() const {
        return _filter;
    }

private:

    MafOamSpiEventConsumerHandleT _consumerHandle;
    std::string _eventType;
    const FilterMatcher<EventT>* _filter;

};
}


/**
 * a reference counting object.
 */
class RefCounter
{
public:
    RefCounter() : _refCounter(1) {
        if(pthread_mutex_init(&_counter_mutex, NULL) != 0)
        {
          DEBUG_OAMSA_CMEVENT("Refcounter init failed");
        }
    }
    virtual ~RefCounter() {
        if (pthread_mutex_destroy(&_counter_mutex) != 0)
        {
          DEBUG_OAMSA_CMEVENT("Failed destroying mutex!");
        }
    }

    /** Increments the object's reference count.
     */
    void addRef()  {
        pthread_mutex_lock(&_counter_mutex);
        ++_refCounter;
        pthread_mutex_unlock(&_counter_mutex);
    };

    /** Decrements the object's reference count
     * and deletes the object if the count reaches zero.
     */
    void releaseRef()
    {
        DEBUG_OAMSA_CMEVENT("RefCounter::releaseRef(): _refCounter(%d)",_refCounter);
        pthread_mutex_lock(&_counter_mutex);
        if (--_refCounter == 0)
        {
            DEBUG_OAMSA_CMEVENT("RefCounter::releaseRef(): _refCounter(%d) equal zero, deleting this object",_refCounter);
            pthread_mutex_unlock(&_counter_mutex);
            delete this;
            return ;
        }
        pthread_mutex_unlock(&_counter_mutex);

    }

private:

    int _refCounter;
    pthread_mutex_t _counter_mutex;
};



/**
 * Helper class for implementing an event producer (it wraps the event router SPI).
 *
 *
 * The template argument EventT is a class representing the event.
 * This must subclass the event struct (which is template argument EventStructT) and it needs
 * to implement methods for reference counting (which may be done by inheriting the RefCounter class above).
 * The object will delete itself when all consumers has processed the notification.
 * Example :
 *  class FmNotification : public MafOamSpiNotificationFmStruct_2T, public Com::RefCounter
    {
    public:

        FmNotification(const Alarm& alarm) {}; // Initiate the MafOamSpiNotificationFmStruct_2T

        virtual ~FmNotification() {};
    };

    The EventProducer is defined like this (example) :

    EventProducer<FmNotification, MafOamSpiNotificationFmStruct_2T> _myPusher;

    The usage is simple, there are only three methods: start(), stop() and push()

    The third Template argument is a filter evaluator. It default to an evaluator that matches
    everything.
    Example:
    EventProducer<FmNotification, MafOamSpiNotificationFmStruct_2T, MyFilterMatcher> _myPusher;
    The class MatchAllFilter must have a constructor taking a MAF list of Name value pair.
    To see how a specific filter (for instance MyFilterMatcher) is implemented, see class MatchAllFilter.
 */
template <class EventT, typename EventStructT, class FilterT = MatchAllFilter<EventT>  >
class EventProducer
{
public:

    typedef eventproducerInternal::Consumer<EventT> ConsumerT;
    typedef std::list<ConsumerT> ConsumerListT;

    /**
     * constructor
     */
    EventProducer()  {
        _eventRouterIf = 0;
        _producerHandle = 0;
        _producerIf.addFilter      = evtProdAddFilter;
        _producerIf.removeFilter  = evtProdRemoveFilter;
        _producerIf.doneWithValue = evtProdDoneWithValue;
        _producerIf.clearAll      = evtProdClearAll;
        if(_instance != NULL)
        {
            WARN_OAMSA_CMEVENT("EventProducer::EventProducer(): _instance != NULL");
        }
        _instance = this;
    }

    /**
     * destructor
     */
    ~EventProducer() {
        _instance = NULL;
    }

    /**
     * starts the event pusher
     * @param portal a handle to the portal
     * @param eventType the name of the event type
     */
    template <typename PortalT>
    void start(PortalT& portal, const char* eventType)
    {
        ENTER_OAMSA_CMEVENT();
        if(pthread_mutex_init(&_mutex, NULL) != 0)
        {
            DEBUG_OAMSA_CMEVENT("EventProducer::start(): pthread_mutex_init failed");
        }
        _eventRouterIf = getEventRouter(portal);
        _eventType = eventType;

        DEBUG_OAMSA_CMEVENT("EventProducer::start(): registerProducer");
        if (_eventRouterIf->registerProducer(&_producerIf, &_producerHandle) != MafOk)
        {
            ERR_OAMSA_CMEVENT("Could not register event producer");
            ::abort();
        }

        DEBUG_OAMSA_CMEVENT("EventProducer::start(): addProducerEvent: (%s)",eventType);
        if (_eventRouterIf->addProducerEvent( _producerHandle, eventType) != MafOk) {
            ERR_OAMSA_CMEVENT("Could not register event producer event");
            ::abort();
        }
        LEAVE_OAMSA_CMEVENT();

    }

    /**
     * stop the event pusher
     */
    void stop() {
        ENTER_OAMSA_CMEVENT();
        pthread_mutex_lock(&_mutex);
        if(_eventRouterIf) {
            _eventRouterIf->removeProducerEvent(_producerHandle, _eventType.c_str());
            _eventRouterIf->unregisterProducer( _producerHandle, &_producerIf);
        }
        _consumers.clear();
        _eventRouterIf = NULL;
        pthread_mutex_unlock(&_mutex);
        LEAVE_OAMSA_CMEVENT();
    }


    /**
     * pushes one event.
     * @event the event to push
     */
    void push(EventT& event) {
        ENTER_OAMSA_CMEVENT();
        DEBUG_OAMSA_CMEVENT("EventProducer::push(): ENTER");
        std::list<ConsumerT> matches;
        pthread_mutex_lock(&_mutex);
        for (typename ConsumerListT::iterator i = _consumers.begin(); i != _consumers.end(); ++i)
        {
            ConsumerT f(*i);
            const FilterMatcher<EventT>* filter = f.getFilterMatcher();
            if (filter->isMatch(event))
            {
               matches.push_back(f);
            }
        }
        for (typename ConsumerListT::iterator i = matches.begin(); i != matches.end(); ++i)
        {
            event.addRef();
            DEBUG_OAMSA_CMEVENT("EventProducer::push(): Sending notification now");
            const FilterMatcher<EventT>* filter = i->getFilterMatcher();
            _eventRouterIf->notify(_producerHandle, i->getConsumerHandle(), _eventType.c_str(), filter->getFilter(), dynamic_cast<EventStructT*>(&event));
        }
        pthread_mutex_unlock(&_mutex);
        DEBUG_OAMSA_CMEVENT("EventProducer::push(): LEAVE");
        LEAVE_OAMSA_CMEVENT();
    }


private:

    EventProducer(const EventProducer&);
    EventProducer& operator=(const EventProducer&);

    /*
     * lookup the event router SPI
     */
    MafOamSpiEventRouter_1T* getEventRouter(MafMgmtSpiInterfacePortal_3T& portal) {
        ENTER_OAMSA_CMEVENT();
        DEBUG_OAMSA_CMEVENT("EventProducer::getEventRouter(): ENTER");
        MafOamSpiEventRouter_1T* eventRouterIf = NULL;
        if (portal.getInterface(MafOamSpiEventService_1Id, (MafMgmtSpiInterface_1T**)&eventRouterIf) != MafOk)
        {
            ERR_OAMSA_CMEVENT("getEventRouter(): Could not lookup event service");
            ::abort();
        }
        DEBUG_OAMSA_CMEVENT("EventProducer::getEventRouter(): LEAVE");
        LEAVE_OAMSA_CMEVENT();
        return eventRouterIf;
    }

    void onClearAll()
    {
         pthread_mutex_lock(&_mutex);
        _consumers.clear();
        pthread_mutex_unlock(&_mutex);
    }

    void onDoneWithValue(const char* eventType, void* voidValue)
    {
        ENTER_OAMSA_CMEVENT();
        DEBUG_OAMSA_CMEVENT("Event type %s", eventType);
        EventStructT* structValue = reinterpret_cast<EventStructT*>(voidValue);
        EventT* event = static_cast<EventT*>(structValue);
        event->releaseRef();
        LEAVE_OAMSA_CMEVENT();
    }

    bool onAddFilter(MafOamSpiEventConsumerHandleT consumerHandle, const char * eventType,	MafNameValuePairT ** aFilter)
    {
        ENTER_OAMSA_CMEVENT();
        FilterT* filter = new FilterT(aFilter); // Deleted in onRemoveFilter

        if(filter->isValid())
        {
            ConsumerT f(consumerHandle, eventType, filter);
            pthread_mutex_lock(&_mutex);
            _consumers.push_back(f);
            pthread_mutex_unlock(&_mutex);
            LEAVE_OAMSA_CMEVENT();
            return true;
        }
        else
        {
            delete filter;
            LEAVE_OAMSA_CMEVENT();
            return false;
        }
    }

    void onRemoveFilter(
        MafOamSpiEventConsumerHandleT consumerHandle,
        const char * eventType,
        MafNameValuePairT ** filters) {
        ENTER_OAMSA_CMEVENT();
        pthread_mutex_lock(&_mutex);

        ConsumerT old_filter( consumerHandle, eventType, NULL);
        typename ConsumerListT::iterator iter = find(_consumers.begin(), _consumers.end(), old_filter);

        if( _consumers.end() != iter ) {
            const FilterMatcher<EventT>* filter = iter->getFilterMatcher();
            delete filter;
            _consumers.erase( iter );
        }
        pthread_mutex_unlock(&_mutex);
        LEAVE_OAMSA_CMEVENT();
    }

    static MafReturnT  evtProdAddFilter(
        MafOamSpiEventConsumerHandleT consumerHandle,
        const char * eventType,
        MafNameValuePairT ** filter) {
        ENTER_OAMSA_CMEVENT();
        DEBUG_OAMSA_CMEVENT("evtProdAddFilter(): adding consumerHandle (%lu) eventType (%s) filter (%p)", consumerHandle, eventType, filter);
        if(filter != NULL)
        {
            //DEBUG("evtProdAddFilter(): filter != NULL");
            for(int i=0; filter[i] != NULL; i++)
            {
              if( (filter[i]->name != NULL) && (filter[i]->value != NULL) )
                {
                  DEBUG_OAMSA_CMEVENT("      evtProdAddFilter(): filterName(%s), filterValue(%s)",filter[i]->name, filter[i]->value);
                }
                else
                {
                  if(filter[i]->name == NULL)
                   {
                     DEBUG_OAMSA_CMEVENT("      evtProdAddFilter(): filterName = NULL");
                   }
                  if(filter[i]->value == NULL)
                   {
                     DEBUG_OAMSA_CMEVENT("      evtProdAddFilter(): filterValue = NULL");
                   }
               }
             } // end of for
        }

        if (_instance->onAddFilter(consumerHandle, eventType, filter))
        {
            DEBUG_OAMSA_CMEVENT("evtProdAddFilter(): returning MafOk");
            LEAVE_OAMSA_CMEVENT();
            return MafOk;
        }
        else
        {
            DEBUG_OAMSA_CMEVENT("evtProdAddFilter(): returning MafFailure");
            LEAVE_OAMSA_CMEVENT();
            return MafFailure;
        }
    }


    static MafReturnT evtProdRemoveFilter(
           MafOamSpiEventConsumerHandleT consumerHandle,
           const char * eventType,
           MafNameValuePairT ** filter)
    {
        ENTER_OAMSA_CMEVENT();
        DEBUG_OAMSA_CMEVENT("evtProdRemoveFilter(): remove consumerHandle (%lu) eventType (%s) filter (%p)", consumerHandle, eventType, filter);
        if (_instance) {
           _instance->onRemoveFilter(consumerHandle, eventType, filter);
         }
         LEAVE_OAMSA_CMEVENT();
         return MafOk;
    }

    static MafReturnT evtProdClearAll() {
        ENTER_OAMSA_CMEVENT();
        DEBUG_OAMSA_CMEVENT("evtProdClearAll(): clear all called");
        if (_instance)
        {
         _instance->onClearAll();
        }
        LEAVE_OAMSA_CMEVENT();
        return MafOk;
    }

    static MafReturnT evtProdDoneWithValue(const char* eventType, void* value)
    {
        ENTER_OAMSA_CMEVENT();
        DEBUG_OAMSA_CMEVENT("evtProdDoneWithValue(): eventType(%s), value(%p)", eventType, value);
        if (_instance)
        {
           _instance->onDoneWithValue(eventType, value);
        }
        LEAVE_OAMSA_CMEVENT();
        return MafOk;
    }

    static EventProducer* _instance;

    MafOamSpiEventRouter_1T* _eventRouterIf;
    MafOamSpiEventProducer_1T _producerIf;
    MafOamSpiEventProducerHandleT  _producerHandle;

    pthread_mutex_t _mutex;

    std::list<ConsumerT> _consumers;
    std::string _eventType;
};  // end of class EventProducer

/**
 * the instance (only one instance per EventT/EventStructT can exist)
 */
template <class EventT, typename EventStructT, class FilterT>
EventProducer<EventT, EventStructT, FilterT>* EventProducer<EventT, EventStructT, FilterT>::_instance = 0;

}

#endif
