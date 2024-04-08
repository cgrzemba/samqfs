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

#pragma ident	"$Revision: 1.10 $"

#include "samcli.h"

/* Parsed parameter pointers and values */

sqm_lst_t	 *pathlist = NULL;
sqm_lst_t	 *csdlist = NULL;
sqm_lst_t	 *destlist = NULL;
sqm_lst_t	 *filstructlist = NULL;
sqm_lst_t	 *dmplist = NULL;
sqm_lst_t	 *restlist = NULL;
sqm_lst_t	 *cpylist = NULL;
sqm_lst_t	 *entrylist = NULL;
v_command_t	command = v_null;	/* Command requested */
int		no_action = 0;		/* Indicate test or failed command */

/* Forward pointers */
arg_t	  *cli_args[];

/* Macro to refer to an integer on a list */
#define	LISTINT(X) (*((int *)(X)))

/*
 * Utility routines to place strings and integers on a list
 */

node_t *
newnode(sqm_lst_t ** top)
{
	if (*top == NULL) {	/* Allocate list if necessary */
		*top = malloc(sizeof (struct sqm_lst));
		if (*top == NULL)
			return (NULL);
		(*top)->head = NULL;
		(*top)->tail = NULL;
		(*top)->length = 0;
	}
	return (malloc(sizeof (struct node)));
}

void
addtail(sqm_lst_t ** top, node_t *ptr)
{
	ptr->next = NULL;		/* This is end of list */
	if ((*top)->head == NULL) {
		(*top)->tail = ptr;	/* Empty list, this is only entry */
		(*top)->head = ptr;
	} else {
		(*top)->tail->next = ptr;
		/* Add this entry to end of list */
		(*top)->tail = ptr;
	}
	(*top)->length++;		/* Increment count of entries */
}

node_t *
newstr(sqm_lst_t ** top, int length)
{
	node_t	 *ptr;

	ptr = newnode(top);
	if (ptr == NULL)
		return (NULL);
	ptr->data = malloc(length);	/* Allocate string buffer */
	if (ptr->data == NULL)
		return (NULL);		/* leaks node pointer */
	addtail(top, ptr);
	return (ptr);
}

node_t *
newint(sqm_lst_t ** top, int value)
{
	node_t	 *ptr;

	ptr = newnode(top);
	if (ptr == NULL)
		return (NULL);
	ptr->data = malloc(sizeof (int));
	if (ptr->data == NULL)
		return (NULL);
	*(int *)(ptr->data) = value;

	addtail(top, ptr);
	return (ptr);
}



/*
 * cmdopt - Command switch option utility routine
 */
static void
cmdopt(v_command_t opt)
{
	if (command != v_null) {
		printf("Illegal duplicate commands; numbers %d and %d\n",
		    command, opt);
		no_action = 1;
	} else {
		command = opt;
	}
}

/*
 * parser routines. These will all be some code followed by an arg_t structure.
 */

/*
 * Nullify actions switch
 */
static int
nullify(char *ptr)
{
	printf("No action will be performed\n");
	no_action = 1;
	return (1);
}

arg_t
p_null = {
	nullify, "-x", "-no_action", NULL,
	"     Prevent utility from taking any action. Useful to\n"
	"     debug complex command strings\n\n"
};

/*
 * Command switches, all mutually exclusive
 */

static int
setcsd(char *ptr)
{
	cmdopt(v_setcsd);
	return (1);
}
static int
getcsd(char *ptr)
{
	cmdopt(v_getcsd);
	return (1);
}
static int
listdumps(char *ptr)
{
	cmdopt(v_listdumps);
	return (1);
}
static int
restore(char *ptr)
{
	cmdopt(v_restore);
	return (1);
}
static int
takedump(char *ptr)
{
	cmdopt(v_takedump);
	return (1);
}
static int
stagefiles(char *ptr)
{
	cmdopt(v_stagefiles);
	return (1);
}
static int
listdir(char *ptr)
{
	cmdopt(v_listdir);
	return (1);
}
static int
listver(char *ptr)
{
	cmdopt(v_listver);
	return (1);
}
static int
getverdetails(char *ptr)
{
	cmdopt(v_getverdetails);
	return (1);
}
static int
search(char *ptr)
{
	cmdopt(v_search);
	return (1);
}
static int
getfilestatus(char *ptr)
{
	cmdopt(v_getfilestatus);
	return (1);
}
static int
getfiledetails(char *ptr)
{
	cmdopt(v_getfiledetails);
	return (1);
}
static int
getdumpstatus(char *ptr)
{
	cmdopt(v_getdumpstatus);
	return (1);
}
static int
decompressdump(char *ptr)
{
	cmdopt(v_decompressdump);
	return (1);
}

