/*
 * ParameterVerifier.h
 *
 *  Created on: Sep 22, 2011
 *      Author: uabjoy
 *
 *   Modified: xnikvap 2012-08-30  support for COM MO SPI Ver.3 (Ver.1 is not supported any more)
 */

#ifndef PARAMETERVERIFIER_H_
#define PARAMETERVERIFIER_H_
#include <vector>
// CoreMW
#include "saAis.h"
#include "saImm.h"
#include "saImmOm.h"
// COM
#include "MafMgmtSpiCommon.h"
#include "MafMgmtSpiComponent_1.h"
#include "MafMgmtSpiInterfacePortal_1.h"
#include "MafOamSpiModelRepository_1.h"
#include "MafOamSpiManagedObject_3.h"
// COMSA
#include "OamSATransactionRepository.h"
#include "OamSACache.h"
#include "TxContext.h"
#include "ImmCmd.h"

class ParameterVerifier {

public:
	ParameterVerifier() {	m_parameterModel = 0; }
	virtual MafMoAttributeValueContainer_3T **getParams()=0;
	virtual MafOamSpiParameterT* getParameterModelList()=0;
	virtual void checkParameters(SaImmAdminOperationParamsT_2 **params);
	virtual ~ParameterVerifier() {

		vector<MafMoAttributeValueContainer_3T*>::iterator iter = m_paramList.begin();
		while (iter != m_paramList.end()) {
			MafMoAttributeValueContainer_3T* p = *iter;
			if (p != 0) {
				if (p->values != 0)
					delete p->values;
				delete p, p = 0;
			}
			iter++;
		}
		m_paramList.clear();

		MafOamSpiParameterT* modelParam = m_parameterModel;
		while (modelParam) {
			if (modelParam->parameterType.derivedDatatype != 0) {
				delete modelParam->parameterType.derivedDatatype;
			}
			MafOamSpiParameterT* next = modelParam->next;
			delete modelParam;
			modelParam = next;
		}
		m_parameterModel = 0;
	}

protected:
	MafOamSpiParameter* createParameter(MafOamSpiDatatype dataType, char* name,
			MafOamSpiParameter* prev);
	void setParam(MafOamSpiMoAttributeType dataType,
				  MafMoAttributeValue_3 *value);
	void checkTypeAndValue(MafOamSpiDatatypeT comType, SaImmValueTypeT immType,
						   MafMoAttributeValue_3 *expectedValue, void *actualValuePt);

	vector<MafMoAttributeValueContainer_3T*> m_paramList;
	MafOamSpiParameter *m_parameterModel;
};

#endif /* PARAMETERVERIFIER_H_ */
