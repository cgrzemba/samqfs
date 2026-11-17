#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include "sam/types.h"
#include "sam/names.h"

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
char stdatapath[PATH_MAX] = { CPFN };


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
                sprintf(stdatapath, "%s/archiver/%s/%s%1d", datapath, fsname, IDLIST, copy);
	} else {
                sprintf(stdatapath, "%s/archiver/%s/%s%1d", SAM_VARIABLE_PATH, fsname, IDLIST, copy);
        }
        struct stat buf;
	if (stat(stdatapath, &buf) != 0) {
		fprintf(stderr, "could not open %s, invalid path\n", stdatapath);
		exit (2);
	}

	il = (struct IdList *)MapInFile(stdatapath , O_RDONLY, &size);

	if (il == NULL) {
		fprintf(stderr, "Cannot map in file '%s' errno: %d\n",
		    stdatapath , errno);
		return (-1);
	}

	if (debug) {
		printf("copy=%d count=%d size=%d\n", copy, il->IlCount, size);
	}

	if (phe)
		printf("idx off/ino\n");
	/* il = &idlist[copy]; */
	for (int i = 0; i < il->IlCount; i++) {
		struct IdRef *ir;

		ir = &il->IlEntry[i];
		printf("%d %8d\n",
		    ir->IrAtIndex,
		    ir->IrOffset);
		cnt++;
	}

	munmap(idlist, size);
	printf("num entries=%d\n", cnt);
	return (0);
}
