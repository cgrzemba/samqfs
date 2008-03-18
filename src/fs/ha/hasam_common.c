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

/*
 * hasam_common.c - Common routines for SUNW.hasam RT.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/vfstab.h>
#include <sys/mnttab.h>
#include <sys/sysmacros.h>
#include <arpa/inet.h>
#include <procfs.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include "sam/mount.h"
#include "sam/lib.h"
#include "sam/nl_samfs.h"

#include "hasam.h"

/* By default disable message output to STDERR */
int hasam_debug = 0;
int samd_timer = 10;

/* This routine is not declared in dirent.h */
extern int readdir64_r(DIR *, dirent64_t *, dirent64_t **);

/* This routine is not declared in header files */
extern char *dirname(char *);

/*
 * Helper function to use read() correctly
 *
 * Return: success = # of bytes read [or] failure = -1
 */
int
myreadbuf(
	int	fd,
	void	*buffer,
	int	len)
{
	int	numread = 0;
	int	ret;
	char	*bufp;

	if ((buffer == NULL) || (len < 1) || (fd == -1)) {
		return (-1);
	}

	bufp = buffer;

	while (numread < len) {
		ret = read(fd, bufp, (len - numread));

		if (ret == -1) {
			if (errno == EAGAIN) {
				continue;
			}
			numread = -1;
			break;
		}

		numread += ret;
		bufp += ret;
	}

	return (numread);
}

/*
 * Read the proc table and search if the required SAM daemons
 * are listed in it.
 *
 * Return: success = 0 [or] failure = -1
 */
int
sam_find_processes(struct daemons_list	*d_list)
{
	DIR*		dirp;
	dirent64_t	*dent;
	dirent64_t	*dentp;
	char		pname[MAXLEN];
	int		procfd; /* fd for /proc/nnnnn/psinfo */
	psinfo_t	info;   /* process information from /proc */
	int		ret;
	int		len = sizeof (info);
	int		prindex;
	int		daemons = 0;
	int		allfound;

	if ((dirp = opendir(PROCDIR)) == NULL) {
		scds_syslog(LOG_ERR,
		    "Unable to open /proc");
		return (-1);
	}

	/* allocate the dirent structure */
	dent = malloc(MAXLEN + sizeof (dirent64_t));
	if (dent == NULL) {
		closedir(dirp);
		scds_syslog(LOG_ERR, "Out of Memory");
		return (-1);
	}

	/* find each active process --- */
	while ((ret = readdir64_r(dirp, dent, &dentp)) == 0) {

		if (dentp == NULL) {
			break;
		}

		/* skip . and .. */
		if (dentp->d_name[0] == '.') {
			continue;
		}

		snprintf(pname, MAXLEN, "%s/%s/%s", PROCDIR,
		    dentp->d_name, "psinfo");

		procfd = open64(pname, O_RDONLY);
		if (procfd == -1) {
			/* process may have ended while we were processing */
			continue;
		}

		/*
		 * Get the info structure for the process and close quickly.
		 */
		ret = myreadbuf(procfd, &info, len);

		(void) close(procfd);

		if (ret == -1) {
			break;
		}

		if (info.pr_lwp.pr_state == 0)
			continue;

		/* ensure cmd buffers properly terminated */
		info.pr_psargs[PRARGSZ-1] = '\0';
		info.pr_fname[PRFNSZ-1] = '\0';

		/* is it one of the procs we're looking for ? */
		for (prindex = 0; d_list[prindex].daemon_name != NULL;
		    prindex++) {
			if (d_list[prindex].found == 1)
				continue;

			if (strcmp(info.pr_fname,
			    d_list[prindex].daemon_name) == 0) {
				d_list[prindex].found = 1;
				daemons++;
			}
		}
	}

	closedir(dirp);
	free(dent);

	allfound = 0;
	for (prindex = 0; d_list[prindex].daemon_name != NULL;
	    prindex++) {
		if (!d_list[prindex].found) {
			allfound = -1;
			scds_syslog(LOG_ERR, "Daemon not found: %s.",
			    d_list[prindex].daemon_name);
			dprintf(stderr, "Daemon not found: %s\n",
			    d_list[prindex].daemon_name);
		} else {
			dprintf(stderr, "Found essential daemon: %s\n",
			    d_list[prindex].daemon_name);
		}
	}

	dprintf(stderr, "Total daemons found = %d of %d\n",
	    daemons, prindex);

	return (allfound);
}


/*
 * Verify if essential SAM daemons - sam-fsd, sam-catserverd are
 * running on the node on which atleast one tape library with
 * drives are configured.
 *
 * Return: success = B_TRUE [or] failure = B_FALSE
 */
boolean_t
verify_essential_sam_daemons(void)
{
	struct daemons_list sam_daemons_list[] = {
		{ "sam-fsd", 		0 },
		{ "sam-catserverd",	0 },
		{ NULL,				0 }
	};

	if (sam_find_processes(sam_daemons_list) < 0) {
		return (B_FALSE);
	}

	return (B_TRUE);
}


/*
 * Verify if atleast sam-fsd is running on the node on
 * which tape library is not configured.
 *
 * Return: success = B_TRUE [or] failure = B_FALSE
 */
boolean_t
is_sam_fsd(void)
{
	struct daemons_list sam_daemons_list[] = {
		{ "sam-fsd", 		0 },
		{ NULL,				0 }
	};

	if (sam_find_processes(sam_daemons_list) < 0) {
		return (B_FALSE);
	}

	return (B_TRUE);
}


/*
 * Check if a supplied SAM daemon is running on the node.
 *
 * Return: success = B_TRUE [or] failure = B_FALSE
 */
boolean_t
check_sam_daemon(char *sam_daemon_name)
{
	return (find_string(GET_DAEMONS_CMD, sam_daemon_name));
}


/*
 * Run the given command and check if the supplied string
 * exists in the output of that command.
 *
 * Return: success = B_TRUE [or] failure = B_FALSE
 */
