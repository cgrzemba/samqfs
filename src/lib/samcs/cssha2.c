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
static SHA384_CTX *sha384_context = NULL;
static SHA512_CTX *sha512_context = NULL;

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

void cs_sha384(uint64_t *cookie, uchar_t *buf, uint64_t len, csum_t *val)
{
	int i,j;

	uint8_t csum_buf[SHA384_DIGEST_LENGTH];

	if (cookie != NULL) {
		/* initialization */
		for (i = 0; i < CS_LEN; i++) {
			val->csum_val[i] = 0;
		}
		if (sha384_context == NULL) {
			SamMalloc(sha384_context, sizeof(SHA384_CTX));
		}
		SHA384Init(sha384_context);
		return;
	} else {
		if (buf != NULL && len > 0) {
			SHA384Update(sha384_context, buf, len);
			return;
		}
	}
	SHA384Final(csum_buf, sha384_context);
	for (i=0,j=0;i < (SHA384_DIGEST_LENGTH>>2);i++){
		val->csum_val[i] = (csum_buf[j]<<24) + (csum_buf[j+1]<<16)
		    + (csum_buf[j+2]<<8) + csum_buf[j+3];
		j += sizeof(uint32_t);
	}
	free(sha384_context);
	sha384_context = NULL;
	return;
}

void cs_sha512(uint64_t *cookie, uchar_t *buf, uint64_t len, csum_t *val)
{
	int i,j;

	uint8_t csum_buf[SHA512_DIGEST_LENGTH];

	if (cookie != NULL) {
		/* initialization */
		for (i = 0; i < CS_LEN; i++) {
			val->csum_val[i] = 0;
		}
		if (sha512_context == NULL) {
			SamMalloc(sha512_context, sizeof(SHA512_CTX));
		}
		SHA512Init(sha512_context);
		return;
	} else {
		if (buf != NULL && len > 0) {
			SHA512Update(sha512_context, buf, len);
			return;
		}
	}
	SHA512Final(csum_buf, sha512_context);
	for (i=0,j=0;i < (SHA512_DIGEST_LENGTH>>2);i++){
		val->csum_val[i] = (csum_buf[j]<<24) + (csum_buf[j+1]<<16)
		    + (csum_buf[j+2]<<8) + csum_buf[j+3];
		j += sizeof(uint32_t);
	}
	free(sha512_context);
	sha512_context = NULL;
	return;
}
