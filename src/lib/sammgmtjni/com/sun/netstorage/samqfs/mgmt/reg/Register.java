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

// ident	$Id: Register.java,v 1.9 2008/12/16 00:08:57 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.reg;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import javax.crypto.Cipher;
import javax.crypto.SecretKey;
import javax.crypto.spec.IvParameterSpec;

/**
 * This class encapsulates the registration functionality for CSN Registration.
 */
public final class Register {

    /**
     *	@return key value string in the same fromat as that required
     *	required by the register call.
     */
    public static String getRegistration(Ctx c) throws SamFSException {
	return (getRegistration(c, SAMQFS_ASSET_PREFIX));
    }


    /*
     * register SAM/QFS on the host being called with CNS.
     * The key value string must contain:
     * sun_login
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
     *
     * sun_password (passed as arg clientPass)
     * prxy_passwd(passed as arg proxyPass)
     *
     * The clientPass byte array must contain the client password for
     * the sun account
     *
     * If proxy.authenticate is set to true, the proxyPass must be
     * the encrypted proxy password, using the same key as above.
     *
     * The arguments clientPass and proxyPass are byte arrays so that
     * they can be zero'd after use.
     */
    public static void register(Ctx c, String kv_string, byte clientPass[],
			   byte proxyPass[]) throws SamFSException {

	/*
	 * get the public key and signature from the server and verify
	 * that the signature was generated with the known public key
	 */
	SimpleSignature sign = getPublicKey(c);
	sign.verifySignature();

	DH dh = new DH();
	SecretKey sk = dh.generateSecret(sign.getData(), "DESede");


        /*
         * encrypt, using DES in ECB mode
         */
	byte[] ciphertext1 = null;
	byte[] ciphertext2 = null;
	try {
	    Cipher jniCipher = Cipher.getInstance("DESede/CBC/PKCS5Padding");

	    IvParameterSpec iv = new IvParameterSpec(IV);

	    jniCipher.init(Cipher.ENCRYPT_MODE, sk, iv);

	    ciphertext1 = jniCipher.doFinal(clientPass);

	    // Don't try to encrypt if the proxy pass is null
	    if (proxyPass != null) {
		ciphertext2 = jniCipher.doFinal(proxyPass);
	    }

	} catch (Exception e) {
	    throw new SamFSException("Unable to secure passwords. " +
                                     "Registration will not be attempted", e);
	}

	register(c, kv_string, ciphertext1, ciphertext2,
		 dh.getPublicKey().toString(16));

    }


    private static final byte IV[] = {
	(byte)0xa7, (byte)0xb8, (byte)0xf8, (byte)0x02,
	(byte)0xbc, (byte)0xf8, (byte)0x45, (byte)0x2f
    };


    private static final String SAMQFS_ASSET_PREFIX = "sam-qfs";

    private static native String getRegistration(Ctx c, String asset_prefix)
	throws SamFSException;

    private static native void register(Ctx c, String kv_string,
	byte cl_passwd[], byte proxyPasswd[], String localPublicKey)
	throws SamFSException;

    private static native SimpleSignature getPublicKey(Ctx c)
	throws SamFSException;

}
