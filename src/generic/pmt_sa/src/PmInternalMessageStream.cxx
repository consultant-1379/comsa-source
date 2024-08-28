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
 * File: PmInternalMessageStream.cxx
 *
 * Author: ejonajo 2011-09-01
 *
 * Reviewed: efaiami 2012-04-14
 *
 ************************************************************************** */
#include "PmInternalMessageStream.hxx"
#include "PmtSaTrace.hxx"
#include <unistd.h>
#include <cassert>

/**
 * @file PmInternalMessageStream.cxx
 *
 * @ingroup PmtSa
 *
 * @brief Implementation of the interface
 *
 */

using namespace std;

namespace PmtSa {

PmInternalMessageStream::PmInternalMessageStream(std::string streamName) :
	m_name(streamName)
{
    ENTER_PMTSA();
	pthread_mutex_init(&m_mutex, NULL);
	if (pipe(m_pipeFds) != 0)
	{
	    PMTSA_ERR("Couldn't create pipe!");
	}
    LEAVE_PMTSA();
}

PmInternalMessageStream::~PmInternalMessageStream()
{
    ENTER_PMTSA();
        close(m_pipeFds[0]);
        close(m_pipeFds[1]);
	pthread_mutex_destroy(&m_mutex);
    LEAVE_PMTSA();
}

const bool PmInternalMessageStream::empty()
{
    ENTER_PMTSA();
	bool isEmpty;
	pthread_mutex_lock(&m_mutex);
	isEmpty = m_messages.empty();
	pthread_mutex_unlock(&m_mutex);
    LEAVE_PMTSA();
	return isEmpty;
}


void PmInternalMessageStream::push(PmInternalMsg* message)
{
    ENTER_PMTSA();
	pthread_mutex_lock(&m_mutex);
	m_messages.push_back(message);
	signal();
	pthread_mutex_unlock(&m_mutex);
    LEAVE_PMTSA();
}

std::list<PmInternalMsg*>* PmInternalMessageStream::pop()
{
    ENTER_PMTSA();
	pthread_mutex_lock(&m_mutex);
	std::list<PmInternalMsg*>* listOfMsgs = NULL;
	if (!m_messages.empty())
	{
		listOfMsgs = new std::list<PmInternalMsg*>(m_messages);
		m_messages.clear();
		clear();
	}
	pthread_mutex_unlock(&m_mutex);
	LEAVE_PMTSA();
	return listOfMsgs;
}

/**
 * @ingroup PmtSa
 *
 * This method write one byte on the output file descriptor, telling
 * any user of this stream that now there's something to pop() from
 * it.
 *
 */
void PmInternalMessageStream::signal()
{
    ENTER_PMTSA();
	char signChar = 'S';
	assert(writeHandle() != 0);
	ssize_t ret = write(writeHandle(), &signChar, 1);
	if (ret != 1)
	{
	    // If we end up here, something is really rotten.
	    PMTSA_ERR("Failed writing to pipe!");
	}
    LEAVE_PMTSA();
}

/**
 * @ingroup PmtSa
 *
 * Clears the input file descriptor to this stream.
 *
 */
void PmInternalMessageStream::clear()
{
    ENTER_PMTSA();
	char buf[127];
	ssize_t ret;
	while (true)
	{
		// Clear that pipe
		ret = read(readHandle(), &buf, 126);
		if (ret > 0 && ret < 126)
		{
			break;
		}
		else if (ret < 0)
		{
			// Ouch, we're in trouble!
		    PMTSA_ERR("Read from pipe returned negative!");
		}
	}
    LEAVE_PMTSA();
}

} /* namespace PmtSa */
