/*
 * sam-init/samfm_service.c - StorADE API.
 *
 * Receive a XML message request for a list dev_ent_t devices, process
 * the request, and send a XML message in response.
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sched.h>
#include <thread.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pkginfo.h>
#include <syslog.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <dlfcn.h>
#include <libxml/parser.h>
#include "sam/spm.h"
#include "sam/types.h"
#include "sam/lib.h"
#include "aml/shm.h"
#include "aml/device.h"
#include "sam/devnm.h"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#define	HERE _SrcFile, __LINE__ /* Sam syslog debug info. */

/*
 * Tape drive proprites - drive, fw, cartridge.
 */
typedef enum prop_type {
	PROP_TYPE_DRIVE,	 /* drive */
	PROP_TYPE_CAPABLE,	/* firmware */
	PROP_TYPE_MEDIA	  /* cartridge */
} prop_type_t;

/*
 * Message ids representing the xml id string.
 */
typedef enum msg_id {
	MSG_ID_DEVENT	    /* get shared memory device attributes */
} msg_id_t;

/*
 * Message structure for received xml message.
 */
typedef struct msg {
	msg_id_t id;		/* which message */
	int error;		/* xml parse error */
} msg_t;

/*
 * XML parser errors.
 */
typedef enum xml_error {
	XML_ERROR_NONE,	  /* no error */
	XML_ERROR_ID,	    /* message id */
	XML_ERROR_ATT,	   /* element attribute */
	XML_ERROR_VAL,	   /* element value */
	XML_ERROR_TAG,	   /* tag */
	XML_ERROR_FLAGS	  /* missing attribute value pair */
} xml_error_t;

/*
 * Gnome XML shared library function pointers.
 */
typedef void (*xmlinitfptr_t)(void);
typedef int (*xmlparserfptr_t)(xmlSAXHandlerPtr, void *, const char *, int);

/*
 * Global memory.
 */
static shm_alloc_t master_shm;
static xmlinitfptr_t xmlinit;
static xmlparserfptr_t xmlparser;

/*
 * External function prototypes.
 */

/*
 * Local function prototypes.
 */
static char *strapp(char *str, char *fmt, ...);
static char *vstrapp(char *str, char *fmt, va_list args);
static char *get_prop(properties_t prop, prop_type_t type);
static char *get_bool(int val);
static char *get_msg_header(char *id, char *status);
static char *get_devent_msg_body(dev_ent_t *un);
static char *get_msg_trailer(void);
static char *get_response_msg(void);
static char *read_msg(int fd);
static void send_msg(int fd, char *xml);
static void send_msg_fail(int fd, char *id);
static void msg_start_elements(
    void *userData, const xmlChar *name, const xmlChar **atts);
static int parse_msg(char *xml, msg_t *msg);
static void process_msg(int fd);
void *samfm_service(void *arg);

/*
 * Append string with formatted data.
 *
 *	Returns NULL if error, pointer if successful.
 */
static char *
strapp(
	char *str,
	char *fmt,
	...)
{
	va_list args;
	char    *ptr;

	if (fmt == NULL) {
		sam_syslog(LOG_DEBUG, "%s:%d strapp fmt null.", HERE);
		return (NULL);
	}

	va_start(args, fmt);

	ptr = vstrapp(str, fmt, args);

	va_end(args);

	return (ptr);
}

/*
 * Variable argument list append string.
 *
 *	Returns NULL if error, pointer if successful.
 */
static char *
vstrapp(
	char *str,
	char *fmt,
	va_list args)
{
#define	TMP_LEN 10
	int	count;
	int	offset;
	char    *ptr;
	char    tmp[TMP_LEN];

	if (fmt == NULL) {
		sam_syslog(LOG_DEBUG, "%s:%d vstrapp fmt null.", HERE);
		return (NULL);
	}

	if ((count = vsnprintf(tmp, TMP_LEN, fmt, args)) == -1) {
		sam_syslog(LOG_DEBUG, "%s:%d vstrapp find count error #%d",
		    HERE, errno);
		return (NULL);
	}

	if (str == NULL) {
		if ((ptr = (char *)malloc(count+1)) == NULL) {
			sam_syslog(LOG_DEBUG,
			    "%s:%d vstrapp count %d malloc error #%d",
			    HERE, count+1, errno);
			return (NULL);
		}
		ptr[0] = '\0';
		offset = 0;
	} else {
		if ((ptr = (char *)realloc(str, strlen(str)+count+1)) ==
		    NULL) {
			sam_syslog(LOG_DEBUG,
			    "%s:%d vstrapp count %d realloc error #%d",
			    HERE, strlen(str)+count+1, errno);
			return (NULL);
		}
		offset = strlen(str);
	}

	(void) vsprintf(ptr+offset, fmt, args);

	return (ptr);
}

