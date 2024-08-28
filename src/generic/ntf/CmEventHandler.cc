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
*
* Modified: eaparob 2013-03-21:   Implemented the CM Notification Cache
*
* Reviewed: efaiami 2013-03-27
*
* Modified: eaparob 2013-05-03:   Functionality of CM Cache extended to handle simple types as inputs
*
* Modified: xanhdao 2013-10-23:   MR24146: support Floation point
*
* Modified: uabjoy  2014-03-24:   Added support for Trace CC.
*
*******************************************************************************/

#ifdef UNIT_TEST
#define TRACEPOINT_DEFINE
#define TRACEPOINT_PROBE_DYNAMIC_LINKAGE
#endif

#include "CmEventHandler.h"
#include <string.h>

#ifndef UNIT_TEST
CmEventProducer* CmEventHandler::_cmEventProducer = NULL;
#endif

/*******************************************************
 *  Functions of CmEventHandler
 *******************************************************/

CmEventHandler::CmEventHandler(MafMgmtSpiInterfacePortal_3T& portal_MAF,
		SaAisErrorT (*subscriberFunction)(void),
		SaAisErrorT (*unsubscriberFunction)(void))
: _notificationCacheInDiscardMode(false)
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("CmEventHandler::CmEventHandler(): ENTER");
	portal = portal_MAF;
	subscriberFunc = subscriberFunction;
	unsubscriberFunc = unsubscriberFunction;
	DEBUG_OAMSA_CMEVENT("CmEventHandler::CmEventHandler(): LEAVE");
	LEAVE_OAMSA_CMEVENT();
}

CmEventHandler::~CmEventHandler()
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("CmEventHandler::~CmEventHandler(): ENTER");
	DEBUG_OAMSA_CMEVENT("CmEventHandler::~CmEventHandler(): LEAVE");
	LEAVE_OAMSA_CMEVENT();
}

MafReturnT CmEventHandler::start()
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("CmEventHandler::start(): ENTER");
	MafReturnT ret = MafOk;

#ifndef UNIT_TEST
	if(initializeMutex())
	{
            DEBUG_OAMSA_CMEVENT("CmEventHandler::start(): pthread_mutex_init failed");
        }

	_cmEventProducer = new CmEventProducer(subscriberFunc, unsubscriberFunc);
	ret = _cmEventProducer->start(portal);
#endif
	DEBUG_OAMSA_CMEVENT("CmEventHandler::start(): LEAVE with (%d)",ret);
	LEAVE_OAMSA_CMEVENT();
	return ret;
}

MafReturnT CmEventHandler::pushEvent(MafOamSpiCmEvent_Notification_1T &mafCmNot)
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("CmEventHandler::pushEvent(): ENTER");
	MafReturnT ret = MafOk;
	ret = CmEventHandler::sendEvent(mafCmNot);
	DEBUG_OAMSA_CMEVENT("CmEventHandler::pushEvent(): LEAVE with (%d)",ret);
	LEAVE_OAMSA_CMEVENT();
	return ret;
}

MafReturnT CmEventHandler::sendEvent(MafOamSpiCmEvent_Notification_1T &mafCmNot)
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("CmEventHandler::sendEvent(): ENTER");
	MafReturnT ret = MafOk;
#ifndef UNIT_TEST
	aquireMutex();
        if(_cmEventProducer) {
	    ret = _cmEventProducer->sendEvent(mafCmNot);
        }
        releaseMutex();
#endif
	DEBUG_OAMSA_CMEVENT("CmEventHandler::sendEvent(): LEAVE with (%d)",ret);
	LEAVE_OAMSA_CMEVENT();
	return ret;
}

MafReturnT CmEventHandler::stop()
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("CmEventHandler::stop(): ENTER");
	MafReturnT ret = MafOk;

#ifndef UNIT_TEST
	aquireMutex();
        if(_cmEventProducer) {
	    DEBUG_OAMSA_CMEVENT("CmEventHandler::stop(): stopping CM Event Producer");
	    ret = _cmEventProducer->stop();
	    delete _cmEventProducer;
            _cmEventProducer = NULL;
        }
	releaseMutex();

	if(destroyMutex())
	{
            DEBUG_OAMSA_CMEVENT("CmEventHandler::stop(): pthread_mutex_destroy failed");
        }
#endif
	DEBUG_OAMSA_CMEVENT("CmEventHandler::stop(): LEAVE with (%d)",ret);
	LEAVE_OAMSA_CMEVENT();
	return ret;
}

/*
 * CmEventHandler::isDiscardedCcb
 * Returns True if the ccbId given was marked for discarding.
 */
bool CmEventHandler::isDiscardedCcb(SaImmOiCcbIdT ccbId)
{
	bool found = false;
#ifndef UNIT_TEST
	aquireMutex();
	DEBUG_OAMSA_CMEVENT("CmEventHandler::isDiscardedCcb ccbId : %llu.", ccbId);
	try {
		found = (_discardedCcbList.end() != std::find(_discardedCcbList.begin(), _discardedCcbList.end(), ccbId));
	}
	catch (...) {
		DEBUG_OAMSA_CMEVENT("CmEventHandler::isDiscardedCcb Exception while trying to access _discardedCcbList.");
	}
	releaseMutex();
#endif
	return found;
}

void CmEventHandler::addDiscardedCcb(SaImmOiCcbIdT ccbId)
{
#ifndef UNIT_TEST
	DEBUG_OAMSA_CMEVENT("CmEventHandler::addDiscardedCcb ccbId : %llu.", ccbId);
	aquireMutex();
	_discardedCcbList.push_front(ccbId);
	releaseMutex();
#endif
}

void CmEventHandler::removeDiscardedCcb(SaImmOiCcbIdT ccbId)
{
#ifndef UNIT_TEST
	aquireMutex();
	DEBUG_OAMSA_CMEVENT("CmEventHandler::removeDiscardedCcb ccbId : %llu.", ccbId);
	try {
		_discardedCcbList.remove(ccbId);
	}
	catch (...) {
		DEBUG_OAMSA_CMEVENT("CmEventHandler::removeDiscardedCcb Exception while trying to access _discardedCcbList.");
	}
	releaseMutex();
#endif
}

void CmEventHandler::setNotificationCacheDiscardMode(bool discardMode)
{
#ifndef UNIT_TEST
	_notificationCacheInDiscardMode = discardMode;
#else
	::notificationCacheInDiscardMode_unittest = discardMode;
#endif
}

bool CmEventHandler::isNotificationCacheInDiscardMode()
{
#ifndef UNIT_TEST
	return _notificationCacheInDiscardMode;
#else
	return ::notificationCacheInDiscardMode_unittest;
#endif
}

void CmEventHandler::buildOverflowEvent(MafOamSpiCmEvent_Notification_1T* overflowEvent)
{
	ENTER_OAMSA_CMEVENT();
	overflowEvent->txHandle = 0;
	// Set event time
	overflowEvent->eventTime = getTime();
	if(overflowEvent->eventTime == 0)
	{
		// Error situation, no time available.
		ERR_OAMSA_CMEVENT("CmEventHandler.buildOverflowEvent : Failed to get time");
	}

	overflowEvent->events = (MafOamSpiCmEvent_1T**)( calloc(2, sizeof(MafOamSpiCmEvent_1T*)));
	overflowEvent->events[0] = (MafOamSpiCmEvent_1T*)calloc(1, sizeof(MafOamSpiCmEvent_1T));
	overflowEvent->events[1] = NULL;
	// Overflow!
	overflowEvent->events[0]->eventType = MafOamSpiCmEvent_Overflow_1;
	overflowEvent->events[0]->attributes = (MafMoNamedAttributeValueContainer_3T**)calloc(1, sizeof(MafMoNamedAttributeValueContainer_3T*));
	overflowEvent->events[0]->attributes[0] = NULL;
	// Add an empty string here
	overflowEvent->events[0]->dn = (char*)calloc(1, sizeof(char*));
	overflowEvent->sourceIndicator = MafOamSpiCmEvent_ResourceOperation_1;

	LEAVE_OAMSA_CMEVENT();
	return;
}

#ifndef UNIT_TEST
bool CmEventHandler::initializeMutex()
{
	return (0 == pthread_mutex_init(&_cmEventHandlerMutex, NULL)) ? true : false;
}

void CmEventHandler::aquireMutex()
{
	pthread_mutex_lock(&_cmEventHandlerMutex);
}

void CmEventHandler::releaseMutex()
{
	pthread_mutex_unlock(&_cmEventHandlerMutex);
}

