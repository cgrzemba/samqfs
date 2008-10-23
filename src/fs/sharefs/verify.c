/*
 * verify.c - Exchange message between server/client at connect.
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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#pragma ident "$Revision: 1.34 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <syslog.h>
#ifdef linux
#include <string.h>
#endif

/* POSIX headers. */
#include <sys/vfs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>

/* Solaris headers. */

/* Socket headers. */
#include <arpa/inet.h>
#include <netinet/in.h>

/* SAM-FS headers. */
#include "bswap.h"
#include "sam/mount.h"
#include "sam/quota.h"
#include "sam/syscall.h"
#include "sam/shareops.h"
#include "sam/sam_trace.h"
#include "sblk.h"
#include "share.h"
#include "samhost.h"
#include "behavior_tags.h"
#include "pub/version.h"

/* Local headers. */
#include "sharefs.h"
#include "config.h"


extern struct shfs_config config;


/*
 * Byte swapping data structures and routines.
 */

#define	VFY_MSG_KEY		0x8282838485868789ULL

#define	VFY_MSG_CONREQ		0x5463702f636e786eULL	/* "Tcp/Cnxn" */
#define	VFY_MSG_OK		0x436e786e20414f4bULL	/* "Cnxn AOK" */
#define	VFY_MSG_REJECT		0x2e52656a6563742eULL	/* ".Reject." */

/*
 * Size of over-the-wire buffer holding comma-separated list of tag names.
 */
#define	QFS_TAG_LIST_SZ 1024

#define	QFS_MAJORV_LEN	8	/* space in message for SAM_MAJORV */
#define	QFS_MINORV_LEN	8	/* space in message for SAM_MINORV */
#define	QFS_FIXV_LEN	16	/* space in message for SAM_FIXV */
#define	QFS_SYS_NMLN	50	/* space for uname(2) fields */

/*
 * A copy of this message is passed each way between client and
 * server at connect time.  It's used to detect byte order
 * differences between the two, and to verify that the connecting
 * entity is actually a a shared FS intending to connect.
 *
 * If this object's fields change, then vfy_message_swap_descriptor
 * (below) may also need to change.
 */
struct vfy_message {
	uint64_t	unused;		/* future use; encryption iv perhaps */
	uint64_t	key;		/* implicit byte order code */
	uint64_t	magic;		/* magic number */
	sam_time_t	now;		/* current time on sending host */
	sam_time_t	fsid;		/* fs's creation time */
	uint32_t	labelversion;	/* label block's version # */
	uint32_t	labelgen;	/* label block's generation # */
	uint32_t	serverord;	/* ordinal of the server */
	uint32_t	flags;		/* flags for random checks */
	int32_t		tag_list_size;	/* QFS_TAG_LIST_SZ */
	int32_t		tag_cnt;	/* number of tags in list */
	upath_t		hname;		/* client host name */
	char		qfsmajorv[QFS_MAJORV_LEN]; /* SAM_MAJORV */
	char		qfsminorv[QFS_MINORV_LEN]; /* SAM_MINORV */
	char		qfsfixv[QFS_FIXV_LEN]; /* SAM_FIXV */
	char		sysname[QFS_SYS_NMLN];
	char		release[QFS_SYS_NMLN];
	char		machine[QFS_SYS_NMLN];
	char		tag_list[QFS_TAG_LIST_SZ];
};

/*
 * Flags for flags in vfy_message
 */
#define		VFY_F_NOTSRVR	0x01	/* Calling client can't be server */

/*
 * If the vfy_message structure changes, this needs to too.
 */
static struct swap_descriptor vfy_message_swap_descriptor[] = {
	{ 0, 8, 3 },		/* 3 8-byte variables */
	{ 24, 4, 8 },		/* 8 4-byte variables following */
	{ 56, 0, 0 },
};


/*
 * Add each tag name after verifying that it will fit into the buffer.
 */
static void
__FillBehaviorTag(char *tag_list, int32_t *tag_cnt, char *tag_name)
{
	if (tag_list[0] == '\0') {
		ASSERT(strlen(tag_name) < QFS_TAG_LIST_SZ);
		strcat(tag_list, tag_name);
		goto out_incr;
	}

	ASSERT(strlen(tag_list) + 1 + strlen(tag_name) < QFS_TAG_LIST_SZ);
	strcat(tag_list, ",");
	strcat(tag_list, tag_name);

out_incr:
	++*tag_cnt;
}

/*
 * Load the tag_list with a comma-separated list of our tag names.
 */