arg_t
p_setcsd = {
	setcsd, "setcsd", NULL, NULL,
	"     setcsd. Set parameters for periodic metadata exports.\n\n"
};
arg_t
p_getcsd = {
	getcsd, "getcsd", NULL, NULL,
	"     getcsd. Retrieve parameters for periodic metadata export\n"
	"     for a particular filesystem.\n\n"
};
arg_t
p_listdumps = {
	listdumps, "listdumps", NULL, NULL,
	"     listdumps. Show the list of dumps available for reading.\n\n"
};
arg_t
p_restore = {
	restore, "restore", NULL, NULL,
	"     restore. Restore one or more files from dumps read in.\n\n"
};
arg_t
p_takedump = {
	takedump, "takedump", NULL, NULL,
	"     takedump. Cause a samfsdump to be taken\n\n"
};
arg_t
p_stagefiles = {
	stagefiles, "stagefiles", NULL, NULL,
	"     stagefiles. Request that one or more files be staged.\n"
};
arg_t
p_listdir = {
	listdir, "listdir", NULL, NULL,
	"     listdir. List the contents of a directory on disk.\n\n"
};
arg_t
p_listver = {
	listver, "listver", NULL, NULL,
	"     listver. List the contents of a directory in a dump.\n\n"
};

arg_t
p_getverdetails = {
	getverdetails, "getverdetails", NULL, NULL,
	"     getverdetails. Determine details of a file version.\n"
};


arg_t
p_search = {
	search, "search", NULL, NULL,
	"     search. List the contents of a dump,"
	" subject to restrictions.\n\n"
};
arg_t
p_getfilestatus = {
	getfilestatus, "getfilestatus", NULL, NULL,
	"     getfilestatus. Determine status of a file on disk.\n"
};

arg_t
p_getfiledetails = {
	getfiledetails, "getfiledetails", NULL, NULL,
	"     getfiledetails. Determine details of a file on disk.\n"
};

arg_t
p_getdumpstatus = {
	getdumpstatus, "getdumpstatus", NULL, NULL,
	"     getdumpstatus. Determine status of a list of dumps.\n"
};

arg_t
p_decompressdump = {
	decompressdump, "decompressdump", NULL, NULL,
	"     decompressdump. Stage, decompress and index a dump file.\n"
};

/*
 * Print out help text. Generic code called by several entries.
 */

static void
print_help_text(arg_t ** argp)
{
	int		i;
	while (*argp != NULL) {
		for (i = 0; i < 3; i++) {
			if ((*argp)->name[i] != NULL) {
				if (i != 0)
					printf(",");
				printf("%s", (*argp)->name[i]);
			}
		}
		printf("\n%s", (*argp)->desc);
		argp++;
	}
	no_action = 1;
}

static int
cli_help(char *ptr)
{
	print_help_text(cli_args);
	return (1);
}

arg_t
p_helpc = {
	cli_help, "-h", "-help", NULL,
	"     Print out this help message\n\n"
};

/*
 * filepath specification
 */

static int
filepath(char *ptr)
{
	node_t	 *path;

	if (ptr != NULL) {
		path = newstr(&pathlist, strlen(ptr));
		strcpy(path->data, ptr);	/* input */
		return (2);
	} else {
		printf("Missing <-filepath> argument\n");
		no_action = 1;
		return (1);
	}
}
arg_t
p_filepath = {
	filepath, "-p", "-path", "-filepath",
	"     Argument of -path is a filepath, somewhere in user space.\n\n"
};

/*
 * set string specification
 */

static int
csdstr(char *ptr)
{
	node_t	 *path;

	if (ptr != NULL) {
		path = newstr(&csdlist, strlen(ptr));
		strcpy(path->data, ptr);	/* input */
		return (2);
	} else {
		printf("Missing <-s> argument\n");
		no_action = 1;
		return (1);
	}
}
arg_t
p_csdstr = {
	csdstr, "-s", "-csdstr", NULL,
	"     Specify CSD dump options.\n\n"
};

/*
 * destination specification
 */

static int
dest(char *ptr)
{
	node_t	 *path;

	if (ptr != NULL) {
		path = newstr(&destlist, strlen(ptr));
		strcpy(path->data, ptr);	/* input */
		return (2);
	} else {
		printf("Missing <-s> argument\n");
		no_action = 1;
		return (1);
	}
}
arg_t
p_dest = {
	dest, "-d", "-dest", "-destination",
	"     Specify a destination pathname.\n\n"
};