/*
 * Get tape drive property string for drive, fw or media.
 *
 *	Returns Regular if error, property type if successful.
 */
static char *
get_prop(
	properties_t prop,
	prop_type_t type)
{
	char	*p;
	static char	*regular = "regular";
	static char	*volsafe = "volsafe";
	static char	*worm = "worm";

	p = regular;
	switch (type) {
	case PROP_TYPE_DRIVE:
		if (prop & PROPERTY_WORM_DRIVE) {
			p = worm;
		} else if (prop & PROPERTY_VOLSAFE_DRIVE) {
			p = volsafe;
		}
		break;
	case PROP_TYPE_CAPABLE:
		if (prop & PROPERTY_WORM_CAPABLE) {
			p = worm;
		} else if (prop & PROPERTY_VOLSAFE_CAPABLE) {
			p = volsafe;
		}
		break;
	case PROP_TYPE_MEDIA:
		if (prop & PROPERTY_WORM_MEDIA) {
			p = worm;
		} else if (prop & PROPERTY_VOLSAFE_MEDIA) {
			p = volsafe;
		}
		break;
	default:
		sam_syslog(LOG_DEBUG,
		    "%s:%d tape property conversion type %d error.",
		    HERE, type);
	}

	return (p);
}

/*
 * Convert boolean value to boolean string value.
 *
 *	Returns True if error, bool string if successful.
 */
static char *
get_bool(
	int val)
{
	static char	*false = "false";
	static char	*true = "true";

	return (val ? true : false);
}

/*
 * Build response message start.
 *
 *	Returns NULL if error, XML message header if successful.
 */
static char *
get_msg_header(
	char *id,
	char *status)
{
	char	*xml;
	static char	*version = NULL;
	static char	*patch_id = NULL;
	static char	*none = "none";

	if (version == NULL) {
		if ((version = pkgparam("SUNWsamfsr", "VERSION")) == NULL) {
			sam_syslog(LOG_DEBUG, "%s:%d pkg version query failed.",
			    HERE);
			return (NULL);
		}
	}

	if (patch_id == NULL) {
		if ((patch_id = pkgparam("SUNWsamfsr", "SUNW_PATCHID")) ==
		    NULL) {
			patch_id = none;
		}
	}

	xml = strapp(0, "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n");
	xml = strapp(xml,
	    "<!DOCTYPE message SYSTEM \"/opt/SUNWsamfs/doc/message.dtd\">\n");
	xml = strapp(xml, "\n");
	xml = strapp(xml, "<message\n");
	xml = strapp(xml, "  id=\"%s\"\n", id);
	xml = strapp(xml, "  version=\"1.0\">\n");
	xml = strapp(xml, "  <msg_response\n");
	xml = strapp(xml, "    status=\"%s\">\n", status);
	xml = strapp(xml, "<samfm>\n");
	xml = strapp(xml, "  <pkg\n");
	xml = strapp(xml, "    version=\"%s\"\n", version);
	xml = strapp(xml, "    patch_id=\"%s\"/>\n", patch_id);

	return (xml);
}

/*
 * Build device shared memory message body.
 *
 *	Returns NULL if error, XML message if successful.
 */
