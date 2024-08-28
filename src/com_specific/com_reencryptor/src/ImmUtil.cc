#include "ImmUtil.h"
#include "Trace.h"

#include <poll.h>

#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <sstream>

void* startDispatchThread(void* arg);

#define IMM_API_CALL(F) { \
		int count = 0; \
		do { \
			if (count++) { \
				usleep(USLEEP_HALF_SECOND); \
			} \
			rc = F; \
		} while (((SA_AIS_ERR_TRY_AGAIN == rc) || (SA_AIS_ERR_BUSY == rc)) && (count <= IMM_MAX_RETRIES)); \
}

#define HANDLE_IMM_API_CALL(Action, Recover) { \
		IMM_API_CALL(Action); \
		if (SA_AIS_ERR_BAD_HANDLE == rc) { \
			if (Recover) { \
				IMM_API_CALL(Action); \
			} \
		} \
}

ImmUtil::CcbModifiedAttribute::CcbModifiedAttribute()
{

}

ImmUtil::CcbModifiedAttribute::CcbModifiedAttribute(const std::string& attribute, const std::string& value)
{
	std::vector<std::string> valArray;
	valArray.push_back(value);
	attrValMap[attribute] = valArray;
}

ImmUtil::CcbModifiedAttribute::~CcbModifiedAttribute()
{
	attrValMap.clear();
}

void ImmUtil::CcbModifiedAttribute::addTo(const std::string& attribute, const std::string& value)
{
	AttrValMapIterT it = attrValMap.find(attribute);
	if (it != attrValMap.end()) {

		it->second.push_back(value);
	}
	else {

		std::vector<std::string> valArray;
		valArray.push_back(value);
		attrValMap[attribute] = valArray;
	}
}

ImmUtil::ImmUtil() : _immOiApplierHandle(0),
                     _selectionObject(0),
                     _dispatchThread(0),
                     _exitDispatchThread(true),
                     _implementerName(COM_REENCRYPTOR_APPLIER),
                     _classImplementerName(SEC_ENCRYPTION_PARTICIPANTM),
                     _dispatchFlags(SA_DISPATCH_ONE),
                     _immOmHandle(0),
                     _immAccessorHandle(0),
                     _immSearchHandle(0),
                     _searchScope(SA_IMM_SUBLEVEL),
                     _searchOptions(SA_IMM_SEARCH_GET_NO_ATTR | SA_IMM_SEARCH_ONE_ATTR),
                     _adminOwnerHandle(0),
                     _adminOwnerScope(SA_IMM_ONE),
                     _immAdminOwnerName(COM_REENCRYPTOR),
                     _ccbHandle(0),
                     _ccbFlags(SA_IMM_CCB_REGISTERED_OI | SA_IMM_CCB_ALLOW_NULL_OI)

{
	_immVersion.releaseCode = IMM_RELEASE_CODE;
	_immVersion.majorVersion = IMM_MAJOR_VERSION;
	_immVersion.minorVersion = IMM_MINOR_VERSION;
}

ImmUtil::~ImmUtil()
{

}

bool ImmUtil::initializeImmOiApplierHandle(SaImmOiCallbacksT_2* pImmOiApplierCallbacks)
{
	bool retVal = true;
	SaAisErrorT rc = SA_AIS_OK;

	if (_immOiApplierHandle) {
		finalizeImmOiApplierHandle();
	}

	IMM_API_CALL(saImmOiInitialize_2(&_immOiApplierHandle, (const SaImmOiCallbacksT_2*)pImmOiApplierCallbacks, &_immVersion));

	if (SA_AIS_OK != rc) {
		retVal = false;
		WARN_COM_RP ("ImmUtil::initializeImmOiApplierHandle(): saImmOiInitialize_2 failed. rc: %d", int(rc));
	}

	DEBUG_COM_RP ("ImmUtil::initializeImmOiApplierHandle(): rc: %d", int(rc));
	return retVal;
}

bool ImmUtil::finalizeImmOiApplierHandle()
{
	bool retVal = true;
	SaAisErrorT rc = SA_AIS_OK;

	if (_immOiApplierHandle) {

		IMM_API_CALL(saImmOiFinalize(_immOiApplierHandle));

		if (SA_AIS_OK != rc) {
			retVal = false;
			WARN_COM_RP ("ImmUtil::finalizeImmOiApplierHandle(): saImmOiFinalize failed. rc: %d", int(rc));
		}

		DEBUG_COM_RP ("ImmUtil::finalizeImmOiApplierHandle(): rc: %d", int(rc));
		_immOiApplierHandle = 0;
	}

	return retVal;
}

