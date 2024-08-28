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
 * File: PmThreadTemplate.hxx
 *
 * Author: ejonajo 2011-09-01
 *
 * Reviewed: efaiami 2012-04-14
 *
 * ************************************************************************ */

#ifndef PMTHREADTEMPLATE_HXX_
#define PMTHREADTEMPLATE_HXX_

#include "PmtSaTrace.hxx"

#include <pthread.h>

/**
 * @file PmThreadTemplate.hxx
 *
 * @ingroup PmtSa
 *
 * @brief Template for using PThreads in C++
 *
 * Generic template to use when pthreads should be mixed with C++.  This is
 * the way to do it.
 */

namespace PmtSa {

/**
 * @ingroup PmtSa
 *
 * This template class handles PThreads in combination with C++.  With this,
 * it's easy to wrap a method within an object to be run as a thread.
 */
template <class T>
class PmThreadType {
public:
    /**
     * @ingroup PmtSa
     *
     * Constructor method
     *
     * @param[in] 		method  Reference to a method that should be run in a thread
     */
	PmThreadType(void (T::*method)());

	/**
     * @ingroup PmtSa
     *
     * Start the thread execution via a call to this method.
     *
     * @param[in] 		object  Pointer to the object (typically 'this').
     *
     * @return 	bool    true on success, false on failure
     *
     */
	bool run(T* object);

	/**
	 * @ingroup PmtSa
	 *
	 * Stop (kill) thread execution.
	 *
	 * @return  bool    true on success, false on failure
	 */
	bool terminate();

	/**
	 * @ingroup PmtSa
	 *
	 * 'Join' the thread with the parent.
     *
     * @return  bool    true on success, false on failure
	 */
	bool join();

	/**
	 * @ingroup PmtSa
	 *
	 * Get reference to the thread, ie the thread ID
	 *
	 * @return pthread_t    ID of thread
	 */
	pthread_t thread() const;

private:
	/**
	 * Declared but not implemented
	 */
	PmThreadType(const T&);
    /**
     * Declared but not implemented
     */
	T& operator=(const T&);

	/**
	 * Wrapper method that is used to actually kick off execution in the thread.
	 *
	 * @param[in]   thisPointer Pointer to 'this'
	 *
	 * @return  void*   Pointer to NULL
	 */
	static void* objectWrapper(void* thisPointer);

	/**
	 * Pointer to 'owning' object
	 */
	T* m_object;

	/**
	 * Pointer to method in 'owning' object that we should run
	 */
	void (T::*m_method)();

	/**
	 * PThread ID
	 */
	pthread_t m_thread;
};

template <class T>
PmThreadType<T>::PmThreadType(void (T::*method)()) :
	m_method(method),
	m_thread(0)
{
}

template <class T>
bool PmThreadType<T>::terminate()
{
	ENTER_PMTSA();
	if (pthread_self() == m_thread)
	{
		pthread_exit(0);
	}
	LEAVE_PMTSA();
	return (pthread_cancel(m_thread) == 0 ? true : false);
}

template <class T>
bool PmThreadType<T>::join()
{
	ENTER_PMTSA();
	LEAVE_PMTSA();
	return (pthread_join(m_thread, NULL) == 0 ? true : false);
}

template <class T>
pthread_t PmThreadType<T>::thread() const
{
	ENTER_PMTSA();
	LEAVE_PMTSA();
	return m_thread;
}

template <class T>
bool PmThreadType<T>::run(T* object)
{
	ENTER_PMTSA();
	m_object = object;
	LEAVE_PMTSA();
	return (pthread_create(&m_thread, NULL, &PmThreadType<T>::objectWrapper, this) != 0 ? false : true);
}

template <class T>
void* PmThreadType<T>::objectWrapper(void* thisPtr)
{
	ENTER_PMTSA();
	PmThreadType<T>* me = reinterpret_cast<PmThreadType<T>*>(thisPtr);
	// Kick off the specified method in the thread
	((me->m_object)->*(me->m_method))();
	LEAVE_PMTSA();
	return reinterpret_cast<void*>(0);
}
}

#endif /* PMTHREADTEMPLATE_HXX_ */
