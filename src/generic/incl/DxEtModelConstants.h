#ifndef __DXETMODELCONSTANTS_H
#define __DXETMODELCONSTANTS_H
/********************************************************************************************
 *   Copyright (C) 2010 by Ericsson AB
 *   S - 125 26  STOCKHOLM
 *   SWEDEN, tel int + 46 10 719 0000
 *
 *   The copyright to the computer program herein is the property of
 *   Ericsson AB. The program may be used and/or copied only with the
 *   written permission from Ericsson AB, or in accordance with the terms
 *   and conditions stipulated in the agreement/contract under which the
 *   program has been supplied.
 *
 *   All rights reserved.
 *
 *
 *   File:   DxEtModelConstants.h
 *
 *   Author: xnikvap
 *
 *   Date:   2013-10-25
 *
 *   This file declares hardcoded names used in model files.
 *   These names are hardcoded in the DX ET Tool and appear in generated XML model files.
 *   COM knows these names by hardcoding and/or reading the XML model files.
 *   COMSA needs to know these names to be able to retrieve domain extension values by
 *   making calls via MAF MR SPI Ver.3. The MOM prefix names are also defined in this file
 *   The initial version is created from a list provided in e-mail from the DX Tool team.
 *
 *   Reviewed: 
 *   Modified:   
 *
 *****************************************************************************************/

/**
 * Domain names and Domain Estension names
 */

/**
 * Domain names and domain extension names related to Mim:
 *
 *   <domainExtension domain="ECIM">
 *     <extension name="ecimMomName" value="EcimMomA"/>
 *     <extension name="ecimMomVersion" value="1"/>
 *     <extension name="ecimMomRelease" value="2"/>
 *     <extension name="immNamespace" value="MOM_NAME"/>
 *   </domainExtension>
 */
#define DOMAIN_NAME_ECIM              "ECIM"
#define DOMAIN_EXT_NAME_MOM_NAME      "ecimMomName"
#define DOMAIN_EXT_NAME_MOM_VER       "ecimMomVersion"
#define DOMAIN_EXT_NAME_MOM_REL       "ecimMomRelease"
#define DOMAIN_EXT_NAME_IMM_NAMESPACE "immNamespace"

/**
 * Domain names and domain extension names related to Attributes, Parameters and Struct members:
 *
 *      <domainExtension domain="ECIM">
 *        <extension name="isNillable" value="true"/>
 *      </domainExtension>
 *
 *    Note: This domain extension is to be replaced by the <isNillable/> tag. 
 *          Planned for ET 2.9 (end of year 2013), if COM is OK with it.
 */
#define DOMAIN_EXT_NAME_IS_NILLABLE "isNillable"

/**
 * Domain names and domain extension names related to Structs, Enumerations and Derived data types:
 *
 *    <domainExtension domain="ECIM">
 *      <extension name="originatingMimVersion" value="22"/>
 *      <extension name="originatingMimName" value="LibC"/>
 *      <extension name="originatingMimRelease" value="11"/>
 *    </domainExtension>
 */
#define DOMAIN_EXT_NAME_ORIG_MIM_VER  "originatingMimVersion"
#define DOMAIN_EXT_NAME_ORIG_MIM_NAME "originatingMimName"
#define DOMAIN_EXT_NAME_ORIG_MIM_REL  "originatingMimRelease"

/**
 * Domain names and domain extension names related to Struct members:
 *
 *      <domainExtension domain="ECIM">
 *          <extension name="isKey" value="true"/>
 *        </domainExtension>
 *
 *    Note: This domain extension is to be replaced by the <isKey/> tag. 
 *          Planned for ET 2.9 (end of year 2013), if COM is OK with it.
 */
#define DOMAIN_EXT_NAME_IS_KEY "isKey"

/**
 * Domain names and domain extension names related to Relationships (since missing on Containment):
 *
 *    <domainExtension domain="CoreMW">
 *        <extension name="splitImmDn" value="true"/>
 *      </domainExtension>
 *
 *    Note: Suggetsed by the DX Tool team that the the "CoreMW" name is the one that will be used but in
 *          the models used for split IMM DN.
 */
#define DOMAIN_NAME_COREMW        "CoreMW" /* in CM Action related models */
#define DOMAIN_NAME_IMM           "IMM"    /* actually seen in the current XML files generated with ET 2.8.0 */
#define DOMAIN_EXT_NAME_SPLIT_IMM_DN "splitImmDn"

/**
 * Domain names and domain extension names related to Actions:
 *
 *      <domainExtension domain="CoreMW">
 *          <extension name="admOpId" value="33"/>
 *        </domainExtension>
 */
#define DOMAIN_EXT_NAME_ADM_OP_ID "admOpId"

/**
 * Boolean values used for domain extensions in model XML files
 */
#define DOMAIN_EXT_VALUE_TRUE  "true"
#define DOMAIN_EXT_VALUE_FALSE "false"

/**
 * MOM name
 */
#define DN_VALUE_MOM_NAME "MOM_NAME"

/**
 * Other string constants
 *
 */
#define CONTAINMENT_SEPARATOR "_to_"


#endif /* __DXETMODELCONSTANTS_H */
