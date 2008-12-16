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
#pragma ident   "$Revision: 1.7 $"

static char *_SrcFile = __FILE__;  /* Using __FILE__ makes duplicate strings */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <strings.h>
#include <time.h>
#include <sys/utsname.h>
#include <sys/systeminfo.h>
#include "csn/scrkd.h"
#include "scrkd_xml_string.h"
#include "scrkd_xml.h"


/* Common XML tags */
const char *CM_ELEM_HEADER = \
	"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";
const char *CM_ELEM_MSGXSD		= "<message>";
const char *CM_ELEM_MSGXSD_END		= "</message>\n";
const char *CM_ELEM_SITEID		= "<site-id>";
const char *CM_ELEM_SITEID_END		= "</site-id>\n";
const char *CM_ELEM_HOSTID		= "<host-id>";
const char *CM_ELEM_HOSTID_END		= "</host-id>\n";
const char *CM_ELEM_SYSTEM		= "<system-id>";
const char *CM_ELEM_SYSTEM_END		= "</system-id>\n";
const char *CM_ELEM_ASSET		= "<asset-id>";
const char *CM_ELEM_ASSET_END		= "</asset-id>\n";
const char *CM_ELEM_PRODID		= "<product-id>";
const char *CM_ELEM_PRODID_END		= "</product-id>\n";
const char *CM_ELEM_PRODNAME		= "<product-name>";
const char *CM_ELEM_PRODNAME_END	= "</product-name>\n";
const char *CM_ELEM_PRCONT		= "<primary-contact>";
const char *CM_ELEM_PRCONT_END		= "</primary-contact>\n";
const char *CM_ELEM_COMPANY		= "<company>";
const char *CM_ELEM_COMPANY_END		= "</company>\n";
const char *CM_ELEM_CONTACT		= "<first-name>";
const char *CM_ELEM_CONTACT_END		= "</first-name>\n";
const char *CM_ELEM_PHONE		= "<phone>";
const char *CM_ELEM_PHONE_END		= "</phone>\n";
const char *CM_ELEM_CDATA		= "<![CDATA[\n";
const char *CM_ELEM_CDATA_END		= "]]>\n";

/* Product Registration XML Tags */
const char *PR_ELEM_PRODREG	= \
	"<product-registration xmlns=\"http://sunconnection.sun.com/xml\">\n";
const char *PR_ELEM_PRODREG_END		= "</product-registration>\n";
const char *PR_ELEM_PRODUCT		= "<product>\n";
const char *PR_ELEM_PRODUCT_END		= "</product>\n";
const char *PR_ELEM_NAME		= "<name>";
const char *PR_ELEM_NAME_END		= "</name>\n";
const char *PR_ELEM_ID			= "<id>";
const char *PR_ELEM_ID_END		= "</id>\n";
const char *PR_ELEM_DESCR		= "<description>";
const char *PR_ELEM_DESCR_END		= "</description>\n";
const char *PR_ELEM_VER			= "<version>";
const char *PR_ELEM_VER_END		= "</version>\n";
const char *PR_ELEM_VENDOR		= "<vendor>\n";
const char *PR_ELEM_VENDOR_END		= "</vendor>\n";
const char *PR_ELEM_REGSTR		= "<registration>\n";
const char *PR_ELEM_REGSTR_END		= "</registration>\n";
const char *PR_ELEM_ADDINFO		= "<additional-information>\n";
const char *PR_ELEM_ADDINFO_END		= "</additional-information>\n";
const char *PR_ELEM_MAYCONT		= "<may-contact>";
const char *PR_ELEM_MAYCONT_END		= "</may-contact>\n";
const char *PR_ELEM_ENTLMNT		= "<entitlement>";
const char *PR_ELEM_ENTLMNT_END		= "</entitlement>\n";
const char *PR_ELEM_CNTID		= "<contract-id>";
const char *PR_ELEM_CNTID_END		= "</contract-id>\n";
const char *PR_ELEM_LICENSE		= "<license>";
const char *PR_ELEM_LICENSE_END		= "</license>\n";
const char *PR_ELEM_SUBCODE		= "<subscription-code>";
const char *PR_ELEM_SUBCODE_END		= "</subscription-code>\n";
const char *PR_DASH			= "-";
const char *PR_TRUE			= "true";
const char *PR_FALSE			= "false";

