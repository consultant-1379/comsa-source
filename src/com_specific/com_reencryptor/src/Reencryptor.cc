#include "Reencryptor.h"
#include "Trace.h"
#include "FileReaderUtil.h"
#include "SecUtil.h"

#include <cstring>
#include <sstream>

//IMM OI Callbacks
static void handleOiCcbAbortCallback (SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId);
static void handleOiCcbApplyCallback (SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId);
static SaAisErrorT handleOiCcbCompletedCallback (SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId);
static SaAisErrorT handleOiCcbObjectCreateCallback (SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId, const SaImmClassNameT className, const SaNameT *parentName, const SaImmAttrValuesT_2 **attr);
static SaAisErrorT handleOiCcbObjectDeleteCallback (SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId, const SaNameT *objectName);
static SaAisErrorT handleOiCcbObjectModifyCallback (SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId, const SaNameT *objectName, const SaImmAttrModificationT_2 **attrMods);

static SaImmOiCallbacksT_2 sImmOiApplierCallbacks = { NULL,
                                                      handleOiCcbAbortCallback,
                                                      handleOiCcbApplyCallback,
                                                      handleOiCcbCompletedCallback,
                                                      handleOiCcbObjectCreateCallback,
                                                      handleOiCcbObjectDeleteCallback,
                                                      handleOiCcbObjectModifyCallback,
                                                      NULL
                                                     };

enum InfoAttrT {
	Empty = 0,
	Success,
	Inprogress,
	ErrImmGet,
	ErrImmApply,
	ErrOI,
	ErrFileRead,
	ErrSecCrypto,
	ErrInternal
};

const std::string InfoAttrMsg[] = {
		std::string(),
		std::string("Successful Re-encryption"),
		std::string("Re-encryption is In-progress"),
		std::string("Unable to get data from CoreMW IMM"),
		std::string("Unable to apply new changes in CoreMW IMM."),
		std::string("Error from OI: "),
		std::string("Failed to fetch Schema data for the Secret attributes."),
		std::string("SEC Crypto API libraries not reacheable"),
		std::string("Internal Error Occurred.")
};

Reencryptor::Reencryptor() : _parentImmDn(SEC_ENCRYPTION_PARTICIPANTM_OBJECT),
                             _childImmDn(SEC_ENCRYPTION_PARTICIPANT_OBJECT),
                             _parentKeyUuid(""),
                             _childKeyUuid(""),
                             _encryptionStatus(SUCCESS),
                             _ccbId(0)
{

}

Reencryptor::~Reencryptor()
{
	_encryptionStatus = SUCCESS;
	_ccbId = 0;
}

SaAisErrorT Reencryptor::getSingleStringAttribute(const std::string& immObject, const std::string& attributeName, std::string& value)
{
	SaAisErrorT retVal = SaAisErrorT(0);

	SaImmAttrValuesT_2** attrValues = NULL;
	if(SA_AIS_OK != (retVal = ImmUtil::Instance().getAttributeValue(immObject, attributeName, &attrValues))) {
		DEBUG_COM_RP ("Reencryptor::updateParentKeyUuid(): Unable to fetch Attr from IMM. retVal = %d", int(retVal));
		return retVal;
	}

	if (attrValues && *attrValues) {

		for (int i = 0; NULL != attrValues[i]; i++) {

			if(0 == strncmp(attributeName.c_str(), (const char*)attrValues[i]->attrName, strlen(attributeName.c_str()))) {

				if (SA_IMM_ATTR_SASTRINGT == attrValues[i]->attrValueType) {

					if (1 == attrValues[i]->attrValuesNumber) {

						value = std::string(*(char**)(attrValues[i]->attrValues[0]));
						retVal = SA_AIS_OK;
					}
					else if (0 == attrValues[i]->attrValuesNumber) {

						value = std::string("");
						retVal = SA_AIS_OK;
					}
				}
				break;
			}
		}
	}
	return retVal;
}