static boolean_t
find_string(
	char *cmd,
	char *string)
{
	boolean_t flag = B_FALSE;
	char line[80];
	char *s;
	size_t len;
	FILE *ptr;

	if ((ptr = popen(cmd, "r")) != NULL) {
		while (NULL != fgets(line, 80, ptr)) {
			/* printf("\n%s", line); */
			s = strstr(line, string);
			if (s != NULL) {
				flag = B_TRUE;
				len = strlen(s);
				s[len -1] = '\0';
				break;
			}
		}
	}
	pclose(ptr);
	return (flag);
}


/*
 * Run samfsinfo command and check its output on the node.
 *
 * Return: success = B_TRUE [or] failure = B_FALSE
 */
boolean_t
check_samfsinfo(char *fs_name)
{
	char cmd[MAXLEN];

	(void) snprintf(cmd, sizeof (cmd),
	    SAM_EXECUTE_PATH "/samfsinfo %s > /dev/null 2>&1",
	    fs_name);

	if (run_cmd_4_hasam(cmd) != 0) {
		return (B_FALSE);
	}
	return (B_TRUE);
}


/*
 * Run samsharefs command and check its output on the node.
 *
 * Return: success = B_TRUE [or] failure = B_FALSE
 */
boolean_t
check_samshare(char *fs_name)
{
	char cmd[MAXLEN];

	(void) snprintf(cmd, sizeof (cmd),
	    SAM_EXECUTE_PATH "/samsharefs %s > /dev/null 2>&1",
	    fs_name);

	if (run_cmd_4_hasam(cmd) != 0) {
		return (B_FALSE);
	}
	return (B_TRUE);
}


/*
 * Check if all the QFS filesystems supplied to HA-SAM are mounted.
 *
 * Return: success = B_TRUE [or] failure = B_FALSE
 */
boolean_t
check_all_fs_mounted(struct FsInfo_4hasam *fp)
{
	int i;
	boolean_t mnt_status = B_FALSE;

	if (num_fs <= 0) {
		return (mnt_status);
	}

	for (i = 0; i < num_fs; i++) {
		mnt_status = check_fs_mounted(fp[i].fi_fs);
		if (mnt_status == B_FALSE) {
			scds_syslog(LOG_ERR, "Error FS not mounted %s !",
			    fp[i].fi_fs);
			break;
		}
	}

	return (mnt_status);
}


/*
 * Mounts the QFS filesystem given the family set name
 * specified via the QFSName Extension property.
 * Calls /usr/sbin/mount to do the actual mounting.
 */
int
mount_fs_4hasam(char *fs_info)
{
	char cmd[MAXLEN];

	(void) snprintf(cmd, MAXLEN, "/usr/sbin/mount %s", fs_info);

	return (run_cmd_4_hasam(cmd));
}



/*
 * Check if a given QFS filesystem is mounted.
 *
 * Return: success = B_TRUE [or] failure = B_FALSE
 */
boolean_t
check_fs_mounted(char *fs_name)
{
	boolean_t mounted = B_FALSE;
	char *mountpt;

	mountpt = get_mntpt_from_fsname(fs_name);
	if (mountpt == NULL) {
		dprintf(stderr, "Error getting %s FS mount point\n");
		scds_syslog(LOG_ERR, "Error getting %s FS mount point");
		return (B_FALSE);
	}
	dprintf(stderr, "%s FS mounted\n", mountpt);
	scds_syslog(LOG_DEBUG, "%s FS mounted", mountpt);

	return (B_TRUE);
}

/*
 * Check if the given filesystem exists in mnttab on the node.
 *
 * Return: success = B_TRUE [or] failure = B_FALSE
 */
char *
get_mntpt_from_fsname(
	char *fsname)    /* Input, file system name */
{
	FILE *mnttab;
	struct mnttab mnt;
	struct mnttab inmnt;
	int ret;
	char *mntpt;

	mnttab = fopen64(MNTTAB, "r");
	if (mnttab == NULL) {
		scds_syslog(LOG_ERR, "Unable to open MNTTAB");
		return (NULL);
	}
	rewind(mnttab);   /* ensure set to top of file */

	/*
	 * set up the mnttab structure for the filesystem
	 * we're looking for
	 */
	memset(&inmnt, 0, sizeof (struct mnttab));
	inmnt.mnt_special = fsname;

	ret = getmntany(mnttab, &mnt, &inmnt);

	fclose(mnttab);

	if (ret != 0) {   /* no match */
		scds_syslog(LOG_ERR, "No such FS %s", fsname);
		return (NULL);
	}

	mntpt = strdup(mnt.mnt_mountp);
	return (mntpt);
}


/*
 * Check if the catalog mount point exists on the node.
 *
 * Return: success = B_TRUE [or] failure = B_FALSE
 */
boolean_t
check_catalogfs(void)
{
	FILE *mfp;
	struct mnttab  mget;
	int ret;

	if (catalogfs_mntpt == NULL) {
		scds_syslog(LOG_ERR,
		    "Error Catalog FS %s not configured in RT !",
		    catalogfs_mntpt);
		return (B_FALSE);
	}

	/*
	 * Check if catalog FS has a global mount option
	 * in vfstab to make sure it is a cluster filesystem
	 */
	if (!check_vfstab(catalogfs_mntpt, "other", "global")) {
		return (B_FALSE);
	}

	/*
	 * Settings in vfstab looks good, now check if
	 * catalog FS is actually mounted
	 */
	ret = search_mount_point(catalogfs_mntpt);
	if (ret < 0) {
		/* Catalog FS not mounted */
		scds_syslog(LOG_ERR,
		    "Error Catalog FS %s not configured or mounted !",
		    catalogfs_mntpt);
		dprintf(stderr,
		    "Error Catalog FS %s not configured or mounted !\n",
		    catalogfs_mntpt);
		return (B_FALSE);

	} else if (ret == 1) {
		/* Catalog FS not mounted and it must be mounted */
		scds_syslog(LOG_DEBUG, "Catalog FS %s requires mount",
		    catalogfs_mntpt);
		dprintf(stderr, "Catalog FS %s requires mount\n",
		    catalogfs_mntpt);

		/*
		 * Attempt to mount catalog FS,
		 * if it fails return B_FALSE
		 */
		if (mount_fs_4hasam(catalogfs_mntpt) != 0) {
			scds_syslog(LOG_ERR,
			    "Unable to mount %s, mount it manually !",
			    catalogfs_mntpt);
			dprintf(stderr,
			    "Unable to mount %s, mount it manually !",
			    catalogfs_mntpt);
			return (B_FALSE);
		}
	}

	dprintf(stderr, "Catalog FS %s mounted\n",
	    catalogfs_mntpt);

	return (B_TRUE);
}


