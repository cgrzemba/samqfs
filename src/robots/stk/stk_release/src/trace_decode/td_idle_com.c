#ifndef lint
static char SccsId[] = "@(#)td_idle_com.c	2.0 1/10/94 ";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_idle_command
 *
 * Description:
 *      Decode the message content of a idle request/response packet.
 *
 * Return Values:
 *      NONE
 *
 * Implicit Inputs:
 *      NONE
 *
 * Implicit Outputs:
 *      NONE
 *
 * Considerations:
 *      NONE
 *
 * Revision History:
 *
 *      M. H. Shum          10-Sep-1993     Original.
 *
 */

#include "td.h"

void
td_idle_command(VERSION version)
{
    printf("Idle command has no command specific portion\n");
}


