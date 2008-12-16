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

#pragma	ident	"$Revision: 1.43 $"

/*
 * notify_summary.c
 * Get the email address and the subscriptions. Some email notifications
 * are sent via scripts (/etc/opt/SUNWsamfs/scripts), parsing is heavily
 * dependent on the syntax
 * Provide support to add email addresses, change subscriptions for emails,
 * and delete email addresses.
 */

/* ANSI C headers. */
#include <stdio.h>
#include <libgen.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

/* POSIX headers. */
#include <sys/stat.h>

/* SAM API headers. */
#include "pub/mgmt/notify_summary.h"
#include "pub/mgmt/error.h"
#include "mgmt/util.h"
#include "mgmt/config/common.h"
#include "pub/mgmt/file_util.h"


/* SAM-FS headers. */
#include "sam/sam_trace.h"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#define	NOTIFYFILE	CFG_DIR"/notify.cmd"
#define	MAIL		"mailx"
#define	STR_EQ "="
#define	STR_EMPTY ""

/* the type of actions */
typedef enum notf_act {
	ADD, /* add an email */
	DEL, /* delete and email */
	MOD /* modify an email */
} notf_act_t;

/* parsed info from a file */
typedef struct line_info {
	int		numtoks; /* # of tokens */
	boolean_t	pat_exists; /* add pattern exists */
	boolean_t	rep_exists; /* repl. pattern exists */
	boolean_t	here_doc; /* "<<" pattern */
	boolean_t	comment; /* Is the line commented */
	upath_t		token; /* get a token */
	upath_t		token1; /* get a token */
} line_info_t;

static struct {
	int  type;
	char *name;
} events[] = {
	{ DEVICEDOWN, DEV_DOWN },
	{ ARCHINTR, ARCH_INTR },
	{ MEDREQD, MED_REQ },
	{ RECYSTATRC, RECY_STAT_RC },
	{ DUMPINTR, DUMP_CLASS""DUMP_INTERRUPTED_SUBCLASS },
	{ FSFULL, "ENospace" }, /* subtype of FS_CLASS */
	{ HWM_EXCEED, "HwmExceeded" }, /* subtype of FS_CLASS */
	{ ACSLS_ERR, ACSLS_CLASS""ACSLS_ERROR_SUBCLASS },
	{ ACSLS_WARN, ACSLS_CLASS""ACSLS_INFO_SUBCLASS },
	{ DUMP_WARN, DUMP_CLASS""DUMP_WARN_SUBCLASS },
	{ LONGWAIT_TAPE, "TapeWait" },
	{ FS_INCONSISTENT, "FsInconsistent" },
	{ SYSTEM_HEALTH, "SystemHealth" },
	{ 0, "" },
};


/* The new email passed in for modification */
static upath_t newemail;

static boolean_t newemail_set = B_FALSE;

/* private functions */
static boolean_t addr_exists(const char *addrs, const char *addr);
static int get_addr_from_script(char *script, sqm_lst_t **addr_lst);
static int addrs2notifysumm(
	notf_subj_t subj, sqm_lst_t *addr_lst, sqm_lst_t **notf_lst);
static int add_notify_summary_file(upath_t, char *);
static int del_notify_summary_file(upath_t, char *);
static int tokenize_line(char *, upath_t, line_info_t *);
static int search_replace(char *, upath_t, notf_act_t, line_info_t);
static int update_notf_lst(
	notf_subj_t subj, char *addr, char *addr_value, sqm_lst_t *notf_lst);
static int add_notf_if_not_exist(upath_t emailaddr, char *notfscript);
static int get_mail_tokens(FILE *fp, upath_t *mail_token, int *n);
static int get_notification_for_addr(char *addr, notf_summary_t **summary);
static int add_addr(char *type, char *new_addr);
static int del_addr(char *type, char *del_addr);
static char *get_value(char *key, char *script);

/* Implementations */
/*
 * get_notify_summary().
 * Collect the addresses that are setup to receive any notifications on this
 * system
 * Error codes returned:
 * 0: Success, returns an empty list of no notifications are found
 * -1: Failure
 */
int
get_notify_summary(
ctx_t *ctx,				/* ARGSUSED */
sqm_lst_t ** notf_lst)			/* the list of notf_summ */
{

	sqm_lst_t *addr_lst = NULL;
	int i = 0;

	Trace(TR_MISC, "getting notification summary...");

	if (ISNULL(notf_lst)) {
		Trace(TR_ERR, "get notification failed: %s", samerrmsg);
		return (-1);
	}

	*notf_lst = NULL;
	if ((*notf_lst = lst_create()) == NULL) {
		Trace(TR_ALLOC, "get notf. summary failed: %s", samerrmsg);
		return (-1);
	}

	/*
	 * Four notifications are implemented as scripts. For these, check
	 * if the scripts exist, these scripts are optional but if they
	 * exist, they are in /etc/opt/SUNWsamfs/scripts
	 *
	 */
	for (i = 0; i < SCRIPTS_MAX; i++) {
		if ((file_exists(NULL, events[i].name) == 0) &&
		    (get_addr_from_script(
		    events[i].name, &addr_lst) == 0))  {

			addrs2notifysumm(events[i].type, addr_lst, notf_lst);
			lst_free(addr_lst);
		}
	}

	for (i = SCRIPTS_MAX; i < SUBJ_MAX; i++) {
		if (get_addr(events[i].name, &addr_lst) == 0) {
			addrs2notifysumm(events[i].type, addr_lst, notf_lst);
			lst_free(addr_lst);
		}
	}

	Trace(TR_MISC, "got notification summary...");

	return (0);
}

/*
 * get_email_addrs_by_subj()
 *
 * Returns a comma separated list of email addresses subscribed to
 * a specific notification subject.
 *
 * Notification subjects are as defined in the enum notf_subj_t
 */
int get_email_addrs_by_subj(
	ctx_t		*clnt,			/* ARGSUSED */
	notf_subj_t	subj_wanted,
	char		**addrs)
{
	int		rval = 0;
	char		*getstr;
	sqm_lst_t		*addr_lst = NULL;

	Trace(TR_MISC, "getting email addrs...");

	if (ISNULL(addrs)) {
		Trace(TR_ERR, "get email addrs failed: %s", samerrmsg);
		return (-1);
	}

	*addrs = NULL;
	getstr = events[subj_wanted].name;

	if (subj_wanted < SCRIPTS_MAX) {

		if (file_exists(NULL, getstr) != 0) {
			*addrs = copystr(STR_EMPTY);
			return (0);
		}

		rval = get_addr_from_script(getstr, &addr_lst);
	} else {

		rval = get_addr(getstr, &addr_lst);
	}


	/*
	 * No notifications defined is not an error.  Just return an
	 * empty string.
	 */
	if (rval != 0) {
		*addrs = copystr(STR_EMPTY);
		return (0);
	}

	/* use lst2str() to create the return string */
	*addrs = lst2str(addr_lst, ",");

	if (addr_lst != NULL) {
		lst_free(addr_lst);
	}

	return (rval);
}


/*
 * get_addr_from_script
 * Read the script to get the email addresses
 * The mail section in the script is expected to be in this format:
 *
 * mail root <<EOF
 * <some message>
 * EOF
 *
 * or
 *
 * mail -s "<some message>" root <<EOF
 * <some details here>
 * EOF
 *
 */
