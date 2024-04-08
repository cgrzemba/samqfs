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
#include <stddef.h>
#include <ctype.h>
#include <strings.h>
#include <sys/systeminfo.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/bn.h>
#include <openssl/rand.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <curl/curl.h>
#include <sys/utsname.h>
#include <sys/systeminfo.h>
#include <pthread.h>
#include "scrkd_xml_string.h"
#include "scrkd_xml.h"
#include "csn/scrkd.h"
#include "scrkd_curl.h"

static const char *REGSTR_PRV_PASSWD	= "0bfusc@teMyP$$WD";
unsigned char IV[8] = {0xa7, 0xb8, 0xf8, 0x02, 0xbc, 0xf8, 0x45, 0x2f};

const char *SCRK_SUCCESS_CODE		= "CODE=0";
const char *SCRK_CLIENT_ID		= "SC_CLIENT_REG_ID";


typedef struct {
	char    *scrk_offer_class;
	char    *scrk_desc;
} regstr_telem_type_t;


typedef struct sig_ctx {
	EVP_MD_CTX  md_ctx;
	EVP_PKEY   *pkey;
} SIG_CTX;


static int regstr_post_data(regstr_telem_type_t *type, cl_reg_cfg_t *cl,
    xml_string_t *sb, char *prod_name, char *prod_version);

static int get_b64_signature(xml_string_t *sb, char *asset_prefix,
	char **result);

static int generate_asset_id(char *asset_prefix, char **asset_id);

static int get_config_value(char *which_key, char *res, int res_len);

static int extract_scrk_id_or_error(char *response, boolean_t client_reg,
    char **scrkid);

static int xlate_curl_error_code(int curl_response_code);

static int xlate_csn_error_code(int curl_response_code);


static int sign_data(char *key_file, char *data, int data_len,
    unsigned char **sig, int *sig_len);

extern size_t Curl_base64_encode(const char *input, size_t size, char **str);


static pthread_once_t scrk_once = PTHREAD_ONCE_INIT;
static void scrkd_init(void);

static pthread_mutex_t dhdata_mutex = PTHREAD_MUTEX_INITIALIZER;
static DH *dhdata = NULL;
static DH *get_dh_params();


static void
scrkd_init() {
	int rnd_ret;

	curl_global_init(CURL_GLOBAL_SSL);
	OpenSSL_add_all_algorithms();

	/*
	 * seed the random number generator. RAND_load_file performs
	 * non-blocking I/0
	 */
	rnd_ret = RAND_load_file("/dev/random", 2048);
	_LOG(MSG_LVL, "initialized scrk seeded %d random", rnd_ret);
}


/*
 * This method generates the Public-Private Key pair.
 * The keys will be stored in files at:
 * <csn_basedir>/<asset_prefix>/key.pub
 * <csn_basedir>/<asset_prefix>/key.priv
 *
 * key.priv will have read write permissions and will
 * contain an obfuscated key written using
 * PEM_write_PKCS8PrivateKey with the PEM_PK_PASSWD. This
 * same password will be used to decrypt the private key
 * for the purpose of signing telemetry messages.
 *
 * Returns 0 on success.
 * Returns -1 for internal errors or a scrk_errnos for
 * potentially user correctable errors.
 */
int
regstr_generate_public_private_key(
char *asset_prefix)
{

	RSA *rsa = NULL;
	int fdpriv = -1;
	int fdpub = -1;
	FILE *privfp = NULL;
	FILE *pubfp = NULL;
	char keypath[MAXPATHLEN];

	pthread_once(&scrk_once, scrkd_init);

	_LOG(MSG_LVL, "Generating public and private keys");
	if (asset_prefix == NULL) {
		_LOG(ERR_LVL, "Generating keys failed: null asset prefix");
		return (-1);
	}

	/*
	 * open the private key file, restricting permissions to owner
	 * read and write
	 */
	snprintf(keypath, MAXPATHLEN, CNS_CLPRIVKEYPATH_SPEC, asset_prefix);
	if ((fdpriv = open(keypath, O_WRONLY|O_CREAT|O_TRUNC, 0600)) != -1) {
		privfp = fdopen(fdpriv, "w");
	}
	if (!privfp) {
		if (fdpriv != -1) {
			close(fdpriv);
		}
		_LOG(ERR_LVL, "Cannot open private key file %d", errno);
		return (SCRK_CANT_WRITE_TO_DATA_DIR);
	}

	snprintf(keypath, MAXPATHLEN, CNS_CLPUBKEYPATH_SPEC, asset_prefix);
	if ((fdpub = open(keypath, O_WRONLY|O_CREAT|O_TRUNC, 0600)) != -1) {
		pubfp = fdopen(fdpub, "w");
	}
	if (!pubfp) {
		if (fdpub != -1) {
			close(fdpub);
		}
		_LOG(ERR_LVL, "Cannot open public key file");
		fclose(privfp);
		return (SCRK_CANT_WRITE_TO_DATA_DIR);
	}

	/* Load the crypto library error strings, SSL_xxx loads more. */
	ERR_load_crypto_strings();

	/* generate a key pair */
	if ((rsa = RSA_generate_key(1024, RSA_F4, NULL, NULL)) != NULL)	{
		EVP_PKEY *pkey = EVP_PKEY_new();
		if (pkey == NULL) {
			fclose(privfp);
			fclose(pubfp);
			_LOG(ERR_LVL, "Key generation failed");
			return (-1);
		}

		/*  convert RSA key to EVP_PKEY format */
		EVP_PKEY_assign_RSA(pkey, rsa);

		/*
		 * For the PEM write routines if the kstr parameter is not NULL
		 * then klen bytes at kstr are used as the passphrase and cb is
		 * ignored.
		 *
		 * If the cb parameters is set to NULL and the u
		 * parameter is not NULL then the u parameter is
		 * interpreted as a null terminated string to use as
		 * the passphrase. If both cb and u are NULL then the
		 * default callback routine is used which will
		 * typically prompt for the passphrase on the current
		 * terminal with echoing turned off.
		 *
		 * Write a private key (using PKCS#8 format) using basic
		 * DES encryption.
		 */
		if (PEM_write_PKCS8PrivateKey(privfp, pkey, EVP_des_ede3_cbc(),
		    NULL, 0, 0, (char *)REGSTR_PRV_PASSWD) == 0) {
			_LOG(ERR_LVL, "Error writing Private key");
			fclose(privfp);
			fclose(pubfp);
			EVP_PKEY_free(pkey);
			return (-1);
		}

		if (PEM_write_RSA_PUBKEY(pubfp, rsa) == 0) {
			_LOG(ERR_LVL, "Error writing Public key");
			fclose(privfp);
			fclose(pubfp);
			EVP_PKEY_free(pkey);
			return (-1);
		}

		EVP_PKEY_free(pkey);
	} else {
		_LOG(ERR_LVL, "Error generating RSA key pair");
		fclose(privfp);
		fclose(pubfp);
		return (-1);
	}

	fclose(privfp);
	fclose(pubfp);

	return (0);
}