static void
FillBehaviorTags(int32_t *tag_cnt, char *tag_list)
{
	*tag_cnt = 0;
	tag_list[0] = '\0';

	/*
	 * Example code.  Use tag name definitions from "behavior_tags.h":
	 *
	 *  __FillBehaviorTag(tag_list, tag_cnt, QFS_TAG_BLK_QUOTA_V2_STR);
	 *  __FillBehaviorTag(tag_list, tag_cnt, QFS_TAG_SPECIAL_STUFF_STR);
	 */

	__FillBehaviorTag(tag_list, tag_cnt, QFS_TAG_VFSSTAT_V2_STR);
}



/*
 * Parse the comma-separated list of behavior tags from the other machine
 * and convert them to a bit mask.
 *
 * We count a tag as "found" even if we don't recognize that tag.  We will
 * silently ignore any tags we don't recognize.  The other machine will
 * receive a copy of our own tag list and it will know which tags we don't
 * recognize.
 */
static int32_t
ProcessBehaviorTags(char *tag_list, uint64_t *tags_mask)
{
	char *lasts, *tok;
	int32_t tags_found = 0;

	*tags_mask = 0;
	if ((tok = strtok_r(tag_list, ",", &lasts)) == NULL) {
		goto out;
	}
	do {
		++tags_found;


		/*
		 * Example code.  Use tag name and bit definitions
		 * from "behavior_tags.h":
		 *
		 * if (strcmp(tok, QFS_TAG_BLK_QUOTA_V2_STR) == 0) {
		 *	*tags_mask |= QFS_TAG_BLK_QUOTA_V2;
		 *	continue;
		 * }
		 *
		 * if (strcmp(tok, QFS_TAG_SPECIAL_STUFF_STR) == 0) {
		 *	*tags_mask |= QFS_TAG_SPECIAL_STUFF;
		 *	continue;
		 * }
		 *
		 */

		if (strcmp(tok, QFS_TAG_VFSSTAT_V2_STR) == 0) {
			*tags_mask |= QFS_TAG_VFSSTAT_V2;
			continue;
		}

	} while ((tok = strtok_r(NULL, ",", &lasts)) != NULL);

out:
	return (tags_found);
}


static void
SetQFSVer(struct vfy_message *msg, char *fi_name)
{
	struct utsname uts;

	strncpy(msg->qfsmajorv, SAM_MAJORV, QFS_MAJORV_LEN);
	msg->qfsmajorv[QFS_MAJORV_LEN-1] = '\0';
	strncpy(msg->qfsminorv, SAM_MINORV, QFS_MINORV_LEN);
	msg->qfsminorv[QFS_MINORV_LEN-1] = '\0';
	strncpy(msg->qfsfixv, SAM_FIXV, QFS_FIXV_LEN);
	msg->qfsfixv[QFS_FIXV_LEN-1] = '\0';

	if (uname(&uts) < 0) {
		SysError(HERE,
		    "FS %s: error %d from uname(2)",
		    fi_name, errno);
		msg->sysname[0] = '\0';
		msg->release[0] = '\0';
		msg->machine[0] = '\0';
		return;
	}
	strncpy(msg->sysname, uts.sysname, QFS_SYS_NMLN);
	msg->sysname[QFS_SYS_NMLN-1] = '\0';
	strncpy(msg->release, uts.release, QFS_SYS_NMLN);
	msg->release[QFS_SYS_NMLN-1] = '\0';
	strncpy(msg->machine, uts.machine, QFS_SYS_NMLN);
	msg->machine[QFS_SYS_NMLN-1] = '\0';
}

/*
 * ----- VerifyClientSocket
 *
 * Send a message to the daemon at the other end of the given
 * socket (which should reside on the server).  Read up the
 * message that it should send to this end.
 *
 * Abort if it doesn't look right.
 *
 * Returns:
 *	+1: Success; server has reversed byte order.
 *	0: Success;
 *	-1: error
 */
