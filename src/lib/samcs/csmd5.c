#include <sys/types.h>
#include <sys/uio.h>
#include <md5.h>
#include <stdio.h>
#include <stdlib.h>

#include "sam/types.h"
#include "sam/checksum.h"
#include "sam/sam_malloc.h"

static char *_SrcFile = __FILE__;

static MD5_CTX *md5_context = NULL;

void cs_md5(uint64_t *cookie, uchar_t *buf, uint64_t len, csum_t *val)
{
	int i,j;
		
	uint8_t csum_buf[CS_LEN*sizeof(uint32_t)];

	if (cookie != NULL) {
		/* initialization */
		for (i = 0; i < CS_LEN; i++) {
			val->csum_val[i] = 0;
		}
		if (md5_context == NULL) {
			SamMalloc(md5_context, sizeof(MD5_CTX));
		}
		MD5Init(md5_context);
		return;
	} else {
		if (buf != NULL && len > 0) {
			MD5Update(md5_context, buf, len);
			return;	
		}
	}
	MD5Final(csum_buf, md5_context);
	for (i=0,j=0;i<CS_LEN;i++){
		val->csum_val[i] = (csum_buf[j]<<24) + (csum_buf[j+1]<<16) + (csum_buf[j+2]<<8) + csum_buf[j+3];
		j += sizeof(uint32_t);
	}
	free(md5_context);
	md5_context = NULL;
	return;
}