static int
get_addr_from_script(
	char *script,	/* notification script */
	sqm_lst_t **addr_lst	/* return - list of addresses */
)
{

	FILE	*fp = NULL;
	upath_t mail_token[512];
	int num = 0, i = 0;

	Trace(TR_DEBUG, "getting address from script...");

	/* open the notification script */
	if ((fp = fopen(script, "r")) == NULL) {

		samerrno = SE_FILE_READ_OPEN_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_FILE_READ_OPEN_FAILED), script);

		Trace(TR_DEBUG, "%s", samerrmsg);

		return (-1);
	}

	get_mail_tokens(fp, mail_token, &num);

	for (*addr_lst = lst_create(), i = 0; i < num; i++) {
		lst_append(*addr_lst, strdup(mail_token[i]));
	}

	fclose(fp);

	Trace(TR_DEBUG, "got address from script");

	return (0);
}


/*
 * addrs2notifysumm
 *
 * Put the addrs in the notify_summary_t format
 * the structure holds the email and a boolean array of notifications types
 * just update the subj boolean flags if the addr already exists in notf_lst
 *
 */
static int
addrs2notifysumm(
	notf_subj_t subj,	/* type of notification (enum) */
	sqm_lst_t *addr_lst,	/* list of addrs for this type */
	sqm_lst_t **notf_lst	/* return - list of notify_summ_t */
)
{

	node_t *addr_node = NULL;
	notf_summary_t *summary = NULL;
	char *email = NULL, *email_value;

	addr_node = addr_lst->head;
	while (addr_node != NULL) {

		email = (char *)addr_node->data;
		email_value = NULL;

		if (email == NULL) {
			continue;
		}

		/*
		 * If email is a variable, store value in admin_name
		 * e.g. CONTACTS=root name1 name2
		 * mail $CONTACTS
		 */
		if (email != NULL && email[0] == '$') {
			email_value =  get_value(email, events[subj].name);
		}
		/*
		 * If subscriber is already in list, update the subscription
		 * flag in notf_summary to notification subscriptions, else
		 * add a new subscriber to the list
		 */
		if (((*notf_lst)->length == 0) ||
		    update_notf_lst(subj, email, email_value, *notf_lst) != 1) {

			summary = (notf_summary_t *)mallocer(
			    sizeof (notf_summary_t));
			if (ISNULL(summary)) {
				Trace(TR_ALLOC, "addr to summary failed: %s",
				    samerrmsg);
				return (-1);
			}
			memset(summary, 0, sizeof (notf_summary_t));

			strcpy(summary->admin_name,
			    (email_value != NULL) ?  email_value : email);

			strcpy(summary->emailaddr, email);
			summary->notf_subj_arr[subj] = B_TRUE;
			if (lst_append(*notf_lst, summary) != 0) {
				Trace(TR_ALLOC, "addr to summary failed: %s",
				    samerrmsg);
				free(summary);
				return (-1);
			}
		}
		addr_node = addr_node->next;
	}
	return (0);
}

static char *
get_value(char *key, char *script) {

	FILE *fp = NULL;
	char buffer[MAXPATHLEN] = {0};
	char *ptr = NULL, *ptr_key = NULL;
	int n = 0;

	ptr_key = key;
	ptr_key++;

	int keylen = strlen(ptr_key);
	Trace(TR_DEBUG, "getting key-value from script...");

	/* open the notification script, if err, return empty string */
	if ((fp = fopen(script, "r")) == NULL) {
		return (copystr(STR_EMPTY));
	}
	for (;;) {
		ptr = fgets(buffer, sizeof (buffer), fp);
		if (ptr == NULL) {
			fclose(fp);
			return (copystr(STR_EMPTY));
		}
		/* remove new line char */
		ptr[strlen(buffer) - 1] = '\0';
		if (*ptr == CHAR_POUND) {
			continue;
		}
		n = strcspn(ptr, STR_EQ);
		if ((n == keylen) && strncmp(ptr_key, ptr, keylen) == 0) {
			fclose(fp);
			return (copystr(ptr + keylen + 1));
		}
	}
	/* statement not reached */
}


static int
get_mail_tokens(
FILE *fp,
upath_t *mail_token,
int *num)
{
	char	line[MAX_LINE]; /* line read in */
	char	*tmp1 = NULL;
	char	*tok1 = NULL; /* a token */
	int n = 0;

	while (fgets(line, sizeof (line), fp) != NULL) {

		/* line is not a comment and has 'mail' in it... */
		if ((strchr(line, '#') == NULL) &&
		    (strstr(line, "mail") != NULL)) {


			/* duplicate the line... */
			tmp1 = strdup(line);
			tok1 = strtok(tmp1, " \t");

			Trace(TR_DEBUG,
			    "composing notfn. summary for line %s:\n"
			    " token is %s", line, tok1);

			while (tok1 != NULL) {

				tok1 = strtok(NULL, " \t");
				/*
				 * This is to account for
				 * the case where the '<<'
				 * anchor is missing.
				 */
				if (tok1 == NULL) {
					break;
				}

				/* If this line has the '-s'/-t etc. option */
				if (tok1[0] ==  '-') {
					continue;
				}
				/* skip over the message within " " */
				if (strchr(tok1, '"')) {
					do {
						tok1 = strtok(NULL, " \t");
					} while (strchr(tok1, '"') == NULL);
					continue;
				}

				Trace(TR_DEBUG,
				    "composing notfn. summary for line %s:\n"
				    " token is %s", line, tok1);

				/* we want to get out if we hit this */
				if (strstr(tok1, "<<")) {
					break;
				}

				/*
				 * This is email token, save this
				 */
				strlcpy(mail_token[n++], tok1, NOTF_LEN);

			}
			*num = n;
			free(tmp1);
			return (0);
		}
	}
	return (1);
}

/*
 * add_notify_summary().
 * Add notification information for specified events.
 * The email address is the unique identifier.
 * If the specified email address already exists, ignore the request
 *
 * Error codes returned:
 *  0: Success
 * -1: Failure
 * -2: Partial success
 */
int
add_notify_summary(
ctx_t *ctx,	/* ARGSUSED */
notf_summary_t *notf_summ) /* notf. summary struct */
{
	int	n;
	notf_summary_t *cur_summ;

	Trace(TR_MISC, "adding notification...");

	if (ISNULL(notf_summ)) {
		Trace(TR_ERR, "add notification failed: %s", samerrmsg);
		return (-1);
	}

	/* check if email addrs already exists */
	if (get_notification_for_addr(notf_summ->emailaddr, &cur_summ) == 0) {
		/* email addr already exists, error */
		samerrno = SE_NOTIFICATION_ALREADY_EXISTS;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_NOTIFICATION_ALREADY_EXISTS),
		    notf_summ->emailaddr);
		Trace(TR_ERR, "Add notify summary failed: %s", samerrmsg);
		return (-1);
	}

	/* Loop through the notifications that are implemented as scripts */
	for (n = 0; n < SCRIPTS_MAX; n++) {
		if ((notf_summ->notf_subj_arr[n] == B_TRUE) &&
		    add_notify_summary_file(
		    notf_summ->emailaddr, events[n].name) == -1) {

			Trace(TR_ERR, "add notification failed: %s", samerrmsg);
			return (-1);
		}
	}
	/*
	 * The notification types are positional
	 * 0 - SCRIPTS_MAX : are notifications sent from SAM scripts
	 * SCRIPTS_MAX (4) : Dump interrupted notification sent using sysevent
	 * (5) : File system full sent using sysevent
	 */
	for (n = SCRIPTS_MAX; n < SUBJ_MAX; n++) {

		if (notf_summ->notf_subj_arr[n] == 1) {
			if (add_addr(
			    events[n].name, notf_summ->emailaddr) == -1) {

				Trace(TR_DEBUG, "Add notification failed:%s",
				    samerrmsg);
				return (-1);
			}
		}
	}

	Trace(TR_MISC, "notification added successfully...");
	send_mail(notf_summ->emailaddr,
	    GetCustMsg(SE_MAIL_SUBJECT_NOTIFICATION_SETUP),
	    GetCustMsg(SE_MAIL_MSG_CONFIRM_NOTIFICATION_SETUP));

	return (0);
}


