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
#pragma ident   "$Revision: 1.17 $"
static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/wait.h>
#include "pub/mgmt/mgmt.h"
#include "pub/mgmt/sqm_list.h"

#include "mgmt/config/common.h"
#include "pub/mgmt/task_schedule.h"
#include "mgmt/util.h"
#include "sam/sam_trace.h"


/*
 * MAX_SCHED_STRLEN must be sufficient to hold all of the additional
 * parameters possible for snapshot scheduling. There are 17 possible
 * paths in the snapshot schedule. The extra MAXPATHLEN here accomodates the
 * schedule, the key names and values for the non-path options to the snapshot
 * schedule.
 */
#define	MAX_SCHED_STRLEN 18 * MAXPATHLEN
#define	IAS_TASKLET_SCHEDULE_FILE CFG_DIR"/tasklet_sched.conf"
#define	FSMGMT_SCHEDULE_FILE CFG_DIR"/mgmt_sched.conf"

static int sig_ias_daemon(task_sched_t *ts);
static int validate_ias_sched(task_sched_t *in);
static int validate_mgmt_sched(task_sched_t *in);
static int set_cron_sched(task_sched_t *in);
static int remove_cron_sched(task_sched_t *in);
static int read_schedule_file(char *filename, char *task,
    char *id, sqm_lst_t **l);

static void free_task_sched(task_sched_t *ts);
static int write_schedule(char *filename, sqm_lst_t *l);
static int get_task_index(char *task);
static int update_crontab(char *croncmd, sqm_lst_t *cronlist,
    boolean_t do_delete);

static int generate_crontab_entry(char *start, char *period, char *cmdline,
		char **croncmd);


typedef enum which_schedule {
	IAS_SCHEDULE_ID,
	MGMT_SCHEDULE_ID
} which_schedule_t;


static char *schedule_files[] = {
	FSMGMT_SCHEDULE_FILE,
	IAS_TASKLET_SCHEDULE_FILE,
	""
};


typedef struct task_handler {
	char *task_type;
	int (*validate)(task_sched_t *ts);
	int (*set)(task_sched_t *ts);
	int (*remove)(task_sched_t *ts);
	which_schedule_t file_id;
	boolean_t add_allowed;
	boolean_t remove_allowed;
} task_handler_t;


static task_handler_t handlers[] = {
	{ "RC", validate_mgmt_sched, NULL, NULL, 0, 1, 1},
	{ "SN", validate_cron_sched, set_cron_sched, remove_cron_sched, 0, 1,
		1},
	{ "RP", validate_cron_sched, set_cron_sched, remove_cron_sched, 0, 1,
		1},
	{ "AW", validate_ias_sched, sig_ias_daemon, sig_ias_daemon, 1, 0, 0},
	{ "HC", validate_ias_sched, sig_ias_daemon, sig_ias_daemon, 1, 0, 0},
	{ "HI", validate_ias_sched, sig_ias_daemon, sig_ias_daemon, 1, 0, 0},
	{ "PA", validate_ias_sched, sig_ias_daemon, sig_ias_daemon, 1, 0, 0},
	{ "AD", validate_ias_sched, sig_ias_daemon, sig_ias_daemon, 1, 0, 0},
	{ "DD", validate_ias_sched, sig_ias_daemon, sig_ias_daemon, 1, 0, 0},
	{ "" }
};


static parsekv_t task_sched_tokens[] = {
	{"task", offsetof(struct task_sched, task),
		parsekv_string_3},
	{"starttime", offsetof(struct task_sched, starttime),
		parsekv_string_16},
	{"duration", offsetof(struct task_sched, duration),
		parsekv_string_16},
	{"periodicity", offsetof(struct task_sched, periodicity),
		parsekv_string_16},
	{"id", offsetof(struct task_sched, id), parsekv_string_1072},
	{"", 0, NULL}
};


/*
 * The input list can either be NULL in which case a list will be created
 * or it can point to an existing list to which results will be appended.
 * In the event of an error the contents of the list will be freed.
 *
 * Returns a list of task_sched_t structures. This does not validate
 * the individual kv pairs. It simply parses out the string values
 * for the keys.
 */