int
VerifyClientSocket(int sockfd, uint64_t *server_tags)
{
	int n;
	int byteswap = 0;
	struct cfdata *cfp;
	struct vfy_message msg, reply;
	int32_t tagcnt;

	cfp = &config.cp[config.curTab];

	bzero((char *)&msg, sizeof (msg));
	msg.unused = 0;
	msg.key = VFY_MSG_KEY;
	msg.magic = VFY_MSG_CONREQ;

	SetQFSVer(&msg, Host.fs.fi_name);

	msg.now = time(NULL);
	msg.fsid = cfp->fsInit;
	msg.labelversion = cfp->lb.info.lb.version;
	msg.labelgen = cfp->lb.info.lb.gen;
	msg.serverord = cfp->lb.info.lb.serverord;
	msg.flags = (config.mnt.params.fi_status & FS_NODEVS) ?
	    VFY_F_NOTSRVR : 0;
	msg.tag_list_size = QFS_TAG_LIST_SZ;
	msg.tag_cnt = 0;
	msg.tag_list[0] = '\0';

	FillBehaviorTags(&msg.tag_cnt, msg.tag_list);

	if (!(msg.flags & VFY_F_NOTSRVR)) {
		int i;

		/*
		 * If the FS hasn't yet been mounted fi_status won't be set.
		 * Check the mount partitions for any "nodev" devices.
		 */
		for (i = 0; i < config.mnt.params.fs_count; i++) {
			if (strcmp(config.mnt.part[i].pt_name, "nodev") == 0) {
				msg.flags |= VFY_F_NOTSRVR;
				break;
			}
		}
	}
	strncpy(msg.hname, Host.hname, sizeof (msg.hname));

	if ((n = write(sockfd, (char *)&msg, sizeof (msg))) != sizeof (msg)) {
		SysError(HERE,
		    "FS %s: write to server failed (%d/%d); shutting down",
		    Host.fs.fi_name, n, sizeof (msg));
		goto err;
	}
	if ((n = read(sockfd, (char *)&reply, sizeof (reply))) !=
	    sizeof (reply)) {
		if (n < 0) {
			SysError(HERE,
			    "FS %s: read from server failed (%d/%d); "
			    "shutting down",
			    Host.fs.fi_name, n, sizeof (reply));
		} else {
			SysError(HERE,
			    "FS %s: short read from server (%d/%d); "
			    "shutting down",
			    Host.fs.fi_name, n, sizeof (reply));
		}
		goto err;
	}
	errno = 0;
	if (reply.key != VFY_MSG_KEY) {
		/*
		 * Does the server have a different byte-order?
		 */
		if (sam_byte_swap(
		    vfy_message_swap_descriptor, (void *)&reply,
		    sizeof (reply))) {
			SysError(HERE, "FS %s: sam_byte_swap error",
			    Host.fs.fi_name);
			goto err;
		}
		if (reply.key != VFY_MSG_KEY) {
			/*
			 * Apparently we just got junk.
			 */
			SysError(HERE,
			    "FS %s: Unrecognized message from server; "
			    "shutting down",
			    Host.fs.fi_name);
			goto err;
		}
		byteswap = 1;
	}
	if (reply.magic == VFY_MSG_REJECT) {
		SysError(HERE, "FS %s: Server rejected connection; shutting "
		    "down",
		    Host.fs.fi_name);
		goto err;
	}
	if (reply.magic != VFY_MSG_OK) {
		SysError(HERE, "FS %s: Invalid reply from server; shutting "
		    "down",
		    Host.fs.fi_name);
		goto err;
	}
	if (reply.fsid != cfp->fsInit ||
	    reply.labelversion != msg.labelversion ||
	    reply.labelgen != msg.labelgen ||
	    reply.serverord != msg.serverord) {
		SysError(HERE, "FS %s: stale or mismatched server reply; "
		    "rejecting",
		    Host.fs.fi_name);
		if (reply.fsid != cfp->fsInit) {
			Trace(TR_MISC, "FS %s: reply.fsid = %x (%x expected)",
			    Host.fs.fi_name, reply.fsid, msg.fsid);
		}
		if (reply.labelversion != msg.labelversion) {
			Trace(TR_MISC, "FS %s: reply.labelversion = %x "
			    "(%x expected)",
			    Host.fs.fi_name, reply.labelversion,
			    msg.labelversion);
		}
		if (reply.labelgen != msg.labelgen) {
			Trace(TR_MISC, "FS %s: reply.labelgen = %x "
			    "(%x expected)",
			    Host.fs.fi_name, reply.labelgen, msg.labelgen);
		}
		if (reply.serverord != msg.serverord) {
			Trace(TR_MISC, "FS %s: reply.serverord = %x "
			    "(%x expected)",
			    Host.fs.fi_name, reply.serverord, msg.serverord);
		}
		goto err;
	}
	if (reply.tag_list_size != QFS_TAG_LIST_SZ) {
		SysError(HERE, "FS %s: server and client differ in "
		    "tag_list_size, rejecting",
		    Host.fs.fi_name);
		Trace(TR_MISC, "FS %s: server and client differ in "
		    "tag_list_size",
		    Host.fs.fi_name);
		goto err;
	}
	if (abs(msg.now - reply.now) > 60 &&
	    (n = time(NULL)) && abs(n - msg.now) < 15) {
		Trace(TR_MISC, "FS %s: server and client time differ by > 60s",
		    Host.fs.fi_name);
	}

	Trace(TR_MISC, "FS %s: Server's sam_majorv: %s",
	    Host.fs.fi_name, reply.qfsmajorv);
	Trace(TR_MISC, "FS %s: server's sam_minorv: %s",
	    Host.fs.fi_name, reply.qfsminorv);
	Trace(TR_MISC, "FS %s: Server's sam_fixv: %s",
	    Host.fs.fi_name, reply.qfsfixv);
	Trace(TR_MISC, "FS %s: Server's sysname: %s",
	    Host.fs.fi_name, reply.sysname);
	Trace(TR_MISC, "FS %s: Server's release: %s",
	    Host.fs.fi_name, reply.release);
	Trace(TR_MISC, "FS %s: Server's machine: %s",
	    Host.fs.fi_name, reply.machine);

	Trace(TR_MISC, "FS %s: Server's tag_list: %s",
	    Host.fs.fi_name, reply.tag_list);

	tagcnt = ProcessBehaviorTags(reply.tag_list, server_tags);
	if (tagcnt != reply.tag_cnt) {
		SysError(HERE, "FS %s: tag count mismatch, found %d, "
		    "expected %d",
		    Host.fs.fi_name, tagcnt, reply.tag_cnt);
		Trace(TR_MISC, "FS %s: found %d tags (%d expected)",
		    Host.fs.fi_name, tagcnt, reply.tag_cnt);
		goto err;
	}

	Trace(TR_MISC, "FS %s: Server's locally understood tag mask: 0x%llx",
	    Host.fs.fi_name, (unsigned long long)*server_tags);

	return (byteswap);

err:
	return (-1);
}


