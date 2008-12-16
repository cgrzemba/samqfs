/*
 * dis_remote.c - Display remote SAM-FS devices
 *
 * Displays the contents of the device table entry for the
 * selected remote sam device.
 *
 */

/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at pkg/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at pkg/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#pragma ident "$Revision: 1.20 $"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "aml/device.h"
#include "sam/devnm.h"
#include "aml/remote.h"
#include "sam/nl_samfs.h"
#include "samu.h"


void
DisRemote()
{
	int fl, i, h_error;
	int af;
	equ_t eq;		/* Device equipment ordinal */
	char buffer[1024];
	struct hostent *hpp;
	dev_ent_t *dev;		/* Device entry */
	srvr_clnt_t  *sp;

	eq = DisRm;
	dev = (dev_ent_t *)SHM_ADDR(master_shm, Dev_Tbl->d_ent[eq]);
	if (dev->equ_type == DT_PSEUDO_SS)
		Mvprintw(0, 0, "%s eq: %d",
		    catgets(catfd, SET, 7307, "Remote server"), eq);
	else if (dev->equ_type == DT_PSEUDO_SC)
		Mvprintw(0, 0, "%s eq: %d",
		    catgets(catfd, SET, 7308, "Remote client"), eq);
	else {
		Mvprintw(0, 0, "%s eq: %d",
		    catgets(catfd, SET, 7309, "Unknown Remote device"), eq);
		return;
	}

	Mvprintw(0, 24, "addr: %.8x", (int)Dev_Tbl->d_ent[eq]);
	if (dev->dis_mes[DIS_MES_CRIT][0] != '\0') {
		Attron(A_BOLD);
		Mvprintw(ln++, 0, "%s: %s",
		    catgets(catfd, SET, 7310, "message"),
		    dev->dis_mes[DIS_MES_CRIT]);
		Attroff(A_BOLD);
	} else {
		Mvprintw(ln++, 0, "%s: %s",
		    catgets(catfd, SET, 7310, "message"),
		    dev->dis_mes[DIS_MES_NORM]);
	}

	ln++;
	Clrtoeol();
	if (dev->equ_type == DT_PSEUDO_SS) {
	int last_line = ln;

	sp = (srvr_clnt_t *)SHM_ADDR(master_shm, dev->dt.ss.clients);
	for (i = 0; i < RMT_SAM_MAX_CLIENTS; i++, sp++) {
	boolean_t found = B_FALSE;

	af = sp->flags & SRVR_CLNT_IPV6 ? AF_INET6 : AF_INET;
	fl = ln;			  /* first line of data  */

	if (sp->control_addr6._S6_un._S6_u32[0] != 0 ||
	    sp->control_addr6._S6_un._S6_u32[3] != 0) {
		if (hpp = getipnodebyaddr((char *)&sp->control_addr6,
		    sizeof (sp->control_addr6), af, &h_error)) {
			Mvprintw(ln, 0, "%s%d: %s %s",
			    catgets(catfd, SET, 7327, "Client IPv"),
			    af == AF_INET6 ? 6 : 4,
			    hpp->h_name,
			    inet_ntop(af, sp->control_addr6.s6_addr,
			    buffer, 1024));
			Mvprintw(ln++, 65, "%s - %d",
			    catgets(catfd, SET, 7349, "port"),
			    dev->dt.ss.serv_port);
			freehostent(hpp);
			found = B_TRUE;
		} else {
			Mvprintw(ln++, 0, "%s%d: %s %s",
			    catgets(catfd, SET, 7327, "Client IPv"),
			    af == AF_INET6 ? 6 : 4,
			    catgets(catfd, SET, 7312, "unknown"),
			    inet_ntop(af, sp->control_addr6.s6_addr,
			    buffer, 1024));
			ln++;
		}
	}
	if (!found) continue;
	last_line = ln;
	ln = fl + 1;
	Mvprintw(ln++, 10, "%s - %d  %s - %d %s - %.4x  %s",
	    catgets(catfd, SET, 7321, "client index"), sp->index,
	    catgets(catfd, SET, 7349, "port"), sp->port,
	    catgets(catfd, SET, 7323, "flags"), sp->flags,
	    (sp->flags & SRVR_CLNT_CONNECTED) ?
	    catgets(catfd, SET, 7325, "connected") : "");

	if (ln < last_line)
		ln = last_line;

	ln++;
	}
	} else if (dev->equ_type == DT_PSEUDO_SC) {

		sp = (srvr_clnt_t *)SHM_ADDR(master_shm, dev->dt.sc.server);
		af = sp->flags & SRVR_CLNT_IPV6 ? AF_INET6 : AF_INET;

		if (sp->control_addr6._S6_un._S6_u32[0] != 0 ||
		    sp->control_addr6._S6_un._S6_u32[3] != 0) {
			if (hpp = getipnodebyaddr((char *)&sp->control_addr6,
			    sizeof (sp->control_addr6), af, &h_error)) {
				Mvprintw(ln, 0, "%s%d: %s %s",
				    catgets(catfd, SET, 7411, "Server IPv"),
				    af == AF_INET6 ? 6 : 4,
				    hpp->h_name,
				    inet_ntop(af, sp->control_addr6.s6_addr,
				    buffer, 1024));
				Mvprintw(ln++, 65, "%s - %d",
				    catgets(catfd, SET, 7349, "port"),
				    sp->port);
				freehostent(hpp);
			} else {
				Mvprintw(ln++, 0, "%s: %s %s",
				    catgets(catfd, SET, 7410, "Server"),
				    catgets(catfd, SET, 7312, "unknown"),
				    inet_ntop(af, sp->control_addr6.s6_addr,
				    buffer, 1024));
			}
		}
	}
}