bool CmEventHandler::destroyMutex()
{
	return (0 == pthread_mutex_destroy(&_cmEventHandlerMutex)) ? true : false;
}
#endif
/*******************************************************
 *  Functions of CmNotificationCache
 *******************************************************/

CmNotificationCache::CmNotificationCache()
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("CmNotificationCache::CmNotificationCache(): ENTER");
	DEBUG_OAMSA_CMEVENT("CmNotificationCache::CmNotificationCache(): LEAVE");
	LEAVE_OAMSA_CMEVENT();
}

CmNotificationCache::~CmNotificationCache()
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("CmNotificationCache::~CmNotificationCache(): ENTER");
	DEBUG_OAMSA_CMEVENT("CmNotificationCache::~CmNotificationCache(): LEAVE");
	LEAVE_OAMSA_CMEVENT();
}

bool CmNotificationCache::isStruct(std::string immRdn)
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("CmNotificationCache::IsStruct(): ENTER");
	bool ret = false;
	const char *structId = "id=";
	size_t structIdLength = strlen(structId);

	// if immRdn starts with "id=" then it is a part of a struct
	// else simple type
	if( (structIdLength < immRdn.length()) && !strncmp(immRdn.c_str(),structId,structIdLength) )
	{
		ret = true;
		DEBUG_OAMSA_CMEVENT("CmNotificationCache::IsStruct(): true");
	}
	else
	{
		ret = false;
		DEBUG_OAMSA_CMEVENT("CmNotificationCache::IsStruct(): false");
	}
	LEAVE_OAMSA_CMEVENT();
	return ret;
}

CmNotificationCache::attributeType CmNotificationCache::getAttrType(SaImmOiCcbIdT ccbId, std::string immRdn, MafMoAttributeValueContainer_3T *avc)
{
	ENTER_OAMSA_CMEVENT();
	attributeType ret = unknownType;
	if(ccbId == 0 && avc->type == MafOamSpiMoAttributeType_3_STRUCT)
	{
		ret = runtimeStructType;
		DEBUG_OAMSA_CMEVENT("CmNotificationCache::getAttrType(): runtime struct type");
	}
	else if(avc != NULL && avc->type == MafOamSpiMoAttributeType_3_STRUCT)
	{
		ret = referenceType;
		DEBUG_OAMSA_CMEVENT("CmNotificationCache::getAttrType(): reference type");
	}
	else if(isStruct(immRdn))
	{
		ret = configStructType;
		DEBUG_OAMSA_CMEVENT("CmNotificationCache::getAttrType(): config struct type");
	}
	else
	{
		ret = simpleType;
		DEBUG_OAMSA_CMEVENT("CmNotificationCache::getAttrType(): simple type");
	}
	LEAVE_OAMSA_CMEVENT();
	return ret;
}

// "addAttr" and "setDn" function calls comes always in pairs.
// That means only 1 temp cache is needed to store the data in "addAttr" until "setDn" called
MafReturnT CmNotificationCache::addAttr(SaImmOiCcbIdT ccbId, std::string attrName, std::string immRdn, std::string memberName, MafMoAttributeValueContainer_3T *avc)
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("CmNotificationCache::addAttr(): ENTER with ccbId (%llu) attrName (%s) immRdn (%s) memberName (%s)", ccbId, attrName.c_str(), immRdn.c_str(), memberName.c_str());
	// get the right CcbCache using the ccbId as a key
	DEBUG_OAMSA_CMEVENT("CmNotificationCache::addAttr(): get the right CcbCache using key ccbId: (%llu)", ccbId);

	if(maxSAMemoryForCMEvents)
	{
		if (getMemory() > maxSAMemoryForCMEvents) {
			DEBUG_OAMSA_CMEVENT("CmNotificationCache::addAttr(): RETURN false, ccbId : %llu, memoryUsage: %llu", ccbId, getMemory());
			discardAllCcbs();
			LEAVE_OAMSA_CMEVENT();
			return MafNoResources;
		}
		if(!existsCache(ccbId))
		{
			updateMemory(sizeof(CcbCache), this);
		}
	}


	CcbCache* pCcbCache = getCache(ccbId);

	// DN is unknown, so get a temp cache to store the data which we have now.
	// DN will be set by setDn() function
	DEBUG_OAMSA_CMEVENT("CmNotificationCache::addAttr(): get temp cache");

	//Check for cache empty. Cache can be emptied in other thread during the ntf service stop
	if(pCcbCache != NULL) {
		DnCache* tempDnCache = pCcbCache->getTempCache();

		// Mark the runtime struct objects (only the referenced object in IMM).
		// The getEventType() function will need this info later.
		if(tempDnCache != NULL) {
			if(ccbId == 0 && isStruct(immRdn))
			{
				tempDnCache->setRuntimeStructObjectCalled();
			}

			// from the DN cache get the right AVC cache using attrName as a key
			DEBUG_OAMSA_CMEVENT("CmNotificationCache::addAttr(): from the DN cache get the right AVC cache using attrName: (%s)",attrName.c_str());
			AvcCache* pAvcCache = tempDnCache->getCache(attrName);
			DEBUG_OAMSA_CMEVENT("CmNotificationCache::addAttr():    pAvcCache(%p)",pAvcCache);
			StructElementCache* pStructElementCache = NULL;

			// get the "type" of the attribute (this "type" is not the type of the attribute value, e.g. not int or string etc.)
			// this "type" shows if the attribute belongs to simple/struct type or a reference.
			if(pAvcCache != NULL) {
				switch(getAttrType(ccbId, immRdn, avc))
				{
				case configStructType:
					// from the AVC cache get the right StructElement cache using immRdn as a key
					DEBUG_OAMSA_CMEVENT("CmNotificationCache::addAttr(): from the AVC cache get the right StructElement cache using immRdn: (%s)",immRdn.c_str());
					pStructElementCache = pAvcCache->getCache(immRdn);

					DEBUG_OAMSA_CMEVENT("CmNotificationCache::addAttr():    configStructType case, pStructElementCache(%p)",pStructElementCache);

					// add the struct element (name and value) to the list
					if(pStructElementCache != NULL) {
						if(!pStructElementCache->addToStructElementList(memberName, avc))
						{
							DEBUG_OAMSA_CMEVENT("CmNotificationCache::addAttr(): failed to add struct element to the list, returning false");
							LEAVE_OAMSA_CMEVENT();
							return MafInvalidArgument;
						}
						if(maxSAMemoryForCMEvents)
							pAvcCache->resetMemory(pStructElementCache->getMemory());
						DEBUG_OAMSA_CMEVENT("CmNotificationCache::addAttr():       pStructElementCache->structElementList.size(%d)",(int)pStructElementCache->structElementList.size());
					}
					break;
				case runtimeStructType:
					/* ToDo: Need to clear the memory that is allocated, instead of overwrite.
					Same applies for simpleType and referenceType */
					if(pAvcCache->avcRuntimeStruct != NULL)
					{
						DEBUG_OAMSA_CMEVENT("CmNotificationCache::addAttr(): function called more than one time with runtime struct attribute (%s)",attrName.c_str());
					}
					pAvcCache->avcRuntimeStruct = avc;
					if(maxSAMemoryForCMEvents)
						pAvcCache->resetMemory(memgetAVC(avc));
					break;
				case simpleType:
					if(pAvcCache->avcSimple != NULL)
					{
						DEBUG_OAMSA_CMEVENT("CmNotificationCache::addAttr(): function called more than one time with simpleType attribute (%s)",attrName.c_str());
					}
					pAvcCache->avcSimple = avc;
					if(maxSAMemoryForCMEvents)
						pAvcCache->resetMemory(memgetAVC(avc));
					break;
				case referenceType:
					if(pAvcCache->avcReference != NULL)
					{
						DEBUG_OAMSA_CMEVENT("CmNotificationCache::addAttr(): function called more than one time with referenceType attribute (%s)",attrName.c_str());
					}
					pAvcCache->avcReference = avc;
					tempDnCache->setReferenceCalled();
					if(maxSAMemoryForCMEvents)
						pAvcCache->resetMemory(memgetAVC(avc));
					break;
				case unknownType:
					DEBUG_OAMSA_CMEVENT("CmNotificationCache::addAttr(): unknown type: returning false");
					LEAVE_OAMSA_CMEVENT();
					return MafInvalidArgument;
				}

				if(maxSAMemoryForCMEvents)
					updateMemory(pAvcCache->getMemory(), this, pCcbCache, tempDnCache);
			}
		}
	}
	DEBUG_OAMSA_CMEVENT("CmNotificationCache::addAttr(): returning true");
	LEAVE_OAMSA_CMEVENT();
	return MafOk;
}