/*
 * add_notify_summary_file().
 * Add a login/email-addr to a notification
 * file. Only ONE login/email-addr to ONE file.
 * Client must provide complete path of file.
 * Error Codes returned:
 *	-1
 *	SE_NOTF_ADDR_EXISTS
 */
int
add_notify_summary_file(
upath_t emailaddr,	/* the loginid/email-addr */
char *notfscript)	/* notification script */
{

	int	ret;
	line_info_t linfo;
	notf_act_t	action;
	boolean_t new_file_copied = B_FALSE;
	char default_script[1024] = {0};
	char scriptname[64] = {0};

	if (ISNULL(notfscript)) {
		Trace(TR_ERR, "add notification failed: %s", samerrmsg);
		return (-1);
	}

	char *separator = strrchr(notfscript, '/');
	if (ISNULL(separator)) {
		Trace(TR_ERR, "could not get script name");
		return (-1);
	}
	snprintf(scriptname, sizeof (scriptname), separator + 1);
	if (scriptname == NULL || scriptname[0] == '\0') {
		Trace(TR_ERR, "could not get script name");
		return (-1);
	}

	Trace(TR_DEBUG, "adding notfn. to file %s:", notfscript);

	/* initialize fields */
	linfo.pat_exists = B_FALSE;
	linfo.rep_exists = B_FALSE;
	linfo.here_doc = B_FALSE;
	linfo.comment = B_TRUE; /* line is commented by default */
	linfo.numtoks = 0;
	linfo.token[0] = '\0';

	action = ADD;


	/* Now check if the file exists */
	if (file_exists(NULL, notfscript) != 0) {

		/* if it does NOT exist, copy the default */
		snprintf(default_script, sizeof (default_script),
		    "%s/%s", SAM_EXAMPLES_PATH, scriptname);

		if (cp_file(default_script, notfscript) == 0) {
			new_file_copied = TRUE;
		} else {
			/* copy operation failed, samerrmsg is set */
			return (-1);
		}
	}

	/* gather information about the line */
	ret = tokenize_line(notfscript, emailaddr, &linfo);

	if (ret < 0) {
		Trace(TR_DEBUG, "adding notfn. to file failed");
		return (-1);
	}

	Trace(TR_DEBUG, "adding notfn.to file:"
	    " The number of tokens is %d\n",
	    linfo.numtoks);

	if (linfo.pat_exists == B_TRUE) {

			samerrno = SE_NOTF_ADDR_EXISTS;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_NOTF_ADDR_EXISTS),
			    emailaddr, notfscript);

			Trace(TR_DEBUG,
			    "adding notfn. to file: %s",
			    samerrmsg);
			return (0);
	}

	if (new_file_copied == B_TRUE) {

		action = MOD;

		/* copy the replacement email into newemail */
		strlcpy(newemail, emailaddr, sizeof (upath_t));

		newemail_set = B_TRUE;

		ret = search_replace(notfscript, "root", action,
		    linfo);

		if (ret < 0) {
			Trace(TR_DEBUG,
			    "adding notfn. to file failed");
			return (-1);
		}
		new_file_copied = B_FALSE;
	} else {

		ret = search_replace(notfscript, emailaddr, action,
		    linfo);

		if (ret < 0) {
			Trace(TR_DEBUG,
			    "adding notfn. to file failed");
			return (-1);
		}
	}

	Trace(TR_DEBUG,
	    "notification added to file %s", notfscript);


	return (0);
}

/*
 * del_notify_summary().
 * Error codes returned:
 *  0: Success
 * -1: Failure
 * -2: Partial success
 */
int
del_notify_summary(
ctx_t *ctx,	/* ARGSUSED */
notf_summary_t *notf_summ) /* notf. summary struct */
{
	int	n;
	char type[32];


	if (ISNULL(notf_summ)) {
		Trace(TR_ERR, "delete notification failed: %s", samerrmsg);
		return (-1);
	}

	Trace(TR_MISC, "delete notification for addr %s", notf_summ->emailaddr);

	/* Loop through the notifications that are implemented as scripts */
	for (n = 0; n < SCRIPTS_MAX; n++) {
		/* Which notf event has been set? */
		if ((notf_summ->notf_subj_arr[n] == 1) &&
		    del_notify_summary_file(
		    notf_summ->emailaddr, events[n].name) == -1) {

			Trace(TR_DEBUG, "Delete notification failed:%s",
			    samerrmsg);
			return (-1);
		}
	}

	/* delete notifications that are not scripts */
	for (n = SCRIPTS_MAX; n < SUBJ_MAX; n++) {

		if (notf_summ->notf_subj_arr[n] == 1) {
			strcpy(type, events[n].name);
			if (del_addr(type, notf_summ->emailaddr) == -1) {

				Trace(TR_DEBUG, "Delete notification failed:%s",
				    samerrmsg);
				return (-1);
			}
		}
	}

	Trace(TR_MISC, "delete notification success...");

	return (0);
}

/*
 * del_notify_summary_file().
 * Delete one entry from a file.
 * If that entry is the only entry,
 * as of now we do NOT allow deleting
 * that entry because then entire
 * notification case in the file
 * becomes useless.
 *	Error codes returned:
 * -1
 * SE_EMAIL_NOT_PRESENT
 */
int
del_notify_summary_file(
upath_t emailaddr,	/* email to be deleted */
char *notfscript)	/* filename */
{

	int	ret;
	line_info_t linfo;
	notf_act_t	action;

	Trace(TR_DEBUG, "deleting notfn. from file %s",
	    notfscript);

	linfo.pat_exists = B_FALSE;
	linfo.rep_exists = B_FALSE;
	linfo.here_doc = B_FALSE;
	linfo.comment = B_TRUE;
	linfo.numtoks = 0;
	linfo.token[0] = '\0';

	action = DEL;

	/* gather information about the line */
	ret = tokenize_line(notfscript, emailaddr, &linfo);
	if (ret < 0) {
		Trace(TR_DEBUG, "del. notfn. from file %s failed",
		    notfscript);
		return (-1);
	}

	/* the pattern to delete is not present */
	if (linfo.pat_exists == B_FALSE) {

			samerrno = SE_EMAIL_NOT_PRESENT;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_EMAIL_NOT_PRESENT),
			    emailaddr, notfscript);

			Trace(TR_DEBUG,
			    "del. notfn.from file %s failed:\n%s",
			    notfscript, samerrmsg);
			return (-1);

	}

	/* call the search/replace method */
	ret = search_replace(notfscript, emailaddr, action,
	    linfo);

	if (ret < 0) {
		Trace(TR_DEBUG, "del. notfn. from file %s failed",
		    notfscript);
		return (-1);
	}

	Trace(TR_DEBUG, "deleted notfn. from file %s",
	    notfscript);

	return (0);
}

