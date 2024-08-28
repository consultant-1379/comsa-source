/*
 * ComSANtf_unittest.h
 *
 *  Created on: Apr 30, 2013
 *      Author: eaparob
 *
 */

#ifndef CMCACHE_UNITTEST_H_
#define CMCACHE_UNITTEST_H_

#include <list>
#include <map>
#include <string>
#include <iostream>
#include <assert.h>
#include <gtest/gtest.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <saAis.h>
#include <saImmOi.h>
#include "MafOamSpiCmEvent_1.h"
#include "CmEventHandler.h"

static bool notificationCacheInDiscardMode_unittest = false;
MafReturnT push_CmCacheUnittest(MafOamSpiCmEvent_Notification_1T *mafCmNot);

#endif
