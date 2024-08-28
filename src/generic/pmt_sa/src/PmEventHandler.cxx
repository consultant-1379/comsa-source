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
 * File: PmEventHandler.cxx
 *
 * Author: ejonajo 2011-09-01
 *
 * Reviewed: efaiami 2012-04-14
 * Modified: xnikvap 2013-12-15 Converted to use COM PM SPI Ver.2
 *
 ************************************************************************** */
#include "PmEventHandler.hxx"
#include "PmComComponent.hxx"
#include "PmInternalMsg.hxx"
#include "PmRunnable.hxx"
#include "PmConsumerInterface.hxx"
#include "PmtSaTrace.hxx"

#include <cassert>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <algorithm>

#define MAX_NO_OF_GPS 7

/**
 * @file PmEventHandler.cxx
 *
 * @brief This is the implementation of the PmEventHandler interface
 *
 * This class handles the communication to and from COM via the event producer
 * interface, and the performance management interface.  Basically, when PMT-SA
 * starts up, we tell COMs event-router that we publish PM-events.  We also say
 * that we implement the PM-interfaces, so COM should say that they want to
 * subscribe to the PM-events.  Once they do that, we will tell our thread
 * (@see PmRunnable::startSubscription()) to start subscribing to PM-events from
 * the OpenSAF PM-service.
 * When we get a signal from the PM-service that there's data available, we will
 * tell COM that "here's a particular GP-id that you can fetch if you want!".
 * COM must then explicitly ask for that GP-id via the PM-interface method,
 * and also remove it via the releaseGpData() method.
 */

using namespace PmtSa;

PmEventHandler* PmEventHandler::s_instance = NULL;

PmEventHandler::PmEventHandler()
{
	ENTER_PMTSA();
	assert(s_instance == NULL);
	s_instance = this;
	m_eventRouter = NULL;
	m_pmComComponent = NULL;
	if (pthread_mutex_init(&m_pmDataReadyMutex, NULL) != 0)
	{
		PMTSA_ERR("Failed initializing mutex!");
		assert(!"Failed initializing mutex!");
	}
	m_COMcurrentGp.jobHandler = 0;
	m_COMcurrentResult = NULL;

	//m_COMcurrentGp2 and m_COMcurrentResult2 are accessed when pmCollectionTimer feature is enabled. Initialized always as the feature enabled/disabled state is not known by SA
	for (int i = 0; i < MAX_NO_OF_GPS; i++) {
		m_COMcurrentGp2[i].jobHandler = 0;
		m_COMcurrentResult2[i] = NULL;
	}
	LEAVE_PMTSA();
}

PmEventHandler::~PmEventHandler()
{
	ENTER_PMTSA();
	if (pthread_mutex_destroy(&m_pmDataReadyMutex) != 0)
	{
		PMTSA_ERR("Failed destroying mutex!");
		assert(!"Failed destroying mutex!");
	}
	s_instance = NULL;
	LEAVE_PMTSA();
}

/**
 * @ingroup PmtSa
 *
 *  When the PMT-SA is started via a start-call to the PmComComponent-class, its start-method will
 *  call this method and initiate event-listening so that we might catch when COM is finished with
 *  the GP-data.
 *
 * @param[in] 	pmComComponent 		A pointer to the 'main' component
 *
 * @return 	ComReturnT	The method returns 'ComOk' if everything went fine.
 *
 */
ComReturnT PmEventHandler::start(PmComComponent* pmComComponent)
{
	ENTER_PMTSA();
	PMTSA_LOG("Starting event handling interfaces");

	//m_finishedGp2 cleared always as the pmCollectionTimer feature enabled/disabled state is not known by SA
	for (int i = 0; i < MAX_NO_OF_GPS; i++) {
		m_finishedGps2[i].clear();
	}

	m_finishedGps.clear();

	m_pmComComponent = pmComComponent;

	//Determine the respective structure index based on GpName received at the time of pmCollectionTimer feature enabled
	gpMap.insert(std::pair<std::string, int>("ONE_MIN", 0));
        gpMap.insert(std::pair<std::string, int>("FIVE_MIN", 1));
        gpMap.insert(std::pair<std::string, int>("FIFTEEN_MIN", 2));
        gpMap.insert(std::pair<std::string, int>("THIRTY_MIN", 3));
        gpMap.insert(std::pair<std::string, int>("ONE_HOUR", 4));
        gpMap.insert(std::pair<std::string, int>("TWELVE_HOUR", 5));
        gpMap.insert(std::pair<std::string, int>("ONE_DAY", 6));

	//To Determine the GpName at the time of receiveing events from coremw
	gpMapTime.insert(std::pair<int, std::string>(60,"ONE_MIN"));
        gpMapTime.insert(std::pair<int, std::string>(300,"FIVE_MIN"));
        gpMapTime.insert(std::pair<int, std::string>(900,"FIFTEEN_MIN"));
        gpMapTime.insert(std::pair<int, std::string>(1800,"THIRTY_MIN"));
        gpMapTime.insert(std::pair<int, std::string>(3600,"ONE_HOUR"));
        gpMapTime.insert(std::pair<int, std::string>(43200,"TWELVE_HOUR"));
        gpMapTime.insert(std::pair<int, std::string>(86400,"ONE_DAY"));

	// TODO: Register this in COM, but with what interface??
	m_eventRouter = m_pmComComponent->eventRouterInterface();
	// Make sure COM knows who to call
	m_eventProducerInterface.addFilter = ::pmtsa_addFilter;
	m_eventProducerInterface.removeFilter = ::pmtsa_removeFilter;
	m_eventProducerInterface.doneWithValue = ::pmtsa_doneWithValue;
	m_eventProducerInterface.clearAll = ::pmtsa_clearAll;
	if (m_eventRouter != NULL)
	{
		// Now we must set up that we're producers too
		ComReturnT rc = m_eventRouter->registerProducer(&m_eventProducerInterface, &m_eventProducerHandle);
		if (rc != ComOk)
		{
			PMTSA_ERR("Event registerProducer FAILED");
			LEAVE_PMTSA();
			return rc;
		}
		else
		{
			PMTSA_LOG("Registered as event producer OK");
		}
		// And this is the event that we'll publish
		LEAVE_PMTSA();
		return m_eventRouter->addProducerEvent(m_eventProducerHandle, ComOamSpiPmEventTypeGpReady_2);
	}
	else
	{
		LEAVE_PMTSA();
		return ComInvalidArgument;
	}
}

