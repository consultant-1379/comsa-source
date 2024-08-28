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
 * File: PmInternalMsg.cxx
 *
 * Author: ejonajo 2011-09-01
 *
 * Reviewed: efaiami 2012-04-14
 *
 ************************************************************************** */
#include "PmInternalMsg.hxx"

/**
 * @file PmInternalMsg.cxx
 *
 * @brief Implementation of the PmInternalMsg class.
 *
 * The internal messages sent from/to the event handling class to
 * the thread that listens for PM events are implemented here.
 */

namespace PmtSa {

/**
 * @ingroup PmtSa
 *
 * Constructor for the base-class
 *
 * @param[in] 	pmMessType	The kind of message we're creating
 *
 * @return 	PmInternalMessage	Base-class object for internal messages
 *
 */
PmInternalMsg::PmInternalMsg(PmMessageTypeT pmMessType) :
	m_pmMessageType(pmMessType)
{
    ENTER_PMTSA();
    LEAVE_PMTSA();
}

/**
 *  The destructor is declared as pure virtual in the interface, thus
 *  this one shouldn't be used ...
 */
PmInternalMsg::~PmInternalMsg()
{
    ENTER_PMTSA();
    LEAVE_PMTSA();
}
} /* namespace PmtSa */
