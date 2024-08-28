#include <string>
#include <cstring>
#include <cstdio>

#define TRACE printf ("             "); printf

int encrypt (char** encryptedString, const char* inputString)
{
	TRACE("MockSecUtil.cc encrypt() ENTER %s\n", inputString);
	std::string in(inputString);

	size_t pos = in.find("@failEncrypt@");
	if (pos != std::string::npos) {
		TRACE("MockSecUtil.cc encrypt() @failEncrypt@\n");
		return 1;
	}

	std::string out("Avi:");
	out += in;
	(*encryptedString) = strdup(out.c_str());
	TRACE("MockSecUtil.cc encrypt() LEAVE %s\n", *encryptedString);
	return 0;
}

int decrypt (char** decryptedString, const char* inputString)
{
	TRACE("MockSecUtil.cc decrypt ENTER %s\n", inputString);
	std::string in(inputString);

	size_t pos = in.find("@failDecrypt@");
	if (pos != std::string::npos) {
		TRACE("MockSecUtil.cc decrypt() @failDecrypt@\n");
		return 1;
	}

	pos = in.find(":");
	if (pos == std::string::npos)
		return -1;
	std::string out = in.substr(pos + 1);
	(*decryptedString) = strdup(out.c_str());
	TRACE("MockSecUtil.cc decrypt() LEAVE %s\n", *decryptedString);
	return 0;
}

extern "C" int sec_crypto_encrypt_ecimpassword(char** encryptedString, const char* inputString)
{
	int retValue = encrypt(encryptedString, inputString);
	return retValue;
}

extern "C" int sec_crypto_decrypt_ecimpassword (char** decryptedString, const char* inputString)
{
	int retValue = decrypt(decryptedString, inputString);
	return retValue;
}
