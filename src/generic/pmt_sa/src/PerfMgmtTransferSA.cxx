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
* File: PerfMgmtTransferSA.cxx
*
* Author: ejonajo 2011-09-01
*
* Reviewed: efaiami 2012-04-14
*
* Modify: ejnolsz 2014-04-15 add and use new method getMillisecondsSinceEpochUnixTime() for marking PMT SA start and stop procedure limits
* Modify: xdonngu 2014-07-15 Using timer and signal handler for polling the trace configuration files
* Modify: xdonngu 2014-07-15 Using shared memory to transfer trace_flag value from comsa lib to pmtsa lib instead of polling "hard code path" file
*
* ************************************************************************ */

#define TRACEPOINT_DEFINE
#define TRACEPOINT_PROBE_DYNAMIC_LINKAGE

#include "PerfMgmtTransferSA.hxx"
#include "PmComComponent.hxx"
#include "PmtSaTrace.hxx"
#include "OamSATranslator.h"
#include "OamSATransactionalResource.h"

#include <ComMwSpiServiceIdentities_1.h>

#include <iostream>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <pthread.h>
#include <fstream>
#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>

#ifdef CODE_COVERAGE
// #include <gcc/gcov-io.h>  /* The prototype below should be in this header file */
#ifdef __cplusplus
extern "C" void __gcov_flush(); /* define here until we find the header file */
#endif
#endif
// Forward declarations of trace functions
extern void log_to_file(const char* logfile);
extern void log_init (const char *ident);
/**
* @file PerfMgmtTransferSA.cxx
*
* @brief This is the implementation of the PMT-SA component interface.
*
* This class is responsible for start and stop of the PMT-SA component.
* That gives that when COM calls on the comLCMinit()-method, the component
* should load its configuration (that is passed on with the call) and
* then register the interfaces it provides in the portal so that COM knows
* what services this component provides.
* When COM calls on the comLCMterminate()-method, the component should
* gracefully terminate itself.
*/

using namespace PmtSa;

#define SECONDS_TO_SLEEP 20
#define NANOSECONDS_TO_SLEEP 0

int trace_flag = 0;
timer_t timerid;

#define TIMER_SIG (SIGRTMIN+1)

// Use the implementation date for shared memory key
key_t shm_key = (key_t)20140715;
int *ptr_trace_flag = NULL;
int segment_id = 0;

static void timerHandlerCheckTraceFlag(int sig, siginfo_t *si, void *uc)
{
	//update trace_flag from shared memory
	if(ptr_trace_flag)
	{
		trace_flag = *ptr_trace_flag;
	}
}

PerfMgmtTransferSA PerfMgmtTransferSA::s_instance;

/**
* @ingroup PmtSa
*
* Default constructor method for the PerfMgmtTransferSA object.
*
*/
PerfMgmtTransferSA::PerfMgmtTransferSA() :
m_portal(NULL), PortalVersion("1"), m_xmlParserCtxtPtr(NULL),
m_xmlDocPtr(NULL)
{
	ENTER_PMTSA();
	PMTSA_DEBUG("PerfMgmtTransferSA::PerfMgmtTransferSA() called");
	m_configuration.clear();
	LEAVE_PMTSA();
}

/**
* @ingroup PmtSa
*
* Default destructor
*/
PerfMgmtTransferSA::~PerfMgmtTransferSA()
{
	ENTER_PMTSA();
	PMTSA_DEBUG("PerfMgmtTransferSA::~PerfMgmtTransferSA() called");
	LEAVE_PMTSA();
}