static char *
get_devent_msg_body(
	dev_ent_t *un)
{
	char	*xml;
	int	i;
	char	*str;

	xml = strapp(0, "  <devent\n");
	xml = strapp(xml, "    name=\"%s\"\n", un->name);
	xml = strapp(xml, "    set=\"%s\"\n", un->set);
	xml = strapp(xml, "    eq=\"%u\"\n", un->eq);
	xml = strapp(xml, "    fseq=\"%u\"\n", un->fseq);
	xml = strapp(xml, "    type=\"%u\"\n", un->type);
	str = "\0";
	for (i = 0; dev_nmtp[i] != NULL; i++) {
		if (i == (un->type & DT_MEDIA_MASK)) {
			str = dev_nmtp[i];
			break;
		}
	}
	xml = strapp(xml, "    media_type=\"%s\"\n", str);
	xml = strapp(xml, "    equ_type=\"%u\"\n", un->equ_type);
	xml = strapp(xml, "    state=\"%s\"\n", dev_state[un->state]);
	xml = strapp(xml, "    ord=\"%u\"\n", un->ord);
	xml = strapp(xml, "    media=\"%u\"\n", un->media);
	xml = strapp(xml, "    delay=\"%u\"\n", un->delay);
	xml = strapp(xml, "    unload_delay=\"%u\"\n", un->unload_delay);
	xml = strapp(xml, "    label_time=\"%d\"\n", un->label_time);
	xml = strapp(xml, "    slot=\"%u\"\n", un->slot);
	xml = strapp(xml, "    space=\"%llu\"\n", un->space);
	xml = strapp(xml, "    capacity=\"%llu\"\n", un->capacity);
	xml = strapp(xml, "    sector_size=\"%u\"\n", un->sector_size);
	xml = strapp(xml, "    vsn=\"%s\"\n", un->vsn);
	xml = strapp(xml, "    vendor=\"%s\"\n", un->vendor_id);
	xml = strapp(xml, "    product=\"%s\"\n", un->product_id);
	xml = strapp(xml, "    revision=\"%s\"\n", un->revision);
	xml = strapp(xml, "    serial=\"%s\"\n", un->serial);
	xml = strapp(xml, "    scsi_type=\"%d\"\n", un->scsi_type);
	xml = strapp(xml, "    version=\"%d\"\n", un->version);
	xml = strapp(xml, "    devlog_flags=\"0x%x\">\n", un->log.flags);
	xml = strapp(xml, "    <dt>\n");
	if (IS_TAPE(un)) {
	xml = strapp(xml, "      <tp\n");
	xml = strapp(xml, "        position=\"%u\"\n", un->dt.tp.position);
	xml = strapp(xml, "        stage_pos=\"%u\"\n", un->dt.tp.stage_pos);
	xml = strapp(xml, "        next_read=\"%u\"\n", un->dt.tp.next_read);
	xml = strapp(xml, "        default_blocksize=\"%u\"\n",
	    un->dt.tp.default_blocksize);
	xml = strapp(xml, "        position_timeout=\"%u\"\n",
	    un->dt.tp.position_timeout);
	xml = strapp(xml, "        max_blocksize=\"%u\"\n",
	    un->dt.tp.max_blocksize);
	xml = strapp(xml, "        default_capacity=\"%llu\"\n",
	    un->dt.tp.default_capacity);
	xml = strapp(xml, "        samst_name=\"%s\"\n", un->dt.tp.samst_name);
	xml = strapp(xml, "        medium_type=\"%u\">\n",
	    un->dt.tp.medium_type);
	xml = strapp(xml, "        <tpstatus\n");
	xml = strapp(xml, "          fix_block_mode=\"%s\"\n",
	    get_bool(un->dt.tp.status.b.fix_block_mode));
	xml = strapp(xml, "          compression=\"%s\"\n",
	    get_bool(un->dt.tp.status.b.compression));
	xml = strapp(xml, "          needs_format=\"%s\"/>\n",
	    get_bool(un->dt.tp.status.b.needs_format));
	xml = strapp(xml, "        <properties\n");
	xml = strapp(xml, "          drive=\"%s\"\n",
	    get_prop(un->dt.tp.properties, PROP_TYPE_DRIVE));
	xml = strapp(xml, "          capable=\"%s\"\n",
	    get_prop(un->dt.tp.properties, PROP_TYPE_CAPABLE));
	xml = strapp(xml, "          media=\"%s\"/>\n",
	    get_prop(un->dt.tp.properties, PROP_TYPE_MEDIA));
	xml = strapp(xml, "      </tp>\n");
	} else if (IS_OPTICAL(un)) {
	xml = strapp(xml, "      <od\n");
	xml = strapp(xml, "        medium_type=\"%u\"\n",
	    un->dt.od.medium_type);
	xml = strapp(xml, "        flip=\"%s\"/>\n", get_bool(un->flip_mid));
	} else if (IS_ROBOT(un)) {
	xml = strapp(xml, "      <rb\n");
	xml = strapp(xml, "        name=\"%s\"\n", un->dt.rb.name);
	xml = strapp(xml, "        port_num=\"%u\"\n", un->dt.rb.port_num);
	xml = strapp(xml, "        capid=\"%d\">\n", un->dt.rb.capid);
	xml = strapp(xml, "        <rbstatus\n");
	xml = strapp(xml, "          barcodes=\"%s\"\n",
	    get_bool(un->dt.rb.status.b.barcodes));
	xml = strapp(xml, "          export_unavail=\"%s\"\n",
	    get_bool(un->dt.rb.status.b.export_unavail));
	xml = strapp(xml, "          shared_access=\"%s\"/>\n",
	    get_bool(un->dt.rb.status.b.shared_access));
	xml = strapp(xml, "      </rb>\n");
	} /* IS_ROBOT(un) */
	xml = strapp(xml, "    </dt>\n");
	xml = strapp(xml, "    <status\n");
	xml = strapp(xml, "      maint=\"%s\"\n", get_bool(un->status.b.maint));
	xml = strapp(xml, "      scan_err=\"%s\"\n",
	    get_bool(un->status.b.scan_err));
	xml = strapp(xml, "      audit=\"%s\"\n", get_bool(un->status.b.audit));
	xml = strapp(xml, "      attention=\"%s\"\n",
	    get_bool(un->status.b.attention));
	xml = strapp(xml, "      scanning=\"%s\"\n",
	    get_bool(un->status.b.scanning));
	xml = strapp(xml, "      mounted=\"%s\"\n",
	    get_bool(un->status.b.mounted));
	xml = strapp(xml, "      scanned=\"%s\"\n",
	    get_bool(un->status.b.scanned));
	xml = strapp(xml, "      read_only=\"%s\"\n",
	    get_bool(un->status.b.read_only));
	xml = strapp(xml, "      labeled=\"%s\"\n",
	    get_bool(un->status.b.labeled));
	xml = strapp(xml, "      wr_lock=\"%s\"\n",
	    get_bool(un->status.b.wr_lock));
	xml = strapp(xml, "      unload=\"%s\"\n",
	    get_bool(un->status.b.unload));
	xml = strapp(xml, "      requested=\"%s\"\n",
	    get_bool(un->status.b.requested));
	xml = strapp(xml, "      opened=\"%s\"\n",
	    get_bool(un->status.b.opened));
	xml = strapp(xml, "      ready=\"%s\"\n", get_bool(un->status.b.ready));
	xml = strapp(xml, "      present=\"%s\"\n",
	    get_bool(un->status.b.present));
	xml = strapp(xml, "      bad_media=\"%s\"\n",
	    get_bool(un->status.b.bad_media));
	xml = strapp(xml, "      stor_full=\"%s\"\n",
	    get_bool(un->status.b.stor_full));
	xml = strapp(xml, "      i_e_port=\"%s\"\n",
	    get_bool(un->status.b.i_e_port));
	xml = strapp(xml, "      cleaning=\"%s\"\n",
	    get_bool(un->status.b.cleaning));
	xml = strapp(xml, "      positioning=\"%s\"\n",
	    get_bool(un->status.b.positioning));
	xml = strapp(xml, "      forward=\"%s\"\n",
	    get_bool(un->status.b.forward));
	xml = strapp(xml, "      wait_idle=\"%s\"\n",
	    get_bool(un->status.b.wait_idle));
	xml = strapp(xml, "      fs_active=\"%s\"\n",
	    get_bool(un->status.b.fs_active));
	xml = strapp(xml, "      write_protect=\"%s\"\n",
	    get_bool(un->status.b.write_protect));
	xml = strapp(xml, "      strange=\"%s\"\n",
	    get_bool(un->status.b.strange));
	xml = strapp(xml, "      stripe=\"%s\"\n",
	    get_bool(un->status.b.stripe));
	xml = strapp(xml, "      labelling=\"%s\"/>\n",
	    get_bool(un->status.b.labelling));
	xml = strapp(xml, "    <devid\n");
	xml = strapp(xml, "      multiport=\"%s\"\n",
	    get_bool(un->devid.multiport));
	xml = strapp(xml, "      port_id_valid=\"%s\"\n",
	    get_bool(un->devid.port_id_valid));
	xml = strapp(xml, "      port_id=\"%d\">\n", un->devid.port_id);
	for (i = 0; i < un->devid.count; i++) {
	xml = strapp(xml, "      <ident_data\n");
	xml = strapp(xml, "        ident=\"%s\"\n", un->devid.data[i].ident);
	xml = strapp(xml, "        assoc=\"%d\"\n", un->devid.data[i].assoc);
	xml = strapp(xml, "        type=\"%d\"/>\n", un->devid.data[i].type);
	}
	xml = strapp(xml, "      <protocol\n");
	xml = strapp(xml, "        lun_valid=\"%s\"\n",
	    get_bool(un->devid.protocol.lun_valid));
	xml = strapp(xml, "        lun=\"%d\"\n", un->devid.protocol.lun);
	xml = strapp(xml, "        port_valid=\"%s\"\n",
	    get_bool(un->devid.protocol.port_valid));
	xml = strapp(xml, "        port=\"%d\"/>\n", un->devid.protocol.port);
	xml = strapp(xml, "    </devid>\n");
	xml = strapp(xml, "    <tapealert\n");
	xml = strapp(xml, "      supported=\"%s\"\n",
	    get_bool(un->tapealert & TAPEALERT_SUPPORTED));
	xml = strapp(xml, "      enabled=\"%s\"\n",
	    get_bool(un->tapealert & TAPEALERT_ENABLED));
	xml = strapp(xml, "      flags=\"0x%016llx\"/>\n", un->tapealert_flags);
	xml = strapp(xml, "    <sef\n");
	xml = strapp(xml, "      supported=\"%s\"\n",
	    get_bool(un->sef_sample.state & SEF_SUPPORTED));
	xml = strapp(xml, "      enabled=\"%s\"\n",
	    get_bool(un->sef_sample.state & SEF_ENABLED));
	xml = strapp(xml, "      interval=\"%d\"\n", un->sef_sample.interval);
	xml = strapp(xml, "      counter=\"%d\"\n", un->sef_sample.counter);
	xml = strapp(xml, "      state=\"%x\"/>\n", un->sef_sample.state);
	xml = strapp(xml, "  </devent>\n");

	return (xml);
}