/*
 * mod_notify_summary().
 * Modifies the notifications received
 * Error codes returned:
 *  0: Success
 * -1: Failure
 * -2: Partial success
 */
int
mod_notify_summary(
ctx_t *ctx,	/* ARGSUSED */
upath_t	oldname, /* old name to be modified */
notf_summary_t *notf_summ) /* notf summary struct */
{
	int	n;
	int	ret = 0; /* default return value */
	notf_summary_t *summ;

	Trace(TR_MISC, "modifying notification...");

	if (ISNULL(notf_summ, oldname)) {
		Trace(TR_ERR, "Modify notification failed: %s", samerrmsg);
		return (-1);
	}

	/*
	 * Loop through all the notifications
	 * If the current notification settings is true and
	 * requested notification setting is false, then delete
	 * that notification.
	 * If the current notification setting is false and
	 * requested notofication setting is true, then add
	 * the requested notofocation
	 */

	/* Get current notification settings */
	if (get_notification_for_addr(notf_summ->emailaddr, &summ) == -1) {
		/* leave samerrno as set */
		Trace(TR_ERR, "Modify notify summary failed: %s", samerrmsg);
		return (-1);
	}

	for (n = 0; n < SCRIPTS_MAX; n++) {
		if (((notf_summ->notf_subj_arr[n]) == B_TRUE) &&
		    ((summ->notf_subj_arr[n]) == B_FALSE)) {

			/*
			 * current notification setting is false
			 * requested notification setting is true
			 *
			 * Add the email from this notification script
			 */
			ret = add_notify_summary_file(
			    notf_summ->emailaddr, events[n].name);

		} else if (((notf_summ->notf_subj_arr[n]) == B_FALSE) &&
		    ((summ->notf_subj_arr[n]) == B_TRUE)) {

			/*
			 * current notification setting is true
			 * requested notification setting is false
			 *
			 * Delete the email from this notification script
			 */
			ret = del_notify_summary_file(
			    notf_summ->emailaddr, events[n].name);
		} /* no change in the notification setting is requested */

		if (ret == -1) {
			Trace(TR_DEBUG,
			    "modifying notification failed for file: %s",
			    events[n].name);
			free(summ);
			return (-1);
		}
	}

	for (n = SCRIPTS_MAX; n < SUBJ_MAX; n++) {
		if ((notf_summ->notf_subj_arr[n] == B_TRUE) &&
		    (summ->notf_subj_arr[n] == B_FALSE)) {
			if (add_addr(
			    events[n].name, notf_summ->emailaddr) != 0) {
				free(summ);
				return (-1);
			}
		} else if ((notf_summ->notf_subj_arr[n] == B_FALSE) &&
		    (summ->notf_subj_arr[n] == B_TRUE)) {
			if (del_addr(
			    events[n].name, notf_summ->emailaddr) != 0) {
				free(summ);
				return (-1);
			}
		}

	}

	free(summ);

	Trace(TR_MISC, "modified notification...");
	return (ret);
}


/*
 * A method to tokenize the
 * relevant line in the file
 * and extract all info that
 * one may need.
 *	Error codes returned:
 *	-1
 *	SE_FILE_READ_OPEN_FAILED
 *	SE_MALFORMED_NOTF_LINE
 */
int
tokenize_line(
char *notfscript,	/* the notification script */
upath_t	emailaddr,		/* the email to match */
line_info_t *linfo)	/* line info pointer */
{

	FILE	*fp;
	char	line[MAX_LINE]; /* line read in */
	char	*tmp1, *tmp2; /* pointers to tmp strings */
	char	*tok1; /* a token */
	int	numtoks = 0;

	Trace(TR_DEBUG, "tokenizing line in file %s", notfscript);

	/* open the notification file */
	if ((fp = fopen(notfscript, "r")) == NULL) {

		samerrno = SE_FILE_READ_OPEN_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_FILE_READ_OPEN_FAILED), notfscript);

		Trace(TR_DEBUG, "tokenize line failed: %s", samerrmsg);
		return (-1);
	}

	while (fgets(line, sizeof (line), fp) != NULL) {

		/*
		 * If the line does not have a comment and has
		 * the pattern "mail."
		 */
		if ((strchr(line, '#') == NULL) &&
		    (strstr(line, "mail") != NULL)) {

			/* this line IS uncommented */
			linfo->comment = B_FALSE;

			/* remove the newline if any */
			if (strchr(line, '\n') != NULL) {
				line[strlen(line) - 1] = '\0';
			}

			/* Duplicate the original line */
			tmp1 = strdup(line);

			/* Now tokenize the line */
			tok1 = strtok(tmp1, " \t");

			numtoks++;

			Trace(TR_DEBUG, "tokenizing line %s:\n"
			    " token is %s", line, tok1);

			while (tok1 != NULL) {

				tok1 = strtok(NULL, " \t");

				if (tok1 == NULL) break;

				Trace(TR_DEBUG, "tokenizing line: %s,\n"
				    " token is %s", line, tok1);

				/* If this line has the '-s' option */
				if (strcmp(tok1, "-s") == 0) {
					continue;
				}

				/* skip over the message within " " */
				if (strchr(tok1, '"')) {
					do {
						tok1 = strtok(NULL, " \t");
					} while (strchr(tok1, '"') == NULL);
					continue;
				}

				numtoks++;

				Trace(TR_DEBUG, "tokenizing line: %s,\n"
				    " # of tokens is %d\n", line, numtoks);

				/* check if the pattern exists */
				if (strcmp(tok1, emailaddr) == 0) {
					linfo->pat_exists = B_TRUE;
				}

				/*
				 * Check if the replacement email,
				 * used in modification, exists
				 */
				if (newemail_set == B_TRUE &&
				    (strcmp(tok1, newemail) == 0)) {

					linfo->rep_exists = B_TRUE;
				}

				/* check if the here doc symbol exists */
				if ((strstr(tok1, "<<")) != 0) {

					if (tok1[0] != '<') {
						samerrno =
						    SE_MALFORMED_NOTF_LINE;
						snprintf(samerrmsg,
						    MAX_MSG_LEN,
						    GetCustMsg(
						    SE_MALFORMED_NOTF_LINE),
						    notfscript);

						Trace(TR_DEBUG,
						    "tokenizing line %s:\n"
						    " failed %s",
						    line, samerrmsg);
						fclose(fp);
						return (-1);
					}

					/* Copy the "<<" symbol */
					strlcpy(linfo->token, tok1, 2);
					linfo->token[2] = '\0';

					linfo->here_doc = B_TRUE;

					/* Now get the end marker */
					if ((tmp2 =
					    strchr(tok1, '/')) == NULL) {
						/*
						 * No '/' character.
						 * So we just move two
						 * chars ahead of '<<'.
						 */
						tmp2 = tok1 + 2;
					} else {
						/*
						 * The '/' char is present.
						 * So we move tmp2 to one
						 * char ahead of what strchr()
						 * returns.
						 */
						tmp2++;
					}

					Trace(TR_DEBUG, "the endmarker is:%s",
					    tmp2);

					strlcpy(linfo->token1, tmp2,
					    sizeof (upath_t));

				}

			linfo->numtoks = numtoks;
			}
			free(tmp1);
			break;
		}
	}

	Trace(TR_DEBUG, "tokenized line...");

	fclose(fp);

	return (0);
}


