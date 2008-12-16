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
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <libgen.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>

#include "db.h"
#include "mgmt/fsmdb.h"
#include "mgmt/file_metrics.h"
#include "pub/devstat.h"

#define	HOUR	60 * 60
#define	DAY	24 * HOUR

/* Common argument structure for all metrics reports */
typedef struct {
	uint32_t	snapdate;
	boolean_t	done;
} fmRptArg_t;

/*
 * Struct to hold report results - filled in by the report functions
 * Note the result size is required to store the results in a database
 * for quick retrieval/report processing.
 */
typedef struct {
	DB			*rptDB;
	uint32_t		rptSize;
	void			*result;
} fmFuncRes_t;

/*
 *  All file metrics functions conform to this prototype
 */
typedef int (*file_metric_func_t)(
	filinfo_t	*filinfo,
	filvar_t	*filvar,
	fmRptArg_t	*arg,
	int		*media,
	fmFuncRes_t	*results
);

/*  Used to construct top 10 lists */
typedef struct agg_list_s {
	agg_size_t		*agg;
	struct agg_list_s	*next;
	struct agg_list_s	*prev;
} agg_list_t;

/*  Function Prototypes */
static int
do_file_age(filinfo_t *filinfo, filvar_t *filvar, fmRptArg_t *arg,
	int *media, fmFuncRes_t *results);

static int
do_file_useful(filinfo_t *filinfo, filvar_t *filvar, fmRptArg_t *arg,
	int *media, fmFuncRes_t *results);

static int
do_top10_users(filinfo_t *filinfo, filvar_t *filvar, fmRptArg_t *arg,
	int *media, fmFuncRes_t *results);

static int
do_top10_groups(filinfo_t *filinfo, filvar_t *filvar, fmRptArg_t *arg,
	int *media, fmFuncRes_t *results);

static int
do_storage_tiers(filinfo_t *filinfo, filvar_t *filvar, fmRptArg_t *arg,
	int *media, fmFuncRes_t *results);

static int
do_top10(filinfo_t *filinfo, filvar_t *filvar, boolean_t done,
	fm_rptno_t which, fmFuncRes_t *results);

/* Globals */

file_metric_func_t	rptFuncArr[NUM_FM_RPTS] = {
	do_file_age,
	do_file_useful,
	do_top10_users,
	do_top10_groups,
	do_storage_tiers,
};

static char *xmlAgeStrs[6] = {
	"0",
	"0-30",
	"30-90",
	"90-180",
	"180-365",
	">365"
};

char *xmlRptName[] = {
	"FileAge",
	"FunctionalLife",
	"Top10Users",
	"Top10Groups",
	"StorageTierUse"
};

/*
 *  gather_snap_metrics()
 *
 *  Metrics are generated as data is being input.  While this may slow
 *  down the import slightly, it's faster than processing the same data
 *  twice.
 */
int
gather_snap_metrics(
	fs_db_t 	*fsdb,			/* in */
	fsmsnap_t	*snapinfo, 		/* in */
	filinfo_t	*finfo,			/* in */
	filvar_t	*filvar,		/* in */
	void		**rptArg,		/* in/out, opaque */
	int		*media,			/* in */
	void		**rptRes		/* in/out, opaque */
)
{
	int		st;
	int		i;
	fmRptArg_t	*rptArgp;
	fmFuncRes_t	*rptResp;

	if ((fsdb == NULL) || (snapinfo == NULL) || (finfo == NULL) ||
	    (filvar == NULL) || (rptArg == NULL) || (rptRes == NULL)) {
		return (-1);
	}

	if (*rptArg == NULL) {
		rptArgp = calloc(1, sizeof (fmRptArg_t));
		if (rptArgp == NULL) {
			return (ENOMEM);
		}
		*rptArg = rptArgp;
		rptArgp->snapdate = snapinfo->snapdate;
		rptArgp->done = FALSE;

	} else {
		rptArgp = *rptArg;
	}

	if (*rptRes == NULL) {
		rptResp = calloc(NUM_FM_RPTS, sizeof (fmFuncRes_t));
		if (rptResp == NULL) {
			free(rptArgp);
			return (ENOMEM);
		}
		*rptRes = rptResp;
	} else {
		rptResp = *rptRes;
	}

	for (i = 0; i < NUM_FM_RPTS; i++) {
		st = rptFuncArr[i](finfo, filvar, rptArgp, media,
		    &(rptResp[i]));
		if (st != 0) {
			break;
		}
	}

	if (st != 0) {
		free_metrics_results(rptArg, rptRes);
	}

	return (st);
}

