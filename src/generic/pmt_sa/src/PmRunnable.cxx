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
* File: PmRunnable.cxx
*
* Author: ejonajo 2011-09-01
*
* Reviewed: efaiami 2012-04-14
* Modified: xtronle 2013-12-25 Support PDF counter
* Modified: xnikvap 2015-08-02 MR42277 Remove old interfaces exposed to COM & CoreMW that are no longer supported for SLES12
*
************************************************************************** */
#include "PmRunnable.hxx"
#include "PmConsumer.hxx"
#include "PmConsumerInterface.hxx"
#include "PmEventHandler.hxx"
#include "PmComComponent.hxx"
#include "PmtSaTrace.hxx"
#include "ComSA.h"
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <cstdio>
#include <sys/time.h>


/**
* @ingroup PmtSa
*
* @file PmRunnable.cxx
*
* @brief This class encapsulates the thread that listens for PM-events, eg GP-switches.
*
* The PmRunnable class is only available when the PMT_SA component is supposed to be active.
* When COM registers that they are interested in getting PM-data, the class starts to listen
* for events on the handle provided from the PM-api.
*/

using namespace PmtSa;

PmRunnable* PmRunnable::s_instance = NULL;

PmRunnable::PmRunnable() :
	m_myThread(&PmRunnable::pmtSelectLoop),
	m_input("Thread input stream"),
	m_output("Thread output stream"),
	m_pmConsApi(NULL)
{
	ENTER_PMTSA();
	s_instance = this;
	m_isRunning = false;
	m_osafPmHandle = 0;
	m_osafPmVersion.releaseCode = pmReleaseCode;
	m_osafPmVersion.majorVersion = pmMajorVersion;
	m_osafPmVersion.minorVersion = pmMinorVersion;
	if (pthread_mutex_init(&m_pmMutex, NULL) != 0)
	{
		PMTSA_WARN("Failed initializing mutex in PmRunnable!");
	}

	m_osafPmPollFd = 0;
	std::memset(&m_osafPmSubscribeInfo, 0x00, sizeof(m_osafPmSubscribeInfo));
	m_pmConsApi = PmConsumerInterface::instance();
	LEAVE_PMTSA();
}

PmRunnable::~PmRunnable()
{
	ENTER_PMTSA();
	if (pthread_mutex_destroy(&m_pmMutex) != 0)
	{
		PMTSA_WARN("Failed destroying mutex in PmRunnable!");
	}
	if (m_pmConsApi)
	{
		delete m_pmConsApi;
	}
	s_instance = NULL;
	LEAVE_PMTSA();
}

/**
* @ingroup PmtSa
*
* This method is run in a separate thread.	 Basically all we do is that
* we check the in-pipe and see if there's any message waiting for us, if
* we've opened up the PM services then we look at that file descriptor too
* and then we take appropriate action depending on if something has happened
* on the file descriptors.
* The select-call is timeout controlled, we wait maximum one second, then
* we start over again.
*/
void PmRunnable::pmtSelectLoop()
{
	ENTER_PMTSA();
	while(m_isRunning)
	{
		fd_set readFds;
		FD_ZERO(&readFds);
		FD_SET(s_instance->inputReadFD(), &readFds);
		int maxFd;
		if (m_osafPmPollFd != 0)
		{
			// This is only set if we're having a consumer of the PM-data
			FD_SET(m_osafPmPollFd, &readFds);
			// Add zero to const int FD so compiler can make it unsigned in compare ...
			maxFd = (m_osafPmPollFd > (unsigned int)(s_instance->inputReadFD()+0) ? m_osafPmPollFd : s_instance->inputReadFD()) + 1;
		}
		else
		{
			maxFd = s_instance->inputReadFD() + 1;
		}
		/*
		* We have a timeout each 100:th millisecond
		*/
		struct timeval timeOut;
		timeOut.tv_sec = 0;
		timeOut.tv_usec = 100000;
		// See if something happens ...
		int selRet = select(maxFd, &readFds, NULL, NULL, &timeOut);
		if (selRet == -1 && errno == EINTR)
		{
			// We probably don't have to log this
			// PMTSA_LOG("Select-call was interupted (EINTR)");
			continue;
		}
		if (selRet < 0)
		{
			PMTSA_WARN("Select returned %d (Errno %d: %s)", selRet, errno, strerror(errno));
			break;
		}
		// See where the message came from, check PM-service first
		if ((selRet > 0) && (m_osafPmPollFd != 0) && (FD_ISSET(m_osafPmPollFd, &readFds)))
		{
			m_pmConsApi->dispatch(m_osafPmHandle, SA_DISPATCH_ONE);
			//PMTSA_LOG("Dispatched one GP");
		}
		if ((selRet > 0) && (FD_ISSET(s_instance->inputReadFD(), &readFds)))
		{
			handlePmInternalMessage();
		}
	}
	LEAVE_PMTSA();
}