/**
* @ingroup PmtSa
*
* The method is responsible for registering all the services that the
* component provides for COM, and also to register all dependencies so
* that COM can start up the components in the right order.
*
* @param[in]           accessor        Pointer to the COM portal
* @param[in]           config      Pointer to configuration file
*
* @return      ComReturnT              ComFailure - On non-recoverable errors, basically bad input
* @return  ComReturnT      ComTryAgain - When the portal couldn't be fetched via the
*                                        provided accessor pointer
* @return  ComReturnT      ComOk - When the component manages to register itself in the
*                                  COM portal
*
*/
ComReturnT PerfMgmtTransferSA::comLCMinit(ComMgmtSpiInterfacePortalAccessorT* accessor, const char* config)
{
	// Make sure that we save the configuration at least, don't know what it's used
	// for right now. TODO: Find out!
	ENTER_PMTSA();
	PMTSA_LOG("PerfMgmtTransferSA::comLCMinit() called with config (%s)",config);
	m_configuration = (config != NULL ? config : "");
	if (!m_configuration.empty())
	{
		// Load the configuration and try to parse it
#ifndef UNIT_TEST
		m_xmlParserCtxtPtr = xmlNewParserCtxt();
#else
		m_xmlParserCtxtPtr = NULL;
#endif
		if (m_xmlParserCtxtPtr)
		{
			m_xmlDocPtr = xmlCtxtReadMemory(m_xmlParserCtxtPtr,
			m_configuration.c_str(), m_configuration.length(), "config",
			NULL, 0);
			if (m_xmlDocPtr == NULL)
			{
				// Couldn't parse configuration
				PMTSA_ERR("Couldn't parse configuration");
				LEAVE_PMTSA();
				return ComFailure;
			}
		}
		else
		{
			PMTSA_ERR("Couldn't create XML-parser context");
			LEAVE_PMTSA();
			return ComFailure;
		}
	}
	// Initilize Log Stream
	if (false == PmtSaTrace::Instance().initLogStream()) {
		syslog(LOG_USER*8 +LOG_WARNING, "%s", "PerfMgmtTransferSA::comLCMinit() : Unable to initialize LOG Stream. Further logs might not be logged.");
	}
	/* Allocate a shared memory segment. */
	segment_id = shmget(shm_key, sysconf(_SC_PAGESIZE), IPC_CREAT | 0666);
	if(segment_id < 0)
	{
		PMTSA_ERR("comLCMinit: can't get shared memory, segment_id = %d, errno: %d, pmtsa trace will be disabled!", segment_id, errno);
	}
	else
	{
		/* Attach the shared memory segment. */
		ptr_trace_flag = (int*) shmat(segment_id, NULL, 0);
		if(ptr_trace_flag == (int *) -1)
		{
			PMTSA_ERR("comLCMinit: can't attach to shared memory, segment_id = %d, errno: %d, pmtsa trace will be disabled!", segment_id, errno);
			ptr_trace_flag = NULL;
		}
	}
#ifndef UNIT_TEST
	// If get shared memory successful, setup a timer for polling shared value.
	if(ptr_trace_flag)
	{
		/* timer variables */
		struct sigevent sev;
		struct itimerspec its;
		sigset_t mask;
		struct sigaction sa;
		struct sigaction old_sa;
		/* Establish handler for timer signal */
		sa.sa_flags = SA_SIGINFO;
		sa.sa_sigaction = timerHandlerCheckTraceFlag;
		sigemptyset(&sa.sa_mask);
		if (sigaction(TIMER_SIG, &sa, &old_sa) == -1)
		{
			PMTSA_DEBUG("sigaction err");
		}
		if(SIG_DFL == old_sa.sa_handler)
		{
			/* Block timer signal temporarily */
			sigemptyset(&mask);
			sigaddset(&mask, TIMER_SIG);
			if (sigprocmask(SIG_SETMASK, &mask, NULL) == -1)
			{
				PMTSA_DEBUG("sigprocmask lock error: %d", errno);
			}
			/* Create the timer */
			sev.sigev_notify = SIGEV_SIGNAL;
			sev.sigev_signo = TIMER_SIG;
			sev.sigev_value.sival_ptr = &timerid;
			if (timer_create(CLOCK_REALTIME, &sev, &timerid) == -1)
			{
				PMTSA_DEBUG("timer_create error: %d", errno);
			}
			/* Start the timer */
			its.it_value.tv_sec = SECONDS_TO_SLEEP;
			its.it_value.tv_nsec = NANOSECONDS_TO_SLEEP;
			its.it_interval.tv_sec = its.it_value.tv_sec;
			its.it_interval.tv_nsec = its.it_value.tv_nsec;
			if (timer_settime(timerid, 0, &its, NULL) == -1)
			{
				PMTSA_DEBUG("timer_settime error: %d", errno);
			}
			/* Unlock the timer signal, so that timer notification
			can be delivered */
			PMTSA_DEBUG("Unblocking signal %d", TIMER_SIG);
			if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1)
			{
				PMTSA_DEBUG("sigprocmask unlock error: %d", errno);
			}
		}
		else
		{
			PMTSA_ERR("TIMER: old action for signal (SIGRTMIN+1) (%d) is not SIG_DFL (0): %lld, can't register signal handler for pmtsa trace config polling, pmtsa trace will be disabled.",
			TIMER_SIG, (long long)old_sa.sa_handler);
			sigaction(TIMER_SIG, &old_sa, NULL);
		}
		struct log_state_T log;
		// Initialize member only if trace
		log.level = LOG_LEVEL_DEBUG;
		log.tags = TRACE_TAG_LOG | TRACE_TAG_ENTER | TRACE_TAG_LEAVE;
		log.mode = LOG_MODE_FILE | LOG_MODE_FILELINE | LOG_MODE_TIMESTAMP;

		log_control( &log, 0 );
		log_to_file("/var/opt/comsa/pmtsa.trc");
		log_init("PMT_SA");
	}

