#ifndef CM_EVENT_HANDLER_H
#define CM_EVENT_HANDLER_H

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
* File:   CmEventHandler.h
*
* Author: eaparob
*
* Date:   2013-02-05
*
* This file declares the needed functions for forwarding of notifications
* of type object_create/object_delete and attribute_change notifications from a SAF notification service. The
* notifications are converted to a suitable format for COM/MAF.
*
* Modified: eaparob 2013-03-21:   Implemented the CM Notification Cache
*
* Modified: eaparob 2013-05-03:   Functionality of CM Cache extended to handle simple types as inputs
*
*******************************************************************************/

#include <time.h>
#include <errno.h>
#include "ComSA.h"
#include "trace.h"
#include "ComSANtf.h"
#include "MafMgmtSpiInterfacePortal_3.h"

#ifndef UNIT_TEST
#include "CmEventHandlerInterface.h"
#include "CmEventProducer.h"
#else
#include <map>
#include <list>
#include <string>
#include <saImmOi.h>
#include "MafOamSpiCmEvent_1.h"
#define tempCacheKey "TEMP_CACHE"
#include "CmCache_unittest.h"
#endif

//SaImmOiCcbIdT ccbId_T;
//typedef unsigned long long ccbId_T;

class CmEventHandler;
//template <typename CacheKeyT, class CacheValueT> class BaseCache;
class CmNotificationCache;
class CcbCache;
class DnCache;
class AvcCache;
class StructElementCache;

static CmEventHandler *cmEventHandler = NULL;
static CmNotificationCache *cmNotificationCache = NULL;

class CmEventHandler
{
public:
	CmEventHandler(MafMgmtSpiInterfacePortal_3T& portal_MAF, SaAisErrorT (*subscriberFunction)(void), SaAisErrorT (*unsubscriberFunction)(void));
	~CmEventHandler();
	MafReturnT start(void);
	MafReturnT pushEvent(MafOamSpiCmEvent_Notification_1T &mafCmNot);
	MafReturnT stop(void);

	bool isDiscardedCcb(SaImmOiCcbIdT ccbId);
	void addDiscardedCcb(SaImmOiCcbIdT ccbId);
	void removeDiscardedCcb(SaImmOiCcbIdT ccbId);
	void buildOverflowEvent(MafOamSpiCmEvent_Notification_1T* overflowEvent);
	void setNotificationCacheDiscardMode(bool discardMode);
	bool isNotificationCacheInDiscardMode();
private:
	MafReturnT sendEvent(MafOamSpiCmEvent_Notification_1T &mafCmNot);
	MafMgmtSpiInterfacePortal_3T portal;
	SaAisErrorT (*subscriberFunc)(void);
	SaAisErrorT (*unsubscriberFunc)(void);

	/* This contains the active CCB IDs that are discarded due to
	 * high memory consumption.
	 */
	std::list<SaImmOiCcbIdT> _discardedCcbList;
	bool _notificationCacheInDiscardMode;
#ifndef UNIT_TEST
	static CmEventProducer* _cmEventProducer;

        /* mutex and supported functions */
	pthread_mutex_t _cmEventHandlerMutex;
        bool initializeMutex();
        void aquireMutex();
        void releaseMutex();
        bool destroyMutex();
#endif
};

template <typename CacheKeyT, class CacheValueT>
class BaseCache
{
public:
	// typedefs
	typedef std::map<CacheKeyT,CacheValueT*> cacheMap_T;
	// need to help out the compiler with typename
	typedef typename cacheMap_T::iterator cacheMapIterator_T;
	// enum type for attributes
	typedef enum  {
		simpleType = 1,
		configStructType = 2,
		runtimeStructType = 3,
		referenceType = 4,
		unknownType = 5
	} attributeType;
	BaseCache() : cacheMemory(std::make_pair(0,0)), freeInternalData(true)
	{
		ENTER_OAMSA_CMEVENT();
		DEBUG_OAMSA_CMEVENT("BaseCache::BaseCache(): constructor called");
		if(initializeMutex())
		{
			DEBUG_OAMSA_CMEVENT("BaseCache(): pthread_mutex_init failed");
		}
		LEAVE_OAMSA_CMEVENT();
	}
	~BaseCache()
	{
		ENTER_OAMSA_CMEVENT();
		DEBUG_OAMSA_CMEVENT("BaseCache::~BaseCache(): destructor called");
		if(freeInternalData || cmEventHandler->isNotificationCacheInDiscardMode())
		{
			aquireMutex();
			for(cacheMapIterator_T i = cacheMap.begin(); i != cacheMap.end();i++)
			{
				DEBUG_OAMSA_CMEVENT("BaseCache::~BaseCache():    freeing memory of the value in the map");
				delete i->second;
				i->second = NULL;
			}
			cacheMap.clear();
			releaseMutex();
		}
		if(destroyMutex())
		{
			DEBUG_OAMSA_CMEVENT("BaseCache(): pthread_mutex_destroy failed");
		}
		DEBUG_OAMSA_CMEVENT("BaseCache::~BaseCache(): destructing finished");
		LEAVE_OAMSA_CMEVENT();
	}
	// map
	cacheMap_T cacheMap;