/**
* @ingroup PmtSa
*
* The method that pthreads call when it's started
*
* @return	bool	true if the thread was started, false otherwise
*/
bool PmRunnable::run()
{
	ENTER_PMTSA();
	m_isRunning = true;
	LEAVE_PMTSA();
	return m_myThread.run(this);
}

/**
* @ingroup PmtSa
*
* Returns the FD of the 'read-end' of the input stream to the thread
*
* @return	int	Read-end of the input message-stream
*
*/
const int PmRunnable::inputReadFD()
{
	ENTER_PMTSA();
	LEAVE_PMTSA();
	return m_input.readHandle();
}

/**
* @ingroup PmtSa
*
* Returns the FD of the write-end of the input stream to the thread
*
* @return	int Write-end of the input message-stream
*
*/
const int PmRunnable::inputWriteFD()
{
	ENTER_PMTSA();
	LEAVE_PMTSA();
	return m_input.writeHandle();
}

/**
* @ingroup PmtSa
*
* Returns the FD of the 'read-end' of the output stream of the thread
*
* @return	int	Read-end of the output message-stream
*
*/
const int PmRunnable::outputReadFD()
{
	ENTER_PMTSA();
	LEAVE_PMTSA();
	return m_output.readHandle();
}

/**
* @ingroup PmtSa
*
* Returns the FD of the 'write-end' of the output stream of the thread
*
* @return	int	Write-end of the output message-stream
*
*/
const int PmRunnable::outputWriteFD()
{
	ENTER_PMTSA();
	LEAVE_PMTSA();
	return m_output.writeHandle();
}

/**
* @ingroup PmtSa
*
* Once there's data from the PM-service in CoreMW, this is the place
* where we'll end up.
*
* @param[in]	data	Information about what job it is that is finished
*
* @return	SaAisErrorT We always return SA_AIS_OK here.
*
*/
SaAisErrorT pmtsa_pmConsumerCallback(SaPmCCallbackInfoT* data)
{
	// Tell COM about the presence of a job
	ENTER_PMTSA();
	PmEventHandler::instance()->sendPmDataReady(data);
	LEAVE_PMTSA();
	return SA_AIS_OK;
}

/**
* @ingroup PmtSa
*
* We won't start subscribing to PM events until we have a subscriber
* to the PM-data.	Thus when we get the message of a subscriber being
* present, we connect to the PM-service and start delivering data.	 Thus
* the (PMTSA) component might be active and alive but not doing anything
* against the PM-service.
*
* @return	bool	true on success, false on failure
*
*/
bool PmRunnable::startSubscription()
{
	ENTER_PMTSA();
	if (m_osafPmHandle == 0)
	{
		PMTSA_LOG("Initializing PM consumer services, requesting PM Consumer API ver. %c.%u.%u ",
				m_osafPmVersion.releaseCode, m_osafPmVersion.majorVersion, m_osafPmVersion.minorVersion);
		m_osafPmSubscribeInfo_2.rmInfo.clientHdlr = 0;
		m_osafPmSubscribeInfo_2.rmInfo.cbPtr = ::pmtsa_pmConsumerCallback;

		SaAisErrorT rc = m_pmConsApi->initialize_2(&m_osafPmHandle, &m_osafPmVersion, SA_PMF_JOB_TYPE_MEASUREMENT,
				PMC_JOB_DATA_ORGANIZATION_MOCLASS_MEASOBJ, &m_osafPmSubscribeInfo_2);
		if (rc != SA_AIS_OK)
		{
			PMTSA_WARN("FAILED initializing PM consumer services, rc = %d", rc);
			m_osafPmHandle = 0;
			LEAVE_PMTSA();
			return false;
		}

		rc = m_pmConsApi->selectionObjectGet(m_osafPmHandle, &m_osafPmPollFd);
		if (rc != SA_AIS_OK)
		{
			PMTSA_WARN("FAILED getting PM consumer poll handle, rc = %d", rc);
			m_osafPmHandle = 0;
			m_osafPmPollFd = 0;
			LEAVE_PMTSA();
			return false;
		}

		rc = m_pmConsApi->activate(m_osafPmHandle);
		if (rc != SA_AIS_OK)
		{
			PMTSA_WARN("FAILED activating PM consumer services, rc = %d", rc);
			m_osafPmHandle = 0;
			m_osafPmPollFd = 0;
			LEAVE_PMTSA();
			return false;
		}
		else
		{
			PMTSA_LOG("PM consumer services started OK, ver. %c.%u.%u, handle=%x, PDF counters supported",
					m_osafPmVersion.releaseCode, m_osafPmVersion.majorVersion, m_osafPmVersion.minorVersion, m_osafPmHandle);
		}
	}
	else
	{
		PMTSA_LOG("PM consumer services already started");
	}
	// Unless something is really screwed up, we're already connected
	// to the PM-service
	LEAVE_PMTSA();
	return true;
}