/*
 * A function to search for
 * a given pattern and perform
 * replacements with sed.
 */
int
search_replace(
char *notfscript,	/* the notification file */
upath_t	emailaddr,	/* the email address */
notf_act_t action,	/* action: add, del, modify */
line_info_t linfo)	/* the line info struct */
{

	char	cmd1[2 * MAXPATHLEN], cmd2[2 * MAXPATHLEN];
	char	cmd3[2 * MAXPATHLEN];
	upath_t	notf_tmp_old, notf_tmp_new;
	int	ret;
	FILE	*ptr;

	Trace(TR_DEBUG, "search and replacing in file %s", notfscript);


	/* Form the backup file names */
	snprintf(notf_tmp_new, sizeof (notf_tmp_new),
	    "%s.new", notfscript);

	snprintf(notf_tmp_old, sizeof (notf_tmp_old), "%s.old", notfscript);

	/* copy the original file */
	snprintf(cmd1, sizeof (cmd1),
	    "%s %s %s", "/usr/bin/cp -f", notfscript, notf_tmp_old);

	/* exec the command */
	if ((ptr = popen(cmd1, "w")) == NULL) {
		samerrno = SE_POPEN_WRITE_OPEN_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_POPEN_WRITE_OPEN_FAILED),
		    cmd1);
		Trace(TR_DEBUG, "search and replace failed on file %s: %s",
		    notfscript, samerrmsg);
		return (-1);
	}

	if (pclose(ptr) == -1) {
		samerrno = SE_POPEN_CLOSE_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_POPEN_CLOSE_FAILED),
		    cmd1);
		Trace(TR_DEBUG, "search and replace failed on file %s: %s",
		    notfscript, samerrmsg);
		return (-1);
	}

	/*
	 * Adding a notification has to deal with
	 * the following cases:
	 * 	1) Uncommented, existing notification
	 *	text in the HERE doc format. Insert before
	 *	the HERE doc symbol.
	 *	2) Uncommented, existing notification
	 *	text NOT in the HERE doc format. Append
	 *	at end of existing notification text.
	 *	3) No existing notification text in
	 *	uncommented state. Add new text to file.
	 */
	if (action == ADD) {

		/* if the "<<" symbol exists */
		if ((linfo.here_doc == B_TRUE) &&
		    (linfo.comment == B_FALSE)) {

			/*
			 * Search and replace with sed.
			 * The command is of the form:
			 * 's/<<EOF/emailaddr <<EOF/'
			 * notfscript.
			 * The anchor we have is <<.
			 * The output is redirected
			 * to a tmp file.
			 */
			snprintf(cmd2,
			    sizeof (cmd2),
			    "%s %s%s%s%s %s%s %s %s %s",
			    "/usr/bin/sed", "'/^[^#]/s/",
			    linfo.token, "/", emailaddr,
			    linfo.token, "/'", notfscript, ">",
			    notf_tmp_new);
		/* if the "<<" pattern does not exist */
		} else if ((linfo.here_doc == B_FALSE) &&
		    (linfo.comment == B_FALSE)) {

		/*
		 * append the pattern.
		 */
		snprintf(cmd2, sizeof (cmd2),
		    "%s %s %s%s %s %s %s",
		    "/usr/bin/sed",
		    "'s/\\(mail.*\\)/\\1",
		    emailaddr, "/'",
		    notfscript, ">",
		    notf_tmp_new);
		Trace(TR_DEBUG, "search and replace: sed cmd is %s\n", cmd2);
		/*
		 * We also add if previous notification
		 * exists.This is indicated by no
		 * uncommented 'mail' command.
		 */
		} else if (linfo.comment == B_TRUE) {
			ret = add_notf_if_not_exist(
			    emailaddr, notfscript);
			if (ret < 0) {
				Trace(TR_DEBUG,
				    "search and replace failed");
				return (-1);
			}
			return (0);
		}

	} else if (action == DEL) {

		/* if the "<<" pattern exists */
		if (linfo.here_doc == B_TRUE) {

			/*
			 * 3 tokens: "mail", "<email>",
			 * "<<".
			 * This is the delete last email
			 * case. If there is a <<EOF:/EOF
			 * format, we delete ALL lines between
			 * and including the endmarkers.
			 * Example, in the text as under,
			 * 	mail root <</EOF
			 * 	blah blah blah
			 * 	EOF
			 * every line including /EOF to
			 * the one with EOF
			 * gets deleted.
			 */
			if (linfo.numtoks <= 3) {
				snprintf(cmd2,
				    sizeof (cmd2),
				    "%s %s%s%s%s%s%s %s %s %s",
				    "/usr/bin/sed",
				    "'/^[^#].*", linfo.token1,
				    "$/,", "/^[^#].*", linfo.token1,
				    "$/d'", notfscript, ">",
				    notf_tmp_new);


			} else {

				/*
				 * Search and replace with sed.
				 * The command is of the form:
				 * sed 's/<pat.>//' notfscript.
				 */
				snprintf(cmd2,
				    sizeof (cmd2),
				    "%s %s%s%s %s %s %s",
				    "/usr/bin/sed",
				    "'/^[^#]/s/ \\<", emailaddr,
				    "\\>//'", notfscript, ">",
				    notf_tmp_new);
			}
		} else if (linfo.here_doc == B_FALSE) {
			/*
			 * 2 tokens:
			 * "mail", "<email>"
			 */
			if (linfo.numtoks <= 2) {
				/*
				 * Delete the entire line
				 * if this is the last
				 * email.
				 */
				snprintf(cmd2,
				    sizeof (cmd2),
				    "%s %s%s%s%s%s %s %s %s",
				    "/usr/bin/sed",
				    "'/^[^#]*", MAIL,
				    ".*", emailaddr,
				    "/d'",
				    notfscript, ">",
				    notf_tmp_new);
			} else {
				/*
				 * Search and replace with sed.
				 * The command is of the form:
				 * sed 's/<pat.>//' notfscript.
				 */
				snprintf(cmd2,
				    sizeof (cmd2), "%s %s%s%s %s %s %s",
				    "/usr/bin/sed", "'/^[^#]/s/ \\<", emailaddr,
				    "\\>//'", notfscript, ">", notf_tmp_new);
			}

		}
	} else if (action == MOD) {

			/*
			 * Search and replace with sed.
			 * The command is of the form:
			 * sed 's/emailaddr/newemail/' notfscript.
			 * The output is redirected to a tmp file.
			 */
			snprintf(cmd2, sizeof (cmd2),
			    "%s %s%s%s%s%s %s %s %s",
			    "/usr/bin/sed", "'/^[^#]/s/ \\<", emailaddr,
			    "\\>/ ", newemail, "/'", notfscript, ">",
			    notf_tmp_new);
	}



	Trace(TR_DEBUG, "search and replace: sed cmd is %s\n", cmd2);

	if ((ptr = popen(cmd2, "w")) == NULL) {
		samerrno = SE_POPEN_WRITE_OPEN_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_POPEN_WRITE_OPEN_FAILED),
		    cmd2);

		Trace(TR_DEBUG,
		    " search and replace failed on file %s: %s",
		    notfscript, samerrmsg);
		return (-1);
	}

	if (pclose(ptr) == -1) {
		samerrno = SE_POPEN_CLOSE_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_POPEN_CLOSE_FAILED),
		    cmd2);
		Trace(TR_DEBUG,
		    " search and replace failed on file %s: %s",
		    notfscript, samerrmsg);
		return (-1);
	}

	/*
	 * The use of 2 diff command buffers instead of
	 * only one, which is memset to '0' before reuse
	 * seems a bit inefficient but safer and avoids 3
	 * calls to memset().
	 *
	 * copy this file back to the original script
	 */

	/* copy the modifications back to the original file */
	snprintf(cmd3, sizeof (cmd3),
	    "%s %s %s", "/usr/bin/cp -f",
	    notf_tmp_new, notfscript);

	/* exec the command */
	if ((ptr = popen(cmd3, "w")) == NULL) {
		samerrno = SE_POPEN_WRITE_OPEN_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_POPEN_WRITE_OPEN_FAILED),
		    cmd3);
		/* remove the tmp file */
		unlink(notf_tmp_new);	/* if cannot remove, it is not an err */
		Trace(TR_DEBUG,
		    "search and replace failed on file %s: %s",
		    notfscript, samerrmsg);
		return (-1);
	}

	if (pclose(ptr) == -1) {
		samerrno = SE_POPEN_CLOSE_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_POPEN_CLOSE_FAILED),
		    cmd3);
		/* remove the tmp file */
		unlink(notf_tmp_new);	/* if cannot remove, it is not an err */
		Trace(TR_DEBUG,
		    "search and replace failed on file %s: %s",
		    notfscript, samerrmsg);
		return (-1);
	}

	/* remove the tmp file */
	unlink(notf_tmp_new);	/* if cannot remove, it is not an err */

	Trace(TR_DEBUG, "searched and replaced");

	return (0);
}