/*
 * Build a list of all mount points listed in MNTTAB and VFSTAB
 * This list will be used later by catalog, stager and diskarchive
 * to check if the directory path is a valid entry and it is on a
 * filesystem that is mounted
 *
 * Return: success = a pointer to allocated node [or]
 *         failure = NULL
 */
mp_node_t *
mp_alloc(void)
{
	mp_node_t *newnode;

	newnode = (mp_node_t *)malloc(sizeof (mp_node_t));
	if (newnode == NULL) {
		scds_syslog(LOG_ERR, "malloc failed for %s",
		    strerror(errno));
		dprintf(stderr, "malloc failed for %s\n",
		    strerror(errno));
		return (NULL);
	}

	newnode->mp_name = NULL;
	newnode->next = NULL;
	return (newnode);
}


/*
 * Read mnttab and load all entries into a linked list
 * A new node is allocated for each entry and saved for
 * use later
 *
 * Return: success = 0 [or] failure = -1
 */
int
parse_mnttab(void)
{
	FILE    *mfp;
	struct  mnttab  mget;
	mp_node_t *newnode, *prevnode;

	headnode_m = mp_alloc();
	if (headnode_m == NULL) {
		return (-1);
	}
	prevnode = headnode_m;

	/* Read mnttab */
	mfp = fopen(MNTTAB, "ro");
	if (mfp == NULL) {
		scds_syslog(LOG_ERR, "Unable to open mnttab: %s",
		    strerror(errno));
		dprintf(stderr, "Unable to open mnttab: %s\n",
		    strerror(errno));
		free(headnode_m);
		return (-1);
	}

	while (getmntent(mfp, &mget) != -1) {

		if (mget.mnt_mountp) {
			newnode = mp_alloc();
			if (newnode == NULL) {
				fclose(mfp);
				return (-1);
			}

			newnode->mp_name = strdup(mget.mnt_mountp);
			prevnode->next = newnode;
			prevnode = newnode;
		}
	}

	(void) fclose(mfp);
	return (0);
}


/*
 * Read vfstab and load all entries into a linked list
 * A new node is allocated for each entry and saved for
 * use later
 *
 * Return: success = 0 [or] failure = -1
 */
int
parse_vfstab(void)
{
	FILE    *vfp;
	struct  vfstab vfsent;
	mp_node_t *newnode, *prevnode;

	headnode_v = mp_alloc();
	if (headnode_v == NULL) {
		return (-1);
	}
	prevnode = headnode_v;

	/* Read vfstab file */
	vfp = fopen(VFSTAB, "ro");
	if (vfp == NULL) {
		scds_syslog(LOG_ERR, "Unable to open vfstab: %s",
		    strerror(errno));
		dprintf(stderr, "Unable to open vfstab: %s\n",
		    strerror(errno));
		free(headnode_v);
		return (-1);
	}

	while (getvfsent(vfp, &vfsent) != -1) {

		if (vfsent.vfs_mountp) {
			newnode = mp_alloc();
			if (newnode == NULL) {
				fclose(vfp);
				return (-1);
			}

			newnode->mp_name = strdup(vfsent.vfs_mountp);
			prevnode->next = newnode;
			prevnode = newnode;
		}
	}

	(void) fclose(vfp);
	return (0);
}



/*
 * Collect all mount points from mnttab and vfstab
 *
 * Return: success = 0 [or] failure = -1
 */
int
get_all_mount_points(void)
{
	if (parse_mnttab() < 0) {
		return (-1);
	}

	if (parse_vfstab() < 0) {
		return (-1);
	}

	return (0);
}



/*
 * Free the list of mount points collected from mnttab and vfstab
 */
void
free_all_mount_points(void)
{
	mp_node_t *p, *q;

	/* Free mount point linked list collected from mnttab */
	if (headnode_m) {
		for (p = headnode_m; p != NULL; p = q) {
			q = p->next;
			free(p->mp_name);
			free(p);
		}
	}

	/* Free mount point linked list collected from vfstab */
	if (headnode_v) {
		for (p = headnode_v; p != NULL; p = q) {
			q = p->next;
			free(p->mp_name);
			free(p);
		}
	}
}


/*
 * Search for a match in the list of mount points collected
 * from mnttab and vfstab
 *
 * Return: success = 0 if mounted [or] failure = -1
 *         1 if mount point is found in vfstab but not mounted
 */
int
search_mount_point(const char *dir_to_find)
{
	mp_node_t *currnode;

	/*
	 * Compare the given directory with the mount point
	 * list from mnttab to confirm if it is mounted
	 */
	currnode = headnode_m->next;
	while (currnode) {
		if ((strcmp(currnode->mp_name, dir_to_find)) == 0) {
			dprintf(stderr, "Found %s in mnttab\n",
			    currnode->mp_name);
			return (0);
		}
		currnode = currnode->next;
	}

	/*
	 * Compare the given directory with the mount point
	 * list from vfstab to confirm if it exists but it
	 * needs to be mounted
	 */
	currnode = headnode_v->next;
	while (currnode) {
		if ((strcmp(currnode->mp_name, dir_to_find)) == 0) {
			dprintf(stderr,
			    "Found %s in vfstab but not mounted !\n",
			    currnode->mp_name);
			return (1);
		}
		currnode = currnode->next;
	}

	return (-1);
}