	// bytes used <self, container>
	std::pair<SaUint64T, SaUint64T> cacheMemory;

	// methods
	bool existsCache(CacheKeyT key)
	{
		ENTER_OAMSA_CMEVENT();
		bool retVal = false;
		aquireMutex();
		cacheMapIterator_T i = cacheMap.find(key);
		if(i != cacheMap.end())
		{
			DEBUG_OAMSA_CMEVENT("BaseCache::existsCache(): cache found in map, returning true");
			retVal = true;
		}
		else
		{
			DEBUG_OAMSA_CMEVENT("BaseCache::existsCache(): cache not found in map, returning false");
		}
		LEAVE_OAMSA_CMEVENT();
		releaseMutex();
		return retVal;
	}

	CacheValueT* getCache(CacheKeyT key)
	{
		ENTER_OAMSA_CMEVENT();
		CacheValueT* pCache = NULL;
		if(existsCache(key))
		{
			pCache = getCacheFromMap(key);
			DEBUG_OAMSA_CMEVENT("BaseCache::getCache(): cache found, returning reference of cache");
			LEAVE_OAMSA_CMEVENT();
			return pCache;
		}
		else
		{
			DEBUG_OAMSA_CMEVENT("BaseCache::getCache(): cache not found, create new cache");
			CacheValueT *pTemp = new CacheValueT;
			if (maxSAMemoryForCMEvents)
				pTemp->cacheMemory.first = sizeof(CacheValueT);
			DEBUG_OAMSA_CMEVENT("BaseCache::getCache(): calling addToCacheMap()");
			addToCacheMap(key, pTemp);
			DEBUG_OAMSA_CMEVENT("BaseCache::getCache(): calling getCacheFromMap()");
			pCache = getCacheFromMap(key);
			DEBUG_OAMSA_CMEVENT("BaseCache::getCache(): returning the new cache");
			LEAVE_OAMSA_CMEVENT();
			return pCache;
		}
	}
	void addToCacheMap(CacheKeyT key, CacheValueT* value)
	{
		ENTER_OAMSA_CMEVENT();
		DEBUG_OAMSA_CMEVENT("BaseCache::addToCacheMap(): adding cache to cacheMap");
		aquireMutex();
		if(cacheMap.find(key) != cacheMap.end())
		{
			DEBUG_OAMSA_CMEVENT("BaseCache::addToCacheMap(): cache already exists");
		}
		cacheMap.insert(std::pair<CacheKeyT,CacheValueT*>(key, value));
		releaseMutex();
		DEBUG_OAMSA_CMEVENT("BaseCache::addToCacheMap(): return");
		LEAVE_OAMSA_CMEVENT();
	}
	void deleteCacheFromMap(CacheKeyT key, bool freeMemory = true)
	{
		ENTER_OAMSA_CMEVENT();
		DEBUG_OAMSA_CMEVENT("BaseCache::deleteCache(): delete cache from cacheMap");
		aquireMutex();
		cacheMapIterator_T i = cacheMap.find(key);
		if(i != cacheMap.end())
		{
			if(freeMemory)
			{
				DEBUG_OAMSA_CMEVENT("BaseCache::deleteCache():    freeing memory of the value in the map");
				delete i->second;
				i->second = NULL;
			}
			cacheMap.erase(i);
		}
		releaseMutex();
		LEAVE_OAMSA_CMEVENT();
	}
	void skipFreeingInsideDestructor(void)
	{
		freeInternalData = false;
	}
	int size(void)
	{
		ENTER_OAMSA_CMEVENT();
		int mapSize = cacheMap.size();
		DEBUG_OAMSA_CMEVENT("BaseCache::size(): cache map size: (%d)",mapSize);
		LEAVE_OAMSA_CMEVENT();
		return mapSize;
	}
	SaUint64T getMemory()
	{
		aquireMutex();
		SaUint64T retVal = cacheMemory.first + cacheMemory.second;
		DEBUG_OAMSA_CMEVENT("BaseCache::getMemory(): memoryUsage : (%llu)", retVal);
		releaseMutex();
		return retVal;
	}
	void setMemory(SaUint64T bytes, bool memoryDeleted = false)
	{
		aquireMutex();
		if (!memoryDeleted)
			cacheMemory.second += bytes;
		else {
			if (cacheMemory.second < bytes) {
				DEBUG_OAMSA_CMEVENT("BaseCache::setMemory(): deleted memory is more than calculated memory. Reseting memory counter.");
				cacheMemory.second = 0;
			}
			else
				cacheMemory.second -= bytes;
		}
		releaseMutex();
	}

