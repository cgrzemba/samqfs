/*
 * command.c - command dispatcher
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
 * or https://illumos.org/license/CDDL.
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

#pragma ident "$Revision: 1.90 $"

/* Using __FILE__ makes duplicate strings */
static char    *_SrcFile = __FILE__;


/* ANSI C headers. */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/mman.h>

/* POSIX headers. */
#include <fcntl.h>
#include <unistd.h>

/* SAM-FS headers. */
#include "aml/archiver.h"
#include "aml/stager.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "aml/device.h"
#define	NEED_DL_NAMES
#include "aml/dev_log.h"
#undef NEED_DL_NAMES
#include "aml/fifo.h"
#include "aml/message.h"
#define	TRACE_CONTROL
#include "sam/sam_trace.h"
#include "aml/samapi.h"
#include "sam/uioctl.h"
#include "sam/syscall.h"
#include "sam/nl_samfs.h"
#include "aml/stager.h"
#include "aml/diskvols.h"

/* Local headers. */
#include "samu.h"


/* Private functions. */
static void	CmdAridle(void);
static void	CmdArrerun(void);
static void	CmdArrestart(void);
static void	CmdArrmarchreq(void);
static void	CmdArrun(void);
static void	CmdArscan(void);
static void	CmdArstop(void);
static void	CmdArtrace(void);
static void	CmdAudit(void);
static void	CmdClearvsn(void);
static void	CmdDevlog(void);
static void	CmdDtrace(void);
static void	CmdExit(void);
static void	CmdExport(void);
static void	CmdImport(void);
static void	CmdLoad(void);
static void	CmdSetFsConfig(void);
static void	CmdSetFsParam(void);
static void	CmdSetFsDskCmd(void);
static void	CmdStidle(void);
static void	CmdStrun(void);
void		CmdMount(void);
static void	CmdFs(void);
static void	CmdOpen(void);
static void	CmdPriority(void);
static void	CmdRead(void);
static void	CmdRefresh(void);
static void	CmdSetstate(void);
static void	CmdThresh(void);
static void	CmdSnap(void);
static void	CmdUnload(void);
static void	CmdZap(void);
static void	CmdStclear(void);
static void	CmdDiskvols(void);
static void	MsgFunc(int code, char *msg);
static void	Usage(void);
static void	daemonSetExec(char *name, char *ctrl);
static int	find_preview(char *s);
static void	issue_fifo_cmd(int cmd);
static boolean_t runCmd(int IsSamcmd);

