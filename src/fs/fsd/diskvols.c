/*
 * diskvols.c - Read disk volumes configuration file.
 */

/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

#pragma ident "$Revision: 1.30 $"

static char *_SrcFile = __FILE__;
/* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/param.h>
#include <fcntl.h>
#include <unistd.h>

/* Berkeley DB header. */
#include <db.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/dbfile.h"
#include "aml/diskvols.h"
#include "sam/lib.h"
#include "sam/readcfg.h"
#include "sam/sam_malloc.h"

/* Local headers. */
#include "fsd.h"

#define	DV_FLAGS_TO_PRESERVE (DV_labeled | DV_unavail | DV_read_only | \
				DV_bad_media | DV_needs_audit | DV_archfull)

/*
 * Disk volume dictionary type.
 */
enum {
	DT_client,
	DT_disk,
	DT_honeycomb
};

/* Private definitions. */
struct diskVolsConfig {
	boolean_t	client;
	vsn_t		vsn;
	host_t		host;
	host_t		addr;
	long		port;
	uint_t		flags;
	media_t		media;
	int			pathlen;
	char		path[1];
};

/* Directive processing functions. */
static void dirNoclients(void);
static void dirClients(void);
static void dirEndclients(void);
static void procClients(void);
static void cfgVolume(void);
static struct diskVolsConfig *createEntry(char *vsn, char *host,
					char *path, int pathlen, int type);
static void assignHoneycombProps(char *volname);
static void fatalDictError(DiskVolsDictionary_t *dict, char *msg);

/* Directives table */
static DirProc_t dirProcTable[] = {
	{ "endclients",		dirNoclients,		DP_set },
	{ "clients",		dirClients,		DP_set },
	{ NULL,			cfgVolume,		DP_param }
};

/* Private data. */
static struct {
	int 		count;
	size_t 		alloc;
	struct diskVolsConfig *data;
} diskVols = { 0, 0, NULL };

static char dirname[TOKEN_SIZE];
static char token[TOKEN_SIZE];
static char *noEnd = NULL;
static char *fname = NULL;

/*
 * Read disk volumes.
 */
void
ReadDiskVolumes(
		char *diskvols_name)	/* NULL if default conf file name */
{
	extern char *program_name;

	int	errors;
	int ret;
	DiskVolumeInfo_t *dv;

	fname = SAM_CONFIG_PATH"/diskvols.conf";
	if (diskvols_name != NULL) {
		fname = diskvols_name;
	}

	DiskVolCount = 0;
	DiskVolClientCount = 0;

	if (diskVols.data != NULL) {
		SamFree(diskVols.data);
		diskVols.count = 0;
		diskVols.alloc = 0;
		diskVols.data = NULL;
	}

	errors = ReadCfg(fname, dirProcTable, dirname, token, ConfigFileMsg);
	if (errors != 0 && !(errors == -1) && diskvols_name == NULL) {
		/* Absence of the default diskvols conf file not an error. */
		if (errors > 0) {
			errno = 0;
		}
		/* Read diskvols %s failed */
		/* Problem with file system command file. */
		FatalError(17230, fname);
	}
}

/*
 * Write diskvols dictionary.
 */