/*
 * Check if file name is a link and it points to a directory
 *
 * Return: success = mount point found for directory [or]
 *         failure = NULL
 */
char *
search_link(char *link_name)
{
	struct stat64  lbuf;
	char	linkbuf[1024];
	int readlen;

	/* Check if given link is valid */
	if (lstat64(link_name, &lbuf) != 0 ||
	    !S_ISLNK(lbuf.st_mode)) {
		scds_syslog(LOG_ERR,
		    "%s NOT a link to directory in cluster filesystem !",
		    link_name);
		dprintf(stderr,
		    "%s NOT a link to directory in cluster filesystem !\n",
		    link_name);
		return (NULL);
	}

	/*
	 * File is configured as a link, check if the linked
	 * directory exists in cluster filesystem.
	 */
	readlen = readlink(link_name, linkbuf, sizeof (linkbuf));
	if (readlen <= 0) {
		scds_syslog(LOG_ERR,
		    "Can't get DIR from link %s", link_name);
		dprintf(stderr,
		    "Can't get DIR from link %s\n", link_name);
		return (NULL);
	}

	linkbuf[readlen] = '\0';
	dprintf(stderr, "Directory %s linked to %s\n",
	    linkbuf, link_name);
	/*
	 * Got a valid directory name, now validate if its good
	 * and return the status directly from function search_dir
	 */
	return (search_dir(linkbuf));
}



/*
 * Check if directory exists and it mount point is on a cluster FS
 *
 * Return: success = mount point found for directory [or]
 *         failure = NULL
 */
char *
search_dir(char *srch_dir)
{
	struct stat64	statbuf;
	int dirlen, mount_now;
	char *srcdir;
	char *found;

	/*
	 * Root dir can't be data directory for HA-SAM configuration,
	 * so return seach failure if the supplied directory is root
	 */
	if ((strcmp(srch_dir, "/")) == 0) {
		dprintf(stderr, "Root directory can't be cluster FS\n",
		    srch_dir);
		return (NULL);
	}

	/* Check if given directory is valid */
	if (lstat64(srch_dir, &statbuf) != 0 ||
	    !S_ISDIR(statbuf.st_mode)) {
		scds_syslog(LOG_ERR,
		    "Directory %s does not exist !", srch_dir);
		dprintf(stderr,
		    "Directory %s does not exist !\n", srch_dir);
		return (NULL);
	}

	/*
	 * Remove trailing slashes if it exists in the
	 * directory name to search
	 */
	dirlen = strlen(srch_dir) - 1;
	while (dirlen > 0 && srch_dir[dirlen] == '/') {
		srch_dir[dirlen--] = '\0';
	}

	srcdir = strdup(srch_dir);

	/*
	 * Check is the given directory matches any of
	 * mount points in mnttab, if not it needs to be
	 * mounted
	 */
	while (1) {
		mount_now = search_mount_point(srch_dir);
		if (mount_now == 0) {
			dprintf(stderr,
			    "Got mount point %s for directory %s\n",
			    srch_dir, srcdir);
			break;

		} else if (mount_now == 1) {
			/*
			 * Found the mount point in vfstab but its
			 * not actually mounted. So this needs to be
			 * mounted
			 */
			scds_syslog(LOG_DEBUG,
			    "Mount %s to access directory %s",
			    srch_dir, srcdir);
			dprintf(stderr,
			    "Mount %s to access directory %s\n",
			    srch_dir, srcdir);
			break;
		}

		/*
		 * Go back one level and check again
		 * if its a mount point
		 */
		dirname(srch_dir);
		if ((strcmp(srch_dir, "/")) == 0) {
			scds_syslog(LOG_ERR,
			    "Mount point not found for directory %s !",
			    srcdir);
			dprintf(stderr,
			    "Mount point not found for directory %s !\n",
			    srcdir);
			break;
		}
		dprintf(stderr, "Next lookup  : %s\n", srch_dir);
	}

	found = strdup(srch_dir);
	return (found);
}



/*
 * Wrapper to run samd stop command on the node.
 *
 * Return: success = B_TRUE [or] failure = B_FALSE
 */
boolean_t
samd_stop(void)
{
	char cmd[MAXLEN];
	int rc;

	(void) snprintf(cmd, MAXLEN, SAM_EXECUTE_PATH "/samd stop");

	rc = run_cmd_4_hasam(cmd);
	if (rc != 0) {
		return (B_FALSE);
	}
	return (B_TRUE);
}


/*
 * Wrapper to run samd start command on the node.
 *
 * Return: success = B_TRUE [or] failure = B_FALSE
 */
boolean_t
samd_start(void)
{
	char cmd[MAXLEN];
	int rc;

	(void) snprintf(cmd, MAXLEN,
	    SAM_EXECUTE_PATH "/samd start > /dev/null 2>&1");

	rc = run_cmd_4_hasam(cmd);
	if (rc != 0) {
		return (B_FALSE);
	}
	return (B_TRUE);
}


/*
 * Wrapper to run samd config command on the node.
 *
 * Return: success = B_TRUE [or] failure = B_FALSE
 */
boolean_t
samd_config(void)
{
	char cmd[MAXLEN];
	int rc;

	(void) snprintf(cmd, MAXLEN,
	    SAM_EXECUTE_PATH "/samd config > /dev/null 2>&1");

	rc = run_cmd_4_hasam(cmd);
	if (rc != 0) {
		return (B_FALSE);
	}
	return (B_TRUE);
}


/*
 * Wrapper to check stager configuration on the node
 * /var/opt/SUNWsamfs/stager should be a link to a directory
 * on cluster filesystem. This is required for stager daemons
 * to access the staging queue from all nodes in a cluster
 *
 * Return: success = B_TRUE [or] failure = B_FALSE
 */