/* Private data. */
static struct {
	char	*name;		/* Command name */
	sam_level sam_lvl;	/* lowest level that this command is valid */
	int	samcmd;		/* 1 if valid from samcmd, 0 if disallowed */
	void	(*cmd) (void);	/* Command processor */
	int	par;		/* Parameter */
	int	usage;		/* msg # of parameter string for help display */
	char	*usage_msg;	/* default parameter string for help display */
	int	help;		/* msg # of explanation for help display */
	char	*help_msg;	/* default explanation for help display */

}cmd_t[] = {
	{
		"", QFS_STANDALONE, 0, NULL, 0, 0, "",
		7105, "File System commands - miscellaneous:"
	},
	{
		"stripe", QFS_STANDALONE, 1, CmdSetFsParam, 0, 7050,
		"eq value", 7065, "Set stripe width"
	},
	{
		"suid", QFS_STANDALONE, 1, CmdSetFsConfig, 0, 7110,
		"eq", 7061, "Turn on setuid capability"
	},
	{
		"nosuid", QFS_STANDALONE, 1, CmdSetFsConfig, 0, 7110,
		"eq", 7062, "Turn off setuid capability"
	},
	{
		"sync_meta", QFS_STANDALONE, 1, CmdSetFsParam, 0, 7050,
		"eq value", 7066, "Set sync_meta mode"
	},
	{
		"atime", QFS_STANDALONE, 1, CmdSetFsParam, 0, 7050,
		"eq value", 7188, "Set access time (atime) update mode"
	},
	{
		"trace", QFS_STANDALONE, 1, CmdSetFsConfig, 0, 7110,
		"eq", 7166, "Turn on file system tracing"
	},
	{
		"notrace", QFS_STANDALONE, 1, CmdSetFsConfig, 0, 7110,
		"eq", 7165, "Turn off file system tracing"
	},
	{
		"add", QFS_STANDALONE, 1, CmdSetFsDskCmd, 0, 7110,
		"eq", 7191, "Add eq to mounted file system"
	},
	{
		"remove", QFS_STANDALONE, 1, CmdSetFsDskCmd, 0,	7110,
		"eq", 7192, "Remove eq; copy files to ON eqs"
	},
	{
		"release", SAM_DISK, 1, CmdSetFsDskCmd, 0, 7110,
		"eq", 7193, "Release eq and mark files offline"
	},
	{
		"alloc", QFS_STANDALONE, 1, CmdSetFsDskCmd, 0, 7110,
		"eq", 7190, "Enable allocation on partition"
	},
	{
		"noalloc", QFS_STANDALONE, 1, CmdSetFsDskCmd, 0, 7110,
		"eq", 7189, "Disable allocation on partition"
	},
	{
		"def_retention", QFS_STANDALONE, 1, CmdSetFsParam, 0, 7107,
		"eq interval", 7186, "Set default WORM retention time"
	},
	{
		"ci", QFS_STANDALONE, 1, CmdSetFsConfig, 0, 7110,
		"eq", 7194, "Set filename casesensitivity off"
	},
	{
		"noci", QFS_STANDALONE, 1, CmdSetFsConfig, 0, 7110,
		"eq", 7195, "Set filename casesensitivity on (default)"
	},
	{
		"", QFS_STANDALONE, 0, NULL, 0, 0, "",
		7051, "File System commands - SAM:"
	},
	{
		"hwm_archive", SAM_DISK, 1, CmdSetFsConfig, 0, 7110,
		"eq", 7068, "Turn on hwm archiver start"
	},
	{
		"nohwm_archive", SAM_DISK, 1, CmdSetFsConfig, 0, 7110,
		"eq", 7069, "Turn off hwm archiver start"
	},
	{
		"maxpartial", SAM_DISK, 1, CmdSetFsParam, 0, 7050,
		"eq value", 7070, "Set maximum partial size in kilobytes"
	},
	{
		"partial", SAM_DISK, 1, CmdSetFsParam, 0, 7050,
		"eq value", 7134, "Set size to remain online in kilobytes"
	},
	{
		"partial_stage", SAM_DISK, 1, CmdSetFsParam, 0, 7050,
		"eq value", 7071, "Set partial stage-ahead point in kilobytes"
	},
	{
		"stage_flush_behind", SAM_DISK, 1, CmdSetFsParam, 0, 7050,
		"eq value", 7072, "Set stage flush behind size in kilobytes"
	},
	{
		"stage_n_window", SAM_DISK, 1, CmdSetFsParam, 0, 7050,
		"eq value", 7073, "Set direct stage size in kilobytes"
	},
	{
		"stage_retries", SAM_DISK, 1, CmdSetFsParam, 0, 7050,
		"eq value", 7074, "Set number of stage retries"
	},
	{
		"thresh", SAM_DISK, 1, CmdThresh, 0, 7114, "eq high low",
		7138, "Set high and low thresholds"
	},
	{
		"", QFS_STANDALONE, 0, NULL, 0, 0, "",
		7052, "File System commands - I/O:"
	},
	{
		"dio_rd_consec", QFS_STANDALONE, 1, CmdSetFsParam, 0, 7050,
		"eq value", 7053, "Set number of consecutive dio reads"
	},
	{
		"dio_rd_form_min", QFS_STANDALONE, 1, CmdSetFsParam, 0, 7050,
		"eq value", 7054, "Set size of well-formed dio reads"
	},
	{
		"dio_rd_ill_min", QFS_STANDALONE, 1, CmdSetFsParam, 0, 7050,
		"eq value", 7055, "Set size of ill-formed dio reads"
	},
	{
		"dio_wr_consec", QFS_STANDALONE, 1, CmdSetFsParam, 0, 7050,
		"eq value", 7056, "Set number of consecutive dio writes"
	},
	{
		"dio_wr_form_min", QFS_STANDALONE, 1, CmdSetFsParam, 0, 7050,
		"eq value", 7057, "Set size of well-formed dio writes"
	},
	{
		"dio_wr_ill_min", QFS_STANDALONE, 1, CmdSetFsParam, 0, 7050,
		"eq value", 7058, "Set size of ill-formed dio writes"
	},
	{
		"flush_behind", QFS_STANDALONE, 1, CmdSetFsParam, 0, 7050,
		"eq value", 7067, "Set flush behind value in kilobytes"
	},
	{
		"forcedirectio", QFS_STANDALONE, 1, CmdSetFsConfig, 0, 7110,
		"eq", 7059, "Turn on directio mode"
	},
	{
		"noforcedirectio", QFS_STANDALONE, 1, CmdSetFsConfig, 0, 7110,
		"eq", 7060, "Turn off directio mode"
	},
	{
		"force_nfs_async", QFS_STANDALONE, 1, CmdSetFsConfig, 0, 7110,
		"eq", 7087, "Turn on NFS async"
	},
	{
		"noforce_nfs_async", QFS_STANDALONE, 1, CmdSetFsConfig, 0, 7110,
		"eq", 7088, "Turn off NFS async"
	},
	{
		"readahead", QFS_STANDALONE, 1, CmdSetFsParam, 0, 7050,
		"eq value", 7157, "Set maximum readahead in kilobytes"
	},
	{
		"writebehind", QFS_STANDALONE, 1, CmdSetFsParam, 0, 7050,
		"eq value", 7158, "Set maximum writebehind in kilobytes"
	},
	{
		"sw_raid", QFS_STANDALONE, 1, CmdSetFsConfig, 0, 7110,
		"eq", 7063, "Turn on software RAID mode"
	},
	{
		"nosw_raid", QFS_STANDALONE, 1, CmdSetFsConfig, 0, 7110,
		"eq", 7064, "Turn off software RAID mode"
	},
	{
		"wr_throttle", QFS_STANDALONE, 1, CmdSetFsParam, 0, 7050,
		"eq value", 7075, "Set outstanding write size in kilobytes"
	},
	{
		"abr", QFS_STANDALONE, 1, CmdSetFsConfig, 0, 7110,
		"eq", 7094, "Enable Application Based Recovery"
	},
	{
		"noabr", QFS_STANDALONE, 1, CmdSetFsConfig, 0, 7110,
		"eq", 7095, "Disable Application Based Recovery"
	},
	{
		"dmr", QFS_STANDALONE, 1, CmdSetFsConfig, 0, 7110,
		"eq", 7096, "Enable Directed Mirror Reads"
	},
	{
		"nodmr", QFS_STANDALONE, 1, CmdSetFsConfig, 0, 7110,
		"eq", 7097, "Disable Directed Mirror Reads"
	},
	{
		"dio_szero", QFS_STANDALONE, 1, CmdSetFsConfig, 0, 7110,
		"eq", 7098, "Turn on dio sparse zeroing"
	},
	{
		"nodio_szero", QFS_STANDALONE, 1, CmdSetFsConfig, 0, 7110,
		"eq", 7099, "Turn off dio sparse zeroing"
	},
	{
		"", QFS_STANDALONE, 0, NULL, 0, 0, "",
		7091, "File System commands - QFS:"
	},
	{
		"mm_stripe", QFS_STANDALONE, 1, CmdSetFsParam, 0, 7050,
		"eq value", 7081, "Set meta stripe width"
	},
	{
		"qwrite", QFS_STANDALONE, 1, CmdSetFsConfig, 0, 7110,
		"eq", 7082, "Turn on qwrite mode"
	},
	{
		"noqwrite", QFS_STANDALONE, 1, CmdSetFsConfig, 0, 7110,
		"eq", 7083, "Turn off qwrite mode"
	},
	{
		"", QFS_STANDALONE, 0, NULL, 0, 0, "",
		7092, "File System commands - multireader:"
	},
	{
		"invalid", QFS_STANDALONE, 1, CmdSetFsParam, 0, 7050,
		"eq interval", 7086, "Set multireader invalidate cache delay"
	},
	{
		"refresh_at_eof", QFS_STANDALONE, 1, CmdSetFsConfig, 0, 7110,
		"eq", 7089, "Turn on refresh at eof mode"
	},
	{
		"norefresh_at_eof", QFS_STANDALONE, 1, CmdSetFsConfig, 0, 7110,
		"eq", 7090, "Turn off refresh at eof mode"
	},
	{
		"", QFS_STANDALONE, 0, NULL, 0, 0, "",
		7093, "File System commands - shared fs:"
	},
	{
		"minallocsz", QFS_STANDALONE, 1, CmdSetFsParam, 0, 7050,
		"eq value", 7079, "Set minimum allocation size"
	},
	{
		"maxallocsz", QFS_STANDALONE, 1, CmdSetFsParam, 0, 7050,
		"eq value", 7080, "Set maximum allocation size"
	},
	{
		"meta_timeo", QFS_STANDALONE, 1, CmdSetFsParam, 0, 7107,
		"eq interval", 7161, "Set shared fs meta cache timeout"
	},
	{
		"lease_timeo", QFS_STANDALONE, 1, CmdSetFsParam, 0, 7107,
		"eq interval", 7185, "Set shared fs lease relinquish timeout"
	},
	{
		"min_pool", QFS_STANDALONE, 1, CmdSetFsParam, 0, 7050,
		"eq value", 7187, "Set shared fs minimum threads count"
	},
	{
		"mh_write", QFS_STANDALONE, 1, CmdSetFsConfig, 0, 7110,
		"eq", 7077, "Turn on multihost read/write"
	},
	{
		"nomh_write", QFS_STANDALONE, 1, CmdSetFsConfig, 0, 7110,
		"eq", 7078, "Turn off multihost read/write"
	},
	{
		"aplease", QFS_STANDALONE, 1, CmdSetFsParam, 0, 7107,
		"eq interval", 7076, "Set append lease time"
	},
	{
		"rdlease", QFS_STANDALONE, 1, CmdSetFsParam, 0, 7107,
		"eq interval", 7084, "Set read lease time"
	},
	{
		"wrlease", QFS_STANDALONE, 1, CmdSetFsParam, 0, 7107,
		"eq interval", 7085, "Set write lease time"
	},
	{
		"", SAM_REMOVABLE, 0, NULL, 0, 0, "", 7100,
		"Device commands:"
	},
	{
		"devlog", SAM_REMOVABLE, 1, CmdDevlog, 0, 7111,
		"eq [option ...]", 7130, " Set device logging options"
	},
	{
		"idle", SAM_REMOVABLE, 1, CmdSetstate, DEV_IDLE, 7110,
		"eq", 7132, "Idle device"
	},
	{
		"off", SAM_REMOVABLE, 1, CmdSetstate, DEV_OFF, 7110,
		"eq", 7135, "Turn off device"
	},
	{
		"on", SAM_REMOVABLE, 1, CmdSetstate, DEV_ON, 7110,
		"eq", 7136, "Turn on device"
	},
	{
		"readonly", SAM_REMOVABLE, 1, CmdSetstate, DEV_RO, 7110,
		"eq", 7137, "Make device read only"
	},
	{
		"ro", SAM_REMOVABLE, 1, CmdSetstate, DEV_RO, 7110,
		"eq", 7137, "Make device read only"
	},
	{
		"unavail", SAM_REMOVABLE, 1, CmdSetstate, DEV_UNAVAIL, 7110,
		"eq", 7139, "Make device unavailable"
	},
	{
		"unload", SAM_REMOVABLE, 1, CmdUnload, 0, 7110,
		"eq", 7140, "Unload device"
	},
	{
		"", SAM_REMOVABLE, 0, NULL, 0, 0, "", 7101, "Robot commands:"
	},
	{
		"audit", SAM_REMOVABLE, 1, CmdAudit, 0, 7108,
		"[-e] eq[:slot[:side]]", 7141, "Audit slot or library"
	},
	{
		"import", SAM_REMOVABLE, 1, CmdImport, 0, 7110,
		"eq", 7142, "Import cartridge from mailbox"
	},
	{
		"export", SAM_REMOVABLE, 1, CmdExport, FALSE, 7116,
		"[-f] eq:slot",	7143, "Export cartridge to mailbox"
	},
	{
		"export", SAM_REMOVABLE, 1, CmdExport, FALSE, 7115,
		"[-f] mt.vsn", 7143, "Export cartridge to mailbox"
	},
	{
		"load", SAM_REMOVABLE, 1, CmdLoad, TRUE, 7109,
		"eq:slot[:side]", 7144, "Load cartridge in drive"
	},
	{
		"load", SAM_REMOVABLE, 1, CmdLoad, TRUE, 7115,
		"mt.vsn", 7144, "Load cartridge in drive"
	},
	{
		"priority", SAM_REMOVABLE, 1, CmdPriority, 0, 7179,
		"pid priority",	7180, "Set priority in preview queue"
	},
	{
		"", SAM_DISK, 0, NULL, 0, 0, "", 7102, "Archiver commands:"
	},
	{
		"aridle", SAM_DISK, 1, CmdAridle, 0, 7162,
		"[dk | rm | fs.fsname]", 7163, "Idle archiving"
	},
	{
		"arrerun", SAM_DISK, 1, CmdArrerun, 0, 7123,
		"", 7174, "Soft restart archiver"
	},
	{
		"arrestart", SAM_DISK, 1, CmdArrestart, 0, 7123,
		"", 7145, "Restart archiver"
	},
	{
		"arrmarchreq", SAM_DISK, 1, CmdArrmarchreq, 0, 7177,
		"fsname.[* | arname]", 7178, "Remove ArchReq(s)"
	},
	{
		"arrun", SAM_DISK, 1, CmdArrun, 0, 7162,
		"[dk | rm | fs.fsname]", 7146, "Start archiving"
	},
	{
		"arscan", SAM_DISK, 1, CmdArscan, 0, 7172,
		"fsname[.dir|..inodes][int]", 7173, "Scan filesystem"
	},
	{
		"arstop", SAM_DISK, 1, CmdArstop, 0, 7162,
		"[dk | rm | fs.fsname]", 7164, "Stop archiving"
	},
	{
		"artrace", SAM_DISK, 1, CmdArtrace, 0, 7170,
		"[fs.fsname]", 7171, "Trace archiver"
	},
	{
		"", SAM_DISK, 0, NULL, 0, 0, "", 7106, "Stager commands:"
	},
	{
		"stclear", SAM_DISK, 1, CmdStclear, 0, 7168,
		"mt.vsn", 7169, "Clear stage request"
	},
	{
		"stidle", SAM_DISK, 1, CmdStidle, 0, 0, "",
		7175, "Idle staging"
	},
	{
		"strun", SAM_DISK, 1, CmdStrun, 0, 0, "",
		7176, "Start staging"
	},
	{
		"", QFS_STANDALONE, 0, NULL, 0, 0, "", 7103,
		"Miscellaneous commands:"
	},
	{
		"clear", SAM_REMOVABLE, 1, CmdClearvsn, 0, 7118, "vsn [index]",
		7148, "Clear load request"
	},
	{
		"dtrace", QFS_STANDALONE, 1, CmdDtrace, 0, 7167,
		"daemon[.variable] value", 7229, "Daemon trace controls"
	},
	{
		"fs", QFS_STANDALONE, 0, CmdFs, 0, 7159,
		"filesystem", 7160, "Select a filesystem name (ex samfs1)"
	},
	{
		"mount", QFS_STANDALONE, 0, CmdMount, 0, 7119,
		"mntpt", 7149, "Select a mount point"
	},
	{
		"open", QFS_STANDALONE, 0, CmdOpen, 0, 7110,
		"eq", 7150, "Open device"
	},
	{
		"q", QFS_STANDALONE, 0, CmdExit, 0, 7123,
		"", 7151, "Exit from samu"
	},
	{
		"refresh", QFS_STANDALONE, 0, CmdRefresh, 0, 7120,
		"[interval]", 7152, "Set display refresh interval"
	},
	{
		"read", QFS_STANDALONE, 0, CmdRead, 0, 7121,
		"address", 7153, "Read sector from device"
	},
	{
		"snap", QFS_STANDALONE, 0, CmdSnap, 0, 7122,
		"[filename]", 7154, "Snapshot screen to file"
	},
	{
		"diskvols", SAM_DISK, 1, CmdDiskvols, 0, 7183,
		"volume +flag | -flag",	7184, "Set or clear disk volume flags"
	},
};
static int cmd_n;
static sam_cmd_fifo_t cmd_block; /* FIFO command block */