// this function called directly after "addAttr" function call(s).
// Only 1 temp cache is needed to store the data in "addAttr" call(s) until "setDn" called.
MafReturnT CmNotificationCache::setDn(SaImmOiCcbIdT ccbId, MafOamSpiCmEvent_SourceIndicator_1T sourceIndicator, std::string dn, MafOamSpiCmEvent_EventType_1T eventType, bool ccbLast)
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("CmNotificationCache::setDn(): ENTER with ccbId (%llu) sourceIndicator (%d) dn (%s) eventType (%d) ccbLast (%d)", ccbId, sourceIndicator, dn.c_str(), eventType, ccbLast);

	if(maxSAMemoryForCMEvents)
	{
		if (getMemory() > maxSAMemoryForCMEvents) {
			DEBUG_OAMSA_CMEVENT("CmNotificationCache::setDn(): RETURN false, ccbId : %llu, memoryUsage: %llu", ccbId, getMemory());
			discardAllCcbs();
			LEAVE_OAMSA_CMEVENT();
			return MafNoResources;
		}
		if(!existsCache(ccbId))
		{
			updateMemory(sizeof(CcbCache), this);
		}
	}
	CcbCache* pCcbCache = getCache(ccbId);
	//Check for cache empty. Cache can be emptied in the other thread during the ntf service stop
	if(pCcbCache != NULL) {
		pCcbCache->sourceIndicator = sourceIndicator;
		// if tempDnCache exists then load it
		DnCache* tempDnCache = NULL;
		if(pCcbCache->tempCacheExists())
		{
			// load back the old temp cache
			tempDnCache = pCcbCache->getTempCache();
			// set eventType
			if(tempDnCache != NULL)
				tempDnCache->setEventType(eventType);
		}
		/* If the following case happens that means:
		 * * -"setDn()" function called, but there was no "addAttr()" call with the same ccbId, so there is no tempDnCache to set.
		 * * Creating new DN cache
		 * */
		if( !(pCcbCache->existsCache(dn)) &&  tempDnCache == NULL )
		{
			DEBUG_OAMSA_CMEVENT("CmNotificationCache::setDn(): DN and tempDNCache not exist, creating new DN Cache (%s)",dn.c_str());
			DnCache* emptyDnCache = pCcbCache->getCache(dn);
			if(emptyDnCache != NULL) {
				emptyDnCache->setEventType(eventType);
				if(maxSAMemoryForCMEvents)
					updateMemory(emptyDnCache->getMemory(), this, pCcbCache);
			}
		}
		// If the DN already exists and there is a tempDnCache, then fill the already existing DnCache with the AttrCaches stored in the tempDnCache.
		// and also copy other data: -setReferenceCalled, -eventTypeSet
		else if(pCcbCache->existsCache(dn) && tempDnCache != NULL)
		{
			/*
			 * * Reset tempDn's memory and exact memory post parsing of tempDn
			 * * container will be added in subsequent stages respectively.
			 * */
			if(maxSAMemoryForCMEvents)
				updateMemory(tempDnCache->getMemory(), this, pCcbCache, NULL, NULL, true);

			// get the already existing DnCache
			DnCache* dnCache = pCcbCache->getCache(dn);
			/*
			 * * Copy data from tempDnCache to dnCache
			 * */
			// copy reference
			if(dnCache != NULL) {
				if(tempDnCache->isReferenceCalled())
				{
					dnCache->setReferenceCalled();
				}
				// copy eventTypeSet
				if(tempDnCache->eventTypeSet.create)
				{
					dnCache->setEventType(MafOamSpiCmEvent_MoCreated_1);
				}
				if(tempDnCache->eventTypeSet.mod)
				{
					dnCache->setEventType(MafOamSpiCmEvent_AttributeValueChange_1);
				}
				if(tempDnCache->eventTypeSet.del)
				{
					dnCache->setEventType(MafOamSpiCmEvent_MoDeleted_1);
				}
				// fill the already existing DnCache with the AttrCaches stored in the tempDnCache
				for(DnCache::cacheMapIterator_T iterAttr = tempDnCache->cacheMap.begin(); iterAttr != tempDnCache->cacheMap.end(); iterAttr++)
				{
					DEBUG_OAMSA_CMEVENT("CmNotificationCache::setDn(): fill in DnCache with the data stored in the tempDnCache: (%s)",iterAttr->first.c_str());
					if(dnCache->existsCache(iterAttr->first))
					{
						DEBUG_OAMSA_CMEVENT("CmNotificationCache::setDn(): (%s) exists, loading its data",iterAttr->first.c_str());
						AvcCache *tempAvc = iterAttr->second;
						AvcCache *dnAvc = dnCache->getCache(iterAttr->first);

						std::map<std::string,StructElementCache*>::iterator iterTempAvc;

						for(iterTempAvc = tempAvc->cacheMap.begin(); iterTempAvc != tempAvc->cacheMap.end(); iterTempAvc++)
						{
							if(dnAvc != NULL) {
								if(dnAvc->existsCache(iterTempAvc->first))
								{
									DEBUG_OAMSA_CMEVENT("CmNotificationCache::setDn():     structElementCache(%s) already exists, filling in the data",iterTempAvc->first.c_str());
									StructElementCache* structTempCache = iterTempAvc->second;
									StructElementCache *structDnCache = dnAvc->getCache(iterTempAvc->first);

									//Clear the previously calculated memory for AvcCache and consolidate.
									if(structDnCache != NULL) {
										if(maxSAMemoryForCMEvents)
											updateMemory(structDnCache->getMemory(), this, pCcbCache, dnCache, dnAvc, true);

										for(StructElementCache::structElementListIteratorT iterList = structTempCache->structElementList.begin(); iterList != structTempCache->structElementList.end(); iterList++)
										{
											structDnCache->addToStructElementList(iterList->begin()->first, iterList->begin()->second);
										}
										// free structTempCache since the data stored inside it already copied to structDnCache
										delete structTempCache;
										if(maxSAMemoryForCMEvents)
											updateMemory(structDnCache->getMemory(), this, pCcbCache, dnCache, dnAvc);
									}
								}
								else
								{
									DEBUG_OAMSA_CMEVENT("CmNotificationCache::setDn():     adding (%s) to the DN cache",iterTempAvc->first.c_str());
									dnAvc->addToCacheMap(iterTempAvc->first, iterTempAvc->second);

									if(maxSAMemoryForCMEvents)
										updateMemory(iterTempAvc->second->getMemory(), this, pCcbCache, dnCache, dnAvc);
								}
							}
					}
						tempAvc->skipFreeingInsideDestructor();
						delete tempAvc;
					}
					else
					{
						dnCache->addToCacheMap(iterAttr->first, iterAttr->second);
						if(maxSAMemoryForCMEvents)
							updateMemory(iterAttr->second->getMemory(), this, pCcbCache, dnCache);
					}
				}
				DEBUG_OAMSA_CMEVENT("CmNotificationCache::setDn(): tempCache data copied, freeing tempCache");
				tempDnCache->skipFreeingInsideDestructor();
				delete tempDnCache;
				DEBUG_OAMSA_CMEVENT("CmNotificationCache::setDn(): set tempCache pointer to NULL");
				tempDnCache = NULL;
			}
		}
		// DN does not exist, so add to CcbCache
		else if( !(pCcbCache->existsCache(dn)) && tempDnCache != NULL)
		{
			// move the value of the temp cache under the right key using the DN which received in this function
			pCcbCache->addToCacheMap(dn, tempDnCache);
		}
		// else: do nothing here, since there was no DN to set to the tempDnCache

		// Delete the temp cache from the ccbCache map, since not needed anymore.
		// Do not free the memory of the temp cache, since that data is moved to under the right DN cache.
		pCcbCache->deleteTempCache();

		/* if ccbLast received then:
		 *    -build the CM Event Struct
		 *    -push to the CM Event Handler
		 *    -delete from the ccbMap
		 */
		if(ccbLast)
		{
			/* The following filter is needed in a special case where no attributes added the CM Cache, but ccbLast=true.
			* In that case there is no need to send a CM event.
			 * For the remaining cases where the type is createMo or deleteMo: default behavior is to send notification(even in case where setDn called with no attributes set before and isNotify=false)
			 */
			if(pCcbCache->getCache(dn)->size() == 0 && eventType == MafOamSpiCmEvent_AttributeValueChange_1)
			{
				DEBUG_OAMSA_CMEVENT("CmNotificationCache::setDn(): empty DN cache with eventType=AttributeValueChange, skip sending CM event");
			}
			else
			{
				MafOamSpiCmEvent_Notification_1T cmEventStruct;
				// build the CM Event Struct, which can be pushed to the CM Event Handler
				pCcbCache->buildEventStruct(&cmEventStruct);

				// push the CM Event Struct to the CM Event Handler
		#ifndef UNIT_TEST
				push_CmEventHandler(&cmEventStruct);
		#else
				push_CmCacheUnittest(&cmEventStruct);
		#endif
			}
			// delete ccbId since we processed it, and don't need anymore
			if(maxSAMemoryForCMEvents)
				updateMemory(pCcbCache->getMemory(), this, NULL, NULL, NULL, true);
			deleteCacheFromMap(ccbId);
		}
	}
	DEBUG_OAMSA_CMEVENT("CmNotificationCache::setDn(): RETURN true");
	LEAVE_OAMSA_CMEVENT();
	return MafOk;
}

