/* ***************************************************************************
* Copyright (C) 2011 by Ericsson AB
* S - 125 26  STOCKHOLM
* SWEDEN, tel int + 46 10 719 0000
*
* The copyright to the computer program herein is the property of
* Ericsson AB. The program may be used and/or copied only with the
* written permission from Ericsson AB, or in accordance with the terms
* and conditions stipulated in the agreement/contract under which the
* program has been supplied.
*
* All rights reserved.
*
* File: PmEventHandler.hxx
*
* Author: ejonajo 2011-09-01
*
* Reviewed: efaiami 2012-04-14
* Modified: xnikvap 2013-12-15 Converted to use COM PM SPI Ver.2
*
************************************************************************** */

#ifndef PMEVENTHANDLER_HXX_
#define PMEVENTHANDLER_HXX_

#include "PmConsumer.hxx"
#include <ComOamSpiEvent_1.h>
#include "ComOamSpiPm_2.h"
#include "ComOamSpiPm_2_1.h"

#include <vector>
#include <map>
#include <pthread.h>

/**
* @file PmEventHandler.hxx
*
* @ingroup PmtSa
*
* @brief This class handles communication between COM and PMTSA.
*
* This file holds the interface of the PM Event Services that PMTSA
* provides for COM and also defines the Event Producer interfaces.
*/

namespace PmtSa {
/**
* Forward declaration of PmComComponent so that we don't need to
* include the header file in here.
*/
	class PmComComponent;

/**
* @ingroup PmtSa
*
* @brief This class holds the PM & event producing interfaces.
*
* This class is responsible for handling subscribers of PM events,
* to notify them when events happens. The class is also responsible
* for fetching PM-data and deleting the PM-data when COM is finished
* with it.
*/
	class PmEventHandler {
	public:
	/**
	* @ingroup PmtSa
	*
	* Constructor
	*/
	PmEventHandler();

	/**
	* @ingroup PmtSa
	*
	* Destructor
	*/
	virtual ~PmEventHandler();

	/**
	* @ingroup PmtSa
	*
	* Returns a pointer to the singleton instance of this class.
	*
	* @return 	PmEventHandler*     A pointer to the singleton instance.
	*
	*/
	static PmEventHandler* instance();

	/**
	* @ingroup PmtSa
	*
	* Starts up the services from this object, ie the PM-events and
	* transport mechanism for the PM-data
	*
	* @param[in] 	pmComComponent*  Pointer to starting object
	*
	* @return 	ComReturnT  ComOk on successful startup
	*
	*/
	ComReturnT start(PmComComponent* pmComComponent);

	/**
	* @ingroup PmtSa
	*
	* Stops the service from this object.
	*
	* @return 	ComReturnT  ComOk on successful stop
	*
	*/
	ComReturnT stop();

	// ---------------------------------------------------------
	// Required for the ComOamSpiPm interface
	// ---------------------------------------------------------

	/**
	* @ingroup PmtSa
	*
	* Retrieves the PM-data associated with the GP-id that is
	* provided and puts it in memory organized according to the
	* SPI that COM has specified.  This is a required method for
	* the ComOamSpiPm interface.  The PM-data that is put into
	* memory should be released with a call to the releaseGpData()
	* method.
	*
	* @param[in] 		pmGpId      Unique id of PM-data
	* @param[out] 		data        Pointer to PM-data
	*
	* @return 	ComReturnT  ComOk if data was found
	*
	*/
	ComReturnT getGpData(ComOamSpiPmGpId_2T pmGpId,
				ComOamSpiPmGpData_2T** data);

	/**
        * @ingroup PmtSa
        *
        * Retrieves the PM-data associated with the GP-id that is
        * provided and puts it in memory organized according to the
        * SPI that COM has specified.  This is a required method for
        * the ComOamSpiPm interface.  The PM-data that is put into
        * memory should be released with a call to the releaseGpData()
        * method.
        *
        * @param[in]            pmGpId      Unique id of PM-data
	* @param[in]            gpName      To store PM-data of respective Gp in individual GpName structure.
        * @param[out]           data        Pointer to PM-data
        *
        * @return       ComReturnT  ComOk if data was found
        *
        */
	ComReturnT getGpData2(ComOamSpiPmGpId_2T pmGpId, const char* gpName,
                                ComOamSpiPmGpData_2T** data);

	/**
	* @ingroup PmtSa
	*
	* Release memory that has been allocated with the getGpData()
	* method.  Notice that when the method is called, the data
	* will also be removed from the underlying MW. This is a
	* required method for the ComOamSpiPm interface.
	*
	* @param[in] 	pmGpId  Unique id of PM-data
	*
	* @return 	ComReturnT  ComOk if the removal went OK.
	*
	*/
	ComReturnT releaseGpData(ComOamSpiPmGpId_2T pmGpId);