/*
 * This method does the client registration.
 * Returns 0 if successful and sets cl->reg_id to the clients registration id.
 *
 * Note: When telemetry or heartbeat are implemented, the proxy
 * password if entered will need to be stored. For now don't store it.
 */
int
regstr_postclient_regis_data(
cl_reg_cfg_t *cl,
char *sunpasswd) {

	char *response, *errmsg, *data;

	char *scrkid = NULL;
	int code, datalen;
	struct curl_httppost *post = NULL;
	struct curl_httppost *last = NULL;
	CURLFORMcode curlfrmerr;
	FILE *keyfp;
	struct stat st;
	char clnt_url[1024];
	char keypath[MAXPATHLEN];


	pthread_once(&scrk_once, scrkd_init);

	code = 0;
	errmsg = NULL;

	if (sunpasswd == NULL) {
		_LOG(ERR_LVL, "register client failed: no password");
		return (-1);
	}
	if (check_cl_reg_cfg(cl, B_TRUE) != 0) {
		_LOG(ERR_LVL, "register client failed: invalid inputs");
		return (-1);
	}

	/* Build Form Data */
	if ((curlfrmerr = curl_formadd(&post, &last,
				CURLFORM_COPYNAME, "VERSION",
				CURLFORM_COPYCONTENTS, "1.1.0",
				CURLFORM_END)) != CURL_FORMADD_OK) {
		_LOG(ERR_LVL, "register client failed adding version: %d",
			curlfrmerr);
		return (-1);
	}

	if ((curlfrmerr = curl_formadd(&post, &last,
				CURLFORM_COPYNAME, "SOA_ID",
				CURLFORM_COPYCONTENTS, cl->sun_login,
				CURLFORM_END)) != CURL_FORMADD_OK) {

		_LOG(ERR_LVL, "register client failed adding soa_id: %d",
		    curlfrmerr);
		curl_formfree(post);
		return (-1);
	}

	if ((curlfrmerr = curl_formadd(&post, &last,
				CURLFORM_COPYNAME, "SOA_PW",
				CURLFORM_COPYCONTENTS, sunpasswd,
				CURLFORM_END)) != CURL_FORMADD_OK) {

		_LOG(ERR_LVL, "register client failed adding passwd:%d",
		    curlfrmerr);
		curl_formfree(post);
		return (-1);
	}

	/* asset_id is put into the cl so it will be freed with cl's fields */
	if (generate_asset_id(cl->asset_prefix, &(cl->asset_id)) != 0) {
		_LOG(ERR_LVL, "register client failed generating asset id",
		    curlfrmerr);
		curl_formfree(post);
		return (-1);
	}

	if ((curlfrmerr = curl_formadd(&post, &last,
				CURLFORM_COPYNAME, "ASSET_ID",
				CURLFORM_COPYCONTENTS, cl->asset_id,
				CURLFORM_END)) != CURL_FORMADD_OK) {

		_LOG(ERR_LVL, "register client failed adding assetid:%d",
		    curlfrmerr);
		curl_formfree(post);
		return (-1);
	}

	/*
	 * Read in and append the public key. Note the key has been
	 * transformed into human readable characters by PEM write. So
	 * no extra work is needed here to avoid special chararcters.
	 */
	snprintf(keypath, MAXPATHLEN, CNS_CLPUBKEYPATH_SPEC, cl->asset_prefix);
	if ((keyfp = fopen(keypath, "r")) == NULL) {
		_LOG(ERR_LVL, "register client failed opening key file");
		curl_formfree(post);
		return (-1);
	}

	if (stat(keypath, &st) < 0) {
		_LOG(ERR_LVL, "register client stat failed for key file");
		fclose(keyfp);
		curl_formfree(post);
		return (-1);
	}

	if ((data = malloc(st.st_size)) == NULL) {
		_LOG(ERR_LVL, "register client failed- ran out of memory");
		fclose(keyfp);
		curl_formfree(post);
		return (-1);
	}

	datalen = st.st_size;
	if (fread(data, 1, datalen, keyfp) != datalen) {
		_LOG(ERR_LVL, "register client failed reading key");
		free(data);
		fclose(keyfp);
		curl_formfree(post);
		return (-1);
	}
	fclose(keyfp);
	keyfp = 0;

	if ((curlfrmerr = curl_formadd(&post, &last,
				CURLFORM_COPYNAME, "PUBLIC_KEY",
				CURLFORM_COPYCONTENTS, data,
				CURLFORM_CONTENTSLENGTH, datalen,
				CURLFORM_END)) != CURL_FORMADD_OK) {
		_LOG(ERR_LVL, "register client failed adding public key:%d",
		    curlfrmerr);
		free(data);
		curl_formfree(post);
		return (-1);
	}

	/*
	 * curl_formadd with CURLFORM_COPYCONTENTS copies the data so it
	 * can now be freed
	 */
	free(data);

	/* if an updated url is not present, use the defined one */
	if (get_config_value("client_url", clnt_url, sizeof (clnt_url)) != 0) {
		strlcpy(clnt_url, CLIENT_REGISTRATION_URL, sizeof (clnt_url));
	}

	/* Send the request and get the response */
	response = (char *)curl_send_request(&code, &errmsg, post, clnt_url,
				    FORMDATA_CONTENT_TYPE, &(cl->prxy));
	curl_formfree(post);

	if (response) {
		_LOG(MSG_LVL, "client registration response: %s", response);
		int srv_rsp_code = extract_scrk_id_or_error(response,
		    B_TRUE, &scrkid);

		free(response);
		if (srv_rsp_code != 0) {
			_LOG(ERR_LVL, "register client failed no reg id %d",
			    srv_rsp_code);
			return (srv_rsp_code);
		}
	} else {
		/*
		 * If there is no response it means a problem occurred
		 * with curl. Translate the curl return code into
		 * a scrk error.
		 */
		_LOG(ERR_LVL, "No response during client registration");
		scrkid = NULL;
		return (xlate_curl_error_code(code));
	}

	cl->reg_id = scrkid;
	return (0);
}