#ifdef METADATA_SERVER

/*
 * Read from a socket to verify that it talks like a client.  If it does,
 * return an "OK" message, otherwise return a "reject" message.  Client
 * end of the socket writes first.
 *
 * Return values:
 *	-1: error
 *	 0: OK
 */
int
VerifyServerSocket(
	int sockfd,
	char *client,
	int *srvrcap,
	int *byterev,
	uint64_t *client_tags)
{
	int n;
	struct cfdata *cfp;
	struct vfy_message msg, reply;
	int32_t tagcnt;

	if (byterev) {
		*byterev = 0;
	}
	if (srvrcap) {
		*srvrcap = 1;
	}

	cfp = &config.cp[config.curTab];

	if ((n = read(sockfd, (char *)&msg, sizeof (msg))) != sizeof (msg)) {
		if (n < 0) {
			SysError(HERE, "FS %s: read from client failed "
			    "(%d/%d); rejecting",
			    Host.fs.fi_name, n, sizeof (msg));
		} else {
			SysError(HERE, "FS %s: short read from client "
			    "(%d/%d); rejecting",
			    Host.fs.fi_name, n, sizeof (msg));
		}
		goto reject;
	}

	errno = 0;
	if (msg.key != VFY_MSG_KEY) {
		/*
		 * Does the client have a different byte-order?
		 */
		if (sam_byte_swap(
		    vfy_message_swap_descriptor, (void *)&msg,
		    sizeof (msg))) {
			SysError(HERE, "FS %s: sam_byte_swap error",
			    Host.fs.fi_name);
			goto reject;
		}
		if (msg.key != VFY_MSG_KEY) {
			/*
			 * Looks like junk.
			 */
			SysError(HERE, "FS %s: Unrecognized message from "
			    "client; closing",
			    Host.fs.fi_name);
			goto reject;
		}
		if (srvrcap) {
			*srvrcap = 0;
		}
		if (byterev) {
			*byterev = 1;
		}
	}

	if (msg.flags & VFY_F_NOTSRVR) {
		/*
		 * Client says it can't be a server.
		 */
		if (srvrcap) {
			*srvrcap = 0;
		}
	}

	if (msg.magic != VFY_MSG_CONREQ ||
	    msg.fsid != cfp->fsInit ||
	    msg.labelversion != cfp->lb.info.lb.version ||
	    msg.labelgen != cfp->lb.info.lb.gen ||
	    msg.serverord != Host.serverord ||
	    strncasecmp(msg.hname, client, sizeof (msg.hname))) {
		SysError(HERE,
		    "FS %s: stale or mismatched client initialization "
		    "msg; rejecting",
		    Host.fs.fi_name);
		if (msg.magic != VFY_MSG_CONREQ) {
			Trace(TR_MISC, "FS %s: msg.magic = %llx "
			    "(%llx expected)",
			    Host.fs.fi_name, msg.magic, VFY_MSG_CONREQ);
		}
		if (msg.fsid != cfp->fsInit) {
			Trace(TR_MISC, "FS %s: msg.fsid/expected fsInit = "
			    "%x/%x",
			    Host.fs.fi_name, msg.fsid, cfp->fsInit);
		}
		if (msg.labelversion != cfp->lb.info.lb.version) {
			Trace(TR_MISC,
			    "FS %s: msg.labelversion/expected label "
			    "version = %x/%x",
			    Host.fs.fi_name, msg.labelversion,
			    cfp->lb.info.lb.version);
		}
		if (msg.labelgen != cfp->lb.info.lb.gen) {
			Trace(TR_MISC,
			    "FS %s: msg.labelversion/expected gen "
			    "version = %d/%d",
			    Host.fs.fi_name, msg.labelgen,
			    cfp->lb.info.lb.gen);
		}
		if (msg.serverord != Host.serverord) {
			Trace(TR_MISC, "FS %s: msg.serverord/expected "
			    "serverord = %d/%d",
			    Host.fs.fi_name, msg.serverord, Host.serverord);
		}
		if (strncasecmp(msg.hname, client, sizeof (msg.hname))) {
			Trace(TR_MISC, "FS %s: msg.hname/expected client "
			    "name = %s/%s",
			    Host.fs.fi_name, msg.hname, client);
		}
		goto reject;
	}

	if (msg.tag_list_size != QFS_TAG_LIST_SZ) {
		SysError(HERE, "FS %s: server and client %s differ in "
		    "tag_list_size; rejecting",
		    Host.fs.fi_name, client);
		Trace(TR_MISC, "FS %s: server and client %s differ in "
		    "tag_list_size",
		    Host.fs.fi_name, client);
		goto reject;
	}

	Trace(TR_MISC, "FS %s: Client %s's sam_majorv: %s",
	    Host.fs.fi_name, client, msg.qfsmajorv);
	Trace(TR_MISC, "FS %s: Client %s's sam_minorv: %s",
	    Host.fs.fi_name, client, msg.qfsminorv);
	Trace(TR_MISC, "FS %s: Client %s's sam_fixv: %s",
	    Host.fs.fi_name, client, msg.qfsfixv);
	Trace(TR_MISC, "FS %s: Client %s's sysname: %s",
	    Host.fs.fi_name, client, msg.sysname);
	Trace(TR_MISC, "FS %s: Client %s's release: %s",
	    Host.fs.fi_name, client, msg.release);
	Trace(TR_MISC, "FS %s: Client %s's machine: %s",
	    Host.fs.fi_name, client, msg.machine);

	Trace(TR_MISC, "FS %s: Client %s's tag_list: %s",
	    Host.fs.fi_name, client, msg.tag_list);

	tagcnt = ProcessBehaviorTags(msg.tag_list, client_tags);
	if (tagcnt != msg.tag_cnt) {
		SysError(HERE, "FS %s: tag count mismatch, found %d, "
		    "expected %d",
		    Host.fs.fi_name, tagcnt, msg.tag_cnt);
		Trace(TR_MISC, "FS %s: found %d tags (%d expected)",
		    Host.fs.fi_name, tagcnt, msg.tag_cnt);
		goto reject;
	}

	Trace(TR_MISC, "FS %s: Client %s's locally understood tag mask: 0x%llx",
	    Host.fs.fi_name, client, (unsigned long long)*client_tags);

	bzero((char *)&reply, sizeof (reply));
	reply.key = VFY_MSG_KEY;
	reply.magic = VFY_MSG_OK;
	SetQFSVer(&reply, Host.fs.fi_name);
	reply.now = time(NULL);
	reply.fsid = cfp->fsInit;
	reply.labelversion = cfp->lb.info.lb.version;
	reply.labelgen = cfp->lb.info.lb.gen;
	reply.serverord = Host.serverord;
	strncpy(reply.hname, msg.hname, sizeof (reply.hname));
	reply.tag_list_size = QFS_TAG_LIST_SZ;
	reply.tag_cnt = 0;
	reply.tag_list[0] = '\0';

	FillBehaviorTags(&reply.tag_cnt, reply.tag_list);

	if ((n = write(sockfd, (char *)&reply, sizeof (reply))) !=
	    sizeof (reply)) {
		SysError(HERE, "FS %s: write to client failed (%d/%d)",
		    Host.fs.fi_name, n, sizeof (reply));
		return (-1);
	}
	return (0);

reject:
	bzero((char *)&reply, sizeof (reply));
	reply.key = VFY_MSG_KEY;
	reply.magic = VFY_MSG_REJECT;
	SetQFSVer(&reply, Host.fs.fi_name);
	reply.now = time(NULL);
	reply.fsid = msg.fsid;
	reply.tag_list_size = QFS_TAG_LIST_SZ;
	reply.tag_cnt = 0;
	reply.tag_list[0] = '\0';

	if ((n = write(sockfd, (char *)&reply, sizeof (reply))) !=
	    sizeof (reply)) {
		SysError(HERE, "FS %s: write to client failed (%d/%d)",
		    Host.fs.fi_name, n, sizeof (reply));
	}
	return (-1);
}


