/******************************************************************************
 * Copyright (C) 2010 by Ericsson AB
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
 * Author: uablrek
 *
 * File:  ComSAOamComponent.c
 *****************************************************************************
 *
 * Reviewed: efaiami 2010-08-17
 *
 * Modify: efaiami 2011-02-24  for log and trace function
 *
 * Reviewed: eaparob 2011-09-19
 * Reviewed: ejnolsz 2011-09-20
 * Modify: eozasaf 2011-09-02  added the ComOamSpiRegisterObjectImplementer_1 interface to
 * 								the list of interfaces that the ComSA Oam component exports
 * 								and the ComOamSpiTransactionMaster_2Id to the dependencies
 * 								of the ComSA Oam component
 *
 * Modify: eaparob 2012-05-24 update ComNtfServiceOpen call for SDP1120 - temporary change to provide the MAF portal to ComNtfServiceOpen to be able to use MAF service for SDP1120
 * Modify: xjonbuc 2012-09-06  Implement SDP1694 - support MAF SPI
 * Modify: xjonbuc 2014-03-01  Implement MR29333 No root classes (Register MAF MR SPI V3, too)
 * Modify: xjonbuc 2014-06-24  Implement Transaction SPI v2 instead of v1
 * Modify: xnikvap 2015-08-02  MR42277 Remove old interfaces exposed to COM & CoreMW that are no longer supported for SLES12
 *
 ****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <ComSA.h>
#include <MafMwSpiServiceIdentities_1.h>
#include <MafOamSpiServiceIdentities_1.h>
#include <MafMgmtSpiInterface_1.h>
#include <OamSAManagedObjects.h>
#include <OamSATransactionalResource.h>
#include <OamSARegisterObjectImplementer.h>
#include <MafOamSpiTransaction_2.h>
#include <MafOamSpiEvent_1.h>
#include <MafOamSpiModelRepository_1.h>
#include <MafOamSpiModelRepository_4.h>
#include <ComSANtf.h>
#include <trace.h>
#include "LogEventProducer.h"
extern void pushComLogLevelEvents();

MafOamSpiTransaction_2T* MafOamSpiTransactionStruct_p_v2 = NULL;
MafOamSpiModelRepository_1T* theModelRepo_v1_p = NULL;
MafOamSpiModelRepository_4T* theModelRepo_v4_p = NULL;

int theModelRepoVersion = 0;

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE -1
#endif

#define UUID_CONF_STR "uuidmapping"
#define MAX_LINE_BUFF 1024
UuidCfg uuid_cfg_flag = UuidDisableHandle;
bool lock_mo_for_config_change = false;
bool non_root_mo_notifications = false;
SaUint64T maxSAMemoryForCMEvents = 0;

/* Forward declarations; */
/* MDF registration functions */
extern void setUpFileMConsumer();
/* OIProxy tearDown */
extern void ObjImp_finalize_imm(bool,bool);

/* Create Flat file needed by COM Reencryptor OI */
extern void createSecretAttrFile();

/* Data */
#define OAM_COMPONENT_NAME  "MW_OAM"
static char const* const oam_component_name = OAM_COMPONENT_NAME;

extern char* _CC_NAME;
/* Dependencies. Need to specified in order to let the components to
 * be started in the right order */

static MafMgmtSpiInterface_1T ModelRepId = MafOamSpiModelRepository_1Id;
static MafMgmtSpiInterface_1T TransId_v2 = MafOamSpiTransaction_2Id;
static MafMgmtSpiInterface_1T TransactionMasterId = MafOamSpiTransactionMaster_2Id;
static MafMgmtSpiInterface_1T EventServiceId = MafOamSpiEventService_1Id;
static MafMgmtSpiInterface_1T* deparray[] = { (MafMgmtSpiInterface_1T*) &ModelRepId, &TransId_v2, &EventServiceId, &TransactionMasterId, NULL };
static MafMgmtSpiInterface_1T* ifarray[5];
static MafMgmtSpiInterface_1T* maf_optarray[] =  { NULL };

static MafReturnT maf_comSAOamComponentStart(MafStateChangeReasonT reason);
static MafReturnT maf_comSAOamComponentStop(MafStateChangeReasonT reason);

static MafMgmtSpiComponent_2T maf_mw_oam = {
	.base = {OAM_COMPONENT_NAME, "MafMgmtSpiComponent", "2"},
	.interfaceArray = ifarray,
	.dependencyArray = deparray,
	.start = maf_comSAOamComponentStart,
	.stop = maf_comSAOamComponentStop,
	.optionalDependencyArray = maf_optarray
};

static MafMgmtSpiInterfacePortal_3T* ifportal_MAF = NULL;