/*
 * Build response message end.
 *
 *	Returns NULL if error, XML message if successful.
 */
static char *
get_msg_trailer(void)
{
	char	*xml;

	xml = strapp(0, "</samfm>\n");
	xml = strapp(xml, "  </msg_response>\n");
	xml = strapp(xml, "</message>\n");

	return (xml);
}

/*
 * Build response message.
 *
 *	Returns NULL if error, XML message if successful.
 */
static char *
get_response_msg(void)
{
	char	*msg;
	char	*body;
	char	*trailer;
	dev_ent_t *device;

	if ((msg = get_msg_header("devent_complete", "successful")) == NULL) {
		sam_syslog(LOG_DEBUG, "%s:%d build msg header failed.", HERE);
		return (NULL);
	}

	device = (dev_ent_t *)SHM_REF_ADDR(
	    ((shm_ptr_tbl_t *)master_shm.shared_memory)->first_dev);

	while (device != NULL) {
		if (device->scsi_type == 1 || device->scsi_type == 8 ||
		    device->scsi_type == 7) {
			if ((body = get_devent_msg_body(device)) == NULL) {
				free(msg);
				sam_syslog(LOG_DEBUG,
				    "%s:%d build msg body failed.", HERE);
				return (NULL);
			}
			msg = strapp(msg, "%s", body);
			free(body);
			if (msg == NULL) {
				sam_syslog(LOG_DEBUG,
				    "%s:%d msg body append failed.", HERE);
				return (NULL);
			}
		}
		device = (dev_ent_t *)SHM_REF_ADDR(device->next);
	}

	if ((trailer = get_msg_trailer()) == NULL) {
		free(msg);
		sam_syslog(LOG_DEBUG, "%s:%d build msg trailer failed.", HERE);
		return (NULL);
	}

	msg = strapp(msg, "%s", trailer);
	free(trailer);
	if (msg == NULL) {
		sam_syslog(LOG_DEBUG, "%s:%d msg trailer append failed.", HERE);
		return (NULL);
	}

	return (msg);
}