/*
 *  finish_snap_metrics()
 *
 *  Post-processes any data, if needed, and stores in the metrics database.
 */
int
finish_snap_metrics(
	fs_db_t		*fsdb,			/* in */
	fsmsnap_t	*snapinfo, 		/* in */
	void		**rptArg,		/* in/out, opaque */
	void		**rptRes		/* in/out, opaque */
)
{
	int		st;
	DB		*dbp;
	int		i;
	DBT		key;
	DBT		data;
	fmrpt_key_t	rptKey;
	DB_TXN		*txn;
	fmRptArg_t	*rptArgp;
	fmFuncRes_t	*rptResp;

	if ((fsdb == NULL) || (snapinfo == NULL) || (rptArg == NULL) ||
	    (rptRes == NULL)) {
		return (-1);
	}

	rptArgp = *rptArg;
	rptResp = *rptRes;

	if ((rptResp == NULL) || (rptArgp == NULL)) {
		return (-1);
	}

	/*  If the specific reports need to post-process results, do it now */
	rptArgp->done = TRUE;

	for (i = 0; i < NUM_FM_RPTS; i++) {
		st = rptFuncArr[i](NULL, NULL, rptArgp, NULL, &(rptResp[i]));
		if (st != 0) {
			goto done;
		}
	}

	dbp = fsdb->metricsDB;

	for (i = 0; i < NUM_FM_RPTS; i++) {
		if (rptResp[i].result == NULL) {
			/* no report available */
			continue;
		}

		memset(&key, 0, sizeof (DBT));
		memset(&data, 0, sizeof (DBT));

		rptKey.rptType = i;
		rptKey.snapid = snapinfo->snapid;

		key.data = &rptKey;
		key.size = sizeof (rptKey);

		data.data = rptResp[i].result;
		data.size = rptResp[i].rptSize;

		st = dbEnv->txn_begin(dbEnv, NULL, &txn, 0);
		if (st != 0) {
			break;
		}

		CKPUT(dbp->put(dbp, txn, &key, &data, 0));

		if (st != 0) {
			txn->abort(txn);
			break;
		}

		st = txn->commit(txn, 0);
		if (st != 0) {
			break;
		}
	}

done:
	free_metrics_results(rptArg, rptRes);

	return (st);
}

/*
 *  free_metrics_results()
 *
 *  Post-gathering cleanup
 */
void
free_metrics_results(
	void		**rptArg,		/* in/out, opaque */
	void		**rptRes		/* in/out, opaque */
)
{
	fmFuncRes_t	*rptResp;
	int		i;

	if ((rptArg != NULL) && (*rptArg != NULL)) {
		free(*rptArg);
		*rptArg = NULL;
	}

	if (rptRes == NULL) {
		return;
	}

	rptResp = *rptRes;
	if (rptResp == NULL) {
		return;
	}

	for (i = 0; i < NUM_FM_RPTS; i++) {
		/* some reports create in-memory databases, close them */
		if (rptResp[i].rptDB != NULL) {
			(rptResp[i].rptDB)->close(rptResp[i].rptDB, 0);
		}
		if (rptResp[i].result != NULL) {
			free(rptResp[i].result);
		}

	}

	free(rptResp);
	*rptRes = NULL;
}

