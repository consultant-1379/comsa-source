/*
 * StubIMM.h
 *
 *  Created on: Sep 22, 2011
 *      Author: uabjoy
 */

#ifndef STUBIMM_H_
#define STUBIMM_H_

#include <vector>
#include <string>
#include <map>
#include <cstdio>
#include "saAis.h"
#include "saImm.h"
#include "saImmOm.h"

/////// IMM tracks memory //////////
class ImmItem;
class ImmMemoryTracker {
public:
	std::vector<SaImmAttrDefinitionT_2**> attrDefs;
	std::vector<SaImmAttrValuesT_2**> attrVals;

	void reg(SaImmAttrDefinitionT_2** defArray) {
		attrDefs.push_back(defArray);
	}

	void reg(SaImmAttrValuesT_2** valArray) {
		attrVals.push_back(valArray);
	}

	void clear(SaImmAttrValuesT_2* v) {
		SaImmAttrValuesT_2* res = v;
		for (int i = 0; i < res->attrValuesNumber; i++) {
			switch (res->attrValueType) {
			case SA_IMM_ATTR_SANAMET:
				delete (SaNameT*) res->attrValues[i];
				break;
			case SA_IMM_ATTR_SASTRINGT:
				delete[] (char*) (*(char**) res->attrValues[i]);
				delete (char**) res->attrValues[i];
				break;
			}
		}
		delete[] res->attrName;
		delete[] res->attrValues;
		delete v;
	}

	void clear(SaImmAttrDefinitionT_2* d) {
		//delete [] (char*)d->attrName;  // why invalid!?!?
		delete d;
	}

	void clear(SaImmAttrValuesT_2** vArray) {
		int i = 0;
		while (vArray[i] != NULL) {
			clear(vArray[i]);
			i = i + 1;
		}
		delete[] vArray;
	}

	void clear(SaImmAttrDefinitionT_2** dArray) {
		int i = 0;
		while (dArray[i] != NULL) {
			clear(dArray[i]);
			i = i + 1;
		}
		delete[] dArray;
	}

	void cleanup() {
		for (int i = 0; i < attrDefs.size(); i++) {
			clear(attrDefs[i]);
		}
		for (int i = 0; i < attrVals.size(); i++) {
			clear(attrVals[i]);
		}
	}
};

class ImmItem {
	// default constructor and we never copy
public:

	unsigned int type;
	std::vector<std::string> data;

	bool isEmpty() {
		return data.size() == 0;
	}

	std::string toString() {
		std::string outp;
		outp.append("ImmItem( type=");
		switch (type) {
		case 0:
			outp.append("0");
			break;
		case 1:
			outp.append("1");
			break;
		case 2:
			outp.append("2");
			break;
		case 3:
			outp.append("3");
			break;
		case 4:
			outp.append("4");
			break;
		case 5:
			outp.append("5");
			break;
		case 6:
			outp.append("6");
			break;
		case 7:
			outp.append("7");
			break;
		case 8:
			outp.append("8");
			break;
		case 9:
			outp.append("9");
			break;
		default:
			outp.append("n");
			break;
		}
		outp.append(", data=");
		for (int i = 0; i < data.size(); i++) {
			outp.append(data[i]);
			outp.append(" ");
		}
		outp.append(")");
		return outp;
	}
};

class ImmAttrDef {
public:
	unsigned int attrType;
	std::string attrName;
};

class ImmClassDef {
public:
	std::vector<ImmAttrDef> attrDef;
};

class ImmStorage {

	// Storage backend for fake IMM impl
	std::map<std::string, ImmItem> immStorage;
	std::map<std::string, int> createLog;
	std::map<std::string, int> deleteLog;
	std::map<std::string, ImmClassDef> immClassStorage;

public:

	void reset() {
		immStorage.clear();
		createLog.clear();
		deleteLog.clear();
		immClassStorage.clear();
	}

	int getItemCount() {
		return immStorage.size();
	}

	bool isEmpty() {
		return immStorage.empty();
	}

	/**
	 * Add a class attribute definition, used by saImmOmClassDecriptionGet_2
	 */
	void addImmClassAttributeDef(std::string className, std::string attrName,
			unsigned int attrType) {
		ImmAttrDef attr;
		attr.attrType = attrType;
		attr.attrName = attrName;
		immClassStorage[className].attrDef.push_back(attr);
	}

	ImmClassDef getClassDef(std::string className) {
		return immClassStorage[className];
	}

	void addToImmStorage(const char* dn, const char* attr, unsigned int type,
			std::vector<std::string> values) {
		std::string key = makeKey(dn, attr);
		ImmItem item;
		item.type = type;
		item.data = values;
		immStorage[key] = item;
	}

	ImmItem readFromImmStorage(const char* dn, const char* attr) {
		std::string key = makeKey(dn, attr);
		if (immStorage.find(key) == immStorage.end()) {
			printf("Error: no such element, dn=%s attribute=%s\n", dn, attr);
			printf("Available elements:\n");
			for (std::map<std::string, ImmItem>::iterator it =
					immStorage.begin(); it != immStorage.end(); it++) {
				printf("   [%s]=>%s\n", (*it).first.c_str(),
						(*it).second.toString().c_str());
			}
		}
		ImmItem item = immStorage[key]; // creates default value if not found.
		return item;
	}

	ImmItem deleteFromImmStorage(const char* dn, const char* attr) {
		immStorage.erase(makeKey(dn, attr));
	}

	void logCreate(const char* dn) {
		createLog[std::string(dn)] = 1;
	}

	void logDelete(const char* dn) {
		deleteLog[std::string(dn)] = 1;
	}

private:

	std::string makeKey(const char* dn, const char* attr) {
		std::string key(dn);
		key.append(",");
		key.append(attr);
		return key;
	}

};



#endif /* STUBIMM_H_ */