/* Fault Telemetry XML tags */
const char *TR_ELEM_MESGID		= "<message-id>";
const char *TR_ELEM_MESGID_END		= "</message-id>\n";
const char *TR_ELEM_SUMMARY		= "<summary>";
const char *TR_ELEM_SUMMARY_END		= "</summary>\n";
const char *TR_ELEM_SEVERITY		= "<severity>";
const char *TR_ELEM_SEVERITY_END	= "</severity>\n";
const char *TR_ELEM_DESC		= "<description>";
const char *TR_ELEM_DESC_END		= "</description>";
const char *TR_ELEM_ARESP		= "<auto-response-description>";
const char *TR_ELEM_ARESP_END		= "</auto-response-description>\n";
const char *TR_ELEM_IMPCT		= "<impact>";
const char *TR_ELEM_IMPCT_END		= "</impact>\n";
const char *TR_ELEM_RECAC		= "<required-action>";
const char *TR_ELEM_RECAC_END		= "</required-action>\n";
const char *TR_ELEM_COMP		= "<component>";
const char *TR_ELEM_COMP_END		= "</component>\n";
const char *TR_ELEM_EVENT		= "<event>";
const char *TR_ELEM_EVENT_END		= "</event>\n";
const char *TR_ELEM_PR_EVENT		= "<primary-event-information>";
const char *TR_ELEM_PR_EVENT_END	= "</primary-event-information>\n";

/* Heartbeat Telemetry XML tags */
const char *HB_ELEM_HRTBEAT		= "<heartbeat>";
const char *HB_ELEM_HRTBEAT_END		= "</heartbeat>\n";
const char *HB_ELEM_PAYLOAD		= "<payload name=\"diagnostics\">\n";
const char *HB_ELEM_PAYLOAD_END		= "</payload>\n";


static int build_common_hbflttag_xml(sf_prod_info_t *sf, cl_reg_cfg_t *cl,
    xml_string_t *);