/*
 * Command processor.
 */
void
command(
	char *cmd)	/* Command string */
{
	char *s;

	s = cmd;
	for (Argc = 0; Argc < numof(Argv) - 1; Argc++) {
		while (*s != '\0' && isspace(*s))
			s++;
		if (*s == '\0')
			break;
		Argv[Argc] = s;
		while (*s != '\0' && !isspace(*s))
			s++;
		if (*s != '\0')
			*s++ = '\0';
	}
	Argv[Argc] = NULL;

	move(LINES - 1, 0);	/* Clear type-in area */
	clrtoeol();

	if (Argc == 0)
		return;
	if (runCmd(0))
		return;

	if (strlen(Argv[0]) == 1 && SetDisplay(*Argv[0]) != -1)
		return;
	Error(catgets(catfd, SET, 1408, "Invalid command (%s)"), cmd);
}

boolean_t
runCmd(
	int IsSamcmd
)
{
	for (cmd_n = 0; cmd_n < numof(cmd_t); cmd_n++) {
		if (*cmd_t[cmd_n].name == '\0')
			continue;
		if (strcmp(Argv[0], cmd_t[cmd_n].name) != 0)
			continue;
		/*
		 * this is the right command, check if valid at this level.
		 */
		if (((cmd_t[cmd_n].sam_lvl == SAM_REMOVABLE) &&
		    !IsSamRunning) ||
		    ((cmd_t[cmd_n].sam_lvl == SAM_DISK) && !IsSam) ||
		    (cmd_t[cmd_n].samcmd == 0 && IsSamcmd == 1)) {
			Error(catgets(catfd, SET, 1408,
			    "Invalid command (%s)"), Argv[0]);
		}
		cmd_t[cmd_n].cmd();
		return (B_TRUE);
	}
	return (B_FALSE);
}


