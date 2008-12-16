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
 * or http://www.opensolaris.org/os/licensing.
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
#pragma ident   "$Revision: 1.11 $"

static char *_SrcFile = __FILE__;  /* Using __FILE__ makes duplicate strings */

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <strings.h>
#include <fcntl.h>
#include "mgmt/util.h"

#include "csn/scrkd.h"
#include "sam/sam_trace.h"
#include "pub/mgmt/file_util.h"
#include "pub/mgmt/license.h"
#include "pub/mgmt/csn_registration.h"
#include "pub/mgmt/error.h"
#include "pub/mgmt/mgmt.h"

static void set_registration_error(int errcode);
static int save_client_conf(char *filename, cl_reg_cfg_t *cl);
static boolean_t is_registered(cl_reg_cfg_t *cl);
static boolean_t conditional_print(FILE *f, char *str, boolean_t printit);
static int get_associated_package_info(char **addn_info);

#define	MAX_REG_KV 2048

static sf_prod_info_t samqfs_swordfish_data = {
	"Sun StorageTek SAM-FS 4.6 Software",
	"urn:uuid:8d3de4be-6e89-11db-817b-080020a9ed93",
	NULL,
	"4.6",
	"Sun Microsystems, Inc.",
	"urn:uuid:e97a46f7-3e54-11d9-9607-080020a9ed93",
	NULL
};

static sf_prod_info_t qfs_swordfish_data = {
	"Sun StorageTek QFS 4.6 Software",
	"urn:uuid:6de2a1e5-6e8b-11db-817b-080020a9ed93",
	NULL,
	"4.6",
	"Sun Microsystems, Inc.",
	"urn:uuid:e97a46f7-3e54-11d9-9607-080020a9ed93",
	NULL
};

static parsekv_t cl_reg_cfg_tokens[] = {
	{"sun_login", offsetof(struct cl_reg_cfg, sun_login),
		parsekv_mallocstr},
	{"name", offsetof(struct cl_reg_cfg, name),
		parsekv_mallocstr},
	{"email", offsetof(struct cl_reg_cfg, email),
		parsekv_mallocstr},
	{"asset_prefix", offsetof(struct cl_reg_cfg, asset_prefix),
		parsekv_mallocstr},
	{"asset_id", offsetof(struct cl_reg_cfg, asset_id), parsekv_mallocstr},
	{"reg_id", offsetof(struct cl_reg_cfg, reg_id), parsekv_mallocstr},
	{"registered", offsetof(struct cl_reg_cfg, registered),
		parsekv_bool_YN},
	{"prxy_enabled", offsetof(struct cl_reg_cfg, prxy) +
		offsetof(struct proxy_cfg, enabled), parsekv_bool_YN},
	{"prxy_auth", offsetof(struct cl_reg_cfg, prxy) +
		offsetof(struct proxy_cfg, auth), parsekv_bool_YN},
	{"prxy_host", offsetof(struct cl_reg_cfg, prxy) +
		offsetof(struct proxy_cfg, host),  parsekv_mallocstr},
	{"prxy_port", offsetof(struct cl_reg_cfg, prxy) +
		offsetof(struct proxy_cfg, port),  parsekv_mallocstr},
	{"prxy_user", offsetof(struct cl_reg_cfg, prxy) +
		offsetof(struct proxy_cfg, user),  parsekv_mallocstr},
	{"", 0, NULL}
};


/*
 * get the current registration for the identified product.
 * product should be the asset_id prefix for the product
 * you wish to retrieve the registration for.
 * for sam-qfs it should be SAMQFS_ASSET.
 *
 * reg_kv will return the registration information for
 * the named product in the format listed for cns_set_registration
 *
 * If there is no registration for the product *reg_kv will be set
 * to null.
 */