/* Returns -1 on failure, 0 on success */
int
build_product_regstr_xml(
sf_prod_info_t *sf,
cl_reg_cfg_t *cl,
xml_string_t *sb) {

	time_t gmt;
	struct tm  mytm;
	char    c_hostname[SYS_NMLN];

	char p_buf [200], installdate [150];

	/* Check that all MANDATORY fields are non-NULL */
	if (check_sf_prod_info(sf) != 0) {
		return (-1);
	}
	if (check_cl_reg_cfg(cl, B_FALSE) != 0) {
		return (-1);
	}

	xml_string_clear(sb); /* Clear the String */

	if (!xml_string_add(sb, CM_ELEM_HEADER))
		return (-1);
	if (!xml_string_add(sb, PR_ELEM_PRODREG))
		return (-1);

	if (!xml_string_add(sb, PR_ELEM_PRODUCT))
		return (-1);

	/* MANDATORY: swoRDFish product name */
	if (!xml_string_add(sb, PR_ELEM_NAME))
		return (-1);
	if (!xml_string_add(sb, sf->name))
			return (-1);
	if (!xml_string_add(sb, PR_ELEM_NAME_END))
		return (-1);

	/* MANDATORY: the swoRDFish ID */
	if (!xml_string_add(sb, PR_ELEM_ID))
		return (-1);
	if (!xml_string_add(sb, sf->id))
		return (-1);
	if (!xml_string_add(sb, PR_ELEM_ID_END))
		return (-1);

	/* OPTIONAL: swordfish product description */
	if (sf->descr) {
		if (!xml_string_add(sb, PR_ELEM_DESCR))
			return (-1);
		if (!xml_string_add(sb, sf->descr))
			return (-1);
		if (!xml_string_add(sb, PR_ELEM_DESCR_END))
			return (-1);
	}

	/* MANDATORY: product version from swoRDFish */
	if (!xml_string_add(sb, PR_ELEM_VER))
		return (-1);
	if (!xml_string_add(sb, sf->version))
		return (-1);
	if (!xml_string_add(sb, PR_ELEM_VER_END))
		return (-1);

	/* OPTIONAL: vendor information */
	if (sf->vendor || sf->vendor_id) {
		if (!xml_string_add(sb, PR_ELEM_VENDOR))
			return (-1);

		/* OPTIONAL: vendor name */
		if (sf->vendor) {
			if (!xml_string_add(sb, PR_ELEM_NAME))
				return (-1);
			if (!xml_string_add(sb, sf->vendor))
				return (-1);

			if (!xml_string_add(sb, PR_ELEM_NAME_END))
				return (-1);
		}

		/* OPTIONAL: vendor name */
		if (sf->vendor_id) {
			if (!xml_string_add(sb, PR_ELEM_ID))
				return (-1);
			if (!xml_string_add(sb, sf->vendor_id))
				return (-1);
			if (!xml_string_add(sb, PR_ELEM_ID_END))
				return (-1);
		}
		if (!xml_string_add(sb, PR_ELEM_VENDOR_END))
			return (-1);
	}

	/* Add any additional info */
	if (sf->additional_info) {
		if (!xml_string_add(sb, PR_ELEM_ADDINFO))
			return (-1);
		if (!xml_string_add(sb, sf->additional_info))
			return (-1);
		if (!xml_string_add(sb, PR_ELEM_ADDINFO_END))
			return (-1);
	}

	/* End product tag */
	if (!xml_string_add(sb, PR_ELEM_PRODUCT_END))
		return (-1);


	/* Begin registration tag */
	if (!xml_string_add(sb, PR_ELEM_REGSTR))
		return (-1);

	/* Install Date use current time for now */
	time(&gmt);
	localtime_r(&gmt, &mytm);

	snprintf(installdate, 150,
	    "<install-date-time month=\"%d\" day=\"%d\" year=\"%d\" " \
	    "hour=\"%d\" minute=\"%d\" second=\"%d\" timezone=\"%s\"/>",
	    mytm.tm_mon, mytm.tm_mday, mytm.tm_year + 1900,
	    mytm.tm_hour, mytm.tm_min, mytm.tm_sec,
	    (tzname[0] ? tzname[0] : "GMT"));

	if (!xml_string_addn(sb, installdate, strlen(installdate)))
		return (-1);

	/* Host ID is optional if sysinfo returns success add it */
	if (sysinfo(SI_HOSTNAME, c_hostname, SYS_NMLN) > 0) {
		if (!xml_string_add(sb, CM_ELEM_SYSTEM))
			return (-1);
		if (!xml_string_add(sb, c_hostname))
			return (-1);
		if (!xml_string_add(sb, CM_ELEM_SYSTEM_END))
			return (-1);
	}

	/*
	 * MANDATORY: asset id is mandatory for the code in this
	 * library even though it is not mandatory for CNS. It
	 * has been checked above in the check_cl_reg_info.
	 */
	if (!xml_string_add(sb, CM_ELEM_ASSET))
		return (-1);
	if (!xml_string_add(sb, cl->asset_id))
		return (-1);
	if (!xml_string_add(sb, CM_ELEM_ASSET_END))
		return (-1);

	/* Insert Contact Information Here */
	if (cl->name || cl->email) {
		if (!xml_string_add(sb, PR_ELEM_ADDINFO))
			return (-1);

		if (cl->name) {
			snprintf(p_buf, 200, "Contact name:%s\n", cl->name);
			if (!xml_string_add(sb, p_buf))
				return (-1);
		}
		if (cl->email) {
			snprintf(p_buf, 200, "Contact email: %s\n", cl->email);
			if (!xml_string_add(sb, p_buf))
				return (-1);
		}
		if (!xml_string_add(sb, PR_ELEM_ADDINFO_END))
			return (-1);
	}

	if (!xml_string_add(sb, PR_ELEM_REGSTR_END))
		return (-1);

	/* End of Registration Tag */

	if (!xml_string_add(sb, PR_ELEM_PRODREG_END))
		return (-1);

	return (0);
}

/*
 * Functions below here will not be used in 4.6 They are included to
 * as a starting point when telemetry and heartbeat are supported.
 */

static char *cns_subsystem[] = {
	"Internal",
	"UPS",
	"Environmental",
	"RAID",
	"Backup",
	"Network",
	"Mirroring",
};

static char *cns_severity[] = {
	"Minor",    /* Notice */
	"Minor",    /* Warning */
	"Major",    /* Error */
	"Critical", /* Critical */
};

