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
 * File: PmComComponent.cxx
 *
 * Author: ejonajo 2011-09-01
 *
 * Reviewed: efaiami 2012-04-14
 * Modified: xngangu 2012-08-30 Changed interface to ComOamSpiCmRouterService_3Id
 * Modified: xnikvap 2013-12-15 Converted to use COM PM SPI Ver.2
 * Modified: ejnolsz 2014-04-15 use new method getMillisecondsSinceEpochUnixTime() for marking PMT SA start and stop procedure limits
 *
 ************************************************************************** */

#include "PmComComponent.hxx"
#include "PmRunnable.hxx"
#include "PmEventHandler.hxx"
#include "PerfMgmtTransferSA.hxx"
#include "PmShowCounters.hxx"
#include "PmtSaTrace.hxx"

#include <ComMwSpiServiceIdentities_1.h>
#include <ComOamSpiServiceIdentities_1.h>

#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <cassert>

/**
 * @file PmComComponent.cxx
 *
 * @ingroup PmtSa
 *
 * @brief Implementation of the PmComComponent class.
 *
 * This is the implementation of the COM component interface in the PMT-SA.
 * This class is responsible for starting and stopping the component, and
 * to register the interfaces and services that we provide.
 */

using namespace PmtSa;

PmComComponent PmComComponent::s_instance; /*!< Ref to the singleton instance */

/**
 * @ingroup PmtSa
 *
 * Constructor method that sets all vital attributes.
 *
 */
PmComComponent::PmComComponent() :
	m_cmwLogInterface(NULL),
	m_eventRouterInterface(NULL),
	m_executingObject(NULL)
{
	ENTER_PMTSA();
	PMTSA_DEBUG("PmComComponent::PmComComponent() called");
	initComponent();
	LEAVE_PMTSA();
}

/**
 * @ingroup PmtSa
 *
 * Set up misc structures before we actually connect to COM.
 */
void PmComComponent::initComponent()
{
	ENTER_PMTSA();
	PMTSA_DEBUG("PmComComponent::initComponent() called");
	m_component.base = PMT_SA_COMP_ID;
	m_dependencyArray[0] = const_cast<ComMgmtSpiInterface_1T*>(&ComMwSpiLog_1Id);
	m_dependencyArray[1] = const_cast<ComMgmtSpiInterface_1T*>(&ComMwSpiTrace_1Id);
	m_dependencyArray[2] = const_cast<ComMgmtSpiInterface_1T*>(&ComOamSpiEventService_1Id);
	m_dependencyArray[3] = const_cast<ComMgmtSpiInterface_1T*>(&ComOamSpiCmRouterService_3Id);
	m_dependencyArray[4] = NULL;

	// Make sure we have the PM-libraries installed
	PmConsumerInterface* pci = PmConsumerInterface::instance();
	if (pci->foundSaPmConsumer())
	{
		PMTSA_LOG("Succeeded in loading PM-libraries");
		m_pmInterface.base = ComOamSpiPmInterface_2Id;
		m_pmInterface.base.componentName = "PmComComponent";
		m_pmInterface.getGp = ::pmtsa_getGpData;
		m_pmInterface.releaseGp = ::pmtsa_releaseGpData;
		m_interfaceArray[0] = reinterpret_cast<ComMgmtSpiInterface_1T*>(&m_pmInterface);

		if (!pci->isShowCounterV2Enabled()) {
			m_pmMeasInterface.base = ComOamSpiPmMeasurementsInterface_3Id;
			m_pmMeasInterface.base.componentName = "PmComComponent";
			m_pmMeasInterface.getMeasurementValues = ::pmtsa_getMeasurementValues;
			m_pmMeasInterface.getMeasurementNames = ::pmtsa_getMeasurementNames;
			m_pmMeasInterface.getPmJobIds = ::pmtsa_getPmJobIds;

			m_interfaceArray[1] = reinterpret_cast<ComMgmtSpiInterface_1T*>(&m_pmMeasInterface);
		}
		else {
			m_pmMeasInterface_2.base = ComOamSpiPmMeasurementsInterface_4Id;
			m_pmMeasInterface_2.base.componentName = "PmComComponent";
			m_pmMeasInterface_2.getMeasurementValues = ::pmtsa_getMeasurementValues_2;
			m_pmMeasInterface_2.cancelGetMeasurementValues = ::pmtsa_cancelGetMeasurementValues;
			m_pmMeasInterface_2.getMeasurementNames = ::pmtsa_getMeasurementNames_2;
			m_pmMeasInterface_2.getPmJobIds = ::pmtsa_getPmJobIds_2;
			m_pmMeasInterface_2.getPmGroupIds = ::pmtsa_getPmGroupIds;
			m_pmMeasInterface_2.getMeasuredObjects = ::pmtsa_getMeasuredObjects;
			m_pmMeasInterface_2.getMeasurementTypeDescription = ::pmtsa_getMeasurementTypeDescription;
			m_pmMeasInterface_2.getGroupIdDescription = ::pmtsa_getGroupIdDescription;

			m_interfaceArray[1] = reinterpret_cast<ComMgmtSpiInterface_1T*>(&m_pmMeasInterface_2);
		}
		//Added interface for the pmCollectionTimer feature to handle parallel thread request of getGpData_2 and releaseGpData_2 for each Gp
		m_pmInterface_2.base = ComOamSpiPmInterface_2_1Id;
                m_pmInterface_2.base.componentName = "PmComComponent";
                m_pmInterface_2.getGp2 = ::pmtsa_getGpData_2;
                m_pmInterface_2.releaseGp2 = ::pmtsa_releaseGpData_2;
                m_interfaceArray[2] = reinterpret_cast<ComMgmtSpiInterface_1T*>(&m_pmInterface_2);
		m_interfaceArray[3] = NULL;
	}
	else
	{
		// We're running on a system without any PM implementation
		PMTSA_WARN("Failed loading PM-libraries");
		m_interfaceArray[0] = NULL;
		m_interfaceArray[1] = NULL;
	}

	m_component.interfaceArray = m_interfaceArray;
	m_component.dependencyArray = m_dependencyArray;
	m_component.start = ::pmtsa_start;
	m_component.stop = ::pmtsa_stop;
	LEAVE_PMTSA();
}