void CmNotificationCache::discardAllCcbs()
{
	MafOamSpiCmEvent_Notification_1T mafCmNotOverFlow;
	cmEventHandler->setNotificationCacheDiscardMode(true);
	cmEventHandler->buildOverflowEvent(&mafCmNotOverFlow);

#ifndef UNIT_TEST
	push_CmEventHandler(&mafCmNotOverFlow);
#else
	push_CmCacheUnittest(&mafCmNotOverFlow);
#endif

	DEBUG_OAMSA_CMEVENT("CmNotificationCache::discardAllCcbs(): adding discarded Ccb's");
	for(cacheMapIterator_T i = cacheMap.begin(); i != cacheMap.end();i++)
	{
		cmEventHandler->addDiscardedCcb(i->first);
		updateMemory(i->second->getMemory(), this, NULL, NULL, NULL, true);
		deleteCacheFromMap(i->first);
	}
	cmEventHandler->setNotificationCacheDiscardMode(false);
}

void CmNotificationCache::updateMemory(SaUint64T memoryValue,
		CmNotificationCache* cmCache,
		CcbCache* ccbCache,
		DnCache* dnCache,
		AvcCache* avcCache,
		bool memoryDeleted)
{
	if (avcCache) avcCache->setMemory(memoryValue, memoryDeleted);
	if (dnCache)  dnCache->setMemory(memoryValue, memoryDeleted);
	if (ccbCache) ccbCache->setMemory(memoryValue, memoryDeleted);
	if (cmCache)  setMemory(memoryValue, memoryDeleted);
}

bool isDiscardedCcb(SaImmOiCcbIdT ccbId)
{
	if (!cmEventHandler || !maxSAMemoryForCMEvents) {
		DEBUG_OAMSA_CMEVENT("isCcbDiscarded: unable to verify if ccbId in discarded ccb list.");
		return false;
	}
	DEBUG_OAMSA_CMEVENT("isCcbDiscarded: ccbId : %llu", ccbId);
	return cmEventHandler->isDiscardedCcb(ccbId);
}

void removeDiscardedCcb(SaImmOiCcbIdT ccbId)
{
	if (!cmEventHandler || !maxSAMemoryForCMEvents) {
		DEBUG_OAMSA_CMEVENT("removeDiscardedCcb: unable to remove ccbId from discarded ccb list.");
		return;
	}
	DEBUG_OAMSA_CMEVENT("removeDiscardedCcb: ccbId : %llu", ccbId);
	cmEventHandler->removeDiscardedCcb(ccbId);
}

/*******************************************************
 *  Functions of CcbCache
 *******************************************************/

CcbCache::CcbCache()
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("CcbCache::CcbCache(): ENTER");
	txHandle = 0;
	sourceIndicator = MafOamSpiCmEvent_Unknown_1;
	DEBUG_OAMSA_CMEVENT("CcbCache::CcbCache(): LEAVE");
	LEAVE_OAMSA_CMEVENT();
}

CcbCache::~CcbCache()
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("CcbCache::~CcbCache(): destructor called");
	LEAVE_OAMSA_CMEVENT();
}

void CcbCache::setDn(std::string dn)
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("CcbCache::setDn(): setting DN (%s)",dn.c_str());
	LEAVE_OAMSA_CMEVENT();
}

bool CcbCache::tempCacheExists()
{
	ENTER_OAMSA_CMEVENT();
	bool ret = existsCache(tempCacheKey);
	DEBUG_OAMSA_CMEVENT("CcbCache::tempCacheExists(): return (%d)",ret);
	LEAVE_OAMSA_CMEVENT();
	return ret;
}

DnCache* CcbCache::getTempCache()
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("CcbCache::getTempCache(): get tempCache");
	DnCache* pCache = getCache(tempCacheKey);
	LEAVE_OAMSA_CMEVENT();
	return pCache;
}

void CcbCache::deleteTempCache()
{
	ENTER_OAMSA_CMEVENT();
	if(tempCacheExists())
	{
		DEBUG_OAMSA_CMEVENT("CcbCache::deleteTempCache(): delete tempCache");
		deleteCacheFromMap(tempCacheKey,false);
		DEBUG_OAMSA_CMEVENT("CcbCache::deleteTempCache(): tempCache deleted");
	}
	else
	{
		DEBUG_OAMSA_CMEVENT("CcbCache::deleteTempCache(): tempCache not exists");
	}
	LEAVE_OAMSA_CMEVENT();
}

/*
 * This function builds the CM Event Struct, which can be pushed to the CM Event Handler
 */
MafOamSpiCmEvent_Notification_1T* CcbCache::buildEventStruct(MafOamSpiCmEvent_Notification_1T* ES)
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("CcbCache::buildEventStruct(): building the CM Event Struct now");

	// fill out the parameters
	ES->txHandle = txHandle;
	ES->eventTime = getTime();
	ES->sourceIndicator = sourceIndicator;

	// the number of events in the event array is the number of DNs we have.
	int nrOfEvents = size();

	// allocate memory for the event array
	ES->events = (MafOamSpiCmEvent_1T**)calloc(nrOfEvents + 1,sizeof(MafOamSpiCmEvent_1T*));

	// allocate memory and fill in data for the events
	cacheMapIterator_T iterEvent = cacheMap.begin();
	for(int i = 0; i < nrOfEvents; i++, iterEvent++)
	{
		// allocate memory for the event
		ES->events[i] = (MafOamSpiCmEvent_1T*)calloc(1,sizeof(MafOamSpiCmEvent_1T));

		// fill in the data
		DEBUG_OAMSA_CMEVENT("CcbCache::buildEventStruct(): dn: (%s)",iterEvent->first.c_str());
		ES->events[i]->dn = strdup(iterEvent->first.c_str());
		ES->events[i]->eventType = iterEvent->second->getEventType();

		// the number of attributes in the attribute array is the size of the attribute cache.
		int nrOfAttrs = iterEvent->second->size();

		// allocate memory for the attribute array
		ES->events[i]->attributes = (MafMoNamedAttributeValueContainer_3T**)calloc(nrOfAttrs + 1,sizeof(MafMoNamedAttributeValueContainer_3T*));

		// allocate memory and fill in data for the attributes
		std::map<std::string, AvcCache*>::iterator iterAttr = iterEvent->second->cacheMap.begin();
		for(int j = 0; j < nrOfAttrs; j++, iterAttr++)
		{
			// allocate memory for the event
			ES->events[i]->attributes[j] = (MafMoNamedAttributeValueContainer_3T*)calloc(1,sizeof(MafMoNamedAttributeValueContainer_3T));

			// set the attribute name
			ES->events[i]->attributes[j]->name = strdup(iterAttr->first.c_str());

			/* The 3 kind of AVC building methods:
			 *
			 *   buildAVC_configStructType():   for config structs
			 *   buildAVC_runtimeStructType():  for runtime structs
			 *   buildAVC_simpleType():         for config and runtime simple types
			 */
			if(iterAttr->second->IsConfigStructType())
			{
				buildAVC_configStructType(iterAttr->first, iterAttr->second, &(ES->events[i]->attributes[j]->value));
			}
			else if(iterAttr->second->IsRuntimeStructType())
			{
				buildAVC_runtimeStructType(iterAttr->first, iterAttr->second, &(ES->events[i]->attributes[j]->value));
			}
			else
			{
				buildAVC_simpleType(iterAttr->first, iterAttr->second, &(ES->events[i]->attributes[j]->value));
			}
		}
		// end the array with NULL
		ES->events[i]->attributes[nrOfAttrs] = NULL;
	}
	// end the array with NULL
	ES->events[nrOfEvents] = NULL;

	DEBUG_OAMSA_CMEVENT("CcbCache::buildEventStruct(): done, returning");
	LEAVE_OAMSA_CMEVENT();
	return ES;
}