#endif
	if (accessor != NULL)
	{
		m_portal = static_cast<ComMgmtSpiInterfacePortal_1T*>(accessor->getPortal(portalVersion().c_str()));
		assert(m_portal != NULL && "Couldn't fetch portal via accessor");
		if (m_portal != NULL)
		{
			PMTSA_LOG("Registering PmComponent in portal");
			ComReturnT regResult = m_portal->registerComponent(&PmComComponent::instance().component());
			PMTSA_LOG("Result of registration of PMT-SA = %d (%s)", regResult, (regResult == ComOk ? "OK" : "FAILED"));
			LEAVE_PMTSA();
			return regResult;
		}
		else
		{
			PMTSA_ERR("Couldn't access portal to register component");
			LEAVE_PMTSA();
			return ComTryAgain;
		}
	}
	else
	{
		PMTSA_ERR("Portal accessor not set");
		LEAVE_PMTSA();
		return ComFailure;
	}
}

/**
* @ingroup PmtSa
*
* This method unregisters the component from COM.
*
* @return      ComReturnT      ComOk - On success, otherwise fault-code from unregisterComponent()
*
*/
ComReturnT PerfMgmtTransferSA::comLCMterminate()
{
	ENTER_PMTSA();
	ComReturnT res = ComOk;
	PMTSA_LOG("PerfMgmtTransferSA::comLCMterminate() called");

#ifndef UNIT_TEST
	if(ptr_trace_flag)
	{
		shmdt(ptr_trace_flag);
		timer_delete(timerid);
		// marked shared memory should be deallocated when no thread attach to it.
		shmctl(segment_id, IPC_RMID, 0);
	}
#endif

	if(m_portal != NULL)
	{
		PMTSA_DEBUG("PerfMgmtTransferSA::comLCMterminate() m_portal!=NULL");
		res = m_portal->unregisterComponent(&PmComComponent::instance().component());
		if (m_xmlDocPtr)
		{
			xmlFreeDoc(m_xmlDocPtr);
			m_xmlDocPtr = NULL;
		}
		if (m_xmlParserCtxtPtr)
		{
			xmlFreeParserCtxt(m_xmlParserCtxtPtr);
			m_xmlParserCtxtPtr = NULL;
		}
		m_configuration.clear();
		PMTSA_LOG("Unregistered PMT-SA from COM");
	}
	else
	{
		PMTSA_ERR("Portal is NULL, can't terminate.");
		res = ComFailure;
	}
	PMTSA_LOG("PerfMgmtTransferSA::comLCMterminate() res = %d", res);

	PmtSaTrace::Instance().finalizeLogStream();

	LEAVE_PMTSA();
	return res;
}

/**
* @ingroup PmtSa
*
* Returns a pointer to COM service provider interface portal.
*
* @return      ComMgmtSpiInterfacePortal_1T*   Pointer to COM SPI portal
*
* @par Globals: Write a description here or remove if not used!
*
*/
ComMgmtSpiInterfacePortal_1T* PerfMgmtTransferSA::portal()
{
	ENTER_PMTSA();
	PMTSA_DEBUG("PerfMgmtTransferSA::portal() called");
	LEAVE_PMTSA();
	return m_portal;
}