/**
 * @ingroup PmtSa
 *
 * Destructor, makes sure that underlying objects are stopped and killed before termination.
 *
 */
PmComComponent::~PmComComponent()
{
	ENTER_PMTSA();
	PMTSA_LOG("Destroying component");
	PmEventHandler* pmEvHdlr = PmEventHandler::instance();
	if (pmEvHdlr != NULL)
	{
		pmEvHdlr->stop();
		delete pmEvHdlr;
	}
	if (m_executingObject != NULL)
	{
		if (m_executingObject->running())
		{
			m_executingObject->stop();
		}
		delete m_executingObject;
		m_executingObject = NULL;
	}
	LEAVE_PMTSA();
}

/**
 * @ingroup PmtSa
 *
 * The component starts providing service once this method is called. Currently
 * we don't care about what 'reason' we get as in parameter.
 *
 * @param[in]	reason	Start state code from COM
 *
 * @return	ComReturnT	ComOk if we managed to start as expected
 * @return	ComReturnT	ComFailure if we couldn't start
 */
ComReturnT PmComComponent::pmtsa_start(ComStateChangeReasonT reason)
{
	ENTER_PMTSA();
	PMTSA_LOG("Received PmComComponent::pmtsa_start() call");
	/*
	 * Try to connect to the Log interface so that we might see what happens in here
	 */
	m_cmwLogInterface = reinterpret_cast<ComMwSpiLog_1T*>(PerfMgmtTransferSA::instance().interface(ComMwSpiLog_1Id));
	/*
	 * Make sure things are set up the way they're supposed to
	 */
	initComponent();
	/*
	 * The initComponent method will verify if we have PM-libs on
	 * the system.	If there are no such thing, we shouldn't do more
	 * than saying that we're up and running, but we won't provide any
	 * particular services to the system.
	 */
	if (PmConsumerInterface::instance()->foundSaPmConsumer() == true)
	{
		m_executingObject = new PmRunnable();
		if (m_executingObject != NULL)
		{
			if (m_executingObject->run() != true)
			{
				PMTSA_ERR("Couldn't start thread for accessing PM-services");
				LEAVE_PMTSA();
				return ComFailure;
			}
			else
			{
				PMTSA_LOG("Thread up and running, ready to start consuming");
			}
			// Get the event routing interface
			PerfMgmtTransferSA& pmtSa = PerfMgmtTransferSA::instance();
			m_eventRouterInterface = reinterpret_cast<ComOamSpiEventRouter_1T*>(pmtSa.interface(ComOamSpiEventService_1Id));
			// The event handler/producer can now be started
			PmEventHandler* pmEvHdlr = new PmEventHandler();
			if (pmEvHdlr == NULL)
			{
				PMTSA_ERR("Couldn't start event handling services for PM");
				LEAVE_PMTSA();
				return ComFailure;
			}
			pmEvHdlr->start(&s_instance);
		}
		else
		{
			PMTSA_ERR("Couldn't create thread-object");
			LEAVE_PMTSA();
			return ComFailure;
		}
	}
	else
	{
		PMTSA_WARN("***********************************************************************");
		PMTSA_WARN("WARNING: PM-libraries NOT FOUND on system, no PM-data will be available");
		PMTSA_WARN("***********************************************************************");
	}
	PMTSA_LOG("PmComComponent::pmtsa_start() started OK");
	LEAVE_PMTSA();
	return ComOk;
}