/**
 * @ingroup PmtSa
 *
 *  This method is called when the compontent shuts down, we should then un-register
 *  ourselves as receivers of events, as well as producers of events.
 *
 * @return 	ComReturnT	ComOk if everything went fine, otherwise error-code
 *
 */
ComReturnT PmEventHandler::stop()
{
	ENTER_PMTSA();
	if (m_eventRouter != NULL)
	{
		PMTSA_LOG("Stopping PM event production!");

		//Iterator to release the memory that has been allocated by getGpData2()and also the data to be removed from underlying MW
		std::vector<SaPmCCallbackInfoT*>::iterator it;

		//Iterator to release the memory that has been allocated by getGpData()and also the data to be removed from underlying MW
		std::vector<SaPmCCallbackInfoT*>::iterator it2;

		try {
			// no need to send this internal message when stopping PMTSA anymore
			// Tell that we shouldn't listen for PM-events anymore
			//PmRunnable* myPmConsumerThread = PmRunnable::instance();
			//PmInternalMsg* noConsumerMessage = new PmInternalMsg(PmInternalMsg::EVENT_NO_CONSUMERS);
			//myPmConsumerThread->pushToInput(noConsumerMessage);

			pthread_mutex_lock(&m_pmDataReadyMutex);
		for (int i = 0; i < MAX_NO_OF_GPS; i++)
                {
			it = m_finishedGps2[i].begin();
			while(it != m_finishedGps2[i].end())
			{
				if ((*it)->jobHandler == m_COMcurrentGp2[i].jobHandler)
				{
					//Above condition will apply only when pmCollectionTimer feature is enabled and getGpData2() method is in processing.
					//Remove the unused feature disabled structure(m_finishedGps) to avoid next iteration it2
					removefinishedGp((*it)->jobHandler);
					PMTSA_ERR("COM orders PM to stop, but haven't cleaned up properly!");
					// The current job that has been given to COM,
					// apparently COM has forgotten to clean up...
					// We should still free up the memory, and possibly
					// make COM crash. (mail from QPEREBJ - 20120131)
					if (m_COMcurrentResult2[i] != NULL)
					{
						freeData2(m_COMcurrentResult2[i],i);
						m_COMcurrentResult2[i] = NULL;
					}
					memset(&m_COMcurrentGp2[i], 0x00, sizeof(SaPmCCallbackInfoT));
					PMTSA_ERR("COM is in a risky state now.");

					//Added the below set of code inside the pmCollectionTimer feature enabled condition.
					//Inorder to avoid removing the data from the underlying MW in both cases feature enabled or disabled
					SaPmCCallbackInfoT* pmData = (*it);
					// Ignore return value
					static_cast<void>(PmConsumerInterface::instance()->jobGranularityPeriodDataRemove(pmData->jobHandler));
					if (pmData->jobname != NULL)
					{
						free(pmData->jobname);
						pmData->jobname = NULL;
					}
					delete pmData;
				}
				//when pmCollectionTimer feature is enabled or disabled, feature enable structure(m_finishedGps2) will be cleared
				it = m_finishedGps2[i].erase(it);
			}
                 }

		 it2 = m_finishedGps.begin();
                 while(it2 != m_finishedGps.end())
                 {
                       //Above while condition will be satisfied only when pmCollectionTimer feature is disabled
		       //And also condition will be satisfied when pmCollectionTimer feature is enabled and getGpData2() method is not in processing stage.
		       //Note: when pmCollectionTimer feature is disabled, removefinishedGp method will never be called at the time of PM stop.
                       if ((*it2)->jobHandler == m_COMcurrentGp.jobHandler)
                       {
                                        PMTSA_ERR("COM orders PM to stop, but haven't cleaned up properly!");
                                        // The current job that has been given to COM,
                                        // apparently COM has forgotten to clean up...
                                        // We should still free up the memory, and possibly
                                        // make COM crash. (mail from QPEREBJ - 20120131)
                                        if (m_COMcurrentResult != NULL)
                                        {
                                                freeData(m_COMcurrentResult);
                                                m_COMcurrentResult = NULL;
                                        }
                                        memset(&m_COMcurrentGp, 0x00, sizeof(SaPmCCallbackInfoT));
                                        PMTSA_ERR("COM is in a risky state now.");
                       }
		       //when pmCollectionTimer feature is disabled, data will be removed from underlying MW
		       //when pmCollectionTimer feature is enabled and getGpData2() method is not in processing stage,data will be removed from underlying MW
                       SaPmCCallbackInfoT* pmData = (*it2);
                       // Ignore return value
                       static_cast<void>(PmConsumerInterface::instance()->jobGranularityPeriodDataRemove(pmData->jobHandler));
                       if (pmData->jobname != NULL)
                       {
                            free(pmData->jobname);
                            pmData->jobname = NULL;
                       }
                       delete pmData;
                       it2 = m_finishedGps.erase(it2);
                 }
			// Finally
			// Unregister produced event
			m_eventRouter->removeProducerEvent(m_eventProducerHandle, ComOamSpiPmEventTypeGpReady_2);
			// Unregister event producer
			m_eventRouter->unregisterProducer(m_eventProducerHandle, &m_eventProducerInterface);
			pthread_mutex_unlock(&m_pmDataReadyMutex);
		}
		catch (const std::exception& e)
		{
			pthread_mutex_unlock(&m_pmDataReadyMutex);
			PMTSA_WARN("Exception when trying to lock mutex: %s", e.what());
			LEAVE_PMTSA();
			return ComFailure;
		}
		catch (...)
		{
			pthread_mutex_unlock(&m_pmDataReadyMutex);
			PMTSA_ERR("Unknown kind of exception caught!");
			LEAVE_PMTSA();
			return ComFailure;
		}

	}
	else
	{
		PMTSA_WARN("Stop called on PM events when not active");
		LEAVE_PMTSA();
		return ComFailure;
	}
	LEAVE_PMTSA();
	return ComOk;
}

