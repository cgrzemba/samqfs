#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "sam/types.h"
#include "copy_defs.h"
#include "sam/names.h"
#include "aml/stager_defs.h"
#include "stager_config.h"
#include "stager_lib.h"

#define CPFN SAM_VARIABLE_PATH"/stager/"STAGE_REQS_FILENAME

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
	size_t size;
	int j,i, entries;
	int phe = 1, act = 0, cpn = 0, opt;

	while((opt = getopt(argc, argv, "Hpc")) != -1)
	{
		switch(opt)
		{
		    case 'H':  /* print no header */
			phe = 0;
			break;
		    case 'p':  /* print only Copy with pax tar hdr */
			act = 1;
			break;
		    case 'c':  /* print also copy number in 0 */
			cpn = 1;
			break;
		}
	}
	stageReqs = (FileInfo_t *)MapInFile(CPFN , O_RDWR, &size);

	if (stageReqs == NULL) {
		fprintf(stderr, "Cannot map in file '%s' errno: %d\n",
		    CPFN , errno);
		return (-1);
	}

	entries = size / sizeof (FileInfo_t);

	if (phe)
		printf("ino       feq cp dio    pid flg err  teq  sort   next\n");
	for (i = 0; i < entries; i++) {
		if(stageReqs[i].fseq != 0) {
			int pcp = cpn ? 1 : (stageReqs[i].copy !=0);
			int pac = act ? stageReqs[i].flags & STAGE_COPY_PAXHDR : pcp;
			if (pac)
				printf("%8d %4d  %1d %#2x %6d %#2x %2d  %2d %6d %6d\n",
			stageReqs[i].id,
			stageReqs[i].fseq,
			stageReqs[i].copy,
			stageReqs[i].directio,
			stageReqs[i].context,
			stageReqs[i].flags,
			stageReqs[i].error,
			stageReqs[i].eq,
			stageReqs[i].sort,
			stageReqs[i].next);
		}
	}

	munmap(stageReqs, size);
	return (0);
}