bool Reencryptor::getEncryptionStatus()
{
	bool retVal = false;

	SaImmAttrValuesT_2** attrValues = NULL;
	if(SA_AIS_OK != ImmUtil::Instance().getAttributeValue(_childImmDn, std::string(ENCRYPTION_STATUS), &attrValues)) {
		DEBUG_COM_RP ("Reencryptor::updateParentKeyUuid(): Unable to fetch Attr from IMM.");
		return retVal;
	}

	if (attrValues && *attrValues) {

		for (int i = 0; NULL != attrValues[i]; i++) {

			if (0 == strncmp(ENCRYPTION_STATUS, (const char*)attrValues[i]->attrName, strlen(ENCRYPTION_STATUS))) {

				if (SA_IMM_ATTR_SAINT32T == attrValues[i]->attrValueType) {

					if (1 == attrValues[i]->attrValuesNumber) {

						if ((attrValues[i]->attrValues) && (attrValues[i]->attrValues[0])) {

							_encryptionStatus = EncryptionStatusT(*((SaUint32T *)((attrValues[i])->attrValues[0])));
							retVal = true;
						}
					}
					else if (0 == attrValues[i]->attrValuesNumber) {

						_encryptionStatus = EncryptionStatusT(0);
						retVal = true;
					}
				}
				break;
			}
		}
	}

	if(false == retVal) {
		DEBUG_COM_RP ("Reencryptor::updateParentKeyUuid(): Unable to parse encryptionStatus attribute vale received from IMM");
	}
	return retVal;
}

bool Reencryptor::isReencryptionNeeded()
{
	bool retVal = true;

	if (SA_AIS_OK != getSingleStringAttribute(_childImmDn, ENCRYPTION_KEY_UUID, _childKeyUuid)) {
		WARN_COM_RP("Reencryptor::isReencryptionNeeded(): unable to get encryption key uuid. Not Re-encrypting.");
		_encryptionStatus = FAILURE;
		_additionalStatusInfo = InfoAttrMsg[ErrImmGet];
		retVal = false;
	}
	else {
		_encryptionStatus = INPROGRESS;
		_additionalStatusInfo = InfoAttrMsg[Inprogress];

		if (_parentKeyUuid == _childKeyUuid) {

			if (getEncryptionStatus()) {

				if (SUCCESS == _encryptionStatus) {
					DEBUG_COM_RP("Reencryptor::isReencryptionNeeded(): No need to reencrypt.");
					retVal = false;
				}
			}
			else {
				WARN_COM_RP("Reencryptor::isReencryptionNeeded(): unable to get encryptionStatus. Not Re-encrypting.");
				_encryptionStatus = FAILURE;
				_additionalStatusInfo = InfoAttrMsg[ErrImmGet];
				retVal = false;
			}
		}
	}

	if (SUCCESS != _encryptionStatus) {
		std::ostringstream ostr;
		ostr<< _encryptionStatus;
		ImmUtil::Instance().addModifyObject(_childImmDn, ENCRYPTION_STATUS, ostr.str());
		ImmUtil::Instance().addModifyObject(_childImmDn, ENCRYPTION_KEY_UUID, _parentKeyUuid);
		ImmUtil::Instance().addModifyObject(_childImmDn, ADDITIONAL_STATUS_INFO, _additionalStatusInfo);
		if (false == ImmUtil::Instance().applyTransaction()) {
			WARN_COM_RP("Reencryptor::doReencryption(): Unable to set %s to %s", ENCRYPTION_KEY_UUID, _childImmDn.c_str());
			retVal = false;
		}
	}

	if (retVal) DEBUG_COM_RP("Reencryptor::isReencryptionNeeded(): Reencryption needed.");

	return retVal;
}

bool Reencryptor::getObjects(const std::queue<std::string>& mocList, std::list<std::string>& foundObjects)
{
	bool retVal = true;
	if(mocList.empty()) {
		WARN_COM_RP("Reencryptor::getObjects(): mocList is empty.");
		return retVal;
	}

	std::queue<std::string>tmpMocList = mocList;
	std::list<std::string> parentObjects;
	std::string className = std::string(tmpMocList.front());

	DEBUG_COM_RP("Reencryptor::getObjects(): className %s", className.c_str());

	std::list<std::string>::iterator it;
	if (true == (retVal = ImmUtil::Instance().findImmObjects("", className, parentObjects))) {

		tmpMocList.pop();
		while(!tmpMocList.empty() && !parentObjects.empty()) {

			DEBUG_COM_RP("Reencryptor::getObjects(): found objects");
			std::list<std::string> childObjects;
			className = std::string(tmpMocList.front());

			DEBUG_COM_RP("Reencryptor::getObjects(): next Class name %s", className.c_str());
			for(it = parentObjects.begin(); it != parentObjects.end(); it++) {

				std::list<std::string> tmpObjects;
				if (true == (retVal = ImmUtil::Instance().findImmObjects(*it, className, tmpObjects))) {
					childObjects.merge(tmpObjects);
				}
				else {
					WARN_COM_RP("Reencryptor::getObjects(): Unable to fetch objects from IMM.");
					tmpObjects.clear();
					childObjects.clear();
					parentObjects.clear();
					while (false == tmpMocList.empty()) {
						tmpMocList.pop();
					}

					return retVal;
				}
			}
			parentObjects = childObjects;
			tmpMocList.pop();
		}
		foundObjects = parentObjects;
		DEBUG_COM_RP("Reencryptor::getObjects(): Found %d objects in IMM with secrets data", static_cast<int>(foundObjects.size()));
		retVal = true;
	}
	else {
		WARN_COM_RP("Reencryptor::getObjects(): Unable to fetch objects from IMM.");
	}

	parentObjects.clear();
	while (false == tmpMocList.empty()) {
		tmpMocList.pop();
	}
	return retVal;
}

