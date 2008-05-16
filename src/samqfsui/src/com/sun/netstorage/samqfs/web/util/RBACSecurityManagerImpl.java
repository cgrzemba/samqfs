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

// ident	$Id: RBACSecurityManagerImpl.java,v 1.9 2008/05/16 18:39:07 am143972 Exp $

package com.sun.netstorage.samqfs.web.util;

import com.iplanet.jato.RequestManager;
import com.iplanet.sso.SSOException;
import com.iplanet.sso.SSOToken;
import com.iplanet.sso.SSOTokenManager;
import com.sun.management.services.authorization.AuthorizationException;
import com.sun.management.services.authorization.AuthorizationService;
import com.sun.management.services.authorization.AuthorizationServiceFactory;
import com.sun.management.services.authorization.SolarisRbacPermission;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import javax.security.auth.Subject;

/**
 * default implementation for the security manager. This implementation checks
 * for authorizations based on the Solaris RBAC module provided by
 * Lockhart.
 */
public class RBACSecurityManagerImpl implements SecurityManager {

    /**
     * an intermediate holding variable for the authorizations the currently
     * logged in user has.
     */
    private int currentAuthorization;


    /**
     * the name of the logged in user
     */
    private String userName;

    // temp
    private String authorizationString;

    /**
     * package protected only constructor. Only SecurityManagerFactory is
     * allowed to instantiate this class.
     */
    RBACSecurityManagerImpl() {
        initAuthorization();
    }

    /**
     * read the /etc/auth_attr file to determine the authorizatioins for the
     * currently logged in user. - note: this process is performed once per
     * login session.
     */
    protected void initAuthorization() {
        // read the /etc/auth_attr file to determine the authorization level
        Authorization [] allAuths = {Authorization.CONFIG,
                                     Authorization.MEDIA_OPERATOR,
                                     Authorization.SAM_CONTROL,
                                     Authorization.FILE_OPERATOR,
                                     Authorization.FILESYSTEM_OPERATOR};

        this.currentAuthorization = 0;

        try { // being try
            SSOTokenManager tokenManager = SSOTokenManager.getInstance();
            SSOToken token =
                tokenManager.createSSOToken(RequestManager.getRequest());

            // retrieve the username
            this.userName = token.getPrincipal().getName();

            // NOTE: conflict Access Manager
            // watch out for lockhart fix
            Subject subject = token.getSubject();
            AuthorizationService authService = AuthorizationServiceFactory.
                getAuthorizationService("SolarisRbac");

            if (authService == null) {
                throw new SamFSException(null, -2200);
            }

            // loop through all the authorizations and update current
            // authorization with the authorizations for the currently logged
            // in user
            for (int i = 0; i < allAuths.length; i++) { // foreach auth
                SolarisRbacPermission permission =
                    new SolarisRbacPermission(allAuths[i].toString());
                if (authService.checkPermission(subject, permission)) {
                    this.currentAuthorization |= allAuths[i].intValue();
                    if (authorizationString == null) {
                        authorizationString =
                            SamUtil.getResourceString(allAuths[i].toString());
                    } else {
                        authorizationString =
                            authorizationString.concat(":").
                                concat(SamUtil.getResourceString(
                                            allAuths[i].toString()));
                    }
                } // end if
            } // end for
        } catch (SSOException ssoe) {
            LogUtil.error(this.getClass(),
                          "initAuthorization",
                          "Error while retrieving the SSOToken");
        } catch (AuthorizationException ae) {
            LogUtil.error(this.getClass(),
                          "initAuthorization",
                          "Error while retrieve RBAC authorizations");
        } catch (SamFSException sfe) {
            LogUtil.error(this.getClass(),
                          "initAuthorization",
                          "Unable to retrieve the authorization service");
        }
    }

    /**
     * check if the currently logged in user [role] has the <code>auth</code>
     * authorization.
     *
     * @param Authorization auth -
     * @return boolean - true if the currently logged in user has the specified
     * authorization else false
     */
    public boolean hasAuthorization(Authorization auth) {
        // check if the supplied authorization matches any of the allowed
        // authorizations for the currently logged in user.
        return (this.currentAuthorization & auth.intValue()) == auth.intValue();
    }

    public String getCurrentAuthorization() {
        return authorizationString;
    }

    /**
     * retrieve the username of the currently logged in user
     */
    public String getUserName() {
        return this.userName;
    }
}
