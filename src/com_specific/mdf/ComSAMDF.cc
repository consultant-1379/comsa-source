/*******************************************************************************
 * Copyright (C) 2011 by Ericsson AB
 * S - 125 26  STOCKHOLM
 * SWEDEN, tel int + 46 10 719 0000
 *
 * The copyright to the computer program herein is the property of
 * Ericsson AB. The program may be used and/or copied only with the
 * written permission from Ericsson AB, or in accordance with the terms
 * and conditions stipulated in the agreement/contract under which the
 * program has been supplied.
 *
 * All rights reserved.
 *
 *
 * Author: uabjoy
 *
 * File: ComSAMDF.cc
 * Created on: Aug 22, 2011
 *
 * Model Delivery Function that link the following models
 *
 * The model type IMM-I-FileM_R1 to type IMM_R1,
 * if the model fragment FileM exist in the MOM Repository of COM and
 * the MDF service exist in coreMW. This code will not do anything
 * if not MDF exist in coreMW to ensure the backward functionality.
 *
 * The model type IMM-I-FM_R1 is linked to type IMM_R1
 * The linking is executed when the COMSA library is loaded by COM BASIC
 *
 * The model type IMM-I-Local-Authorization_R1 is linked to type IMM_R1
 * The linking is executed when the COMSA library is loaded by COM BASIC.
 *
 * The instance model linkage is never removed, so if COMSA is removed from
 * the system, the model linkage must be removed in the remove campaign.
 *******************************************************************************
 *
 * Reviewed: eaparob 2011-09-19
 * Reviewed: ejnolsz 2011-09-20
 *
 * Modify:   uabjoy 2012-08-28
 */

#include <stdio.h>
#include <stdlib.h>
#include <ComSA.h>
#include "OamSATranslator.h"
#include "ProcessUtil.h"
#include <signal.h>
/**
 *  External references
 */
extern OamSATranslator theTranslator;

/**
 * Forward declarations */
/* Constants */
static const char* ModelTypeName = "IMM-I-FileM_R1";
static const char* BaseModelType = "IMM_R1";
static const char* TopClassName  = "FileM";

/*
 * Instance model types published by COMSA
 */
static const unsigned int NumberOfInstanceModels = 2;
static const char*        InstanceModelTypes[NumberOfInstanceModels] = { "IMM-I-FM_R1",
											                             "IMM-I-Local_Authorization_R1"};

/* Test code */
/* static const char* TopClassName = "ManagedElement"; */

/* Data */

// Flag
// True if we have registered as consumers
// False if we not have registered
static bool Registered = false;

/*  --------------------------------------------------------------------------------------------
 *  setUpInstanceModelConsumers
 *  The function perform the following task
 *  1. Check if the MDF service is available on the coreMW component.
 *  2. If 2 is true, link in MDF that IMM-I-FM_R1 and IMM-I-Local-Authorization_R1
 *     of the ModelTypeName is consumed as an IMM_R1 model type.
 *
 *  The function do not return any value, so it can not fail :-)
 */
extern "C" void setUpInstanceModelConsumers() {
	/* Command used */
	const char* check_modeltype_link = "cmw-modeltype-link --help";
	const char* link_command         = "cmw-modeltype-link";
	int ret = 0;
	ENTER_MWSA();

	DEBUG_MWSA("setUpInstanceModelConsumers : enter setUpInstanceModelConsumers phase 1");
	/* 1. Check if the MDF service is available on the coreMW component. */

	aaservice::ProcessUtil pu;
	std::vector<std::string> result;
	ret = pu.run(check_modeltype_link, result);

	// The execution is expected to fail, but the result
	// command not found
	// Means no MDF on the system, in that case ret is equal to 127
	// which is bash return code for command not found.
	if (ret == 127) {
		/* Okej, no MDF on system */
		DEBUG_MWSA("setUpInstanceModelConsumers : No MDF present on the system");
		LEAVE_MWSA();
		return;
	}

	DEBUG_MWSA("setUpInstanceModelConsumers : enter setUpInstanceModelConsumers phase 2");
	/*  3. If 2 is true, link in MDF that model-types of the InstanceModelTypes is consumed as an
	 *     IMM model-type */
	unsigned int modelTypeCounter = 0;
	do {
		// first create registration string
		std::string myExpression = std::string(link_command) + " "
				+ std::string(InstanceModelTypes[modelTypeCounter]) + " " + std::string(BaseModelType);
		result.clear();
		ret = pu.run(myExpression, result);
		if (ret != 0) {
			// We do not give but try to register the next type.
			// But we make a note in the log so it is clear that we failed to do this.
			WARN_MWSA("setUpInstanceModelConsumers : Failed to link %s model-type", InstanceModelTypes[modelTypeCounter]);
		} else {
			LOG_MWSA("setUpInstanceModelConsumers : ComSA linked %s model-type", InstanceModelTypes[modelTypeCounter]);
		}
		++modelTypeCounter;
	}while (modelTypeCounter < NumberOfInstanceModels);
	LEAVE_MWSA();

	return;
}