static int
extract_scrk_id_or_error(char *response, boolean_t client_reg, char **scrkid) {
	char *ptr, *lastptr;
	char seps[]   = "\n";
	boolean_t error = B_FALSE;

	if (!response) {
		return (-1);
	}

	if (client_reg && !scrkid) {
		return (-1);
	}

	if (strstr(response, "ERROR")) {
		error = B_TRUE;
	} else if (!strstr(response, "SUCCESS")) {
		/*
		 * Note that ERROR and SUCCESS are not true and false
		 * of the same boolean here. If neither result is
		 * returned it indicates a failure of the signature
		 * validation on the server side or an invalid
		 * request.  These are unlikely cases over which the
		 * user has no control. So simply return a -1
		 */
		_LOG(ERR_LVL, "signature verify failed or Invalid Request:%s",
		    response);

		return (-1);
	}

	/* Parse the response */
	ptr = (char *)strtok_r(response, seps, &lastptr);
	if (ptr == NULL) {
		return (-1);
	}
	while ((ptr = (char *)strtok_r(NULL, seps, &lastptr)) != NULL) {
		/* Only try to extract the code if there was an error */
		if (error) {
			if (strstr(ptr, "CODE")) {
				/*
				 * found the error code
				 * extract it and translate it to
				 * a csn error code
				 */
				if (ptr != NULL) {
					long err;
					char *end_ptr;

					ptr = (char *)strtok_r(ptr, "=",
					    &lastptr);

					err = strtol(lastptr, &end_ptr, 10);
					if (ptr == end_ptr) {
						err = -1;
					}

					_LOG(ERR_LVL, "srck failed:%d %s",
					    err, response);

					return (xlate_csn_error_code(err));
				}
			}
		} else if (client_reg && strstr(ptr, SCRK_CLIENT_ID)) {
			/* extract the scrk id */

			if (scrkid == NULL) {
				_LOG(DBG_LVL, "Found id but arg is null: %s",
				    response);
				return (0);
			}
			if (ptr != NULL) {
				ptr = (char *)strtok_r(ptr, "=", &lastptr);
				if (lastptr == NULL) {

					_LOG(ERR_LVL, "client id format"
					    "problem in %s", response);
					return (-1);
				}

				*scrkid = strdup(lastptr);
				_LOG(DBG_LVL, "client id found: %s", response);
				return (0);
			}
		}
	}

	/*
	 * If we get to here we have a successful product registration
	 * or in the future a successful sending of telemetry
	 */

	return (0);
}


/*
 * This method does the product registration.
 * Returns zero if successful. -1 if un-successful.
 */
int
regstr_postproduct_regis_data(
sf_prod_info_t *sf,
cl_reg_cfg_t *cl)
{
	int rc;
	FILE *xml_fp = NULL;
	xml_string_t sb;
	char regfile[MAXPATHLEN];
	regstr_telem_type_t type =
		    {REGSTR_PRODUCT_OFFER_CLASS, "Product Registration"};


	pthread_once(&scrk_once, scrkd_init);

	if (check_cl_reg_cfg(cl, B_TRUE) != 0) {
		_LOG(ERR_LVL, "product Registration failed: bad client arg");
		return (-1);
	}
	if (check_sf_prod_info(sf) != 0) {
		_LOG(ERR_LVL, "product Registration failed: bad product arg");
		return (-1);
	}

	/* 2. Build XML */
	xml_string_init(&sb, XML_STRING_INCREMENT_MAX);

	if (build_product_regstr_xml(sf, cl, &sb) < 0) {
		_LOG(ERR_LVL, "product registration failed building XML");
		xml_string_free(&sb);
		return (-1);
	}

	snprintf(regfile, MAXPATHLEN, XML_REG_FILE_SPEC, cl->asset_prefix);
	xml_fp = fopen(regfile, "w");
	if (xml_fp) {
		if (fwrite(sb.str, 1, sb.len, xml_fp) != sb.len) {
			_LOG(ERR_LVL, "product registration failed writing" \
			    "xml to file ");
			xml_string_free(&sb);
			fclose(xml_fp);
		}
		fclose(xml_fp);
		xml_fp = 0;
	}

	rc = regstr_post_data(&type, cl, &sb, sf->name, sf->version);
	xml_string_free(&sb);
	return (rc);
}

/*
 * This is essentially a telemetry post function. But is used by
 * the product registration too.
 */