boolean_t
check_stager_conf(void)
{
	char *link_name = SAM_STAGERD_HOME;
	char *mntpt;

	mntpt = search_link(link_name);
	if (mntpt == NULL) {
		scds_syslog(LOG_ERR, "Error in stager configuration !");
		dprintf(stderr, "Error in stager configuration !\n");
		return (B_FALSE);
	}

	dprintf(stderr,
	    "Stager directory %s for link %s configured\n",
	    mntpt, link_name);
	/*
	 * Now check if catalog FS has a global mount option
	 * in vfstab to make sure it is a cluster filesystem
	 */
	if (!check_vfstab(mntpt, "other", "global")) {
		scds_syslog(LOG_ERR,
		    "%s must have \"global\" as mount option !\n",
		    mntpt);
		dprintf(stderr,
		    "%s must have \"global\" as mount option !\n",
		    mntpt);
		return (B_FALSE);
	}

	return (B_TRUE);
}


/*
 * Wrapper to run stop stager command on the node
 * and wait till the stidle timer expires.
 *
 * Return: success = B_TRUE [or] failure = B_FALSE
 */
boolean_t
stop_stager(void)
{
	char cmd[MAXLEN];
	int rc;

	hasam_stidle_timer = 15;

	(void) snprintf(cmd, MAXLEN, SAM_EXECUTE_PATH "/samcmd stidle");

	rc = run_cmd_4_hasam(cmd);
	sleep(hasam_stidle_timer);
	if (rc != 0) {
		return (B_FALSE);
	}
	return (B_TRUE);
}


/*
 * Wrapper to signal sam-fsd to stop respawning archiver
 * and stager daemons on QFS MDS during failover.
 *
 * Return: void
 */
void
samd_hastop(void)
{
	char cmd[MAXLEN];
	int rc;

	/*
	 * Use samd hastop command to signal sam-fsd
	 */
	(void) snprintf(cmd, MAXLEN, SAM_EXECUTE_PATH "/samd hastop");
	rc = run_cmd_4_hasam(cmd);
	if (rc != 0) {
		scds_syslog(LOG_ERR, "hastop failed !");
		dprintf(stderr, "hastop failed !\n");
	}
	sleep(5);
}


/*
 * Run ping command to a remote host and check its output.
 * This command is mainly used by disk archive function.
 *
 * Return: success = B_TRUE [or] failure = B_FALSE
 */
boolean_t
ping_host(char *host2ping)
{
	char cmd[MAXLEN];

	(void) snprintf(cmd, sizeof (cmd),
	    "/usr/sbin/ping %s > /dev/null 2>&1", host2ping);

	if (run_cmd_4_hasam(cmd) != 0) {
		return (B_FALSE);
	}
	return (B_TRUE);
}


/*
 * Main routine to run sam-fs utilities using popen call.
 *
 * Return: status of command execution on the node.
 */
int
run_cmd_4_hasam(char *testcmd)
{
	FILE *ptr;
	int cmd_status;

	scds_syslog(LOG_DEBUG, "run_cmd_4_hasam %s", testcmd);

	if ((ptr = popen(testcmd, "w")) == NULL) {
		return (-1);
	}
	cmd_status = pclose(ptr);
	return (cmd_status);
}

/*
 * Get all the ds/cluster/rg information we need to handle
 * the resource group, and save it in the given rgp structure.
 *
 * Return: success = 0 [or] failure = -1
 */
int
GetRgInfo_4hasam(
	int ac,
	char **av,
	struct RgInfo_4hasam *rgp)
{
	scha_extprop_value_t	*xprop = NULL;
	scha_err_t				e;

	bzero((char *)rgp, sizeof (*rgp));

	if (scds_initialize(&rgp->rg_scdsh, ac, av) != SCHA_ERR_NOERR) {
		return (-1);
	}

	/*
	 * Get Cluster handle.
	 */
	e = scha_cluster_open(&rgp->rg_cl);
	if (e != SCHA_ERR_NOERR) {
		/* 22644 */
		scds_syslog(LOG_ERR, "Unable to get cluster handle: %s.",
		    scds_error_string(e));
		return (-1);
	}

	/*
	 * Get nodename of this node.
	 */
	e = scha_cluster_get(rgp->rg_cl, SCHA_NODENAME_LOCAL,
	    &rgp->rg_nodename);
	if (e != SCHA_ERR_NOERR) {
		/* 22645 */
		scds_syslog(LOG_ERR, "Unable to get local node name: %s.",
		    scds_error_string(e));
		return (-1);
	}

	/*
	 * Get the name of the resource group we're handling.
	 */
	rgp->rg_name = scds_get_resource_group_name(rgp->rg_scdsh);
	if (!rgp->rg_name) {
		/* 22688 */
		scds_syslog(LOG_ERR, "Unable to get resource group name.");
		return (-1);
	}
retry:
	e = scha_resourcegroup_open(rgp->rg_name, &rgp->rg_rgh);
	if (e != SCHA_ERR_NOERR) {
		/* 22697 */
		scds_syslog(LOG_ERR, "Unable to open resource group '%s': %s.",
		    rgp->rg_name, scds_error_string(e));
		return (-1);
	}

	/*
	 * Get the list of nodes that can host the resource.
	 */
	e = scha_resourcegroup_get(rgp->rg_rgh, SCHA_NODELIST,
	    &rgp->rg_nodes);
	if (e != SCHA_ERR_NOERR || !rgp->rg_nodes ||
	    !rgp->rg_nodes->array_cnt) {
		if (e == SCHA_ERR_SEQID) {
			/* 22698 */
			scds_syslog(LOG_INFO,
			    "Retrying resource group get (%s): %s.",
			    rgp->rg_name, scds_error_string(e));
			(void) scha_resourcegroup_close(rgp->rg_rgh);
			rgp->rg_rgh = NULL;
			(void) sleep(1);
			goto retry;
		}
		/* 22689 */
		scds_syslog(LOG_ERR,
		    "Unable to get nodelist for resource group %s.",
		    rgp->rg_name);
		return (-1);
	}

