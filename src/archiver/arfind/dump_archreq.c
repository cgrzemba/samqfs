/*
 * compile:
 * gcc -m64 -I ../../../include -I../include -I../../../include/aml -g -o dump_archreq dump_archreq.c -L../lib/obj/SunOS_omnios-r151054_amd64 -larch -L../../../lib/obj/SunOS_omnios-r151054_amd64 -lsamut
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>

#define DEC_INIT

#include "sam/types.h"
#include "sam/lib.h"
#include "sam/names.h"

#include "aml/archiver.h"
#include "sam/fs/ino.h"
#include "arfind.h"
#include "aml/shm.h"

shm_alloc_t master_shm;
struct ArchLib *ArchLibTable;
struct ArchiverdState *AdState;
struct ArfindState *AfState;

void*
MapInFile(
	char *file_name,
	int mode,
	size_t *len)
{
	int prot;
	int fd;
	int err;
	struct stat st;
	void *mp = NULL;
	int retry;

	prot = (O_RDONLY == mode) ? PROT_READ : PROT_READ | PROT_WRITE;

	retry = 3;
	fd = -1;
	while (fd == -1 && retry-- > 0) {
		fd = open(file_name, mode);
		if (fd == -1) {
			fprintf(stderr, "MapInFile open failed: '%s', errno %d\n",
			    file_name, errno);
			if (errno == EACCES)
				exit(errno);
			sleep(5);
		}
	}

	if (fd == -1) {
		if (len != NULL) {
			*len = 0;
		}
		return (NULL);
	}

	err = fstat(fd, &st);
	if (err != 0) {
		exit(errno);
	}

	retry = 3;
	mp = MAP_FAILED;
	while (mp == MAP_FAILED && retry-- > 0) {
		mp = mmap(NULL, st.st_size, prot, MAP_SHARED, fd, 0);
		if (mp == MAP_FAILED) {
			fprintf(stderr, "MapInFile mmap failed: '%s', errno: %d\n",
			    file_name, errno);
			if (errno == EACCES)
				exit(errno);
			sleep(5);
		}
	}

	if (mp == MAP_FAILED) {
		exit(errno);
	}

	(void) close(fd);

	if (len != NULL) {
		*len = st.st_size;
	}

	return (mp);
}

static struct ArchReq *
_ArchReqAttach(
    	char *basedir,
        char *arname,   /* ArchReq name */
        int mode)       /* O_RDONLY = read only, read/write otherwise */
{
        static char name[512];
        struct ArchReq *ar;

        snprintf(name, sizeof (name), "%s/%s", basedir, arname);
        ar = MapFileAttach(name, ARCHREQ_MAGIC, mode);
        if (ar != NULL && ar->ArVersion != ARCHREQ_VERSION) {
                (void) MapFileDetach(ar);
                errno = EINVAL;
                ar = NULL;
        }
        return (ar);
}

int
main(int argc, char *argv[])
{
	int copy=0, phe = 1, cpn = 0, debug = 0, opt;
	char *datapath = NULL;
	char *fsname = NULL; /* filesystem name */
	size_t size = 0;
	int entries = 0;
	int cnt = 0;
	struct stat buf;
	struct dirent *dirent;
	DIR *dirp;
	struct ArchReq *ar;
	char sdtdatapath[255];

	while((opt = getopt(argc, argv, "Hp:c:df:")) != -1)
	{
		switch(opt)
		{
		    case 'H':  /* print no header */
			phe = 0;
			break;
		    case 'p':
			datapath = optarg;
			break;
		    case 'c': 
			int c = atoi(optarg);
			if (c > 0 && c < 5) copy = c;
			break;
		    case 'd':  /* debug */
			debug = 1;
			break;
		    case 'f':
			fsname = optarg;
			break;
		}
	}
	if (datapath == NULL) {
                struct stat buf;

                sprintf(sdtdatapath, "%s/archiver/%s/ArchReq", SAM_VARIABLE_PATH, fsname);
        } else {
		if (datapath[strlen(datapath)-1] != '/')
			strcat(datapath, "/");
                sprintf(sdtdatapath, "%s/archiver/%s/ArchReq", datapath, fsname);
	}

        if (stat(sdtdatapath, &buf) != 0) {
        	fprintf(stderr, "could not open %s, invalid path\n", sdtdatapath);
        	exit (2);
        }

	if ((dirp = opendir(sdtdatapath)) == NULL) {
        	fprintf(stderr, "could not open %s, invalid path\n", sdtdatapath);
        	exit (2);
        }

	if (phe)
		printf("idx seq copy files space vsn\n");
	while ((dirent = readdir(dirp)) != NULL) {
		if (*dirent->d_name == '.') continue;
		ar = _ArchReqAttach(sdtdatapath, dirent->d_name, O_RDONLY);
		if (ar == NULL) {
			fprintf(stderr, "Can't attach %s, errno %d\n", dirent->d_name, errno);
			continue;
		}

		printf("%d %d %d %d %d %s\n\t%s\n\t%s\n\t%s\n\t%s\n",
		    ar->ArAtIndex,
		    ar->ArSeqnum, ar->ArCopy+1, ar->ArFiles, ar->ArSpace, ar->ArCpi[0].CiVsn,
		    ar->ArCpi[0].CiOprmsg, 
		    ar->ArCpi[1].CiOprmsg,
		    ar->ArCpi[2].CiOprmsg,
		    ar->ArCpi[3].CiOprmsg);

		(void) MapFileDetach(ar);
	}
	closedir(dirp);
	return (0);
}
