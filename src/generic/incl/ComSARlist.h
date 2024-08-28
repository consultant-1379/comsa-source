#ifndef COMSARLIST_H
#define COMSARLIST_H
/*******************************************************************************
* Copyright (C) 2015 by Ericsson AB
* S - 125 26  STOCKHOLM
* SWEDEN, tel int + 46 10 719 0000
*
* The copyright to the computer program herein is the property of
* Ericsson AB. The program may be used and/or copied only with the
* written permission from Ericsson AB, or in accordance with the terms
* and conditions stipulated in the agreement/contract under which the
* program has been supplied..
*
* All rights reserved.
*
* Author: xanhdao
*
* Date:   2015-03-02
*
* This file declares the required functions for implementing the replicated list
*
* Modified: xnikvap 2015-08-02  MR42277 Remove old interfaces exposed to COM & CoreMW that are no longer supported for SLES12*
*******************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <trace.h>
#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>

#include <ComSA.h>
#include <MafMwSpiReplicatedList_1.h>
#include <MafMwSpiServiceIdentities_1.h>

struct RLIterator {
	unsigned int id;
	unsigned int index;
	unsigned long size;
	void* data;
};
/* The item name must hold the ascii representation of a ULLONG_MAX,
 * i.e. "18446744073709551615". */
#define MAX_ITEM_NAME_LEN 20
// OS shared memory segment
#define shmmaxFile "/proc/sys/kernel/shmmax"

/* Forward declarations; */
extern unsigned long long rlist_maxsize;
extern bool clearAlarmsOnClusterReboot;

#endif /* not defined COMSARLIST_H */