/*
 * walk the list of notf. summaries.
 * match the email passed in. If the addr_value passed in is not null,
 * match it as well If it exists. update the notf_subj_arr.
 */
int
update_notf_lst(
notf_subj_t notf_subj,
char *addr,
char *addr_value, /* can be NULL */
sqm_lst_t *notf_lst)
{
	node_t  *walker;
	notf_summary_t  *node; /* a node of type notf_summary_t */

	Trace(TR_DEBUG, "update the subscriptions if addr found");

	walker = notf_lst->head;
	for (; walker != NULL; walker = walker->next) {

		node = (notf_summary_t *)walker->data;
		if (node != NULL) {

			/* check if the email already exists */
			if ((strcmp(node->emailaddr, addr)) == 0) {
				if (addr_value != NULL &&
				    strcmp(node->admin_name, addr_value) != 0) {
					return (0);
				}
				/* update the notf_subj_arr */
				node->notf_subj_arr[notf_subj] = 1;

				Trace(TR_DEBUG, "checked notf. existence...");
				return (1);
			}
		}
	}

	Trace(TR_DEBUG, "did not find the address");

	return (0);
}


/*
 * this routine is used to
 * add notifications to files
 * which do not have any previous
 * notifications. While for
 * 3 of the 4 cases handled, the
 * the action is to append a standard
 * static text in the file, for the
 * case of archiver.sh, a new file
 * is copied in and the given email
 * is inserted.
 */
int
add_notf_if_not_exist(
upath_t emailaddr,
char *notfscript)
{
	char default_script[1024] = {0};
	FILE	*fp;
	char scriptname[64] = {0};

	if (ISNULL(notfscript)) {
		Trace(TR_ERR, "could not get script name");
		return (-1);
	}
	char *separator = strrchr(notfscript, '/');
	if (ISNULL(separator)) {
		Trace(TR_ERR, "could not get script name");
		return (-1);
	}
	snprintf(scriptname, sizeof (scriptname), separator + 1);
	if (scriptname == NULL || scriptname[0] == '\0') {
		Trace(TR_ERR, "could not get script name");
		return (-1);
	}

	char *dev_down_text =
	    "/usr/bin/mailx -s \"SAM-FS Device downed\" %s <<EOF\n"
	    "`date`\n"
	    "SAM-FS has marked the device $5,\n"
	    "as down or off.  Check device log.\n"
	    "EOF\n";

	char *load_notify_text =
	    "/usr/bin/mailx -s \"SAM-FS needs VSN $5\" %s <<EOF\n"
	    "`date`\n"
	    "SAM-FS needs VSN $5 manually loaded or imported."
	    " Check preview display.\n"
	    "EOF\n";

	char *recycler_text =
	    "/usr/bin/mailx -s \"Robot $6 at hostname `hostname` recycle.\" %s"
	    "<</eof\n"
	    "The "SBIN_DIR"/recycler.sh "
	    "script was called by the SAM-FS recycler\n"
	    "with the following arguments:\n"
	    "\n"
	    "\tMedia type: $5($1)  VSN: $2  Slot: $3  Eq: $4\n"
	    "\tLibrary: $6\n"
	    "\n"
	    SBIN_DIR"/recycler.sh is a script"
	    " which is called when the recycler\n"
	    "determines that a VSN has been drained "
	    "of all known active archive\n"
	    "copies.  You should determine your site "
	    "requirements for disposition of\n"
	    "recycled media - some sites wish to relabel "
	    "and reuse the media, some\n"
	    "sites wish to take the media out of the "
	    "library for possible later use\n"
	    "to access historical files. "
	    "Consult the recycler(1m) man page for more\n"
	    "information.\n"
	    "/eof\n";

	line_info_t	linfo; /* variable to store line info */
	int	ret;

	Trace(TR_DEBUG, "Attempting to add email: %s "
	    "in file: %s with no previous notifications",
	    emailaddr, notfscript);

	/* initialize fields */
	linfo.pat_exists = B_FALSE;
	linfo.rep_exists = B_FALSE;
	linfo.here_doc = B_FALSE;
	linfo.comment = B_TRUE; /* line is commented by default */
	linfo.numtoks = 0;

	/* if not the archiver.sh */
	if (strcmp(notfscript, ARCH_INTR) != 0) {
		Trace(TR_DEBUG, "the file being added to: %s\n",
		    notfscript);

		/* open notification file in append mode */
		fp = fopen(notfscript, "a");
		if (fp == NULL) {
			samerrno = SE_FILE_APPEND_OPEN_FAILED;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_FILE_APPEND_OPEN_FAILED),
			    notfscript);

			Trace(TR_DEBUG, "Add notification to file: %s"
			    "  w/o previous notification failed: %s",
			    notfscript, samerrmsg);
			return (-1);
		}

		if ((strcmp(notfscript, DEV_DOWN)) == 0) {
			/* Write out the dev down text */
			fprintf(fp, dev_down_text, emailaddr);
		} else if ((strcmp(notfscript, MED_REQ)) == 0) {
			/* Write out the load notify text */
			fprintf(fp, load_notify_text, emailaddr);
		} else if ((strcmp(notfscript, RECY_STAT_RC)) == 0) {
			/* Write out recycler text */
			fprintf(fp, recycler_text, emailaddr);
		}

		fclose(fp);

	} else { /* the archiver.sh */

		/* copy from examples dir */
		snprintf(default_script, sizeof (default_script),
		    "%s/%s", SAM_EXAMPLES_PATH, scriptname);

		ret = cp_file(default_script, notfscript);
		if (ret < 0) {
			Trace(TR_DEBUG, "adding notfn. to file failed");
			return (-1);
		}

		/* copy the replacement email into newemail */
		strlcpy(newemail, emailaddr, sizeof (upath_t));

		newemail_set = B_TRUE;

		ret = search_replace(notfscript, "root", MOD,
		    linfo);

		if (ret < 0) {
			Trace(TR_DEBUG,
			    "adding notfn. to file failed");
			return (-1);
		}
	}

	Trace(TR_DEBUG, "added email: %s in file: %s "
	    "with no previous notifications",
	    emailaddr, notfscript);

	return (0);
}