// This function fills in the AVC with the AVC struct stored in AvcCache
void CcbCache::buildAVC_simpleType(std::string attrName, AvcCache *avcCache, MafMoAttributeValueContainer_3T* avc)
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("CcbCache::buildAVC_simpleType(): ENTER with attrName (%s)",attrName.c_str());
	if(avcCache->avcSimple != NULL)
	{
		// fill in attribute data
		avc->type = avcCache->avcSimple->type;
		avc->nrOfValues = avcCache->avcSimple->nrOfValues;
		avc->values = avcCache->avcSimple->values;

		// free only the MafMoAttributeValueContainer_3T but do not free the memory of avcCache.avcSimple->values, because that is needed later.
		free(avcCache->avcSimple);
		avcCache->avcSimple = NULL;
	}
	else
	{
		DEBUG_OAMSA_CMEVENT("CcbCache::buildAVC_simpleType(): the stored avc is NULL");
		// fill in attribute data
		avc->type = MafOamSpiMoAttributeType_3_STRUCT;
		avc->nrOfValues = 0;
		avc->values = NULL;
	}
	DEBUG_OAMSA_CMEVENT("CcbCache::buildAVC_simpleType(): done, returning");
	LEAVE_OAMSA_CMEVENT();
}

// This function builds up the AVC from the config struct fragments stored in the list of the given AvcCache
void CcbCache::buildAVC_configStructType(std::string attrName, AvcCache *avcCache, MafMoAttributeValueContainer_3T* avc)
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("CcbCache::buildAVC_configStructType(): ENTER with attrName (%s)",attrName.c_str());

	// fill in attribute data
	avc->type = MafOamSpiMoAttributeType_3_STRUCT;
	// the nrOfValues is the number of elements in the multi-value struct
	avc->nrOfValues = (unsigned int)avcCache->size();

	// allocate memory for the values
	avc->values = (MafMoAttributeValue_3T*)calloc(avc->nrOfValues,sizeof(MafMoAttributeValue_3T));

	std::map<std::string, StructElementCache*>::iterator iterSE = avcCache->cacheMap.begin();
	// Walk through all the elements in the array (multi-value struct)
	for(unsigned int i = 0; i < avc->nrOfValues; i++, iterSE++)
	{
		DEBUG_OAMSA_CMEVENT("CcbCache::buildAVC_configStructType(): values[%d].value.structMember",i);

		// temporary struct member element in the linked list
		MafMoAttributeValueStructMember_3 *SM = NULL;

		// the head of the linked list
		MafMoAttributeValueStructMember_3 *headSM = NULL;

		bool firstTimeInLoop = true;
		DEBUG_OAMSA_CMEVENT("CcbCache::buildAVC_configStructType():    structElementList.size(%d)",(int)iterSE->second->structElementList.size());
		// set struct member value
		for(StructElementCache::structElementListIteratorT iterList = iterSE->second->structElementList.begin(); iterList != iterSE->second->structElementList.end(); iterList++)
		{
			std::string memberName = iterList->begin()->first;
			MafMoAttributeValueContainer_3T *memberValue = iterList->begin()->second;

			if(firstTimeInLoop)
			{
				// allocate memory for the first element (head) of the linked list
				SM = (MafMoAttributeValueStructMember_3*)calloc(1,sizeof(MafMoAttributeValueStructMember_3));
				// save the head
				headSM = SM;
				firstTimeInLoop = false;
			}
			// Handle the "next" element in the linked list from the second run of the loop
			else
			{
				SM->next = (MafMoAttributeValueStructMember_3*)calloc(1,sizeof(MafMoAttributeValueStructMember_3));
				SM = SM->next;
			}
			// set struct member name
			SM->memberName = strdup(memberName.c_str());
			DEBUG_OAMSA_CMEVENT("CcbCache::buildAVC_configStructType():    memberName (%s)",SM->memberName);

			// set struct member value
			SM->memberValue = memberValue;
		}
		// set NULL to the end of the linked list
		SM->next = NULL;
		avc->values[i].value.structMember = headSM;
	}

	DEBUG_OAMSA_CMEVENT("CcbCache::buildAVC_configStructType(): done, returning");
	LEAVE_OAMSA_CMEVENT();
}

// This function fills in the AVC with the AVC struct stored in AvcCache
void CcbCache::buildAVC_runtimeStructType(std::string attrName, AvcCache *avcCache, MafMoAttributeValueContainer_3T* avc)
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("CcbCache::buildAVC_runtimeStructType(): ENTER with attrName (%s)",attrName.c_str());
	if(avcCache->avcRuntimeStruct != NULL)
	{
		// fill in attribute data
		avc->type = avcCache->avcRuntimeStruct->type;
		avc->nrOfValues = avcCache->avcRuntimeStruct->nrOfValues;
		avc->values = avcCache->avcRuntimeStruct->values;

		// free only the MafMoAttributeValueContainer_3T but do not free the memory of avcCache.avcRuntimeStruct->values, because that is needed later.
		free(avcCache->avcRuntimeStruct);
		avcCache->avcRuntimeStruct = NULL;
	}
	else
	{
		DEBUG_OAMSA_CMEVENT("CcbCache::buildAVC_runtimeStructType(): the stored avc is NULL");
		// fill in attribute data
		avc->type = MafOamSpiMoAttributeType_3_STRUCT;
		avc->nrOfValues = 0;
		avc->values = NULL;
	}
	DEBUG_OAMSA_CMEVENT("CcbCache::buildAVC_runtimeStructType(): done, returning");
	LEAVE_OAMSA_CMEVENT();
}

/*******************************************************
 *  Functions of DnCache
 *******************************************************/

DnCache::DnCache()
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("DnCache::DnCache(): constructor called");
	eventTypeSet.create = false;
	eventTypeSet.mod = false;
	eventTypeSet.del = false;
	modifiedReference = false;
	modifiedRuntimeStructObject = false;
	LEAVE_OAMSA_CMEVENT();
}

DnCache::~DnCache()
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("DnCache::~DnCache(): destructor called");
	LEAVE_OAMSA_CMEVENT();
}

void DnCache::setEventType(MafOamSpiCmEvent_EventType_1T eventType)
{
	ENTER_OAMSA_CMEVENT();
	switch(eventType)
	{
	case MafOamSpiCmEvent_MoCreated_1:
		eventTypeSet.create = true;
		break;
	case MafOamSpiCmEvent_MoDeleted_1:
		eventTypeSet.del = true;
		break;
	case MafOamSpiCmEvent_AttributeValueChange_1:
		eventTypeSet.mod = true;
		break;
	case MafOamSpiCmEvent_Overflow_1:
	default:
		DEBUG_OAMSA_CMEVENT("DnCache::setEventType(): wrong type");
		break;
	}
	LEAVE_OAMSA_CMEVENT();
}

MafOamSpiCmEvent_EventType_1T DnCache::getEventType(void)
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("DnCache::getEventType(): ENTER");
	MafOamSpiCmEvent_EventType_1T ret = MafOamSpiCmEvent_Overflow_1;

	// The event type of runtime struct objects (only the referenced object in IMM) is always AttributeValueChange
	if(isRuntimeStructObjectCalled())
	{
		DEBUG_OAMSA_CMEVENT("DnCache::getEventType():    runtime struct object");
		ret = MafOamSpiCmEvent_AttributeValueChange_1;
		DEBUG_OAMSA_CMEVENT("DnCache::getEventType():    AttributeValueChange");
	}
	else
	{
		// DEBUG the types which set before
		if(eventTypeSet.create)
		{
			DEBUG_OAMSA_CMEVENT("DnCache::getEventType():    eventTypeSet.create");
		}
		if(eventTypeSet.mod)
		{
			DEBUG_OAMSA_CMEVENT("DnCache::getEventType():    eventTypeSet.mod");
		}
		if(eventTypeSet.del)
		{
			DEBUG_OAMSA_CMEVENT("DnCache::getEventType():    eventTypeSet.del");
		}
		if(isReferenceCalled())
		{
			DEBUG_OAMSA_CMEVENT("DnCache::getEventType():    reference is called before");
		}
		// end of DEBUG

		// Set the event type
		if(isReferenceCalled())
		{
			if(eventTypeSet.create && eventTypeSet.mod)
			{
				ret = MafOamSpiCmEvent_AttributeValueChange_1;
				DEBUG_OAMSA_CMEVENT("DnCache::getEventType():    AttributeValueChange");
			}
			else if(eventTypeSet.create)
			{
				ret = MafOamSpiCmEvent_MoCreated_1;
				DEBUG_OAMSA_CMEVENT("DnCache::getEventType():    MoCreated");
			}
			else
			{
				ret = MafOamSpiCmEvent_AttributeValueChange_1;
				DEBUG_OAMSA_CMEVENT("DnCache::getEventType():    AttributeValueChange of struct reference");
			}
		}

		// modify could be called together with other event types, so handle these cases here
		if(eventTypeSet.mod)
		{
			if(eventTypeSet.del)
			{
				ret = MafOamSpiCmEvent_AttributeValueChange_1;
				DEBUG_OAMSA_CMEVENT("DnCache::getEventType():    AttributeValueChange");
			}
			else if(eventTypeSet.create)
			{
				ret = MafOamSpiCmEvent_AttributeValueChange_1;
				DEBUG_OAMSA_CMEVENT("DnCache::getEventType():    AttributeValueChange");
			}
			else
			{
				ret = MafOamSpiCmEvent_AttributeValueChange_1;
				DEBUG_OAMSA_CMEVENT("DnCache::getEventType():    AttributeValueChange");
			}
		}
		else
		{
			if(eventTypeSet.create)
			{
				ret = MafOamSpiCmEvent_MoCreated_1;
				DEBUG_OAMSA_CMEVENT("DnCache::getEventType():    MoCreated");
			}
			else if(eventTypeSet.del)
			{
				ret = MafOamSpiCmEvent_MoDeleted_1;
				DEBUG_OAMSA_CMEVENT("DnCache::getEventType():    MoDeleted");
			}
		}

	}
	DEBUG_OAMSA_CMEVENT("DnCache::getEventType(): RETURN");
	LEAVE_OAMSA_CMEVENT();
	return ret;
}