/* SDP1694 - support MAF SPI
** Be careful switching between MAF and COM interfaces
** There are indirect linkages and shared resources between MAF and COM
** TransactionalResouce and ManagedObject both use ContextMap so they need to be the same type.
** TransactionalResouce uses either MafOamSpiTransactionStruct_p / ComOamSpiTransactionStruct_p so
** must not separate them.
*/

static MafReturnT maf_comSAOamComponentStart(MafStateChangeReasonT reason)
{
	ENTER_OAMSA();
	MafReturnT maf_rc;

	DEBUG_OAMSA("maf_comSAOamComponentStart called...");
	LOG_OAMSA("maf_comSAOamComponentStart(): OAM SA component start procedure begins: %llu",getMillisecondsSinceEpochUnixTime());

	maf_rc = ifportal_MAF->getInterface(MafOamSpiTransaction_2Id, (MafMgmtSpiInterface_1T**) &MafOamSpiTransactionStruct_p_v2);
	if (maf_rc != MafOk) {
		WARN_OAMSA("getInterface MafOamSpiTransaction_2Id, rc=%d", (int) maf_rc);
		LEAVE_OAMSA();
		return maf_rc;
	}

	// Get the model repository version 1 here, it is mandatory
	maf_rc = ifportal_MAF->getInterface(MafOamSpiModelRepository_1Id, (MafMgmtSpiInterface_1T**)&theModelRepo_v1_p);
	if (maf_rc != MafOk) {
		WARN_OAMSA("getInterface MafOamSpiModelRepository_1Id, rc=%d", (int) maf_rc);
		LEAVE_OAMSA();
		return maf_rc;
	}

	// get MAF model repository version 4 here
	DEBUG_OAMSA("maf_comSAOamComponentStart getting MAF MR SPI V.4 ...");
	maf_rc = ifportal_MAF->getInterface(MafOamSpiModelRepository_4Id, (MafMgmtSpiInterface_1T**)&theModelRepo_v4_p);
	if (maf_rc != MafOk) {
		WARN_OAMSA("getInterface MafOamSpiModelRepository_4Id, rc=%d",
		     (int)maf_rc);
	}
	else
	{
		DEBUG_OAMSA("### MAF MR SPI V4 REGISTERED SUCCESSFULLY");
		theModelRepoVersion = 4;
	}
	DEBUG_OAMSA("maf_comSAOamComponentStart getting MAF MR SPI V.4 returned: %p MafOk!", theModelRepo_v4_p);

	if ((maf_rc = OamSATransactionalResourceOpen()) != MafOk) {
		WARN_OAMSA("OamSATransactionalResourceOpen, rc=%d", (int) maf_rc);
		LEAVE_OAMSA();
		return maf_rc;
	}
	DEBUG_OAMSA("OamSATransactionalResourceOpen() Finished OK");

	if (0 == strcmp((const char*)CC_NAME, "Com")) {
		createSecretAttrFile(); //for COM only.
	}

	if ((maf_rc =  (MafReturnT) OamManagedObjectOpen()) != MafOk) {
		WARN_OAMSA("OamManagedObjectOpen, rc=%d", (int) maf_rc);
		LEAVE_OAMSA();
		return maf_rc;
	}
	DEBUG_OAMSA("OamManagedObjectOpen() Finished OK");

	if ((maf_rc = maf_ComNtfServiceOpen(ifportal_MAF)) != MafOk) {
		WARN_OAMSA("ComNtfServiceOpen, rc=%d", (int)maf_rc);
		LEAVE_OAMSA();
		return maf_rc;
	}
	DEBUG_OAMSA("maf_ComNtfServiceOpen() Finished OK");

	if (0 != strcmp((const char*)_CC_NAME, "lm")) {
		start_LogEventProducer(ifportal_MAF);
		pushComLogLevelEvents();
	}

	// Registration of the Model-Type IMM-I-FileM
	setUpFileMConsumer();
	DEBUG_OAMSA("setUpFileMConsumer() Finished OK");
	DEBUG_OAMSA("maf_comSAOamComponentStart return OK");
	LOG_OAMSA("maf_comSAOamComponentStart(): OAM SA component start procedure completed: %llu",getMillisecondsSinceEpochUnixTime());
	LEAVE_OAMSA();
	return maf_rc;
}

