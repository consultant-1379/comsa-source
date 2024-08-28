#ifndef CM_EVENT_PRODUCER_H
#define CM_EVENT_PRODUCER_H

/*******************************************************************************
* Copyright (C) 2010 by Ericsson AB
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
* File:   CmEventProducer.h
*
* Author: eaparob
*
* Date:   2013-02-06
*
* This file declares the needed functions and classes to use the
* EventProducer template class to be able to handle CM event producing
*
*******************************************************************************/
#include <stdlib.h>
#include <map>
#include "MafOamSpiCmEvent_1.h"
#include "EventProducer.h"
#include <RegularExpression.h>
#include "CmEventHandlerInterface.h"

class CmEventProducer;

class CmEventProducer
{
public:

    CmEventProducer(SaAisErrorT (*subscriberFunc)(void), SaAisErrorT (*unsubscriberFunc)(void));
    virtual ~CmEventProducer();

    virtual MafReturnT start(MafMgmtSpiInterfacePortal_3& portal);
    virtual MafReturnT stop(void);
    virtual MafReturnT sendEvent(MafOamSpiCmEvent_Notification_1T &mafCmNot);

#ifndef UNIT_TEST

private:

#endif
	class CmNotification :  public MafOamSpiCmEvent_Notification_1T, public Com::RefCounter
	    {
	    public:

		CmNotification(MafOamSpiCmEvent_Notification_1T &mafCmNot);

	        virtual ~CmNotification();

	        std::string toString() const;

	    private:

	    };

	/**
	 * The filter that evaluates the given regular expressions arrays.
	 * Each element of the array contains one regular expression.
	 */
	class RegexpFilter : public Com::FilterMatcher<CmNotification>
	{
	public:

		//RegexpFilter(MafNameValuePairT** filters) : Com::FilterMatcher<CmNotification>(filters) {}
		RegexpFilter(MafNameValuePairT** filters);
		~RegexpFilter();

		/**
		 * Check all the regular expressions in the array.
		 * By definition the logical operation for evaluating multiple regular expressions is "or" (at least one expression is matching).
		 *
		 * return true:
		 *               -if at least one of the expressions matches
		 *               -no expression provided (NULL filter, which is matching to everything)
		 */
		virtual bool isMatch(const CmNotification& filter) const;
		// returns true if all the filters in the filter array are valid regular expressions
		virtual bool isValid(void) const;
	private:
		// type definitions
		typedef std::pair<std::string, std::string> filterPair_T;
		typedef std::map<Com::RegularExpression*, filterPair_T > filterMap_T;
		typedef std::list<filterMap_T> filterList_T;
		typedef filterList_T::iterator filterListIterator_T;
		// the filter list
		mutable filterList_T filterList;
		// methods
		void addToFilterList(Com::RegularExpression *regExp, std::string filterName, std::string filterValue) const;
		void clearFilterList(void) const;

	}; // end of class RegexpFilter

#ifdef UNIT_TEST
	CmEventProducer::RegexpFilter* createRegexpFilterInstance(MafNameValuePairT** filters)
	    {
	    	RegexpFilter *regexpFilterInstance = new RegexpFilter(filters);
	    	return regexpFilterInstance;
	    };
	private:
#endif
	Com::EventProducer<CmNotification, MafOamSpiCmEvent_Notification_1T, RegexpFilter> _eventProducer;
	SaAisErrorT (*subscriberFunction)(void);
	SaAisErrorT (*unsubscriberFunction)(void);
};

// Support functions
void freeCmEvents(MafOamSpiCmEvent_1T** events);
void freeAVC(MafMoAttributeValueContainer_3T *avc);

#endif