/*
 * samcmd processor.
 */
void
SamCmd(
	void)
{
	if (Argc == 0)
		return;
	if (runCmd(1))
		return;
	/*
	 * Didn't find it as a command, try it as a display.
	 */
	if (RunDisplay(Argv[0][0])) {
		Error(catgets(catfd, SET, 1408, "Invalid command (%s)"),
		    Argv[0]);
	}
}


/*
 * aridle
 */
static void
CmdAridle(
	void)
{
	daemonSetExec(SAM_ARCHIVER, "idle");
}


/*
 * arrerun
 */
static void
CmdArrerun(
	void)
{
	char	    msg[80];

	(void) ArchiverControl("rerun", NULL, msg, sizeof (msg));
	if (*msg != '\0') {
		Error(msg);
	}
}


/*
 * arrestart
 */
static void
CmdArrestart(
		void)
{
	char	    msg[80];

	(void) ArchiverControl("restart", NULL, msg, sizeof (msg));
	if (*msg != '\0') {
		Error(msg);
	}
}


/*
 * arrmarchreq
 */
static void
CmdArrmarchreq(
		void)
{
	char	    msg[80];
	upath_t	 ident;

	if (Argc != 2) {
		Usage();
	}
	snprintf(ident, sizeof (ident), "rmarchreq.%s", Argv[1]);
	(void) ArchiverControl(ident, NULL, msg, sizeof (msg));
	if (*msg != '\0') {
		Error(msg);
	}
}


/*
 * arrun
 */
static void
CmdArrun(
	void)
{
	daemonSetExec(SAM_ARCHIVER, "run");
}


/*
 * arscan
 */
static void
CmdArscan(
	void)
{
	char	    msg[80];
	upath_t	 ident;
	char	   *value;

	if (Argc < 2 || Argc > 3) {
		Usage();
	}
	snprintf(ident, sizeof (ident), "scan.%s", Argv[1]);
	if (Argc == 3) {
		value = Argv[2];
	} else {
		value = NULL;
	}
	(void) ArchiverControl(ident, value, msg, sizeof (msg));
	if (*msg != '\0') {
		Error(msg);
	}
}


/*
 * arstop
 */
static void
CmdArstop(
	void)
{
	daemonSetExec(SAM_ARCHIVER, "stop");
}


/*
 * artrace
 */
static void
CmdArtrace(
	void)
{
	char	    msg[80];
	char	   *value;

	if (Argc > 2) {
		Usage();
	}
	if (Argc == 2) {
		value = Argv[1];
	} else {
		value = NULL;
	}
	(void) ArchiverControl("trace", value, msg, sizeof (msg));
	if (*msg != '\0') {
		Error(msg);
	}
}