// Kill all the child processes if the pid is greater than zero,
// when the com is gracefully terminated
extern "C" void killChildIfAny() {

	int childPid = aaservice::ProcessUtil::childProcessId;
	if ( childPid > 0 ){
		DEBUG_MWSA_GENERAL ("killChildIfAny: child pid %d", childPid);
		kill(-(childPid), 9);
		aaservice::ProcessUtil::childProcessId = 0;
	}

}

/*  --------------------------------------------------------------------------------------------
 *  setUpFileMConsumer
 *  This function is called in AMF-state SA_AMF_HA_ACTIVE
 *  The function perform the following task
 *  1. Check if the MDF service is available on the coreMW component.
 *  2. If 1 is true, check in the MOMRespoitory of COM if the class TopClassName is present
 *     in the MOM, MOM = Managed Object Model.
 *  3. If 2 is true, link in MDF that model-types of the ModelTypeName is consumed as an
 *     IMM_R1 model type.
 *
 *  The function do not return any value, so it can not fail :-)
 *  But it can take more than 1 second to execute since it starts a bash shell and execute
 *  a shell script. If this is a problem it must be started in a separate thread.
 */

extern "C" void setUpFileMConsumer() {
	/* Command used */
	const char* check_modeltype_link = "cmw-modeltype-link --help";
	const char* link_command         = "cmw-modeltype-link";
	int ret = 0;
	ENTER_MWSA();

	DEBUG_MWSA("setUpFileMConsumer : enter setUpFileMConsumer phase 1");
	/* 1. Check if the MDF service is available on the coreMW component. */

	aaservice::ProcessUtil pu;
	std::vector<std::string> result;
	ret = pu.run(check_modeltype_link, result);

	// The execution is expected to fail, but the result
	// command not found
	// Means no MDF on the system, in that case ret is equal to 127
	// which is bash return code for command not found.
	if (ret == 127) {
		/* Okej, no MDF on system */
		DEBUG_MWSA("setUpFileMConsumer : No MDF present on the system");
		LEAVE_MWSA();
		return;
	}

	DEBUG_MWSA("setUpFileMConsumer : enter setUpFileMConsumer phase 2");
	/*  2. If 1 is true, check in the MOMRespoitory of COM if the class TopClassName is present
	 *     in the MOM, MOM = Managed Object Model.*/
	if (!theTranslator.IsClassNamePresent(std::string(TopClassName))) {
		// Okej, No FileM in the repository
		DEBUG_MWSA("setUpFileMConsumer : No %s in the MOM Repository", TopClassName);
		LEAVE_MWSA();
		return;
	}

	DEBUG_MWSA("setUpFileMConsumer : enter setUpFileMConsumer phase 3");
	/*  3. If 2 is true, link in MDF that model-types of the ModelTypeName is consumed as an
	 *     IMM model-type */
	// first create registration string
	std::string myExpression = std::string(link_command) + " "
	+ std::string(ModelTypeName) + " " + std::string(BaseModelType);
	result.clear();
	ret = pu.run(myExpression, result);
	if (ret != 0) {
		WARN_MWSA("setUpFileMConsumer : Failed to link %s model-type", ModelTypeName);
		LEAVE_MWSA();
		return;
	} else {
		LOG_MWSA("setUpFileMConsumer : ComSA linked %s model-type", ModelTypeName);
		LEAVE_MWSA();
		Registered = true;
		return;
	}

	LEAVE_MWSA();

	return;
}

/*  --------------------------------------------------------------------------------------------
 *  removeFileMConsumer
 *  This is called in AMF state SA_AMF_HA_QUIESCING
 *  The function calls MDF command cmw--modeltype-unlink to stop consuming of model-type ModelTypeName
 *  on this node. The active instance of ComSA will be responsible for the consuming
 *  IMM-I-FileM models
 *
 *  The function do not return any value, so it can not fail :-)
 */
// TODO : Kill child process  when its hang as a part of re-factoring comsa code
extern "C" void removeFileMConsumer() {
	/* Okej, lets unregister if we have registered before. */
	if (Registered) {
		LOG_MWSA("removeFileMConsumer Unregister %s model-type", ModelTypeName);
		aaservice::ProcessUtil pu;
		std::vector<std::string> result;
		std::string unRegisterComSa = "cmw-modeltype-unlink " + std::string(ModelTypeName);
		pu.run(unRegisterComSa);

		Registered = false;
	}
}