/*
 * VerifyClientCap -- verify client capabilities
 *
 * This is called on the server after a connection from a client
 * is received.  We take the info we get (is the client a client-only
 * host?) and ensure that that doesn't conflict with the shared info
 * (i.e., client-only hosts ought to have a zero in their server
 * priority field).
 *
 * If there is a problem, we complain to the system console.
 */

/* ARGSUSED2 */
void
VerifyClientCap(
	int hostord,		/* host ordinal from shared hosts file */
	int srvrcap,		/* value from VerifyServerSocket */
	int byterev)		/* value from VerifyServerSocket */
{
	char ***hosts, *errmsg;
	int ent, pri, errnum;
	struct cfdata *cfp = &config.cp[config.curTab];

	hosts = SamHostsCvt(&cfp->ht->info.ht, &errmsg, &errnum);
	if (hosts == NULL) {
		Trace(TR_MISC, "FS %s: Couldn't convert hosts file "
		    "(error %d: %s)",
		    Host.fs.fi_name, errnum, errmsg);
		return;
	}
	for (ent = 0; hosts[ent] != NULL; ) {
		ent++;
	}
	if (hostord >= ent) {
		Trace(TR_MISC, "FS %s: Host ordinal out-of-range (%d/%d)",
		    Host.fs.fi_name, hostord, ent);
		goto out;
	}
	if (hosts[hostord][HOSTS_PRI] == NULL) {
		Trace(TR_MISC, "FS %s: Host priority missing from hosts file "
		    "(host %d)",
		    Host.fs.fi_name, hostord);
		goto out;
	}
	pri = atoi(hosts[hostord][HOSTS_PRI]);
	if (pri != 0 && !srvrcap) {
		openlog("sam-sharefsd",
		    LOG_NOWAIT | LOG_CONS | LOG_PID, LOG_DAEMON);
		errno = 0;
		syslog(LOG_ERR,
		    "FS %s: Host %s cannot be server; "
		    "server priority (%d) in shared hosts file should be 0.",
		    Host.fs.fi_name, hosts[hostord][HOSTS_NAME], pri);
		closelog();

		errno = 0;
		SysError(HERE,
		    "FS %s: Host %s cannot be server; "
		    "server priority (%d) in shared hosts file should be 0.",
		    Host.fs.fi_name, hosts[hostord][HOSTS_NAME], pri);
	}

out:
	SamHostsFree(hosts);
}

