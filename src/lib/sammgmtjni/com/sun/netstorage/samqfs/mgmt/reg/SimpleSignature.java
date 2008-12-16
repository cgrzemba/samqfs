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

// ident	$Id: SimpleSignature.java,v 1.7 2008/12/16 00:08:57 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.reg;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import java.math.BigInteger;
import java.security.spec.RSAPublicKeySpec;
import java.security.Signature;
import java.security.SignatureException;
import java.security.KeyFactory;
import java.security.PublicKey;

public final class SimpleSignature {

    private String data;
    private byte signature[] = null;
    private PublicKey verificationKey = null;


    public SimpleSignature(String data, byte signature[]) {
	this.data = data;
	this.signature = signature;
    }

    /* Set a verification key other than the default */
    public void setVerificationKey(PublicKey verificationKey) {
	this.verificationKey = verificationKey;
    }

    public String getData() { return data; }

    /*
     * Throws SamFSException if the signature does not match or
     * if it is not possible to setup the signature verification.
     */
    public void verifySignature() throws SamFSException {
	Signature sig;
	if (signature == null) {
	    throw new SamFSException("The public key was not signed. " +
                                     "Registration not attempted");
	}

	try {
	    if (verificationKey == null) {
		/* use the default verification key */
		BigInteger exp = new BigInteger("010001", 16);

		/*
		 * The public verification key for the server. This
		 * hex string represents the hex value for the 1014 bit
		 * public key in the  file /etc/opt/SUNWsamfs/cns/key.pub
		 */
		BigInteger mod =
                    new BigInteger(
                        "A4A5C06690082B61492DF39930A3B39E" +
                        "33DD15EAD380AE27DB057E7B72CDF9CC" +
                        "09B8FAE37C399527D769852F95811F08" +
                        "26B5E390E89D09CD6377088BDF05F7E8" +
                        "25931D4C2BDB7DD3BE16DBF47713843A" +
                        "569B8F02F879311BDBFF6339EBAD7BC0" +
                        "3360776ADEEE0BCF48BCE5E3D848D04E" +
                        "D772DC8DD78BFAF57706BD63B00C34B5",
                        16);

		RSAPublicKeySpec spec = new RSAPublicKeySpec(mod, exp);

		KeyFactory kf = KeyFactory.getInstance("RSA");
		verificationKey = kf.generatePublic(spec);
	    }
	    sig = Signature.getInstance("SHA1withRSA");
	    sig.initVerify(verificationKey);
	    sig.update(this.getData().getBytes());

	} catch (Exception e) {
	    throw new SamFSException("Signature verification failed." +
		" Registration not attempted", e);
	}

	try {
	    if (!sig.verify(signature)) {
		throw new SamFSException("Signature verification failed." +
		    " Registration not attempted");
	    }
	} catch (SignatureException se) {
	    throw new SamFSException("Signature verification failed." +
		" Registration not attempted", se);
	}
    }

    public String toString() {
	String s = "Signature:data = " + ((data != null) ? data : "null") +
	    "signlen = " +  ((signature == null)? signature.length:0) +
	    "verificationKey = " +
	    ((verificationKey != null) ? "is set" : "null");
	return (s);
    }
}