static int
do_file_age(
	filinfo_t	*filinfo,
	filvar_t	*filvar,
	fmRptArg_t	*arg,
	int		*media,	/* ARGSUSED */
	fmFuncRes_t	*results)
{
	file_age_t	*ageres = NULL;
	int		i;
	uint32_t	age;
	boolean_t	match = FALSE;
	int32_t		start;
	int32_t		end;

	if (arg == NULL) {
		return (-1);
	}

	if (arg->done == TRUE) {
		return (0);
	}

	if ((results == NULL) || (filinfo == NULL) || (filvar == NULL)) {
		return (-1);
	}

	ageres = results->result;

	if (ageres == NULL) {
		/*  There are 5 ranges in the file age report */
		results->rptSize = 5 * (sizeof (file_age_t));
		ageres = calloc(1, results->rptSize);

		if (ageres == NULL) {
			return (-1);
		}
		/*  set up the ranges */
		ageres[0].start = 0;
		ageres[0].end   = ageres[1].start = 30;
		ageres[1].end   = ageres[2].start = 90;
		ageres[2].end   = ageres[3].start = 180;
		ageres[3].end   = ageres[4].start = 365;
		ageres[4].end   = -1;

		results->result = ageres;
	}

	age = arg->snapdate - filvar->mtime;

	for (i = 0; i < 5; i++) {
		start = (ageres[i].start == -1) ? -1 : ageres[i].start * DAY;
		end = (ageres[i].end == -1) ? -1 : ageres[i].end * DAY;

		if ((start == end) || (start == -1)) {
			if (age == end) {
				match = TRUE;
			}
		} else if (end == -1) {
			if (age >= start) {
				match = TRUE;
			}
		} else {
			if ((age >= start) && (age < end)) {
				match = TRUE;
			}
		}

		if (match == TRUE) {
			ageres[i].count++;
			ageres[i].total_size += filinfo->size;
			ageres[i].total_osize += filinfo->osize;
			break;
		}
	}

	return (0);
}

static int
do_file_useful(
	filinfo_t	*filinfo,
	filvar_t	*filvar,
	fmRptArg_t	*arg,
	int		*media,	/* ARGSUSED */
	fmFuncRes_t	*results)
{
	file_age_t	*ageres = NULL;
	int		i;
	uint32_t	age;
	boolean_t	match = FALSE;
	int32_t		start;
	int32_t		end;

	if (arg == NULL) {
		return (-1);
	}

	if (arg->done == TRUE) {
		return (0);
	}

	if ((filinfo == NULL) || (filvar == NULL) || (results == NULL)) {
		return (-1);
	}

	ageres = results->result;

	if (ageres == NULL) {
		/*  There are 6 ranges in the file useful report */
		results->rptSize = 6 * (sizeof (file_age_t));

		ageres = calloc(1, results->rptSize);

		if (ageres == NULL) {
			return (-1);
		}
		/*  set up the ranges */
		ageres[0].start = ageres[0].end   = ageres[1].start = 0;
		ageres[1].end   = ageres[2].start = 30;
		ageres[2].end   = ageres[3].start = 90;
		ageres[3].end   = ageres[4].start = 180;
		ageres[4].end   = ageres[5].start = 365;
		ageres[5].end   = -1;

		results->result = ageres;
	}

	age = arg->snapdate - filvar->atime;

	for (i = 0; i < 5; i++) {
		start = (ageres[i].start == -1) ? -1 : ageres[i].start * DAY;
		end = (ageres[i].end == -1) ? -1 : ageres[i].end * DAY;

		if ((start == end) || (start == -1)) {
			if (age == end) {
				match = TRUE;
			}
		} else if (end == -1) {
			if (age >= start) {
				match = TRUE;
			}
		} else {
			if ((age >= start) && (age < end)) {
				match = TRUE;
			}
		}

		if (match == TRUE) {
			ageres[i].count++;
			ageres[i].total_size += filinfo->size;
			ageres[i].total_osize += filinfo->osize;
			break;
		}
	}

	return (0);
}