/*
 * gets notification summary for email
 *
 */
int
get_notification_for_addr(
char *addr,		/* email address of recepient */
notf_summary_t **notification	/* return - notification */
)
{
	sqm_lst_t *lst;
	node_t *node;

	if (ISNULL(addr, notification)) {
		Trace(TR_ERR, "Get notf for addr failed: [%s]", samerrmsg);
		return (-1);
	}

	*notification = NULL;
	Trace(TR_DEBUG, "Get notifications for addr %s", addr);

	if (get_notify_summary(NULL, &lst) == -1) {
		Trace(TR_ERR, "Get notification for addr %s failed: %s",
		    addr, samerrmsg);
		return (-1);
	}

	node = lst->head;
	for (; node != NULL; node = node->next) {

		if ((strcmp(((notf_summary_t *)node->data)->emailaddr, addr))
		    == 0) {

			*notification = (notf_summary_t *)node->data;
			Trace(TR_DEBUG, "Got notf for email [%s]", addr);

			lst_free(lst);
			return (0);
		}
	}

	/* No notifications for email <addr> */
	samerrno = SE_NOT_FOUND;
	snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno), addr);

	lst_free(lst);
	Trace(TR_ERR, "Get notification by email[%s] failed: %s",
	    addr, samerrmsg);

	return (-1);
}

/*
 * Checks if <addr> exists in the list of addrs
 * addrs are separated by commas
 * Returns B_TRUE if addr exists, B_FALSE if addr does not exist
 */
boolean_t
addr_exists(
	const char *addrs,
	const char *addr)
{

	char *p_addrs = NULL;
	char *ptr1, *ptr2, *ptr3;

	p_addrs = mallocer(strlen(addrs));
	strlcpy(p_addrs, addrs, strlen(addrs));

	while (p_addrs != NULL) {

		/* trim leading white spaces */
		p_addrs += strspn(p_addrs, WHITESPACE);
		if ((ptr1 = strchr(p_addrs, ',')) != NULL) {
			*ptr1 = '\0';
			ptr1++;
		}
		ptr2 = p_addrs;

		/* trim leading white spaces */
		ptr2 += strspn(ptr2, WHITESPACE);
		/* remove trailing white spaces */
		ptr3 = strrspn(ptr2, WHITESPACE);
		if (ptr3 != NULL)
			*ptr3 = '\0';

		if (*ptr2 != '\0') {
			if (strcmp(addr, p_addrs) == 0) {
				/* address exists */
				return (B_TRUE);
			}
		}
		p_addrs = ptr1;
	}
	return (B_FALSE);
}


int
get_addr(char *type, sqm_lst_t **addrs)
{
	struct	stat	buf;	/* struct for file info */
	FILE *in;
	char *ptr;
	int typeln, i;
	char buffer[1024];

	if (ISNULL(addrs)) {
		/* samerrno and samerrmsg are already set */
		return (-1);
	}

	if (stat(NOTIFYFILE, &buf) != 0) {
		/* if file does not exists, then no notifications are setup */
		samerrno = SE_FILE_NOT_PRESENT;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), NOTIFYFILE);
		return (-1);
	}

	if ((in = fopen(NOTIFYFILE, "r")) == NULL) {
		samerrno = SE_FILE_READ_OPEN_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), NOTIFYFILE);
		return (-1);
	}

	typeln = strlen(type);	/* len of notification type keyword */

	/*
	 * Loop reading every line in file looking for notification type
	 * provided as input
	 */
	for (;;) {
		ptr = fgets(buffer, 1024, in);

		if (ptr == NULL) {	/* End of file, Didn't find notif */
			fclose(in);
			samerrno = SE_NOT_FOUND;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(samerrno), type);

			return (-1);
		}
		/* remove new line char */
		ptr[strlen(buffer) - 1] = '\0';
		if (ptr == NULL) { /* line consisting of only a newline */
			continue;
		}
		i = strcspn(ptr, WHITESPACE); /* Find end of notif type */

		if ((i == typeln) && strncmp(type, ptr, typeln) == 0) {

			/* get email recepients for notification type <type> */
			/* convert string to a list, strip whitespace */
			*addrs = str2lst((ptr+(typeln + 1)), ",");
			if (addrs == NULL) {
				/* samerrno and samerrmsg are already set */
				fclose(in);
				return (-1);
			}
			break;	/* Found it, stop searching file. */
		}
	}

	fclose(in);
	return (0);

}


/*
 * delete email address <addr> specified as input for notification type <type>
 *
 * returns success(0) if addr is deleted successfully
 * returns error(-1) if internal system failures
 * if addr is not found, it is not considered an error
 */