#endif	/* METADATA_SERVER */


#ifdef	CLUSTER_SVCS

/*
 * Bits and pieces used to support/service shared QFS in a SunCluster
 * environment.
 */


/*
 * SetClusterInfo(char *fsname)
 *
 * Notify the kernel about hosts that are known 'down'.  If all
 * of the remaining hosts connect to the server, early failover
 * completion will be allowed.
 *
 * This function is used in conjunction with SunCluster, which
 * has knowledge of down hosts, and/or can guarantee that the
 * host(s) cannot connect or write to FS disks (e.g., via fencing).
 */
void
SetClusterInfo(char *fs)
{
	signed char *hstate = NULL;
	struct cfdata *cfp = &config.cp[config.curTab];
	struct sam_mount_info *mntp = &config.mnt;
	char ***hosts = NULL;
	char *errmsg = "";
	int errline = 0;
	int i, nhosts, r, nonClusterNode;

	nhosts = cfp->ht->info.ht.count;
	hosts = SamHostsCvt(&cfp->ht->info.ht, &errmsg, &errline);
	if (hosts == NULL) {
		Trace(TR_MISC, "FS %s: SamHostsCvt failed: %s/%d",
		    fs, errmsg, errline);
		return;
	}

	/*
	 * If this node isn't a node in a SunCluster group, then
	 * we return.  If it is, then we look at other options.
	 */
	if ((mntp->params.fi_config1 & MC_CLUSTER_MGMT) == 0) {
		Trace(TR_MISC, "FS %s: Cluster mgmt off", fs);
		goto out;
	}

	nonClusterNode = 0;
	/*
	 * Background:  When there are non-SunCluster nodes that are
	 * party to the shared filesystem, fencing generally cannot
	 * ensure that insane cluster nodes can't get at the shared
	 * disks.  As a result, we want to wait the longest possible
	 * possible time for all nodes to connect.
	 *
	 * This can be overridden in the cluster framework by setting
	 * QFSLeaseRecoveryPolicy to "Cluster".
	 *
	 * So here we query the cluster framework to see about the other
	 * hosts attached to the FS.  If all the hosts belong to the
	 * cluster, then we default to an accelerated failover (wherein
	 * the failover doesn't wait for nodes that are "down" to attach
	 * to the server).  If there are non-cluster hosts in the shared
	 * hosts file, then we default to waiting, regardless of whether
	 * or not all cluster hosts have attached.
	 *
	 * Finally, if MC_CLUSTER_FASTSW is set, then we try for an
	 * accelerated failover anyway.  This is set by the cluster
	 * framework iff QFSLeaseRecoveryPolicy is set to "cluster".
	 */
	hstate = (signed char *)malloc(nhosts * sizeof (signed char));
	if (hstate == NULL) {
		goto out;
	}
	bzero(&hstate[0], nhosts * sizeof (signed char));
	for (i = 0; i < nhosts && hosts[i] && hosts[i][HOSTS_NAME]; i++) {
		hstate[i] = IsClusterNodeUp(hosts[i][HOSTS_NAME]);
		if (hstate[i] < 0) {
			nonClusterNode = 1;
		}
	}

	if (nonClusterNode &&
	    (mntp->params.fi_config1 & MC_CLUSTER_FASTSW) == 0) {
		Trace(TR_MISC, "FS %s: Cluster managed; non-cluster nodes "
		    "present", fs);
		return;
	}

	for (i = 0; i < nhosts && hosts[i] && hosts[i][HOSTS_NAME]; i++) {
		if (hstate[i] == 1) {
			Trace(TR_MISC, "FS %s: host '%s': DOWN",
			    fs, hosts[i][HOSTS_NAME]);
			r = sam_shareops(fs, SHARE_OP_HOST_INOP, i+1);
			if (r < 0) {
				SysError(HERE, "FS %s: "
				    "sam_shareops('%s', HOST_INOP, '%s') "
				    "failed",
				    Host.fs.fi_name, Host.fs.fi_name,
				    hosts[i][HOSTS_NAME]);
			}
		}
	}

out:
	if (hosts) {
		SamHostsFree(hosts);
	}
	if (hstate) {
		free((void *)hstate);
	}
}


