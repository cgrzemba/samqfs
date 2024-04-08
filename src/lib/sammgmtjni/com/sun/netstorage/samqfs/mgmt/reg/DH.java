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

// ident	$Id: DH.java,v 1.7 2008/12/16 00:08:57 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.reg;

import java.math.BigInteger;
import com.sun.netstorage.samqfs.mgmt.SamFSException;

import java.security.PublicKey;
import java.security.KeyFactory;
import java.security.KeyPair;
import java.security.KeyPairGenerator;
import javax.crypto.KeyAgreement;
import javax.crypto.SecretKey;
import javax.crypto.spec.DHParameterSpec;
import javax.crypto.interfaces.DHPublicKey;
import javax.crypto.spec.DHPublicKeySpec;

public final class DH {

    public DH() {
	this.modulus = defaultModulus;
	this.base = defaultBase;
    }

    private BigInteger modulus = null;
    private BigInteger base = null;
    private BigInteger publicKey = null;

    public BigInteger getPublicKey() { return publicKey; }

    /*
     * Using the DH Parmeters and the provided public key from the server
     * generate the shared secret using the algorithm specified in secretAlg.
     */
    public SecretKey generateSecret(String servPubHex, String secretAlg)
	throws SamFSException {

	SecretKey secretKey = null;

	try {
	    // First convert the serverPublicHex to a BigInteger for use as y
	    BigInteger servPubBI = new BigInteger(servPubHex, 16);

	    // use pre-arranged, default DH parameters
	    DHParameterSpec dhParams = new DHParameterSpec(defaultModulus,
                                       defaultBase);


	    // create DH key pair, using the default DH parameters
	    KeyPairGenerator jniKpairGen = KeyPairGenerator.getInstance("DH");
	    jniKpairGen.initialize(dhParams);
	    KeyPair jniKpair = jniKpairGen.generateKeyPair();

	    /*
	     * create and initialize DH KeyAgreement object. Note that
	     * the private key is passed to init because it contains
	     * all of the necessary parameters.
	     */
	    KeyAgreement jniKeyAgree = KeyAgreement.getInstance("DH");
	    jniKeyAgree.init(jniKpair.getPrivate());


	    /*
	     * Caller will need to return its public key to the
	     * other party so set the object variable public key.
	     */
	    this.publicKey = ((DHPublicKey)jniKpair.getPublic()).getY();


	    /*
	     * We have to do some gymnastics to get server key in the
	     * right format. Instantiate a public key spec to hold the
	     * servers public key using the unencoded key bytes
	     * from our dh params object and the servers public key.
	     */
	    BigInteger cY = servPubBI;
	    BigInteger cP = dhParams.getP();
	    BigInteger cG = dhParams.getG();
	    DHPublicKeySpec cdhkeyspec = new DHPublicKeySpec(cY, cP, cG);
	    KeyFactory ckf = KeyFactory.getInstance("DH");
	    PublicKey cPubKey = ckf.generatePublic(cdhkeyspec);

	    /*
	     * Once you generate a secret you will have to doPhase again
	     * to regenerate it. For this class that means a second call to
	     * generateSecret.
	     */
	    jniKeyAgree.doPhase(cPubKey, true);
	    secretKey = jniKeyAgree.generateSecret(secretAlg);

	} catch (Exception e) {
	    throw new SamFSException("Key agreement failed." +
		" Unable to securely complete registration");
	}

	return (secretKey);
    }

    /*
     * DH Params shared with the c code. These parameter values were
     * generated with openssl dhparams command
     */
    private static final byte dh1024_p[] = {
	(byte)0x93, (byte)0xF6, (byte)0x86, (byte)0x83, (byte)0x78,
	(byte)0xE7, (byte)0x43, (byte)0x17, (byte)0xD0, (byte)0x04,
	(byte)0xEA, (byte)0x5A, (byte)0xF7, (byte)0x75, (byte)0x02,
	(byte)0x30, (byte)0x7A, (byte)0x65, (byte)0x96, (byte)0x5D,
	(byte)0x05, (byte)0x26, (byte)0x93, (byte)0x3E, (byte)0xC9,
	(byte)0x18, (byte)0x64, (byte)0x5C, (byte)0x5C, (byte)0x9B,
	(byte)0x10, (byte)0xAE, (byte)0x55, (byte)0xFB, (byte)0x0A,
	(byte)0x3C, (byte)0x2B, (byte)0x0C, (byte)0x75, (byte)0x84,
	(byte)0xF2, (byte)0xF9, (byte)0x11, (byte)0x5F, (byte)0x96,
	(byte)0x66, (byte)0xCD, (byte)0x2A, (byte)0x21, (byte)0x92,
	(byte)0x66, (byte)0xE4, (byte)0xBE, (byte)0x99, (byte)0xB7,
	(byte)0x2D, (byte)0x04, (byte)0x4F, (byte)0x56, (byte)0x23,
	(byte)0x40, (byte)0xC7, (byte)0xEE, (byte)0x96, (byte)0x98,
	(byte)0xB1, (byte)0xFF, (byte)0xBD, (byte)0xF3, (byte)0x40,
	(byte)0xAB, (byte)0xE5, (byte)0x2F, (byte)0x21, (byte)0xD1,
	(byte)0x15, (byte)0x87, (byte)0x4C, (byte)0xC4, (byte)0xAE,
	(byte)0xF1, (byte)0x56, (byte)0xAF, (byte)0x07, (byte)0x5F,
	(byte)0xD5, (byte)0x48, (byte)0xA9, (byte)0x50, (byte)0x3B,
	(byte)0x30, (byte)0x50, (byte)0xDA, (byte)0xD5, (byte)0x83,
	(byte)0x83, (byte)0xEE, (byte)0x5F, (byte)0x5A, (byte)0x93,
	(byte)0xC4, (byte)0x26, (byte)0x54, (byte)0xA0, (byte)0xD4,
	(byte)0x89, (byte)0xCF, (byte)0xEF, (byte)0xE4, (byte)0x3C,
	(byte)0x00, (byte)0x35, (byte)0x23, (byte)0xF7, (byte)0x14,
	(byte)0x35, (byte)0xEC, (byte)0xC2, (byte)0x0A, (byte)0xD7,
	(byte)0xCC, (byte)0xFD, (byte)0x68, (byte)0x61, (byte)0xE1,
	(byte)0x0D, (byte)0xB0, (byte)0xFB
    };

    private static final BigInteger defaultModulus =
	new BigInteger(1, dh1024_p);

    private static final BigInteger defaultBase = BigInteger.valueOf(2);

}