bool ImmUtil::setImplementerName()
{
	bool retVal = true;
	SaAisErrorT rc = SA_AIS_OK;

	IMM_API_CALL(saImmOiImplementerSet(_immOiApplierHandle, const_cast<SaImmOiImplementerNameT>(_implementerName.c_str())));

	if (SA_AIS_OK != rc) {
		retVal = false;
		WARN_COM_RP ("ImmUtil::setImplementerName(): saImmOiImplementerSet failed. rc: %d", int(rc));
	}

	DEBUG_COM_RP ("ImmUtil::setImplementerName(): rc: %d", int(rc));
	return retVal;
}

bool ImmUtil::clearImplementerName()
{
	bool retVal = true;
	SaAisErrorT rc = SA_AIS_OK;

	IMM_API_CALL(saImmOiImplementerClear(_immOiApplierHandle));

	if (SA_AIS_OK != rc) {
		retVal = false;
		WARN_COM_RP ("ImmUtil::clearImplementerName(): saImmOiImplementerClear failed. rc: %d", int(rc));
	}
	DEBUG_COM_RP ("ImmUtil::clearImplementerName(): rc: %d", int(rc));
	return retVal;
}

bool ImmUtil::setClassImplementer()
{
	bool retVal = true;
	SaAisErrorT rc = SA_AIS_OK;

	IMM_API_CALL(saImmOiClassImplementerSet(_immOiApplierHandle, const_cast<char*>(_classImplementerName.c_str())));

	if (SA_AIS_OK != rc) {
		retVal = false;
		WARN_COM_RP ("ImmUtil::setClassImplementer(): saImmOiClassImplementerSet failed. rc: %d", int(rc));
	}

	DEBUG_COM_RP ("ImmUtil::setClassImplementer(): rc: %d", int(rc));
	return retVal;

}

bool ImmUtil::releaseClassImplementer()
{
	bool retVal = true;
	SaAisErrorT rc = SA_AIS_OK;

	IMM_API_CALL(saImmOiClassImplementerRelease(_immOiApplierHandle, const_cast<char*>(_classImplementerName.c_str())));

	if (SA_AIS_OK != rc) {
		retVal = false;
		WARN_COM_RP ("ImmUtil::releaseClassImplementer(): saImmOiClassImplementerRelease failed. rc: %d", int(rc));
	}

	DEBUG_COM_RP ("ImmUtil::releaseClassImplementer(): rc: %d", int(rc));

	return retVal;
}


bool ImmUtil::initializeImmOmHandle()
{
	bool retVal = true;
	SaAisErrorT rc = SA_AIS_OK;

	(void)finalizeImmOmHandle();

	IMM_API_CALL(saImmOmInitialize(&_immOmHandle, NULL, &_immVersion));

	if (SA_AIS_OK != rc) {
		retVal = false;
		WARN_COM_RP ("ImmUtil::initializeImmOmHandle(): saImmOmInitialize failed. rc: %d", int(rc));
	}

	DEBUG_COM_RP ("ImmUtil::initializeImmOmHandle(): rc: %d", int(rc));

	return retVal;
}

bool ImmUtil::finalizeImmOmHandle()
{
	SaAisErrorT rc = SA_AIS_OK;

	if (_immOmHandle) {
		IMM_API_CALL(saImmOmFinalize(_immOmHandle));

		DEBUG_COM_RP ("ImmUtil::finalizeImmOmHandle(): rc: %d", int(rc));
		_immOmHandle = 0;
	}
	return (SA_AIS_OK == rc);
}

bool ImmUtil::initializeImmOmAccessor()
{
	bool retVal = true;
	SaAisErrorT rc = SA_AIS_OK;

	(void)finalizeImmOmAccessor();

	HANDLE_IMM_API_CALL(saImmOmAccessorInitialize(_immOmHandle, &_immAccessorHandle),
	                    initializeImmOmHandle());

	if (SA_AIS_OK != rc) {
		retVal = false;
		WARN_COM_RP ("ImmUtil::initializeImmOmAccessor(): saImmOmAccessorInitialize failed. rc: %d", int(rc));
	}

	DEBUG_COM_RP ("ImmUtil::initializeImmOmAccessor(): rc: %d", int(rc));

	return retVal;
}

bool ImmUtil::finalizeImmOmAccessor()
{
	SaAisErrorT rc = SA_AIS_OK;

	if (_immAccessorHandle) {
		IMM_API_CALL(saImmOmAccessorFinalize(_immAccessorHandle));

		DEBUG_COM_RP ("ImmUtil::finalizeImmOmAccessor(): rc: %d", int(rc));
		_immAccessorHandle = 0;
	}
	return (SA_AIS_OK == rc);
}