static int
regstr_post_data(
regstr_telem_type_t *type,
cl_reg_cfg_t *cl,
xml_string_t *sb,
char *prod_name,
char *prod_version) {

	struct curl_httppost	*pst = NULL;
	struct curl_httppost	*lst = NULL;
	CURLFORMcode	curlfrmerr;
	char		telemdescr[100];
	char		prdt_url[1024];
	char		*b64sig;
	char		*resp;
	char		*errmsg;
	int		errcode;
	int		retcode = 0;


	if (type == NULL || cl == NULL || sb == NULL ||
	    prod_name == NULL || prod_version == NULL) {
		return (-1);
	}

	/* Get a digital signature of the telemetry data */
	if ((retcode = get_b64_signature(sb, cl->asset_prefix,
		&b64sig)) != 0) {
		_LOG(ERR_LVL, "Post data failed signing message");
		return (retcode);
	}

	/* Build Telemetetry */
	errcode = 0;
	errmsg = NULL;

	if ((curlfrmerr = curl_formadd(&pst, &lst,
			    CURLFORM_COPYNAME, "VERSION",
			    CURLFORM_COPYCONTENTS, "1.1.0",
			    CURLFORM_END)) != CURL_FORMADD_OK) {

		_LOG(ERR_LVL, "post data failed adding version:%d", curlfrmerr);
		free(b64sig);
		curl_formfree(pst);
		return (-1);
	}

	if ((curlfrmerr = curl_formadd(&pst, &lst,
			    CURLFORM_COPYNAME, "SC_CLIENT_REG_ID",
			    CURLFORM_COPYCONTENTS, cl->reg_id,
			    CURLFORM_END)) != CURL_FORMADD_OK) {
		_LOG(ERR_LVL, "post data failed adding reg id:%d", curlfrmerr);
		free(b64sig);
		curl_formfree(pst);
		return (-1);
	}

	if ((curlfrmerr = curl_formadd(&pst, &lst,
			    CURLFORM_COPYNAME, "TELEMETRY_DATA",
			    CURLFORM_COPYCONTENTS, sb->str,
			    CURLFORM_CONTENTSLENGTH, sb->len,
			    CURLFORM_END)) != CURL_FORMADD_OK) {
		_LOG(ERR_LVL, "post data failed adding data:%d", curlfrmerr);

		free(b64sig);
		curl_formfree(pst);
		return (-1);
	}

	snprintf(telemdescr, 100, "%s %s %s", prod_name, prod_version,
		    type->scrk_desc);
	if ((curlfrmerr = curl_formadd(&pst, &lst,
			    CURLFORM_COPYNAME, "TELEMETRY_DESCRIPTION",
			    CURLFORM_COPYCONTENTS, telemdescr,
			    CURLFORM_CONTENTSLENGTH, strlen(telemdescr),
			    CURLFORM_END)) != CURL_FORMADD_OK) {
		_LOG(ERR_LVL, "post data failed adding description:%d",
		    curlfrmerr);

		free(b64sig);
		curl_formfree(pst);
		return (-1);
	}

	if ((curlfrmerr = curl_formadd(&pst, &lst,
			    CURLFORM_COPYNAME, "OFFERING_CLASS",
			    CURLFORM_COPYCONTENTS, type->scrk_offer_class,
			    CURLFORM_END)) != CURL_FORMADD_OK) {
		_LOG(ERR_LVL, "post data failed adding offering class:%d",
		    curlfrmerr);
		free(b64sig);
		curl_formfree(pst);
		return (-1);
	}

	if ((curlfrmerr = curl_formadd(&pst, &lst,
			    CURLFORM_COPYNAME, "PRODUCT_ID",
			    CURLFORM_COPYCONTENTS, prod_name,
			    CURLFORM_CONTENTSLENGTH, strlen(prod_name),
			    CURLFORM_END)) != CURL_FORMADD_OK) {
		_LOG(ERR_LVL, "post data failed adding product id:%d",
		    curlfrmerr);
		free(b64sig);
		curl_formfree(pst);
		return (-1);
	}

	if ((curlfrmerr = curl_formadd(&pst, &lst,
			    CURLFORM_COPYNAME, "MSG_SIGNATURE",
			    CURLFORM_COPYCONTENTS, b64sig,
			    CURLFORM_END)) != CURL_FORMADD_OK) {
		_LOG(ERR_LVL, "post data failed adding signature:%d",
		    curlfrmerr);
		free(b64sig);
		curl_formfree(pst);
		return (-1);
	}


	/* if an updated url is not present, use the defined one */
	if (get_config_value("product_url", prdt_url, sizeof (prdt_url)) != 0) {
		strlcpy(prdt_url, PRODUCT_REGISTRATION_URL, sizeof (prdt_url));
	}

	resp = (char *)curl_send_request(&errcode, &errmsg, pst, prdt_url,
			    FORMDATA_CONTENT_TYPE, &(cl->prxy));

	if (resp) {
		_LOG(MSG_LVL, "%s Response: %s", type->scrk_desc, resp);
		retcode = extract_scrk_id_or_error(resp, B_FALSE, NULL);
		free(resp);
	} else {
		/*
		 * If there is no response it means a problem occurred
		 * with curl. Translate the curl return code into
		 * a scrk error.
		 */
		_LOG(ERR_LVL, "No HTTP(S) response during %s",
		    type->scrk_desc);
		retcode = xlate_curl_error_code(errcode);
	}

	/* Clean up */
	curl_formfree(pst);
	pst = 0;

	if (b64sig != NULL) {
		free(b64sig);
	}
	b64sig = NULL;

	return (retcode);
}