/*
 * dump file specification
 */

static int
dumps(char *ptr)
{
	node_t	 *path;

	if (ptr != NULL) {
		path = newstr(&dmplist, strlen(ptr));
		strcpy(path->data, ptr);	/* input */
		return (2);
	} else {
		printf("Missing <-dump> argument\n");
		no_action = 1;
		return (1);
	}
}
arg_t
p_dump = {
	dumps, "-u", "-dump", "-dumpfile",
	"     Specify a dumpfile pathname.\n\n"
};

/*
 * Restrictions specification
 */

static int
rest(char *ptr)
{
	node_t	 *path;

	if (ptr != NULL) {
		path = newstr(&restlist, strlen(ptr));
		strcpy(path->data, ptr);	/* input */
		return (2);
	} else {
		printf("Missing <-restrictions> argument\n");
		no_action = 1;
		return (1);
	}
}
arg_t
p_rest = {
	rest, "-r", "-rest", "-restrictions",
	"     Specify restrictions in file searching.\n\n"
};

/*
 * filestructure specification
 */

static int
filestruct(char *ptr)
{
	node_t	 *path;

	if (ptr != NULL) {
		path = newstr(&filstructlist, strlen(ptr));
		strcpy(path->data, ptr);	/* input */
		return (2);
	} else {
		printf("Missing <-filestructure> argument\n");
		no_action = 1;
		return (1);
	}
}
arg_t
p_filestruct = {
	filestruct, "-f", "-filestruct", "-filestructure",
	"     Specify the name of a file structure.\n\n"
};

/*
 * Copy index
 */

static int
copyidx(char *ptr)
{
	if (ptr != NULL) {
		newint(&cpylist, atol(ptr));
			/* input, parsed into decimal */
		return (2);
	} else {
		printf("Missing <-copy> argument\n");
		no_action = 1;
		return (1);
	}
}
arg_t
p_copyidx = {
	copyidx, "-c", "-copy", "-copyindex",
	"     Specify the index of archive copy.\n\n"
};

/*
 * max entries
 */

static int
entries(char *ptr)
{
	if (ptr != NULL) {
		newint(&entrylist, atol(ptr));
			/* input, parsed into decimal */
		return (2);
	} else {
		printf("Missing <-entry> argument\n");
		no_action = 1;
		return (1);
	}
}
arg_t
p_entries = {
	entries, "-e", "-entries", "-maxentries",
	"     Specify maximum number of entries to return.\n\n"
};

/*
 * aparse, main entry to parser
 */

void
aparse(int argc, char *argv[], arg_t *argt[])
{
	char	*ptr;
	int	argn;
	int	argfound;
	int	i;
	char	*arg;
	arg_t	**argp;
	char	*namep;

	/* Parse command-line arguments */
	argn = 1;		/* First argument we want is #1 */

	while (argn < argc) {	/* While arguments remain, parse them */
		arg = argv[argn];
				/* Pointer to this argument string */
		argfound = 0;	/* Haven't parsed this argument yet */

		/* Walk through list of legal commands */
		for (argp = argt; argfound == 0; argp++) {
			if (*argp == NULL) {
				/* If reached end of table, give up */
				printf("Unrecognized switch %s\n", arg);
				argn++;
				break;
			}
			/* Try three possible names per switch */
			for (i = 0; i < 3; i++) {
			/* N.B Bad indentation to meet cstyle requirements */
			namep = (*argp)->name[i];
			if (namep == NULL)
				continue;
			if (strcasecmp(namep, arg) == 0) {
				if (argn + 1 < argc)
					ptr = argv[argn + 1];
				/* Next cmd token, if it exists */
				else
					ptr = NULL;
				argn += ((*argp)->routine) (ptr);
					/* Call parse routine */
				argfound = 1;
				break;
			} /* end if */
			} /* end for */
		}
	} /* End while argn */
}

/* ---------------------------------------------------------------- */

arg_t  *cli_args[] = {
	&p_helpc,
	&p_filepath,
	&p_setcsd,
	&p_getcsd,
	&p_listdumps,
	&p_restore,
	&p_takedump,
	&p_stagefiles,
	&p_listdir,
	&p_getfilestatus,
	&p_getfiledetails,
	&p_getdumpstatus,
	&p_getverdetails,
	&p_decompressdump,
	&p_listver,
	&p_search,
	&p_csdstr,
	&p_dest,
	&p_filestruct,
	&p_dump,
	&p_rest,
	&p_entries,
	&p_copyidx,
	NULL
};
