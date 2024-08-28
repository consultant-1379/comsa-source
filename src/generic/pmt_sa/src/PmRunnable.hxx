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
 * File: PmRunnable.hxx
 *
 * Author: ejonajo 2011-09-01
 *
 * Reviewed: efaiami 2012-04-14
 * Modified: xtronle 2013-12-25 Support PDF counter
 * Modified: xnikvap 2015-08-02  MR42277 Remove old interfaces exposed to COM & CoreMW that are no longer supported for SLES12
 * Modified: xtronle 2015-09-15 MR36219 PM counters to support troubleshooting
 *
 ************************************************************************** */
#ifndef PMRUNNABLE_HXX_
#define PMRUNNABLE_HXX_

/**
 * The PM-runnable class
 */
#include "PmThreadTemplate.hxx"
#include "PmInternalMsg.hxx"
#include "PmInternalMessageStream.hxx"
#include "PmConsumerInterface.hxx"
#include "PmtSaTrace.hxx"

// Com Spi
#include <ComMgmtSpiComponent_1.h>
#include <ComOamSpiEvent_1.h>
#include <ComMgmtSpiInterface_1.h>
#include <ComMwSpiAvailabilityController_1.h>

/**
 * @file PmRunnable.hxx
 *
 * @ingroup PmtSa
 *
 * @brief Thread connected to PM-service
 *
 * This object connects to the external PM-service provided by the middle-
 * ware that runs on the system.  When a granularity-period ends, a PM-event
 * is received in this object.  That event is then propagated to COM.
 */

namespace PmtSa
{

/**
 * @ingroup PmtSa
 *
 * @brief Thread connected to external PM service.
 *
 * The interface for the thread that is hooked up with the PM service in
 * the middle-ware.
 */
class PmRunnable {
	friend class PmShowCounters;
public:
	/**
	 * @ingroup PmtSa
	 *
	 * Constructor
	 *
	 */
	PmRunnable();

	/**
	 * @ingroup PmtSa
	 *
	 * Destructor
	 *
	 */
	virtual ~PmRunnable();

	/**
	 * @ingroup PmtSa
	 *
	 * Returns a pointer to the singleton instance.
	 *
	 * @return PmRunnable*  Pointer to the singleton instance.
	 *
	 */
	static PmRunnable* instance();

	/**
	 * @ingroup PmtSa
	 *
	 * Method that starts execution of the embedded thread
	 *
	 * @return bool true if the thread was started, false otherwise
	 */
	bool run();

	/**
	 * @ingroup PmtSa
	 *
	 * Method that should stop the thread from execution.
	 */
	void stop();

	/**
	 * @ingroup PmtSa
	 *
	 * Predicate that tells if the thread is running or not
	 *
	 * @return bool true if the thread is running, false if not
	 */
	bool running() const;

	/**
	 * @ingroup PmtSa
	 *
	 * Predicate that tells if we're still connected to PM libs
	 *
	 * @return bool true if we're connected, false if not
	 */
	bool libsConnected() const;

	/**
	 * @ingroup PmtSa
	 *
	 * Input read file descriptor
	 *
	 * @return int File descriptor for input read
	 */
	const int inputReadFD();

	/**
	 * @ingroup PmtSa
	 *
	 * Input write file descriptor
	 *
	 * @return int File descriptor for input write
	 */
	const int inputWriteFD();

	void pushToInput(PmInternalMsg* msg);

	/**
	 * @ingroup PmtSa
	 *
	 * Output read file descriptor
	 *
	 * @return int File descriptor for output read
	 */
	const int outputReadFD();

	/**
	 * @ingroup PmtSa
	 *
	 * Output write file descriptor
	 *
	 * @return int File descriptor for output write
	 */
	const int outputWriteFD();

private:
	void pmtSelectLoop();

	void handlePmInternalMessage();
	bool startSubscription();
	bool stopSubscription();

	static PmRunnable* s_instance; /*!< Pointer to singleton instance */
	volatile bool m_isRunning; /*!< Is the thread running or not */
	// Since stopSubscription method might get multiple calls, we must protect the method with a mutex.
        pthread_mutex_t m_pmMutex;
	PmThreadType<PmRunnable> m_myThread; /*!< Reference to the thread */

	PmInternalMessageStream m_input; /*!< Incoming data to the thread */
	PmInternalMessageStream m_output; /*!< Outgoing data from the thread */

	// OpenSAF PM attributes
	SaPmCHandleT m_osafPmHandle; /*!< Handle to external PM services */
	SaVersionT m_osafPmVersion; /*!< New version of PM services support PDF counter*/
	SaSelectionObjectT m_osafPmPollFd; /*!< File descriptor to external PM service */
	SaPmCSubscribeInfoT m_osafPmSubscribeInfo; /*!< Subscription information for external PM service */
	SaPmCSubscribeInfoT_2 m_osafPmSubscribeInfo_2; /*!< Subscription information for external PM service */

	PmConsumerInterface* m_pmConsApi; /*!< Pointer to external PM consumer API */
};

inline PmRunnable* PmRunnable::instance()
{
	ENTER_PMTSA();
	LEAVE_PMTSA();
	return s_instance;
}

inline void PmRunnable::stop()
{
	ENTER_PMTSA();
	PMTSA_LOG("Stopping consumer thread");
	// Call stopSubscription directly here to avoid memory leak when PMT-SA stopping
	if (m_isRunning)
	{
		if(!stopSubscription())
		{
			PMTSA_ERR("Internal message: Stop subscription FAILED");
			// FIXME: What do we do here??
		}
	}
	m_isRunning = false;
	bool res = m_myThread.join();
	if (res == true)
	{
		PMTSA_LOG("Consumer thread stopped OK");
	}
	else
	{
		PMTSA_ERR("Failed stopping consumer thread!");
	}
	LEAVE_PMTSA();
}

inline bool PmRunnable::libsConnected() const
{
	ENTER_PMTSA();
	LEAVE_PMTSA();
	return (m_osafPmPollFd != 0 ? true : false);
}

inline bool PmRunnable::running() const
{
	ENTER_PMTSA();
	LEAVE_PMTSA();
	return m_isRunning;
}

inline void PmRunnable::pushToInput(PmInternalMsg* msg)
{
	ENTER_PMTSA();
	LEAVE_PMTSA();
	m_input.push(msg);
}

} // Name-space ending

extern "C" void* pmtsa_consumerThread(void* vPtr);
extern "C" SaAisErrorT pmtsa_pmConsumerCallback(SaPmCCallbackInfoT* data);

#endif /* PMRUNNABLE_HXX_ */