void
WriteDiskVolumes(boolean_t reconfig)
{
	extern char *program_name;
	static DiskVolumeInfo_t *elem = NULL;
	static DiskVolumeVersionVal_t *version = NULL;

	int ret;
	int i;
	size_t offset;
	size_t size;
	char *key;
	DiskVolumeInfo_t *dv;
	struct diskVolsConfig *config;
	DiskVolsDictionary_t *hdrDict;
	DiskVolsDictionary_t *vsnDict;
	DiskVolsDictionary_t *cliDict;

	if (reconfig == B_FALSE) {
		DiskVolsRecover(program_name);
	}

	/*
	 *	If no disk volumes or trusted clients configured.
	 */
	if (diskVols.count == 0) {
		DiskVolsUnlink();
		return;
	}

	/*
	 * Initialize dictionary handles.
	 */
	ret = DiskVolsInit(&hdrDict, DISKVOLS_HDR_DICT, program_name);
	if (ret != 0) {
		fatalDictError(hdrDict,
		    "Unable to initialize disk header dictionary");
	}

	ret = DiskVolsInit(&vsnDict, DISKVOLS_VSN_DICT, program_name);
	if (ret != 0) {
		fatalDictError(vsnDict,
		    "Unable to initialize disk volume dictionary");
	}

	ret = DiskVolsInit(&cliDict, DISKVOLS_CLI_DICT, program_name);
	if (ret != 0) {
		fatalDictError(vsnDict,
		    "Unable to initialize disk client dictionary");
	}

	/*
	 * Open dictionary header.
	 */
	ret = hdrDict->Open(hdrDict, 0);
	if (ret != 0) {
		DiskVolumeVersionVal_t versR45;

		/*
		 * Header doesn't exist.  If a dictionary header does not
		 * exist, either we have no dictionary or existing dictionary
		 * is 4.5.
		 * Create a header and set the version to 4.5.
		 */
		Trace(TR_MISC, "WriteDiskVolumes create hdr");
		ret = hdrDict->Open(hdrDict, DISKVOLS_CREATE);
		if (ret != 0) {
			fatalDictError(hdrDict,
			    "Unable to create disk header dictionary");
		}

		versR45 = DISKVOLS_VERSION_R45;

		ret = hdrDict->PutVersion(hdrDict, &versR45);
		if (ret != 0) {
			fatalDictError(hdrDict,
			    "Unable to write disk header dictionary");
		}
	}

	ret = hdrDict->GetVersion(hdrDict, &version);
	if (ret != 0) {
		fatalDictError(hdrDict,
		    "Unable to get disk dictionary version");
	}

	/*
	 * Open existing vsn dictionary.  Preserve flags we want from
	 * existing dictionary elements, then delete all volumes in the
	 * existing dictionary.
	 */
	ret = vsnDict->Open(vsnDict, 0);
	if (ret == 0) {

		for (i = 0, offset = 0; i < diskVols.count; i++) {

			config = (struct diskVolsConfig *)(void *)
			    ((char *)&diskVols.data[0] + offset);

			if (config->client == B_FALSE) {
				/*
				 * Get existing disk volume dictionary element.
				 * Set flags in new dictionary element that
				 * we want to preserve from the existing
				 * dictionary element.
				 */
				if (*version == DISKVOLS_VERSION_R45) {
					ret = vsnDict->GetOld(vsnDict,
					    config->vsn, &dv, *version);
				} else {
					ret = vsnDict->Get(vsnDict,
					    config->vsn, &dv);
				}
				if (ret == 0 && dv != NULL) {
					config->flags |=
					    (dv->DvFlags &
					    DV_FLAGS_TO_PRESERVE);
				}
			}
			offset += STRUCT_RND(sizeof (struct diskVolsConfig) +
			    config->pathlen);
		}

		/*
		 * Delete existing elements from dictionary.
		 */
		vsnDict->BeginIterator(vsnDict);
		while (vsnDict->GetIterator(vsnDict, &key, &dv) == 0) {
			ret = vsnDict->Del(vsnDict, key);
			if (ret != 0) {
				vsnDict->EndIterator(vsnDict);
				fatalDictError(vsnDict,
				    "Unable to remove disk volume "
				    "dictionary element");
			}
		}
		vsnDict->EndIterator(vsnDict);

	} else {
		/*
		 * Dictionary doesn't already exist, create it.
		 */
		ret = vsnDict->Open(vsnDict, DISKVOLS_CREATE);
		if (ret != 0) {
			fatalDictError(vsnDict,
			    "Unable to create disk volume dictionary");
		}
	}

	/*
	 * Open existing client dictionary and delete all clients from it.
	 */
	ret = cliDict->Open(cliDict, 0);
	if (ret == 0) {
		/*
		 * Delete existing elements from dictionary.
		 */
		cliDict->BeginIterator(cliDict);
		while (cliDict->GetIterator(cliDict, &key, &dv) == 0) {
			ret = cliDict->Del(cliDict, key);
			if (ret != 0) {
				cliDict->EndIterator(cliDict);
				fatalDictError(cliDict,
				    "Unable to remove disk client "
				    "dictionary element");
			}
		}
		cliDict->EndIterator(cliDict);

	} else {
		/*
		 * Dictionary doesn't already exist, create it.
		 */
		ret = cliDict->Open(cliDict, DISKVOLS_CREATE);
		if (ret != 0) {
			fatalDictError(cliDict,
			    "Unable to create disk client dictionary");
		}
	}

	if (*version == DISKVOLS_VERSION_R45) {
		ret = hdrDict->Del(hdrDict, DISKVOLS_VERSION_KEY);
		if (ret == 0) {
			*version = DISKVOLS_VERSION_VAL;
			ret = hdrDict->PutVersion(hdrDict, version);
		}
		if (ret != 0) {
			fatalDictError(hdrDict,
			    "Unable to set version in disk hdr "
			    "dictionary");
		}
	}


	/*
	 * Write vsns and clients to dictionary.
	 */
	for (i = 0, offset = 0; i < diskVols.count; i++) {

		config = (struct diskVolsConfig *)(void *)
		    ((char *)&diskVols.data[0] + offset);

		/*
		 * Allocate a new dictionary element.
		 */
		size = STRUCT_RND(sizeof (struct DiskVolumeInfo) +
		    config->pathlen);
		SamRealloc(elem, size);
		memset(elem, 0, size);

		if (config->client == B_TRUE) {
			/*
			 *	Trusted client.
			 */
			strncpy(elem->DvHost, config->host,
			    sizeof (elem->DvHost));

			key = config->host;

			ret = cliDict->Put(cliDict, key, elem);
			if (ret != 0) {
				fatalDictError(cliDict,
				    "Unable to write disk client "
				    "dictionary element");
			}

		} else {
			/*
			 *	Disk volume.
			 */
			elem->DvFlags = config->flags;
			elem->DvMedia = config->media;

			/*
			 * If not honeycomb and host name found, set volume
			 * defined on remote host.
			 */
			if ((config->media == DT_STK5800) &&
			    *config->host != '\0') {
				/* volume defined on remote host */
				elem->DvFlags |= DV_remote;
			}
			elem->DvPathLen = config->pathlen;
			strncpy(elem->DvPath, config->path, elem->DvPathLen);
			strncpy(elem->DvHost, config->host,
			    sizeof (elem->DvHost));

			if (DISKVOLS_IS_HONEYCOMB(elem)) {
				strncpy(elem->DvAddr, config->addr,
				    sizeof (elem->DvAddr));
				elem->DvPort = config->port;
				elem->DvSpace = elem->DvCapacity = FSIZE_MAX;
			}

			key = config->vsn;

			ret = vsnDict->Put(vsnDict, key, elem);
			if (ret != 0) {
				fatalDictError(vsnDict,
				    "Unable to write disk volume "
				    "dictionary element");
			}
		}
		offset += STRUCT_RND(sizeof (struct diskVolsConfig) +
		    config->pathlen);
	}

	(void) vsnDict->Close(hdrDict);
	(void) vsnDict->Close(vsnDict);
	(void) cliDict->Close(cliDict);

	(void) DiskVolsDestroy(hdrDict);
	(void) DiskVolsDestroy(vsnDict);
	(void) DiskVolsDestroy(cliDict);

	if (diskVols.data != NULL) {
		SamFree(diskVols.data);
		diskVols.count = 0;
		diskVols.alloc = 0;
		diskVols.data = NULL;
	}
}


