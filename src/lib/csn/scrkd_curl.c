/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at pkg/OPENSOLARIS.LICENSE
 * or https://illumos.org/license/CDDL.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at pkg/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */
#pragma ident   "$Revision: 1.8 $"

static char *_SrcFile = __FILE__;  /* Using __FILE__ makes duplicate strings */

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>
#include <curl/curl.h>
#include "csn/scrkd.h"
#include "scrkd_curl.h"


#define	HTTP_OK			200
#define	CURL_VERIFY_SRV		1
#define	CURL_VERIFY_HOST	2
#define	CURL_HTTP_CLIENT_ERR	HTTP_CLIENT_ERR

#ifdef TESTNONSECURE
#define	CURL_REQUIRE_SSL	0
#else
#define	CURL_REQUIRE_SSL	1
#endif

typedef struct {
	char *buf;
	int size;	/* size of buf in bytes, including null byte */
} resp_data_t;

static CURLcode curl_setup_opts(CURL *easyhandle, struct curl_slist *headers,
    const char *url, resp_data_t *rp, const char *prx_host_port,
    const char *prx_user_pass, const char *CA_path);

static size_t curl_write_data_mem(void *, size_t, size_t, void *);

struct data {
	char trace_ascii; /* 1 or 0 */
};

#ifdef CURL_DBG_TO_FILE

static void
dump(
const char *text,
FILE *stream,
unsigned char *ptr,
size_t size,
char nohex)
{

	size_t i;
	size_t c;
	unsigned int width = 0x10;


	if (!text || !stream || !ptr) {
		return;
	}

	if (nohex) {
		/* without the hex output, we can fit more on screen */
		width = 0x40;
	}

	fprintf(stream, "%s, %zd bytes (0x%zx)\n", text, size, size);

	for (i = 0; i < size; i += width) {
		fprintf(stream, "%04zx: ", i);
		if (!nohex) {
			/* hex not disabled, show it */
			for (c = 0; c < width; c++) {
				if (i+c < size) {
					fprintf(stream, "%02x ", ptr[i+c]);
				} else {
					fputs("   ", stream);
				}
			}
		}

		for (c = 0; (c < width) && (i+c < size); c++) {
			/*
			 * Check for 0D0A; if found, skip past and start a
			 * new line of output
			 */
			if (nohex && (i+c+1 < size) && ptr[i+c] == 0x0D &&
			    ptr[i+c+1] == 0x0A) {
				i += (c+2-width);
				break;
			}
			fprintf(stream, "%c", (ptr[i+c] >= 0x20) &&
			    (ptr[i+c] < 0x80) ? ptr[i+c] : '.');

			/*
			 * Check again for 0D0A, to avoid an extra \n if it's
			 * at width.
			 */
			if (nohex && (i+c+2 < size) && ptr[i+c+1] == 0x0D &&
			    ptr[i+c+2] == 0x0A) {
				i += (c+3-width);
				break;
			}
		}
		fputc('\n', stream); /* newline */
	}
	fflush(stream);
}
#endif /* CURL_DBG_TO_FILE */

static int
curl_trace(
CURL *handle,
curl_infotype type,
unsigned char *data,
size_t size,
void *userp) {


	FILE *config;
	const char *text;

	/* check the args that get used */
	if (!data) {
		return (0);
	}

	switch (type) {
		case CURLINFO_TEXT:
			_LOG(DBG_LVL, "httpc: %s", data);

		case CURLINFO_HEADER_OUT:
			text = "=> Send header";
			break;

		case CURLINFO_DATA_OUT:
			text = "=> Send data";
			break;

		case CURLINFO_HEADER_IN:
			text = "<= Recv header";
			break;

		case CURLINFO_DATA_IN:
			text = "<= Recv data";
			break;

		case CURLINFO_SSL_DATA_IN:
			text = "<= Recv SSL data";
			break;

		case CURLINFO_SSL_DATA_OUT:
			text = "<= Send SSL data";
			break;
		default: /* in case a new one is introduced to shock us */
			return (0);

	}

#ifdef CURL_DBG_TO_FILE
	if (userp) {
		/* Dump the trace into curl.log file */
		config = (FILE *)userp;
		dump(text, config, data, size, 1);
	}
#endif

	return (0);
}


/*
 * curl_write_data_mem
 *
 * Write the received data to a string buffer.  This function may be called
 * many times for the entire data.  One extra byte is created for the null
 * character.
 *
 * Returns the number of bytes added to the buffer which is the bytes
 * received during this call, otherwise returns 0.
 */