/**
* @ingroup PmtSa
*
* If we're subscribing for PM-data, make sure that we turn that off and
* then return.	 We might get multiple calls to this method when shutting
* down, that's alrigt.
*
* @return true	When shutdown of PM-subsciption succeeded (or is already turned off).
* @return false	If the finalize() call on the PM-api failed.
*/
bool PmRunnable::stopSubscription()
{
	ENTER_PMTSA();
        pthread_mutex_lock(&m_pmMutex);
	if (m_osafPmHandle != 0)
	{
		PMTSA_LOG("Stopping PM consumer subscription with handle=%x", m_osafPmHandle);
		SaAisErrorT rc = m_pmConsApi->deactivate(m_osafPmHandle);
		if (rc != SA_AIS_OK)
		{
			// Even if the deactiveate call fails, we shouldn't bail out
			// and return false, instead we should really try to finalize
			// the API.	 Thus only a warning in the log.
			PMTSA_WARN("PM consumer service deactivate FAILED, rc = %d", rc);
		}
		else
		{
			PMTSA_LOG("PM consumer services deactivated");
		}
		rc = m_pmConsApi->finalize(m_osafPmHandle);
		if (rc != SA_AIS_OK)
		{
			PMTSA_WARN("PM consumer services finalize FAILED, rc = %d", rc);
			pthread_mutex_unlock(&m_pmMutex);
			LEAVE_PMTSA();
			return false;
		}
		else
		{
			PMTSA_LOG("PM consumer services finalized");
		}
		m_osafPmHandle = 0;
		m_osafPmPollFd = 0;
		memset(&m_osafPmSubscribeInfo, 0x00, sizeof (m_osafPmSubscribeInfo));
	}
	PMTSA_LOG("PM consumer services Stopped");
        pthread_mutex_unlock(&m_pmMutex);
	LEAVE_PMTSA();
	return true;
}

/**
* @ingroup PmtSa
*
* Checks the input-stream for messages and does what it takes with them
*
* @return	void	Doesn't return anything
*
*/
void PmRunnable::handlePmInternalMessage()
{
	ENTER_PMTSA();
	std::list<PmInternalMsg*>* poppedMsgs = m_input.pop();
	if (poppedMsgs != NULL)
	{
		std::list<PmInternalMsg*>::iterator it;
		for (it = poppedMsgs->begin(); it != poppedMsgs->end(); ++it)
		{
			PmInternalMsg* msg = *it;
			switch(msg->pmMessageType())
			{
			case PmInternalMsg::EVENT_CONSUMER_REGISTERED:
				PMTSA_LOG("Internal message: Consumer registered");
				if (!startSubscription())
				{
					PMTSA_ERR("Internal message: Start subscription FAILED");
					// FIXME: What do we do here??
				}
				break;
			case PmInternalMsg::EVENT_NO_CONSUMERS:
				PMTSA_LOG("Internal message: No consumers left");
				if (!stopSubscription())
				{
					PMTSA_ERR("Internal message: Stop subscription FAILED");
					// FIXME: What do we do here??
				}
				break;
			case PmInternalMsg::EVENT_TEST:
				PMTSA_DEBUG("PMT-SA: Internal message: TEST");
				break;
			case PmInternalMsg::EVENT_NEW_GPDATA:
				PMTSA_ERR("Internal message: NEW_GPDATA - FAILED");
				assert(!"This message ended up in the wrong place");
				break;
			default:
				PMTSA_ERR("Internal message: Default case - FAILED");
				assert(!"We sure don't have anything to do here!!");
			}
			delete msg;
		}
		delete poppedMsgs;
	}
	LEAVE_PMTSA();
}

/**
* C-interface for the run-method
*/
void* pmtsa_consumerThread(void* vPtr)
{
	ENTER_PMTSA();
	PmRunnable::instance()->run();
	LEAVE_PMTSA();
	return vPtr;
}
