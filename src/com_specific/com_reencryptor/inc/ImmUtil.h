#ifndef __REENCRYPTOR_IMMCMD_H
#define __REENCRYPTOR_IMMCMD_H

#include "Defines.h"

//for IMM APIs
#include "saAis.h"
#include "saImm.h"
#include "saImmOm.h"
#include "saImmOi.h"

#include <pthread.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <list>
#include <vector>
#include <map>

static std::string EmptyString;

class ImmUtil
{
	typedef struct {
		SaUint16T length;
		SaUint8T value[SA_MAX_NAME_LENGTH];
	} TheSaNameT;

	typedef std::map<std::string, std::vector<std::string> > AttrValMapT;
	typedef std::map<std::string, std::vector<std::string> >::iterator AttrValMapIterT;

	typedef struct CcbModifiedAttribute {

		AttrValMapT attrValMap;

		CcbModifiedAttribute();
		CcbModifiedAttribute(const std::string& attribute, const std::string& value);

		~CcbModifiedAttribute();

		void addTo(const std::string& attribute, const std::string& value);

	} CcbModifiedAttributeT;

	typedef std::map<std::string, CcbModifiedAttributeT> CcbModifiedAttributesMapT;
	typedef std::map<std::string, CcbModifiedAttributeT>::iterator CcbModifiedAttributesMapIterT;

	SaVersionT _immVersion;

	// OI stuff
	SaImmOiHandleT            _immOiApplierHandle;
	SaSelectionObjectT        _selectionObject;
	pthread_t                 _dispatchThread;
	bool                      _exitDispatchThread;
	const std::string         _implementerName;
	const std::string         _classImplementerName;
	const SaDispatchFlagsT    _dispatchFlags;

	// OM Stuff
	SaImmHandleT              _immOmHandle;
	SaImmAccessorHandleT      _immAccessorHandle;

	SaImmSearchHandleT        _immSearchHandle;
	const SaImmScopeT         _searchScope;
	const SaImmSearchOptionsT _searchOptions;

	SaImmAdminOwnerHandleT    _adminOwnerHandle;
	const SaImmScopeT         _adminOwnerScope;
	const std::string         _immAdminOwnerName;

	// CCB Stuff
	SaImmCcbHandleT           _ccbHandle;
	const SaImmCcbFlagsT      _ccbFlags;

	// Internal Cache for CCB stuff
	std::list<std::string>    _lockedObjectsList; /* List of IMM Objects that are locked as part of on-going CCB */
	CcbModifiedAttributesMapT _ccbObjects; /* Cached Data of all changes for a CCB */

	ImmUtil();

	//OI utilities
	bool initializeImmOiApplierHandle(SaImmOiCallbacksT_2* pImmOiApplierCallbacks);
	bool finalizeImmOiApplierHandle();

	bool setImplementerName();
	bool clearImplementerName();

	//TODO: use saImmOiObjectImplementerSet instead of class.
	bool setClassImplementer();
	bool releaseClassImplementer();

	//OM utilities
	bool initializeImmOmHandle();
	bool finalizeImmOmHandle();

	bool initializeImmOmAccessor();
	bool finalizeImmOmAccessor();

	bool initializeImmOmSearch(const std::string& parent, const std::string& className);
	void finalizeImmOmSearch();

	bool initializeAdminOwner();
	bool finalizeAdminOwner();
	bool setAdminOwner(std::string immObject);
	bool releaseAdminOwner();

	bool initializeCcb();
	void closeTransaction(bool abortTransaction = false);

	std::string getErrorStrings();

public:

	static inline ImmUtil& Instance() {
		static ImmUtil immUtil;
		return immUtil;
	}

	~ImmUtil();

	bool stop();
	bool registerApplierOI(SaImmOiCallbacksT_2* pImmOiApplierCallbacks);
	bool unregisterApplierOI();
	bool dispatch();

	SaAisErrorT getAttributeValue(const std::string& immDn, const std::string& attribute, SaImmAttrValuesT_2*** attrValues);
	bool findImmObjects(const std::string& parent, const std::string& className, std::list<std::string>& objects);
	bool createObjectSecEncryptionParticipant();
	void addModifyObject(const std::string& object, const std::string& attribute, const std::string& value);
	bool applyTransaction(std::string& oiErrorStrings = EmptyString);
	void abortTransaction();
	void clearCcbObjects();

	SaNameT* makeSaNameT(const std::string& name);
	std::string makeString(const SaNameT* immObject);
};

#endif