static size_t
curl_write_data_mem(
void *ptr,	/* Data to copy */
size_t size,
size_t nmemb,
void *data) /* Response buffer */
{


	char *buffer;
	int add_sz;
	int alloc_sz;
	int offset;
	resp_data_t *data_ptr;

	if (!ptr || !data) {
		_LOG(ERR_LVL, "writing response data failed: out of memory");
		return (0);
	}
	data_ptr = (resp_data_t *)data;

	/* calculate the incoming size */
	alloc_sz = add_sz = size * nmemb;

	/*
	 * add 1 byte to the size to accommodate the null terminator if
	 * this is the first call
	 */
	if (data_ptr->buf == NULL) {
		alloc_sz++;
		offset = 0;
	} else {
		/*
		 * set offset so that the null terminator will be overwritten
		 */
		offset = data_ptr->size - 1;
	}

	/* Add the existing size to the incoming */
	alloc_sz += data_ptr->size;

	/* Allocate and copy into the response buffer */
	if ((buffer = (char *)realloc(data_ptr->buf, alloc_sz)) == NULL) {
		_LOG(ERR_LVL, "writing response data failed: out of memory");
		return (0);
	}

	data_ptr->buf = buffer;

	bcopy(ptr, &(data_ptr->buf[offset]), add_sz);

	data_ptr->size = alloc_sz;

	/* set the null terminator */
	data_ptr->buf[alloc_sz - 1] = 0;

	return (add_sz);
}


/*
 * curl_setup_opts
 *
 * Setup options (SSL, http request, and receive function).
 *
 * Returns CURLE_OK (0) for success, otherwise a positive number
 * corresponding to a curl defined error.
 *
 * prx_host_port shold contain <proxy_host>[:port]
 */
static CURLcode
curl_setup_opts(
CURL *easyhandle,
struct curl_slist *headers,
const char *url,
resp_data_t *rp,
const char *prx_host_port,
const char *prx_user_pass,
const char *CA_path) {

	CURLcode rc = CURLE_OK;
	boolean_t use_ssl = B_FALSE;

	if (!easyhandle || !headers || !url || !rp) {
		return (CURLE_BAD_FUNCTION_ARGUMENT);
	}
	/* url is https set use_ssl to B_TRUE */
	if (*url != '\0' && strstr(url, "https") != NULL) {
		use_ssl = B_TRUE;
	}

	if (CA_path && use_ssl) {

		/* set the file name that contains the certificate */
		if ((rc = curl_easy_setopt(easyhandle, CURLOPT_CAINFO,
		    CA_path)) != CURLE_OK)
			return (rc);

		if ((rc = curl_easy_setopt(easyhandle, CURLOPT_SSL_VERIFYPEER,
		    CURL_VERIFY_SRV)) != CURLE_OK)
			return (rc);

		if ((rc = curl_easy_setopt(easyhandle, CURLOPT_SSL_VERIFYHOST,
		    CURL_VERIFY_HOST)) != CURLE_OK)
			return (rc);
	}

	if ((rc = curl_easy_setopt(easyhandle, CURLOPT_URL, url)) != CURLE_OK)
		return (rc);

	/* pass our list of custom made headers */
	if ((rc = curl_easy_setopt(easyhandle, CURLOPT_HTTPHEADER, headers)) !=
	    CURLE_OK)
		return (rc);

	if ((rc = curl_easy_setopt(easyhandle, CURLOPT_HTTP_VERSION,
	    CURL_HTTP_VERSION_1_1)) != CURLE_OK)
		return (rc);

	if ((rc = curl_easy_setopt(easyhandle, CURLOPT_NOSIGNAL, 1)) !=
	    CURLE_OK)
		return (rc);

	if ((rc = curl_easy_setopt(easyhandle, CURLOPT_CONNECTTIMEOUT, 30)) !=
	    CURLE_OK)
		return (rc);

	if ((rc = curl_easy_setopt(easyhandle, CURLOPT_TIMEOUT, 45)) !=
	    CURLE_OK)
		return (rc);

	if ((rc = curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION,
	    curl_write_data_mem)) != CURLE_OK)
		return (rc);

	if ((rc = curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, rp)) !=
	    CURLE_OK)
		return (rc);

	/* Non-null prx_host_port indicates to use the proxy */
	if (prx_host_port)  {
		if ((rc = curl_easy_setopt(easyhandle, CURLOPT_PROXY,
		    prx_host_port)) != CURLE_OK) {
			return (rc);
		}

		/* non-null prx_user_pass means to use proxy auth */
		if (prx_user_pass) {
			if ((rc = curl_easy_setopt(easyhandle,
			    CURLOPT_PROXYUSERPWD,
			    prx_user_pass)) != CURLE_OK)
				return (rc);
		}

		/* Only for HTTPS Post, via HTTPS Tunneling */
		if (use_ssl) {
			if ((rc = curl_easy_setopt(easyhandle,
			    CURLOPT_HTTPPROXYTUNNEL, 1))  != CURLE_OK)
				return (rc);
		}
	}

	return (0);
}