/**
 * @ingroup PmtSa
 *
 * The component stops providing service once this method is called.
 *
 * @param[in]	reason	Stop code from COM
 *
 * @return	ComReturnT	Will always return ComOk!
 */
ComReturnT PmComComponent::pmtsa_stop(ComStateChangeReasonT reason)
{
	ENTER_PMTSA();
	PMTSA_LOG("Received PmComComponent::pmtsa_stop() stop call");
	PmEventHandler* pmEvHdlr = PmEventHandler::instance();
	if (pmEvHdlr != NULL)
	{
		PMTSA_DEBUG("PmComComponent::pmtsa_stop() pmEvHdlr = Not Null");
		pmEvHdlr->stop();
		delete pmEvHdlr;
	}
	if (m_executingObject != NULL)
	{
		PMTSA_DEBUG("PmComComponent::pmtsa_stop() m_executingObject = Not Null");
		if (m_executingObject->running())
		{
			PMTSA_DEBUG("PmComComponent::pmtsa_stop() m_executingObject->running = ok");
			int naps = 0;
			while ((m_executingObject->libsConnected() == true) && (naps < 1500))
			{
				if (naps == 0)
				{
					PMTSA_LOG("Waiting for PM libs to be disconnected");
				}
				++naps;
				usleep(1000);
			}
			m_executingObject->stop();
			PMTSA_DEBUG("PmComComponent::pmtsa_stop() m_executingObject->stop = ok");
			delete m_executingObject;
			m_executingObject = NULL;
		}
	}
	// There is an instance of the ConsumerInterface floating around,
	// make sure it's removed before termination.
	PmConsumerInterface* pic = PmConsumerInterface::instance();
	delete pic;
	PMTSA_LOG("Stopped execution of component");
	LEAVE_PMTSA();
	return ComOk;
}

/**
 *	C wrapper function that starts up the PMT-SA component via
 *	PmtSa::PmComComponent::pmtsa_start()
 *
 * @param[in]	r			COM state change reason
 *
 * @return	ComReturnT	Tells caller if we're OK or not
 *
 */
ComReturnT pmtsa_start(ComStateChangeReasonT r)
{
	ENTER_PMTSA();
	PMTSA_DEBUG("pmtsa_start() called");
	PMTSA_LOG("pmtsa_start(): PMT SA component start procedure begins: %llu", getMillisecondsSinceEpochUnixTime());
	ComReturnT retVal = PmComComponent::instance().pmtsa_start(r);
	PMTSA_LOG("pmtsa_start(): PMT SA component start procedure completed: %llu", getMillisecondsSinceEpochUnixTime());
	LEAVE_PMTSA();
	return retVal;
}

/**
 *	C wrapper function that shuts down the PMT-SA component via
 *	PmtSa::PmComComponent::pmtsa_stop()
 *
 * @param[in]	r			COM state change reason
 *
 * @return	ComReturnT	Tells caller if we're OK or not
 *
 */
ComReturnT pmtsa_stop(ComStateChangeReasonT r)
{
	ENTER_PMTSA();
	PMTSA_LOG ("pmtsa_stop(): PMT SA component stop procedure begins: %llu", getMillisecondsSinceEpochUnixTime());
	PMTSA_DEBUG("pmtsa_stop() called");
	ComReturnT retVal = PmComComponent::instance().pmtsa_stop(r);
	PMTSA_LOG ("pmtsa_stop(): PMT SA component stop procedure completed: %llu", getMillisecondsSinceEpochUnixTime());
	LEAVE_PMTSA();
	return retVal;
}
