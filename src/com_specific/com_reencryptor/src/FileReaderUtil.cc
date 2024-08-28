#include "FileReaderUtil.h"
#include "Trace.h"
#include "Defines.h"

FileReaderUtil::FileReaderUtil()
{
	_encryptedAttrData.clear();
}

FileReaderUtil::~FileReaderUtil()
{
	_encryptedAttrData.clear();
	_immFilePath.clear();
}

void FileReaderUtil::init(const std::string &filePath)
{
	bool flag = false;
	_immFilePath = filePath;

	int count = 0;
	do {
		if (count++) {
			usleep(USLEEP_HALF_SECOND);
		}

		flag = parseImmFile();
		if (flag) {
			break;
		}

	} while (count <= MAX_FILE_ACCESS_RETRIES);

	if (false == flag) {
		WARN_COM_RP("FileReaderUtil::init():File open failed!!!");
	}
}

void FileReaderUtil::tokenizeFileData(const std::string& strFromFile)
{
	char* tmpStr1;
	char* tmpStr2;

	DEBUG_COM_RP("FileReaderUtil::tokenizeFileData(): string from file is [%s]", strFromFile.c_str());

	if (strFromFile == "") {
		return;
	}

	tmpStr1 = const_cast<char*>(strFromFile.c_str());
	tmpStr2 = strtok_r(tmpStr1, SEMI_COLON_DELIMITER , &tmpStr1);

	if(tmpStr1 && tmpStr2) {
		std::queue<std::string> vKey;
		std::vector<std::string> vVal;
		char* token;

		while ((token = strtok_r(tmpStr2, COMMA_DELIMITER, &tmpStr2))) {
			vKey.push(token);
		}

		while ((token = strtok_r(tmpStr1, COMMA_DELIMITER, &tmpStr1))) {
			vVal.push_back(token);
		}
		_encryptedAttrData[vKey] = vVal;
	}
}

bool FileReaderUtil::parseImmFile()
{
	bool retVal = false;
	std::ifstream fileS;

	fileS.open (_immFilePath.c_str());
	if (fileS.is_open()) {

		std::string tmpLine;
		DEBUG_COM_RP("FileReaderUtil::parseImmFile():File open success!!!");

		while (fileS.good()) { // loop continue until reaching the end of file.
			std::getline(fileS, tmpLine);
			tokenizeFileData(tmpLine);
		}
		fileS.close();
		retVal = true;
	}
	return retVal;
}

bool FileReaderUtil::getSecretAttrData(mapImmPathToAttributesT& pEncryptedAttrData)
{
	bool retVal = false;
	if (_encryptedAttrData.empty()) {

		retVal = parseImmFile();
		if (retVal) {
			pEncryptedAttrData = _encryptedAttrData;
		}
		else {
			WARN_COM_RP("FileReaderUtil::getEncryptedAttrData(): Unable to open file [%s]", _immFilePath.c_str());
		}
	}
	else {
		pEncryptedAttrData = _encryptedAttrData;
		retVal = true;
	}

	return retVal;;
}
