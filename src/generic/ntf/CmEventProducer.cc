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
* File:   CmEventProducer.cc
*
* Author: eaparob
*
* Date:   2013-02-05
* Modify: xdonngu 2014-04-18: Fix memory leak.
* Modify: uabjoy 2014-03-24: Adopt to Trace CC.
*
*******************************************************************************/

#include "CmEventProducer.h"

static CmEventProducer* s_theCmEventProducer = NULL;
static bool alreadySubscribed = false;

CmEventProducer::CmEventProducer(SaAisErrorT (*subscriberFunc)(void), SaAisErrorT (*unsubscriberFunc)(void))
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("CmEventProducer::CmEventProducer(): ENTER");
	s_theCmEventProducer = this;
	subscriberFunction = subscriberFunc;
	unsubscriberFunction = unsubscriberFunc;
	alreadySubscribed = false;
	DEBUG_OAMSA_CMEVENT("CmEventProducer::CmEventProducer(): LEAVE");
	LEAVE_OAMSA_CMEVENT();
}
CmEventProducer::~CmEventProducer( )
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("CmEventProducer::~CmEventProducer(): ENTER");
	if(alreadySubscribed && unsubscriberFunction)
	{
		if(SA_AIS_OK == unsubscriberFunction())
		{
			alreadySubscribed = false;
		}
	}
	s_theCmEventProducer = NULL;
	DEBUG_OAMSA_CMEVENT("CmEventProducer::~CmEventProducer(): LEAVE");
	LEAVE_OAMSA_CMEVENT();
}

MafReturnT CmEventProducer::start(MafMgmtSpiInterfacePortal_3& portal)
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("CmEventProducer::start(): ENTER");
	_eventProducer.start(portal, MafOamSpiCmEvent_Notification_1);
	DEBUG_OAMSA_CMEVENT("CmEventProducer::start(): LEAVE");
	LEAVE_OAMSA_CMEVENT();
	return MafOk;
}

MafReturnT CmEventProducer::sendEvent(MafOamSpiCmEvent_Notification_1T &mafCmNot)
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("CmEventProducer::sendEvent(): ENTER");
	MafReturnT ret = MafOk;
	CmNotification* eventValue = new CmNotification(mafCmNot);
	_eventProducer.push(*eventValue);
	eventValue->releaseRef();
	DEBUG_OAMSA_CMEVENT("CmEventProducer::sendEvent(): LEAVE with %d",ret);
	LEAVE_OAMSA_CMEVENT();
	return ret;

}

MafReturnT CmEventProducer::stop()
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("CmEventProducer::stop(): ENTER");
	_eventProducer.stop();
	DEBUG_OAMSA_CMEVENT("CmEventProducer::stop(): LEAVE");
	LEAVE_OAMSA_CMEVENT();
	return MafOk;
}


// Copy the content of the MAF CM notification struct ("mafCmNot") to local data
// Memory of the original "mafCmNot" shall be freed by the allocator
// Memory of the local data (held by this class) shall be freed in the destructor of this class
CmEventProducer::CmNotification::CmNotification(MafOamSpiCmEvent_Notification_1T &mafCmNot)
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("CmEventProducer::CmNotification::CmNotification(): Copy the content of the MAF CM notification struct");

	txHandle = mafCmNot.txHandle;
	eventTime = mafCmNot.eventTime;
	sourceIndicator = mafCmNot.sourceIndicator;
	events = mafCmNot.events;

	DEBUG_OAMSA_CMEVENT("CmEventProducer::CmNotification::CmNotification(): RETURN");
	LEAVE_OAMSA_CMEVENT();
}

// This destructor is called when all the consumers notified
// That means we don't need that notification struct data any more, so free the memory of this data structure
CmEventProducer::CmNotification::~CmNotification()
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("CmEventProducer::CmNotification::~CmNotification(): Deleting the event struct array, deleting memory now");
	freeCmEvents(events);

	DEBUG_OAMSA_CMEVENT("CmEventProducer::CmNotification::~CmNotification(): RETURN");
	LEAVE_OAMSA_CMEVENT();
}

/* RegexpFilter constructor function
 *
 * This function called:
 *
 *     When a CM event consumer calls addFilter.
 *     So for each CM event consumer a RegexpFilter instance will be created.
 *
 * Functionality:
 *
 *     Call NTF subscriberFunction only at the first instance creation of RegexpFilter.
 *     That means we need to subscribe for the NTF notifications when there is at least 1 event consumer.
 *     So the first event consumer's addFilter call will trigger our NTF subscription call.
 *     For the coming additional RegexpFilter instance creation (addFilter calls) do not subscribe again.
 */