/*
 * audit eq
 */
static void
CmdAudit(
	void)
{
	/* Equipment ordinal */
	int eq = 0;
	/* slot to audit, -1 means whole library */
	uint_t  slot = ROBOT_NO_SLOT;
	/* if -e option */
	int eodflag = FALSE;
	/* Device type */
	dtype_t	 ty;
	/* Device entry */
	dev_ent_t *dev;
	struct VolId    vid;
	struct CatalogEntry ced;
	struct CatalogEntry *ce = &ced;


	if (Argc == 3) {
		if (strcmp(Argv[1], "-e") == 0) {
			eodflag = TRUE;
		} else {
			Usage();
		}
	} else if (Argc != 2) {
		Usage();
	}
	CatlibInit();
	/*
	 * Convert specifier to volume identifier.
	 */
	if (StrToVolId(Argv[Argc - 1], &vid) != 0) {
		Error(catgets(catfd, SET, 18207,
		    "Volume specification error %s:"),
		    Argv[Argc - 1]);
	/* NOTREACHED */
	} else if ((ce = CatalogCheckVolId(&vid, &ced)) == NULL) {
	/* Must be just  "audit [-e] eq"; disallow "-e" on entire robot */
		eq = findDev(Argv[Argc - 1]);
		eodflag = FALSE;
	} else {
		eq = vid.ViEq;
		slot = ce->CeSlot;
	}
	dev = (dev_ent_t *)SHM_ADDR(master_shm, Dev_Tbl->d_ent[eq]);
	ty = dev->type & DT_CLASS_MASK;

	if (ty != DT_ROBOT || dev->state > DEV_IDLE) {
		Error(catgets(catfd, SET, 571, "Cannot audit device (%s)."),
		    Argv[Argc - 1]);
	}
	memset(&cmd_block, 0, sizeof (cmd_block));
	cmd_block.slot = slot;
	cmd_block.eq = eq;
	if (eodflag)
		cmd_block.flags = CMD_AUDIT_EOD;
	/*
	 * Set the audit flag for a single slot
	 */
	if (slot != ROBOT_NO_SLOT) {
		(void) CatalogSetFieldByLoc(eq, ce->CeSlot, 0, CEF_Status,
		    CES_needs_audit, 0);
	}
	issue_fifo_cmd(CMD_FIFO_AUDIT);
}


/*
 * clear vsn [slot]
 */
static void
CmdClearvsn(
	    void)
{
	int		index = -1;
	int		count;

	if (Argc < 2)
		Usage();
	if (Argc > 2)
		index = strtol(Argv[2], NULL, 0);
	count = Preview_Tbl->ptbl_count;
	if (index >= 0) {
		if (index > count)
			Error(catgets(catfd, SET, 2358,
			    "Index %d not in preview."), index);
		if (strcmp(Preview_Tbl->p[index].resource.archive.vsn,
		    Argv[1])) {
			Error(catgets(catfd, SET, 2872,
			    "VSN and index inconsistent."));
		}
	} else {
		index = find_preview(Argv[1]);
		if (index < 0)
			Error(catgets(catfd, SET, 2884, "VSN not found."));
	}
	memset(&cmd_block, 0, sizeof (cmd_block));
	cmd_block.slot = index;
	issue_fifo_cmd(CMD_FIFO_DELETE_P);
}


/*
 * devlog option ...
 */
static void
CmdDevlog(
	void)
{
	int		an;
	int		eq;	/* Equipment ordinal */
	int		flags;
	dev_ent_t	*un;	/* Device entry */

	if (Argc < 2)
		Usage();
	eq = findDev(Argv[1]);
	un = (dev_ent_t *)SHM_ADDR(master_shm, Dev_Tbl->d_ent[eq]);

	flags = un->log.flags;
	for (an = 2; an < Argc; an++) {
		enum DL_event   to;
		char	   *name;
		int		not;

		name = Argv[an];
		if (*name != '-')
			not = FALSE;
		else {
			name++;
			not = TRUE;
		}

		for (to = 0; strcmp(name, DL_names[to]) != 0; to++) {
			if (to >= DL_MAX) {
				Error(catgets(catfd, SET, 7002,
				    "Unknown devlog option: %s"),
				    name);
			}
		}
		if (DL_none == to)
			flags = 0;
		else if (DL_all == to)
			flags = DL_all_events;
		else if (DL_default == to)
			flags = DL_def_events;
		else if (not)
			flags &= ~(1 << to);
		else
			flags |= 1 << to;
	}
	un->log.flags = flags;
}


/*
 * dtrace daemon
 */
static void
CmdDtrace(
	void)
{
	static char	value[STR_OPTIONS_BUF_SIZE];
	char	   *msg;
	int		i;
	int		next;

	if (Argc < 2) {
		Usage();
	}
/*
 * Collect values.
 */
	next = 0;
	for (i = 2; i < Argc; i++) {
		int		l;

		l = strlen(Argv[i]);
		if (next + l > sizeof (value) - 1) {
			break;
		}
		if (next != 0) {
			*(value + next++) = ' ';
		}
		memmove(value + next, Argv[i], l);
		next += l;
	}
	*(value + next) = '\0';
	msg = TraceControl(Argv[1], value, NULL);
	if (*msg != '\0') {
		Error(msg);
	}
}


/*
 * q
 */
static void
CmdExit(
	void)
{
	exit(0);
}


/*
 * import eq
 */
static void
CmdImport(
	void)
{
	int		eq;	/* Equipment ordinal */
	dtype_t	 ty;	/* Device type */
	dev_ent_t	*dev;	/* Device entry */

	if (Argc < 2)
		Usage();
	eq = findDev(Argv[1]);
	dev = (dev_ent_t *)SHM_ADDR(master_shm, Dev_Tbl->d_ent[eq]);

	ty = dev->type & DT_CLASS_MASK;
	if (ty != DT_ROBOT || dev->state > DEV_IDLE ||
	    dev->type == DT_DLT2700 || dev->type == DT_LMS4500 ||
	    dev->type == DT_METD28) {
		Error(catgets(catfd, SET, 592, "Cannot import for robot (%s)."),
		    Argv[1]);
	}
	memset(&cmd_block, 0, sizeof (cmd_block));
	cmd_block.eq = eq;
	issue_fifo_cmd(CMD_FIFO_IMPORT);
}