bool ImmUtil::initializeImmOmSearch(const std::string& parent, const std::string& className)
{
	(void)finalizeImmOmSearch();

	DEBUG_COM_RP ("ImmUtil::initializeImmOmSearch(): parent[%s], className[%s]", parent.c_str(), className.c_str());

	SaNameT* immParent = NULL;
	if (false == parent.empty()) {
		immParent = makeSaNameT(parent);
		if (NULL == immParent) {
			DEBUG_COM_RP ("ImmUtil::initializeImmOmSearch(): unable to makeSaNameT()");
			return false;
		}
	}

	char* tempClassName = const_cast<char*>(className.c_str());
	SaImmSearchParametersT_2 searchParam;
	searchParam.searchOneAttr.attrName      = (char*)SA_IMM_ATTR_CLASS_NAME;
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue     = static_cast<void*>(&tempClassName);

	SaAisErrorT rc = SA_AIS_OK;
	bool retVal = true;

	HANDLE_IMM_API_CALL(saImmOmSearchInitialize_2(_immOmHandle,
	                                              (const SaNameT*)immParent,
	                                              _searchScope,
	                                              _searchOptions,
	                                              (const SaImmSearchParametersT_2*)&searchParam,
	                                              NULL,
	                                              &_immSearchHandle),
	                    initializeImmOmHandle());

	if (SA_AIS_OK != rc) {
		WARN_COM_RP("ImmUtil::initializeImmOmSearch(): saImmOmSearchInitialize_2 failed rc = %d", int(rc));
		retVal = false;
	}

	if (immParent) {
		delete immParent;
		immParent = NULL;
	}

	DEBUG_COM_RP("ImmUtil::initializeImmOmSearch(): saImmOmSearchInitialize_2 rc %d", int(rc));
	return retVal;
}

void ImmUtil::finalizeImmOmSearch()
{
	DEBUG_COM_RP("ImmUtil::finalizeImmOmSearch(): ENTER");
	if (_immSearchHandle) {
		SaAisErrorT rc;
		IMM_API_CALL(saImmOmSearchFinalize(_immSearchHandle));
		_immSearchHandle = 0;
	}
}

bool ImmUtil::initializeAdminOwner()
{
	DEBUG_COM_RP("ImmUtil::initializeAdminOwner(): ENTER");

	bool retVal = true;

	(void)finalizeAdminOwner();

	SaAisErrorT rc;
	HANDLE_IMM_API_CALL(saImmOmAdminOwnerInitialize(_immOmHandle,
	                                                (const SaImmAdminOwnerNameT)_immAdminOwnerName.c_str(),
	                                                SA_TRUE,
	                                                &_adminOwnerHandle),
	                    initializeImmOmHandle());

	if (SA_AIS_OK != rc) {
		WARN_COM_RP("ImmUtil::initializeAdminOwner(): saImmOmAdminOwnerInitialize failed rc = %d", rc);
		retVal = false;
	}
	return retVal;
}

bool ImmUtil::finalizeAdminOwner()
{
	bool retVal = true;

	if (_adminOwnerHandle) {
		SaAisErrorT rc;
		IMM_API_CALL(saImmOmAdminOwnerFinalize(_adminOwnerHandle));

		if (SA_AIS_OK != rc) {
			WARN_COM_RP("ImmUtil::finalizeAdminOwner(): saImmOmAdminOwnerFinalize failed rc = %d", rc);
			retVal = false;
		}
		_adminOwnerHandle = 0;
	}

	return retVal;
}

bool ImmUtil::setAdminOwner(std::string immObject)
{
	bool retVal = true;

	if (immObject.empty()) {
		DEBUG_COM_RP("ImmUtil::setAdminOwner(): immObject empty!");
		return retVal;
	}

	SaNameT* objDns[2];
	objDns[0] = makeSaNameT(immObject);
	objDns[1] = NULL;

	SaAisErrorT rc = SA_AIS_OK;

	if (objDns[0]) {

		IMM_API_CALL(saImmOmAdminOwnerSet(_adminOwnerHandle, (const SaNameT**)objDns, _adminOwnerScope));

		delete objDns[0];
		objDns[0] = NULL;

		if (SA_AIS_OK != rc) {
			WARN_COM_RP("ImmUtil::setAdminOwner(): saImmOmAdminOwnerSet failed rc = %d", rc);
			retVal = false;
		}
		else {
			_lockedObjectsList.push_back(immObject);
		}
	}
	else {
		DEBUG_COM_RP("ImmUtil::setAdminOwner(): objDns[0] empty. unable to convert [%s] to SaNameT", immObject.c_str());
		retVal = false;
	}
	return retVal;
}

