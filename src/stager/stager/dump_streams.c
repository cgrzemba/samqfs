#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <strings.h>
#include <time.h>
#include "sam/types.h"
#include "copy_defs.h"
#include "sam/names.h"
#include "aml/stager_defs.h"
#include "stager_config.h"
#include "stager_lib.h"

#define STDIR SAM_VARIABLE_PATH"/stager/streams/"

int debug = 0;
StreamInfo_t *StreamInfo;

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

#define idx(i) ((LEN_TAPE_VSN+1)*i)
int
getVsns(char **vsnlist)
{
	DIR *dirp;
	struct dirent *dp;
	int i=0, j=0;
	char *vp;

	vp = (char*) malloc(32 * LEN_TAPE_VSN+1);
	memset(vp, 0, 32 * LEN_TAPE_VSN+1);
	*vsnlist = vp;

	dirp = opendir(STDIR);
	if (dirp == NULL) {
		fprintf(stderr, "could not open %s\n", STDIR);
		return (0);
	}
	while ((dp = readdir(dirp)) != NULL) {
		if (strcspn(dp->d_name, ".") == LEN_TAPE_VSN) {
			int new=1;
			for (int k=0; k<j; k++) {
				if (strncmp(dp->d_name, vp + idx(k), LEN_TAPE_VSN) == 0)
					new = 0;
			}
			if (new) {
				strncpy(vp + idx(j++), dp->d_name, LEN_TAPE_VSN);
			}
		}
	}
	(void)closedir(dirp);

	return (j);
}

int
setSeqnums(const char *vsn, int seqnums[])
{
	DIR *dirp;
	struct dirent *dp;
	int i=0;

	dirp = opendir(STDIR);
	if (dirp == NULL) {
		fprintf(stderr, "could not open %s\n", STDIR);
		return (0);
	}
	while ((dp = readdir(dirp)) != NULL) {
		if (strncmp(dp->d_name, vsn, LEN_TAPE_VSN) == 0) {
			if (debug) fprintf(stderr, "%d %s\n", i, dp->d_name);
			seqnums[i++] = atoi(&dp->d_name[LEN_TAPE_VSN+1]);
		}
	}
	(void)closedir(dirp);
	seqnums[i] = -1;

	return (i);
}

int
main(int argc, char *argv[])
{
	int vcnt, opt, all = 0, quite=0;
	static upath_t fullpath;
	char *vsn = NULL;
	char *vsnlist;

	while((opt = getopt(argc, argv, "adqv:")) != -1)
	{
		switch(opt)
		{
		    case 'a':
			all = 1;
			break;
		    case 'q':
			quite = 1;
			break;
		    case 'd':
			debug = 1;
			break;
		    case 'v':
			vsn = optarg;
			break;
		}
	}
	if (!all && vsn == NULL) {
		printf("%s: [-adq] [-v <vsn>]\n", argv[0]);
		exit(-1);
	}

	if (all)
		vcnt = getVsns(&vsnlist);
	else {
		char *vp;
		vp = (char*) malloc(1 * (LEN_TAPE_VSN+1));
		memset(vp, 0, 1 * (LEN_TAPE_VSN+1));
		vsnlist = vp;
		strncpy(vp, vsn, LEN_TAPE_VSN);
		vcnt = 1;
	}
	if (debug) printf("vcnt=%d\n", vcnt);
	printf("		  date	       addr  num    vsn   pid flags	  next first last err\n");
	for (int j = 0; j < vcnt; j++) {
		int icnt, i;
		int seqnums[16] = {0};

		vsn = vsnlist + idx(j);
		icnt = setSeqnums(vsn, seqnums);
		if (debug) printf("icnt=%d\n", icnt);
		for (i = 0; i < icnt; i++) {
			char *crtime;
			size_t len;
			char flg[32] = {'\0'};

			(void) sprintf(fullpath, "%s/%s.%d", STDIR, vsn, seqnums[i]);
			if (debug) printf("%d path=%s\n", i, fullpath);
			StreamInfo = (StreamInfo_t *)MapInFile(fullpath , O_RDWR, &len);

			if (StreamInfo == NULL) {
				fprintf(stderr, "Cannot map in file '%s' errno: %d\n",
				    fullpath , errno);
				return (-1);
			}
			if (!debug && StreamInfo->create == 0) {
				munmap(StreamInfo, len);
				continue;
			}
			if (debug)
				printf("%d,%d date: %d\n", j, i, StreamInfo->create);
			crtime = asctime(localtime(&StreamInfo->create));
			crtime[strlen(crtime)-1] = '\0';
			if (StreamInfo->flags & SR_ACTIVE)
					flg[i++] = 'a';
			if (StreamInfo->flags & SR_DONE)
					flg[i++] = 'd';
			if (StreamInfo->flags & SR_LOADING)
					flg[i++] = 'l';
			if (StreamInfo->flags & SR_WAIT)
					flg[i++] = 'w';
			if (StreamInfo->flags & SR_WAITDONE)
					flg[i++] = 'W';
			if (StreamInfo->flags & SR_ERROR)
					flg[i++] = 'e';
			if (StreamInfo->flags & SR_CLEAR)
					flg[i++] = 'c';
			if (StreamInfo->flags & SR_UNAVAIL)
					flg[i++] = 'u';
			if (StreamInfo->flags & SR_DCACHE_CLOSE)
					flg[i++] = 'u';

			printf("%s: %p %4d %3s %6d %s %p %6d %6d %6d\n",
			   crtime,
			   StreamInfo,
			   StreamInfo->count,
			   StreamInfo->vsn,
			   StreamInfo->pid,
			   flg,
			   StreamInfo->next,
			   StreamInfo->first,
			   StreamInfo->last,
			   StreamInfo->error);
			munmap(StreamInfo, len);
		}
	}
	free(vsnlist);

	return (0);
}