/* Returns 1 0n failure, 0 on success */
static int
build_common_hbflttag_xml(
sf_prod_info_t *sf,
cl_reg_cfg_t *cl,
xml_string_t *sb) {

	char *hostname, *location;
	char  msgtime[30], p_buf[200];
	time_t secs;
	struct tm tm;
	char    c_hostname[SYS_NMLN];

	if (!sysinfo(SI_HOSTNAME, c_hostname, SYS_NMLN)) {
		(void) _LOG(ERR_LVL, "Unable to obtain hostname");
		return (-1);
	}

	hostname = c_hostname;
	location = c_hostname;

	secs = time(0);
	localtime_r(&secs, &tm);
	strftime(msgtime, 30, "%Y-%m-%dT%T", &tm);

	if (!xml_string_add(sb, CM_ELEM_SITEID))
		return (-1);
	if (!xml_string_addn(sb, location, strlen(location)))
		return (-1);
	if (!xml_string_add(sb, CM_ELEM_SITEID_END))
		return (-1);

	if (!xml_string_add(sb, CM_ELEM_HOSTID))
		return (-1);
	if (!xml_string_addn(sb, hostname, strlen(hostname)))
		return (-1);
	if (!xml_string_add(sb, CM_ELEM_HOSTID_END))
		return (-1);

	snprintf(p_buf, 200,
	    "<message-time timezone=\"%s\">%s</message-time>\n",
	    (tzname[0]) ? tzname[0] : "GMT", msgtime);
	if (!xml_string_addn(sb, p_buf, strlen(p_buf)))
		return (-1);

	if (!xml_string_add(sb, CM_ELEM_SYSTEM))
		return (-1);
	if (!xml_string_add(sb, "systemid"))
		return (-1);
	if (!xml_string_add(sb, CM_ELEM_SYSTEM_END))
		return (-1);

	if (!xml_string_add(sb, CM_ELEM_ASSET))
		return (-1);
	if (!xml_string_add(sb, "assetid"))
		return (-1);
	if (!xml_string_add(sb, CM_ELEM_ASSET_END))
		return (-1);

	/*
	 * swoRDFish product ID
	 */
	if (!xml_string_add(sb, CM_ELEM_PRODID))
		return (-1);
	if (!xml_string_add(sb, sf->id))
		return (-1);
	if (!xml_string_add(sb, CM_ELEM_PRODID_END))
		return (-1);

	if (!xml_string_add(sb, CM_ELEM_PRODNAME))
		return (-1);
	if (!xml_string_add(sb, sf->name))
		return (-1);

	if (!xml_string_add(sb, CM_ELEM_PRODNAME_END))
		return (-1);

	return (0);
}


