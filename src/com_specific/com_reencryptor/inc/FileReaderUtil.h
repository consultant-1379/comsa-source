#ifndef __REENCRYPTOR_FILE_READER_UTIL_H
#define __REENCRYPTOR_FILE_READER_UTIL_H
#include <stdlib.h>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <queue>
#include <cstring>

typedef std::map<std::queue<std::string>, std::vector<std::string> > mapImmPathToAttributesT;
typedef std::map<std::queue<std::string>, std::vector<std::string> >::iterator mapImmPathToAttributesIterT;

class FileReaderUtil
{
private:
	mapImmPathToAttributesT _encryptedAttrData;
	std::string _immFilePath;

	FileReaderUtil();

	/* Parses the given string into dn and attribute strings*/
	void tokenizeFileData(const std::string& strFromFile);
	bool parseImmFile();

public:

	static inline FileReaderUtil& Instance() {
		static FileReaderUtil fileReaderUtil;
		return fileReaderUtil;
	}

	~FileReaderUtil();

	void init(const std::string& filePath);
	bool getSecretAttrData(mapImmPathToAttributesT& pEncryptedAttrData);
};

#endif