/*
 * Used by QFS sam-sharefsd daemon to check the status
 * of SunCluster nodes.
 * Uses scha_cluster_get(SCHA_NODESTATE_NODE) API
 * to do its job.
 *
 * Wraps up the call in a dlopen() to avoid
 * QFS having run-time dependency on SunCluster.
 *
 * Used by QFS during failover to avoid having to
 * wait for client leases to expire for clients
 * which are known to be DOWN.
 *
 */
#include <scha.h>
#include <dlfcn.h>


typedef int (*CL_OPEN_FUNC)(scha_cluster_t *handle);
typedef int (*CL_GET_FUNC)(scha_cluster_t *handle,
		char *optag, char *hostname, scha_node_state_t *st);
typedef char *(*CL_ERR_FUNC)(scha_err_t e);

/*
 * Return values
 * 0 - Node is UP
 * 1 - Node is DOWN
 * -1, not a cluster node, machine is not a cluster,
 * booted in non-cluster mode or other some error.
 * The intent is that when this function returns non-zero, QFS
 * falls back on its usual algorithm of making sure the
 * node has reset its client leases, if any and wait till
 * the lease expiration time if the client does not get
 * back.
 *
 * Keeps a static handle to the pointer looked up via
 * dlopen(), as well as the Cluster handle opened
 * via scha_cluster_open().
 *
 * Only cost of this function on a non-Cluster system is
 * that of a failed call to /usr/sbin/clinfo, which ought to be
 * pretty quick. We save the result in a static variable
 * so that this is done only once too.
 */