/* Returns 1 0n failure, 0 on success */
int
build_fault_telemetry_xml(
sf_prod_info_t *sf,
cl_reg_cfg_t *cl,
xml_string_t *sb,
int subsys,
int slevel,
char *subject,
char *problem,
char *action,
int msgid) {

	char  msgtime[30], p_buf[200];
	time_t secs;
	struct tm tm;

	secs = time(0);
	localtime_r(&secs, &tm);
	strftime(msgtime, 30, "%Y-%m-%dT%T", &tm);

	xml_string_clear(sb); /* Clear the String */

	if (!xml_string_add(sb, CM_ELEM_HEADER))
		return (-1);

	if (!xml_string_add(sb, CM_ELEM_MSGXSD))
		return (-1);

	/* Build tags common to Heartbeat and Telemetry */
	if (build_common_hbflttag_xml(sf, cl, sb) < 0)
		return (-1);

	if (!xml_string_add(sb, TR_ELEM_EVENT))
		return (-1);
	if (!xml_string_add(sb, TR_ELEM_PR_EVENT))
		return (-1);

	if (!xml_string_add(sb, TR_ELEM_MESGID))
		return (-1);
	snprintf(p_buf, 200, "NAS-%d", msgid);
	if (!xml_string_addn(sb, p_buf, strlen(p_buf)))
		return (-1);
	if (!xml_string_add(sb, TR_ELEM_MESGID_END))
		return (-1);

	snprintf(p_buf, 200,
	    "<event-time timezone=\"MST\">%s</event-time>\n",
	    msgtime  /* tm.tm_zone, */);
	if (!xml_string_addn(sb, p_buf, strlen(p_buf)))
		return (-1);

	if (!xml_string_add(sb, TR_ELEM_SEVERITY))
		return (-1);
	if (!xml_string_add(sb, cns_severity[slevel]))
		return (-1);
	if (!xml_string_add(sb, TR_ELEM_SEVERITY_END))
		return (-1);

	if (!xml_string_add(sb, TR_ELEM_COMP))
		return (-1);
	snprintf(p_buf, 200,
	    "<uncategorized name=\"%s\"></uncategorized>\n",
	    cns_subsystem[subsys]);
	if (!xml_string_addn(sb, p_buf, strlen(p_buf)))
		return (-1);
	if (!xml_string_add(sb, TR_ELEM_COMP_END))
		return (-1);

	if (!xml_string_add(sb, TR_ELEM_SUMMARY))
		return (-1);
	if (!xml_string_add(sb, CM_ELEM_CDATA))
		return (-1);
	if (!xml_string_addn(sb, subject, strlen(subject)))
		return (-1);
	if (!xml_string_add(sb, CM_ELEM_CDATA_END))
		return (-1);
	if (!xml_string_add(sb, TR_ELEM_SUMMARY_END))
		return (-1);

	if (!xml_string_add(sb, TR_ELEM_DESC))
		return (-1);
	if (!xml_string_add(sb, CM_ELEM_CDATA))
		return (-1);
	if (!xml_string_addn(sb, problem, strlen(problem)))
		return (-1);
	if (!xml_string_add(sb, CM_ELEM_CDATA_END))
		return (-1);
	if (!xml_string_add(sb, TR_ELEM_DESC_END))
		return (-1);

	if (!xml_string_add(sb, TR_ELEM_ARESP))
		return (-1);
	if (!xml_string_add(sb, CM_ELEM_CDATA))
		return (-1);
	if (!xml_string_addn(sb, problem, strlen(problem)))
		return (-1);
	if (!xml_string_add(sb, CM_ELEM_CDATA_END))
		return (-1);
	if (!xml_string_add(sb, TR_ELEM_ARESP_END))
		return (-1);

	if (!xml_string_add(sb, TR_ELEM_IMPCT))
		return (-1);
	if (!xml_string_add(sb, CM_ELEM_CDATA))
		return (-1);
	if (!xml_string_addn(sb, problem, strlen(problem)))
		return (-1);
	if (!xml_string_add(sb, CM_ELEM_CDATA_END))
		return (-1);
	if (!xml_string_add(sb, TR_ELEM_IMPCT_END))
		return (-1);

	if (!xml_string_add(sb, TR_ELEM_RECAC))
		return (-1);
	if (!xml_string_add(sb, CM_ELEM_CDATA))
		return (-1);
	if (!xml_string_addn(sb, action, strlen(action)))
		return (-1);
	if (!xml_string_add(sb, CM_ELEM_CDATA_END))
		return (-1);
	if (!xml_string_add(sb, TR_ELEM_RECAC_END))
		return (-1);

	if (!xml_string_add(sb, TR_ELEM_PR_EVENT_END))
		return (-1);
	if (!xml_string_add(sb, TR_ELEM_EVENT_END))
		return (-1);
	if (!xml_string_add(sb, CM_ELEM_MSGXSD_END))
		return (-1);

	return (0);
}


/* Returns -1 on failure, 0 on success */
int
build_heartbeat_xml(
sf_prod_info_t *sf,
cl_reg_cfg_t *cl,
xml_string_t *sb) {

	char  hrtbttime[30], p_buf[200];
	time_t secs;
	struct tm tm;

	secs = time(0);
	localtime_r(&secs, &tm);
	strftime(hrtbttime, 30, "%Y-%m-%dT%T", &tm);

	xml_string_clear(sb); /* Clear the String */

	if (!xml_string_add(sb, CM_ELEM_HEADER))
		return (-1);
	if (!xml_string_add(sb, CM_ELEM_MSGXSD))
		return (-1);

	/* Build tags common to Heartbeat and Telemetry */
	if (build_common_hbflttag_xml(sf, cl, sb) < 0)
		return (-1);

	if (!xml_string_add(sb, HB_ELEM_HRTBEAT))
		return (-1);
	snprintf(p_buf, 200,
	    "<time timezone=\"MST\">%s</time>\n", /* tm.tm_zone, */ hrtbttime);
	if (!xml_string_addn(sb, p_buf, strlen(p_buf)))
		return (-1);
	if (!xml_string_add(sb, HB_ELEM_HRTBEAT_END))
		return (-1);

	if (!xml_string_add(sb, HB_ELEM_PAYLOAD))
		return (-1);

	if (!xml_string_add(sb, CM_ELEM_CDATA))
		return (-1);

	if (!xml_string_add(sb, CM_ELEM_CDATA_END)) {
		return (-1);
	}

	if (!xml_string_add(sb, HB_ELEM_PAYLOAD_END)) {
		return (-1);
	}

	if (!xml_string_add(sb, CM_ELEM_MSGXSD_END))
		return (-1);

	return (0);
}