static int
do_storage_tiers(filinfo_t *filinfo, filvar_t *filvar, fmRptArg_t *arg,
	int *media, fmFuncRes_t *results)
{
	tier_usage_t		*tiers = NULL;
	int			i;

	if (arg == NULL) {
		return (-1);
	}

	if (arg->done == TRUE) {
		return (0);
	}

	if ((filinfo == NULL) || (filvar == NULL) || (results == NULL)) {
		return (-1);
	}

	tiers = results->result;

	if (tiers == NULL) {
		results->rptSize = 4 * TIER_SIZE;

		tiers = calloc(1, results->rptSize);

		if (tiers == NULL) {
			return (-1);
		}

		results->result = tiers;

		tiers[0].mtype = 0;		/* disk cache */
		tiers[1].mtype = DT_DISK;	/* disk archive */
		tiers[2].mtype = DT_TAPE;	/* tape & optical archive */
		tiers[3].mtype = DT_STK5800;	/* honeycomb archive */
	}

	tiers[0].used += filinfo->osize;

	if (media == NULL) {
		/* nothing else to do */
		return (0);
	}

	/* process all the archive copies */
	for (i = 0; i < 4; i++) {
		if (media[i] == 0) {
			continue;
		}

		/*
		 * Honeycomb masks out as DT_DISK with DT_CLASS_MASK so
		 * call it out separately here.
		 */
		if (media[i] == DT_STK5800) {
			tiers[3].used += filinfo->size;
			continue;
		}

		switch (media[i] & DT_CLASS_MASK) {
			case DT_DISK:
				tiers[1].used += filinfo->size;
				break;
			case DT_TAPE:
			default:
				/*
				 * for now, if it's not disk, or Honeycomb,
				 * it's tape.
				 */
				tiers[2].used += filinfo->size;
				break;
		}
	}
	return (0);
}

static int
do_top10_users(
	filinfo_t	*filinfo,
	filvar_t	*filvar,
	fmRptArg_t	*arg,
	int		*media,	/* ARGSUSED */
	fmFuncRes_t	*results)
{
	int		st;

	if ((arg == NULL) || (results == NULL)) {
		return (-1);
	}

	st = do_top10(filinfo, filvar, arg->done, TOP_USERS, results);

	return (st);
}

static int
do_top10_groups(
	filinfo_t	*filinfo,
	filvar_t	*filvar,
	fmRptArg_t	*arg,
	int		*media,	/* ARGSUSED */
	fmFuncRes_t	*results)
{
	int		st;

	if ((arg == NULL) || (results == NULL)) {
		return (-1);
	}

	st = do_top10(filinfo, filvar, arg->done, TOP_GROUPS,
	    results);

	return (st);
}

/*
 *  worker function shared between top10Users and top10Groups
 *
 *  Creates in-memory only database to store temporary results
 *  as long as done == FALSE.  As soon as done is set to TRUE,
 *  generate the top10 lists for the database.
 *
 *  The results parameter is used to pass the database pointer around
 *  while calculating, then changes to an agg_type_t when complete.
 *
 */