	/*
	 * Get the list of filesystems in the resource group.
	 */
	e = scds_get_ext_property(rgp->rg_scdsh,
	    QFSNAME, SCHA_PTYPE_STRINGARRAY, &xprop);
	if (e != SCHA_ERR_NOERR ||
	    !xprop ||
	    !xprop->val.val_strarray ||
	    !xprop->val.val_strarray->array_cnt) {
		/* 22600 */
		scds_syslog(LOG_ERR,
		    "Failed to retrieve the property %s: %s.",
		    QFSNAME, scds_error_string(e));
		return (-1);
	}
	rgp->rg_mntpts = xprop->val.val_strarray;
	scds_syslog(LOG_DEBUG, "QFSName %s", rgp->rg_mntpts);

	e = scds_get_ext_property(rgp->rg_scdsh,
	    CATALOGFS, SCHA_PTYPE_STRING, &xprop);
	if (e != SCHA_ERR_NOERR ||
	    !xprop ||
	    !xprop->val.val_str) {
		scds_syslog(LOG_ERR,
		    "Failed to retrieve the property %s: %s.",
		    CATALOGFS, scds_error_string(e));
		return (-1);
	}
	rgp->rg_catalog = xprop->val.val_str;
	strcpy(catalogfs_mntpt, rgp->rg_catalog);
	scds_syslog(LOG_DEBUG, "CatalogFS %s", rgp->rg_catalog);

	return (0);
}


/*
 * Close all the ds/cluster/rg handles and zero out
 * the associated info structure.
 */
void
RelRgInfo_4hasam(struct RgInfo_4hasam *rgp)
{
	scha_err_t e;

	if ((e = scha_resourcegroup_close(rgp->rg_rgh)) != SCHA_ERR_NOERR) {
		scds_syslog(LOG_ERR,
		    "RG %s: scha_resourcegroup_close() failed: %s.",
		    rgp->rg_name, scds_error_string(e));
	}
	if ((e = scha_cluster_close(rgp->rg_cl)) != SCHA_ERR_NOERR) {
		scds_syslog(LOG_ERR,
		    "RG %s: scha_cluster_close() failed: %s.",
		    rgp->rg_name, scds_error_string(e));
	}
	scds_close(&rgp->rg_scdsh);

	bzero((char *)rgp, sizeof (*rgp));
}


/*
 * Free up a struct FsInfo array allocated by GetFileSystemInfo_4hasam
 */
void
FreeFileSystemInfo_4hasam(struct FsInfo_4hasam *fip)
{
	int i;

	for (i = 0; fip[i].fi_mntpt != NULL; i++) {
		free((void *)fip[i].fi_mntpt);
		if (fip[i].fi_fs != NULL) {
			free((void *)fip[i].fi_fs);
		}
	}
	free((void *)fip);

	/* Free all mount points gathered from mnttab and vfstab */
	free_all_mount_points();
}

/*
 * Allocate an array of FsInfo_4hasam structures, one for each file system
 * in the resource group.  Set each file system's mount point and its
 * family set name into an FsInfo structure.  Leave the rest zero.
 * Add an extra entry to the end with a NULL fi_mntpnt pointer.
 * Collect a list of mount points which will be used later by
 * catalog, stager and disk archives.
 *
 * Return: success = struct FsInfo_4hasam [or] failure = NULL
 */
struct FsInfo_4hasam *
GetFileSystemInfo_4hasam(
	struct RgInfo_4hasam *rgp)
{
	int i, fscount;
	struct FsInfo_4hasam *fip;

	fscount = rgp->rg_mntpts->array_cnt;
	num_fs = fscount;

	fip = (struct FsInfo_4hasam *)malloc((fscount + 1) * sizeof (*fip));
	if (fip == NULL) {
		/* 22602 */
		scds_syslog(LOG_ERR, "Out of Memory");
		return (NULL);
	}
	bzero((char *)fip, (fscount + 1) * sizeof (*fip));

	/*
	 * Initialize the FsInfo_4hasam array with the FS mount points
	 * and "specials" (AKA "devices" AKA "family set names").
	 */
	for (i = 0; i < fscount; i++) {
		fip[i].fi_fs = strdup(rgp->rg_mntpts->str_array[i]);
		if (fip[i].fi_fs == NULL) {
			/* 22602 */
			scds_syslog(LOG_ERR, "Out of Memory");
			goto err;
		}

		/*
		 * Check if the FS is actually mounted and get it's
		 * mount point
		 */
		fip[i].fi_mntpt = get_mntpt_from_fsname(fip[i].fi_fs);
		if (fip[i].fi_mntpt == NULL) {
			dprintf(stderr, "%s: Error getting mount point\n",
			    fip->fi_fs);
			scds_syslog(LOG_ERR, "%s: Error getting mount point",
			    fip->fi_fs);
			goto err;
		}

		dprintf(stderr, "FSname = %s, Mountpt = %s\n",
		    fip->fi_fs, fip[i].fi_mntpt);

		/*
		 * Now check QFS is declared as "samfs" for
		 * FS type and has "shared" mount option in vfstab
		 */
		if (!check_vfstab(fip[i].fi_mntpt, "samfs", "shared")) {
			dprintf(stderr, "%s: Error FS type is not samfs\n",
			    fip[i].fi_mntpt);
			scds_syslog(LOG_ERR, "%s: Error FS type is not samfs",
			    fip[i].fi_mntpt);
			goto err;
		}
	}

	/*
	 * Gather a list of mount points from mnttab and vfstab
	 * to validate the mount points for catalog, stager and
	 * disk archive directory resources
	 */
	if (get_all_mount_points() < 0) {
		scds_syslog(LOG_ERR,
		    "Unable to get mount points from mnttab and vfstab");
		dprintf(stderr,
		    "Unable to get mount points from mnttab and vfstab");
		goto err;
	}

	return (fip);

err:
	FreeFileSystemInfo_4hasam(fip);
	free_all_mount_points();
	return (NULL);
}


/*
 * Check if the given filesystem mount point with required
 * FS type and mount option exist in vfstab
 *
 * Return: success = B_TRUE [or] failure = B_FALSE
 */