static MafReturnT maf_comSAOamComponentStop(MafStateChangeReasonT reason)
{

	ENTER_OAMSA();
	MafReturnT com_rc = MafOk;
	MafReturnT tmp_rc;
	DEBUG_OAMSA("maf_comSAOamComponentStop called...");
	LOG_OAMSA ("maf_comSAOamComponentStop(): OAM SA component stop procedure begins: %llu", getMillisecondsSinceEpochUnixTime());

	// Remove the IMM OIProxy registrations
	// Stop OIProxy registration in the IMM database.
	ObjImp_finalize_imm(true,true);

	/* Run all close functions even if some fails */
	if ((tmp_rc = OamSATransactionalResourceClose()) != MafOk)
		com_rc = tmp_rc;
	if ((tmp_rc = OamManagedObjectClose()) != MafOk)
		com_rc = tmp_rc;
        setMafStateChangeReason(reason);
	if ((tmp_rc = maf_ComNtfServiceClose()) != MafOk)
		com_rc = tmp_rc;
	LOG_OAMSA ("maf_comSAOamComponentStop(): OAM SA component stop procedure completed: %llu", getMillisecondsSinceEpochUnixTime());
	LEAVE_OAMSA();
	return com_rc;
}


static int case_insensitive_compare_uuid(const char* ucstr, char* str)
{
    /* this is always invented again, wish that someone could put it in stdlib */
    int ret_val = FALSE;
    int    i;

    if (strlen(str) >= strlen(ucstr))
    {
        ret_val = TRUE;
        for (i = 0; i < strlen(ucstr); i++)
        {
            if (toupper(str[i]) != toupper(ucstr[i]))
            {
                ret_val = FALSE;
                break;
            }
        }
    }
    DEBUG_OAMSA("case_insensitive_compare_uuid return %d", ret_val);
    return ret_val;
}

static UuidCfg get_config_state(const char* buf)
{
    UuidCfg ret_val = UuidDisableHandle;
    char* cptr = strchr(buf,'=');
    if (cptr != NULL)
    {
        cptr++;
        if (*cptr == '1')
        {
            ret_val = UuidMapToAddInfoName;
        }
        else if (*cptr == '2')
        {
            ret_val = UuidMapValueToAddText;
        }
        else if (*cptr == '0')
        {
            ret_val = UuidDisableHandle;
        }
        else
        {
            WARN_OAMSA("UUID configuration is invalid, take default config: uuid_cfg_flag = %d", ret_val);
        }
    }
    DEBUG_OAMSA("get_config_state return %d", ret_val);
    return ret_val;
}

static int getUUIDConfigurationPath(char* configPath)
{
    FILE *fpipe;
    int resCode = TRUE;

    char* getConfigDir = NULL;
    asprintf(&getConfigDir, "%s%s", _CC_NAME, "sa_pso getPath config"); // get the config path on the node
    if ( !(fpipe = (FILE*)popen(getConfigDir, "r")) )
    {
        ERR_OAMSA ("getUUIDConfigurationPath popen failed");
        if (getConfigDir != NULL)
        {
            free(getConfigDir);
            getConfigDir = NULL;
        }
        return FALSE;
    }
    int fd = fileno(fpipe);
    DEBUG_OAMSA("getUUIDConfigurationPath Open file desc = %d", fd);

    if (getConfigDir != NULL)
    {
        free(getConfigDir);
        getConfigDir = NULL;
    }

    char* str = NULL;
    while (fgets(configPath, MAX_LINE_BUFF, fpipe))
    {
        str = configPath;
        if (str[strlen(str) - 1] != '\n')
        {
          ERR_OAMSA("getUUIDConfigurationPath fgets failed, returns, configPath=%s", configPath);
          return FALSE;
        } else
        {
            str[strlen(str) - 1] = '\0';
            break;
        }
    }
    DEBUG_OAMSA("getUUIDConfigurationPath str=%s",str);

    if (pclose(fpipe) == -1)
    {
        ERR_OAMSA("getUUIDConfigurationPath pclose failed");
        return FALSE;
    }
    DEBUG_OAMSA("getUUIDConfigurationPath Closed file desc = %d", fd);

    if (!str)
    {
        ERR_OAMSA("getUUIDConfigurationPath str is NULL");
        return FALSE;
    }
    return resCode;
}
static void load_uuid_configuration()
{
    DEBUG_OAMSA("load_uuid_configuration called");

    FILE* cfg_fp = NULL;
    char configPath[MAX_LINE_BUFF];
    char UuidCfgAbsPath[MAX_LINE_BUFF];
    if (!getUUIDConfigurationPath(configPath))
    {
        WARN_OAMSA("load_uuid_configuration Fail to load uuid configuration");
        return;
    }

    sprintf(UuidCfgAbsPath, "%s%s%s%s%s%s", configPath, "/", COMSA_FOR_COREMW_DIR, "/etc/", _CC_NAME, "sa.cfg");

    LOG_OAMSA("load_uuid_configuration UuidCfgAbsPath=|%s|", UuidCfgAbsPath);
    if ((cfg_fp = fopen(UuidCfgAbsPath,"r")))
    {
        char    buf[MAX_PATH_DATA_LENGTH];
        for (;;)
        {
            if (fgets(buf,sizeof(buf),cfg_fp) == NULL)
                break;
            if ('#' == buf[0])
                continue;
            if (case_insensitive_compare_uuid(UUID_CONF_STR,buf))
            {
                uuid_cfg_flag = get_config_state(buf);
                break;
            }
        }
        LOG_OAMSA("load_uuid_configuration uuid_cfg_flag = %d", uuid_cfg_flag);
        fclose(cfg_fp);
    }
    else
    {
        WARN_OAMSA("load_uuid_configuration loading config [%s] failed", UuidCfgAbsPath);
    }
}

