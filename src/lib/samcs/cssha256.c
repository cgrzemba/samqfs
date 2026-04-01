#include <sys/types.h>
#include <sys/uio.h>
#include <sys/sha2.h>
#include <stdio.h>
#include <stdlib.h>

#include "sam/types.h"
#include "sam/checksum.h"
#include "sam/sam_malloc.h"

static char *_SrcFile = __FILE__;

static SHA256_CTX *sha256_context = NULL;

void cs_sha256(uint64_t *cookie, uchar_t *buf, uint64_t len, csum_t *val)
{
	int i,j;

	uint8_t csum_buf[SHA256_DIGEST_LENGTH];

	if (cookie != NULL) {
		/* initialization */
		for (i = 0; i < CS_LEN; i++) {
			val->csum_val[i] = 0;
		}
		if (sha256_context == NULL) {
			SamMalloc(sha256_context, sizeof(SHA256_CTX));
		}
		SHA256Init(sha256_context);
		return;
	} else {
		if (buf != NULL && len > 0) {
			SHA256Update(sha256_context, buf, len);
			return;
		}
	}
	SHA256Final(csum_buf, sha256_context);
	for (i=0,j=0;i < (SHA256_DIGEST_LENGTH>>2);i++){
		val->csum_val[i] = (csum_buf[j]<<24) + (csum_buf[j+1]<<16)
		    + (csum_buf[j+2]<<8) + csum_buf[j+3];
		j += sizeof(uint32_t);
	}
	free(sha256_context);
	sha256_context = NULL;
	return;
}