/*
 * Process disk volume configuration entry.
 */
static void
cfgVolume(void)
{
	char *host;
	char *path;
	int	pathlen;
	int i;
	size_t offset;
	int dtype;
	struct diskVolsConfig *config;

	/*
	 *	Check for invalid (too long) volume name.
	 */
	if (strlen(dirname) > (sizeof (vsn_t) - 1)) {
		ReadCfgError(CustMsg(2881));
	}

	/*
	 *	Check for duplicate volume definition.
	 */
	for (i = 0, offset = 0; i < diskVols.count; i++) {
		config = (struct diskVolsConfig *)(void *)
		    ((char *)&diskVols.data[0] + offset);
		if (strcmp(dirname, config->vsn) == 0) {
			ReadCfgError(CustMsg(4463), dirname);
		}
		offset += STRUCT_RND(sizeof (struct diskVolsConfig) +
		    config->pathlen);
	}

	/*
	 *	Check if honeycomb configuration.
	 */
	if (strcmp(token, HONEYCOMB_RESOURCE_NAME) == 0) {
		assignHoneycombProps(dirname);
		return;
	}

	if ((path = strchr(token, ':')) != NULL) {
		host = token;
		*path++ = '\0';
	} else {
		host = "";
		path = token;
	}

	pathlen = strlen(path) + 1;

	/*
	 * Create a disk volume configuration entry.
	 */
	config = createEntry(dirname, host, path, pathlen, DT_disk);
	config->media = DT_DISK;

}


/*
 *	Error if found an 'endclients' but no 'clients' statement.
 */
static void
dirNoclients(void)
{
	ReadCfgError(CustMsg(4447), dirname + 3);
}


/*
 *	'clients' statement.
 */
static void
dirClients(void)
{
	static DirProc_t table[] = {
		{ "endclients",	dirEndclients,	DP_set   },
		{ NULL,			procClients,	DP_other },
	};
	char *msg;

	ReadCfgSetTable(table);
	msg = noEnd;
	noEnd = "noclients";
	if (msg != NULL) {
		ReadCfgError(CustMsg(4462), msg);
	}
}