/**
 * @ingroup PmtSa
 *
 * Returns a pointer to the singleton PmEventHandler instance.
 *
 * @return 	PmEventHandler*		Pointer to the PmEventHandler or NULL
 *
 */
PmEventHandler* PmEventHandler::instance()
{
	ENTER_PMTSA();
	LEAVE_PMTSA();
	return s_instance;
}

// PM Event Producer Filter class
// -------------------------------
PmEventHandler::Filter::Filter(ComOamSpiEventConsumerHandleT consumerHandle,
		const char* eventType,
		ComNameValuePairT ** filters)
{
	ENTER_PMTSA();
	this->consumerHandle = consumerHandle;
	this->eventType = eventType;
	this->filters = filters;
	LEAVE_PMTSA();
}

PmEventHandler::Filter::Filter(const Filter& filter)
{
	ENTER_PMTSA();
	consumerHandle = filter.consumerHandle;
	eventType = filter.eventType;
	filters = filter.filters;
	LEAVE_PMTSA();
}

bool PmEventHandler::Filter::operator==(const Filter& rhs) const
		{
	ENTER_PMTSA();
	LEAVE_PMTSA();
	return ( consumerHandle == rhs.consumerHandle &&
			strcmp( eventType, rhs.eventType ) == 0 &&
			filters == rhs.filters );
		}

/**
 * @ingroup PmtSa
 *
 *  Notifies the interested consumers about the fact that we have
 *  a new GP finished with fresh and interesting JobData floating around.
 *
 * @param[in] 	pmCbData		A unique job-number for some GP-data that can be retrieved
 *
 * @return 	ComReturnT	ComOk if we sent to at least one recipient
 *
 */
ComReturnT PmEventHandler::sendPmDataReady(SaPmCCallbackInfoT* pmCbData)
{
	ENTER_PMTSA();
	ComReturnT rc = ComOk;
	if (pmCbData == NULL)
	{
		PMTSA_WARN("Pointer was NULL");
		LEAVE_PMTSA();
		return ComInvalidArgument;
	}
	try
	{
		pthread_mutex_lock(&m_pmDataReadyMutex);

		// Save all callback data
		SaPmCCallbackInfoT* copiedData = new SaPmCCallbackInfoT;
		assert(copiedData != NULL);

		copiedData->clientHdlr = pmCbData->clientHdlr;
		copiedData->jobHandler = pmCbData->jobHandler;
		if (pmCbData->jobname != NULL)
		{
			copiedData->jobname = strdup(pmCbData->jobname);
			/*
			 * Duhh, the job-name contains 'safJob=', that should be removed.
			 * The name looks like 'safJob={TheJobName}' but it
			 * should be '{TheJobName}'
			 */
			{
				/*
				 *           111111
				 * 0123456789012345
				 * safJob=
				 */
				int sjlen = strlen("safJob=");
				int i = sjlen;
				int safJobNameLen = strlen(copiedData->jobname);
				{
					while (i < safJobNameLen && copiedData->jobname[i] != '\0')
					{
						copiedData->jobname[i-sjlen] = copiedData->jobname[i];
						++i;
					}
					/*
					 * NULL-terminate that string.
					 */
					copiedData->jobname[i-sjlen] = '\0';
				}
			}
		}
		else
		{
			PMTSA_WARN("No jobname found for job-id %llu, sending: <pmJob=Faked_Faulty_JobName_From_PM>", pmCbData->jobHandler);
			copiedData->jobname = strdup("pmJob=Faked_Faulty_JobName_From_PM");
		}
		assert(copiedData->jobname != NULL);
		copiedData->gpStartTimestamp = pmCbData->gpStartTimestamp;
		copiedData->gpEndTimestamp = pmCbData->gpEndTimestamp;
		std::string fGpName="";

		// Find the GpName from gpEventStartTime and gpEventEndTime
		fGpName = getGpStartEndTime(pmCbData->gpStartTimestamp,pmCbData->gpEndTimestamp);

		// Find the respective GpName structure to push the event details to its respective structure.
		int gpIndex = gpMap.find(fGpName)->second;

		m_finishedGps.push_back(copiedData);
		//when pmCollectionTimer feature is enabled, getGpData2() method will process the below structure based on gpName.
		m_finishedGps2[gpIndex].push_back(copiedData);

		for(std::vector<Filter>::iterator it = m_pmDataReadyFilters.begin();
				it != m_pmDataReadyFilters.end() && rc == ComOk;
				++it)
		{
			if ((it->eventType != NULL) &&
					(strcmp(it->eventType, ComOamSpiPmEventTypeGpReady_2) == 0))
			{
				// Ok, here we have somebody who's interested, so send a message!
				ComOamSpiPmEventValue_2T* comEvent = new ComOamSpiPmEventValue_2T;
				assert(comEvent != NULL);
				comEvent->gpId = copiedData->jobHandler;
				comEvent->jobId = copiedData->jobname;
				//PMTSA_LOG("Sending job-notification (GP#<name>) = %ld<%s>", comEvent->gpId, comEvent->jobId);
				comEvent->gpStartTimestampInNanoSeconds = copiedData->gpStartTimestamp;
				comEvent->gpEndTimestampInNanoSeconds = copiedData->gpEndTimestamp;
				void* ourEventValue = reinterpret_cast<void*>(comEvent);
				rc = m_eventRouter->notify(m_eventProducerHandle,
						it->consumerHandle,
						ComOamSpiPmEventTypeGpReady_2,
						it->filters,
						ourEventValue);
				//				PMTSA_LOG("Notification sent to COM, return-code: %d", rc);
			}
		}
		pthread_mutex_unlock(&m_pmDataReadyMutex);
	}
	catch (const std::exception& e)
	{
		pthread_mutex_unlock(&m_pmDataReadyMutex);
		PMTSA_ERR("Caught exception: %s", e.what());
		LEAVE_PMTSA();
		return ComFailure;
	}
	catch (...)
	{
		pthread_mutex_unlock(&m_pmDataReadyMutex);
		PMTSA_ERR("Caught unknown exception!");
		LEAVE_PMTSA();
		return ComFailure;
	}
	LEAVE_PMTSA();
	return rc;
}