static int
do_top10(filinfo_t *filinfo, filvar_t *filvar, boolean_t done,	/* ARGSUSED */
	fm_rptno_t which, fmFuncRes_t *results)
{
	int		st;
	DB		*dbp;
	DBC		*curs;
	DBT		key;
	DBT		data;
	DB_TXN		*txn = NULL;
	agg_size_t	agg;
	agg_list_t	*agglist = NULL;
	agg_size_t	*aggp;
	int		i;
	agg_size_t	*aggreport;

	dbp = results->rptDB;

	if (done == FALSE) {
		if ((filinfo == NULL) || (filvar == NULL)) {
			return (-1);
		}

		if (dbp == NULL) {
			st = db_create(&dbp, dbEnv, 0);
			if (st != 0) {
				return (st);
			}
			/* no duplicates, sort on id */
			st = dbp->set_bt_compare(dbp, bt_compare_uint64);
			if (st != 0) {
				dbp->remove(dbp, NULL, NULL, 0);
				return (st);
			}

			st = dbp->open(dbp, NULL, NULL, NULL, DB_BTREE,
			    db_fl, 0644);
			if (st != 0) {
				dbp->remove(dbp, NULL, NULL, 0);
				return (st);
			}

			results->rptDB = dbp;
		}

		memset(&agg, 0, sizeof (agg_size_t));

		if (which == TOP_USERS) {
			agg.id = filinfo->owner;
		} else {
			agg.id = filinfo->group;
		}

		/* fetch id info if it exists */
		memset(&key, 0, sizeof (DBT));
		memset(&data, 0, sizeof (DBT));

		key.data = &agg.id;
		key.size = sizeof (uint64_t);

		data.data = &agg;
		data.size = data.ulen = sizeof (agg_size_t);
		data.flags = DB_DBT_USERMEM;

		dbEnv->txn_begin(dbEnv, NULL, &txn, 0);
		st = dbp->get(dbp, txn, &key, &data, 0);

		if ((st != 0) && (st != DB_NOTFOUND)) {
			txn->abort(txn);
			return (st);
		}

		agg.count++;
		agg.total_size += filinfo->size;
		agg.total_osize += filinfo->osize;

		st = dbp->put(dbp, txn, &key, &data, 0);
		if (st == 0) {
			st = txn->commit(txn, 0);
		} else {
			txn->abort(txn);
		}
	}

	/* final processing */
	if (done == TRUE) {
		uint64_t	aggid = 0;
		int 		numids = 0;
		agg_list_t	*aggent;
		agg_list_t	*ptr;
		agg_list_t	*last;

		if (dbp == NULL) {
			return (-1);
		}

		dbEnv->txn_begin(dbEnv, NULL, &txn, 0);

		st = dbp->cursor(dbp, txn, &curs, 0);
		if (st != 0) {
			/* blast db? */
			txn->abort(txn);
			return (st);
		}

		memset(&key, 0, sizeof (DBT));
		memset(&data, 0, sizeof (DBT));

		key.data = &aggid;
		key.size = key.ulen = sizeof (uint64_t);
		key.flags = DB_DBT_USERMEM;

		data.flags = DB_DBT_MALLOC;

		/* fetch first entry */
		while ((st = curs->c_get(curs, &key, &data, DB_NEXT)) == 0) {
			aggp = (agg_size_t *)data.data;
			last = NULL;

			if (agglist == NULL) {
				aggent = malloc(sizeof (agg_list_t));
				aggent->agg = aggp;

				aggent->prev = NULL;
				aggent->next = NULL;
				agglist = aggent;

				numids = 1;
				continue;
			}

			ptr = agglist;

			while (ptr != NULL) {
				last = ptr;
				if (aggp->total_size >
				    ptr->agg->total_size) {
					break;
				}
				ptr = ptr->next;
			}

			if ((ptr != NULL) || (numids < 10)) {
				aggent = malloc(sizeof (agg_list_t));
				if (aggent == NULL) {
					st = ENOMEM;
					goto done;
				}
				aggent->agg = aggp;
				aggent->next = ptr;
				if (ptr != NULL) {
					aggent->prev = ptr->prev;
					if (ptr->prev != NULL) {
						ptr->prev->next = aggent;
					}
					ptr->prev = aggent;
				} else {
					aggent->prev = last;
					if (last != NULL) {
						last->next = aggent;
					}
				}

				if (aggent->prev == NULL) {
					agglist = aggent;
				}
				numids++;
				aggp = NULL;
			}

			if (numids > 10) {
				ptr = agglist;

				while (ptr->next != NULL) {
					ptr = ptr->next;
				}

				free(ptr->agg);
				ptr->prev->next = NULL;
				free(ptr);
				numids--;
			}

			if (aggp != NULL) {
				free(aggp);
			}
		}

		/* done with the in-mem database now */
		(void) curs->c_close(curs);
		(void) txn->commit(txn, 0);

		(void) dbp->close(dbp, 0);
		results->rptDB = NULL;

		if ((st != 0) && (st != DB_NOTFOUND)) {
			/* something bad happened */
			goto done;
		}

		/* got the top 10, allocate the final structure */
		results->rptSize = 10 * (sizeof (agg_size_t));
		aggreport = calloc(1, results->rptSize);
		results->result = (void *)aggreport;

		if (aggreport == NULL) {
			st = ENOMEM;
			goto done;
		}

		ptr = agglist;

		for (i = 0; ptr != NULL; i++) {
			aggp = ptr->agg;
			memcpy(&aggreport[i], aggp, sizeof (agg_size_t));
			ptr = ptr->next;
		}

		st = 0;
done:
		while (agglist != NULL) {
			ptr = agglist;
			agglist = ptr->next;

			free(ptr->agg);
			free(ptr);
		}

	}

	return (st);
}