/*
 * Handle the following commands:
 * dio_rd_consec eq value
 * dio_rd_form_min eq value
 * dio_rd_ill_min eq value
 * dio_wr_consec eq value
 * dio_wr_form_min eq value
 * dio_wr_ill_min eq value
 * flush_behind eq value
 * stage_flush_behind eq value
 * invalid eq interval
 * meta_timeo eq interval
 * minallocsz eq value
 * maxallocsz eq value
 * partial eq value
 * maxpartial eq value
 * partial_stage eq value
 * readahead eq value
 * writebehind eq value
 * mm_stripe eq value
 * stage_n_window eq value
 * stage_retries eq value
 * stripe eq value
 * sync_meta eq value
 * atime eq value
 * aplease eq interval
 * rdlease eq interval
 * wrlease eq interval
 * def_retention eq interval
 * wr_throttle eq value
 */
static void
CmdSetFsParam(
		void)
{
	struct sam_fs_info fi;
	char	   *msg;

	if (Argc < 3)
		Usage();
	CheckFamilySetByEq(Argv[1], &fi);
	msg = SetFsParam(fi.fi_name, Argv[0], Argv[2]);
	if (*msg != '\0') {
		Error(msg);
	}
}


/*
 * mount mntpt
 */
void
CmdMount(
	void)
{
	if (Argc < 2)
		Usage();
	if (Ioctl_fd != 0) {
		close(Ioctl_fd);
		Ioctl_fd = 0;
	}
	Mount_Point = Argv[1];
	open_mountpt(Argv[1]);
}


/*
 * Select filesystem
 */
static void
CmdFs(
	void)
{
	if (Argc < 2)
		Usage();
	File_System = Argv[1];
	FsInitialized = 1;
}


/*
 * export eq:slot
 * export mt.vsn
 */
static void
CmdExport(
	void)
{
	int		onestep = FALSE;	/* if one step ACSLS export */
	int		wait_response = 0;

	if (Argc == 3) {
		if (strcmp(Argv[1], "-f") == 0) {
			onestep = TRUE;
			CatlibInit();
			(void) SamExportCartridge(Argv[2], wait_response,
			    onestep, MsgFunc);
		} else {
			Usage();
		}
	} else if (Argc != 2) {
		Usage();
	} else {
		CatlibInit();
		(void) SamExportCartridge(Argv[1], wait_response,
		    onestep, MsgFunc);
	}
}

/*
 * load eq:slot[:side]
 * load mt.vsn
 */
static void
CmdLoad(
	void)
{
	int		eq;	/* Equipment ordinal */
	struct VolId    vid;
	dev_ent_t	*dev;
	struct CatalogEntry ced;
	struct CatalogEntry *ce = &ced;

	if (Argc != 2)
		Usage();
	CatlibInit();

	/*
	 * Convert specifier to volume identifier.
	 */
	if (StrToVolId(Argv[1], &vid) != 0) {
		Error(catgets(catfd, SET, 18207,
		    "Volume specification error %s:"),
		    Argv[1]);
	/* NOTREACHED */
	}
	/*
	 * Validate the specifier.
	 */
	if ((ce = CatalogCheckVolId(&vid, &ced)) == NULL) {
		Error(catgets(catfd, SET, 18207,
		    "Volume specification error %s: "),
		    Argv[1]);
	/* NOTREACHED */
	}
	eq = ce->CeEq;
	dev = (dev_ent_t *)SHM_ADDR(master_shm, Dev_Tbl->d_ent[eq]);
	/*
	 * Check device.
	 */
	if (!IS_ROBOT(dev))
		Error(catgets(catfd, SET, 1806,
		    "Not a robot device (%s)."), Argv[1]);

	if (dev->state > DEV_IDLE) {
		Error(catgets(catfd, SET, 884,
		    "Device off or down (%s)."), Argv[1]);
	}
	memset(&cmd_block, 0, sizeof (cmd_block));

	switch (vid.ViFlags) {
	case VI_logical:

		cmd_block.eq = eq;
		cmd_block.slot = 0;
		cmd_block.media = sam_atomedia(vid.ViMtype);
		strncpy(cmd_block.vsn, vid.ViVsn, sizeof (vsn_t));
		issue_fifo_cmd(CMD_FIFO_MOUNT);
		break;

	case VI_onepart:
	case VI_cart:
		cmd_block.eq = vid.ViEq;
		cmd_block.slot = ce->CeSlot;
		cmd_block.part = ce->CePart;
		issue_fifo_cmd(CMD_FIFO_MOUNT_S);
		break;
	default:
		Error(catgets(catfd, SET, 18207,
		    "Volume specification error %s:"),
		    Argv[1]);
		break;
	}
}


/*
 * open eq
 */
static void
CmdOpen(
	void)
{
	if (Argc < 2)
		Usage();
	(void) opendisk(Argv[1]);
	Argc = 0;
	(void) SetDisplay('S');
	(void) readdisk(0);
}


/*
 * priority pid newpri
 */
static void
CmdPriority(
	    void)
{
	int		ret;
	uint_t	  pid;
	float	   newpri;

	if (Argc < 3)
		Usage();
	pid = strtol(Argv[1], NULL, 0);
	errno = 0;
	newpri = (float)strtod(Argv[2], NULL);
	if (errno != 0) {
		Error("%s %f", catgets(catfd, SET, 7181,
		    "Bad priority"), newpri);
	}
	ret = SamSetPreviewPri(pid, newpri);
	if (ret != 0) {
		Error(catgets(catfd, SET, 7182, "pid %d not found"), pid);
	}
}


/*
 * read addr
 */
static void
CmdRead(
	void)
{
	uint_t	  sector;

	if (Argc < 2)
		Usage();
	sector = strtol(Argv[1], NULL, 16);
	(void) readdisk(sector);
	Argc = 0;
	(void) SetDisplay('S');
}


/*
 * refresh delay
 */
static void
CmdRefresh(
	void)
{
	if (Argc > 1)
		Delay = strtol(Argv[1], NULL, 0);
	if (Delay > 25)
		Delay = 25;
	if (Delay == 0) {
		Refresh = FALSE;
		cbreak();
	} else {
		Refresh = TRUE;
		halfdelay(Delay * 10);
	}
}