/*
 * curl_send_request
 *
 * Sends/receives http post request/response.
 *
 * Returns the malloced post response, otherwise 0.
 */
char *
curl_send_request(
int *err_code,
char **err_str,
void *post_data,
const char *url,
const char *content_type,
const proxy_cfg_t *prx) {

	struct curl_slist *headers = NULL;
	CURL		*easyhandle;
	long int	resp_code;
	double		resp_content_len;
	CURLcode	rc = CURLE_OK;
	char		CA_path[128];
	char		curlerrorbuf[CURL_ERROR_SIZE];
	char		usr_pass_buf[USER_PASS_LEN];
	char		host_port_buf[HOST_PORT_LEN];
	char		*usr_pass = NULL;
	char		*host_port = NULL;
	resp_data_t	rp = {0, 0};

#ifdef CURL_DBG_TO_FILE
	FILE		*fp = NULL;
#endif

	*err_code = 0;
	*err_str = NULL;


	if (!err_code || !err_str || !post_data || !url ||
	    !content_type || !prx) {
		_LOG(ERR_LVL, "send request failed: invalid arguments");
		*err_code = -1;
		return (0);
	}
	if (check_proxy_cfg(prx) != 0) {
		_LOG(ERR_LVL, "send request failed: invalid proxy argument");
		*err_code = -1;
		return (0);
	}

	if ((easyhandle = curl_easy_init()) == NULL) {
		*err_code = CURL_HTTP_CLIENT_ERR;
		*err_str = "send request failed: could not create handle";
		_LOG(ERR_LVL, *err_str);
		return (0);
	}

	if ((headers = curl_slist_append(headers, content_type)) == NULL) {
		*err_code = CURL_HTTP_CLIENT_ERR;
		*err_str =  "send request failed: curl_slist_append failed";
		_LOG(ERR_LVL, *err_str);
		curl_easy_cleanup(easyhandle);
		return (0);
	}

	/*
	 * DEBUGFUNCTION has no effect until we enable VERBOSE
	 * If setting up debug or verbose fails log it but continue
	 *
	 * for less verbose debug.
	 * Use curl_easy_setopt(easyhandle, CURLOPT_STDERR, fp);
	 */
	if (curl_easy_setopt(easyhandle, CURLOPT_DEBUGFUNCTION,
	    curl_trace) != CURLE_OK) {
		_LOG(ERR_LVL, "send request warning: setting debug failed");
	}

	if (curl_easy_setopt(easyhandle, CURLOPT_VERBOSE, 1) != CURLE_OK) {
		_LOG(ERR_LVL, "send request warning: setting verbose failed");
	}

	if (curl_easy_setopt(easyhandle, CURLOPT_ERRORBUFFER,
	    curlerrorbuf) != CURLE_OK) {
		_LOG(ERR_LVL, "send request warning:setting errorbuf failed");
	}

#ifdef CURL_DBG_TO_FILE
	/*
	 *  Incomplete, fclose() not call before all return() calls.
	 */
	if ((fp = fopen(CURL_DBG_FILE, "w")) == NULL) {
		_LOG(ERR_LVL, "send request failed: unable to open curl.log");
		curl_easy_cleanup(easyhandle);
		curl_slist_free_all(headers);
		*err_code = -1;
		return (0);
	}

	if (curl_easy_setopt(easyhandle, CURLOPT_DEBUGDATA, fp) != CURLE_OK) {
		_LOG(ERR_LVL, "send request warning: set debug data failed");
	}
#endif


	/* setup proxy and ssl args to curl_setup_opts */
	snprintf(CA_path, 128, "%s", CA_FILE);

	/*
	 * If proxy is enabled setup usr_pass and host_port otherwise
	 * leave them set to NULL. This is done outside of
	 * curl_setup_opts because the buffers these are stored in
	 * must exist while the easyhandle exists.
	 */
	if (prx->enabled) {
		if (prx->host == NULL) {
			curl_easy_cleanup(easyhandle);
			curl_slist_free_all(headers);
			*err_code = CURLE_COULDNT_RESOLVE_PROXY;
			*err_str = (char *)curl_easy_strerror(*err_code);
			_LOG(ERR_LVL, "sending request failed:proxy enabled"
			    " but proxy host is null");
			return (0);
		} else if (prx->port == NULL) {
			strlcpy(host_port_buf, prx->host,
			    sizeof (host_port_buf));
		} else {
			snprintf(host_port_buf, sizeof (host_port_buf),
			    "%s:%s", prx->host, prx->port);
		}
		host_port = host_port_buf;

		/*
		 * If proxy auth is enabled but no user and password are
		 * supplied leave usr_pass set to null and try the connection
		 * anyway as it might succeed.
		 */
		if (prx->auth) {
			if (prx->user && prx->passwd) {
				snprintf(usr_pass_buf, sizeof (usr_pass_buf),
				    "%s:%s", prx->user, prx->passwd);
				usr_pass = usr_pass_buf;
			}
		}
	}

	if (strcmp(content_type, "multipart/form-data") == 0) {

		/* Post Port Data */
		if ((rc = curl_setup_opts(easyhandle, headers, url, &rp,
		    host_port, usr_pass, CA_path)) != CURLE_OK) {
			curl_slist_free_all(headers);
			curl_easy_cleanup(easyhandle);

			*err_code = rc;
			*err_str = (char *)curl_easy_strerror(rc);
			_LOG(ERR_LVL, "send request failed:setting up opts %s",
			    *err_str);
			return (0);
		}

		if ((rc = curl_easy_setopt(easyhandle, CURLOPT_HTTPPOST,
		    ((struct curl_httppost *)post_data))) != CURLE_OK) {
			*err_code = rc;
			*err_str = (char *)curl_easy_strerror(rc);
			_LOG(ERR_LVL, "send request failed:setting data %s",
			    *err_str);
			return (0);
		}

	} else { /* Post Plain Data */

		if ((rc = curl_setup_opts(easyhandle, headers, url, &rp,
		    host_port, usr_pass, CA_path)) != CURLE_OK) {

			curl_slist_free_all(headers);
			curl_easy_cleanup(easyhandle);
			*err_code = rc;
			*err_str = (char *)curl_easy_strerror(rc);
			_LOG(ERR_LVL, "send request failed:setting up opts %s",
			    *err_str);
			return (0);
		}

		/* POST fields specific to this function */
		if ((rc = curl_easy_setopt(easyhandle, CURLOPT_POSTFIELDS,
		    (char *)post_data)) != CURLE_OK) {
			*err_code = rc;
			*err_str = (char *)curl_easy_strerror(rc);
			_LOG(ERR_LVL, "send request failed:setting up opts %s",
			    *err_str);

			return (0);
		}
	}

	/* post the form */
	if ((rc = curl_easy_perform(easyhandle)) != CURLE_OK) {
		curl_slist_free_all(headers);
		curl_easy_cleanup(easyhandle);
		free(rp.buf);
		*err_code = rc;
		*err_str = (char *)curl_easy_strerror(rc);
		_LOG(ERR_LVL, "httpc: got error response %d: %s\n",
		    rc, curlerrorbuf);

		return (0);
	}

	/* get response */
	if ((rc = curl_easy_getinfo(easyhandle, CURLINFO_RESPONSE_CODE,
	    &resp_code)) != CURLE_OK) {
		curl_slist_free_all(headers);
		curl_easy_cleanup(easyhandle);
		free(rp.buf);
		*err_code = rc;
		*err_str = (char *)curl_easy_strerror(rc);
		_LOG(ERR_LVL, "send request failed:getting response %s",
		    *err_str);
		return (0);
	}

	if (resp_code != HTTP_OK) {
		*err_code = CURL_HTTP_CLIENT_ERR;
		*err_str = "httpc: non-OK (200) HTTP response code," \
		    " please check system log";
		curl_slist_free_all(headers);
		curl_easy_cleanup(easyhandle);
		free(rp.buf);
		_LOG(ERR_LVL, "send request failed:HTTP response code: %d",
			resp_code);
		return (0);
	}

	if ((rc = curl_easy_getinfo(easyhandle,
	    CURLINFO_CONTENT_LENGTH_DOWNLOAD,
	    &resp_content_len)) != CURLE_OK) {

		curl_slist_free_all(headers);
		curl_easy_cleanup(easyhandle);
		free(rp.buf);
		*err_code = rc;
		*err_str = (char *)curl_easy_strerror(rc);
		_LOG(ERR_LVL, "send request failed:getting content length %d",
			*err_str);

		return (0);
	}

	curl_slist_free_all(headers); /* free the header list */
	curl_easy_cleanup(easyhandle);

#ifdef CURL_DBG_TO_FILE
	if (fp) {
		fclose(fp);
	}
#endif

	if (resp_content_len <= 0) {
		*err_code = CURL_HTTP_CLIENT_ERR;
		*err_str = "httpc: invalid response len," \
		    " please check system log";
		_LOG(ERR_LVL, "send request failed: invalid response len: %f",
		    resp_content_len);
		free(rp.buf);
		return (0);
	}

	if (rp.buf == 0) {
		*err_code = CURL_HTTP_CLIENT_ERR;
		*err_str = "send request failed: recieved no response";
		_LOG(ERR_LVL, "send request failed: %s", *err_str);
	}

	/* rp.buf is malloced so return it directly */
	return (rp.buf);
}