/*
 * Display initialization.
 */
boolean
InitRemote(void)
{
	short n = 0;
	short starting = 0;
	dev_ent_t  *dev = NULL;

	if (Argc > 1) {
	DisRm = findDev(Argv[1]);
	dev = (dev_ent_t *)SHM_ADDR(master_shm, Dev_Tbl->d_ent[DisRm]);
	n = DisRm;
	}

	if (n != 0)
	starting = n-1;

	for (; n <= Max_Devices; n++) {
	if (dev == NULL) {
		if (Dev_Tbl->d_ent[n] == NULL)  continue;
		dev = (dev_ent_t *)SHM_ADDR(master_shm, Dev_Tbl->d_ent[n]);
	}

	if (dev->equ_type == DT_PSEUDO_SS || dev->equ_type == DT_PSEUDO_SC)
		return (0);
	}

	if (starting != 0) {
	for (n = 0; n < starting; n++) {
		if (dev == NULL) {
			if (Dev_Tbl->d_ent[n] == NULL)  continue;
			dev = (dev_ent_t *)SHM_ADDR(master_shm,
			    Dev_Tbl->d_ent[n]);
		}

		if (dev->equ_type == DT_PSEUDO_SS ||
		    dev->equ_type == DT_PSEUDO_SC)
			return (0);
	}
	}

	return (1);
}


/*
 * Keyboard processing.
 */
boolean
KeyRemote(char key)
{
	short n;
	dev_ent_t *dev;			/* Device entry */

	switch (key) {

	case KEY_full_fwd:
	n = DisRm;
	while (++n != DisRm) {
		if (n > Max_Devices)  n = 0;
		if (Dev_Tbl->d_ent[n] != NULL) {
		dev = (dev_ent_t *)SHM_ADDR(master_shm, Dev_Tbl->d_ent[n]);
		if (dev->equ_type == DT_PSEUDO_SS ||
		    dev->equ_type == DT_PSEUDO_SC)
		break;
		}
	}
	DisRm = n;
	break;

	case KEY_full_bwd:
	n = DisRm;
	while (--n != DisRm) {
	if (n < 0)  n = Max_Devices - 1;
	if (Dev_Tbl->d_ent[n] != NULL) {
		dev = (dev_ent_t *)SHM_ADDR(master_shm, Dev_Tbl->d_ent[n]);
		if (dev->equ_type == DT_PSEUDO_SS ||
		    dev->equ_type == DT_PSEUDO_SC)
		break;
	}
	}
	DisRm = n;
	break;

	default:
	return (FALSE);

	}
	return (TRUE);
}