/*
 * Set device state.
 */
static void
CmdSetstate(
	    void)
{
	int		eq;	/* Equipment ordinal */
	dev_ent_t	*dev;	/* Device entry */

	if (Argc < 2)
		Usage();
	eq = findDev(Argv[1]);
	dev = (dev_ent_t *)SHM_ADDR(master_shm, Dev_Tbl->d_ent[eq]);

	switch (cmd_t[cmd_n].par) {

	case DEV_IDLE:
		if (dev->state != DEV_ON)
			Error(catgets(catfd, SET, 877, "Device not on."));
		break;

	case DEV_ON:
	case DEV_UNAVAIL:
		break;

	case DEV_OFF:
	case DEV_RO:
		break;

	default:
		Error(catgets(catfd, SET, 2764, "Unknown state change."));
		break;

	}
	memset(&cmd_block, 0, sizeof (cmd_block));
	cmd_block.eq = eq;
	cmd_block.state = cmd_t[cmd_n].par;
	issue_fifo_cmd(CMD_FIFO_SET_STATE);
}


/*
 * Snap screen image.
 */
static void
CmdSnap(
	void)
{
	FILE	   *st;
	int		x, y;

	if (Argc < 2)
		Argv[1] = "snapshots";
	if ((st = fopen(Argv[1], "a")) == NULL)
		Error(catgets(catfd, SET, 613, "Cannot open %s"), Argv[1]);
	fprintf(st, "\f\n");
	for (y = 0; y < LINES; y++) {
		for (x = 0; x < COLS; x++) {
			fprintf(st, "%c", (int)mvinch(y, x) & 0177);
		}
		fprintf(st, "\n");
	}
	fclose(st);
}


/*
 * stclear mt.vsn
 * Clear stage request for mt.vsn.
 */
static void
CmdStclear(
	void)
{
	char	    msg[80];
	struct VolId    vid;

	if (Argc < 2 || Argc >= 3) {
		Usage();
	}
	if (StrToVolId(Argv[1], &vid) != 0) {
		Error(catgets(catfd, SET, 18207,
		    "Volume specification error %s:"), Argv[1]);
	}
	(void) StagerControl(Argv[0], Argv[1], msg, sizeof (msg));
	if (*msg != '\0') {
		Error(msg);
	}
}


/*
 * stidle
 */
static void
CmdStidle(
	void)
{
	daemonSetExec(SAM_STAGER, "idle");
}


/*
 * strun
 */
static void
CmdStrun(
	void)
{
	daemonSetExec(SAM_STAGER, "run");
}


/*
 * thresh eq high low
 */
static void
CmdThresh(
	void)
{
	struct sam_fs_info fi;
	char	   *msg;

	if (Argc < 4)
		Usage();
	CheckFamilySetByEq(Argv[1], &fi);
	msg = SetFsParam(fi.fi_name, "high", Argv[2]);
	if (*msg != '\0') {
		Error(msg);
	}
	msg = SetFsParam(fi.fi_name, "low", Argv[3]);
	if (*msg != '\0') {
		Error(msg);
	}
}


/*
 * Handle the following commands:
 * forcedirectio eq
 * noforcedirectio eq
 * hwm_archive eq
 * nohwm_archive eq
 * mh_write eq
 * nomh_write eq
 * suid eq
 * nosuid eq
 * trace eq
 * notrace eq
 * qwrite eq
 * noqwrite eq
 * sw_raid eq
 * nosw_raid eq
 */
static void
CmdSetFsConfig(
		void)
{
	struct sam_fs_info fi;
	char	   *msg;

	if (Argc < 2)
		Usage();
	CheckFamilySetByEq(Argv[1], &fi);
	msg = SetFsConfig(fi.fi_name, Argv[0]);
	if (*msg != '\0') {
		Error(msg);
	}
}


/*
 * Handle the following commands on a disk eq on a mounted file system:
 * add eq
 * remove eq
 * release eq
 * alloc eq
 * noalloc eq
 */
static void
CmdSetFsDskCmd(
		void)
{
	struct sam_fs_info fi;
	int32_t	command;
	char	*errstr;

	if (Argc < 2) {
		Usage();
	}
	errstr = CheckFSPartByEq(Argv[1], &fi);
	if (errstr == NULL) {
		if (strcmp(Argv[0], "add") == 0) {
			command = DK_CMD_add;
		} else if (strcmp(Argv[0], "release") == 0) {
			command = DK_CMD_release;
		} else if (strcmp(Argv[0], "remove") == 0) {
			command = DK_CMD_remove;
		} else if (strcmp(Argv[0], "alloc") == 0) {
			command = DK_CMD_alloc;
		} else if (strcmp(Argv[0], "noalloc") == 0) {
			command = DK_CMD_noalloc;
		} else {
			Error(errstr, Argv[1]);
			return;
		}
		if (SetFsPartCmd(fi.fi_name, Argv[1], command) != 0) {
			Error(catgets(catfd, SET, 7009,
			    "Cannot execute command %s"
			    " for device %s., errno=%d"),
			    Argv[0], Argv[1], errno);
		}
	} else {
		Error(errstr, Argv[1]);
	}
}


/*
 * unload eq
 */
static void
CmdUnload(
	void)
{
	int		eq;	/* Equipment ordinal */
	dev_ent_t	*dev;	/* Device entry */

	if (Argc < 2)
		Usage();
	eq = findDev(Argv[1]);
	dev = (dev_ent_t *)SHM_ADDR(master_shm, Dev_Tbl->d_ent[eq]);

	if (!(IS_OPTICAL(dev) || IS_TAPE(dev) || IS_ROBOT(dev)) ||
	    dev->state > DEV_IDLE) {
		Error(catgets(catfd, SET, 646, "Cannot unload device (%s)."),
		    Argv[1]);
	}
	memset(&cmd_block, 0, sizeof (cmd_block));
	cmd_block.eq = eq;
	issue_fifo_cmd(CMD_FIFO_UNLOAD);
}


/*
 * zap inode
 */
static void
CmdZap(
	void)
{
	struct sam_ioctl_inode rq_inode;	/* Inode request block */

	if (Argc < 2)
		Usage();
	rq_inode.ino = strtol(Argv[1], NULL, 0);
	open_mountpt(Argv[1]);
	rq_inode.mode = 0;
	rq_inode.ip.ptr = NULL;
	rq_inode.pip.ptr = NULL;
	if (ioctl(Ioctl_fd, F_ZAPINO, &rq_inode) != 0)
		Error("ioctl(F_ZAPINO)");
}