bool Reencryptor::makeSetAttributeData(std::list<std::string> objectsList, std::vector<std::string> attrList)
{
	bool retVal = true;
	if (objectsList.empty() || attrList.empty()) {
		return retVal;
	}

	std::list<std::string>::iterator itObj = objectsList.begin();
	while (retVal && (itObj != objectsList.end())) {

		std::vector<std::string>::iterator itAttr = attrList.begin();
		while (itAttr != attrList.end()) {

			std::string value;
			if (SA_AIS_OK != Reencryptor::Instance().getSingleStringAttribute(*itObj, *itAttr, value)) {

				WARN_COM_RP("Reencryptor::makeSetAttributeData: Unable to get value for [%s] in [%s]", itAttr->c_str(), itObj->c_str());
				_additionalStatusInfo = InfoAttrMsg[ErrImmGet];
				retVal = false;
				break;
			}

			if (value.empty()) {
				WARN_COM_RP("Reencryptor::makeSetAttributeData: Found an empty attribute value, skipping reencryption");
				itAttr++;
				continue;
			}

			std::string newValue;
			if (false == (retVal = SecUtil::Instance().reencrypt(value, newValue))) {

				WARN_COM_RP("Reencryptor::makeSetAttributeData: Unable to reencrypt value of [%s] in [%s]", itAttr->c_str(), itObj->c_str());
				_additionalStatusInfo = InfoAttrMsg[ErrSecCrypto];
				break;
			}
			DEBUG_COM_RP("Reencryptor::makeSetAttributeData: value for [%s] in [%s] is [%s]", itAttr->c_str(), itObj->c_str(), value.c_str());

			ImmUtil::Instance().addModifyObject(*itObj, *itAttr, newValue);
			DEBUG_COM_RP("Reencryptor::makeSetAttributeData: Added as modified object");

			itAttr++;
		}
		itObj++;
	}

	if (false == retVal) {
		ImmUtil::Instance().clearCcbObjects();
		WARN_COM_RP("Reencryptor::makeSetAttributeData(): Unable to prepare Attribute values.");
	}
	return retVal;
}

bool Reencryptor::populateData()
{
	bool retVal = true;
	DEBUG_COM_RP("Reencryptor::populateData(): Populating necessary objects...");

	mapImmPathToAttributesT mocAttrData;
	if (false == (retVal = FileReaderUtil::Instance().getSecretAttrData(mocAttrData))) {
		DEBUG_COM_RP("Reencryptor::populateData(): Unable to fetch schema data from file.");
		_additionalStatusInfo = InfoAttrMsg[ErrFileRead];
		return retVal;
	}
	DEBUG_COM_RP("Reencryptor::populateData(): Populated necessary objects.");

	if(mocAttrData.empty()) {
		DEBUG_COM_RP("Reencryptor::populateData(): ...But mocAttrData is empty.");
		_additionalStatusInfo = InfoAttrMsg[ErrFileRead];
		return false;
	}

	mapImmPathToAttributesIterT it = mocAttrData.begin();

	while (it != mocAttrData.end()) {

		DEBUG_COM_RP("Reencryptor::populateData(): Iterating through data...");
		std::queue<std::string> mocList = it->first;
		std::vector<std::string> attrList = it->second;

		std::list<std::string> objects;
		if (true == (retVal = getObjects(mocList, objects))) {

			if (false == objects.empty()) {

				if (false == (retVal = makeSetAttributeData(objects, attrList))) {
					mocAttrData.clear();
					break;
				}
			}
		}
		else {
			break;
		}

		it++;
	}

	if (retVal) {
		DEBUG_COM_RP("Reencryptor::populateData(): Populated necessary objects.");
	}
	else {
		ERR_COM_RP("Reencryptor::populateData(): Aborting reencryption due to  inconsistent data!");
	}

	return retVal;
}

