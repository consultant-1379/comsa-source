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
 * File: PerfMgmtTransferSA.hxx
 *
 * Author: ejonajo 2011-09-01
 *
 * Reviewed: efaiami 2012-04-14
 *
 * Modified: ejnolsz 2014-04-15 use new method getMillisecondsSinceEpochUnixTime() for marking PMT SA start and stop procedure limits
 *
 * ************************************************************************ */

#ifndef PERF_MGMT_TRANSFER_SA_HXX__
#define PERF_MGMT_TRANSFER_SA_HXX__

#include "PmtSaTrace.hxx"

/*
 * Include required COM header-files
 */
#include "ComMgmtSpiCommon.h"
#include "ComMgmtSpiComponent_1.h"
#include "ComMgmtSpiInterfacePortal_1.h"
#include "ComMgmtSpiInterfacePortalAccessor.h"

// System include files
#include <string>
#include <libxml2/libxml/parser.h>

/**
 * @file PerfMgmtTransferSA.hxx
 *
 * @ingroup PmtSa
 *
 * @brief This is the interface of the Performance Management service agent.
 *
 * This class is responsible for handling load/unload of the so-library that is
 * the interface towards PM (Performance Management) in OpenSAF/CoreMW for COM.
 */

namespace PmtSa
{

/**
 * @ingroup PmtSa
 *
 * @brief This class handles load/unload of the component in COM.
 *
 * This is the starting point for the PMT-SA component. This class is responsible for
 * handling load and unload of the component in the COM SPI framework, thus it holds
 * the two required methods comLCMinit() and comLCMterminate().
 */
class PerfMgmtTransferSA {
public:
	/**
     * @ingroup PmtSa
     *
     * Destructor
     *
     */
	virtual ~PerfMgmtTransferSA();

	// --------------------------------------------
	// Required methods for a COM component library
	/**
     * @ingroup PmtSa
     *
     * This method should be called when the component is initially
     * loaded by COM. It will register the service provider interfaces
     * that PMT-SA provides, and also tell COM about what dependencies
     * it has.
     *
     * @param[in] 		accessor 	Pointer to COM portal
     * @param[in] 		config      Path to configuration file
     *
     * @return 	ComReturnT          Result of initialization
     *
     */
	virtual ComReturnT comLCMinit(ComMgmtSpiInterfacePortalAccessorT* accessor, const char* config);

	/**
     * @ingroup PmtSa
     *
     * This method should be called when the component should be
     * unloaded from COM. It will un-register all interfaces from the portal.
     *
     * @return 	ComReturnT	Result of un-registration
     *
     */
	virtual ComReturnT comLCMterminate();
	// ------------------------------------

	/**
     * @ingroup PmtSa
     *
     * Access to the portal received from COM upon startup.
     *
     * @return 	ComMgmtSpiInterfacePortal_1T*	Pointer to COM portal
     *
     */
	virtual ComMgmtSpiInterfacePortal_1T* portal();

	/**
     * @ingroup PmtSa
     *
     * Access to the singleton instance of the component
     *
     * @return 	PerfMgmtTransferSA&     Reference to the component
     *
     */
	static PerfMgmtTransferSA& instance();

	/**
     * @ingroup PmtSa
     *
     * Used by this component so that interfaces from other SPI's can
     * be fetched easily.
     *
     * @param[in] 		ifName  Interface name that we're looking for
     *
     * @return 	ComMgmtSpiInterface_1T*		Pointer to foreign interface.
     *
     */
	virtual ComMgmtSpiInterface_1T* interface( const ComMgmtSpiInterface_1T& ifName );

protected:
	/**
	 * @ingroup PmtSa
	 *
	 * Constructor that shouldn't be called by others, the instance() method should
	 * be used to get hold of this object.
	 */
	PerfMgmtTransferSA();

private:
	const std::string& portalVersion();

	ComMgmtSpiInterfacePortal_1T* m_portal; /*!< Holds pointer to COM portal */

	static PerfMgmtTransferSA s_instance;   /*!< Pointer to singleton instance */
	const std::string PortalVersion;        /*!< Provided as argument when we register in COM */
	std::string m_configuration;            /*!< Name of our configuration, provided by COM */
	xmlParserCtxtPtr m_xmlParserCtxtPtr;    /*!< Pointer to XML-parser */
	xmlDocPtr m_xmlDocPtr;                  /*!< Pointer to XML-document */
};

/**
 * @ingroup PmtSa
 *
 * Method to get access to the portal version
 *
 * @return 	const std::string&		String holding portal version
 *
 */
inline const std::string& PerfMgmtTransferSA::portalVersion()
{
	ENTER_PMTSA();
	LEAVE_PMTSA();
	return PortalVersion;
}

}

#ifdef  __cplusplus
 extern "C"
 {
 #endif
    /**
    * C interface for generating time printouts to the test framework
    */
    unsigned long long getMillisecondsSinceEpochUnixTime();

    /**
     * C interface for PmtSa::PerfMgmtTransferSA::comLCMinit()
     */
    ComReturnT comLCMinit(ComMgmtSpiInterfacePortalAccessorT* ac, const char* conf);

    /**
     * C interface for PmtSa::PerfMgmtTransferSA::comLCMterminate()
     */
    ComReturnT comLCMterminate();
#ifdef  __cplusplus
}
#endif
#endif /* PERF_MGMT_TRANSFER_SA_HXX__ */