static int
get_b64_signature(
xml_string_t *sb,
char *asset_prefix,
char **result)
{
	char		keypath[MAXPATHLEN];
	int		sig_len = 0;
	unsigned char	*sig_buf = NULL;
	char		*b64sig = NULL;
	int		sig_err;

	if (sb == NULL || asset_prefix == NULL || result == NULL) {
		return (-1);
	}

	/* open the private key file for this asset */
	snprintf(keypath, MAXPATHLEN, CNS_CLPRIVKEYPATH_SPEC, asset_prefix);

	if ((sig_err = sign_data(keypath, sb->str, sb->len, &sig_buf,
	    &sig_len)) != 0) {

		_LOG(ERR_LVL, "Signing the data was unsuccessful");
		return (sig_err);
	}

	/*  Base64 encode the signature a zero return indicates error  */
	if (Curl_base64_encode((char *)sig_buf, sig_len, &b64sig) == 0) {
		_LOG(ERR_LVL, "Base 64 encoding failed");
		free(sig_buf);
		free(b64sig);
		return (-1);
	}
	free(sig_buf);
	*result = b64sig;
	return (0);
}

int
sign_data(
char		*key_file,
char		*data,
int		data_len,
unsigned char	**sig,
int		*sig_len)
{

	SIG_CTX		sig_ctx;
	FILE		*privfp = NULL;
	int		len = 0;
	uint_t		ulen = 0;
	unsigned char	*sig_buf;


	if (key_file == NULL || data == NULL || sig == NULL ||
	    sig_len == NULL) {
		_LOG(ERR_LVL, "sign data failed: bad arguments");
		return (-1);
	}
	bzero(&sig_ctx, sizeof (SIG_CTX));

	/* open the private key file for this asset */
	privfp = fopen(key_file, "r");
	if (!privfp) {
		_LOG(ERR_LVL, "sign data failed: cannot open private key");
		return (SCRK_CANT_READ_DATA);
	}

	sig_ctx.pkey = (EVP_PKEY *)PEM_read_PrivateKey(privfp, NULL, NULL,
	    (char *)REGSTR_PRV_PASSWD);

	(void) fclose(privfp);
	if (sig_ctx.pkey == NULL) {
		_LOG(ERR_LVL, "sign data failed: cannot read key");
		return (-1);
	}

	/*
	 * Note the EVP routines return 1 for success and 0 for failure
	 */
	if (EVP_SignInit(&(sig_ctx.md_ctx), EVP_sha1()) != 1) {
		EVP_PKEY_free(sig_ctx.pkey);
		_LOG(ERR_LVL, "sign data failed: init failed");
		return (-1);
	}

	if (EVP_SignUpdate(&(sig_ctx.md_ctx), data, data_len) != 1) {
		EVP_PKEY_free(sig_ctx.pkey);
		EVP_MD_CTX_cleanup(&(sig_ctx.md_ctx));
		_LOG(ERR_LVL, "sign data failed: update failed");
		return (-1);
	}

	len = EVP_PKEY_size(sig_ctx.pkey);
	if (len <= 0) {
		_LOG(ERR_LVL, "sign data failed: invalid key length %d", len);
		EVP_PKEY_free(sig_ctx.pkey);
		EVP_MD_CTX_cleanup(&(sig_ctx.md_ctx));
		return (-1);
	}

	sig_buf = malloc(len);
	if (sig_buf != NULL) {
		/* finalize it */
		ulen = (uint_t)len;
		if (EVP_SignFinal(&(sig_ctx.md_ctx), (uchar_t *)sig_buf,
		    &ulen, sig_ctx.pkey) != 1) {
			_LOG(ERR_LVL, "sign data failed:signing");
			free(sig_buf);
			EVP_PKEY_free(sig_ctx.pkey);
			EVP_MD_CTX_cleanup(&(sig_ctx.md_ctx));
			return (-1);
		}
	} else {
		_LOG(ERR_LVL, "sign data failed: out of memory");
		EVP_PKEY_free(sig_ctx.pkey);
		EVP_MD_CTX_cleanup(&(sig_ctx.md_ctx));
		return (-1);
	}

	EVP_PKEY_free(sig_ctx.pkey);
	sig_ctx.pkey = 0;
	EVP_MD_CTX_cleanup(&(sig_ctx.md_ctx));

	if (len <= 0) {
		_LOG(ERR_LVL, "sign data failed: ");
		free(sig_buf);
		return (-1);
	}

	*sig = sig_buf;
	*sig_len = len;
	return (0);
}


int
check_cl_reg_cfg(
const cl_reg_cfg_t *cl,
const boolean_t for_initial_reg) {

	if (cl == NULL || cl->asset_prefix == NULL) {
		return (-1);
	}

	if (for_initial_reg && cl->sun_login == NULL) {
		return (-1);
	}

	if (!for_initial_reg && cl->reg_id == NULL) {
		return (-1);
	}

	if (check_proxy_cfg(&(cl->prxy)) != 0) {
		return (-1);
	}
	return (0);
}

int
check_proxy_cfg(
const proxy_cfg_t *prxy)
{
	if (prxy == NULL) {
		return (-1);
	}
	if (prxy->enabled && (prxy->port == NULL ||
	    prxy->host == NULL)) {
		return (-1);
	}
	if (prxy->auth && (prxy->user == NULL ||
	    prxy->passwd == NULL)) {
		return (-1);
	}

	return (0);
}


int
check_sf_prod_info(
const sf_prod_info_t *sf)
{
	/* check only for required fields during registration */
	if (sf == NULL || sf->name == NULL || sf->id == NULL ||
	    sf->version == NULL) {
		return (-1);
	}
	return (0);
}


/*
 * Generate an asset id by appending system identifying information to
 * the asset_prefix. The goal of this is that the asset id is unique
 * for this host.
 */
