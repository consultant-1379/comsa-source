#ifndef _OAMSA_TRANSLATEIMMNAME_H
#define _OAMSA_TRANSLATEIMMNAME_H

/**
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
 *   File:   OamSATranslateImmName.h
 *
 *   Author: egorped
 *
 *   Date:   2010-05-21
 *
 *   This file declares a since C-style function for use outside OamSACache object
 *   structure.
 *
 *   Reviewed: efaiami 2010-08-17
 *
 *   Modify: xjonbuc 2012-09-06  Implement SDP1694 - support MAF SPI
 *
 */

#include "saAis.h"
#include "MafOamSpiManagedObject_3.h"

#ifdef  __cplusplus
extern "C" {
#endif

/**
 *  Translates a IMM name string to a 3GPP incarnation.
 */
char* convertTo3Gpp(SaNameT*	theImmName_p);
char* convertTo3GPPDNCOMCASE(char *immRdn);
bool  isNotified(char* attributename, char *classname, char* immRdn);
bool  isStructAttribute(char *immRdn, char *saAttributeName);
char *getStructAttributeName(char *immRdn, char *saAttributeName);
bool  fillStructMembers(MafMoAttributeValueContainer_3T **structAttributePointer, char *immRdn, char *saAttributeName);
bool  removeStructPartOfDn(char **out, const char **dn);
bool  getClassName(char* immRdn, char** className);
MafOamSpiMoAttributeType_3T  getTypeForAttribute(char* attributename, char* immRdn);
MafOamSpiMoAttributeType_3T  getTypeForStructMemberAttribute(char *attributename, char* membername, char *immRdn);

/**
 *  Translates 3GPP DN to IMM DN
 */
bool convertToImm(const char *the3GppDN, SaNameT *immDn);

/*
 * Converts 3GPP DN to IMM DN.
 * This function however is applicable only for a special case.
 *  - When the 3GPP DN contains 'licenseId'(hard-coded) by LM.
 *
 * @param[IN]  the3gppDn the 3GPP format DN.
 * @param[OUT] size      the size of the IMM DN after conversion.
 *
 * return  the IMM DN string after successful conversion.
 *         NULL if conversion fails.
 *
 * TODO: prefer returning 'unisgned' to 'const char*'
 */
int removeMOMPrefix(const char *immDn, unsigned *size, char *prefixImmDN);

#ifdef  __cplusplus
}
#endif
#endif