/*
 * Read client input message.
 *
 *	Returns NULL if error, XML message if successful.
 */
static char *
read_msg(
	int fd)
{
#define	FIFTHTEEN_SEC_TIMEOUT 15000
	char	*xml;
	char	buf[100];
	int	cnt;
	int	bytes;
	struct pollfd fds[1];

	fds[0].fd = fd;
	fds[0].events = POLLIN;
	fds[0].revents = 0;

	cnt = poll(fds, 1, FIFTHTEEN_SEC_TIMEOUT);
	if (cnt != 1 && fds[0].revents != POLLIN < 0) {
		sam_syslog(LOG_DEBUG,
		    "%s:%d poll cnt %d error #%d fd %d revents %x failure.",
		    HERE, cnt, errno, fd, fds[0].revents);
		return (NULL);
	}

	xml = NULL;
	do {
		bytes = read(fd, buf, 100);
		if (bytes == -1) {
			free(xml);
			sam_syslog(LOG_DEBUG,
			    "%s:%d read error #%d fd %d failed.",
			    HERE, errno, fd);
			return (NULL);
		}

		if ((xml = strapp(xml, "%s", buf)) == NULL) {
			sam_syslog(LOG_DEBUG,
			    "%s:%d read fd %d append failure.", HERE, fd);
			return (NULL);
		}

	} while (bytes == 100);

	return (xml);
}

