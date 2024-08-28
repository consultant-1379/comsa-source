#include "OamSATranslator_dummy.h"

OamSATranslator::OamSATranslator()
{

}

OamSATranslator::~OamSATranslator()
{

}

std::string OamSATranslator::ImmRdn2MOTop(const std::string immRdn)
{
	return immRdn;
}

MafOamSpiMoActionT* OamSATranslator::GetComAction(OamSACache::DNList& mo_name, const std::string& actionName)
{
	return 0;
}

MafReturnT  OamSATranslator::ConvertActionReturnToImmParameters(MafOamSpiMoActionT *theAction_p, MafMoAttributeValueContainer_3T **theComParameterList_pp, SaImmAdminOperationParamsT_2 	***theAdminOpParams_ppp, bool isMoSpiVersion2)
{
	return MafOk;
}
OamSATranslator theTranslator;


bool OamSATranslator::MO2Imm_DN(const char* mo_name, char** imm_name)
{
	std::string imm_string(mo_name);
	imm_string = "toImm" + imm_string;
	*imm_name = (char*) malloc(imm_string.length() + 1);
	strcpy(*imm_name, imm_string.c_str());
	return true;
}

MafOamSpiMocT* OamSATranslator::GetComMoc(OamSACache::DNList& mo_name)
{
	ENTER();
	MafOamSpiMocT*	theMoc_p = utMoc;
	LEAVE();
	return theMoc_p;
}

bool OamSATranslator::Imm2MO_DN(MafOamSpiTransactionHandleT txHandle, OamSACache::DNList& imm_name, std::string& n3gpp_name)
{
	return true;
}

void OamSATranslator::ResetMOMRoot() {
}

bool OamSATranslator::MO2Imm_DN(OamSACache::DNList& mo_name, std::string& imm_name)
{
	imm_name = imm_name_update;
	return true;
}
