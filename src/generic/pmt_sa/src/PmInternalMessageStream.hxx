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
 * File: PmInternalMessageStream.hxx
 *
 * Author: ejonajo 2011-09-01
 *
 * Reviewed: efaiami 2012-04-14
 *
 ************************************************************************** */
#ifndef PMINTERNALMESSAGESTREAM_HXX_
#define PMINTERNALMESSAGESTREAM_HXX_

#include "PmtSaTrace.hxx"

#include <list>
#include <string>
#include <pthread.h>

/**
 * @file PmInternalMessageStream.hxx
 *
 * @ingroup PmtSa
 *
 * @brief "Pipe"-class to handle communication between thread and component.
 *
 * Small and practical class that makes sure that we can send messages thread-
 * safe in an ordered way.
 */

namespace PmtSa {

/**
 * Forward declare ...
 */
class PmInternalMsg;

/**
 * @ingroup PmtSa
 *
 * @brief Handles internal messages in PMTSA.
 *
 * This class defines the interface for the internal message passing in
 * PMTSA.  The delivery is thread-safe and ordered FIFO.
 */
class PmInternalMessageStream {
public:
    /**
     * @ingroup PmtSa
     *
     * Constructor method
     *
     * @param[in] 	streamName  The name of the stream
     */
	PmInternalMessageStream(std::string streamName);

	/**
	 * @ingroup PmtSa
	 *
	 * Destructor
	 */
	~PmInternalMessageStream();

	/**
     * @ingroup PmtSa
     *
     * Push a message to the stream, it will end up "last-in-line".
     *
     * @param[in] 		message     Pointer to message
     */
	void push(PmInternalMsg* message);

	/**
	 * @ingroup PmtSa
	 *
	 * Pop <b>all</b> messages from the stream.
	 *
	 * @return  std::list<PmInternalMsg*>*  A pointer to a list of messages
	 */
	std::list<PmInternalMsg*>* pop();

	/**
	 * @ingroup PmtSa
	 *
	 * Predicate that tells if there's something in the stream or not.
	 *
	 * @return  bool    false if there are messages, true if empty
	 */
	const bool empty();

	/**
	 * @ingroup PmtSa
	 *
	 * Gives the name of the stream
	 *
	 * @return std::string& A const ref to the name of the stream
	 */
	const std::string& name() const;

	/**
	 * @ingroup PmtSa
	 *
	 * Give the stream a new name
	 *
	 * @param[in]   n   The (new) name of the stream
	 */
	void name(std::string n);

	/**
	 * @ingroup PmtSa
	 *
	 * Returns the read handle of this stream
	 *
	 * @return int  The read handle
	 */
	const int readHandle() const;

    /**
     * @ingroup PmtSa
     *
     * Returns the write handle of this stream
     *
     * @return int  The write handle
     */
	const int writeHandle() const;

private:
	// Declared but not implemented
	PmInternalMessageStream();

	void signal();
	void clear();
	std::string m_name;
	std::list<PmInternalMsg*> m_messages;
	pthread_mutex_t m_mutex;
	int m_pipeFds[2];
};

inline const std::string& PmInternalMessageStream::name() const
{
	ENTER_PMTSA();
	LEAVE_PMTSA();
	return m_name;
}

inline const int PmInternalMessageStream::readHandle() const
{
	ENTER_PMTSA();
	LEAVE_PMTSA();
	return m_pipeFds[0];
}

inline const int PmInternalMessageStream::writeHandle() const
{
	ENTER_PMTSA();
	LEAVE_PMTSA();
	return m_pipeFds[1];
}

} /* namespace PmtSa */
#endif /* PMINTERNALMESSAGESTREAM_HXX_ */
