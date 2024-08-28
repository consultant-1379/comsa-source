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
 * File: PmInternalMsg.hxx
 *
 * Author: ejonajo 2011-09-01
 *
 * Reviewed: efaiami 2012-04-14
 *
 ************************************************************************** */

#ifndef PMINTERNALMSG_HXX_
#define PMINTERNALMSG_HXX_

#include "PmtSaTrace.hxx"

/**
 * @file PmInternalMsg.hxx
 *
 * @ingroup PmtSa
 *
 * @brief Defines some internal messages for the component
 *
 * The different internal messages used inside PMTSA is defined in this
 * class, the messages can then be passed between the different parts of PMTSA
 * without worries for thread-issues etc.
 */

namespace PmtSa {

/**
 * @ingroup PmtSa
 *
 * @brief Base class for PMTSA internal messages
 *
 * Base class for the different kinds of messages that will be sent over
 * to/from thread.
 */
class PmInternalMsg {
public:
	/**
	 * We must be able to send some different messages to/from the thread.
	 */
	enum PmMessageTypeT
	{
		EVENT_CONSUMER_REGISTERED,	/*!< We must have consumers before starting to consume */
		EVENT_NO_CONSUMERS, 		/*!< Stop consuming when this comes */
		EVENT_NEW_GPDATA, 			/*!< A measurement period is finished, start collecting */
		EVENT_TEST 					/*!< Test */
	};

	/**
	 * @ingroup PmtSa
	 *
	 * Constructor for the possible messages sent between the thread
	 * and the rest of the "application"
	 *
	 * @param[in] 	pmMessType	The kind of message we want to create
	 *
	 * @return 	PmInternalMsg	A message object
	 */
	PmInternalMsg(PmMessageTypeT pmMessType);

	/**
	 * Virtual destructor, should be implemented by "user" of base class if
	 * sub-class contains more data than this basic class...
	 */
	virtual ~PmInternalMsg();

	/**
     * @ingroup PmtSa
     *
     * Returns the message type
     *
     * @return 	PmMessageTypeT  The message type
     *
     */
	const PmMessageTypeT pmMessageType() const;

private:
	// Default constructor defined but not implemented
	PmInternalMsg();

	/**
	 * Attribute describing what kind of message this is
	 */
	const PmMessageTypeT m_pmMessageType;
};

inline const PmInternalMsg::PmMessageTypeT PmInternalMsg::pmMessageType() const
{
	ENTER_PMTSA();
	LEAVE_PMTSA();
	return m_pmMessageType;
}
} /* namespace PmtSa */
#endif /* PMINTERNALMSG_HXX_ */
