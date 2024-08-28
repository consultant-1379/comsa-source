#include <cstdlib>
#include <cstdio>
#include <string>
#include <queue>

#define TRACE printf ("             "); printf

#include "saAis.h"
#include "saImm.h"
#include "saLog.h"

typedef struct {
	SaUint16T length;
	SaUint8T value[256];
} TheSaNameT;

typedef std::queue<std::string> ObjectQueueT;

struct GetNextCache
{
	std::string obj_attr;
	SaImmAttrValuesT_2** value;
	SaImmValueTypeT type;

	GetNextCache(std::string s) : value(NULL), type(SA_IMM_ATTR_SASTRINGT)
	{
		obj_attr = s;
	};
};

struct SearchNextCache
{
	std::string root_class;
	ObjectQueueT objects;

	SearchNextCache(std::string s) : objects(std::queue<std::string>())
	{
		root_class = s;
	}
};

struct CcbCache
{
	std::string obj_attr;
	std::string str;
	int i;

	CcbCache(std::string s) : str(""), i(0)
	{
		obj_attr = s;
	}
};

void add2Accessor(std::string obj, std::string attr, SaImmAttrValuesT_2** value, SaImmValueTypeT type);

void add2Search(std::string root, std::string className, ObjectQueueT objs);

bool assertCcbValue(std::string obj, std::string attr, std::string value);
bool assertCcbValue(std::string obj, std::string attr, int value);

void resetAllCaches();

#ifndef __REENCRYPTOR_IMMCMD_H
#define __REENCRYPTOR_IMMCMD_H


#endif