/*
 * diskvols vsn flags
 * Set or clear disk volume flags
 */
static void
CmdDiskvols(
	    void)
{
	char	   *vsn;
	char	   *flags;
	DiskVolsDictionary_t *diskvols;
	DiskVolumeInfo_t *dv;
	int		mask;

	if (Argc != 3) {
		Usage();
	}
	vsn = Argv[1];
	flags = Argv[2];
	dv = NULL;

	if (flags[0] != '-' && flags[0] != '+') {
		Usage();
	}
	diskvols = DiskVolsNewHandle("samu", DISKVOLS_VSN_DICT, 0);
	if (diskvols != NULL) {
		(void) diskvols->Get(diskvols, vsn, &dv);
	}
	if (dv == NULL) {
		Error(catgets(catfd, SET, 7400, "Disk volume %s not found"),
		    vsn);
		goto out;
	}
	switch (flags[1]) {

	case 'l':
		mask = DV_labeled;
		break;

	case 'U':
		mask = DV_unavail;
		break;

	case 'R':
		mask = DV_read_only;
		break;

	case 'E':
		mask = DV_bad_media;
		break;

	case 'A':
		mask = DV_needs_audit;
		break;

	case 'F':
		mask = DV_archfull;
		break;

	case 'c':
		mask = DV_recycle;
		break;

	default:
		mask = 0;

	}

	if (mask != 0) {
		if (flags[0] == '-') {
			dv->DvFlags &= ~mask;
		} else {
			dv->DvFlags |= mask;
		}
		(void) diskvols->Put(diskvols, vsn, dv);
	} else {
		(void) DiskVolsDeleteHandle(DISKVOLS_VSN_DICT);
		Error(catgets(catfd, SET, 7401,
		    "Unrecognized disk volume flag' %c'"),
		    flags[1]);
	}

out:
	if (diskvols != NULL) {
		(void) DiskVolsDeleteHandle(DISKVOLS_VSN_DICT);
	}
}


/*
 * Error message function to supply to sam api routines.
 */
static void
MsgFunc(
/* LINTED argument unused in function */
	int code,
	char *msg)
{
	Error("%s", msg);
}

/*
 * Send usage message.
 */
static void
Usage(
	void)
{
	Error("%s %s %s", catgets(catfd, SET, 4601, "usage:"), Argv[0],
	    catgets(catfd, SET, cmd_t[cmd_n].usage, cmd_t[cmd_n].usage_msg));
}


/*
 * Display command help.
 */
void
DisCmdHelp(
	int start)
{
	int		n;
	int		m;
	int		cmdgrp = 0;

/*
 *	Skip to start of command group.
 */
	for (m = 0; m < numof(cmd_t); m++) {
		if (*cmd_t[m].name == '\0') {
			if (start == cmdgrp++) {
				break;
			}
		}
	}
	for (n = m; n < numof(cmd_t); n++) {
		if ((cmd_t[n].sam_lvl == SAM_REMOVABLE) && !IsSamRunning)
			continue;
		if ((cmd_t[n].sam_lvl == SAM_DISK) && !IsSam)
			continue;
		if (*cmd_t[n].name == '\0') {
			if (n == m) {
				Mvprintw(ln++, 0, "%s",
				    catgets(catfd, SET, cmd_t[n].help,
				    cmd_t[n].help_msg));
			} else {
				return;	/* the end of this command group. */
			}
		} else {
			char cm[160];	/* allow for 16-bit characters */

			strncpy(cm, cmd_t[n].name, sizeof (cm) - 1);
			strncat(cm, " ", sizeof (cm) - 1);
			strncat(cm, catgets(catfd, SET, cmd_t[n].usage,
			    cmd_t[n].usage_msg),
			    sizeof (cm) - 1);
			Mvprintw(ln++, 0, "    %-31s%s", cm,
			    catgets(catfd, SET, cmd_t[n].help,
			    cmd_t[n].help_msg));
		}
	}
}


/*
 * Process archiver/stager state.
 */
static void
daemonSetExec(
		char *name,
		char *ctrl)
{
	char	   *ident;
	char	    msg[80];

	if (Argc > 2) {
		Usage();
	}
	if (Argc == 2) {
		snprintf(msg, sizeof (msg), "exec.%s", Argv[1]);
		ident = msg;
	} else {
		ident = "exec";
	}
	if (strcmp(name, SAM_ARCHIVER) == 0) {
		(void) ArchiverControl(ident, ctrl, msg, sizeof (msg));
	} else if (strcmp(name, SAM_STAGER) == 0) {
		(void) StagerControl(ident, ctrl, msg, sizeof (msg));
	}
	if (*msg != '\0') {
		Error(msg);
	}
}


/*
 * Find a preview entry.
 */
static int
find_preview(
	char *s) /* VSN */
{
	preview_t	*ent;
	int		i, count, match;

	match = 0;
	count = Preview_Tbl->ptbl_count;
	for (i = 0; i < Preview_Tbl->avail && count != 0; i++) {
		ent = &Preview_Tbl->p[i];
		if (!ent->in_use)
			continue;
		count--;
		if (strcmp(s, ent->resource.archive.vsn) == 0) {
			match++;
			break;
		}
	}
	if (match)
		return (i);
	return (-1);
}



#define	FIFO_path	SAM_FIFO_PATH"/"CMD_FIFO_NAME

static void
issue_fifo_cmd(
	int cmd) /* FIFO command to issue */
{
	int	fifo_fd; /* File descriptor for FIFO */
	int	size;

	fifo_fd = open(FIFO_path, O_WRONLY | O_NONBLOCK);
	if (fifo_fd < 0)
		Error(catgets(catfd, SET, 560, "Cannot open cmd FIFO"));
	cmd_block.magic = CMD_FIFO_MAGIC;
	cmd_block.cmd = cmd;

	size = write(fifo_fd, &cmd_block, sizeof (sam_cmd_fifo_t));
	close(fifo_fd);
	if (size != sizeof (sam_cmd_fifo_t)) {
		Error(catgets(catfd, SET, 1114, "Command FIFO write failed"));
	}
}