/**
* @ingroup PmtSa
*
* Returns pointer to the (singleton) instance of the component.
*
* @return      PerfMgmtTransferSA& Reference to the singleton instance of the component
*
*/
PerfMgmtTransferSA& PerfMgmtTransferSA::instance()
{
	ENTER_PMTSA();
	PMTSA_DEBUG("PerfMgmtTransferSA::instance() called");
	LEAVE_PMTSA();
	return s_instance;
}

/**
* @ingroup PmtSa
*
* Used by this component to fetch other interfaces by name from the COM SPI.
*
* @param[in]   ifName  Name of the interface that we want access to
*
* @return      ComMgmtSpiInterface_1T*         Pointer to the requested interface (or NULL)
*
* @par Globals: Write a description here or remove if not used!
*
*/
ComMgmtSpiInterface_1T* PerfMgmtTransferSA::interface(const ComMgmtSpiInterface_1T& ifName)
{
	ENTER_PMTSA();
	ComMgmtSpiInterface_1T* ret_if;
	ComReturnT retval;
	PMTSA_DEBUG("PerfMgmtTransferSA::interface() called");
	retval = m_portal->getInterface((ComMgmtSpiInterface_1T) ifName, &ret_if);
	if (retval != ComOk)
	{
		PMTSA_WARN("PerfMgmtTransferSA::interface() getInterface failed");
		ret_if = NULL;
	}
	LEAVE_PMTSA();
	return ret_if;
}

/* --------------------------------------------------------------------------
* C-wrappers
* ----------------------------------------------------------------------- */
extern "C"
{
	/**
	* @ingroup PmtSa
	*
	* C wrapper function for the PerfMgmtTransferSA::comLCMinit() method.
	*
	*/
	ComReturnT comLCMinit(ComMgmtSpiInterfacePortalAccessorT* ac, const char* conf)
	{
		ENTER_PMTSA();
		PMTSA_LOG ("comLCMinit(): PMT SA init procedure begins: %llu", getMillisecondsSinceEpochUnixTime());
		PMTSA_LOG("comLCMinit called");
		#ifdef BUILD_WITH_TRACE_TESTS
		PRINT_ALL_TRACE_DOMAIN_WITH_ALL_LEVELS_PMTSA();
		#endif
		ComReturnT retVal = PerfMgmtTransferSA::instance().comLCMinit(ac, conf);
		if(retVal != ComOk)
		{
			PMTSA_ERR("comLCMinit() failed");
		}
		else
		{
			PMTSA_LOG("comLCMinit ok");
		}
		PMTSA_LOG ("comLCMinit(): PMT SA init procedure completed: %llu", getMillisecondsSinceEpochUnixTime());
		LEAVE_PMTSA();
		return retVal;
	}


	/**
	* @ingroup PmtSa
	*
	* C wrapper function for the PerfMgmtTransferSA::comLCMterminate() method.
	*
	*/
	ComReturnT comLCMterminate()
	{
		ENTER_PMTSA();
		PMTSA_LOG("PMT_SA:comLCMterminate called.");
		PMTSA_LOG ("pmtsa_stop(): PMT SA terminate procedure begins: %llu", getMillisecondsSinceEpochUnixTime());

		#ifdef CODE_COVERAGE
		/* This is for Code Coverage testing only */
		PMTSA_LOG("PMT_SA:comLCMterminate: calling __gcov_flush()...");
		#ifdef __cplusplus
		__gcov_flush();
		#endif
		PMTSA_LOG("PMT_SA:comLCMterminate: after calling __gcov_flush()...");
		#endif

		ComReturnT retVal = PerfMgmtTransferSA::instance().comLCMterminate();
		if(retVal != ComOk)
		{
			PMTSA_ERR("comLCMterminate() failed");
		}
		else
		{
			PMTSA_LOG("PMT_SA:comLCMterminate OK");
		}

		PMTSA_LOG ("pmtsa_stop(): PMT SA terminate procedure completed: %llu", getMillisecondsSinceEpochUnixTime());
		LEAVE_PMTSA();
		return retVal;
	}


	unsigned long long getMillisecondsSinceEpochUnixTime()
	{
		ENTER_PMTSA();
		struct timeval tv;
		gettimeofday(&tv, NULL);
		unsigned long long millisecondsSinceEpoch = (unsigned long long)(tv.tv_sec) * 1000 + (unsigned long long)(tv.tv_usec) / 1000;
		LEAVE_PMTSA();
		return millisecondsSinceEpoch;
	}
}