boolean_t
check_vfstab(
	char *fs_mntpt,
	char *fs_type,
	char *option)
{
	FILE *fp;
	struct vfstab vp;
	boolean_t found;

	/* Check for this entry in /etc/vfstab */
	fp = fopen(VFSTAB, "r");
	if (fp == NULL) {
		/* 22603 */
		scds_syslog(LOG_ERR,
		    "Unable to open /etc/vfstab: %s",
		    strerror(errno));
		dprintf(stderr,
		    "Unable to open /etc/vfstab: %s\n",
		    strerror(errno));
		return (B_FALSE);
	}

	found = B_FALSE;
	while (getvfsent(fp, &vp) == 0) {
		if (vp.vfs_mountp == NULL)
			continue;
		if (streq(fs_mntpt, vp.vfs_mountp)) {
			/*
			 * Found the entry, make sure it looks good to us
			 * Basic Test: fstype of type samfs
			 * Skip other fstypes
			 */

			if (streq(fs_type, "samfs")) {
				if (!streq(vp.vfs_fstype, "samfs")) {
					/* 22604 */
					scds_syslog(LOG_ERR,
					    "FStype %s must be samfs not %s.",
					    fs_mntpt, vp.vfs_fstype);
					dprintf(stderr,
					    "FStype %s must be samfs not %s\n",
					    fs_mntpt, vp.vfs_fstype);
					(void) fclose(fp);
					return (B_FALSE);
				}

				if (strstr(vp.vfs_mntopts, option) == NULL) {
					scds_syslog(LOG_ERR,
					    "Set \"shared\" mount opt for %s",
					    fs_mntpt);
					dprintf(stderr,
					    "Set \"shared\" mount opt for %s\n",
					    fs_mntpt);
					(void) fclose(fp);
					return (B_FALSE);
				}
				/*
				 * vfstab entry looks good for samfs
				 * We're done
				 */
				found = B_TRUE;
				break;

			} else if (streq(fs_type, "other") &&
			    streq(option, "global")) {
				/*
				 * For non-samfs filesystems, check if
				 * global is one of the mount options.
				 * This check is required for catalog,
				 * stager etc but NOT required for samfs !
				 */
				if (strstr(vp.vfs_mntopts, option) == NULL) {
					scds_syslog(LOG_ERR,
					    "Set \"global\" mount opt for %s",
					    fs_mntpt);
					dprintf(stderr,
					    "Set \"global\" mount opt for %s\n",
					    fs_mntpt);
					(void) fclose(fp);
					return (B_FALSE);
				}
				/* vfstab entry looks good.  We're done. */
				found = B_TRUE;
				break;
			}
		}
	}

	(void) fclose(fp);
	return (found);
}

/*
 * Routines to handle fork()ing for agent component actions.
 * These fork process routines are taken from scqfs agent to
 * avoid duplication of effort.
 *
 * Agent components may need to execute actions for each of
 * possibly several FSes.  These routines provide a convenient
 * mechanism.
 */

/*
 * fork_proc_4hasam - call the given routine for each FS in fip
 * after forking.
 */
static int
fork_proc_4hasam(
	int ac,
	char **av,
	struct FsInfo_4hasam *fip,
	int (*fn)(int, char **, struct FsInfo_4hasam *))
{
	pid_t pid;
	int rc;

	switch (pid = fork()) {
		case (pid_t)-1:			/* error */
			/* 22641 */
			scds_syslog(LOG_ERR,
			    "%s: Fork failed.  Error: %s",
			    fip->fi_mntpt, strerror(errno));
			return (-1);

		case 0:					/* child */
			rc = (*fn)(ac, av, fip);
			exit(rc);
			/*NOTREACHED*/

		default:				/* parent */
			fip->fi_pid = pid;
	}
	return (0);
}


/*
 * ForkProcs_4hasam
 *
 * For each filesystem in the resource group, we fork off a
 * child, and call the given procedure (in the child).  The
 * procedure should exit (not return), and this routine collects
 * all the exit statuses, and returns.  Any children that exit
 * with signals are noted.
 *
 * Return values:
 *	0 - all forked children exited w/ status 0.
 *  1 - one or more children exited w/ non-zero status.
 *	2 - one or more children exited w/ a signal-received status.
 *	3 - fork() failed
 */
int
ForkProcs_4hasam(
	int ac,
	char **av,
	struct FsInfo_4hasam *fip,
	int (*fn)(int, char **, struct FsInfo_4hasam *))
{
	int nfs, i, rc;

	/*
	 * We've got the FS names, their mount points, and
	 * the list of nodes that can serve as MDSes.  Now
	 * we need to fork/thread off procs for each FS.
	 *
	 * Each forked copy calls (*fn)() with all our arguments.
	 * Once forked off, we wait around for all of them to
	 * return, noting discrepancies as we wait.
	 */
	for (nfs = 0; fip[nfs].fi_mntpt != NULL; ) {
		nfs++;
	}
	if (nfs == 1) {
		/*
		 * Don't bother forking if there's only one filesystem
		 */
		return (*fn)(ac, av, &fip[0]);
	}

	for (i = 0; i < nfs; i++) {
		if (fork_proc_4hasam(ac, av, &fip[i], fn) < 0) {
			return (-3);	/* error logged by fork_proc */
		}
	}

	rc = 0;
	for (;;) {
		pid_t pid;
		int status, prc;

		pid = wait(&status);
		if (pid == (pid_t)-1) {
			if (errno == EAGAIN || errno == EINTR) {
				continue;
			}
			if (errno != ECHILD) {
				/* 22642 */
				scds_syslog(LOG_ERR,
				    "Wait for child returned error: %s",
				    strerror(errno));
			}
			break;
		}
		prc = 0;
		if (WIFSIGNALED(status)) {
			prc = -2;
		} else if (WEXITSTATUS(status)) {
			prc = -1;
		}
		rc = MIN(prc, rc);

		for (i = 0; i < nfs; i++) {
			if (pid == fip[i].fi_pid) {
				fip[i].fi_status = status;
				if (prc == -2) {
					/* 22685 */
					scds_syslog(LOG_ERR,
					    "%s: Exited w/ sig <%d>%s.",
					    fip[i].fi_fs, WTERMSIG(status),
					    WCOREDUMP(status));
				}
				break;
			}
		}
		if (fip[i].fi_mntpt == NULL) {
			scds_syslog(LOG_ERR,
			    "Unknown child proc pid %d, status=%#x",
			    pid, status);
		}
	}
	return (rc);
}


