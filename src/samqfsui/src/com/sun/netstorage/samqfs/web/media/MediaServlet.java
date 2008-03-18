/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

// ident	$Id: MediaServlet.java,v 1.8 2008/03/17 14:43:40 am143972 Exp $


package com.sun.netstorage.samqfs.web.media;

import com.iplanet.jato.RequestContext;
import com.iplanet.jato.ViewBeanManager;
import com.iplanet.jato.RequestContextImpl;

import com.sun.netstorage.samqfs.web.util.AppServletBase;

/**
 * This Servlet class is for media module
 */
public class MediaServlet extends AppServletBase {
    /**
     * Default module URL name
     */
    public static final String DEFAULT_MODULE_URL = "../media";

    /**
     * Package name for the media servlet
     */
    public static String PACKAGE_NAME =
        getPackageName(MediaServlet.class.getName());

    /**
     * Constructor
     */
    public MediaServlet() {
        super();
    }

    /**
     * Set a view bean manager in the request context.  This must be
     * done at the module level because the view bean manager is
     * module specific.
     *
     * @param requestContext The request context
     */
    protected void initializeRequestContext(RequestContext requestContext) {
        super.initializeRequestContext(requestContext);

        ViewBeanManager viewBeanManager =
            new ViewBeanManager(requestContext, PACKAGE_NAME);
        ((RequestContextImpl)requestContext).
                setViewBeanManager(viewBeanManager);
    }

    /**
     * The superclass can be configured from init params specified at
     * deployment time.  If the superclass has been configured with
     * a different module URL, it will return a non-null value here.
     * If it has not been configured with a different URL, we use our
     * (hopefully) sensible default.
     *
     * @return String Name of the module URL
     */
    public String getModuleURL() {
        String result = (super.getModuleURL() != null)
            ? super.getModuleURL()
            : DEFAULT_MODULE_URL;
        return result;
    }
} // end MediaServlet
