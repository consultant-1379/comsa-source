#ifndef __OAMSA_CACHE_H
#define __OAMSA_CACHE_H

#include <memory.h>
#include <string.h>
#include <map>
#include <list>
#include <vector>
#include <string>
// CoreMW
#include "saAis.h"
#include "saImm.h"
#include "saImmOm.h"
// COM
#include "MafOamSpiManagedObject_3.h"
// COMSA
#include "trace.h"

class OamSACache
{
public:
	// Type definitions
 	typedef enum {eObjectSuccess  = 0,
				  eObjectDeleted  = 1,
				  eObjectNotFound = 2,
				  eObjectAlreadyExists = 3} ObjectState;
	typedef std::list<std::string>						DNList;				// These types are used for the object names when it's split into
	typedef std::list<std::string>						MocPathList;		// These types are used for the MO class path when it's split into
	typedef std::list<std::string>::iterator 			DNListIterator;		// sub parts
	typedef std::list<std::string>::reverse_iterator 	DNListReverseIterator;
 				  
 	// Constructors			  
	OamSACache();
	
	// Destructors
	~OamSACache();
};

extern void GlobalSplitDN(const std::string& theDN, OamSACache::DNList& theSplitDN);
extern void GlobalSplitStrDN(const char *theDN, OamSACache::DNList& theSplitDN);
extern void GlobalSplitMocPath(const std::string& mocPath, OamSACache::MocPathList& theSplitPath);

#endif //__OAMSA_CACHE_H
