#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>

#define DEC_INIT
#include "sam/types.h"
#include "sam/names.h"
#include <sam/nl_samfs.h>
// #include <aml/archset.h>

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

struct ScanList {
        MappedFile_t Sl;
        int     SlVersion;              /* Version */
        size_t  SlSize;                 /* Size of all entries */
        int     SlCount;                /* Number of entries */
        int     SlFree;                 /* Number of freed entries */
        ExamMethod_t SlExamine;         /* For showqueue */
        struct ScanListEntry {
                sam_id_t SeId;          /* Inode number */
                sam_time_t SeTime;      /* Time to process entry */
                int      SeAsn;         /* Archive Set number */
                ushort_t SeCopies;  /* Archive copy bits expected during scan */
                ushort_t SeFlags;
        } SlEntry[1];
};

static struct ScanList *scanList;       /* Scan list */

static struct IdList {
        MappedFile_t Il;
        size_t  IlSize;                         /* Size of all entries */
        int     IlCount;                        /* Number of entries */
        uint_t	IlEntry[1];
} *idList;                   /* One for each copy */

#define IDLIST "idList_scan"
#define ID_LOC(ir) ((void *)((char *)scanList + ir))

#define CPFN SAM_VARIABLE_PATH
#define SCANLIST "Scanlist"
#define SCANLIST_MAGIC 0122072714


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

int
main(int argc, char *argv[])
{
	int phe = 1, cpn = 0, debug = 0, opt;
	char *datapath = NULL;
	char *fsname = NULL; /* filesystem name */
	struct IdList *idlist = NULL;
	struct IdList *il = NULL;
	size_t size = 0;
	int entries = 0;
	int cnt = 0;

	while((opt = getopt(argc, argv, "Hp:df:")) != -1)
	{
		switch(opt)
		{
		    case 'H':  /* print no header */
			phe = 0;
			break;
		    case 'p':
			datapath = optarg;
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
		datapath = stdatapath;
	}
        sprintf(itdatapath, "%s/archiver/%s/%s", datapath, fsname, IDLIST);
        sprintf(stdatapath, "%s/archiver/%s/%s", datapath, fsname, SCANLIST);
        struct stat buf;
        if (stat(itdatapath, &buf) != 0) {
        	fprintf(stderr, "could not open %s, invalid path\n", itdatapath);
		exit (2);
	}
        if (stat(stdatapath, &buf) != 0) {
		fprintf(stderr, "could not open %s, invalid path\n", stdatapath);
		exit (2);
        }

	il = (struct IdList *)MapInFile(itdatapath , O_RDONLY, &size);
	scanList = ArMapFileAttach(stdatapath, SCANLIST_MAGIC, O_RDONLY);

	if (il == NULL) {
		fprintf(stderr, "Cannot map in file '%s' errno: %d\n",
		    stdatapath , errno);
		return (-1);
	}

	if (debug) {
		printf("idList count=%d size=%d\n", il->IlCount, size);
		printf("scanList count=%d size=%d free=%d mode=%d\n",
		    scanList->SlCount, scanList->SlSize, scanList->SlFree , scanList->SlExamine );
	}

	if (phe)
		printf("idx id flag time asn copies\n");
	/* il = &idlist[copy]; */
	for (int i = 0; i < il->IlCount; i++) {
		struct ScanListEntry *se;

		uint_t ir = il->IlEntry[i];
		se = (struct ScanListEntry *)ID_LOC(ir);
		printf("%d %d.%d %#x %d %d %#x\n",
		    ir, se->SeId.ino, se->SeId.gen, se->SeFlags, se->SeTime, se->SeAsn, se->SeCopies );
		cnt++;
	}

	munmap(idlist, size);
	printf("num entries=%d\n", cnt);
	return (0);
}