int
IsClusterNodeUp(char *nodename)
{
	static CL_OPEN_FUNC d_cl_open = NULL;
	static CL_GET_FUNC d_cl_get = NULL;
	static CL_ERR_FUNC d_cl_err = NULL;
	static void *dlptr = NULL;
	static scha_cluster_t  cl = NULL;
	static int	am_i_a_cluster = -1;

	scha_err_t			e;
	scha_node_state_t	nstate;

	if (am_i_a_cluster == 1) {
		/* Have already checked, we are not a cluster */
		return (-1);
	}
	if (am_i_a_cluster < 0) {
		FILE *clfp;
		struct stat statbuf;

		/* quick check for SC currently active on this node */
		if ((stat("/var/run/scrpc", &statbuf) < 0) &&
		    ((stat("/var/run/rgmd_receptionist_door", &statbuf)) < 0) &&
		    ((stat("/var/run/rgmd_receptionist_doorglobal", &statbuf))
		    < 0)) {
			Trace(TR_MISC,
			    "FS %s: Not cluster or RGM unstarted",
			    Host.fs.fi_name);
			return (-1);
		}
		/* We have not checked if we are a cluster */
		if ((clfp = popen("/usr/sbin/clinfo", "w")) == NULL) {
			Trace(TR_MISC,
			    "FS %s: Not in cluster, clinfo not avail",
			    Host.fs.fi_name);
			return (-1);
		}
		am_i_a_cluster = pclose(clfp);
	}

	/* clinfo returns 0 if this host is a cluster node, 1 if not */
	if (am_i_a_cluster != 0) {
		Trace(TR_MISC,
		    "FS %s: Not in cluster\n", Host.fs.fi_name);
		return (-1);
	}

	/* Look up the Cluster API functions we need */
	if (dlptr == NULL) {
		dlptr = dlopen("/usr/cluster/lib/libscha.so", RTLD_LAZY);
		if (dlptr == NULL) {
			Trace(TR_MISC,
			    "FS %s: SunCluster API Library not found "
			    "(not a suncluster node?): %s",
			    Host.fs.fi_name, strerror(errno));
			return (-1);
		}
	}
	if (d_cl_open == NULL) {
		d_cl_open = (CL_OPEN_FUNC) dlsym(dlptr, "scha_cluster_open");
		if (d_cl_open == NULL) {
			/* This should not happen */
			Trace(TR_MISC, "FS %s: Unable to resolve "
			    "scha_cluster_open",
			    Host.fs.fi_name);
			return (-1);
		}
	}
	if (d_cl_get == NULL) {
		d_cl_get = (CL_GET_FUNC) dlsym(dlptr, "scha_cluster_get");
		if (d_cl_get == NULL) {
			/* This should not happen */
			Trace(TR_MISC, "FS %s: Unable to resolve "
			    "scha_cluster_get",
			    Host.fs.fi_name);
			return (-1);
		}
	}
	if (d_cl_err == NULL) {
		d_cl_err = (CL_ERR_FUNC) dlsym(dlptr, "scha_strerror");
		if (d_cl_err == NULL) {
			/* This should not happen */
			Trace(TR_MISC, "FS %s: Unable to resolve "
			    "scha_strerror",
			    Host.fs.fi_name);
			return (-1);
		}
	}
	/* Establish a new handle if don't have one cached */
	if (cl == NULL) {
		if ((e = d_cl_open(&cl)) != SCHA_ERR_NOERR) {
			Trace(TR_MISC, "FS %s: Failed to open cluster "
			    "handle: %s",
			    Host.fs.fi_name, d_cl_err(e));
			cl = NULL;
			return (-1);
		}
	}
	if ((e = d_cl_get(cl, SCHA_NODESTATE_NODE, nodename,
	    &nstate)) != SCHA_ERR_NOERR) {
		Trace(TR_MISC, "FS %s: Get status of node %s failed: %s",
		    Host.fs.fi_name, nodename, d_cl_err(e));
		return (-1);
	}

	if (nstate == SCHA_NODE_UP) {
		return (0);
	} else {
		return (1);
	}
}

#else	/* CLUSTER_SVCS */

/*ARGSUSED*/
int
IsClusterNodeUp(char *nodename)
{
	return (-1);
}


/*ARGSUSED*/
void
SetClusterInfo(char *fs)
{
}

#endif	/* CLUSTER_SVCS */