void DnCache::setReferenceCalled()
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("DnCache::setReferenceCalled(): modification of a struct reference identified");
	modifiedReference = true;
	LEAVE_OAMSA_CMEVENT();
}

bool DnCache::isReferenceCalled()
{
	ENTER_OAMSA_CMEVENT();
	bool ret = false;
	if(modifiedReference)
	{
		DEBUG_OAMSA_CMEVENT("DnCache::isReferenceCalled(): true");
		ret = true;
	}
	else
	{
		DEBUG_OAMSA_CMEVENT("DnCache::isReferenceCalled(): false");
		ret = false;
	}
	LEAVE_OAMSA_CMEVENT();
	return ret;
}

void DnCache::setRuntimeStructObjectCalled()
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("DnCache::setRuntimeStructObjectCalled(): modification of a runtime struct object identified");
	modifiedRuntimeStructObject = true;
	LEAVE_OAMSA_CMEVENT();
}

bool DnCache::isRuntimeStructObjectCalled()
{
	ENTER_OAMSA_CMEVENT();
	bool ret = false;
	if(modifiedRuntimeStructObject)
	{
		DEBUG_OAMSA_CMEVENT("DnCache::isRuntimeStructObjectCalled(): true");
		ret = true;
	}
	else
	{
		DEBUG_OAMSA_CMEVENT("DnCache::isRuntimeStructObjectCalled(): false");
		ret = false;
	}
	LEAVE_OAMSA_CMEVENT();
	return ret;
}

/*******************************************************
 *  Functions of AvcCache
 *******************************************************/

AvcCache::AvcCache() : avcSimple(NULL), avcReference(NULL), avcRuntimeStruct(NULL)
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("AvcCache::AvcCache(): constructor called");
	LEAVE_OAMSA_CMEVENT();
}

AvcCache::~AvcCache()
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("AvcCache::~AvcCache(): destructor called");
	// Freeing the memory of the AVC which belongs to a struct reference. (that AVC won't be used anymore)
	// This AVC contains moRef, but the type is STRUCT
	if(avcReference != NULL)
	{
		if(avcReference->type == MafOamSpiMoAttributeType_3_STRUCT)
		{
			for(unsigned int i = 0; i != avcReference->nrOfValues; i++)
			{
				// type is STRUCT but value is type of moRef
				free((void*)avcReference->values[i].value.moRef);
			}
			free(avcReference->values);
		}
		else
		{
			DEBUG_OAMSA_CMEVENT("AvcCache::~AvcCache(): wrong type received(%d): avcReference->type != MafOamSpiMoAttributeType_3_STRUCT",avcReference->type);
		}
		free(avcReference);
		avcReference = NULL;
	}
#ifndef UNIT_TEST
	if(cmEventHandler->isNotificationCacheInDiscardMode())
	{
		if(avcSimple) {
			freeAVC(avcSimple);
			free(avcSimple);
			avcSimple = NULL;
		}

		if(avcRuntimeStruct) {
			freeAVC(avcRuntimeStruct);
			free(avcRuntimeStruct);
			avcRuntimeStruct = NULL;
		}

		if(IsConfigStructType()) {
			for(std::map<std::string, StructElementCache*>::iterator iterSE = cacheMap.begin();
					iterSE != cacheMap.end();
					iterSE++)
			{
				for(StructElementCache::structElementListIteratorT iterList = iterSE->second->structElementList.begin();
						iterList != iterSE->second->structElementList.end();
						iterList++)
				{
					freeAVC(iterList->begin()->second);
					free(iterList->begin()->second);
				}
			}
		}
	}
#endif
	DEBUG_OAMSA_CMEVENT("AvcCache::~AvcCache(): destructing finished");
	LEAVE_OAMSA_CMEVENT();
}

bool AvcCache::IsConfigStructType()
{
	ENTER_OAMSA_CMEVENT();
	if(size() != 0)
	{
		DEBUG_OAMSA_CMEVENT("AvcCache::IsConfigStructType(): true");
		LEAVE_OAMSA_CMEVENT();
		return true;
	}
	else
	{
		DEBUG_OAMSA_CMEVENT("AvcCache::IsConfigStructType(): false");
		LEAVE_OAMSA_CMEVENT();
		return false;
	}
}

bool AvcCache::IsRuntimeStructType()
{
	ENTER_OAMSA_CMEVENT();
	if(avcRuntimeStruct != NULL)
	{
		DEBUG_OAMSA_CMEVENT("AvcCache::IsRuntimeStructType(): true");
		LEAVE_OAMSA_CMEVENT();
		return true;
	}
	else
	{
		DEBUG_OAMSA_CMEVENT("AvcCache::IsRuntimeStructType(): false");
		LEAVE_OAMSA_CMEVENT();
		return false;
	}
}

/*******************************************************
 *  Functions of StructElementCache
 *******************************************************/

StructElementCache::StructElementCache() : cacheMemory(std::make_pair(0,0))
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("StructElementCache::StructElementCache(): constructor called");
	LEAVE_OAMSA_CMEVENT();
}

StructElementCache::~StructElementCache()
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("StructElementCache::~StructElementCache(): destructor called");
	LEAVE_OAMSA_CMEVENT();
}

bool StructElementCache::addToStructElementList(std::string memberName, MafMoAttributeValueContainer_3T *memberValue)
{
	ENTER_OAMSA_CMEVENT();
	//DEBUG_OAMSA_CMEVENT("AvcCache::addToAvcList(): ENTER");

	// Struct inside a struct is not allowed
	if(memberValue->type == MafOamSpiMoAttributeType_3_STRUCT)
	{
		DEBUG_OAMSA_CMEVENT("StructElementCache::addToStructElementList(): struct inside a struct is not allowed, returning false");
		LEAVE_OAMSA_CMEVENT();
		return false;
	}

	DEBUG_OAMSA_CMEVENT("StructElementCache::addToStructElementList(): AVC is valid, adding to the list");
	memberDataT memberData;
	memberData[memberName] = memberValue;
	// add to list
	structElementList.push_back(memberData);
	if(maxSAMemoryForCMEvents)
		setMemory(memgetAVC(memberValue));
	DEBUG_OAMSA_CMEVENT("StructElementCache::addToStructElementList(): returning true");
	LEAVE_OAMSA_CMEVENT();
	return true;
}

SaUint64T StructElementCache::getMemory()
{
	SaUint64T retVal = cacheMemory.first + cacheMemory.second;
	DEBUG_OAMSA_CMEVENT("StructElementCache::getMemory: memoryUsage: (%llu)", retVal);
	return retVal;
}

void StructElementCache::resetMemory(SaUint64T bytes)
{
	cacheMemory.second = bytes;
}

void StructElementCache::setMemory(SaUint64T bytes, bool memoryDeleted)
{
	if (!memoryDeleted)
		cacheMemory.second += bytes;
	else {
		if (cacheMemory.second > bytes)
			cacheMemory.second -= bytes;
		else {
			DEBUG_OAMSA_CMEVENT("BaseCache::setMemory(): deleted memory is more than calculated memory allocation. Reseting memory counter.");
			cacheMemory.second = 0;
		}
	}
}