bool Reencryptor::start(const char* pFlatFilePath)
{
	bool retVal = true;

	if (0 == strlen(pFlatFilePath)) {
		WARN_COM_RP("Reencryptor::start(): Invalid arguments for com-reencryptor-participant: flatfile path");
		return false;
	}
	DEBUG_COM_RP("Reencryptor::start(): flatfile %s", pFlatFilePath);

	FileReaderUtil::Instance().init(pFlatFilePath);

	if (false == (retVal = SecUtil::Instance().initSecCryptoLib())) {
		WARN_COM_RP("Reencryptor::start(): initSecCryptoLib failed.");
		return retVal;
	}

	SaAisErrorT rc;
	//Get the encryptionKeyUuid of the childImmDn from IMM
	if (SA_AIS_OK != (rc = getSingleStringAttribute(_childImmDn, ENCRYPTION_KEY_UUID, _childKeyUuid))) {

		if (SA_AIS_ERR_NOT_EXIST == rc) {

			if (false == ImmUtil::Instance().createObjectSecEncryptionParticipant()) {

				ERR_COM_RP("Reencryptor::start(): unable to create %s.", SEC_ENCRYPTION_PARTICIPANT_OBJECT);
				return false;
			}
		}
		else {

			WARN_COM_RP("Reencryptor::start(): unable to fetch %s from %s.", ENCRYPTION_KEY_UUID, _childImmDn.c_str());
		}
	}
	//Get the encryptionKeyUuid of parentImmDn from IMM
	if (SA_AIS_OK != getSingleStringAttribute(_parentImmDn, ENCRYPTION_KEY_UUID, _parentKeyUuid)) {
		WARN_COM_RP("Reencryptor::start(): unable to fetch %s from %s.", ENCRYPTION_KEY_UUID, _parentImmDn.c_str());
		return false;
	}

	doReencryption();

	if (false == (retVal = ImmUtil::Instance().registerApplierOI(&sImmOiApplierCallbacks))) {
		WARN_COM_RP("Reencryptor::start(): registerApplierOI failed.");
	}

	return retVal;
}

bool Reencryptor::stop()
{
	bool retVal = false;

	DEBUG_COM_RP("Reencryptor::stop(): unregister ApplierOI ");

	retVal = ImmUtil::Instance().unregisterApplierOI();
	retVal &= ImmUtil::Instance().stop();

	return retVal;
}

bool Reencryptor::updateParentEncryptionKeyUuid(const SaImmAttrModificationT_2 **attrMods)
{
	bool retVal = false;

	if (attrMods) {

		for (int i = 0; attrMods[i]; i++) {

			if(0 == strcmp(ENCRYPTION_KEY_UUID, (char*)attrMods[i]->modAttr.attrName)) {

				if (attrMods[i]->modAttr.attrValuesNumber) {

					SaImmAttrValuesT_2 modAttr = attrMods[i]->modAttr;
					SaImmAttrValueT attrValues = modAttr.attrValues[0];
					if (attrValues) {

						_parentKeyUuid = std::string(*(char**)(attrValues));
						retVal = true;
						DEBUG_COM_RP("Reencryptor::updateParentEncryptionKeyUuid: _parentKeyUuid [%s].", _parentKeyUuid.c_str());
					}
				}
				break;
			}
		}
	}

	if (false == retVal) {
		WARN_COM_RP("Reencryptor::updateParentEncryptionKeyUuid: unable to update _parentKeyUuid ");
	}

	return retVal;
}

void Reencryptor::setCcbId(const unsigned long long& ccbId)
{
	if (_ccbId) {
		ImmUtil::Instance().abortTransaction();
	}
	_ccbId = ccbId;
	DEBUG_COM_RP("Reencryptor::setCcbId(): set ccbId[%llu]", _ccbId);
}

void Reencryptor::removeCcbId(const unsigned long long& ccbId, bool abortTx)
{
	if (ccbId == _ccbId) {
		if (abortTx) {
			ImmUtil::Instance().abortTransaction();
		}
		_ccbId = 0;
		DEBUG_COM_RP("Reencryptor::removeCcbId()");
	}
}

