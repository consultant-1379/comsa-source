#ifndef __REENCRYPTOR_SEC_UTIL_H
#define __REENCRYPTOR_SEC_UTIL_H
#include <stdlib.h>
#include <string>

class SecUtil
{
	typedef int (*secEncrypt_T)(char** encryptedString, const char* inputString);
	typedef int (*secDecrypt_T)(char** decryptedString, const char* inputString);

	const std::string _secCryptoLibrary;
	void* _secCryptoHandle;
	secEncrypt_T secEncrypt;
	secDecrypt_T secDecrypt;

	SecUtil();

public:

	static inline SecUtil& Instance() {
		static SecUtil secUtil;
		return secUtil;
	}

	bool initSecCryptoLib();

	bool reencrypt(const std::string& oldSecret, std::string& newSecret);

	~SecUtil();
};

#endif