/*******************************************************
 *  Interface functions of the CM event handler:
 *
 *           -start_CmEventHandler
 *           -push_CmEventHandler
 *           -stop_CmEventHandler
 *
 *******************************************************/

/* This function starts the CmEventHandler.
 * Only one instance of CmEventHandler can exists at a time. This function takes care of this.
 *
 * return values:
 *
 *     MafOK     : if CmEventHandler successfully started
 *     MafFailure: if CmEventHandler not started
 */
MafReturnT start_CmEventHandler(MafMgmtSpiInterfacePortal_3T *portal_MAF, SaAisErrorT (*subscriberFunction)(void), SaAisErrorT (*unsubscriberFunction)(void))
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("start_CmEventHandler(): ENTER");
	/* if event handler instance exists:
	 *
	 *       -stop event handler
	 *       -delete event handler
	 */
	/*if(cmEventHandler != NULL)
	{
		DEBUG_OAMSA_CMEVENT("start_CmEventHandler(): start called while event producer instance already exists, so trying to stop and delete before starting it again");
		if(cmEventHandler->stop() != MafOk)
		{
			DEBUG_OAMSA_CMEVENT("start_CmEventHandler(): stopping of cmEventHandler failed, returning MafFailure");
			LEAVE_OAMSA_CMEVENT();
			return MafFailure;
		}
		delete cmEventHandler;
	}*/

	// instantiate the CM Notification Cache
	newCmNotificationCache();

	// instantiate new CmEventHandler
	DEBUG_OAMSA_CMEVENT("start_CmEventHandler(): create new CmEventHandler");
	cmEventHandler = new CmEventHandler(*portal_MAF, subscriberFunction, unsubscriberFunction);

	// before calling start, check if the allocation went well
	if(cmEventHandler != NULL)
	{
		DEBUG_OAMSA_CMEVENT("start_CmEventHandler(): starting CmEventHandler");
		if(cmEventHandler->start() == MafOk)
		{
			DEBUG_OAMSA_CMEVENT("start_CmEventHandler(): CmEventHandler successfully started, return MafOk");
			LEAVE_OAMSA_CMEVENT();
			return MafOk;
		}
		else
		{
			DEBUG_OAMSA_CMEVENT("start_CmEventHandler(): CmEventHandler not started, return MafFailure");
			LEAVE_OAMSA_CMEVENT();
			return MafFailure;
		}
	}
	else
	{
		DEBUG_OAMSA_CMEVENT("start_CmEventHandler(): failed to allocate memory for CmEventHandler, return MafFailure");
		LEAVE_OAMSA_CMEVENT();
		return MafFailure;
	}
}

/* This function pushes one CM event to CmEventHandler
 * CmEventHandler will send the event to the consumers. If that fails then this function will also fail.
 *
 * return values:
 *
 *     MafOK     : if the whole event sending chain went well
 *     MafFailure: if there was a problem in the event sending
 */
MafReturnT push_CmEventHandler(MafOamSpiCmEvent_Notification_1T *mafCmNot)
{
	ENTER_OAMSA_CMEVENT();
	if(mafCmNot == NULL)
	{
		DEBUG_OAMSA_CMEVENT("push_CmEventHandler(): mafCmNot is NULL, returning MafFailure");
		LEAVE_OAMSA_CMEVENT();
		return MafFailure;
	}

	DEBUG_OAMSA_CMEVENT("push_CmEventHandler(): push mafCmNot(%p)",mafCmNot);
	if(cmEventHandler != NULL)
	{
		if(cmEventHandler->pushEvent(*mafCmNot) == MafOk)
		{
			DEBUG_OAMSA_CMEVENT("push_CmEventHandler(): pushEvent ok, returning MafOk");
			LEAVE_OAMSA_CMEVENT();
			return MafOk;
		}
		else
		{
			DEBUG_OAMSA_CMEVENT("push_CmEventHandler(): pushEvent failed, returning MafFailure");
			LEAVE_OAMSA_CMEVENT();
			return MafFailure;
		}
	}
	else
	{
		// this error case should not happen (cmEventHandler not exists): it must be started before pushing the events.
		// if it happens then it means that the cmEventHandler stopped
		// Not pushing the event
		DEBUG_OAMSA_CMEVENT("push_CmEventHandler(): ERROR case: cmEventHandler not exists, returning MafFailure");
		LEAVE_OAMSA_CMEVENT();
		return MafFailure;
	}
}

/* This function stops the CmEventHandler
 *
 * return values:
 *
 *     MafOK     : if CmEventHandler successfully stopped
 *     MafFailure: if the proper stopping of the CmEventHandler failed
 */
MafReturnT stop_CmEventHandler(void)
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("stop_CmEventHandler(): ENTER");
	DEBUG_OAMSA_CMEVENT("stop_CmEventHandler(): stop CM Notification Cache");
	MafReturnT rc = MafOk;

	if(cmNotificationCache) {
		delete cmNotificationCache;
		cmNotificationCache = NULL;
	}

	if(cmEventHandler)
	{
		if(cmEventHandler->stop() == MafOk)
		{
			DEBUG_OAMSA_CMEVENT("stop_CmEventHandler(): successfully stopped, returning MafOk");
			delete cmEventHandler;
			cmEventHandler = NULL;
			LEAVE_OAMSA_CMEVENT();
			rc = MafOk;
		}
		else
		{
			DEBUG_OAMSA_CMEVENT("stop_CmEventHandler(): stopping failed, returning MafFailure");
			LEAVE_OAMSA_CMEVENT();
			rc = MafFailure;
		}
	}
	return rc;
}

/*******************************************************
 *  Interface functions of the CM Notification Cache:
 *
 *           -newCmNotificationCache
 *
 *******************************************************/

/* This function instantiate the CM Notification Cache.
 * Only one instance of CM Notification Cache can exists at a time.
 *
 * return values:
 *
 *     MafOK     : if CM Notification Cache successfully instantiated
 *     MafFailure: if CM Notification Cache not instantiated
 */
MafReturnT newCmNotificationCache()
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("newCmNotificationCache(): ENTER");
	// if CM Notification Cache instance exists then delete it
	if(cmNotificationCache != NULL)
	{
		DEBUG_OAMSA_CMEVENT("newCmNotificationCache(): CM Notification Cache instance already exists, so delete it");
		//delete cmNotificationCache;
	}

	// instantiate new CM Notification Cache
	DEBUG_OAMSA_CMEVENT("newCmNotificationCache(): create new CM Notification Cache");
	cmNotificationCache = new CmNotificationCache();

	// before calling start, check if the allocation went well
	if(cmNotificationCache == NULL)
	{
		DEBUG_OAMSA_CMEVENT("newCmNotificationCache(): failed to allocate memory for CM Notification Cache, return MafFailure");
		LEAVE_OAMSA_CMEVENT();
		return MafFailure;
	}
	LEAVE_OAMSA_CMEVENT();
	return MafOk;
}

// This function adds data to the CM Notification Cache.
MafReturnT addToCmCache(SaImmOiCcbIdT ccbId, const char *attrName, const char *immRdn, const char *memberName, MafMoAttributeValueContainer_3T *memberValue)
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("addToCmCache(): ENTER with ccbId (%llu) attrName (%s) immRdn (%s) memberName (%s)",ccbId, attrName, immRdn, memberName);
	if(memberValue == NULL)
	{
		DEBUG_OAMSA_CMEVENT("memberValue = NULL");
		return MafFailure;
	}
	DEBUG_OAMSA_CMEVENT("addToCmCache(): memberValue:%s",attrValueContainerToString(*memberValue).c_str());

	MafReturnT ret = MafFailure;
	std::string memberNameStr;
	// memberName can be NULL, and string constructor doesn't allow NULL as input
	if(memberName != NULL)
	{
		memberNameStr = memberName;
	}
	if(NULL != cmNotificationCache) {
		ret = cmNotificationCache->addAttr(ccbId, attrName, immRdn, memberNameStr, memberValue);
	}
	DEBUG_OAMSA_CMEVENT("addToCmCache(): return with %d",ret);
	LEAVE_OAMSA_CMEVENT();
	return ret;
}