std::string PmEventHandler::getGpStartEndTime(uint64_t gpStart,uint64_t gpEnd)
{
	const uint64_t nanoSecondsPerSecond(1000000000);
        uint64_t gpStartTimeInSeconds =  gpStart / nanoSecondsPerSecond;
        uint64_t gpEndTimeInSeconds = gpEnd / nanoSecondsPerSecond;

	uint64_t gpPeriodInSeconds = gpEndTimeInSeconds - gpStartTimeInSeconds;

	std::string gpName="";
	gpName = gpMapTime.find(gpPeriodInSeconds)->second;

	return gpName;
}


/**
 * @ingroup PmtSa
 *
 *  The method is called by consumers that wants to subscribe to
 *  events from us.
 *
 * @param[in] 	consumerHandle		Handle to the consumer
 * @param[in]		eventType			The event that the consumer is interested in
 * @param[in]		filters				Some kind of filter for the event
 *
 * @return 	ComReturnT			ComOk if subscription succeeded
 *
 */
ComReturnT PmEventHandler::addFilter(ComOamSpiEventConsumerHandleT consumerHandle,
		const char* eventType,
		ComNameValuePairT** filters)
{
	ENTER_PMTSA();
	PMTSA_LOG("Received subscription request on PM-events");
	if ((NULL == eventType))
	{
		LEAVE_PMTSA();
		PMTSA_WARN("Event-type is NULL, invalid value");
		return ComInvalidArgument;
	}
	else
	{
		if (::strcmp(ComOamSpiPmEventTypeGpReady_2, eventType) == 0)
		{
			Filter pmdFilter( consumerHandle, eventType, filters );
			std::vector<Filter>::iterator iter;

			pthread_mutex_lock(&m_pmDataReadyMutex);
			iter = find(m_pmDataReadyFilters.begin(), m_pmDataReadyFilters.end(), pmdFilter);
			if( m_pmDataReadyFilters.end() == iter )
			{
				m_pmDataReadyFilters.push_back(pmdFilter);
			}
			else
			{
				pthread_mutex_unlock(&m_pmDataReadyMutex);
				PMTSA_WARN("PMT-SA: Subscriber already enrolled in filter-list");
				LEAVE_PMTSA();
				return ComFailure;
			}
			// Tell that we should listen for PM-events
			PmRunnable* myPmConsumerThread = PmRunnable::instance();
			PmInternalMsg* pmConsumerMessage = new PmInternalMsg(PmInternalMsg::EVENT_CONSUMER_REGISTERED);
			myPmConsumerThread->pushToInput(pmConsumerMessage);

			pthread_mutex_unlock(&m_pmDataReadyMutex);
		}
	}
	PMTSA_LOG("Registered consumer of PM-events");
	LEAVE_PMTSA();
	return ComOk;
}

/**
 * @ingroup PmtSa
 *
 *  The method is called by consumers that wants to un-subscribe to
 *  events from us. When we have no filters, we'll have no consumers
 *  of PM-events so then we must tell our Consumer thread to relax.
 *
 * @param[in] 	consumerHandle		Handle to the consumer
 * @param[in]		eventType			The event that the consumer isn't interested in anymore
 * @param[in]		filters				Some kind of filter for the event
 *
 * @return 	ComReturnT			ComOk if un-subscription succeeded
 *
 */