/*
 *	'endclients' statement.
 */
static void
dirEndclients(void)
{
	ReadCfgSetTable(dirProcTable);
	noEnd = NULL;
}


/*
 * Process clients for disk archiving.
 */
static void
procClients(void)
{
	/*
	 * Create entry for trusted disk archiving client.
	 */
	(void) createEntry("", dirname, "/", 1, DT_client);
	DiskVolClientCount++;
}


/*
 * Create a disk volume entry.
 */
struct diskVolsConfig *
createEntry(
	char *vsn,
	char *host,
	char *path,
	int pathlen,
	int dtype)
{
	struct diskVolsConfig *config;
	size_t size;

	/*
	 * Allocate for new entry.
	 */
	size = STRUCT_RND(sizeof (struct diskVolsConfig) + pathlen);
	SamRealloc(diskVols.data, sizeof (struct diskVolsConfig) +
	    diskVols.alloc + size);
	config = (struct diskVolsConfig *)(void *)
	    ((char *)&diskVols.data[0] + diskVols.alloc);
	diskVols.alloc += size;
	memset(config, 0, sizeof (struct diskVolsConfig));
	diskVols.count++;

	/*
	 * Make a new configuration entry.
	 */
	if (dtype == DT_client) {
		config->client = B_TRUE;
	}

	strncpy(config->vsn,  vsn,  sizeof (config->vsn));
	strncpy(config->host, host, sizeof (config->host));

	config->pathlen = pathlen;
	strncpy(config->path, path, pathlen);

	DiskVolCount++;

	return (config);
}


/*
 * Assign honeycomb properties to disk volume entry.
 */
static void
assignHoneycombProps(char *volname)
{
	host_t addrCfg;
	char *portCfg;
	int port;
	struct diskVolsConfig *config;
	char *pathCfg;
	char *samHostCfg;
	int pathlen;
	upath_t metaDir;

	if (ReadCfgGetToken() == 0) {
		/* Honeycomb addr is required */
		ReadCfgError(CustMsg(31302));
	}
	strcpy(addrCfg, token);

	/*
	 * Honeycomb port is optional.
	 */
	portCfg = NULL;
	if ((portCfg = strchr(token, ':')) != NULL) {
		char *p;

		*portCfg++ = '\0';

		/*
		 * Strip port from address.
		 */
		p = &addrCfg[0];
		while (*p != '\0') {
			if (*p == ':') {
				*p = '\0';
				break;
			}
			p++;
		}
	}

	port = -1;		/* if not specified, use default port */
	if (portCfg != NULL) {
		char *p;

		p = portCfg;
		port = strtol(portCfg, &p, 0);
	}

	/*
	 * Check for meta directory name configuration parameter.
	 * Deferred.
	 */
	if (ReadCfgGetToken() != 0) {

		if (strcmp(token, "-metadir") == 0) {
			if (ReadCfgGetToken() == 0) {
				/* ERROR */
			}

			if ((pathCfg = strchr(token, ':')) != NULL) {
				samHostCfg = token;
				*pathCfg++ - '\0';
			} else {
				samHostCfg = "";
				pathCfg = token;
			}

		}

	} else {
		/*
		 * Set defaults for meta directory name, .ie
		 * /var/opt/SUNWsamfs/catalog/stk5800
		 */
		samHostCfg = "";

		/*
		 * If necessary, make 'stk5800' directory.
		 */
		snprintf(metaDir, sizeof (metaDir), "%s/%s",
		    SAM_CATALOG_DIR, HONEYCOMB_RESOURCE_NAME);
		MakeDir(metaDir);

		pathCfg = metaDir;
	}

	pathlen = strlen(pathCfg) + 1;

	config = createEntry(volname, samHostCfg, pathCfg, pathlen, DT_disk);
	config->media = DT_STK5800;

	strncpy(config->addr, addrCfg, sizeof (config->addr));
	config->port = port;
}

/*
 *	Fatal error writing disk volumes dictionary.  Generate trace
 *	message, cleanup database handles.
 */
static void
fatalDictError(
	DiskVolsDictionary_t *dict,
	char *msg)
{
	if (msg != NULL) {
		Trace(TR_ERR, "%s", msg);
	}

	if (dict != NULL) {
		(void) dict->Close(dict);
		(void) DiskVolsDestroy(dict);
	}

	if (diskVols.data != NULL) {
		SamFree(diskVols.data);
		diskVols.count = 0;
		diskVols.alloc = 0;
		diskVols.data = NULL;
	}

	FatalError(17230, fname);
}