	void resetMemory(SaUint64T bytes = 0)
	{
		aquireMutex();
		cacheMemory.second = bytes;
		releaseMutex();
	}

private:
	bool freeInternalData;
	CacheValueT* getCacheFromMap(CacheKeyT key)
	{
		ENTER_OAMSA_CMEVENT();
		CacheValueT* retCacheValue = (CacheValueT*)NULL;
		aquireMutex();
		cacheMapIterator_T i = cacheMap.find(key);
		if(i != cacheMap.end())
		{
			DEBUG_OAMSA_CMEVENT("BaseCache::getCacheFromMap(): cache found in map, returning pointer of cache");
			retCacheValue = i->second;
		}
		else
		{
			DEBUG_OAMSA_CMEVENT("BaseCache::getCacheFromMap(): cache not found in map, returning NULL");
		}
		LEAVE_OAMSA_CMEVENT();
		releaseMutex();
		return retCacheValue;
	}
    /* mutex and supported functions */
	pthread_mutex_t _cacheMutex;

	bool initializeMutex()
	{
		return (0 == pthread_mutex_init(&_cacheMutex, NULL)) ? true : false;
	}

	void aquireMutex()
	{
		pthread_mutex_lock(&_cacheMutex);
	}

	void releaseMutex()
	{
		pthread_mutex_unlock(&_cacheMutex);
	}

	bool destroyMutex()
	{
		return (0 == pthread_mutex_destroy(&_cacheMutex)) ? true : false;
	}
};

class CmNotificationCache : public BaseCache<SaImmOiCcbIdT, CcbCache>
{
public:
	CmNotificationCache();
	~CmNotificationCache();
	MafReturnT addAttr(SaImmOiCcbIdT ccbId, std::string attrName, std::string immRdn, std::string memberName, MafMoAttributeValueContainer_3T *memberValue);
	MafReturnT setDn(SaImmOiCcbIdT ccbId, MafOamSpiCmEvent_SourceIndicator_1T sourceIndicator, std::string dn, MafOamSpiCmEvent_EventType_1T eventType, bool ccbLast);
	/*
	 * 	Function to update the bytes allocated for the respective containers passed
	 * 	@param [IN] memoryValue contains memory change in bytes to be updated.
	 * 	@param [IN] cmCache CmNotificationCache container, its memory counter gets updated with memoryValue
	 * 	@param [IN] ccbCache CcbCache container, its memory counter gets updated with memoryValue
	 * 	@param [IN] dnCache DnCache container, its memory counter gets updated with memoryValue
	 * 	@param [IN] avcCache AvcCache container, its memory counter gets updated with memoryValue
	 * 	For ccbCache, dnCache, avcCache, if NULL is passed memoryValue will not be updated.
	 * 	@param [IN] memoryDeleted true, if the memoryValue is to be decremented.
	 * 	                          false, memory will be incremented.
	 */
	void updateMemory(SaUint64T memoryValue,
			CmNotificationCache* CmCache,
			CcbCache* ccbCache = NULL,
			DnCache* dnCache = NULL,
			AvcCache* avcCache = NULL,
			bool memoryDeleted = false);
private:
	bool isStruct(std::string immRdn);
	attributeType getAttrType(SaImmOiCcbIdT ccbId, std::string immRdn, MafMoAttributeValueContainer_3T *avc);
	/*
	 * Function to discard all the CCB's if memory consumption is high
	 */
	void discardAllCcbs();
};