CmEventProducer::RegexpFilter::RegexpFilter(MafNameValuePairT** filters) : Com::FilterMatcher<CmNotification>(filters)
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("CmEventProducer::RegexpFilter::RegexpFilter(): ENTER Creating RegexpFilter instance");
	if(alreadySubscribed)
	{
		DEBUG_OAMSA_CMEVENT("CmEventProducer::RegexpFilter::RegexpFilter(): already subscribed");
	}
	else
	{
		DEBUG_OAMSA_CMEVENT("CmEventProducer::RegexpFilter::RegexpFilter(): call subscriber function");
		s_theCmEventProducer->subscriberFunction();
		alreadySubscribed = true;
	}
	DEBUG_OAMSA_CMEVENT("CmEventProducer::RegexpFilter::RegexpFilter(): LEAVE RegexpFilter instance created");
	LEAVE_OAMSA_CMEVENT();
}

CmEventProducer::RegexpFilter::~RegexpFilter()
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("CmEventProducer::RegexpFilter::~RegexpFilter(): ENTER Deleting RegexpFilter instance");
	clearFilterList();
	DEBUG_OAMSA_CMEVENT("CmEventProducer::RegexpFilter::~RegexpFilter(): LEAVE RegexpFilter instance deleted");
	LEAVE_OAMSA_CMEVENT();
}

/*
 * This method called from addFilter
 * If all the regular expressions in the filter array (given by the consumer) are valid then we store the compiled regex and return true
 * Otherwise return false
 */
bool CmEventProducer::RegexpFilter::isValid() const
{
	ENTER_OAMSA_CMEVENT();
	//DEBUG_OAMSA_CMEVENT("CmEventProducer::RegexpFilter::isValid(): ENTER");
	if(_filters != NULL)
	{
		/* If the NULL terminated filter array exists, but does not contain any data,
		 * then according to the SPI: the filtering is turned of.
		 * Return true in that case.
		 */
		if(_filters[0] == NULL)
		{
			DEBUG_OAMSA_CMEVENT("CmEventProducer::RegexpFilter::isValid(): filters[0] = NULL, no filter given, returning true");
			LEAVE_OAMSA_CMEVENT();
			return true;
		}
		/* Checking if the elements of the filter array are valid regular expressions.
		 * The first non-valid element will delete the filterList, break the loop and return false.
		 * If all the expressions are valid then store the compiled expressions in the filterList and return true.
		 * filterList will be used by isMatch function
		 */
		for(int i = 0; _filters[i] != NULL; i++)
		{
			std::string regExpName  = _filters[i]->name;
			std::string regExpValue = _filters[i]->value;

			Com::RegularExpression *regExp = new Com::RegularExpression(regExpValue);
			DEBUG_OAMSA_CMEVENT("CmEventProducer::RegexpFilter::isValid(): checking if the regexp is valid: (%s), regExpName is: (%s)", regExpValue.c_str(), regExpName.c_str());
			if (regExp->isExpressionValid())
			{
					addToFilterList(regExp, regExpName, regExpValue);
			}
			else
			{
				ERR_OAMSA_CMEVENT("The filter element provided by the CM event consumer is not a valid regular expression: (%s), regExpName is: (%s), returning false", regExpValue.c_str(), regExpName.c_str());
				DEBUG_OAMSA_CMEVENT("CmEventProducer::RegexpFilter::isValid(): deleting invalid regexp object");
				delete regExp;
				clearFilterList();
				LEAVE_OAMSA_CMEVENT();
				return false;
			}
		}
		DEBUG_OAMSA_CMEVENT("CmEventProducer::RegexpFilter::isValid(): all regular expressions are valid, returning true");
		LEAVE_OAMSA_CMEVENT();
		return true;
	}
	/*
	 * According to the SPI: this is an error case.
	 * There is no filter array instance, so return false, and log an error.
	 */
	else
	{
		ERR_OAMSA_CMEVENT("Error case: filter = NULL, no filter array instance, returning false");
		LEAVE_OAMSA_CMEVENT();
		return false;
	}
}