static int
generate_asset_id(char *asset_prefix, char **asset_id) {
	char asset_buf[MAX_ASSET_ID];
	char hw_id_buf[256];
	long info_len;

	*asset_buf = '\0';

	if (asset_id == NULL) {
		return (-1);
	}
	if (*asset_id != NULL) {
		_LOG(DBG_LVL, "Using user supplied asset id");
		return (0);
	}

	/*
	 * The combo of SI_HW_PROVIDER  and  SI_HW_SERIAL is likely
	 * to be unique across all SVR4 implementations
	 */
	strlcat(asset_buf, asset_prefix, MAX_ASSET_ID);

	info_len = sysinfo(SI_HW_PROVIDER, hw_id_buf, 256);
	if (info_len == -1) {
		_LOG(ERR_LVL, "Error obtaining HW provider");
		return (-1);
	} else {
		strlcat(asset_buf, hw_id_buf, MAX_ASSET_ID);
	}

	info_len = sysinfo(SI_HW_SERIAL, hw_id_buf, 256);
	if (info_len == -1) {
		_LOG(ERR_LVL, "Error obtaining HW version");
		return (-1);
	} else {
		strlcat(asset_buf, hw_id_buf, MAX_ASSET_ID);
	}

	*asset_id = strdup(asset_buf);
	if (*asset_id == NULL) {
		_LOG(ERR_LVL, "failed to malloc asset id: out of memory");
		return (-1);
	}
	return (0);
}


void
free_sf_prod_info(sf_prod_info_t *sf) {

	if (sf) {
		if (sf->name) {
			free(sf->name);
		}

		if (sf->id) {
			free(sf->id);
		}

		if (sf->descr) {
			free(sf->descr);
		}

		if (sf->version) {
			free(sf->version);
		}

		if (sf->vendor) {
			free(sf->vendor);
		}

		if (sf->name) {
			free(sf->name);
		}
		free(sf);
	}
}

/*
 * free all non-null fields and cl
 */
void
free_cl_reg_cfg(cl_reg_cfg_t *cl) {
	if (cl) {
		free_cl_reg_cfg_fields(cl);
		free(cl);
	}
}

/*
 * free all non-null fields but not cl
 */
void
free_cl_reg_cfg_fields(cl_reg_cfg_t *cl) {

	if (cl) {
		if (cl->sun_login) {
			free(cl->sun_login);
		}
		if (cl->name) {
			free(cl->name);
		}
		if (cl->email) {
			free(cl->email);
		}

		if (cl->asset_prefix) {
			free(cl->asset_prefix);
		}
		if (cl->asset_id) {
			free(cl->asset_id);
		}
		if (cl->reg_id) {
			free(cl->reg_id);
		}
		if (cl->prxy.host) {
			free(cl->prxy.host);
		}

		if (cl->prxy.port) {
			free(cl->prxy.port);
		}
		if (cl->prxy.user) {
			free(cl->prxy.user);
		}
		if (cl->prxy.passwd) {
			free(cl->prxy.passwd);
		}

		/*
		 * zero the buf to avoid potential
		 * problems with the pointers
		 */
		memset(cl, 0, sizeof (cl_reg_cfg_t));
	}
}


/*
 * Perform the phase 1 of our Diffie Hellman key agreement.
 * Return malloced DH Public key in a hex string or -1 and NULL on error.
 *
 * If the DH parameters have not been loaded or if the public key
 * has not yet been computed these steps will be performed.
 */
int
get_public_key(char **hex_pub_key, unsigned char **sig, int *sig_len) {
	char *cs = NULL;
	int errcode;


	if (hex_pub_key == NULL || sig == NULL || sig_len == NULL) {
		_LOG(ERR_LVL, "Invalid null arguments in phase 2 of exchange");
		return (-1);
	}

	*hex_pub_key = NULL;

	/* Initialize encryption algorithms */
	pthread_once(&scrk_once, scrkd_init);

	/* Lock the global dhdata */
	pthread_mutex_lock(&dhdata_mutex);

	/* if dhdata is null get the params */
	if (dhdata == NULL) {
		dhdata = get_dh_params();
		if (dhdata == NULL) {
			pthread_mutex_unlock(&dhdata_mutex);
			_LOG(ERR_LVL, "loading params failed in phase1");
			return (-1);
		}
	}

	/* if local keys are null generate them */
	if (dhdata->priv_key == NULL || dhdata->pub_key == NULL) {

		/* DH_generate_key returns 1 on success */
		if (!DH_generate_key(dhdata)) {
			pthread_mutex_unlock(&dhdata_mutex);
			_LOG(ERR_LVL, "generating keys failed in phase 1");
			return (-1);
		}
	}

	/* translate the key to hex and sign it */
	cs = BN_bn2hex(dhdata->pub_key);

	/* Done with all dhdata access so unlock */
	pthread_mutex_unlock(&dhdata_mutex);


	if (cs == NULL) {
		_LOG(ERR_LVL, "getting public key failed in phase 1");
		return (-1);
	}

	/* Sign the key. Note a return of 1 indicates success */
	if ((errcode = sign_data(CNS_PRIVATE, cs, strlen(cs), sig,
	    sig_len)) != 0) {
		free(cs);
		_LOG(ERR_LVL, "signing key failed in phase 1");
		return (errcode);
	}

	*hex_pub_key = cs;
	return (0);
}


/*
 * calculate the secret key based on the previously generated
 * local dhdata.
 *
 * Note this function will fail unless get_public_key is called
 * first.
 *
 * Returns length in bytes of secret on success and -1 on error.
 */