ComReturnT PmEventHandler::removeFilter(ComOamSpiEventConsumerHandleT consumerHandle,
		const char* eventType,
		ComNameValuePairT** filters)
{
	ENTER_PMTSA();
	PMTSA_LOG("Received un-subscription request of PM-events");
	if ((NULL == eventType))
	{
		PMTSA_WARN("Invalid event type received");
		LEAVE_PMTSA();
		return ComInvalidArgument;
	}
	else
	{
		if (strcmp(ComOamSpiPmEventTypeGpReady_2, eventType) == 0)
		{
			Filter pmdFilter( consumerHandle, eventType, filters );
			std::vector<Filter>::iterator iter;

			pthread_mutex_lock(&m_pmDataReadyMutex);
			iter = find(m_pmDataReadyFilters.begin(), m_pmDataReadyFilters.end(), pmdFilter);
			if( m_pmDataReadyFilters.end() != iter )
			{
				m_pmDataReadyFilters.erase(iter);
			}
			else
			{
				pthread_mutex_unlock(&m_pmDataReadyMutex);
				PMTSA_WARN("PMT-SA: Unknown un-subscription requester of PM-events");
				LEAVE_PMTSA();
				return ComFailure;
			}
			if (m_pmDataReadyFilters.empty() == true)
			{
				// Tell that we shouldn't listen for PM-events anymore
				PmRunnable* myPmConsumerThread = PmRunnable::instance();
				PmInternalMsg* noConsumerMessage = new PmInternalMsg(PmInternalMsg::EVENT_NO_CONSUMERS);
				myPmConsumerThread->pushToInput(noConsumerMessage);
			}
			pthread_mutex_unlock(&m_pmDataReadyMutex);
		}
	}
	PMTSA_LOG("Un-subscription request of PM-events OK");
	LEAVE_PMTSA();
	return ComOk;
}

/**
 * @ingroup PmtSa
 *
 * Part of the PM-interface to COM, this method should be called by COM
 * once they have received the ComOamSpiPmEventTypeGpReady_1 event.
 *
 * @param[in] 		eventType   Pointer to an event-name.
 * @param[in]       value       Pointer to a value that should be removed.
 *
 * @return 	ComReturnT	ComOk if event was of the right kind
 * @return  ComReturnT  ComFailure if the event was of the wrong kind
 *
 */
ComReturnT PmEventHandler::doneWithValue(const char* eventType, void* value)
{
	ENTER_PMTSA();
	if ((eventType == NULL) || (value == NULL))
	{
		LEAVE_PMTSA();
		return ComFailure;
	}
	else
	{
		if (strcmp(ComOamSpiPmEventTypeGpReady_2, eventType) == 0)
		{
			ComOamSpiPmGpId_2T* pmEvValue = static_cast<ComOamSpiPmGpId_2T*>(value);
			delete pmEvValue;
		}
		else
		{
			PMTSA_LOG("Received wrong event-type!");
			LEAVE_PMTSA();
			return ComFailure;
		}
	}
	LEAVE_PMTSA();
	return ComOk;
}

/**
 * @ingroup PmtSa
 *
 * Used for resetting the class
 *
 * @return 	ComReturnT  ComOk on success
 *
 */
ComReturnT PmEventHandler::clearAll()
{
	ENTER_PMTSA();
	try
	{
		pthread_mutex_lock(&m_pmDataReadyMutex);
		m_pmDataReadyFilters.clear();
		pthread_mutex_unlock(&m_pmDataReadyMutex);
	}
	catch (const std::exception& e)
	{
		pthread_mutex_unlock(&m_pmDataReadyMutex);
		PMTSA_ERR("Caught exception: %s", e.what());
		LEAVE_PMTSA();
		return ComFailure;
	}
	catch (...)
	{
		pthread_mutex_unlock(&m_pmDataReadyMutex);
		PMTSA_ERR("Unknown exception!");
		LEAVE_PMTSA();
		return ComFailure;
	}
	LEAVE_PMTSA();
	return ComOk;
}

/**
 * @ingroup PmtSa
 *
 * Use the pmGpId variable to fetch the callback-data needed to pick out
 * the requested GP-data from PM SHMem. Clear?
 *
 * @param[in]	pmGpId		Unique id of GP
 * @param[out]	data		Pointer to GP-data. Note that the memory
 * 							must be free'd with the releaseGpData-method!
 *
 * @return	ComReturnT		ComOk on success, ComFailure otherwise
 *
 */
ComReturnT PmEventHandler::getGpData(ComOamSpiPmGpId_2T pmGpId,
		ComOamSpiPmGpData_2T** data)
{
	ENTER_PMTSA();
	//    PMTSA_LOG("Received request of GP-data for GP-id %ld", pmGpId);
	ComReturnT retVal = ComOk;
	pthread_mutex_lock(&m_pmDataReadyMutex);

	// First make sure that we're not already processing data
	if ((m_COMcurrentGp.jobHandler != 0) || (m_COMcurrentResult != NULL))
	{
		PMTSA_WARN("Bad state for operation: Can't fetch data! Previous data not cleared.");
		retVal = ComFailure;
	}
	else
	{
		// See if we can find the Gp within our finished GP's
		bool found = false;
		std::vector<SaPmCCallbackInfoT*>::iterator it;
		for(it = m_finishedGps.begin();
				it != m_finishedGps.end() && !found;
				++it)
		{
			SaPmCCallbackInfoT* pmData = (*it);
			if (pmData->jobHandler == pmGpId)
			{
				found = true;
				m_COMcurrentGp.jobHandler = pmData->jobHandler;
				m_COMcurrentGp.clientHdlr = pmData->clientHdlr;
				m_COMcurrentGp.gpStartTimestamp = pmData->gpStartTimestamp;
				m_COMcurrentGp.gpEndTimestamp = pmData->gpEndTimestamp;
				m_COMcurrentGp.jobname = pmData->jobname;
			}
		}
		if (!found)
		{
			PMTSA_WARN("Couldn't find job %ld in finished-jobs", pmGpId);
			retVal = ComNotExist;
		}
		else
		{
			m_COMcurrentResult = getData(&m_COMcurrentGp);
			if (m_COMcurrentResult != NULL)
			{
				*data = m_COMcurrentResult;
				//                PMTSA_LOG("GP-data contains %d different objects", m_COMcurrentResult->size);
			}
			else
			{
				PMTSA_WARN("Failed fetching job %ld with getData", pmGpId);
				m_finishedGps.erase(it); // Remove 'dead' data
				memset(&m_COMcurrentGp, 0x00, sizeof(SaPmCCallbackInfoT));
				retVal = ComNotExist;
			}
		}
	}
	pthread_mutex_unlock(&m_pmDataReadyMutex);
	//	if (retVal == ComOk)
	//	{
	//	    PMTSA_LOG("GP-data retrieved OK");
	//	}
	//	else
	if (retVal == ComFailure)
	{
		PMTSA_WARN("Already fetched GP-data not cleared, returning ComFailure");
	}
	else if (retVal == ComNotExist)
	{
		PMTSA_LOG("GP-data not found, returning ComNotExist");
	}
	LEAVE_PMTSA();
	return retVal;
}

