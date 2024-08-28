#include "SecUtil.h"
#include "Trace.h"
#include "Defines.h"

#include <dlfcn.h>

#include <cstdlib>

SecUtil::SecUtil() : _secCryptoLibrary(SEC_CRYPTO_LIB),
                     _secCryptoHandle(NULL),
                     secEncrypt(0),
                     secDecrypt(0)
{

}

SecUtil::~SecUtil()
{
	if (_secCryptoHandle) {
		dlclose(_secCryptoHandle);
		_secCryptoHandle = NULL;
	}

	secEncrypt = NULL;
	secDecrypt = NULL;
}

bool SecUtil::initSecCryptoLib()
{
	if (_secCryptoHandle) {
		DEBUG_COM_RP("SecUtil::initSecCryptoLib: Handle already initialized ");
		dlclose(_secCryptoHandle);
		_secCryptoHandle = NULL;
	}

	_secCryptoHandle = dlopen(_secCryptoLibrary.c_str(), RTLD_LAZY);
	if(NULL == _secCryptoHandle) {
		WARN_COM_RP("SecUtil::initSecCryptoLib(): %s SEC_CRYPTO_LIB not found, %s", _secCryptoLibrary.c_str(), dlerror());
		return false;
	}

	DEBUG_COM_RP("SecUtil::initSecCryptoLib(): SEC_CRYPTO_LIB FOUND %s", _secCryptoLibrary.c_str());

	secEncrypt = reinterpret_cast<secEncrypt_T>(dlsym(_secCryptoHandle, "sec_crypto_encrypt_ecimpassword"));
	if (NULL == secEncrypt) {

		WARN_COM_RP("SecUtil::initSecCryptoLib(): Symbol 'sec_crypto_encrypt_ecimpassword' not found. %s", dlerror());
		dlclose(_secCryptoHandle);
		_secCryptoHandle = NULL;
		return false;
	}
	DEBUG_COM_RP("SecUtil::initSecCryptoLib(): sec_crypto_encrypt_ecimpassword FOUND %s", _secCryptoLibrary.c_str());

	secDecrypt = reinterpret_cast<secDecrypt_T>(dlsym(_secCryptoHandle, "sec_crypto_decrypt_ecimpassword"));
	if (NULL == secDecrypt) {

		WARN_COM_RP("SecUtil::initSecCryptoLib(): Symbol 'sec_crypto_decrypt_ecimpassword' not found. %s", dlerror());
		dlclose(_secCryptoHandle);
		_secCryptoHandle = NULL;
		return false;
	}
	DEBUG_COM_RP("SecUtil::initSecCryptoLib(): sec_crypto_decrypt_ecimpassword FOUND %s", _secCryptoLibrary.c_str());

	DEBUG_COM_RP("SecUtil::initSecCryptoLib(): Successfully loaded libsec_crypto_api.so.1");
	return true;
}

bool SecUtil::reencrypt(const std::string& oldSecret, std::string& newSecret)
{
	if (oldSecret.empty()) {
		DEBUG_COM_RP("SecUtil::reencrypt(): oldSecret empty");
		return false;
	}

	bool retVal = true;

	if ((NULL == _secCryptoHandle)
	     || (NULL == secEncrypt)
	     || (NULL == secDecrypt))
	{
		retVal = initSecCryptoLib();
		if (false == retVal) {
			DEBUG_COM_RP("SecUtil::reencrypt(): failed to initSecCryptoLib()");
			return retVal;
		}
	}

	/* NOTE: DO NOT print value of this attribute anywhere.
	   Called 'Secrets' for a good reason :) */
	char* plainText = NULL; //TODO:memory leaks check
	char* tempSecret = NULL;

	if (SEC_CRYPTO_OK != secDecrypt(&plainText, oldSecret.c_str())) {

		retVal = false;
		DEBUG_COM_RP("SecUtil::reencrypt(): Unable to decrypt oldSecret");
	}

	if (retVal && (SEC_CRYPTO_OK != secEncrypt(&tempSecret, (const char*)plainText))) {

		retVal = false;
		DEBUG_COM_RP("SecUtil::reencrypt(): Unable to encrypt plainText");
	}

	if (retVal) {
		newSecret = std::string(tempSecret);
	}

	if (tempSecret) {
		free (tempSecret);
		tempSecret = NULL;
	}

	if (plainText) {
		free(plainText);
		plainText = NULL;
	}

	DEBUG_COM_RP("SecUtil::reencrypt(): encrypted successfully!");
	return retVal;
}