static int
read_schedule_file(
char *filename,
char *task,
char *id,
sqm_lst_t **l)
{

	FILE	*indat;
	char	*ptr;
	char	buffer[MAX_SCHED_STRLEN];
	int	status = 0;
	task_sched_t *ts = NULL;

	Trace(TR_MISC, "read schedule file %s", Str(filename));

	if (ISNULL(filename, l)) {
		Trace(TR_ERR, "reading schedule file failed %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}

	if (*l == NULL) {
		*l = lst_create();
		if (*l == NULL) {
			Trace(TR_ERR, "reading schedule file failed %d %s",
			    samerrno, samerrmsg);
			return (-1);
		}
	}

	/* Open and read the file into a list */
	indat = fopen64(filename, "r");
	if (indat == NULL) {
		if (errno == ENOENT) {
			/* return an empty list */
			return (0);
		} else {
			lst_free_deep_typed(*l, FREEFUNCCAST(free_task_sched));
			samerrno = SE_CFG_OPEN_FAILED;

			/* Open failed for %s: %s */
			StrFromErrno(errno, buffer, sizeof (buffer));
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_CFG_OPEN_FAILED), filename,
			    buffer);
			Trace(TR_ERR, "reading schedule file failed %d %s",
			    samerrno, samerrmsg);

			return (-1);
		}
	}
	for (;;) {
		uint32_t cnt;

		ptr = fgets(buffer, sizeof (buffer), indat);

		if (ptr == NULL)
			break; /* End of file */

		/* skip comments and whitespace only lines */
		while (isspace(*ptr)) {
			ptr++;
		}

		if (*ptr == '\0' || *ptr == '#') {
			continue;
		}

		cnt = strlen(ptr);
		if (cnt == 0) {
			/* line was comment or white space only */
			continue;
		}
		/* get rid of trailing new lines */
		if (*(ptr + cnt - 1) == '\n') {
			*(ptr + cnt - 1) = '\0';
		}

		ts = mallocer(sizeof (task_sched_t));
		if (ts == NULL) {
			status = -1;
			break;
		}
		memset(ts, 0, sizeof (task_sched_t));

		if (parse_kv(ptr, task_sched_tokens, ts) != 0) {
			status = -1;
			break;
		}

		if ((task != NULL) && (*task != '\0') &&
		    ((strcmp(task, ts->task)) != 0)) {
			continue;
		}

		if ((id != NULL) && (*id != '\0') &&
		    ((strcmp(id, ts->id)) != 0)) {
			continue;
		}

		/* Keep the original string too to avoid rebuilding it. */
		ts->kv_string = strdup(ptr);
		if (ts->kv_string == NULL) {
			setsamerr(SE_NO_MEM);
			status = -1;
			break;
		}

		if (lst_append(*l, ts) != 0) {
			status = -1;
			break;
		}
	}

	fclose(indat);

	if (status == -1) {
		lst_free_deep_typed(*l, FREEFUNCCAST(free_task_sched));
		free(ts);
		*l = NULL;

		Trace(TR_ERR, "reading schedule file failed %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}

	Trace(TR_MISC, "read schedule file %s", filename);
	return (0);
}

/*
 *  For a task class string, determine the index into the handler list
 */
static int
get_task_index(char *task)
{
	int	task_index = -1;
	int	i;

	if (ISNULL(task)) {
		return (-1);
	}

	for (i = 0; *(handlers[i].task_type) != '\0'; i++) {
		if (strcmp(task, handlers[i].task_type) == 0) {
			task_index = i;
			break;
		}
	}

	if (task_index == -1) {
		/* invalid task */
		samerrno = SE_UNRECOGNIZED_TASK;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
		    task);
	}

	return (task_index);
}

/*
 * aggregates schedule information from the ias repository and
 * our own repository. Returns a list of kv_strings describing the
 * tasks scheduled on the system.
 */
int
get_task_schedules(
ctx_t *c /* ARGSUSED */,
sqm_lst_t **l)
{
	int	st;

	st = get_specific_tasks(c, NULL, NULL, l);

	return (st);
}

/*
 * Retrieves schedule information for specific task classes and/or
 * ids from the ias repository and our own repository.
 *
 *  Both task and id are optional fields.
 *
 *  If task is NULL and id is NULL, all entries will be returned.
 *  If task is non-NULL and id is NULL, all entries for that task class
 *  are returned.
 *  If task is NULL and id is non-NULL, all entries for any class that
 *  match the id are returned.
 *
 * Returns a list of kv_strings.
 */
int
get_specific_tasks(ctx_t *c, char *task, char *id, sqm_lst_t **l) /* ARGSUSED */
{
	char		*funcnam = "getting task schedules";
	sqm_lst_t		*tmp = NULL;
	node_t		*n;
	task_sched_t	*ts;
	int		st;

	Trace(TR_MISC, funcnam);
	if (ISNULL(l)) {
		Trace(TR_ERR, "%s failed %d %s", funcnam,
		    samerrno, samerrmsg);
		return (-1);
	}

	st = get_task_structs(task, id, &tmp);
	if (st != 0) {
		Trace(TR_ERR, "%s failed %d %s", funcnam,
		    samerrno, samerrmsg);
		return (-1);
	}

	*l = lst_create();
	if (*l == NULL) {
		Trace(TR_ERR, "%s failed %d %s", funcnam,
		    samerrno, samerrmsg);
		lst_free_deep_typed(tmp, FREEFUNCCAST(free_task_sched));
		return (-1);
	}

	/* Go through tmp moving the kv strings to the return list */
	for (n = tmp->head; n != NULL; n = n->next) {
		ts = (task_sched_t *)n->data;

		if (lst_append(*l, ts->kv_string) != 0) {
			st = -1;
			break;
		}

		/*
		 * don't allow lst_free_deep_typed to free the
		 * kv_string field because it's inserted into the list
		 * being returned.
		 */
		ts->kv_string = NULL;
	}

	lst_free_deep_typed(tmp, FREEFUNCCAST(free_task_sched));

	if (st != 0) {
		lst_free_deep(*l);
		*l = NULL;
		Trace(TR_ERR, "%s failed %d %s", funcnam,
		    samerrno, samerrmsg);
	}

	return (st);
}

/*
 *  Returns a list of task_sched_t structures to the caller.
 *
 *  Both task and id are optional.  See get_specific_tasks() for more
 *  information on how this is interpreted.
 */
int
get_task_structs(char *task, char *id, sqm_lst_t **l)
{
	char		*funcnam = "getting task structs";
	int		i;
	int		task_type = -1;

	Trace(TR_MISC, funcnam);
	if (ISNULL(l)) {
		Trace(TR_ERR, "%s failed %d %s", funcnam,
		    samerrno, samerrmsg);
		return (-1);
	}

	if ((task != NULL) && (*task != '\0')) {
		task_type = get_task_index(task);
		if (task_type == -1) {
			Trace(TR_ERR, "%s failed %d %s", funcnam,
			    samerrno, samerrmsg);
			return (-1);
		}
	}

	*l = lst_create();
	if (l == NULL) {
		Trace(TR_ERR, "%s failed %d %s", funcnam, samerrno,
		    samerrmsg);
		return (-1);
	}

	/* Read each schedule file into the tmp list */
	for (i = 0; *schedule_files[i] != '\0'; i++) {
		/*
		 * if specific tasks were requested, prune the file
		 * list accordingly.
		 */
		if ((task_type != -1) &&
		    (i != handlers[task_type].file_id)) {
			continue;
		}

		if (read_schedule_file(schedule_files[i], task, id,
		    l) != 0) {

			lst_free_deep_typed(*l, FREEFUNCCAST(free_task_sched));
			*l = NULL;
			Trace(TR_ERR, "%s failed %d %s", funcnam,
			    samerrno, samerrmsg);
			return (-1);
		}

	}

	Trace(TR_MISC, "got task structs");
	return (0);
}


int
set_task_schedule(
ctx_t *c /* ARGSUSED */,
char *kv_string)
{
	task_sched_t	input_task;
	boolean_t	found;
	sqm_lst_t		*l = NULL;
	node_t		*n;
	task_sched_t	*ts;
	int		task_type = -1;
	int		st = 0;

	if (ISNULL(kv_string)) {
		Trace(TR_ERR, "set task schedule failed %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}
	Trace(TR_MISC, "setting task schedule for %s", kv_string);

	/*
	 * parse the input string to validate its basic format and get
	 * the task and id
	 */
	memset(&input_task, 0, sizeof (task_sched_t));

	if (parse_kv(kv_string, task_sched_tokens, &input_task) != 0) {
		Trace(TR_ERR, "set task schedule failed %d %s with %s",
		    samerrno, samerrmsg, kv_string);
		return (-1);
	}
	input_task.kv_string = kv_string;

	/* Make sure this request is for a task type we recognize */
	task_type = get_task_index(input_task.task);

	if (task_type == -1) {
		Trace(TR_ERR, "set task schedule failed %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}

	/*
	 * check the individual fields to make sure their values are
	 * appropriate for their scheduler.
	 */
	if (handlers[task_type].validate != NULL) {
		if (handlers[task_type].validate(&input_task) != 0) {
			Trace(TR_ERR, "set task schedule failed %d %s",
			    samerrno, samerrmsg);
			return (-1);
		}
	}


	/* read the appropriate file */
	if (read_schedule_file(
	    schedule_files[handlers[task_type].file_id],
	    NULL, NULL, &l) != 0) {

		st = -1;
		goto done;
	}


	/* find and modify or add the scheduled task */
	found = B_FALSE;
	for (n = l->head; n != NULL; n = n->next) {
		task_sched_t *ts = (task_sched_t *)n->data;

		if (task_matches(ts, &input_task)) {
			found = B_TRUE;

			/*
			 * free the kv string in the task from the list
			 * and copy the input task into the struct
			 */
			free(ts->kv_string);
			memcpy(ts, &input_task, sizeof (task_sched_t));
			/* private not saved in the file */
			ts->private = NULL;
			ts->kv_string = strdup(input_task.kv_string);
			if (ts->kv_string == NULL) {
				setsamerr(SE_NO_MEM);
				st = -1;
				goto done;
			}
			break;
		}
	}
	if (!found) {
		if (!handlers[task_type].add_allowed) {

			/* not supposed to add new tasklet schedules */
			setsamerr(SE_CANT_ADD_SCHED);
			st = -1;
			goto done;
		} else {
			/*
			 * allocate a new task struct and insert into
			 * the list
			 */
			ts = mallocer(sizeof (task_sched_t));

			if (ts != NULL) {
				memcpy(ts, &input_task, sizeof (task_sched_t));
				/* private not saved in file */
				ts->private = NULL;
				ts->kv_string = strdup(kv_string);
				if (ts->kv_string == NULL) {
					free(ts);
					ts = NULL;
				}
			}
			if (ts == NULL) {
				setsamerr(SE_NO_MEM);
				st = -1;
				goto done;
			}

			if (lst_append(l, ts) != 0) {
				free_task_sched(ts);
				st = -1;
				goto done;
			}
		}
	}

	/*
	 * write the appropriate file
	 */
	if (write_schedule(
	    schedule_files[handlers[task_type].file_id], l) != 0) {

		st = -1;
		goto done;
	}

	/*
	 * Call the set function to either signal the appropriate daemon
	 * or setup the change
	 */
	if (handlers[task_type].set != NULL) {
		if (handlers[task_type].set(&input_task) != 0) {
			st = -1;
		}
	}

done:
	if (l != NULL) {
		lst_free_deep_typed(l, FREEFUNCCAST(free_task_sched));
	}

	if (input_task.private != NULL) {
		free(input_task.private);
		input_task.private = NULL;
	}

	if (st == 0) {
		Trace(TR_MISC, "set task schedule complete for %s", kv_string);
	} else {
		Trace(TR_ERR, "set task schedule failed %d %s",
		    samerrno, samerrmsg);
	}

	return (st);
}

/*
 * Remove the schedule for the task described in the kv_string.
 * The schedules for the IAS Tasklets cannot be removed.
 *
 * If an id key is present in either the configuration file or
 * the input string its value will be used to determine which
 * task to delete.
 */
int
remove_task_schedule(
ctx_t *c /* ARGSUSED */,
char *kv_string)
{

	task_sched_t	input_task;
	boolean_t	found = B_FALSE;
	sqm_lst_t		*l = NULL;
	node_t		*n;
	int		task_type  = -1;

	if (ISNULL(kv_string)) {
		Trace(TR_ERR, "remove task schedule failed: %d %s", samerrno,
		    samerrmsg);
		return (-1);
	}
	Trace(TR_MISC, "removing task schedule for %s", kv_string);

	/* parse the input string to validate its format */
	memset(&input_task, 0, sizeof (task_sched_t));

	if (parse_kv(kv_string, task_sched_tokens, &input_task) != 0) {
		Trace(TR_ERR, "remove task schedule failed %d %s", samerrno,
		    samerrmsg);
		return (-1);
	}

	/* determine which task type it is */
	task_type = get_task_index(input_task.task);

	/*
	 * If it was not a recognized task type return an error.
	 */
	if (task_type == -1) {
		Trace(TR_ERR, "set task schedule failed %d %s",
		    samerrno, samerrmsg);

		return (-1);
	}

	if (handlers[task_type].remove_allowed != B_TRUE) {
		setsamerr(SE_CANT_DELETE_SCHED);

		Trace(TR_ERR, "remove task schedule failed %d %s",
		    samerrno, samerrmsg);

		return (-1);
	}

	/* read the appropriate file */
	if (read_schedule_file(
	    schedule_files[handlers[task_type].file_id],
	    NULL, NULL, &l) != 0) {

		Trace(TR_ERR, "set task schedule failed %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}

	/* find and remove the scheduled task */
	for (n = l->head; n != NULL; n = n->next) {
		task_sched_t *ts = (task_sched_t *)n->data;

		if (task_matches(ts, &input_task)) {
			found = B_TRUE;
			if (lst_remove(l, n) != 0) {
				lst_free_deep_typed(l,
				    FREEFUNCCAST(free_task_sched));

				Trace(TR_ERR, "remove schedule failed %d %s",
				    samerrno, samerrmsg);

				return (-1);
			}
			free_task_sched(ts);
		}
	}


	if (!found) {
		lst_free_deep_typed(l, FREEFUNCCAST(free_task_sched));
		Trace(TR_MISC, "task %s not found for remove", kv_string);
		return (0);
	}

	/*
	 * Write out the appropriate file
	 */
	if (write_schedule(
	    schedule_files[handlers[task_type].file_id], l) != 0) {

		lst_free_deep_typed(l, FREEFUNCCAST(free_task_sched));
		Trace(TR_ERR, "set task schedule failed %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}


	lst_free_deep_typed(l, FREEFUNCCAST(free_task_sched));

	/*
	 * call the remove handler for this task type.
	 */
	if (handlers[task_type].remove != NULL) {
		if (handlers[task_type].remove(&input_task) != 0) {
			Trace(TR_ERR, "set task schedule failed %d %s",
			    samerrno, samerrmsg);
			return (-1);
		}
	}

	Trace(TR_MISC, "removed class schedule ");

	return (0);
}


void
free_task_sched(task_sched_t *ts) {
	if (ts != NULL) {
		if (ts->kv_string != NULL) {
			free(ts->kv_string);
		}
		if (ts->private != NULL) {
			free(ts->private);
		}
		free(ts);
	}
}


static int
write_schedule(char *filename, sqm_lst_t *l) {
	FILE		*f = NULL;
	char		buf[80];
	int		fd;
	node_t		*n;
	time_t		the_time;

	if (ISNULL(filename, l)) {
		Trace(TR_ERR, "writing %s schedule failed: %s",
		    Str(filename), samerrmsg);
		return (-1);
	}

	Trace(TR_ERR, "writing %s schedule", Str(filename));

	backup_cfg(filename);

	if ((fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0644)) != -1) {
		f = fdopen(fd, "w");
	}
	if (f == NULL) {
		samerrno = SE_CFG_OPEN_FAILED;
		/* Open failed for %s: %s */
		StrFromErrno(samerrno, buf, sizeof (buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_CFG_OPEN_FAILED), filename, buf);

		Trace(TR_ERR, "writing %s schedule failed: %s",
		    filename, samerrmsg);

		return (-1);
	}
	fprintf(f, "#\n#\t%s\n#", filename);
	the_time = time(0);
	ctime_r(&the_time, buf, sizeof (buf));
	fprintf(f, "\n#\tGenerated by config api %s#\n", buf);


	for (n = l->head; n != NULL; n = n->next) {
		task_sched_t *ts = (task_sched_t *)n->data;
		fprintf(f, "%s\n", ts->kv_string);
	}
	fclose(f);

	backup_cfg(filename);

	Trace(TR_MISC, "wrote %s schedule", filename);

	return (0);
}


static int
validate_ias_sched(task_sched_t *in) {
	if (*(in->duration) == '\0') {
		setsamerr(SE_DURATION_REQUIRED);
		return (-1);
	}
	return (validate_task_schedule(in, B_FALSE));
}

static int
validate_mgmt_sched(task_sched_t *in) {
	return (validate_task_schedule(in, B_TRUE));
}

static int
sig_ias_daemon(task_sched_t *in /*ARGSUSED*/) {
	return (0);
}

/*
 * currently preforms a very basic validation
 */
int
validate_task_schedule(task_sched_t *in, boolean_t extended_units) {
	char *allowed_units;
	uint32_t value;
	char unit;

	if (extended_units) {
		allowed_units = EXTENDED_PERIOD_UNITS;
	} else {
		allowed_units = BASIC_PERIOD_UNITS;
	}

	/* date validation to be added later */

	if (*(in->starttime) == '\0') {
		setsamerr(SE_STARTTIME_REQUIRED);
		return (-1);
	}

	/* Duration can be empty without it being an error */
	if (*(in->duration) != '\0' && translate_period(in->duration,
	    allowed_units, &value, &unit) != 0) {
		return (-1);
	}
	/* A periodicity is required */
	if (*(in->periodicity) == '\0') {
		setsamerr(SE_PERIODICITY_REQUIRED);
		return (-1);
	} else if (translate_period(in->periodicity, allowed_units,
	    &value, &unit) != 0) {
		return (-1);
	}
	return (0);
}

/*
 *  Used when new schedules are added by other functions in this module,
 *  and also by the utility function to generate crontab entries
 *  for handling at kit install/delete time.
 */
int
validate_cron_sched(task_sched_t *in)
{
	int	st = -1;
	char	cmdbuf[2048];
	char	*cmdbufp = NULL;

	if (ISNULL(in)) {
		return (-1);
	}

	/* Metadata Snapshot Schedules */
	if (strcmp(in->task, "SN") == 0) {
		snprintf(cmdbuf, sizeof (cmdbuf), "%s %s",
		    SBIN_DIR"/samcrondump -f", in->id);
		cmdbufp = cmdbuf;
	} else if (strcmp(in->task, "RP") == 0) {
		snprintf(cmdbuf, sizeof (cmdbuf), "%s %s",
		    SBIN_DIR"/samcrondump -L -f", in->id);
		cmdbufp = cmdbuf;
	}

	st = generate_crontab_entry(in->starttime, in->periodicity,
	    cmdbufp, (char **)&(in->private));

	return (st);
}

/* handlers for tasks requiring cron */
static int
set_cron_sched(task_sched_t *in)
{
	int	st = -1;

	if (ISNULL(in)) {
		return (-1);
	}

	/* use the private field to pass around the crontab entry */
	if (ISNULL(in->private)) {
		return (-1);
	}

	st = set_crontab_entry((char *)in->private, NULL);

	return (st);
}

static int
remove_cron_sched(task_sched_t *in)
{
	int	st = -1;
	char	cmdbuf[2048];

	if (ISNULL(in)) {
		return (-1);
	}

	/* Metadata Snapshot Schedules */
	if (strcmp(in->task, "SN") == 0) {
		snprintf(cmdbuf, sizeof (cmdbuf), "%s %s",
		    SBIN_DIR"/samcrondump", in->id);
	} else if (strcmp(in->task, "RP") == 0) {
		snprintf(cmdbuf, sizeof (cmdbuf), "%s %s",
		    SBIN_DIR"/samcrondump -L -f", in->id);
	} else {
		/* Unrecognized task, just ignore the request */
		return (0);
	}

	st = delete_crontab_entry(cmdbuf, NULL);

	return (st);
}

/*
 * compares only the task and id fields of the task_sched_t inputs
 * returns 1 if they match 0 if they don't.
 */
int
task_matches(task_sched_t *t1, task_sched_t *t2) {

	if (strcmp(t1->task, t2->task) == 0) {
		return (strcmp(t1->id, t2->id) == 0);
	}
	return (0);
}

/*  Functions to support validating and setting crontab entries */

/*
 * set_crontab_entry()
 *
 * Given a crontab entry or list of entries, adds to the root crontab file.
 * If entry with same command line exists, updates it.  Once file is complete,
 * notifies cron of the update.
 *
 */
int
set_crontab_entry(char *croncmd, sqm_lst_t *cronlist)
{
	int		st;

	if (ISNULL(croncmd) && ISNULL(cronlist)) {
		return (-1);
	}

	st = update_crontab(croncmd, cronlist, FALSE);

	return (st);
}

/*
 * delete_crontab_entry()
 *
 * Given a crontab entry or list of entries, deletes from the root crontab
 * file.
 */
int
delete_crontab_entry(char *croncmd, sqm_lst_t *cronlist)
{
	int		st;

	if (ISNULL(croncmd) && ISNULL(cronlist)) {
		return (-1);
	}

	st = update_crontab(croncmd, cronlist, TRUE);

	return (st);
}

/*
 * does all the real work updating cron. if cronlist != NULL, processes
 * multiple entries in a single pass
 */
static char *CRONTABFILE = "/var/spool/cron/crontabs/root";

static int
update_crontab(char *croncmd, sqm_lst_t *cronlist, boolean_t do_delete)
{
	int		st = 0;
	node_t		fakenode;
	node_t		*node = NULL;
	char		tmpfilnam[MAXPATHLEN + 1] = {0};
	char		cmdbuf[MAXPATHLEN * 2];
	int		fd = -1;
	FILE		*fp = NULL;
	boolean_t	changed = FALSE;
	boolean_t	replaced;
	char		*ptr;
	char		*ptrp;
	char		*cbuf;
	char		*cbufp;
	char		c;
	int		i;
	off64_t		tabsize;
	off64_t		len;
	off64_t		plen;
	char		**addArr = NULL;
	int		addSz = 0;
	pid_t		pid = -1;

	if ((ISNULL(croncmd)) && (ISNULL(cronlist))) {
		return (-1);
	}

	if (croncmd) {
		fakenode.data = (void*)croncmd;
		fakenode.next = NULL;
		node = &fakenode;
	} else {
		node = cronlist->head;
	}

	/*  read in the existing crontab file */
	cbuf = copylist64(CRONTABFILE, &tabsize);
	if (cbuf == NULL) {
		samerrno = SE_CANT_READ_FILE;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
		    CRONTABFILE);
		return (-1);
	}

	/* if it's empty, and we're deleting, nothing to do */
	if (do_delete && (tabsize == 0)) {
		free(cbuf);
		return (0);
	}

	for (; node != NULL; node = node->next) {
		replaced = FALSE;
		ptr = (char *)node->data;

		if (ptr == NULL) {
			break;
		}
		/*
		 * no need to account for trailing nul.  the comparison
		 * later when deciding if there's room to write this entry
		 * is also a strlen(), and the nul already exists
		 */
		plen = strlen(ptr);

		/* Reduce ptr to just the executable command for matching */
		if (*ptr != '/') {
			/* crontab parts separated by space or tab */
			while (*ptr != '\0') {
				ptrp = strpbrk(ptr, " \t");
				ptr = ++ptrp;
				if (*ptr == '/') {
					break;
				}
			}
		}
		if (*ptr == '\0') {
			continue;
		}

		/* search for a match */
		for (i = 0; i < tabsize; i++) {
			cbufp = &cbuf[i];

			if ((*cbufp == '\0') || (isspace(*cbufp))) {
				continue;
			}

			/* set up offset to the next entry */
			len = strlen(cbufp);
			i += len;

			if (*cbufp == '#') {
				continue;
			}

			ptrp = strstr(cbufp, ptr);
			if (ptrp == NULL) {
				continue;
			}

			/* make sure we don't erroneously match on substrs */
			c = *(ptrp + strlen(ptr));
			if ((c != '\0') && (!isspace(c))) {
				continue;
			}

			/* got a match, wipe out the existing entry */
			memset(cbufp, 0, len);
			changed = TRUE;
			if (do_delete) {
				continue;
			}
			if (plen <= len) {
				/* space to replace the line */
				strcpy(cbufp, (char *)node->data);
				replaced = TRUE;
			}
		}

		/* add to addArr, indicate we need to write */
		if (!do_delete && !replaced) {
			changed = TRUE;
			/* add it in after finished looping */
			addArr = realloc(addArr, (++addSz * sizeof (char *)));
			if (addArr == NULL) {
				samerrno = SE_NO_MEM;
				snprintf(samerrmsg, MAX_MSG_LEN,
				    GetCustMsg(samerrno));
				st = -1;
				goto done;
			}
			addArr[addSz - 1] = (char *)node->data;
		}
	}

	if (!changed) {
		goto done;
	}

	/* backup existing crontab */
	st = backup_cfg(CRONTABFILE);
	if (st != 0) {
		goto done;
	}

	/* open a new file for the results */
	st = mk_wc_path(CRONTABFILE, tmpfilnam, sizeof (tmpfilnam));
	if (st != 0) {
		goto done;
	}

	fd = open(tmpfilnam, O_RDWR|O_CREAT|O_TRUNC, 0644);
	if (fd != -1) {
		fp = fdopen(fd, "w");
	}

	if (fp == NULL) {
		samerrno = SE_FILE_CREATE_OPEN_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
		    tmpfilnam);
		if (fd != -1) {
			close(fd);
		}
		st = -1;
		goto done;
	}

	/* Write out the modified buffer */
	for (i = 0; i < tabsize; i++) {
		cbufp = &cbuf[i];

		if (*cbufp == '\0') {
			continue;
		}

		fprintf(fp, "%s\n", cbufp);
		len = strlen(cbufp);
		i += len;
	}

	/* Write out any additional entries */
	for (i = 0; i < addSz; i++) {
		if (addArr[i] == NULL) {
			/* should never happen */
			continue;
		}
		fprintf(fp, "%s\n", addArr[i]);
	}

done:
	if (addArr != NULL) {
		free(addArr);
	}

	if (cbuf != NULL) {
		free(cbuf);
	}

	if (fp != NULL) {
		fflush(fp);
		fclose(fp);
	}

	/*
	 * update crontab only if everything was successful and the
	 * file has actually been modified.
	 */
	if (changed && (st == 0)) {
		snprintf(cmdbuf, sizeof (cmdbuf), "/usr/bin/crontab %s",
		    tmpfilnam);

		pid = exec_get_output(cmdbuf, NULL, NULL);
		if (pid != -1) {
			st = waitpid(pid, &i, 0);
		}
		if ((pid < 0) || (st == -1) ||
		    ((!WIFEXITED(i) || (WEXITSTATUS(i) != 0)))) {
			samerrno = SE_FORK_EXEC_FAILED;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(samerrno), cmdbuf);
			st = -1;
		} else {
			st = 0;
		}
	}
	unlink(tmpfilnam);

	return (st);
}

/*
 *  generate_crontab_entry
 *
 *  Function that takes a start date and an interval and creates the
 *  required crontab entry.
 *
 *  start & period are of the forms specified in task_schedule.h
 *
 */
static int
generate_crontab_entry(
char *start,
char *period,
char *cmdline,
char **croncmd)
{
	int		st;
	uint32_t	interval;
	uint32_t	minutes = 0;
	uint32_t	hours = 0;
	uint32_t	days = 0;
	uint32_t	weeks = 0;
	uint32_t	months = 0;
	uint32_t	years = 0;
	char		unit;
	struct tm	startdate;
	boolean_t	done = FALSE;
	char		buf[2048];
	char		buf2[256];
	int		i;
	int		j;
	uint32_t	k;
	size_t		len;
	char		*bufp;

	if (ISNULL(start, period, cmdline, croncmd)) {
		return (-1);		/* samerr already set */
	}

	/* make sure it's a valid start date */
	st = translate_date(start, &startdate);
	if (st != 0) {
		return (-1);		/* samerr already set */
	}

	/* parse out the period */
	st = translate_period(period, EXTENDED_PERIOD_UNITS, &interval, &unit);
	if (st != 0) {
		return (-1);		/* samerr already set */
	}

	/*
	 * Rationalize the unit we got.  Hours can also be specified as
	 * 360s, days as 24h, etc.
	 */
	while ((!done) && (st == 0)) {
		switch (unit) {
			case 's':			/* seconds */
				if (interval < 60) {
					/* less than 1 minute is invalid */
					st = -1;
				}
				interval = interval / 60;
				unit = 'm';
				break;
			case 'm':			/* minutes */
				if (interval >= 60) {
					minutes = interval % 60;
					interval = interval / 60;
					unit = 'h';
				} else {
					minutes = interval;
					done = TRUE;
				}
				if ((minutes != 0) && ((60 % minutes) != 0)) {
					st = -1;
				}
				break;
			case 'h':			/* hours */
				if (interval >= 24) {
					hours = interval % 24;
					interval = interval / 24;
					unit = 'd';
				} else {
					hours = interval;
					done = TRUE;
				}
				if ((hours != 0) && ((24 % hours) != 0)) {
					st = -1;
				}
				break;
			case 'd':			/* days */
				if (interval >= 7) {
					days = interval % 7;
					interval = interval / 7;
					unit = 'w';
				} else {
					days = interval;
					done = TRUE;
				}
				if (days > 1) {
					/* can't do random numbers of days */
					st = -1;
				}
				break;
			case 'w':			/* weeks */
				if (interval >= 52) {
					weeks = interval % 52;
					interval = interval / 52;
					unit = 'y';
				} else {
					weeks = interval;
					done = TRUE;
				}
				if (weeks > 1) {
					/* can't handle random weeks either */
					st = -1;
				}
				break;
			case 'M':			/* months */
				if (interval >= 12) {
					months = interval % 12;
					interval = interval / 12;
					unit = 'y';
				} else {
					months = interval;
					done = TRUE;
				}
				if ((months != 0) && ((12 % months) != 0)) {
					st = -1;
				}

				break;
			case 'y':			/* years */
				years = interval;
				if (years > 1) {
					/* more than 1 year is invalid */
					st = -1;
				}
				done = TRUE;
				break;
			case 'D':			/* day of month */
				if (0 < interval <= 31) {
					st = -1;
				}
				done = TRUE;
				break;
			case 'W':			/* day of week */
				if (7 > interval >= 0) {
					st = -1;
				}
				done = TRUE;
				break;
			default:
				/* fail on unrecognized intervals */
				st = -1;
				break;
		}
	}

	/* final validation */
	if (((years > 0) && (minutes || hours || days || weeks || months)) ||
	    ((months) && (minutes || hours || days || weeks)) ||
	    ((weeks) && (minutes || hours || days)) ||
	    ((days) && (minutes || hours))) {
		st = -1;
	}

	/* validation complete at last */
	if (st != 0) {
		samerrno = SE_INVALID_PERIOD;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno), period);
		return (-1);
	}

	/*
	 *  The first 5 fields in a crontab command are:
	 *  	minute (0-59),
	 *  	hour (0-23),
	 *  	day of the month (1-31),
	 *  	month of the year (1-12),
	 *  	day of the week (0-6 with 0 = Sunday).
	 */

	if (unit == 'D') {
		snprintf(buf, sizeof (buf), "%u %u %u * * ",
		    startdate.tm_min, startdate.tm_hour, interval);
	} else if (unit == 'W') {
		snprintf(buf, sizeof (buf), "%u %u * %u * ",
		    startdate.tm_min, startdate.tm_hour, interval);
	} else if (years) {
		snprintf(buf, sizeof (buf), "%u %u %u %u * ",
		    startdate.tm_min, startdate.tm_hour,
		    startdate.tm_mday, startdate.tm_mon);
	} else if (months) {
		bufp = buf2;
		len = sizeof (buf2);

		for (i = 0; i < (12 / months); i++) {
			k = startdate.tm_mon + (i * months);
			if (k >= 12) {
				k -= 12;
			}
			j = snprintf(bufp, len, "%u,", k);

			bufp += j;
			len -= j;
		}
		*(bufp - 1)  = ' ';	/* overwrite last comma with a space */

		snprintf(buf, sizeof (buf), "%u %u %u %s * ",
		    startdate.tm_min, startdate.tm_hour,
		    startdate.tm_mday, buf2);
	} else if (weeks) {
		snprintf(buf, sizeof (buf), "%u %u * * %u ",
		    startdate.tm_min, startdate.tm_hour,
		    startdate.tm_wday);
	} else if (days) {
		snprintf(buf, sizeof (buf), "%u %u * * * ",
		    startdate.tm_min, startdate.tm_hour);
	} else {
		buf[0] = '\0';

		if (minutes == 1) {
			strlcpy(buf2, "* ", sizeof (buf2));
		} else if (minutes) {
			bufp = buf2;
			len = sizeof (buf2);

			for (i = 0; i < (60 / minutes); i++) {
				k = startdate.tm_min + (i * minutes);
				if (k >= 60) {
					k -= 60;
				}
				j = snprintf(bufp, len, "%u,", k);
				bufp += j;
				len -= j;
			}
			*(bufp - 1) = ' ';
		} else {
			snprintf(buf2, sizeof (buf2), "%u ", startdate.tm_min);
		}

		strlcat(buf, buf2, sizeof (buf));

		if (hours > 1) {
			bufp = buf2;
			len = sizeof (buf2);

			for (i = 0; i < (24 / hours); i++) {
				k = startdate.tm_hour + (i * hours);
				if (k >= 24) {
					k -= 24;
				}
				j = snprintf(bufp, len, "%u,", k);
				bufp += j;
				len -= j;
			}
			*(bufp - 1) = ' ';
		} else {
			/*
			 * if we got here, run every hour.  hour is either
			 * unset or set to 1.
			 */
			strlcpy(buf2, "* ", sizeof (buf2));
		}

		strlcat(buf, buf2, sizeof (buf));

		strlcat(buf, "* * * ", sizeof (buf));
	}

	strlcat(buf, cmdline, sizeof (buf));

	*croncmd = strdup(buf);
	if (*croncmd == NULL) {
		samerrno = SE_NO_MEM;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		return (-1);
	}

	return (0);
}
