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
 * File: PmComComponent.hxx
 *
 * Author: ejonajo 2011-09-01
 *
 * Reviewed: efaiami 2012-04-14
 * Modified: xnikvap 2013-12-15 Converted to use COM PM SPI Ver.2
 *
 ************************************************************************** */

#ifndef PMCOMCOMPONENT_HXX_
#define PMCOMCOMPONENT_HXX_

#include "PmtSaTrace.hxx"
// Com Spi
#include <ComMgmtSpiComponent_1.h>
#include <ComOamSpiEvent_1.h>
#include <ComMgmtSpiInterface_1.h>
#include <ComOamSpiPm_2.h>
#include <ComOamSpiPm_2_1.h>
#include <ComMwSpiLog_1.h>
#include "ComOamSpiPmMeasurements_3.h"
#include "ComOamSpiPmMeasurements_4.h"

// System includes
#include <pthread.h>

/**
 * @file PmComComponent.hxx
 *
 * @ingroup PmtSa
 *
 * @brief This file defines the required interfaces for starting and stopping the component.
 *
 * This file handles the registration of the Performance Management Transfer Service Agent,
 * ie it registers the interfaces needed for starting and stopping the component, and also
 * tells COM about our dependencies towards other components in the system.
 */

namespace PmtSa
{

/**
 * Identifies the PM component in COM.
 */
const ComMgmtSpiInterface_1T PMT_SA_COMP_ID = {"PmComComponent", "ComMgmtSpiComponent", "1"};

// Forward declare to remove dependency of include-file
class PmRunnable;

/**
 * @ingroup PmtSa
 *
 * @brief This class is the starting point for the components execution.
 *
 *	This class is the starting point for the components execution. In order to do so,
 *	it must implement the ComMgmtSpiComponent_1T interface. It also makes sure that
 *	the PM service interface methods are loaded and start the thread that reads data
 *	from the PM services. Finally, it creates the event service interface towards
 *	COM.
 *
 *	This class also handles stop of the component.
 */
class PmComComponent {
public:
	/**
	 * @ingroup PmtSa
	 *
	 * Destructor
	 */
	virtual ~PmComComponent();

	/**
	 * @ingroup PmtSa
	 *
	 * This is the start-method of the component.  When the component is created,
	 * it's not running, but when pmtsa_start is called the component publishes the
	 * PM-interfaces (actually we register that we produce PM-data) and then it starts
	 * waiting for interested parties.
	 *
	 * @param[in]		reason	Start-code from COM
	 *
	 * @return	ComReturnT	ComOk - on success
	 * @return	ComReturnT	ComFailure
	 *
	 */
	ComReturnT pmtsa_start(ComStateChangeReasonT reason);

	/**
	 * @ingroup PmtSa
	 *
	 * This method is called by COM when we should terminate execution.
	 *
	 * @param[in]	reason	Stop-code from COM
	 *
	 * @return	ComReturnT	ComOk - on success
	 *
	 */
	ComReturnT pmtsa_stop(ComStateChangeReasonT reason);

	/**
	 * @ingroup PmtSa
	 *
	 * Get a reference to the (static) component
	 *
	 * @return	PmComComponent& Reference to the component
	 *
	 */
	static PmComComponent& instance();

	/**
	 * @ingroup PmtSa
	 *
	 * Get a reference to the COM component data
	 *
	 * @return	ComMgmtSpiComponent_1T& Reference to the COM component
	 *
	 * @par Globals: Write a description here or remove if not used!
	 *
	 */
	ComMgmtSpiComponent_1T& component();

	/**
	 * @ingroup PmtSa
	 *
	 * Returns a pointer to the event routing interface that is
	 * needed by other classes.
	 *
	 * @return	ComOamSpiEventRouter_1T*	Pointer to event router interface
	 */
	virtual ComOamSpiEventRouter_1T* eventRouterInterface();

protected:
	/**
	 * @ingroup PmtSa
	 *
	 * Constructor method that is called only from the instance() method.
	 */
	PmComComponent();

private:
	/**
	 * Do all required initializations for the component
	 */
	void initComponent();

	// Pointer to self
	static PmComComponent s_instance;

	ComMgmtSpiComponent_1T m_component; /*!< Info required for COM */

	ComMgmtSpiInterface_1T* m_interfaceArray[4]; /*!< The interfaces we provide */
	ComMgmtSpiInterface_1T* m_dependencyArray[5]; /*!< Our dependencies */

	ComOamSpiPm_2T m_pmInterface; /*!< The PM interfaces */
	ComOamSpiPm_2_1T m_pmInterface_2;

	ComOamSpiPmMeasurements_3T m_pmMeasInterface; /*!< The PM measurement interface for show counters */
	ComOamSpiPmMeasurements_4T m_pmMeasInterface_2; /*!< The PM measurement interface for show counters with optional features enabled.*/

	ComMwSpiLog_1T* m_cmwLogInterface; /*!< Pointer to log interface in MW so that we can log stuff */

	/**
	 * Pointer to the event routing interface in COM
	 */
	ComOamSpiEventRouter_1T* m_eventRouterInterface;

	/**
	 * Pointer to the object that communicates with the PM-service.
	 */
	PmRunnable* m_executingObject;
};

inline PmComComponent& PmComComponent::instance()
{
	ENTER_PMTSA();
	LEAVE_PMTSA();
	return s_instance;
}

inline ComMgmtSpiComponent_1T& PmComComponent::component()
{
	ENTER_PMTSA();
	LEAVE_PMTSA();
	return m_component;
}

inline ComOamSpiEventRouter_1T* PmComComponent::eventRouterInterface()
{
	ENTER_PMTSA();
	LEAVE_PMTSA();
	return m_eventRouterInterface;
}

}

/**
 * C-interface of PmtSa::PmComComponent::pmtsa_start()
 */
extern "C" ComReturnT pmtsa_start(ComStateChangeReasonT r);

/**
 * C-interface of PmtSa::PmComComponent::pmtsa_start()
 */
extern "C" ComReturnT pmtsa_stop(ComStateChangeReasonT r);

#endif /* PMCOMCOMPONENT_HXX_ */