int
cns_get_registration(
ctx_t *c,	/* ARGSUSED */
char *product,
char **reg_kv)
{

	FILE	*indat;
	char	*ptr;
	char	buffer[MAX_REG_KV];
	char	filename[MAXPATHLEN];
	cl_reg_cfg_t *cl = NULL;


	if (ISNULL(product, reg_kv)) {
		Trace(TR_ERR, "reading registration file failed %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}

	*reg_kv = NULL;

	/* Open and read the file for the asset specified */
	snprintf(filename, MAXPATHLEN, CNS_CLIENTDATA_SPEC, product);

	Trace(TR_MISC, "read registration file %s", filename);

	indat = fopen64(filename, "r");
	if (indat == NULL) {
		if (errno == ENOENT) {
			return (0);
		} else {
			samerrno = SE_CFG_OPEN_FAILED;

			/* Open failed for %s: %s */
			StrFromErrno(errno, buffer, sizeof (buffer));
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_CFG_OPEN_FAILED), filename,
			    buffer);
			Trace(TR_ERR, "reading registration file failed %d %s",
			    samerrno, samerrmsg);

			return (-1);
		}
	}


	cl = mallocer(sizeof (cl_reg_cfg_t));
	if (cl == NULL) {
		fclose(indat);
		Trace(TR_ERR, "reading registration file failed %d %s",
		    samerrno, samerrmsg);

		return (-1);
	}

	/*
	 * The for loop is really just to read through empty or comment
	 * lines. The file will only contain one registration
	 * for a single asset since each one gets its own data directory.
	 */
	for (;;) {
		uint32_t cnt;

		ptr = fgets(buffer, sizeof (buffer), indat);

		if (ptr == NULL) {
			break; /* End of file */
		}

		/* skip comments and whitespace only lines */
		while (isspace(*ptr)) {
			ptr++;
		}

		if (*ptr == '\0' || *ptr == '#') {
			continue;
		}

		cnt = strlen(ptr);
		if (cnt == 0) {
			/* line was comment or white space only */
			continue;
		}
		/* get rid of trailing new lines */
		if (*(ptr + cnt - 1) == '\n') {
			*(ptr + cnt - 1) = '\0';
		}

		if (parse_kv(ptr, cl_reg_cfg_tokens, cl) != 0) {
			Trace(TR_ERR, "parsing registration failed"
			    " for asset %s", Str(product));
			break;
		}

		/*
		 * see if this is for the right product if so dup
		 * the string from the file for return. Else this
		 * indicates an error because there must be only
		 * one entry.
		 */
		if ((*product != '\0') && ((strcmp(product,
		    cl->asset_prefix)) == 0)) {
			*reg_kv = copystr(buffer);
			break;
		} else {
			Trace(TR_ERR, "wrong registration found for asset %s",
			    Str(product));

			break;
		}
	}
	free_cl_reg_cfg(cl);
	fclose(indat);

	Trace(TR_MISC, "read registration file %s", filename);
	return (0);
}


/*
 * register SAM/QFS on the host being called with CNS.
 * The key value string must contain:
 * sun_login
 * sun_password (passed as arg cl_pwd)
 * name
 * email
 * asset_prefix = "sam-qfs"
 *
 * The key value string may contain:
 * prxy_enabled
 * prxy_port
 * prxy_host
 * prxy_auth
 * prxy_user
 * prxy_passwd(passed as arg proxy_pwd)
 *
 * The cl_pwd byte array must contain the client password for
 * the sun account encrypted with the public key returned from
 * cns_get_public_key.
 *
 * If proxy_auth is set to true, the proxy_pwd must be
 * the encrypted proxy password, using the same key as above.
 */