/**
 * @ingroup PmtSa
 *
 * Use the pmGpId variable to fetch the callback-data needed to pick out
 * the requested GP-data from PM SHMem. Clear?
 *
 * @param[in]   pmGpId          Unique id of GP
 * @param[in]   gpName          To store PM-data of respective Gp in individual GpName structure.
 * @param[out]  data            Pointer to GP-data. Note that the memory
 *                                                      must be free'd with the releaseGpData-method!
 *
 * @return      ComReturnT              ComOk on success, ComFailure otherwise
 *
 */
ComReturnT PmEventHandler::getGpData2(ComOamSpiPmGpId_2T pmGpId, const char* gpName,
                ComOamSpiPmGpData_2T** data)
{
        ENTER_PMTSA();
        ComReturnT retVal = ComOk;

	//Find the respective gp structure with the gpName
	int iGp = gpMap.find(gpName)->second;

        // First make sure that we're not already processing data
	// Process the respective PM data structure based on gp index.
        if ((m_COMcurrentGp2[iGp].jobHandler != 0) || (m_COMcurrentResult2[iGp] != NULL))
        {
                PMTSA_WARN("Bad state for operation: Can't fetch data! Previous data not cleared.");
                retVal = ComFailure;
        }
        else
        {
                // See if we can find the Gp within our finished GP's
                bool found = false;
                std::vector<SaPmCCallbackInfoT*>::iterator it;
                for(it = m_finishedGps2[iGp].begin();
                                it != m_finishedGps2[iGp].end() && !found;
                                ++it)
                {
                        SaPmCCallbackInfoT* pmData = (*it);
                        if (pmData->jobHandler == pmGpId)
                        {
                                found = true;
                                m_COMcurrentGp2[iGp].jobHandler = pmData->jobHandler;
                                m_COMcurrentGp2[iGp].clientHdlr = pmData->clientHdlr;
                                m_COMcurrentGp2[iGp].gpStartTimestamp = pmData->gpStartTimestamp;
                                m_COMcurrentGp2[iGp].gpEndTimestamp = pmData->gpEndTimestamp;
                                m_COMcurrentGp2[iGp].jobname = pmData->jobname;
                        }
                }
                if (!found)
                {
                        PMTSA_WARN("Couldn't find job %ld in finished-jobs", pmGpId);
                        retVal = ComNotExist;
                }
		else
                {
                        m_COMcurrentResult2[iGp] = getData2(&m_COMcurrentGp2[iGp],iGp);
                        if (m_COMcurrentResult2[iGp] != NULL)
                        {
                                *data = m_COMcurrentResult2[iGp];
                                //                PMTSA_LOG("GP-data contains %d different objects", m_COMcurrentResult->size);
                        }
                        else
                        {
                                PMTSA_WARN("Failed fetching job %ld with getData", pmGpId);
                                m_finishedGps2[iGp].erase(it); // Remove 'dead' data
                                memset(&m_COMcurrentGp2[iGp], 0x00, sizeof(SaPmCCallbackInfoT));
                                retVal = ComNotExist;
                        }
                }
        }
        //pthread_mutex_unlock(&m_pmDataReadyMutex);
        //      if (retVal == ComOk)
        //      {
        //          PMTSA_LOG("GP-data retrieved OK");
        //      }
        //      else
        if (retVal == ComFailure)
        {
                PMTSA_WARN("Already fetched GP-data not cleared, returning ComFailure");
        }
        else if (retVal == ComNotExist)
        {
                PMTSA_LOG("GP-data not found, returning ComNotExist");
        }
        LEAVE_PMTSA();
        return retVal;
}

/**
 * @ingroup PmtSa
 *
 * This method is part of the Com-PM SPI, and is called when COM
 * considers itself done with the PM-data delivered from the getData
 * method.
 *
 * @param[in] 	pmGpId		The unique identity of a PM-job that should
 * 							be removed from the PM SHMem.
 *
 * @return	ComReturnT	Will most likely return ComOk always. If an invalid
 * 						pmGpId is given, ComNotExist will be returned, and
 * 						upon an internal error, ComFailure will be returned.
 *
 * @par Globals: Write a description here or remove if not used!
 *
 */