/* For each CM event consumer there is a RegexpFilter object created. They are created in addFilter call.
 *
 * Input of this function: 1 CM notification struct, which may contain 1 or more DNs.
 * These DNs will be checked against the filterList.
 * The filterList is stored inside the RegexpFilter object. This filterList stores the filters given by a consumer.
 * The filterList stores the regular expressions in pre-compiled state.
 *
 * Returns true:
 *   -if there is a match for any of the DNs in the given notification struct for any of the stored and compiled regexp filters.
 *   -if no filter was given.
 */
bool CmEventProducer::RegexpFilter::isMatch(const CmNotification& notificationStruct) const
{
	ENTER_OAMSA_CMEVENT();
	//DEBUG_OAMSA_CMEVENT("CmEventProducer::RegexpFilter::isMatch(): ENTER");
	std::string dn;
	typedef std::list<std::string> dnListT;
	typedef dnListT::iterator dnListIteratorT;
	dnListT dnList;

	if(notificationStruct.events == NULL)
	{
		DEBUG_OAMSA_CMEVENT("CmEventProducer::RegexpFilter::isMatch(): events = NULL, returning false");
		LEAVE_OAMSA_CMEVENT();
		return false;
	}
	else
	{
		// There must be at least 1 event in the array.
		// If not, then return false.
		if(notificationStruct.events[0] == NULL)
		{
			DEBUG_OAMSA_CMEVENT("CmEventProducer::RegexpFilter::isMatch(): events[0] = NULL, no event in array, returning false");
			LEAVE_OAMSA_CMEVENT();
			return false;
		}
		for(unsigned int i = 0; notificationStruct.events[i] != NULL; i++)
		{
			dnList.push_back(std::string(notificationStruct.events[i]->dn));
		}
	}

	for(dnListIteratorT dnIter = dnList.begin(); dnIter != dnList.end(); dnIter++)
	{
		DEBUG_OAMSA_CMEVENT("CmEventProducer::RegexpFilter::isMatch(): DN need to be checked: (%s)", dnIter->c_str());
	}

	/* If there is no filter in the list then filtering is turned of.
	 * Return true in that case.
	 */
	if(filterList.size() == 0)
	{
		DEBUG_OAMSA_CMEVENT("CmEventProducer::RegexpFilter::isMatch(): no filter given, returning true");
		LEAVE_OAMSA_CMEVENT();
		return true;
	}
	/* Evaluating the already compiled regular expressions.
	 * The first matching will break the loop and set the return value to true.
	 * If non of the expressions are matching with any of the DNs, then keep the return value false.
	 *
	 * Walk through all the pre-compiled regular expressions.
	 */
	for(filterListIterator_T it = filterList.begin(); it != filterList.end(); it++)
	{
		// Walk through all the DNs
		for(dnListIteratorT dnIter = dnList.begin(); dnIter != dnList.end(); dnIter++)
		{
			/* "it" iterator points to a key-value pair.
			 * key:   a pre-compiled regexp instance
			 * value: a pair of <filterName,filterValue>
			 */
			if(it->begin()->first->match(*dnIter))
			{
				// if any of the provided regular expressions matches to any of the given DNs, then the overall match is true, so return true
				DEBUG_OAMSA_CMEVENT("CmEventProducer::RegexpFilter::isMatch(): regexp (%s) value: (%s) is matching for DN: (%s), returning true",
						it->begin()->second.first.c_str(),
						it->begin()->second.second.c_str(),
						dn.c_str());
				LEAVE_OAMSA_CMEVENT();
				return true;
			}
		}
	}
	DEBUG_OAMSA_CMEVENT("CmEventProducer::RegexpFilter::isMatch(): non of the regular expressions is matching, returning false");
	LEAVE_OAMSA_CMEVENT();
	return false;
}

// Add the compiled regular expressions, their names and their values to the filter list
void CmEventProducer::RegexpFilter::addToFilterList(Com::RegularExpression *regExp, std::string filterName, std::string filterValue) const
{
	ENTER_OAMSA_CMEVENT();
	filterMap_T fMap;
	fMap[regExp] = std::make_pair(filterName, filterValue);
	filterList.push_back(fMap);
	LEAVE_OAMSA_CMEVENT();
}

// clear the filter list
void CmEventProducer::RegexpFilter::clearFilterList(void) const
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("CmEventProducer::RegexpFilter::clearFilterList(): Deleting filterList");
	for(filterListIterator_T i= filterList.begin(); i != filterList.end(); i++)
	{
		delete i->begin()->first;
		i->clear();
	}
	filterList.clear();
	LEAVE_OAMSA_CMEVENT();
}

