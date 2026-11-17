#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <string.h>

#define DEC_INIT

#include "sam/types.h"
#include "sam/names.h"
#include "sam/lib.h"

#include "aml/archiver.h"
#include "aml/archset.h"
#include "aml/archreq.h"
#include "common.h"

/*
 * idList -  List of the location of all inode ids in ArchReqs.
 * Each file in an ArchReq is referenced by an entry in the idList.
 * An idList entry "points" to a sam_id_t struct in the ArchReq using the
 * index of the ArchReq in the archReqTable.
 *
 * The idList section is sorted by inode number.  It is searched using a
 * binary search.
 */

/* Inode reference. */
struct IdRef {
        uint_t  IrAtIndex;
        uint_t  IrOffset;
};

static struct IdList {
        MappedFile_t Il;
        size_t  IlSize;                         /* Size of all entries */
        int     IlCount;                        /* Number of entries */
        struct IdRef IlEntry[1];
} *idList[4];                   /* One for each copy */

#define IDLIST "idlist_copy"
#define IDLIST_MAGIC 012203101104
#define IDLIST_INCR 1000
#define ID_LOC(ir) ((void *) \
        ((char *)(void *)archReqTable[(ir)->IrAtIndex] + (ir)->IrOffset))

#define CPFN SAM_VARIABLE_PATH
char itdatapath[PATH_MAX] = { CPFN };
char stdatapath[PATH_MAX] = { CPFN };

shm_alloc_t master_shm;
struct ArchLib *ArchLibTable;
struct ArchiverdState *AdState;
struct ArfindState *AfState;
size_t VsnListSize;
char * program_name;
int VsnPoolTable;
int VsnPoolSize;
int ArchSetTable;
int ArchSetNumof;
void* ArchSetFile;
int VsnExpSize;
int VsnExpTable;
int VsnListTable;
void *MediaParams;

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
	struct IdList *idlist = NULL;
	struct IdList *il = NULL;
	size_t size = 0;
	int entries = 0;
	int cnt = 0;
	struct ArchReq *archrequests;
	struct dirent *dirent;
	DIR *dirp;
	struct ArchReq *ar;

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
	if (datapath != NULL) {
                sprintf(itdatapath, "%s/archiver/%s/%s%1d", datapath, fsname, IDLIST, copy);
		sprintf(stdatapath, "%s/archiver/%s/ArchReq", datapath, fsname);
	} else {
                sprintf(itdatapath, "%s/archiver/%s/%s%1d", SAM_VARIABLE_PATH, fsname, IDLIST, copy);
		sprintf(stdatapath, "%s/archiver/%s/ArchReq", SAM_VARIABLE_PATH, fsname);
        }
        struct stat buf;
	if (stat(itdatapath, &buf) != 0) {
		fprintf(stderr, "could not open %s, invalid path\n", itdatapath);
		exit (2);
	}

	il = (struct IdList *)MapInFile(itdatapath , O_RDONLY, &size);

	if (il == NULL) {
		fprintf(stderr, "Cannot map in file '%s' errno: %d\n",
		    itdatapath , errno);
		return (-1);
	}

	if (debug) {
		printf("copy=%d count=%d size=%d\n", copy, il->IlCount, size);
	}

	if ((dirp = opendir(stdatapath)) == NULL) {
                fprintf(stderr, "could not open %s, invalid path\n", stdatapath);
                exit (2);
        }
	int fcnt=0;
	while ((dirent = readdir(dirp)) != NULL) {
                if (*dirent->d_name == '.') continue;
		fcnt++;
	}
	closedir(dirp);
	archrequests = (struct ArchReq *)malloc(sizeof(struct ArchReq) * fcnt);
	memset(archrequests, 0, sizeof(struct ArchReq) * fcnt);
	dirp = opendir(stdatapath);
	int idx=0;
	while ((dirent = readdir(dirp)) != NULL) {
                if (*dirent->d_name == '.') continue;
                ar = _ArchReqAttach(stdatapath, dirent->d_name, O_RDONLY);
                if (ar == NULL) {
                        fprintf(stderr, "Can't attach %s, errno %d\n", dirent->d_name, errno);
                        continue;
                }
		archrequests[idx].ArAtIndex = ar->ArAtIndex;
		archrequests[idx].ArSeqnum = ar->ArSeqnum;
		archrequests[idx].ArAsn = ar->ArAsn;
		archrequests[idx].ArState = ar->ArState;
		archrequests[idx].ArCopy = ar->ArCopy;
		archrequests[idx].ArFiles = ar->ArFiles;
		archrequests[idx].ArSpace = ar->ArSpace;
		archrequests[idx].ArCount = ar->ArCount;
		strncpy(archrequests[idx].ArAsname, ar->ArAsname, sizeof(set_name_t));
		strncpy(archrequests[idx].ArFsname, ar->ArFsname, sizeof(uname_t));
		strncpy(archrequests[idx].ArCpi[0].CiVsn, ar->ArCpi[0].CiVsn, 32);
		strncpy(archrequests[idx].ArCpi[0].CiOprmsg, ar->ArCpi[0].CiOprmsg, OPRMSG_SIZE);
		idx++;
		MapFileDetach(ar);
	}
	closedir(dirp);



	if (phe)
		printf("idx off/ino seqnum copy files space vsn oprmsg\n");
	/* il = &idlist[copy]; */
	for (int i = 0; i < il->IlCount; i++) {
		struct IdRef *ir;

		ir = &il->IlEntry[i];
		printf("%2d %8d ", ir->IrAtIndex, ir->IrOffset);
		if (ir->IrAtIndex == 0) {
			printf("no archreq\n");
			continue;
		}

		int j;
		for (j = 0; j < fcnt; j++) {
			if (archrequests[j].ArAtIndex == ir->IrAtIndex)  {
				printf("%6d %d %6d %lld %s %s",
				    archrequests[j].ArSeqnum,
				    archrequests[j].ArCopy+1,
				    archrequests[j].ArFiles,
				    archrequests[j].ArSpace,
				    archrequests[j].ArCpi[0].CiVsn,
				    archrequests[j].ArCpi[0].CiOprmsg
				    );
				cnt++;
				break;
			}
		}
		if (j == fcnt)
			printf("no archreq");
		printf("\n");
	}

	munmap(idlist, size);
	if (phe)
		printf("num entries=%d\n", cnt);
	free(archrequests);
	return (0);
}
