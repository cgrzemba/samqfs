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
#include "copy_defs.h"
#include "sam/names.h"
#include "aml/stager_defs.h"
#include "stager_config.h"
#include "stager_lib.h"

#define CPFN SAM_VARIABLE_PATH"/stager/"STAGE_REQS_FILENAME
char stdatapath[PATH_MAX] = { CPFN };

FileInfo_t *stageReqs;

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
	int numlibs = 0;
	int xnumlibs = 0;
	int numDrives = 8;
	int id = 0;
	int cnt = 0;
	size_t size;
	int j,i, entries;
	int phe = 1, act = 0, cpn = 0, debug = 0, opt;
	char *datapath = NULL;

	while((opt = getopt(argc, argv, "HPp:cdf:")) != -1)
	{
		switch(opt)
		{
		    case 'H':  /* print no header */
			phe = 0;
			break;
		    case 'P':  /* print only Copy with pax tar hdr */
			act = 1;
			break;
		    case 'p':
			datapath = optarg;
			break;
		    case 'c':  /* print also copy number in 0 */
			cpn = 1;
			break;
		    case 'd':  /* debug */
			debug = 1;
			break;
		    case 'f':  /* first */
			id = atoi(optarg);
			break;
		}
	}
	if (datapath != NULL) {
                struct stat buf;

                sprintf(stdatapath, "%s/"STAGE_REQS_FILENAME, datapath);
                if (stat(stdatapath, &buf) != 0) {
                        fprintf(stderr, "could not open %s, invalid path\n", datapath);
                        exit (2);
                }
        }

	stageReqs = (FileInfo_t *)MapInFile(stdatapath , O_RDONLY, &size);

	if (stageReqs == NULL) {
		fprintf(stderr, "Cannot map in file '%s' errno: %d\n",
		    stdatapath , errno);
		return (-1);
	}

	entries = size / sizeof (FileInfo_t);
	if (debug) {
		printf("size=%d entries=%d\n", size, entries);
	}

	if (phe)
		printf("ino       feq cp dio    pid flg err  teq  sort   next\n");
	for (i = 0; i < entries; i++) {
		FileInfo_t *entry;

		entry = &stageReqs[id];
		if (entry->next == 0 || entry->magic != 0x53746752) {
			id = i+1;
			continue;
		}
		printf("%d %4d  %1d %s %x %s %x %s %x %s %x %#2x %6d %#2x %2d  %2d %2d %6d\n",
		    entry->id.ino,
		    entry->fseq,
		    entry->copy,
		    entry->ar[0].section.vsn,
		    entry->ar[0].section.position,
		    entry->ar[1].section.vsn,
		    entry->ar[1].section.position,
		    entry->ar[2].section.vsn,
		    entry->ar[2].section.position,
		    entry->ar[3].section.vsn,
		    entry->ar[3].section.position,
		    entry->directio,
		    entry->context,
		    entry->flags,
		    entry->error,
		    entry->retry,
		    entry->eq,
		    entry->sort);
		cnt++;
		id = entry->next;
		if (id == -1)
			break;
		if (stageReqs[id].magic != 0x53746752)
			break;
	}

	munmap(stageReqs, size);
	printf("num entries=%d\n", cnt);
	return (0);
}