void Reencryptor::doReencryption(const unsigned long long& ccbId)
{
	DEBUG_COM_RP("Reencryptor::doReencryption(): ENTER");

	if (ccbId != _ccbId) {
		DEBUG_COM_RP("Reencryptor::doReencryption(): CCB IDs don't match[%llu, %llu]. Not reencrypting.", ccbId, _ccbId);
		return;
	}
	DEBUG_COM_RP("Reencryptor::doReencryption(): ccbId Match!");

	if(false == isReencryptionNeeded()) {
		DEBUG_COM_RP("Reencryptor::doReencryption(): Reencryption is not needed.");
		return;
	}
	DEBUG_COM_RP("Reencryptor::doReencryption(): Reencryption is needed!");

	if (populateData()) {

		DEBUG_COM_RP("Reencryptor::doReencryption(): Required data successfully populated");

		std::string errorStrings;
		if (ImmUtil::Instance().applyTransaction(errorStrings)) {

			DEBUG_COM_RP("Reencryptor::doReencryption(): Secrets data successfully set to IMM.");
			_encryptionStatus = SUCCESS;
			_additionalStatusInfo = InfoAttrMsg[Success];
		}
		else {
			WARN_COM_RP("Reencryptor::doReencryption(): Unable to set secrets in IMM.");
			_encryptionStatus = FAILURE;
			if (errorStrings.empty()) {
				_additionalStatusInfo = InfoAttrMsg[ErrImmApply];
			}
			else {
				_additionalStatusInfo = InfoAttrMsg[ErrOI] + errorStrings;
			}
		}
	}
	else {
		WARN_COM_RP("Reencryptor::doReencryption(): Unable to populate data needed for re-encryption.");
		_encryptionStatus = FAILURE;
	}

	std::ostringstream ostr1;
	ostr1 << _encryptionStatus;
	ImmUtil::Instance().addModifyObject(_childImmDn, ENCRYPTION_STATUS, ostr1.str());
	ImmUtil::Instance().addModifyObject(_childImmDn, ADDITIONAL_STATUS_INFO, _additionalStatusInfo);

	if (false == ImmUtil::Instance().applyTransaction()) {
		WARN_COM_RP("Reencryptor::doReencryption(): Unable to set value of %s=%d in %s", ENCRYPTION_STATUS, int(_encryptionStatus),  _childImmDn.c_str());
	}
}

static void handleOiCcbAbortCallback (SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId)
{
	DEBUG_COM_RP("handleOiCcbAbortCallback(): callback received for %llu", ccbId);
	Reencryptor::Instance().removeCcbId(ccbId, true);
}

static void handleOiCcbApplyCallback (SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId)
{
	DEBUG_COM_RP("handleOiCcbApplyCallback(): callback received for %llu", ccbId);
	Reencryptor::Instance().doReencryption(ccbId);
	Reencryptor::Instance().removeCcbId(ccbId);
}

static SaAisErrorT handleOiCcbCompletedCallback (SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId)
{
	DEBUG_COM_RP("handleOiCcbCompletedCallback(): callback received for %llu", ccbId);
	return SA_AIS_OK;
}

static SaAisErrorT handleOiCcbObjectCreateCallback (SaImmOiHandleT immOiHandle,
                                                    SaImmOiCcbIdT ccbId,
                                                    const SaImmClassNameT className,
                                                    const SaNameT *parentName,
                                                    const SaImmAttrValuesT_2 **attr)
{
	DEBUG_COM_RP("handleOiCcbObjectCreateCallback(): callback received for %llu", ccbId);
	return SA_AIS_OK;
}

static SaAisErrorT handleOiCcbObjectDeleteCallback (SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId, const SaNameT *objectName)
{
	DEBUG_COM_RP("handleOiCcbObjectDeleteCallback(): callback received for %llu", ccbId);
	return SA_AIS_OK;
}

static SaAisErrorT handleOiCcbObjectModifyCallback (SaImmOiHandleT immOiHandle,
                                                    SaImmOiCcbIdT ccbId,
                                                    const SaNameT *objectName,
                                                    const SaImmAttrModificationT_2 **attrMods)
{

	std::string immObject = ImmUtil::Instance().makeString(objectName);

	DEBUG_COM_RP("handleOiCcbObjectModifyCallback(): callback received object[%s] ccbId[%llu]", immObject.c_str(), ccbId);

	if (immObject == std::string(SEC_ENCRYPTION_PARTICIPANTM_OBJECT)) {

		if (Reencryptor::Instance().updateParentEncryptionKeyUuid(attrMods)) {

			Reencryptor::Instance().setCcbId(ccbId);
		}
	}

	return SA_AIS_OK;
}
