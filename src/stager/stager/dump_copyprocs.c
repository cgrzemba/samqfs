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

#define CPFN SAM_VARIABLE_PATH"/stager/"COPY_PROCS_FILENAME

CopyInstanceList_t *CopyInstanceList;

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
main()
{
	int numlibs = 0;
	int xnumlibs = 0;
	int numDrives = 8;
	size_t size, len;
	int j,i;

	CopyInstanceList = (CopyInstanceList_t *)MapInFile(CPFN , O_RDWR, &len);

	if (CopyInstanceList == NULL) {
		fprintf(stderr, "Cannot map in file '%s' errno: %d\n",
		    CPFN , errno);
		return (-1);
	}

	size = sizeof (CopyInstanceList_t);
	size += (numDrives - 1) * sizeof (CopyInstanceInfo_t);

	if (CopyInstanceList->cl_magic != COPY_INSTANCE_LIST_MAGIC      ||
	    CopyInstanceList->cl_version != COPY_INSTANCE_LIST_VERSION) {

		fprintf(stderr, "Invalid %s found.\n",
		    CPFN);
		CopyInstanceList = NULL;
		return (-1);
	}

	numDrives = CopyInstanceList->cl_entries;
	/*
	 * Mapped in CopyInstanceList has a valid header, then validate the
	 * library config against current library table.
	 */

	j = -1;
	printf("lib  leq  teq    pid vsn first last shut flags busy idlet\n");
	for (i = 0; i < CopyInstanceList->cl_entries; i++) {
		if (CopyInstanceList->cl_data[i].ci_eq == 0)
			continue;

		printf("%3d %4d %4d %6d %s %p %p %#x %#x %#x %d\n",
		    CopyInstanceList->cl_data[i].ci_lib,
		    CopyInstanceList->cl_data[i].ci_eq,
		    CopyInstanceList->cl_data[i].ci_drive,
		    CopyInstanceList->cl_data[i].ci_pid,
		    CopyInstanceList->cl_data[i].ci_vsn,
		    CopyInstanceList->cl_data[i].ci_first,
		    CopyInstanceList->cl_data[i].ci_last,
		    CopyInstanceList->cl_data[i].ci_shutdown,
		    CopyInstanceList->cl_data[i].ci_flags,
		    CopyInstanceList->cl_data[i].ci_busy,
		    CopyInstanceList->cl_data[i].ci_idletime);
	}
	munmap(CopyInstanceList, len);

	return (0);
}