	/**
        * @ingroup PmtSa
        *
        * Release memory that has been allocated with the getGpData()
        * method.  Notice that when the method is called, the data
        * will also be removed from the underlying MW. This is a
        * required method for the ComOamSpiPm interface.
        *
        * @param[in]    pmGpId  Unique id of PM-data
	* @param[in]    gpName  To remove PM-data of respective Gp in individual GpName structure.
        *
        * @return       ComReturnT  ComOk if the removal went OK.
        *
        */
	ComReturnT releaseGpData2(ComOamSpiPmGpId_2T pmGpId, const char* gpName);

	// ---------------------------------------------------------
	// This is required for the ComOamSpiEventProducer - methods
	// ---------------------------------------------------------

	/**
	* @ingroup PmtSa
	*
	* This method is part of the ComOamSpiEventProducer interface.
	* The method is called by the EventRouter (COM) if there are
	* other objects interested in getting PM-events that we produce.
	*
	* An important note: if there are no subscribers of PM-events,
	* the PM-service will NOT aggregate data in the system. Thus at
	* least one subscriber must exist in COM if PM is supposed to
	* run.
	*
	* @param[in] 		consumerHandle  Event-consumer
	* @param[in] 		eventType       The type of events the consumer wants
	* @param[in,out] 	filters         Selection of events
	*
	* @return 	ComReturnT  ComOk if we can provide service
	*
	*/
	ComReturnT addFilter(ComOamSpiEventConsumerHandleT consumerHandle,
				const char* eventType,
				ComNameValuePairT** filters);

	/**
	* @ingroup PmtSa
	*
	* This method is part of the ComOamSpiEventProducer interface.
	* The method is called by the EventRouter (COM) when a consumer
	* isn't interested in PM-events any more.
	*
	* An important note: if there are no subscribers of PM-events,
	* the PM-service will NOT aggregate data in the system. Thus at
	* least one subscriber must exist in COM if PM is supposed to
	* run.  When the last subscriber is removed, the PM-services in
	* the middleware stops aggregation.
	*
	* @param[in]       consumerHandle  Event-consumer
	* @param[in]       eventType       The type of events
	* @param[in,out]   filters         Selection of events
	*
	* @return  ComReturnT  ComOk if we removed subscriber
	*
	*/
	ComReturnT removeFilter(ComOamSpiEventConsumerHandleT consumerHandle,
				const char* eventType,
				ComNameValuePairT** filters);

	/**
	* @ingroup PmtSa
	*
	* This method is part of the ComOamSpiEventProducer interface.
	* Once the subscriber of an event is done, the subscriber should
	* tell the producer of the event.  That is done with this method.
	*
	* @param[in] 	eventType   The type of event
	* @param[in] 	value       The "locally" allocated event value that should be free'd
	*
	* @return 	ComReturnT      ComOk if we could free memory
	*
	*/
	ComReturnT doneWithValue(const char* eventType, void* value);

	/**
	* @ingroup PmtSa
	*
	* This method is part of the ComOamSpiEventProducer interface.
	* It removes all subscribers of PM-events, and stops the PM-service.
	*
	* @return  ComReturnT      ComOk When all filters where cleared up
	*/
	ComReturnT clearAll();

	// ---------------------------------------------------------
	// End: ComOamSpiEventProducer - methods
	// ---------------------------------------------------------

	/**
	* @ingroup PmtSa
	*
	* This method creates the PM-event and sends it to COM's EventRouter
	* when the PM-service in the middle-ware has finished a granularity
	* period.
	*
	* @param[in]   pmCbData A pointer to a PM-job
	*
	* @return  ComReturnT  ComOk if we managed to store the job-data and send the event.
	*/
	ComReturnT sendPmDataReady(SaPmCCallbackInfoT* pmCbData);

	/**
        * @ingroup PmtSa
        *
        * This method identifies granuality period  information from gpStartTimestamp and gpEndTimestamp
        *
        * @param[in]  gpStartTime in nanoseconds
	* @param[in]  gpEndTime in nanoseconds
        *
        * @return  std::string  returns the granuality period in string.
        */
	std::string getGpStartEndTime(uint64_t gpStart,uint64_t gpEnd);

	/**
        * @ingroup PmtSa
        *
        * This method clears the finishedGp structure that carries event information.
        *
        * @param[in] pmGpId  Unique id of PM-data
        *
        * @return  ComReturnT  ComOk if we could remove the data in the structure
        */
	ComReturnT removefinishedGp(ComOamSpiPmGpId_2T pmGpId);