ComReturnT PmEventHandler::releaseGpData(ComOamSpiPmGpId_2T pmGpId)
{
	ENTER_PMTSA();
	ComReturnT retVal = ComOk;
	//PMTSA_DEBUG("PMT-SA: Received request for removal of GP-data\n");
	pthread_mutex_lock(&m_pmDataReadyMutex);

	//Above releaseGpData method() will be called only when pmCollectionTimer feature is disabled
	//Remove the unused feature enabled structure(m_finishedGps2)
        removefinishedGp2(pmGpId);

	// See if we can find the Gp within our finished GP's
	bool found = false;
	std::vector<SaPmCCallbackInfoT*>::iterator it;
	for(it = m_finishedGps.begin(); it != m_finishedGps.end() && !found; ++it)
	{
		if (((*it)->jobHandler == pmGpId) &&
				(pmGpId == m_COMcurrentGp.jobHandler))
		{
			// The current job that has been given to COM
			found = true;
			SaPmCCallbackInfoT* pmData = (*it);
			freeData(m_COMcurrentResult); // Cleans our allocated memory
			m_COMcurrentResult = NULL;
			if (pmData->jobname != NULL)
			{
				free(pmData->jobname);
				pmData->jobname = NULL;
			}
			delete pmData;
			it = m_finishedGps.erase(it);
			memset(&m_COMcurrentGp, 0x00, sizeof(SaPmCCallbackInfoT));
			// Tell PM-service to delete data too
			SaAisErrorT saResult = PmConsumerInterface::instance()->jobGranularityPeriodDataRemove(pmGpId);
			retVal = (saResult == SA_AIS_OK ? ComOk :
					(saResult == SA_AIS_ERR_NOT_EXIST ? ComNotExist :
							(saResult == SA_AIS_ERR_UNAVAILABLE ? ComNotExist : ComFailure)));
		}
		else
		{
			// This is the case when COM decides to remove the GP-data
			// without working with it
			if ((*it)->jobHandler == pmGpId)
			{
				found = true;
				SaPmCCallbackInfoT* pmData = (*it);
				if (pmData->jobname != NULL)
				{
					free(pmData->jobname);
					pmData->jobname = NULL;
				}
				delete pmData;
				it = m_finishedGps.erase(it);
				// Tell PM-service to delete data too
				SaAisErrorT saResult = PmConsumerInterface::instance()->jobGranularityPeriodDataRemove(pmGpId);
				retVal = (saResult == SA_AIS_OK ? ComOk :
						(saResult == SA_AIS_ERR_NOT_EXIST ? ComNotExist :
								(saResult == SA_AIS_ERR_UNAVAILABLE ? ComNotExist : ComFailure)));
			}
		}
	}

	// Remove m_finishedGps2 which is not required for interface2 releaseGpData
	if (found) {
		removefinishedGp2(pmGpId);
	}

	if (!found)
	{
		retVal = ComNotExist;
	}
	pthread_mutex_unlock(&m_pmDataReadyMutex);
	if (retVal != ComOk)
	{
		PMTSA_WARN("Removal of GP-data FAILED, return-value: %d", retVal);
	}
	LEAVE_PMTSA();
	return retVal;
}

/**
 * @ingroup PmtSa
 *
 * This method is part of the Com-PM SPI, and is called when COM
 * considers itself done with the PM-data delivered from the getData
 * method.
 *
 * @param[in]   pmGpId          The unique identity of a PM-job that should
 *                                                      be removed from the PM SHMem.
 * @param[in]   gpName          To remove PM-data of respective Gp in individual GpName structure.
 *
 * @return      ComReturnT      Will most likely return ComOk always. If an invalid
 *                                              pmGpId is given, ComNotExist will be returned, and
 *                                              upon an internal error, ComFailure will be returned.
 *
 * @par Globals: Write a description here or remove if not used!
 *
 */
ComReturnT PmEventHandler::releaseGpData2(ComOamSpiPmGpId_2T pmGpId, const char* gpName)
{
        ENTER_PMTSA();
        ComReturnT retVal = ComOk;
        //PMTSA_DEBUG("PMT-SA: Received request for removal of GP-data\n");
	//Find the respective gp structure with the gpName
	int iGp = gpMap.find(gpName)->second;

	//Above releaseGpData2 method() will be called only when pmCollectionTimer feature is enabled
        //Remove the unused feature disabled structure(m_finishedGps)
        removefinishedGp(pmGpId);

        // See if we can find the Gp within our finished GP's
        bool found = false;
        std::vector<SaPmCCallbackInfoT*>::iterator it;
        for(it = m_finishedGps2[iGp].begin(); it != m_finishedGps2[iGp].end() && !found; ++it)
        {
                if (((*it)->jobHandler == pmGpId) &&
                                (pmGpId == m_COMcurrentGp2[iGp].jobHandler))
                {
                        // The current job that has been given to COM
                        found = true;
                        SaPmCCallbackInfoT* pmData = (*it);
                        freeData2(m_COMcurrentResult2[iGp],iGp); // Cleans our allocated memory
                        m_COMcurrentResult2[iGp] = NULL;
                        if (pmData->jobname != NULL)
                        {
                                free(pmData->jobname);
                                pmData->jobname = NULL;
                        }
                        delete pmData;
                        it = m_finishedGps2[iGp].erase(it);
                        memset(&m_COMcurrentGp2[iGp], 0x00, sizeof(SaPmCCallbackInfoT));
                        // Tell PM-service to delete data too
                        SaAisErrorT saResult = PmConsumerInterface::instance()->jobGranularityPeriodDataRemove(pmGpId);
                        retVal = (saResult == SA_AIS_OK ? ComOk :
                                        (saResult == SA_AIS_ERR_NOT_EXIST ? ComNotExist :
                                                        (saResult == SA_AIS_ERR_UNAVAILABLE ? ComNotExist : ComFailure)));
                }
                else
                {
                        // This is the case when COM decides to remove the GP-data
                        // without working with it
                        if ((*it)->jobHandler == pmGpId)
			{
                                found = true;
                                SaPmCCallbackInfoT* pmData = (*it);
                                if (pmData->jobname != NULL)
                                {
                                        free(pmData->jobname);
                                        pmData->jobname = NULL;
                                }
                                delete pmData;
                                it = m_finishedGps2[iGp].erase(it);
                                // Tell PM-service to delete data too
                                SaAisErrorT saResult = PmConsumerInterface::instance()->jobGranularityPeriodDataRemove(pmGpId);
                                retVal = (saResult == SA_AIS_OK ? ComOk :
                                                (saResult == SA_AIS_ERR_NOT_EXIST ? ComNotExist :
                                                                (saResult == SA_AIS_ERR_UNAVAILABLE ? ComNotExist : ComFailure)));
                        }
                }
        }

	// Remove m_finishedGps which is not required for interface2_1 releaseGpData2
	if (found){
		removefinishedGp(pmGpId);
	}

        if (!found)
        {
                retVal = ComNotExist;
        }
        //pthread_mutex_unlock(&m_pmDataReadyMutex);
        if (retVal != ComOk)
        {
                PMTSA_WARN("Removal of GP-data FAILED, return-value: %d", retVal);
        }
        LEAVE_PMTSA();
        return retVal;
}