void parseNumber(const char* number, uint64_t scale)
{
	char *endptr = NULL;
	SaUint64T num=strtoull(number, &endptr, 10);
	if((endptr != NULL && *endptr=='\0') && num > 0){
		maxSAMemoryForCMEvents = num*scale;
	}
	return;
}

MafReturnT maf_comSAOamComponentInitialize(xmlDocPtr config, MafMgmtSpiInterfacePortal_3T* portal_MAF)
{
	ENTER_OAMSA();
	DEBUG_OAMSA("maf_comSAOamComponentInitialize called...");
	ifportal_MAF = portal_MAF;

	xmlNode* cfg;
	cfg = coremw_find_config_item(oam_component_name, "lockMoForConfigChange");
	if (cfg != NULL)
	{
		char const* value = coremw_xmlnode_contents(cfg);
		if(value && strcasecmp(value , "true") == 0){
			lock_mo_for_config_change = true;
			DEBUG_OAMSA("lockMoForConfigChange=%d", lock_mo_for_config_change);
		}
	}
	cfg = coremw_find_config_item(oam_component_name, "enableNonrootNBINotifications");
        if (cfg != NULL)
        {
                char const* value = coremw_xmlnode_contents(cfg);
                if(value && strcasecmp(value , "true") == 0){
                        non_root_mo_notifications = true;
                        DEBUG_OAMSA("enableNonrootNBINotifications=%d", non_root_mo_notifications);
                }
        }

	cfg = coremw_find_config_item(oam_component_name, "cmNotificationCache");
	if (cfg != NULL)
	{
		char const* maxsize = coremw_xmlnode_get_attribute(cfg, "maxsize");
		if (maxsize != NULL)
		{
			uint64_t scale = 1;
			const char* megaByte = "M";
			const char* kiloByte = "K";
			const char* gigaByte = "G";
			char* output = NULL;
			char* parsedValue = (char*)calloc(1, sizeof(maxsize));
			if((output = strstr(maxsize, kiloByte)) != NULL) {
				scale = 1024;
				strncpy(parsedValue, maxsize, strlen(maxsize)-1);
			} else if((output = strstr(maxsize, megaByte)) != NULL) {
				scale = 1024*1024;
				strncpy(parsedValue, maxsize, strlen(maxsize)-1);
			} else if((output = strstr(maxsize, gigaByte)) != NULL) {
				scale = 1024*1024*1024;
				strncpy(parsedValue, maxsize, strlen(maxsize)-1);
			} else {
				parseNumber(maxsize, scale);
			}
			if(strlen(parsedValue) > 0) {
				parseNumber(parsedValue, scale);
			}
			if(parsedValue != NULL) {
				free(parsedValue);
			}
		}
		DEBUG_OAMSA("maxSAMemoryForCMEvents=%llu", maxSAMemoryForCMEvents);
	}
	load_uuid_configuration();

	deparray[1] = &TransId_v2;
	ifarray[0] = (MafMgmtSpiInterface_1T*) ExportOamSATransactionalResourceInterface_V2();
	ifarray[0]->componentName = oam_component_name;
	ifarray[1] = (MafMgmtSpiInterface_1T*) ExportOamManagedObjectInterface();
	ifarray[1]->componentName = oam_component_name;
	ifarray[2] = (MafMgmtSpiInterface_1T*) ExportOamSARegisterObjectImplementerInterface_2();
	ifarray[2]->componentName = oam_component_name;
	ifarray[3] = (MafMgmtSpiInterface_1T*) ExportOamSARegisterObjectImplementerInterface();
	ifarray[3]->componentName = oam_component_name;
	ifarray[4] = NULL;
	LEAVE_OAMSA();
	return portal_MAF->registerComponent(&maf_mw_oam);
}


void maf_comSAOamComponentFinalize(MafMgmtSpiInterfacePortal_3T* portal_MAF)
{
	ENTER_OAMSA();
	DEBUG_OAMSA("maf_comSAOamComponentFinalize called...");
	portal_MAF->unregisterComponent(&maf_mw_oam);
	LEAVE_OAMSA();
}