	/**
        * @ingroup PmtSa
        *
        * This method clears the finishedGp2 structure that carries event information.
        *
        * @param[in] pmGpId  Unique id of PM-data
        *
        * @return  ComReturnT  ComOk if we could remove the data in the structure
        */
	ComReturnT removefinishedGp2(ComOamSpiPmGpId_2T pmGpId);

	private:
		// Pointer to self
		static PmEventHandler* s_instance;

		/**
		* Pointer to event router interface
		*/
		ComOamSpiEventRouter_1T* m_eventRouter;

		/**
		* Interface registered for the producer
		*/
		ComOamSpiEventProducer_1T m_eventProducerInterface;

		/**
		* Handle to the producer event services
		*/
		ComOamSpiEventProducerHandleT m_eventProducerHandle;

		/**
		* Internal class to hold event subscribers
		*/
		class Filter
		{
		public:
			Filter(
				ComOamSpiEventConsumerHandleT consumerHandle,
				const char* eventType,
				ComNameValuePairT ** filters
			);
			Filter( const Filter& filter );

			bool operator==( const Filter& filter ) const;

			ComOamSpiEventConsumerHandleT consumerHandle;

			// Should perhaps be a std::string instead?
			const char* eventType;

			// Array of name+values
			ComNameValuePairT ** filters;
		};

		/**
		* Keep track of subscribers of PM events
		*/
		std::vector<Filter> m_pmDataReadyFilters;

		/**
		*  Keep track of "active" PM-job gps
		*/
		std::vector<SaPmCCallbackInfoT*> m_finishedGps;

		/**
                *  Keep track of "active" PM-job gps for each Gp's in individual structure
		*  inorder to process parallel gp request
                */
		std::vector<SaPmCCallbackInfoT*> m_finishedGps2[7];

		/**
		*  Hold pointer to cb-info and the result when it is being processed by COM
		*/
		SaPmCCallbackInfoT m_COMcurrentGp;

		/**
                *  Hold pointer to cb-info and the result in individual structure for each Gp
		*  when it is being processed by COM
                */
		SaPmCCallbackInfoT m_COMcurrentGp2[7];

		//To determine individual Gp structue based on the GpName from getGpData2 and releaseGpData2 method.
		std::map<std::string, int> gpMap;

		//To determine the Granuality period from the getGpStartEndTime method
		std::map<int, std::string> gpMapTime;

		/**
		* Pointer to PM-data, fetched via getData() method
		*/
		ComOamSpiPmGpData_2T* m_COMcurrentResult;

		/**
                * Pointer to PM-data, fetched via getData2() method
                */
		ComOamSpiPmGpData_2T* m_COMcurrentResult2[7];

		/**
		* Since we might be accessed by several different threads at the
		* same time, we must protect our filter with a mutex.  We do however
		* NOT support multiple consumers at the same time.
		*/
		pthread_mutex_t m_pmDataReadyMutex;

		/**
		* The object that owns us
		*/
		PmComComponent* m_pmComComponent;
	};

} /* namespace PmtSa */

/**
* C-interface for PmtSa::PmEventHandler::addFilter()
*/
extern "C" ComReturnT pmtsa_addFilter(ComOamSpiEventConsumerHandleT consumerHandle,
					const char* eventType,
					ComNameValuePairT** filters);

/**
* C-interface for PmtSa::PmEventHandler::removeFilter()
*/
extern "C" ComReturnT pmtsa_removeFilter(ComOamSpiEventConsumerHandleT consumerHandle,
					const char* eventType,
					ComNameValuePairT** filters);

/**
* C-interface for PmtSa::PmEventHandler::doneWithValue()
*/
extern "C" ComReturnT pmtsa_doneWithValue(const char* eventType, void* value);

/**
* C-interface for PmtSa::PmEventHandler::clearAll()
*/
extern "C" ComReturnT pmtsa_clearAll();

/**
* C-interface for PmtSa::PmEventHandler::getGpData()
*/
extern "C" ComReturnT pmtsa_getGpData(ComOamSpiPmGpId_2T pmGpId,
					ComOamSpiPmGpData_2T** data);

/**
* C-interface for PmtSa::PmEventHandler::getGpData2()
*/
extern "C" ComReturnT pmtsa_getGpData_2(ComOamSpiPmGpId_2T pmGpId, const char* gpName,
                                        ComOamSpiPmGpData_2T** data);

/**
* C-interface for PmtSa::PmEventHandler::releaseGpData()
*/
extern "C" ComReturnT pmtsa_releaseGpData(ComOamSpiPmGpId_2T pmGpId);

/**
* C-interface for PmtSa::PmEventHandler::releaseGpData2()
*/
extern "C" ComReturnT pmtsa_releaseGpData_2(ComOamSpiPmGpId_2T pmGpId, const char* gpName);


#endif /* PMEVENTHANDLER_HXX_ */