/*
 * Wrapper to core routine in SamQfs to get filesystem equipment ord
 * for a given filesystem name
 *
 * Return: success = eq ord of FS [or] failure = -1
 */
int
get_fs_eq(char *fs_name)
{
	int numfs, i;
	int fs_eq = -1;
	struct sam_fs_status *fsarray;

	if ((numfs = GetFsStatus(&fsarray)) == -1) {
		return (-1);
	}

	for (i = 0; i < numfs; i++) {
		if (strcmp((fsarray + i)->fs_name, fs_name) == 0) {
			/* Found equipment ord for the FS */
			fs_eq = (fsarray + i)->fs_eq;
			break;
		}
	}
	free(fsarray);
	return (fs_eq);
}


/*
 * Wrapper to core routine in SamQfs to get filesystem mount point
 * for a given filesystem name
 *
 * Return: success = mount point of FS [or] failure = NULL
 */
char *
get_fs_mntpt(char *fs_name)
{
	int numfs, i;
	char *fs_mntpt = NULL;
	struct sam_fs_status *fsarray;

	if ((numfs = GetFsStatus(&fsarray)) == -1) {
		return (NULL);
	}

	for (i = 0; i < numfs; i++) {
		if (strcmp((fsarray + i)->fs_name, fs_name) == 0) {
			/* Found mount point */
			fs_mntpt = strdup((fsarray + i)->fs_mnt_point);
			break;
		}
	}
	if (fs_mntpt == NULL) {
		/* 22602 */
		scds_syslog(LOG_ERR, "Out of memory");
		dprintf(stderr, catgets(catfd, SET, 22602, "Out of memory"));
	}
	return (fs_mntpt);
}


/*
 * This function starts the fault monitor for a SAM resource.
 * This is done by starting the probe under PMF. The PMF tag
 * is derived as RG-name,RS-name.mon. The restart option of PMF
 * is used but not the "infinite restart". Instead
 * interval/retry_time is obtained from the RTR file.
 * It starts the daemon "hasam_probe" in the RT base
 * directory.
 *
 * Return: success = 0 [or] failure = 1
 */
int
hasam_mon_start(struct RgInfo_4hasam *rgp)
{
	if (scds_pmf_start(rgp->rg_scdsh, SCDS_PMF_TYPE_MON,
	    SCDS_PMF_SINGLE_INSTANCE, "hasam_probe", 0) != SCHA_ERR_NOERR) {
		/* 22612 */
		scds_syslog(LOG_ERR, "Failed to start fault monitor.");
		return (1);
	}

	/* 22618 */
	scds_syslog(LOG_INFO, "Started the fault monitor.");
	return (0);
}


/*
 * This function stops the fault monitor for a SAM resource.
 * It uses the DSDL API scds_pmf_stop() to do that.
 * Failure here would land the resource in STOP_FAILED
 * state.
 * The monitor is killed with signal SIGKILL and the
 * last argument of "-1" denotes the timeout (infinite).
 *
 * Return: success = 0 [or] failure = 1
 */
int
hasam_mon_stop(struct RgInfo_4hasam *rgp)
{
	if (scds_pmf_stop(rgp->rg_scdsh, SCDS_PMF_TYPE_MON,
	    SCDS_PMF_SINGLE_INSTANCE, SIGKILL, -1) != SCHA_ERR_NOERR) {
		/* 22619 */
		scds_syslog(LOG_ERR, "Failed to stop fault monitor.");
		return (1);
	}

	/* 22620 */
	scds_syslog(LOG_INFO, "Stopped the fault monitor.");
	return (0);
}


/*
 * Create a run file which shows HA-SAM is running on
 * a node in the cluster. This file is used by stk during
 * forced dismount
 *
 * Return: success = B_TRUE [or] failure = B_FALSE
 */
boolean_t
make_run_file(void)
{
	int tmp_fd;
	int retries, status = 0;
	struct stat sb;
	boolean_t file_created = B_FALSE;

	for (retries = 0; retries < 2; retries++) {
		if (stat(HASAM_RUN_FILE, &sb) != 0) {
			/* hasam run file doesn't exist, create it. */
			tmp_fd = open(HASAM_RUN_FILE, (O_CREAT|O_RDWR), 0600);
			if (tmp_fd < 0) {
				scds_syslog(LOG_ERR,
				    "HA-SAM run file create failed %d (%s)",
				    errno, HASAM_RUN_FILE);
				dprintf(stderr,
				    "HA-SAM run file create failed (%s)\n",
				    HASAM_RUN_FILE);
				file_created = B_FALSE;
			} else {
				dprintf(stderr, "Created run file (%s)\n",
				    HASAM_RUN_FILE);
				file_created = B_TRUE;
				close(tmp_fd);
				break;
			}
		} else {
			dprintf(stderr, "HA-SAM run file exists(%s)\n",
			    HASAM_RUN_FILE);
			file_created = B_TRUE;
			break;
		}
		unlink(HASAM_RUN_FILE);
		close(tmp_fd);
	}
	return (file_created);
}


/*
 * Delete the HA-SAM run file which shows HA-SAM is offline
 * on the node.
 */
void
delete_run_file(void)
{
	struct stat sb;

	if (stat(HASAM_RUN_FILE, &sb) == 0) {
		dprintf(stderr, "Removing HA-SAM run file (%s)\n",
		    HASAM_RUN_FILE);
		unlink(HASAM_RUN_FILE);
	}
}
