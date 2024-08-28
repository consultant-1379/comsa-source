#ifndef __REENCRYPTOR_REENCRYPTOR_H
#define __REENCRYPTOR_REENCRYPTOR_H

#include "ImmUtil.h"

#include <string>
#include <list>
#include <vector>
#include <queue>

class Reencryptor
{
	enum EncryptionStatusT {
		SUCCESS = 0,
		INPROGRESS,
		FAILURE
	};

	const std::string _parentImmDn;
	const std::string _childImmDn;
	std::string _parentKeyUuid;
	std::string _childKeyUuid;
	EncryptionStatusT _encryptionStatus;
	std::string _additionalStatusInfo;
	unsigned long long _ccbId;

	Reencryptor();

	bool getEncryptionStatus();
	bool isReencryptionNeeded();
	bool getObjects(const std::queue<std::string>& mocList, std::list<std::string>& foundObjects);
	bool makeSetAttributeData(std::list<std::string> objectsList, std::vector<std::string> attrList);
	bool populateData();

public:

	static inline Reencryptor& Instance() {
		static Reencryptor reencryptor;
		return reencryptor;
	}

	~Reencryptor();

	bool start(const char* pFlatFilePath);
	bool stop();

	/* Used by OI Applier callback functions */
	SaAisErrorT getSingleStringAttribute(const std::string& immObject, const std::string& attributeName, std::string& value);

	bool updateParentEncryptionKeyUuid(const SaImmAttrModificationT_2 **attrMods);
	void setCcbId(const unsigned long long& ccbId);
	void removeCcbId(const unsigned long long& ccbId, bool abortTx = false);

	void doReencryption(const unsigned long long& ccbId = 0);
};

#endif