int
cns_register(
ctx_t *ctx,	/* ARGSUSED */
char *kv_string,
crypt_str_t *cl_pwd,
crypt_str_t *proxy_pwd,
char *clnt_pub_key_hex) {

	char		*cl_clear = NULL;
	unsigned char	*secret = NULL;
	int		secret_len;
	cl_reg_cfg_t	cl;
	sf_prod_info_t	*sf;
	char		asset_dir[MAXPATHLEN];
	int retval;

	Trace(TR_MISC, "csn registration entry");

	/* check the mandatory parameters */
	if (ISNULL(kv_string, cl_pwd, clnt_pub_key_hex)) {
		Trace(TR_ERR, "cns registration failed %d %s", samerrno,
		    samerrmsg);
		return (-1);
	}

	memset(&cl, 0, sizeof (cl_reg_cfg_t));
	if (cl_pwd->str == NULL) {
		samerrno = SE_NULL_PARAMETER;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
		    "password");
		Trace(TR_ERR, "cns registration failed %d %s", samerrno,
		    samerrmsg);
		goto err;
	}


	if (parse_kv(kv_string, cl_reg_cfg_tokens, &cl) != 0) {
		Trace(TR_ERR, "cns registration failed %d %s", samerrno,
			samerrmsg);
		goto err;
	}

	/*
	 * If there was a client registration id free it. Or it will leak.
	 * Note that reregistration is allowed in order to change the keys
	 * that are used for signing the telemetry data.
	 */
	if (cl.reg_id) {
		free(cl.reg_id);
		cl.reg_id = NULL;
	}

	/* Complete the last phase of key agreement */
	secret_len = get_secret(clnt_pub_key_hex, &secret);
	if (secret_len <= 0) {
		set_registration_error(-1);
		Trace(TR_ERR, "cns registration failed:getsecret");
		goto err;
	}

	/* get the password clear text */
	if (decrypt_string(&cl_clear, secret, cl_pwd->str,
	    cl_pwd->str_len) != 0) {
		set_registration_error(-1);
		Trace(TR_ERR, "cns reg failed:bad clear cl pass");
		goto err;
	}

	/* add the proxy password to the cl struct */
	if (proxy_pwd != NULL) {
		/*
		 * proxy password should be populated if it is
		 * present even if proxy authentication is not enabled.
		 * This behavior is really more forward looking for
		 * when telemetry is enabled.
		 */
		if (decrypt_string(&(cl.prxy.passwd), secret, proxy_pwd->str,
		    proxy_pwd->str_len) != 0) {
			set_registration_error(-1);
			Trace(TR_ERR, "cns reg failed:bad clear prxy pass");
			goto err;
		}
	}

	/* check the rest of the parameters */
	if (check_cl_reg_cfg(&cl, B_TRUE) != 0) {
		setsamerr(SE_INVALID_REG_PARAMS);
		Trace(TR_ERR, "cns registration failed %d %s", samerrno,
			samerrmsg);
		goto err;
	}

	/* create the data directory for this asset */
	if (cl.asset_prefix == NULL) {
		set_registration_error(-1);
		Trace(TR_ERR, "cns registration failed: %d %s",
		    samerrno, samerrmsg);
		goto err;
	}
	snprintf(asset_dir, sizeof (upath_t), CNS_ASSETDIR_SPEC,
	    cl.asset_prefix);

	if (create_dir(ctx, asset_dir) != 0) {
		Trace(TR_ERR, "cns registration failed: %d %s",
		    samerrno, samerrmsg);
		goto err;
	}

	/*
	 * generate the public and private keys for this client-asset pair
	 */
	if ((retval = regstr_generate_public_private_key(cl.asset_prefix))
	    != 0) {
		set_registration_error(retval);
		Trace(TR_ERR, "cns registration failed: %d %s",
		    samerrno, samerrmsg);
		goto err;
	}

	strlcat(asset_dir, CNS_CLIENT_CONF, sizeof (upath_t));

	if (get_samfs_type(ctx) == SAMQFS) {
		sf = &samqfs_swordfish_data;
	} else {
		sf = &qfs_swordfish_data;
	}

	/*
	 * This could save the client data now so that it can be
	 * fetched even if the registration fails. The user would still
	 * need to enter the appropriate passwords but nothing else would
	 * be required. The reason it is done later is that the reg_id needs
	 * to be saved and is not yet available.
	 */
	if ((retval = regstr_postclient_regis_data(&cl,
		(char *)cl_clear)) != 0) {

		set_registration_error(retval);
		Trace(TR_ERR, "cns registration failed: %d %s",
		    samerrno, samerrmsg);
		goto err;
	}

	/*
	 * Only post the product registration once. The reason to
	 * post the client registration more than once is that is how
	 * the key pair gets changed
	 */
	if (!is_registered(&cl)) {
		char *addn_info;

		/*
		 * If we can get additional version information about the
		 * packages include it but ignore errors.
		 */
		if (get_associated_package_info(&addn_info) == 0) {
			sf->additional_info = addn_info;
		}

		if ((retval = regstr_postproduct_regis_data(sf, &cl)) != 0) {
			set_registration_error(retval);
			Trace(TR_ERR, "cns registration failed: %d %s",
			    samerrno, samerrmsg);
			goto err;
		}
		cl.registered = B_TRUE;
		if (sf->additional_info) {
			free(sf->additional_info);
			sf->additional_info = NULL;
		}
	}
	if (save_client_conf(asset_dir, &cl) != 0) {
		Trace(TR_ERR, "cns registration failed: %d %s",
		    samerrno, samerrmsg);
		goto err;
	}


	free_sensitive_buf((char **)&secret, secret_len);
	free_sensitive_buf(&(cl_clear), strlen(cl_clear));
	if (cl.prxy.passwd) {
		free_sensitive_buf(&(cl.prxy.passwd), strlen(cl.prxy.passwd));
	}
	free_cl_reg_cfg_fields(&cl);
	Trace(TR_MISC, "cns registration succeeded");
	return (0);