int
get_secret(
char *remote_public,
unsigned char **secret)
{

	int keylen;
	BIGNUM *rm_pub_bn = NULL;
	unsigned char *keybuf;

	pthread_once(&scrk_once, scrkd_init);
	if (remote_public == NULL || secret == NULL) {
		_LOG(ERR_LVL, "Invalid null arguments in phase 2 of exchange");
		return (-1);
	}

	/* create the remote_public BIGNUM. 0 return indicates an error */
	if (!BN_hex2bn(&rm_pub_bn, remote_public)) {
		_LOG(ERR_LVL, "Invalid remote public in phase 2 of exchange");
		return (-1);
	}

	/* Lock for access to the DH struct */
	pthread_mutex_lock(&dhdata_mutex);


	/* Make sure the structure is ready for key computation */
	if (dhdata->p == NULL || dhdata->g == NULL ||
	    dhdata->priv_key == NULL) {
		_LOG(ERR_LVL, "Phase 1 of key exchange was skipped or failed");
		pthread_mutex_unlock(&dhdata_mutex);
		return (-1);
	}


	/* Determine the resulting key size */
	keylen = DH_size(dhdata);
	if (keylen <= 0) {
		_LOG(ERR_LVL, "Invalid size in phase 2 of exchange");
		return (-1);
	}
	keybuf = (unsigned char *)malloc(keylen);
	if (keybuf == NULL) {
		pthread_mutex_unlock(&dhdata_mutex);
		_LOG(ERR_LVL, "Out of memory in phase 2 of exchange");
		return (-1);
	}
	memset(keybuf, 0, keylen);

	/* compute the key from remote public value and local dhdata */
	if (DH_compute_key(keybuf, rm_pub_bn, dhdata) == -1) {
		pthread_mutex_unlock(&dhdata_mutex);
		free(keybuf);
		_LOG(ERR_LVL, "Key computation failed in phase 2 of exchange");
		return (-1);
	}
	pthread_mutex_unlock(&dhdata_mutex);
	*secret = keybuf;

	return (keylen);
}


DH *
get_dh_params()
{
	static unsigned char dh1024_p[] = {
		0x93, 0xF6, 0x86, 0x83, 0x78, 0xE7, 0x43, 0x17, 0xD0, 0x04,
		0xEA, 0x5A, 0xF7, 0x75, 0x02, 0x30, 0x7A, 0x65, 0x96, 0x5D,
		0x05, 0x26, 0x93, 0x3E, 0xC9, 0x18, 0x64, 0x5C, 0x5C, 0x9B,
		0x10, 0xAE, 0x55, 0xFB, 0x0A, 0x3C, 0x2B, 0x0C, 0x75, 0x84,
		0xF2, 0xF9, 0x11, 0x5F, 0x96, 0x66, 0xCD, 0x2A, 0x21, 0x92,
		0x66, 0xE4, 0xBE, 0x99, 0xB7, 0x2D, 0x04, 0x4F, 0x56, 0x23,
		0x40, 0xC7, 0xEE, 0x96, 0x98, 0xB1, 0xFF, 0xBD, 0xF3, 0x40,
		0xAB, 0xE5, 0x2F, 0x21, 0xD1, 0x15, 0x87, 0x4C, 0xC4, 0xAE,
		0xF1, 0x56, 0xAF, 0x07, 0x5F, 0xD5, 0x48, 0xA9, 0x50, 0x3B,
		0x30, 0x50, 0xDA, 0xD5, 0x83, 0x83, 0xEE, 0x5F, 0x5A, 0x93,
		0xC4, 0x26, 0x54, 0xA0, 0xD4, 0x89, 0xCF, 0xEF, 0xE4, 0x3C,
		0x00, 0x35, 0x23, 0xF7, 0x14, 0x35, 0xEC, 0xC2, 0x0A, 0xD7,
		0xCC, 0xFD, 0x68, 0x61, 0xE1, 0x0D, 0xB0, 0xFB
	};

	static unsigned char dh1024_g[] = {
		0x02
	};

	DH *dh;

	if ((dh = DH_new()) == NULL) {
		_LOG(ERR_LVL, "Failed to create DH params");
		return (NULL);
	}

	dh->p = BN_bin2bn(dh1024_p, sizeof (dh1024_p), NULL);
	dh->g = BN_bin2bn(dh1024_g, sizeof (dh1024_g), NULL);

	if ((dh->p == NULL) || (dh->g == NULL)) {
		DH_free(dh);
		_LOG(ERR_LVL, "Failed to create DH params");
		return (NULL);
	}

	return (dh);
}


int
encrypt_string(
char *plain_text,
unsigned char *secret,
unsigned char **cipher_text,
int *cipher_text_len) {

	EVP_CIPHER_CTX ectx;
	int ctext_len;
	int final_len;
	unsigned char *ctext_buf;

	/* check args */
	if (plain_text == NULL || secret == NULL || cipher_text == NULL ||
	    cipher_text_len == NULL) {
		return (-1);
	}

	/* initialize crypto routines if not already done */
	pthread_once(&scrk_once, scrkd_init);

	EVP_CIPHER_CTX_init(&ectx);

	/*
	 * NULL IV is ok with ecb mode but a random IV should be generated
	 * once saved in the apps and used for any other modes
	 * Really should not use ecb mode.
	 */
	if (!EVP_EncryptInit(&ectx, EVP_des_ede3_cbc(), secret, IV)) {
		EVP_CIPHER_CTX_cleanup(&ectx);
		return (-1);
	}

	ctext_buf = (unsigned char *)malloc(strlen(plain_text) +
		EVP_CIPHER_CTX_block_size(&ectx) + 1);
	if (ctext_buf == NULL) {
		EVP_CIPHER_CTX_cleanup(&ectx);
		return (-1);
	}


	/*
	 * Note to avoid overflow if useing ecb or cbc modes
	 * make the cipher text buffer
	 * one full block bigger than the plaintext.
	 * malloc (inputlength + EVP_CIPHER_CTX_block_size());
	 * This is because the padding used in the
	 * cipher could result in output that is longer than the input.
	 */
	if (!EVP_EncryptUpdate(&ectx, ctext_buf, &ctext_len,
		(unsigned char *)plain_text, strlen(plain_text) + 1)) {
		EVP_CIPHER_CTX_cleanup(&ectx);
		free(ctext_buf);
		return (-1);
	}

	/*
	 * Finish up encrypting any leftover partial blocks writing
	 * them to the ctext_buf after the previously encrypted data.
	 * Final len will return the final amount added to the buf and
	 * should be added to ctext_len to get the total length of the
	 * cipher text.
	 */
	if (!EVP_EncryptFinal(&ectx, ctext_buf + ctext_len, &final_len)) {
		EVP_CIPHER_CTX_cleanup(&ectx);
		free(ctext_buf);
		return (-1);
	}

	EVP_CIPHER_CTX_cleanup(&ectx);


	ctext_len += final_len;
	/*
	 * To write this to a file use
	 * f = fopen(proxypassfile, "wb"); // write binary
	 * fwrite(ctext_buf, 1, ctext_len, out);
	 * close(f);
	 */

	*cipher_text = ctext_buf;
	*cipher_text_len = ctext_len;

	return (0);

}