/**
 * @ingroup PmtSa
 *
 * This method clears the finishedGp structure that carries event information.
 *
 * @param[in]   pmGpId          The unique identity of a PM-job that should
 *
 * @return      ComReturnT      ComOk if we could remove the data in the structure
 *
 * @par Globals: Write a description here or remove if not used!
 *
 */
ComReturnT PmEventHandler::removefinishedGp(ComOamSpiPmGpId_2T pmGpId)
{
	ENTER_PMTSA();
	ComReturnT retVal = ComOk;

	//Multiple parallel thread Gp request to modify same structure requires locking mechanism
	pthread_mutex_lock(&m_pmDataReadyMutex);

	std::vector<SaPmCCallbackInfoT*>::iterator itr;
	bool foundgp = false;
	for(itr = m_finishedGps.begin(); itr != m_finishedGps.end() && !foundgp; ++itr)
        {
                if ((*itr)->jobHandler == pmGpId){
			itr = m_finishedGps.erase(itr);
			foundgp = true;
		}
	}

	pthread_mutex_unlock(&m_pmDataReadyMutex);

        LEAVE_PMTSA();
        return retVal;

}

/**
 * @ingroup PmtSa
 *
 * This method clears the finishedGp2 structure that carries event information.
 *
 * @param[in]   pmGpId          The unique identity of a PM-job that should
 *
 * @return      ComReturnT      ComOk if we could remove the data in the structure
 *
 * @par Globals: Write a description here or remove if not used!
 *
 */
ComReturnT PmEventHandler::removefinishedGp2(ComOamSpiPmGpId_2T pmGpId)
{
        ENTER_PMTSA();
        ComReturnT retVal = ComOk;

	// Already inside lock hence it is not required for single thread

	for (int i = 0; i < MAX_NO_OF_GPS; i++) {

		std::vector<SaPmCCallbackInfoT*>::iterator itr;
		bool foundgp = false;

		for(itr = m_finishedGps2[i].begin(); itr != m_finishedGps2[i].end() && !foundgp; ++itr)
		{
			if ((*itr)->jobHandler == pmGpId){
				itr = m_finishedGps2[i].erase(itr);
				foundgp = true;
			}
		}
	}

        LEAVE_PMTSA();
        return retVal;

}

/* ---------------------------------------------------------------------- */

ComReturnT pmtsa_addFilter(ComOamSpiEventConsumerHandleT consumerHandle,
		const char* eventType,
		ComNameValuePairT** filters)
{
	ENTER_PMTSA();
	ComReturnT retVal = PmEventHandler::instance()->addFilter(consumerHandle, eventType, filters);
	LEAVE_PMTSA();
	return retVal;
}

ComReturnT pmtsa_removeFilter(ComOamSpiEventConsumerHandleT consumerHandle,
		const char* eventType,
		ComNameValuePairT** filters)
{
	ENTER_PMTSA();
	ComReturnT retVal = PmEventHandler::instance()->removeFilter(consumerHandle, eventType, filters);
	LEAVE_PMTSA();
	return retVal;
}

ComReturnT pmtsa_doneWithValue(const char* eventType, void* value)
{
	ENTER_PMTSA();
	ComReturnT retVal = PmEventHandler::instance()->doneWithValue(eventType, value);
	LEAVE_PMTSA();
	return retVal;
}

ComReturnT pmtsa_clearAll()
{
	ENTER_PMTSA();
	ComReturnT retVal = PmEventHandler::instance()->clearAll();
	LEAVE_PMTSA();
	return retVal;
}

ComReturnT pmtsa_getGpData(ComOamSpiPmGpId_2T pmGpId, ComOamSpiPmGpData_2T** data)
{
	ENTER_PMTSA();
	ComReturnT retVal = PmEventHandler::instance()->getGpData(pmGpId, data);
	LEAVE_PMTSA();
	return retVal;
}

ComReturnT pmtsa_getGpData_2(ComOamSpiPmGpId_2T pmGpId, const char* gpName, ComOamSpiPmGpData_2T** data)
{
        ENTER_PMTSA();
        ComReturnT retVal = PmEventHandler::instance()->getGpData2(pmGpId, gpName, data);
        LEAVE_PMTSA();
        return retVal;
}

ComReturnT pmtsa_releaseGpData(ComOamSpiPmGpId_2T pmGpId)
{
	ENTER_PMTSA();
	ComReturnT retVal = PmEventHandler::instance()->releaseGpData(pmGpId);
	LEAVE_PMTSA();
	return retVal;
}

ComReturnT pmtsa_releaseGpData_2(ComOamSpiPmGpId_2T pmGpId, const char* gpName)
{
        ENTER_PMTSA();
        ComReturnT retVal = PmEventHandler::instance()->releaseGpData2(pmGpId, gpName);
        LEAVE_PMTSA();
        return retVal;
}
