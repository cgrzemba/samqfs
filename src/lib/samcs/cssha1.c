#include <sys/types.h>
#include <sys/uio.h>
#include <sys/sha1.h>
#include <stdio.h>
#include <stdlib.h>

#include "sam/types.h"
#include "sam/checksum.h"
#include "sam/sam_malloc.h"

static char *_SrcFile = __FILE__;

static SHA1_CTX *sha1_context = NULL;

void cs_sha1(uint64_t *cookie, uchar_t *buf, uint64_t len, csum_t *val)
{
	int i,j;
		
	uint8_t csum_buf[SHA1_DIGEST_LENGTH];

	if (cookie != NULL) {
		/* initialization */
		for (i = 0; i < CS_LEN; i++) {
			val->csum_val[i] = 0;
		}
		if (sha1_context == NULL) {
			SamMalloc(sha1_context, sizeof(SHA1_CTX));
		}
		SHA1Init(sha1_context);
		return;
	} else {
		if (buf != NULL && len > 0) {
			SHA1Update(sha1_context, buf, len);
			return;	
		}
	}
	SHA1Final(csum_buf, sha1_context);
	for (i=0,j=0;i < (SHA1_DIGEST_LENGTH>>2);i++){
		val->csum_val[i] = (csum_buf[j]<<24) + (csum_buf[j+1]<<16) + (csum_buf[j+2]<<8) + csum_buf[j+3];
		j += sizeof(uint32_t);
	}
	free(sha1_context);
	sha1_context = NULL;
	return;
}