err:
	if (sf->additional_info) {
		free(sf->additional_info);
		sf->additional_info = NULL;
	}
	free_sensitive_buf((char **)&secret, secret_len);
	free_sensitive_buf(&(cl_clear), strlen(cl_clear));
	if (cl.prxy.passwd) {
		free_sensitive_buf(&(cl.prxy.passwd), strlen(cl.prxy.passwd));
	}
	free_cl_reg_cfg_fields(&cl);

	return (-1);
}

static int
get_associated_package_info(char **addn_info) {
	sqm_lst_t *l;
	node_t *n;
	char buf[MAX_ADDITIONAL_INFO] = "";

	if (addn_info == NULL) {
		return (-1);
	}
	if (get_package_information(NULL, NULL,
	    PI_PKGINST | PI_VERSION, &l) != 0) {
		*addn_info = NULL;
		return (-1);
	}

	for (n = l->head; n != NULL; n = n->next) {
		if (n->data != NULL) {
			strlcat(buf, (char *)n->data, MAX_ADDITIONAL_INFO);
			strlcat(buf, "\n", MAX_ADDITIONAL_INFO);
		}
	}
	lst_free_deep(l);

	if (*buf != '\0') {
		*addn_info = strdup(buf);
		if (*addn_info == NULL) {
			return (-1);
		}
	}
	return (0);
}


static boolean_t
is_registered(cl_reg_cfg_t *cl) {
	cl_reg_cfg_t cur_reg;
	char *cur_reg_str;
	boolean_t ret;

	if (cns_get_registration(NULL, cl->asset_prefix, &cur_reg_str) != 0) {
		return (B_FALSE);
	}
	if (cur_reg_str == NULL) {
		return (B_FALSE);
	}

	memset(&cur_reg, 0, sizeof (cl_reg_cfg_t));
	if (parse_kv(cur_reg_str, cl_reg_cfg_tokens, &cur_reg) != 0) {
		Trace(TR_ERR, "cns registration failed %d %s", samerrno,
			samerrmsg);
		return (B_FALSE);
	} else {
		ret = cur_reg.registered;
	}
	free(cur_reg_str);
	free_cl_reg_cfg_fields(&cur_reg);

	return (ret);
}