// This class handles a CM Event Structure instance which is identified by a ccbId
class CcbCache : public BaseCache<std::string, DnCache>
{
public:
//	friend class CmNotificationCache;
//private:
	CcbCache();
	~CcbCache();
	MafOamSpiCmEvent_SourceIndicator_1T sourceIndicator;
	bool tempCacheExists(void);
	DnCache* getTempCache(void);
	void deleteTempCache(void);
	MafOamSpiCmEvent_Notification_1T* buildEventStruct(MafOamSpiCmEvent_Notification_1T* ES);
private:
	MafOamSpiTransactionHandleT txHandle;
    void setDn(std::string dn);
    void buildAVC_simpleType(std::string attrName, AvcCache *avcCache, MafMoAttributeValueContainer_3T* avc);
    void buildAVC_configStructType(std::string attrName, AvcCache *avcCache, MafMoAttributeValueContainer_3T* avc);
    void buildAVC_runtimeStructType(std::string attrName, AvcCache *avcCache, MafMoAttributeValueContainer_3T* avc);
    // assigned outside of class declaration
#ifndef UNIT_TEST
    static const std::string tempCacheKey;
#endif
};
#ifndef UNIT_TEST
const std::string CcbCache::tempCacheKey = "TEMP_CACHE";
#endif

// This class handles an event (of the array of events in CM Event Structure) which is identified by a DN
class DnCache : public BaseCache<std::string, AvcCache>
{
public:
	DnCache();
	~DnCache();
	void setEventType(MafOamSpiCmEvent_EventType_1T eventType);
	MafOamSpiCmEvent_EventType_1T getEventType(void);
	void setReferenceCalled(void);
	bool isReferenceCalled(void);
	void setRuntimeStructObjectCalled();
	bool isRuntimeStructObjectCalled(void);
	typedef struct
	{
		bool create;
		bool mod;
		bool del;
	} EventTypeSet;
	EventTypeSet eventTypeSet;
private:
	bool modifiedReference;
	bool modifiedRuntimeStructObject;
};

/* This class handles an attribute (of the array of attributes under an event in CM Event Structure) which is identified by an attributeName.
 * One instance of AvcCache stores multiple StructElementCaches (AVCs) which are elements of the multi-value struct array.
 */
class AvcCache : public BaseCache<std::string, StructElementCache>
{
public:
	AvcCache();
	~AvcCache();
	bool IsConfigStructType();
	bool IsRuntimeStructType();
	MafMoAttributeValueContainer_3T* avcSimple;
	MafMoAttributeValueContainer_3T* avcReference;
	MafMoAttributeValueContainer_3T* avcRuntimeStruct;
};

/* This class handles an element of a multi-value struct array.
 * It has a list to store the struct member data values (pairs of struct member name and value).
 */
class StructElementCache
{
public:
	// This map type is used to store only 1 key-value pair.
	// key:   struct memberName
	// value: struct memberValue
	typedef std::map<std::string, MafMoAttributeValueContainer_3T*> memberDataT;

	typedef std::list<memberDataT> structElementListT;
	typedef structElementListT::iterator structElementListIteratorT;
	StructElementCache();
	~StructElementCache();
	bool addToStructElementList(std::string memberName, MafMoAttributeValueContainer_3T *memberValue);

	// This function counts the number of struct members in an AVC
	//int getNrOfStructMembers(std::string &attrName);

	SaUint64T getMemory();
	void resetMemory(SaUint64T bytes = 0);
	void setMemory(SaUint64T bytes, bool memoryDeleted = false);

	// the list that stores the struct member data (pairs of struct member name and value).
	structElementListT structElementList;

	// bytes used <self, container>
	std::pair<SaUint64T, SaUint64T> cacheMemory;
};

MafReturnT start_CmEventHandler(MafMgmtSpiInterfacePortal_3T *portal_MAF, SaAisErrorT (*subscriberFunction)(void), SaAisErrorT (*unsubscriberFunction)(void));
MafReturnT push_CmEventHandler(MafOamSpiCmEvent_Notification_1T *mafCmNot);
MafReturnT stop_CmEventHandler(void);

MafReturnT newCmNotificationCache();
MafReturnT addToCmCache(SaImmOiCcbIdT ccbId, const char *attrName, const char *immRdn, const char *memberName, MafMoAttributeValueContainer_3T *memberValue);
MafReturnT setDnInCmCache(SaImmOiCcbIdT ccbId, MafOamSpiCmEvent_SourceIndicator_1T sourceIndicator, const char *dn, MafOamSpiCmEvent_EventType_1T eventType, bool ccbLast);

std::string attrValueContainerToString(MafMoAttributeValueContainer_3T &avc);
uint64_t getTime(void);
SaUint64T memgetAVC(const MafMoAttributeValueContainer_3T *avc);


#endif