bool ImmUtil::releaseAdminOwner()
{
	bool retVal = true;

	if (_adminOwnerHandle && (false == _lockedObjectsList.empty())) {

		_lockedObjectsList.unique();

		int size = _lockedObjectsList.size();
		SaNameT* objDns[size+1];
		std::list<std::string>::iterator itList = _lockedObjectsList.begin();

		for (int i = 0; itList != _lockedObjectsList.end(); i++, itList++) {

			DEBUG_COM_RP("ImmUtil::releaseAdminOwner(): dn [%s]", itList->c_str());
			objDns[i] = makeSaNameT(*itList);
			if (NULL == objDns[i]) {
				WARN_COM_RP("ImmUtil::releaseAdminOwner(): unable to convert [%s] to SaNameT format", itList->c_str());
				retVal = false;
				break;
			}
		}
		objDns[size] = NULL;

		if (retVal) {

			SaAisErrorT rc = SA_AIS_OK;

			IMM_API_CALL(saImmOmAdminOwnerRelease(_adminOwnerHandle, (const SaNameT**)objDns, _adminOwnerScope));

			if (SA_AIS_OK != rc) {
				WARN_COM_RP("ImmUtil::releaseAdminOwner(): saImmOmAdminOwnerRelease failed rc = %d", rc);
				retVal = false;
			}
		}

		for (int i = 0; i < size; i++) {

			if (objDns[i]) {

				delete objDns[i];
				objDns[i] = NULL;
			}
		}
		_lockedObjectsList.clear();
	}
	return retVal;
}

bool ImmUtil::initializeCcb()
{
	bool retVal = true;

	if (_ccbHandle) {
		SaAisErrorT rc;
		IMM_API_CALL(saImmOmCcbFinalize(_ccbHandle));
	}

	if(_adminOwnerHandle) {

		SaAisErrorT rc;
		HANDLE_IMM_API_CALL(saImmOmCcbInitialize(_adminOwnerHandle, _ccbFlags, &_ccbHandle),
		                    initializeAdminOwner());

		if (SA_AIS_OK != rc) {
			WARN_COM_RP("ImmUtil::initializeCcb(): saImmOmCcbInitialize failed rc = %d", rc);
			retVal = false;
		}
	}

	return retVal;
}

void ImmUtil::closeTransaction(bool abortTransaction)
{
	if (false == releaseAdminOwner()) {
		WARN_COM_RP("ImmUtil::closeTransaction(): releaseAdminOwner failed");
	}

	if (_ccbHandle) {

		SaAisErrorT rc;
		if (abortTransaction) {

			IMM_API_CALL(saImmOmCcbAbort(_ccbHandle));
			if (SA_AIS_OK != rc) {
				WARN_COM_RP("ImmUtil::closeTransaction(): saImmOmCcbAbort failed");
			}
		}

		IMM_API_CALL(saImmOmCcbFinalize(_ccbHandle));
		if (SA_AIS_OK != rc) {
			DEBUG_COM_RP("ImmUtil::closeTransaction(): saImmOmCcbFinalize failed");
		}
		_ccbHandle = 0;
	}
	_ccbObjects.clear();

	if (SA_AIS_OK != finalizeAdminOwner()) {
		DEBUG_COM_RP("ImmUtil::closeTransaction(): finalizeAdminOwner failed");
	}
}

std::string ImmUtil::getErrorStrings()
{
	std::string returnString;
	const SaStringT* errorStrings;
	SaAisErrorT rc = SA_AIS_OK;
	IMM_API_CALL(saImmOmCcbGetErrorStrings(_ccbHandle, &errorStrings));

	if (SA_AIS_OK == rc) {

		for (int i = 0; errorStrings[i] != NULL; i++) {

			if (returnString.empty()) {
				returnString = std::string(errorStrings[i]);
			}
			WARN_COM_RP("ImmUtil::getErrorStrings(): ErrorStrings: %s", errorStrings[i]);
		}
	}
	return returnString;
}

bool ImmUtil::dispatch()
{
	bool retVal = true;
	SaAisErrorT rc;
	struct pollfd fds[1];
	fds[0].fd = _selectionObject;
	fds[0].events = POLLIN;

	/*
	 * This is the dispatcher loop, it should exit only when the implementor is stopped
	 */
	while (!_exitDispatchThread)
	{
		int res = poll(fds, 1, 20); /* return 0 if timed out */

		if (res == -1)
		{
			if (errno == EINTR)
				continue;
			else
			{
				ERR_COM_RP("DEBUG_COM_RP(): poll FAILED - %s", strerror(errno));
				_exitDispatchThread = true;
				retVal = false;
				break;
			}
		}

		if (fds[0].revents & POLLIN)
		{
			/* There is an Implementor callback waiting to be be processed. Process it */
			DEBUG_COM_RP("ImmUtil::dispatch : Dispatching from IMM");
			IMM_API_CALL(saImmOiDispatch(_immOiApplierHandle, _dispatchFlags));
			if (SA_AIS_OK != rc)
			{
				_exitDispatchThread = true;
				retVal = false;
				WARN_COM_RP("ImmUtil::dispatch : saImmOiDispatch FAILED %u", rc);
				break;
			}
		}
	}

	_exitDispatchThread = true;
	DEBUG_COM_RP("ImmUtil::dispatch : Stopping dispatching\n");

	return retVal;
}

