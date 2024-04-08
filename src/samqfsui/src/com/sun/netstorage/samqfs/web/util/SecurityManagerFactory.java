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

// ident	$Id: SecurityManagerFactory.java,v 1.8 2008/12/16 00:12:27 am143972 Exp $

package com.sun.netstorage.samqfs.web.util;


import javax.servlet.http.HttpSession;

public class SecurityManagerFactory {

    /** the only instance of the security manager */
    private static SecurityManagerFactory factory;

    /** key to store the security manager in the http session */
    private static final String SECURITY_MANAGER = "fsmgr.sm.session.key";

    /**
     * private constructor for the security manager factory to prevent
     * multiple instantiations
     */
    private SecurityManagerFactory() {
    }

    /**
     * the only external function to retrieve the relevant security manager
     */
    public static SecurityManager getSecurityManager() {
        if (factory == null) {
            factory = new SecurityManagerFactory();
        }

        return factory.retrieveSecurityManager();
    }

    protected SecurityManager retrieveSecurityManager() {
        HttpSession session = SamUtil.getCurrentRequest().getSession();
        SecurityManager manager =
            (SecurityManager) session.getAttribute(SECURITY_MANAGER);

        if (manager == null) {
            // instantiate the RBAC version of the security manager - future
            // imlementations can instantiate the Acess Manager based security
            // manager here instead
            manager = new RBACSecurityManagerImpl();
            session.setAttribute(SECURITY_MANAGER, manager);
        }

        return manager;
    }
}