/*
 * Send response message.
 */
static void
send_msg(
	int fd,
	char *xml)
{
	char	*p;
	int	len;
	int	bytes;
	int	total;

	for (total = 0; total < strlen(xml)+1; total += bytes) {
		p = &xml[total];
		if (strlen(p)+1 >= 100) {
			len = 100;
		} else {
			len = strlen(p)+1;
		}
		if ((bytes = write(fd, p, len)) == -1 || bytes != len) {
			sam_syslog(LOG_DEBUG,
			    "%s:%d write bytes %d error #%d fd %d failed.",
			    HERE, bytes, errno, fd);
			return;
		}
	}
}

/*
 * Send response failure message.
 */
static void
send_msg_fail(
	int fd,
	char *id)
{
	char	*msg;
	char	*end;

	if ((msg = get_msg_header(id, "unsuccessful")) == NULL) {
		sam_syslog(LOG_DEBUG,
		    "%s:%d build msg unsuccessful header failed.", HERE);
		return;
	} else if ((end = get_msg_trailer()) == NULL) {
		free(msg);
		sam_syslog(LOG_DEBUG,
		    "%s:%d build msg unsuccessful trailer failed.", HERE);
		return;
	}

	msg = strapp(msg, "%s", end);
	free(end);
	if (msg == NULL) {
		sam_syslog(LOG_DEBUG,
		    "%s:%d msg unsuccessful trailer append failure.", HERE);
		return;
	}

	send_msg(fd, msg);

	free(msg);
}

/*
 * Parse XML tag attributes.
 */
static void
msg_start_elements(
	void *userData,
	const xmlChar *name,
	const xmlChar **atts)
{
	int	i;
	msg_t	*msg = (msg_t *)userData;
	uint_t	flags = 0;

	if (msg->error != XML_ERROR_NONE) {
		return;
	}

	if (strcmp((char *)name, "message") == 0) {
		for (i = 0; atts[i] != NULL; i += 2) {
			if (strcmp((char *)atts[i], "id") == 0) {
				if (strcmp((char *)atts[i+1], "devent") ==
				    0) {
					msg->id = MSG_ID_DEVENT;
					flags |= 0x1;
				} else {
					msg->error = XML_ERROR_ID;
					sam_syslog(LOG_DEBUG,
					    "%s:%d xml parse failed at tag %s"
					    " att %s val %s",
					    HERE, name, atts[i], atts[i+1]);
				}
			} else if (strcmp((char *)atts[i], "version") == 0) {
				if (strcmp((char *)atts[i+1], "1.0") == 0) {
					flags |= 0x2;
				} else {
					msg->error = XML_ERROR_VAL;
					sam_syslog(LOG_DEBUG,
					    "%s:%d xml parse failed at tag %s"
					    " att %s val %s",
					    HERE, name, atts[i], atts[i+1]);
				}
			} else {
				msg->error = XML_ERROR_ATT;
				sam_syslog(LOG_DEBUG,
				    "%s:%d xml parse failed at tag %s att %s",
				    HERE, name, atts[i]);
			}
		}
		if (!msg->error && flags != 0x3) {
			msg->error = XML_ERROR_FLAGS;
			sam_syslog(LOG_DEBUG,
			    "%s:%d xml parse failed at tag %s flags %x",
			    HERE, name, flags);
		}
	} else {
		msg->error = XML_ERROR_TAG;
		sam_syslog(LOG_DEBUG, "%s:%d xml parse failed at tag %s",
		    HERE, name);
	}
}