bool ImmUtil::stop()
{
	bool retVal = true;
	if (_immOiApplierHandle) {
		retVal &= releaseClassImplementer();
		retVal &= clearImplementerName();
	}
	retVal &= finalizeImmOiApplierHandle();

	if (_immOmHandle) {
		(void)finalizeImmOmSearch();
		retVal &= finalizeImmOmAccessor();
	}
	closeTransaction(true);
	retVal &= finalizeAdminOwner();
	retVal &= finalizeImmOmHandle();

	return retVal;
}

/* registerApplierOI
 *
 * Registers as Applier OI for the class 'SecMEncryptionParticipantM'
 * with the callback functions provided by 'pImmOiApplierCallbacks'.
 */
bool ImmUtil::registerApplierOI(SaImmOiCallbacksT_2* pImmOiApplierCallbacks)
{
	bool retVal = true;

	if (_immOiApplierHandle) {
		(void)finalizeImmOiApplierHandle();
	}

	retVal = initializeImmOiApplierHandle(pImmOiApplierCallbacks);

	if (retVal) {

		SaAisErrorT rc = SA_AIS_OK;

		IMM_API_CALL(saImmOiSelectionObjectGet(_immOiApplierHandle, &_selectionObject));

		if (SA_AIS_OK != rc) {
			retVal = false;
			WARN_COM_RP ("ImmUtil::registerApplierOI(): saImmOiSelectionObjectGet failed. rc: %d", int(rc));
		}

		DEBUG_COM_RP ("ImmUtil::registerApplierOI : rc: %d", int(rc));
	}

	if (retVal) {
		retVal = setImplementerName();
	}

	if (retVal) {
		retVal = setClassImplementer();
	}

	if (retVal) {
		_exitDispatchThread = false;
		int createThread = pthread_create(&_dispatchThread, NULL, startDispatchThread, NULL);
		if (createThread) {
			_exitDispatchThread = true;
			retVal = false;
			ERR_COM_RP ("ImmUtil::registerApplierOI : unable to start dispatch thread: %d", createThread);
		}
	}

	return retVal;
}

bool ImmUtil::unregisterApplierOI()
{
	bool retVal = true;

	if (false == _exitDispatchThread) {

		_exitDispatchThread = true;
		int joinThread = pthread_join(_dispatchThread, NULL);
		if(joinThread) {
			DEBUG_COM_RP ("ImmUtil::unregisterApplierOI : failed to join dispatch thread: %d", joinThread);
		}
	}

	if (_immOiApplierHandle) {
		retVal &= releaseClassImplementer();
		retVal &= clearImplementerName();
		retVal &= finalizeImmOiApplierHandle();
	}

	return retVal;
}