static int
del_addr(
	char *in_type,
	char *addr)
{

	struct stat buf;
	int typeln = 0;
	char *type = in_type, *p_type = NULL;

	FILE *infile = NULL, *tmpfile = NULL;
	int fd = -1;
	char tmpfilnam[MAXPATHLEN] = {0};

	char buffer[1024] = {0};
	char *ptr = NULL;

	if (stat(NOTIFYFILE, &buf) != 0) { /* file does exist is not an err */
		return (0);
	}

	type += strspn(type, WHITESPACE); /* trim leading white space */
	p_type = strrspn(type, WHITESPACE); /* remove trailing white spaces */
	if (p_type != NULL)
		*p_type = '\0';

	if (ISNULL(type)) {
		Trace(TR_ERR, "add notification failed: %s", samerrmsg);
		return (-1);
	}
	typeln = strlen(type);

	if (mk_wc_path(NOTIFYFILE, tmpfilnam, sizeof (tmpfilnam)) != 0) {
		Trace(TR_ERR, "Unable to create temporary file for notify");
		return (-1);
	}

	if ((fd = open64(tmpfilnam, O_WRONLY|O_CREAT|O_TRUNC, 0644)) != -1) {
		tmpfile = fdopen(fd, "w+");
	}

	if (tmpfile == NULL) {
		samerrno = SE_NOTAFILE;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
		    tmpfilnam);
		unlink(tmpfilnam);
		if (fd != -1) {
			close(fd);
		}
		return (-1);
	}

	/* file already exists, open it for reading */
	if ((infile = fopen(NOTIFYFILE, "r")) == NULL) {

		samerrno = SE_FILE_READ_OPEN_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_FILE_READ_OPEN_FAILED), NOTIFYFILE);
		Trace(TR_DEBUG, "%s", samerrmsg);

		fclose(tmpfile);
		unlink(tmpfilnam);
		return (-1);
	}

	for (;;) {
		if ((ptr = fgets(buffer, 1024, infile)) == NULL) {
			break;
		}
		ptr += strspn(ptr, WHITESPACE); /* trim leading white space */
		char *ptr1 = strrspn(ptr, WHITESPACE);
		if (ptr1 != NULL) /* trim trailing whitespace */
			*ptr1 = '\0';
		if (ptr == NULL) { /* empty lines */
			continue;
		}

		char linebuf[1024] = {0};
		strncpy(linebuf, ptr, 1023);
		/*
		 * read each line, search for <type> type of notification
		 * then search through the addresses listed for the <type>
		 * if the <addr> to be deleted exists, then do not copy that
		 * address to the tmp file, copy all other address for
		 * notification type <type> into tmp file
		 */

		int i = 0; /* count of characters that are not whitespace */
		i = strcspn(ptr, WHITESPACE); /* Find end of notif type */

		if ((i != typeln) || (strncmp(type, ptr, typeln) != 0)) {
			/* some other notification type, just copy it to out */
			fprintf(tmpfile, "%s\n", ptr);
			continue;

		}
		/* some addresses for notification type <type> exist */
		char nstr[1024] = {0};
		snprintf(nstr, typeln + 2, /* one for space */ "%s ", ptr);

		char *token = NULL, *lasts = NULL;
		token = strtok_r(ptr + typeln + 1, ",", &lasts);
		if (token != NULL) {
			if (strncmp(addr, token, strlen(token)) == 0) {
				/* found addr to be deleted */

				/* are there any more addresses ? */
				int skipln = typeln + 1 + strlen(addr);
				if (strlen(linebuf) > skipln) {
					strlcat(nstr, (ptr + skipln + 1), 1024);
					fprintf(tmpfile, "%s\n", nstr);
					continue; /* write remaining contents */
				}
			} else {
				strlcat(nstr, token, 1024);
			}
			while ((token = strtok_r(NULL, ",", &lasts)) != NULL) {
				if (strncmp(addr, token, strlen(token)) != 0) {
					strlcat(nstr, ",", 1024);
					strlcat(nstr, token, 1024);
				}
			}

			fprintf(tmpfile, "%s\n", nstr);
		}
	}

	fclose(infile);
	fclose(tmpfile);
	rename(tmpfilnam, NOTIFYFILE);
	return (0);

}


int
add_addr(
	char *in_type,
	char *new_addr)
{

	struct stat buf;
	int fd = -1;
	FILE *infile = NULL, *tmpfile = NULL;
	char tmpfilnam[MAXPATHLEN] = {0};

	int typeln = 0;
	char *type = in_type, *p_type = NULL;

	char buffer[1024] = {0};
	char *ptr = NULL, *old_addr = NULL;

	/* If file does not exist, create it, append type and addr */
	if (stat(NOTIFYFILE, &buf) != 0) {
		/* create file for writing */
		if ((fd = open64(NOTIFYFILE, O_WRONLY | O_CREAT, 0644)) != -1) {
			infile = fdopen(fd, "w");
		}
		if (infile == NULL) {
			samerrno = SE_NOTAFILE;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(samerrno), NOTIFYFILE);
			if (fd != -1) {
				close(fd);
			}
			return (-1);
		}
		fprintf(infile, "%s %s\n", type, new_addr);
		fclose(infile);
		infile = NULL;
		return (0);
	}
	type += strspn(type, WHITESPACE); /* trim leading white space */
	p_type = strrspn(type, WHITESPACE); /* remove trailing white spaces */
	if (p_type != NULL)
		*p_type = '\0';

	if (ISNULL(type)) {
		Trace(TR_ERR, "add notification failed: %s", samerrmsg);
		return (-1);
	}
	typeln = strlen(type);

	/* Create a temporary file */
	if (mk_wc_path(NOTIFYFILE, tmpfilnam, sizeof (tmpfilnam)) != 0) {
		Trace(TR_ERR, "Unable to create temporary file: %s", samerrmsg);
		return (-1);
	}
	if ((fd = open64(tmpfilnam,  O_WRONLY|O_CREAT|O_TRUNC, 0644)) != -1) {
		tmpfile = fdopen(fd, "w+");
	}
	if (tmpfile == NULL) {
		samerrno = SE_NOTAFILE;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
		    tmpfilnam);
		Trace(TR_ERR, "Unable to open temporary file: %s", samerrmsg);
		if (fd != -1) {
			close(fd);
		}
		unlink(tmpfilnam);
		return (-1);
	}

	/* file already exists, open it for reading */
	if ((infile = fopen(NOTIFYFILE, "r")) == NULL) {

		samerrno = SE_FILE_READ_OPEN_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_FILE_READ_OPEN_FAILED), NOTIFYFILE);
		Trace(TR_DEBUG, "%s", samerrmsg);

		fclose(tmpfile);
		unlink(tmpfilnam);
		return (-1);
	}
	for (;;) {
		ptr = fgets(buffer, 1024, infile);
		if (ptr == NULL) {
			break;
		}
		ptr += strspn(ptr, WHITESPACE); /* trim leading white space */
		char *ptr1 = strrspn(ptr, WHITESPACE);
		if (ptr1 != NULL) /* trim trailing whitespace */
			*ptr1 = '\0';
		if (ptr == NULL) { /* empty lines */
			continue;
		}
		/*
		 * read each line, search for type <type> of notification
		 * If it is not found, append the type and new addr to the file
		 * If type of notification is found, go to the end of the line
		 * and add this in
		 */
		int i = 0;
		i = strcspn(ptr, WHITESPACE); /* Find end of notif type */

		if ((i != typeln) || (strncmp(type, ptr, typeln) != 0)) {
			/* some other notification type, just copy it to out */
			fprintf(tmpfile, "%s\n", ptr);
			continue;

		}

		if (strlen(ptr) > typeln) {
			/* some address for <type> exist */
			old_addr = copystr(ptr);
		}

	}
	fclose(infile);

	if (old_addr == NULL) {	/* End of file, Didn't find notification */
		fprintf(tmpfile, "%s %s\n", type, new_addr);
		fclose(tmpfile);
		rename(tmpfilnam, NOTIFYFILE);
		return (0);
	}
	/* check if new_addr already exists for notification type <type> */
	if (addr_exists((old_addr + typeln + 1), new_addr) == B_TRUE) {
		/* address is already added as a recepient, Duplicate */
		/* not an error */
		fclose(tmpfile);
		unlink(tmpfilnam);
		return (0);
	}
	/* write the addrs for notification type <type> */
	/* strip the whitespace */
	fprintf(tmpfile, "%s %s,%s\n", type, new_addr, old_addr + typeln + 1);
	fclose(tmpfile);
	rename(tmpfilnam, NOTIFYFILE);
	return (0);
}