/*******************************************************
 *  Support functions
 *******************************************************/

// This function frees all the memory of the events in a CM event struct
void freeCmEvents(MafOamSpiCmEvent_1T** events)
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("freeCmEvents(): Deleting the event struct array, deleting memory now");
	if(events != NULL)
	{
		unsigned int i=0;
		unsigned int j=0;
		for(i = 0; events[i] != NULL; i++)
		{
			if(events[i]->dn != NULL)
			{
				DEBUG_OAMSA_CMEVENT("freeCmEvents(): freeing event[%u]->dn = (%s)",i,events[i]->dn);
				free((void*)(events[i]->dn));
			}
			if(events[i]->attributes != NULL)
			{
				for(j = 0; events[i]->attributes[j]; j++)
				{
					if(events[i]->attributes[j]->name != NULL)
					{
						DEBUG_OAMSA_CMEVENT("freeCmEvents(): freeing events[%u]->attributes[%u]->name = (%s)",i,j,events[i]->attributes[j]->name);
						free((void*)(events[i]->attributes[j]->name));
					}
					DEBUG_OAMSA_CMEVENT("freeCmEvents(): freeing events[%u]->attributes[%u]->value",i,j);
					freeAVC(&(events[i]->attributes[j]->value));
					DEBUG_OAMSA_CMEVENT("freeCmEvents(): freeing events[%u]->attributes[%u]",i,j);
					free(events[i]->attributes[j]);
				}
				DEBUG_OAMSA_CMEVENT("freeCmEvents(): freeing events[%u]->attributes array",i);
				free(events[i]->attributes);
			}
			DEBUG_OAMSA_CMEVENT("freeCmEvents(): freeing event[%u]",i);
			free(events[i]);
		}
		DEBUG_OAMSA_CMEVENT("freeCmEvents(): freeing events array");
		free(events);
	}
	else
	{
		DEBUG_OAMSA_CMEVENT("freeCmEvents(): events = NULL");
	}
	DEBUG_OAMSA_CMEVENT("freeCmEvents(): done");
	LEAVE_OAMSA_CMEVENT();
}
/* This function frees all the memory of an MafMoAttributeValueContainer
 * If the type is struct then this function will be called recursively.
 */
void freeAVC(MafMoAttributeValueContainer_3T *avc)
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("freeAVC(): freeing Attribute Value Container now");

	if(avc == NULL)
	{
		DEBUG_OAMSA_CMEVENT("freeAVC(): Attribute Value Container = NULL");
		LEAVE_OAMSA_CMEVENT();
		return;
	}
	else
	{
		DEBUG_OAMSA_CMEVENT("freeAVC(): avc->type = %d",avc->type);
		DEBUG_OAMSA_CMEVENT("freeAVC(): avc->nrOfValues = %u",avc->nrOfValues);
		for(unsigned int i = 0; i < avc->nrOfValues; i++)
		{
			DEBUG_OAMSA_CMEVENT("freeAVC(): avc->values[%u]",i);
			if(avc->type == MafOamSpiMoAttributeType_3_STRING)
			{
				free((void*)avc->values[i].value.theString);
			}
			// Fix memory leaks
			else if(avc->type == MafOamSpiMoAttributeType_3_REFERENCE)
			{
				free((void*)avc->values[i].value.moRef);
			}
			else if(avc->type == MafOamSpiMoAttributeType_3_STRUCT)
			{
				DEBUG_OAMSA_CMEVENT("freeAVC(): freeing struct members now");
				MafMoAttributeValueStructMember_3 *SM = avc->values[i].value.structMember;
				while(SM != NULL)
				{
					DEBUG_OAMSA_CMEVENT("freeAVC(): freeing SM->memberName = (%s)",SM->memberName);
					free(SM->memberName);

					// recursively call the same function
					freeAVC(SM->memberValue);
					DEBUG_OAMSA_CMEVENT("freeAVC(): freeing SM->memberValue");
					free(SM->memberValue);
					MafMoAttributeValueStructMember_3 *nextSM = SM->next;
					free(SM);
					SM =  nextSM;
				}
			}
		}
	}
	if(avc->nrOfValues >= 1)
	{
		DEBUG_OAMSA_CMEVENT("freeAVC(): delete avc->values");
		free(avc->values);
	}
	DEBUG_OAMSA_CMEVENT("freeAVC(): freeing Attribute Value Container: done");
	LEAVE_OAMSA_CMEVENT();
}