SaAisErrorT ImmUtil::getAttributeValue(const std::string& immDn, const std::string& attribute, SaImmAttrValuesT_2*** attrValues)
{
	if (immDn.empty() || attribute.empty()) {
		DEBUG_COM_RP ("ImmUtil::getAttributeValue(): invalid Input");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	SaNameT* immObject = makeSaNameT(immDn);
	if (NULL == immObject) {
		DEBUG_COM_RP ("ImmUtil::getAttributeValue(): unable to makeSaNameT()");
		return SA_AIS_ERR_FAILED_OPERATION;
	}

	SaImmAttrNameT attrNames[] = {NULL, NULL};
	attrNames[0] = const_cast<char*>(attribute.c_str());

	SaAisErrorT rc = SA_AIS_OK;

	HANDLE_IMM_API_CALL(saImmOmAccessorGet_2(_immAccessorHandle, (const SaNameT*)immObject, (const SaImmAttrNameT*)attrNames, attrValues),
	                    initializeImmOmAccessor());

	delete immObject;
	immObject = NULL;

	if ((SA_AIS_OK != rc) && (SA_AIS_ERR_NOT_EXIST != rc)) {
		WARN_COM_RP ("ImmUtil::getAttributeValue(): saImmOmAccessorGet_2 failed: object: %s, rc = %d", immDn.c_str(), rc);
	}

	DEBUG_COM_RP ("ImmUtil::getAttributeValue(): saImmOmAccessorGet_2 : object: %s, rc = %d", immDn.c_str(), rc);
	return rc;
}

bool ImmUtil::findImmObjects(const std::string& parent, const std::string& className, std::list<std::string>& objects)
{
	if (className.empty()) {
		DEBUG_COM_RP("findImmObjects: clasName can't be empty ");
		return false;
	}

	bool retVal = true;

	if (false == (retVal = initializeImmOmSearch(parent, className))) {
		DEBUG_COM_RP("findImmObjects: Failed to initialize search handle");
		return retVal;
	}

	SaAisErrorT rc = SA_AIS_OK;
	SaNameT immObject;
	SaImmAttrValuesT_2** attrValues = NULL;
	while(true) {

		IMM_API_CALL(saImmOmSearchNext_2(_immSearchHandle, &immObject, (SaImmAttrValuesT_2 ***)&attrValues));
		if (SA_AIS_OK != rc) {

			if (SA_AIS_ERR_NOT_EXIST != rc) {
				retVal = false;
				DEBUG_COM_RP("findImmObjects: saImmOmSearchNext_2 Failed object: %s, rc: %d", parent.c_str(), rc);

			}
			break;
		}

		std::string objectName = makeString(&immObject);
		objects.push_back(objectName);
	}

	(void)finalizeImmOmSearch();

	return retVal;
}

bool ImmUtil::createObjectSecEncryptionParticipant()
{
	bool retVal = true;

	DEBUG_COM_RP("ImmUtil::createObjectSecEncryptionParticipant(): ENTER");

	if (false == (retVal = initializeAdminOwner())) {
		WARN_COM_RP("ImmUtil::createObjectSecEncryptionParticipant(): Unable to initialize Admin Owner.");
		return retVal;
	}

	if (false == (retVal = initializeCcb())) {
		WARN_COM_RP("ImmUtil::createObjectSecEncryptionParticipant(): Unable to start CCB");
		finalizeAdminOwner();
		return retVal;
	}

	SaAisErrorT rc;

	if(false == (retVal = setAdminOwner(std::string(SEC_ENCRYPTION_PARTICIPANTM_OBJECT)))) {
		WARN_COM_RP("ImmUtil::applyTransaction(): Unable to set Admin owner for %s", SEC_ENCRYPTION_PARTICIPANTM_OBJECT);
		IMM_API_CALL(saImmOmCcbFinalize(_ccbHandle));
		_ccbHandle = 0;
		finalizeAdminOwner();
		return retVal;
	}

	SaNameT* parentName = makeSaNameT(std::string(SEC_ENCRYPTION_PARTICIPANTM_OBJECT));
	if (NULL == parentName) {
		WARN_COM_RP("ImmUtil::createObjectSecEncryptionParticipant(): Unable to convert %s to SaNameT", SEC_ENCRYPTION_PARTICIPANTM_OBJECT);
		retVal = false;
	}
	else {
		SaImmClassNameT className = strdup(SEC_ENCRYPTION_PARTICIPANT);

		SaImmAttrNameT rdnAttrName = strdup(SEC_ENCRYPTION_PARTICIPANT_RDN_NAME);
		char* rdnVal = strdup(SEC_ENCRYPTION_PARTICIPANT_RDN_VALUE);
		void *arr1[] = { &rdnVal };
		const SaImmAttrValuesT_2 attrRdn =
		{
				rdnAttrName,
				SA_IMM_ATTR_SASTRINGT,
				1,
				arr1
		};

		SaImmAttrNameT attr2Name = strdup(ENCRYPTION_STATUS);
		int encryptStatus = 0;
		SaImmAttrValueT arr2 = &encryptStatus;
		SaImmAttrValuesT_2 attr2 =
		{
				attr2Name,
				SA_IMM_ATTR_SAINT32T,
				1,
				(SaImmAttrValueT*) &arr2
		};

		SaImmAttrNameT attr3Name = strdup(ADDITIONAL_STATUS_INFO);
		char* statusInfo = const_cast<char*>(EmptyString.c_str());
		void *arr3[] = { &statusInfo };
		const SaImmAttrValuesT_2 attr3 =
		{
				attr3Name,
				SA_IMM_ATTR_SASTRINGT,
				1,
				arr3
		};

		const SaImmAttrValuesT_2* attrVal[] =
		{
				&attrRdn,
				&attr2,
				&attr3,
				NULL
		};

		IMM_API_CALL (saImmOmCcbObjectCreate_2(_ccbHandle, className, parentName, (const SaImmAttrValuesT_2**)attrVal));

		free(attr3Name);
		attr3Name = NULL;

		free (attr2Name);
		attr2Name = NULL;

		free(rdnVal);
		rdnVal = NULL;

		free (rdnAttrName);
		rdnAttrName = NULL;

		free (className);
		className = NULL;

		delete parentName;
		parentName = NULL;

		if ((SA_AIS_OK != rc) && (SA_AIS_ERR_EXIST != rc)) {
			WARN_COM_RP("ImmUtil::createObjectSecEncryptionParticipant(): Cannot create %s. rc: %d", SEC_ENCRYPTION_PARTICIPANT_OBJECT, int(rc));
			retVal = false;
		}
	}

	if (retVal) {
		IMM_API_CALL(saImmOmCcbValidate(_ccbHandle));
		if (SA_AIS_OK != rc) {
			WARN_COM_RP("ImmUtil::createObjectSecEncryptionParticipant(): saImmOmCcbValidate failed rc: %d", int(rc));
			retVal = false;
		}
	}

	if (retVal) {
		IMM_API_CALL(saImmOmCcbApply(_ccbHandle));
		if (SA_AIS_OK != rc) {
			WARN_COM_RP("ImmUtil::createObjectSecEncryptionParticipant(): saImmOmCcbApply failed rc: %d", int(rc));
			retVal = false;
		}
	}
	else {
		IMM_API_CALL(saImmOmCcbAbort(_ccbHandle));
		if (SA_AIS_OK != rc) {
			WARN_COM_RP("ImmUtil::createObjectSecEncryptionParticipant(): saImmOmCcbAbort failed");
		}
	}

	IMM_API_CALL(saImmOmCcbFinalize(_ccbHandle));
	if (SA_AIS_OK != rc) {
		DEBUG_COM_RP("ImmUtil::createObjectSecEncryptionParticipant(): saImmOmCcbFinalize failed");
	}
	_ccbHandle = 0;

	if (false == finalizeAdminOwner()) { //automatically releases locks on objects.
		WARN_COM_RP("ImmUtil::createObjectSecEncryptionParticipant(): finalizeAdminOwner failed");
		retVal = false;
	}
	_lockedObjectsList.clear();

	return retVal;
}

void ImmUtil::addModifyObject(const std::string& object, const std::string& attribute, const std::string& value)
{
	DEBUG_COM_RP ("ImmUtil::addModifyObject(): ENTER object[%s], attr[%s], value [%s]", object.c_str(), attribute.c_str(), value.c_str());

	CcbModifiedAttributesMapIterT it = _ccbObjects.find(object);
	if (it != _ccbObjects.end()) {

		DEBUG_COM_RP ("ImmUtil::addModifyObject(): Found object[%s]", object.c_str());
		it->second.addTo(attribute, value);
	}
	else {

		DEBUG_COM_RP ("ImmUtil::addModifyObject(): Adding new object[%s]", object.c_str());
		_ccbObjects[object] = CcbModifiedAttributeT(attribute, value);
	}
}

bool ImmUtil::applyTransaction(std::string& oiErrorStrings)
{
	bool retVal = true;
	DEBUG_COM_RP("ImmUtil::applyTransaction(): ENTER");

	if (false == (retVal = initializeAdminOwner())) {
		WARN_COM_RP("ImmUtil::applyTransaction(): Unable to initialize Admin Owner.");
		closeTransaction();
		return retVal;
	}

	if (false == (retVal = initializeCcb())) {
		WARN_COM_RP("ImmUtil::applyTransaction(): Unable to start CCB");
		closeTransaction();
		return retVal;
	}

	DEBUG_COM_RP("ImmUtil::applyTransaction(): Objects to be modified: %d", int(_ccbObjects.size()));

	SaAisErrorT rc = SA_AIS_OK;
	CcbModifiedAttributesMapIterT it = _ccbObjects.begin();
	while (it != _ccbObjects.end()) {

		if(false == (retVal = setAdminOwner(std::string(it->first)))) {
			WARN_COM_RP("ImmUtil::applyTransaction(): Unable to set Admin owner for %s", it->first.c_str());
			break;
		}

		DEBUG_COM_RP("ImmUtil::applyTransaction(): Object[%s]", it->first.c_str());
		SaNameT* immObject = makeSaNameT(it->first);
		if (immObject) {
			CcbModifiedAttributeT modObj = it->second;
			int modObjCount = modObj.attrValMap.size();
			SaImmAttrModificationT_2** modObjects = new SaImmAttrModificationT_2*[1 + modObjCount];

			AttrValMapIterT itAttr = modObj.attrValMap.begin();
			for (int i = 0; itAttr != modObj.attrValMap.end(); i++, itAttr++) {

				DEBUG_COM_RP("ImmUtil::applyTransaction(): Attr[%s]", itAttr->first.c_str());
				modObjects[i] = new SaImmAttrModificationT_2;
				modObjects[i]->modType = SA_IMM_ATTR_VALUES_REPLACE;
				modObjects[i]->modAttr.attrName = strdup(itAttr->first.c_str());

				bool isAttrTypeInt = ( (0 == strcmp(SEC_ENCRYPTION_PARTICIPANT_OBJECT, it->first.c_str()))
				                    && (0 == strcmp(ENCRYPTION_STATUS, itAttr->first.c_str())) );

				unsigned int valuesNumber = itAttr->second.size();
				void** vp = new void*[valuesNumber + 1];

				std::vector<std::string>::iterator itVal = itAttr->second.begin();
				for (int j = 0; itVal != itAttr->second.end(); j++, itVal++) {

					if (isAttrTypeInt) {
						std::stringstream ostr(itVal->c_str());
						int32_t encryptStatus = 0;
						ostr >> encryptStatus;
						vp[j] = new int32_t;
						 *(int32_t*) vp[j]   = encryptStatus;
					}
					else {
						vp[j] = new SaStringT;
						*(char**) vp[j] = strdup(itVal->c_str());
					}

					DEBUG_COM_RP("ImmUtil::applyTransaction(): Value[%s]", itVal->c_str());
				}
				vp [valuesNumber] = NULL;

				if (isAttrTypeInt) {
					modObjects[i]->modAttr.attrValueType = SA_IMM_ATTR_SAINT32T;
				}
				else {
					modObjects[i]->modAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
				}

				modObjects[i]->modAttr.attrValuesNumber = valuesNumber;
				modObjects[i]->modAttr.attrValues = vp;

			}
			modObjects[modObjCount] = NULL;

			IMM_API_CALL (saImmOmCcbObjectModify_2(_ccbHandle, immObject, (const SaImmAttrModificationT_2**)modObjects));

			for (int i = 0; i < modObjCount; i++) {

				int valuesNumber = modObjects[i]->modAttr.attrValuesNumber;
				for (int j = 0; j < valuesNumber; j++) {

					if (SA_IMM_ATTR_SASTRINGT == modObjects[i]->modAttr.attrValueType) {
						free(*((char **)(modObjects[i]->modAttr.attrValues[j])));
						if(modObjects[i]->modAttr.attrValues[j]!= NULL){
							delete modObjects[i]->modAttr.attrValues[j];
						}
					}
					else if (SA_IMM_ATTR_SAINT32T == modObjects[i]->modAttr.attrValueType) {
						if(modObjects[i]->modAttr.attrValues[j]!= NULL) {
							delete modObjects[i]->modAttr.attrValues[j];
						}
					}
				}

				delete[] modObjects[i]->modAttr.attrValues;
				modObjects[i]->modAttr.attrValues = NULL;

				free (modObjects[i]->modAttr.attrName);
				modObjects[i]->modAttr.attrName = NULL;

				delete modObjects[i];
				modObjects[i] = NULL;
			}

			delete[] modObjects;
			modObjects = NULL;

			delete immObject;
			immObject = NULL;

			if (SA_AIS_OK != rc) {
				WARN_COM_RP("ImmUtil::applyTransaction(): saImmOmCcbObjectModify_2 failed rc: %d", int(rc));
				if (SA_AIS_ERR_FAILED_OPERATION == rc) {
					oiErrorStrings = getErrorStrings();
				}
				retVal = false;
				break;
			}
		}
		else {
			WARN_COM_RP("ImmUtil::applyTransaction(): Unable to convert [%s] to SaNameT", it->first.c_str());
			retVal = false;
			break;
		}
		it++;
	}

	if (retVal) {
		IMM_API_CALL(saImmOmCcbValidate(_ccbHandle));
		if (SA_AIS_OK != rc) {
			WARN_COM_RP("ImmUtil::applyTransaction(): saImmOmCcbValidate failed rc: %d", int(rc));
			if (SA_AIS_ERR_FAILED_OPERATION == rc) {
				oiErrorStrings = getErrorStrings();
			}
			retVal = false;
		}
	}

	if (retVal) {
		IMM_API_CALL(saImmOmCcbApply(_ccbHandle));
		if (SA_AIS_OK != rc) {
			WARN_COM_RP("ImmUtil::applyTransaction(): saImmOmCcbApply failed rc: %d", int(rc));
			if (SA_AIS_ERR_FAILED_OPERATION == rc) {
				oiErrorStrings = getErrorStrings();
			}
			retVal = false;
		}
	}

	closeTransaction();
	return retVal;
}

void ImmUtil::abortTransaction()
{
	closeTransaction(true);
}

void ImmUtil::clearCcbObjects()
{
	_ccbObjects.clear();
}
/**
 * Convert a char* to an SaNameT
 * Note: the SaNameT* is allocated by new and must be deleted
 *
 * @param[in]	cstr - the name string
 * @return 		pointer to the name type
 */
SaNameT* ImmUtil::makeSaNameT(const std::string& name)
{
	unsigned short len = static_cast<unsigned short>(name.length());
	if (SA_MAX_NAME_LENGTH >= len) {
		SaNameT* saName = new SaNameT;

		TheSaNameT* tempSaName = reinterpret_cast<TheSaNameT*>(saName);
		tempSaName->length = len;
		memcpy(tempSaName->value, name.c_str(), static_cast<unsigned long int>(tempSaName->length));

		return saName;
	}

	return NULL;
}

std::string ImmUtil::makeString(const SaNameT* immObject)
{
	std::string retVal;
	if (NULL == immObject) {
		DEBUG_COM_RP("ImmUtil::makeString(): immObject is NULL!!");
		return retVal;
	}

	TheSaNameT* tempSaName = reinterpret_cast<TheSaNameT*>(const_cast<SaNameT*>(immObject));
	unsigned short len = tempSaName->length;
	if (SA_MAX_NAME_LENGTH >= len) {
		retVal = std::string((char*)tempSaName->value, len);
	}

	return retVal;
}

void* startDispatchThread(void* arg)
{
	DEBUG_COM_RP("startDispatchThread: Enter dispatch thread ");
	if (false == ImmUtil::Instance().dispatch()) {
		WARN_COM_RP(" startDispatchThread: Unable to start dispatch thread.");
	}
	pthread_exit(NULL);
	return NULL;
}