// This function adds data to the CM Notification Cache.
MafReturnT setDnInCmCache(SaImmOiCcbIdT ccbId, MafOamSpiCmEvent_SourceIndicator_1T sourceIndicator, const char *dn, MafOamSpiCmEvent_EventType_1T eventType, bool ccbLast)
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("setDnInCmCache(): ENTER with ccbId (%llu) sourceIndicator (%d) dn (%s) eventType (%d) ccbLast (%d)",ccbId, sourceIndicator, dn, eventType, ccbLast);

	MafReturnT ret = MafFailure;
	if((NULL != cmNotificationCache) && (MafOk == cmNotificationCache->setDn(ccbId, sourceIndicator, dn, eventType, ccbLast)))
	{
		ret = MafOk;
	}
	DEBUG_OAMSA_CMEVENT("setDnInCmCache(): return with %d",ret);
	LEAVE_OAMSA_CMEVENT();
	return ret;
}

/*******************************************************
 *  Support functions
 *******************************************************/

std::string attrValueContainerToString(MafMoAttributeValueContainer_3T &avc)
{
	std::string avcStr;
	char b[100];
	switch(avc.type)
	{
	case MafOamSpiMoAttributeType_3_INT8:
		avcStr = " attrType: INT8";
		sprintf(b,"%u", avc.nrOfValues);
		avcStr += " nrOfValues: " + std::string(b) + " values:";
		for(unsigned int i = 0; i < avc.nrOfValues; i++)
		{
			char buf[100];
			sprintf(buf,"%d", avc.values[i].value.i8);
			avcStr += " " + std::string(buf);
		}
		break;
	case MafOamSpiMoAttributeType_3_INT16:
		avcStr = " attrType: INT16";
		sprintf(b,"%u", avc.nrOfValues);
		avcStr += " nrOfValues: " + std::string(b) + " values:";
		for(unsigned int i = 0; i < avc.nrOfValues; i++)
		{
			char buf[100];
			sprintf(buf,"%d", avc.values[i].value.i16);
			avcStr += " " + std::string(buf);
		}
		break;
	case MafOamSpiMoAttributeType_3_INT32:
		avcStr = " attrType: INT32";
		sprintf(b,"%u", avc.nrOfValues);
		avcStr += " nrOfValues: " + std::string(b) + " values:";
		for(unsigned int i = 0; i < avc.nrOfValues; i++)
		{
			char buf[100];
			sprintf(buf,"%d", avc.values[i].value.i32);
			avcStr += " " + std::string(buf);
		}
		break;
	case MafOamSpiMoAttributeType_3_INT64:
		avcStr = " attrType: INT64";
		sprintf(b,"%u", avc.nrOfValues);
		avcStr += " nrOfValues: " + std::string(b) + " values:";
		for(unsigned int i = 0; i < avc.nrOfValues; i++)
		{
			char buf[100];
			sprintf(buf,"%ld", avc.values[i].value.i64);
			avcStr += " " + std::string(buf);
		}
		break;
	case MafOamSpiMoAttributeType_3_UINT8:
		avcStr = " attrType: UINT8";
		sprintf(b,"%u", avc.nrOfValues);
		avcStr += " nrOfValues: " + std::string(b) + " values:";
		for(unsigned int i = 0; i < avc.nrOfValues; i++)
		{
			char buf[100];
			sprintf(buf,"%u", avc.values[i].value.u8);
			avcStr += " " + std::string(buf);
		}
		break;
	case MafOamSpiMoAttributeType_3_UINT16:
		avcStr = " attrType: UINT16";
		sprintf(b,"%u", avc.nrOfValues);
		avcStr += " nrOfValues: " + std::string(b) + " values:";
		for(unsigned int i = 0; i < avc.nrOfValues; i++)
		{
			char buf[100];
			sprintf(buf,"%u", avc.values[i].value.u16);
			avcStr += " " + std::string(buf);
		}
		break;
	case MafOamSpiMoAttributeType_3_UINT32:
		avcStr = " attrType: UINT32";
		sprintf(b,"%u", avc.nrOfValues);
		avcStr += " nrOfValues: " + std::string(b) + " values:";
		for(unsigned int i = 0; i < avc.nrOfValues; i++)
		{
			char buf[100];
			sprintf(buf,"%u", avc.values[i].value.u32);
			avcStr += " " + std::string(buf);
		}
		break;
	case MafOamSpiMoAttributeType_3_UINT64:
		avcStr = " attrType: UINT64";
		sprintf(b,"%u", avc.nrOfValues);
		avcStr += " nrOfValues: " + std::string(b) + " values:";
		for(unsigned int i = 0; i < avc.nrOfValues; i++)
		{
			char buf[100];
			sprintf(buf,"%lu", avc.values[i].value.u64);
			avcStr += " " + std::string(buf);
		}
		break;
	case MafOamSpiMoAttributeType_3_STRING:
		avcStr = " attrType: STRING";
		sprintf(b,"%u", avc.nrOfValues);
		avcStr += " nrOfValues: " + std::string(b) + " values:";
		for(unsigned int i = 0; i < avc.nrOfValues; i++)
		{
			avcStr += " " + std::string(avc.values[i].value.theString);
		}
		break;
	case MafOamSpiMoAttributeType_3_BOOL:
		avcStr = " attrType: BOOL";
		sprintf(b,"%u", avc.nrOfValues);
		avcStr += " nrOfValues: " + std::string(b) + " values:";
		for(unsigned int i = 0; i < avc.nrOfValues; i++)
		{
			char buf[100];
			sprintf(buf,"%d", avc.values[i].value.theBool);
			avcStr += " " + std::string(buf);
		}
		break;
	case MafOamSpiMoAttributeType_3_REFERENCE:
		DEBUG_OAMSA_CMEVENT("attrValueContainerToString(): not supported type");
		avcStr = " attrType: REFERENCE";
		break;
	case MafOamSpiMoAttributeType_3_ENUM:
		avcStr = " attrType: ENUM";
		sprintf(b,"%u", avc.nrOfValues);
		avcStr += " nrOfValues: " + std::string(b) + " values:";
		for(unsigned int i = 0; i < avc.nrOfValues; i++)
		{
			char buf[100];
			sprintf(buf,"%ld", avc.values[i].value.theEnum);
			avcStr += " " + std::string(buf);
		}
		break;
	case MafOamSpiMoAttributeType_3_STRUCT:

		break;
	case MafOamSpiMoAttributeType_3_DECIMAL64:
		avcStr = " attrType: DECIMAL64";
		sprintf(b,"%u", avc.nrOfValues);
		avcStr += " nrOfValues: " + std::string(b) + " values:";
		for(unsigned int i = 0; i < avc.nrOfValues; i++)
		{
			char buf[100];
			sprintf(buf,"%f", avc.values[i].value.decimal64);
			avcStr += " " + std::string(buf);
		}
		break;
	default:
		DEBUG_OAMSA_CMEVENT("attrValueContainerToString(): invalid type");
		avcStr = "";
		break;
	}
	return avcStr;
}
/*
 * Return Unix time in nanoseconds.
 * If not possible to get the Unix time, then return 0.
 */
uint64_t getTime(void)
{
	ENTER_OAMSA_CMEVENT();
	uint64_t theTime = 0;
	struct timespec ts;
	if (clock_gettime(CLOCK_REALTIME, &ts) == 0)
	{
		theTime = ((uint64_t)ts.tv_nsec) + ((uint64_t)ts.tv_sec) * NANOS_PER_SEC;
	}
	else
	{
		ERR_OAMSA_CMEVENT("getTime(): Failed to get time, errno=<%s>", strerror(errno));
	}
	DEBUG_OAMSA_CMEVENT("getTime(): time value is: (%lu)",theTime);
	LEAVE_OAMSA_CMEVENT();
	return theTime;
}

SaUint64T memgetAVC(const MafMoAttributeValueContainer_3T *avc)
{
	SaUint64T memCount = 0;
	if(avc == NULL) return memCount;
	memCount += sizeof(MafMoAttributeValueContainer_3T);
	if(avc->values == NULL)
	{
		DEBUG_OAMSA_CMEVENT("memgetAVC(): values container is NULL, memory count: %llu", memCount);
		return memCount;
	}
	memCount += (avc->nrOfValues * sizeof(MafMoAttributeValue_3T));
	for(unsigned int i = 0; i < avc->nrOfValues; i++)
	{
		if(avc->type == MafOamSpiMoAttributeType_3_STRING)
		{
			memCount += (strlen(avc->values[i].value.theString) * sizeof(char));
		}
		else if(avc->type == MafOamSpiMoAttributeType_3_REFERENCE)
		{
			memCount += (strlen(avc->values[i].value.moRef) * sizeof(char));
		}
	}
	DEBUG_OAMSA_CMEVENT("memgetAVC(): returning memory count: %llu", memCount);

	return memCount;
}
