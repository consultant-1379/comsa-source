#include "OamSACache_dummy.h"
#include <cstdio>

#define DEBUG printf
#define ERR printf

OamSACache::OamSACache()
{
}

OamSACache::~OamSACache()
{
}

void GlobalSplitMocPath(const std::string& mocPath, OamSACache::MocPathList& theSplitPath) {
	ENTER();

	if(mocPath.length() <= 0) {
		DEBUG("MocPath is empty");
		LEAVE();
		return;
	}

	//	Check if mocPath starts with '/'
	if(mocPath[0] != '/') {
		ERR("MocPath does not start with '/'");
		LEAVE();
		return;
	}

	DEBUG("Split MocPath input string %s\n", mocPath.c_str());

	size_t pos;
	size_t oldpos = 1;

	for(pos=mocPath.find_first_of('/',oldpos); pos!=std::string::npos; pos=mocPath.find_first_of('/',oldpos)) {
		theSplitPath.push_back(mocPath.substr(oldpos,pos-oldpos));
		oldpos = pos + 1;
	}

	theSplitPath.push_back(mocPath.substr(oldpos,mocPath.size()-oldpos));

	LEAVE();
}

void GlobalSplitDN(const std::string& theDN, OamSACache::DNList& theSplitDN)
{
	ENTER();
	size_t pos;
	size_t oldpos = 0;


	DEBUG("Split DN input string %s\n", theDN.c_str());


	for (pos = theDN.find_first_of(',',oldpos) ;pos != std::string::npos; pos = theDN.find_first_of(',',oldpos))
	{
		theSplitDN.push_back(theDN.substr(oldpos,pos-oldpos));
		oldpos = pos + 1;
	}
	theSplitDN.push_back(theDN.substr(oldpos,theDN.size()-oldpos));

	LEAVE();
}
