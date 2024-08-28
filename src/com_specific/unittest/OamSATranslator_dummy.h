#ifndef OAMSATRANSLATOR_H_
#define OAMSATRANSLATOR_H_

#include <map>
#include <string>
#include <algorithm>
#include <cctype>
#include <list>
#include <stack>
#include "saImm.h"
#include "MafOamSpiManagedObject_3.h"
#include "MafOamSpiModelRepository_1.h"
#include "MafOamSpiModelRepository_4.h"
#include "OamSACache.h"
#include "imm_utils.h"
#include "OamSAKeyAttributeRepository.h"

extern SaImmAccessorHandleT accessorHandleOI;

class OamSATranslator 
{
public:
	OamSATranslator();
	~OamSATranslator();
	std::string ImmRdn2MOTop(const std::string immRdn);
	MafOamSpiMoActionT* GetComAction(OamSACache::DNList& mo_name, const std::string& actionName);
	MafReturnT  ConvertActionReturnToImmParameters(MafOamSpiMoActionT *theAction_p, MafMoAttributeValueContainer_3T	**theComParameterList_pp, SaImmAdminOperationParamsT_2 	***theAdminOpParams_ppp, bool isMoSpiVersion2 = false);
	bool MO2Imm_DN(const char* mo_name, char** imm_name);
	MafOamSpiMocT* GetComMoc(OamSACache::DNList& mo_name);
	bool Imm2MO_DN(MafOamSpiTransactionHandleT txHandle, OamSACache::DNList& imm_name, std::string& n3gpp_name);
	bool MO2Imm_DN(OamSACache::DNList& mo_name, std::string& imm_name);
	void ResetMOMRoot();

	MafOamSpiMoc* utMoc;
	std::string imm_name_update;
};

#endif