static int
save_client_conf(char *filename, cl_reg_cfg_t *cl) {
	FILE	*f = NULL;
	int	fd;
	char	buf[80];
	boolean_t need_comma = B_FALSE;

	if (ISNULL(filename, cl)) {
		Trace(TR_ERR, "writing %s client config failed: %s",
			Str(filename), samerrmsg);
		return (-1);
	}

	if ((fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0600)) != -1) {
		f = fdopen(fd, "w");
	}
	if (f == NULL) {
		samerrno = SE_CFG_OPEN_FAILED;
		/* Open failed for %s: %s */
		StrFromErrno(samerrno, buf, sizeof (buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
			GetCustMsg(SE_CFG_OPEN_FAILED), filename, buf);

		Trace(TR_ERR, "writing %s failed: %s", filename, samerrmsg);
		return (-1);
	}


	if (cl->sun_login) {
		fprintf(f, "sun_login = %s", cl->sun_login);
		need_comma = B_TRUE;
	}
	if (cl->name) {
		need_comma = conditional_print(f, ",", need_comma);
		fprintf(f, "name = %s", cl->name);
	}

	if (cl->email) {
		need_comma = conditional_print(f, ",", need_comma);
		fprintf(f, "email = %s", cl->email);
	}
	if (cl->asset_prefix) {
		need_comma = conditional_print(f, ",", need_comma);
		fprintf(f, "asset_prefix = %s", cl->asset_prefix);
	}
	if (cl->asset_id) {
		need_comma = conditional_print(f, ",", need_comma);
		fprintf(f, "asset_id = %s", cl->asset_id);
	}
	if (cl->reg_id) {
		need_comma = conditional_print(f, ",", need_comma);
		fprintf(f, "reg_id = %s", cl->reg_id);
	}
	if (cl->registered) {
		need_comma = conditional_print(f, ",", need_comma);
		fprintf(f, "registered = %s", (cl->registered) ? "Y" : "N");
	}
	if (cl->prxy.enabled) {
		need_comma = conditional_print(f, ",", need_comma);
		fprintf(f, "prxy_enabled = %s", (cl->prxy.enabled) ? "Y" : "N");
	}
	if (cl->prxy.auth) {
		need_comma = conditional_print(f, ",", need_comma);
		fprintf(f, "prxy_auth = %s", (cl->prxy.auth) ? "Y" : "N");
	}

	if (cl->prxy.host) {
		need_comma = conditional_print(f, ",", need_comma);
		fprintf(f, "prxy_host = %s", cl->prxy.host);
	}

	if (cl->prxy.port) {
		need_comma = conditional_print(f, ",", need_comma);
		fprintf(f, "prxy_port = %s", cl->prxy.port);
	}

	if (cl->prxy.user) {
		need_comma = conditional_print(f, ",", need_comma);
		fprintf(f, "prxy_user = %s", cl->prxy.user);
	}
	fclose(f);

	Trace(TR_MISC, "wrote %s client config", filename);

	return (0);
}

/*
 * Get a public key to be used to generate a secret key to
 * encrypt passwords for the cns_register rpc call
 *
 * server_pub_key is a 1024 bit DH public key in a null-terminated hex
 * string. The lifecycle of this key is the same as the lifecycle of
 * the fsmgmtd. When fsmgmtd is restarted a new key is generated.
 *
 * signature is a digital signature calculated using the mgmt daemon's
 * private signature key. It should be used along with mgmt daemon's
 * public signature key by the client to verify that the
 * server_pub_key is from the daemon prior to trusting the server_pub_key.
 */
int
cns_get_public_key(
ctx_t *c,	/* ARGSUSED */
char **server_pub_key,
crypt_str_t **sig) {

	unsigned char *buf;
	int buf_len;
	int retval;

	Trace(TR_MISC, "get public key");
	if (ISNULL(server_pub_key, sig)) {
		Trace(TR_ERR, "getting public key failed failed %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}
	*sig = NULL;
	if ((retval = get_public_key(server_pub_key, &buf, &buf_len)) != 0) {
		set_registration_error(retval);
		Trace(TR_ERR, "getting public key failed failed %d %s",
		    samerrno, samerrmsg);

		return (-1);
	}
	*sig = (crypt_str_t *)mallocer(sizeof (crypt_str_t));
	if (*sig == NULL) {
		Trace(TR_ERR, "getting public key failed failed %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}
	(*sig)->str = buf;
	(*sig)->str_len = buf_len;
	Trace(TR_MISC, "got public key");
	return (0);
}

static boolean_t
conditional_print(FILE *f, char *str, boolean_t printit) {

	if (printit) {
		fprintf(f, str);
	}
	return (B_TRUE);
}


/*
 * There are two classes of errors that we can encounter during registration.
 * 1. Errors that the users might have induced or could fix
 *	1a. Curl/SSL problems (problem with data dir, bad proxy etc.
 *	1b. Authentication errors returned by the CSN servers.
 * 2. Internal errors.
 *
 * For errors of the first class we want to try to explain what might be
 * the problem so that they can fix it.
 *
 * For errors of the second class we want to give a general failure message
 * that indicates that registration is not required for them to use their
 * software. The specific (and more cryptic) errors should be traced in
 * the code that calls this function. This class of error will simply
 * result in a -1.
 */
static void
set_registration_error(int errcode) {
	if (errcode == 0) {
		return;
	} else if (errcode == -1) {
		/* General processing error */
		setsamerr(SE_CSN_PRODUCT_REG_FAILED);
	} else {
		setsamerr(errcode);
	}
}