/*
 * Parse input message using the solaris gnome xml parser.
 */
static int /* 1 if error, 0 if successful. */
parse_msg(
	char *xml,
	msg_t *msg)
{
	int	rtn;
	xmlSAXHandler handler;

	xmlinit();

	(void) memset(&handler, 0, sizeof (xmlSAXHandler));

	handler.startElement = msg_start_elements;

	rtn = xmlparser(&handler, (void*)msg, xml, strlen(xml));
	if (rtn != 0) {
		sam_syslog(LOG_DEBUG, "%s:%d xml parser rtn error %d",
		    HERE, rtn);
	} else if (msg->error) {
		sam_syslog(LOG_DEBUG, "%s:%d xml parser error %d",
		    HERE, msg->error);
		rtn = 1;
	}

	return (rtn);
}

/*
 * Process input message and send a response message.
 */
static void
process_msg(
	int fd)
{
	char	*recv_msg;
	char	*rsp_msg;
	msg_t	msg;
	int	rtn;

	if ((recv_msg = read_msg(fd)) == NULL) {
		send_msg_fail(fd, "devent_complete");
		return;
	}

	(void) memset(&msg, 0, sizeof (msg_t));
	rtn = parse_msg(recv_msg, &msg);
	free(recv_msg);
	if (rtn != 0) {
		send_msg_fail(fd, "devent_complete");
		return;
	}

	switch (msg.id) {
	case MSG_ID_DEVENT:
		if ((rsp_msg = get_response_msg()) != NULL) {
			send_msg(fd, rsp_msg);
			free(rsp_msg);
		} else {
			send_msg_fail(fd, "devent_complete");
		}
		break;

	default:
		sam_syslog(LOG_DEBUG, "%s:%d unknown msg id %d", HERE, msg.id);
		send_msg_fail(fd, "devent_complete");
		break;
	}
}

/*
 * SAMFM service thread.
 */
void * /* Always returns NULL. */
	samfm_service(
	void *arg)
{
	int	error;
	int	client_fd;
	int	service_fd;
	char	ebuf[SPM_ERRSTR_MAX];
	void	*xmlhandle = arg;

	if ((xmlinit = (xmlinitfptr_t)dlsym(
	    xmlhandle, "xmlDefaultSAXHandlerInit")) == NULL) {
		sam_syslog(LOG_INFO, "%s:%d xml parser dl error %s",
		    HERE, dlerror());
		return (NULL);
	}

	if ((xmlparser = (xmlparserfptr_t)dlsym(
	    xmlhandle, "xmlSAXUserParseMemory")) == NULL) {
		sam_syslog(LOG_INFO, "%s:%d xml parser dl error %s",
		    HERE, dlerror());
		return (NULL);
	}

	if ((master_shm.shmid = shmget(SHM_MASTER_KEY, 0, 0464)) == -1) {
		sam_syslog(LOG_INFO, "%s:%d shared memory shmget error #%d",
		    HERE, errno);
		return (NULL);
	}

	if ((master_shm.shared_memory =
	    (void*)shmat(master_shm.shmid, (void *)NULL, 0464)) == (void*)-1) {
		sam_syslog(LOG_INFO, "%s:%d shared memory shmat error #%d",
		    HERE, errno);
		return (NULL);
	}

	if ((service_fd = spm_register_service("samfm", &error, ebuf)) < 0) {
		sam_syslog(LOG_INFO,
		    "%s:%d service register error #%d, error buf %s",
		    HERE, error, ebuf);
		return (NULL);
	}

	sam_syslog(LOG_INFO, "Sun StorADE API Available");

	for (;;) {
		if ((client_fd = spm_accept(service_fd, &error, ebuf)) < 0) {
			sam_syslog(LOG_DEBUG,
			    "%s:%d service socket accept %d %s",
			    HERE, error, ebuf);
			if (sleep(5) != 0) {
				sam_syslog(LOG_INFO,
				    "%s:%d service recovery failed.", HERE);
				return (NULL);
			}
			continue;
		}

		process_msg(client_fd);
		(void) close(client_fd);
	}
}
