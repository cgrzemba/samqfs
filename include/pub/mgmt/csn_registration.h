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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */
#ifndef _CNS_REGISTRATION_H
#define	_CNS_REGISTRATION_H

#pragma ident   "$Revision: 1.6 $"


/*
 * Encrypted strings are not null termintated. This struct can be used
 * to hold non-null termintated strings and ease transformations in
 * rpc and jni. This is not required for transporing public key material
 * that can be readily represented as a null terminated hex string.
 */
typedef struct crypt_str {
	unsigned char *str;
	int str_len;
} crypt_str_t;

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
cns_get_registration(ctx_t *c, char *product, char **reg_kv);


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
 * prxy_authenticate
 * prxy_user
 * prxy_passwd(passed as arg proxy_pwd)
 *
 * The cl_pwd byte array must contain the client password for
 * the sun account encrypted with the public key returned from
 * cns_get_public_key.
 *
 * If proxy.authenticate is set to true, the proxy_pwd must be
 * the encrypted proxy password, using the same key as above.
 *
 */
int
cns_register(ctx_t *ctx, char *kv_string, crypt_str_t *cl_pwd,
    crypt_str_t *proxy_pwd, char *clnt_pub_key_hex);



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
cns_get_public_key(ctx_t *c, char **server_pub_key_hex,
    crypt_str_t **signature);


/*
 * zeros the string and then frees the associated memory for the string
 * and the struct.
 */
void free_crypt_str(crypt_str_t *cs);


#endif /* _CNS_REGISTRATION_H */