/*
 *  Prints out the XML for a filesystem's metrics
 */
int
generate_xml_fmrpt(fs_entry_t *fsent, FILE *outf, fm_rptno_t which,
	time_t start, time_t end)
{
	int		st;
	DB		*dbp;
	DB		*sdbp;
	DBT		key;
	DBT		data;
	DBT		skey;
	DBT		sdata;
	DBC		*curs;
	DB_TXN		*txn = NULL;
	DB_TXN		*stxn = NULL;
	fmrpt_key_t	fmkey;
	fsmsnap_t	*snapinfo;
	int		i;
	file_age_t	*ages;
	tier_usage_t	*tiers;
	agg_size_t	*top10;
	struct passwd	*pwdp;
	struct group	*groupp;
	char		*namep;
	fs_db_t		*fsdb;
	int		t;
	uint64_t	media[4] = {0, 0, 0, 0};

	/*
	 * for now, this is done on a file system basis.  Modify to
	 * accept NULL for a fsname which means loop for ALL file systems
	 *
	 * It may make more sense to add a higher-level function that
	 * is called by the clients that loops through and calls this one
	 * for each file system.  If that happens, move the MetricReport
	 * part of the XML up to that function, (and maybe even the
	 * filesystem name part) and let this just print out the report
	 * specific information.
	 */
	if ((fsent == NULL) || (outf == NULL) || (fsent->fsname == NULL)) {
		return (-1);
	}

	fsdb = fsent->fsdb;

	dbp = fsdb->metricsDB;
	sdbp = fsdb->snapDB;

	if (dbp == NULL) {
		return (-1);
	}

	memset(&key, 0, sizeof (DBT));
	memset(&data, 0, sizeof (DBT));

	/*
	 * Results are not in sorted-date order.  It is the client's
	 * responsibility to sort if required.
	 */
	fmkey.rptType = which;
	fmkey.snapid = 0;

	key.data = &fmkey;
	key.size = key.ulen = sizeof (fmkey);
	key.flags = DB_DBT_USERMEM;

	data.data = NULL;
	data.flags = DB_DBT_MALLOC;

	dbEnv->txn_begin(dbEnv, NULL, &txn, 0);
	st = dbp->cursor(dbp, txn, &curs, DB_READ_COMMITTED);

	if (st != 0) {
		txn->abort(txn);
		return (-1);
	}

	/* write out the header */
	(void) fprintf(outf, "<MetricReport name=\"%s\" created=\"%lu\">\n",
	    xmlRptName[which], time(NULL));
	(void) fprintf(outf, "<filesystem name=\"%s\">\n", fsent->fsname);

	st = curs->c_get(curs, &key, &data, DB_SET_RANGE);

	do {
		if ((st != 0) || (fmkey.rptType != which)) {
			break;
		}

		memset(&skey, 0, sizeof (DBT));
		memset(&sdata, 0, sizeof (DBT));

		skey.data = &(fmkey.snapid);
		skey.size = sizeof (uint32_t);

		sdata.flags = DB_DBT_MALLOC;

		dbEnv->txn_begin(dbEnv, txn, &stxn, 0);
		st = sdbp->get(sdbp, stxn, &skey, &sdata, 0);
		if (st != 0) {
			/* bad snapid? */
			stxn->abort(stxn);
			continue;
		}
		stxn->commit(stxn, 0);
		snapinfo = sdata.data;

		/*
		 * Check that this snapshot has metrics and is within range.
		 * Only return snapshots between start and end.  If both
		 * are 0, return all snapshots.  Otherwise, if start
		 * is 0, return all until date specified as end.
		 * If end is 0, return all starting at start until
		 * no more snapshots.
		 */
		if (!(snapinfo->flags & METRICS_AVAIL)) {
			free(snapinfo);
			continue;
		}

		if (((start != 0) && (snapinfo->snapdate < start)) ||
		    ((end != 0) && (snapinfo->snapdate > end))) {
			free(snapinfo);
			continue;
		}

		(void) fprintf(outf,
		    "<date val=\"%u\" snapdate=\"%s\" snapname=\"%s\">\n",
		    snapinfo->snapdate,
		    ctime((time_t *)&(snapinfo->snapdate)),
		    snapinfo->snapname);

		free(snapinfo);

		switch (which) {
			case FILE_AGE:
				ages = data.data;

				for (i = 0; i < 5; i++) {
					(void) fprintf(outf,
					    "<range val=\"%s\">"
					    "<numfiles>%llu</numfiles>"
					    "<numBytes>%llu</numBytes>"
					    "<numOnlineBytes>%llu"
					    "</numOnlineBytes>"
					    "</range>\n",
					    xmlAgeStrs[i+1],
					    ages[i].count,
					    ages[i].total_size,
					    ages[i].total_osize);
				}
				break;
			case FILE_USEFUL:
				ages = data.data;

				for (i = 0; i < 6; i++) {
					(void) fprintf(outf,
					    "<range val=\"%s\">"
					    "<numfiles>%llu</numfiles>"
					    "<numBytes>%llu</numBytes>"
					    "<numOnlineBytes>%llu"
					    "</numOnlineBytes>"
					    "</range>\n",
					    xmlAgeStrs[i],
					    ages[i].count,
					    ages[i].total_size,
					    ages[i].total_osize);
				}
				break;
			case TOP_USERS:
			case TOP_GROUPS:
				top10 = data.data;

				for (i = 0; i < 10; i++) {
					/* not all values necessarily set */
					if (top10[i].count == 0) {
						break;
					}

					namep = NULL;
					if (which == TOP_USERS) {
						pwdp = getpwuid(top10[i].id);
						if (pwdp != NULL) {
							namep = pwdp->pw_name;
						}
					} else {
						groupp = getgrgid(top10[i].id);
						if (groupp != NULL) {
							namep = groupp->gr_name;
						}
					}

					(void) fprintf(outf,
					    "<id val=\"%llu\">"
					    "<name>%s</name>"
					    "<numFiles>%llu</numFiles>"
					    "<numBytes>%llu</numBytes>"
					    "<numOnlineBytes>%llu"
					    "</numOnlineBytes>"
					    "</id>\n",
					    top10[i].id,
					    (namep == NULL) ? "unknown" : namep,
					    top10[i].count,
					    top10[i].total_size,
					    top10[i].total_osize);
				}
				break;
			case TIER_USE:
			/*
			 * rationalize the results, in case something went
			 * wrong during gathering, and also to account for
			 * no honeycomb results in older metrics storage
			 */
				tiers = data.data;

				memset(&media, 0, sizeof (media));

				for (t = 0; t < (data.size / TIER_SIZE); t++) {
					if ((t > 0) && (tiers[t].mtype == 0)) {
						continue;
					}
					media[t] = tiers[t].used;
				}

				(void) fprintf(outf,
				    "<diskCache>%llu</diskCache>"
				    "<diskArchive>%llu</diskArchive>"
				    "<tapeArchive>%llu</tapeArchive>"
				    "<honeycombArchive>%llu"
				    "</honeycombArchive>\n",
				    media[0],
				    media[1],
				    media[2],
				    media[3]);

				break;
			default:
				/* NOTREACHED */
				break;
		}

		free(data.data);
		data.data = NULL;

		(void) fprintf(outf, "</date>\n");

	} while ((st = curs->c_get(curs, &key, &data, DB_NEXT)) == 0);

	curs->c_close(curs);
	txn->commit(txn, 0);

	/* end of db is not an error */
	if (st == DB_NOTFOUND) {
		st = 0;
	}

	if (data.data != NULL) {
		free(data.data);
	}

	(void) fprintf(outf, "</filesystem>\n");
	(void) fprintf(outf, "</MetricReport>\n");
	(void) fflush(outf);

	return (st);
}