int
decrypt_string(
char **plain_text,
unsigned char *secret,
unsigned char *cipher_text,
int cipher_text_len) {

	EVP_CIPHER_CTX dctx;
	unsigned char *tmp_plain;
	int plain_len;
	int final_len;

	if (plain_text == NULL || secret == NULL || cipher_text == NULL ||
	    cipher_text_len <= 0) {
		return (-1);
	}

	*plain_text = NULL;

	/* initialize crypto routines if not already done */
	pthread_once(&scrk_once, scrkd_init);

	EVP_DecryptInit(&dctx, EVP_des_ede3_cbc(), secret, IV);


	/*
	 * Generate a buffer big enough to hold the plain text which
	 * could be 1 block bigger than the cipher (?) and add one
	 * extra byte to hold a null terminator.
	 */
	plain_len = cipher_text_len + EVP_CIPHER_CTX_block_size(&dctx) + 1;
	tmp_plain = (unsigned char *)malloc(plain_len);
	if (tmp_plain == NULL) {
		EVP_CIPHER_CTX_cleanup(&dctx);
		return (-1);
	}
	memset(tmp_plain, 0, plain_len);


	/* Pass in the entire cipher text */
	if (!EVP_DecryptUpdate(&dctx, tmp_plain, &plain_len, cipher_text,
	    cipher_text_len)) {
		free(tmp_plain);
		EVP_CIPHER_CTX_cleanup(&dctx);
		return (-1);
	}

	/*
	 * unpad the partial block if one existed and add
	 * add it to tmp_plain. Note this step is only required
	 * if the cipher being used is ECB or CBC mode(block based modes)
	 */
	if (!EVP_DecryptFinal(&dctx, &(tmp_plain[plain_len]), &final_len)) {
		free(tmp_plain);
		EVP_CIPHER_CTX_cleanup(&dctx);
		return (-1);
	}

	EVP_CIPHER_CTX_cleanup(&dctx);

	/* update the length with anything added in final phase */
	plain_len += final_len;

	/*
	 * add a null terminator just in case the string was not null
	 * terminated. If it was unneeded it will be ignored and the buffer
	 * is sufficiently large to hold it.
	 */
	tmp_plain[plain_len] = '\0';


	*plain_text = (char *)tmp_plain;
	return (0);
}



/*
 * Must take buf_len because some bufs not null terminated
 */
void
free_sensitive_buf(char **buf, int buf_len) {
	int i;

	if (buf != NULL && *buf != NULL) {
		for (i = 0; i < buf_len; i++) {
			(*buf)[i] = 0;
		}
		free(*buf);
		*buf = NULL;
	}
}


/*
 * If a CNS_CONF file exists read it and look for a key value pair
 * line with a key that matches which key.
 *
 * If a value is found copy the url into the res buffer.
 * Otherwize set *res = '\0' and return.
 *
 * Currently the only keys that the library supports are:
 * "client_url" and "product_url"
 */
static int
get_config_value(char *which_key, char *res, int res_len) {
	FILE *f;
	char *ptr;
	char buffer[256];

	if (which_key == NULL || res == NULL) {
		return (-1);
	}

	*res = '/0';


	/* Open the url config file. */
	f = fopen64(CNS_CONF, "r");
	if (f == NULL) {

		/*
		 * since the file is optional don't put a shocking
		 * failure message in the trace file. But put enough
		 * to allow debugging.
		 */
		_LOG(MSG_LVL, "CNS configuration open: %d", errno);
		return (-1);
	}

	for (;;) {
		ptr = fgets(buffer, sizeof (buffer), f);
		if (ptr == NULL)
			break; /* End of file */

		if (*ptr == '\0' || *ptr == '#') {
			continue;
		}

		/*
		 * See if the line is for the requested key, if so
		 * extract and dup the value
		 */
		if (strstr(ptr, which_key) == ptr) {
			char *value;
			if ((value = strstr(ptr, "=")) != NULL) {
				value++;
				if (value != '\0') {
					strlcpy(res, value, res_len);
				}
			}
			break;
		}

	}
	fclose(f);

	if (*res != '/0') {
		return (0);
	}

	_LOG(ERR_LVL, "CNS configuration key not found: %s", which_key);
	return (-1);
}

static int
xlate_curl_error_code(int curl_response_code) {

	int rc;

	switch (curl_response_code) {
		case 0:
			rc = 0;
			break;
		case CURLE_COULDNT_RESOLVE_PROXY:
			rc = SCRK_COULDNT_RESOLVE_PROXY;
			break;
		case CURLE_COULDNT_RESOLVE_HOST:
			rc = SCRK_COULDNT_RESOLVE_HOST;
			break;
		case CURLE_OPERATION_TIMEOUTED:
			rc = SCRK_OPERATION_TIMED_OUT;
			break;
		case CURLE_SSL_PEER_CERTIFICATE:
			rc = SCRK_BAD_SERVER_CERTIFICATE;
		default:
			rc = -1;
	}
	return (rc);

}

/*
 * translate an error code from the csn server to an error number
 * from scrkd.h. Note, most errors from the csn server are related to
 * condtions the user can not influence. So most will not be
 * exposed at this point as anything other than a general error.
 */
static int
xlate_csn_error_code(int csn_server_error) {
	int rc;

	switch (csn_server_error) {
		case 0:
			rc = 0;
			break;
		case 4:
			rc = SCRK_AUTHENTICATION_FAILED;
			break;
		case 6:
			rc = SCRK_REG_SERVER_FAILURE;
			break;
		default:
			rc = -1;
	}
	return (rc);
}
