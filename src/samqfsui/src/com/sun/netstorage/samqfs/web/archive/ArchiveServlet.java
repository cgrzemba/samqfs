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

// ident	$Id: ArchiveServlet.java,v 1.8 2008/03/17 14:40:43 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.RequestContext;
import com.iplanet.jato.RequestContextImpl;
import com.iplanet.jato.ViewBeanManager;
import com.sun.netstorage.samqfs.web.util.AppServletBase;
import com.sun.netstorage.samqfs.web.util.TraceUtil;

/**
 * Servlet for the archive module.
 */
public class ArchiveServlet extends AppServletBase {
    public static final String DEFAULT_MODULE_URL = "../archive";

    public static String PACKAGE_NAME =
        getPackageName(ArchiveServlet.class.getName());

    public ArchiveServlet() {
        super();
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        TraceUtil.trace3("Exiting");
    }

    /**
     * Set a view bean manager in the request context.  This must be
     * done at the module level because the view bean manager is
     * module specific.
     *
     * @param requestContext The request context
     */
    protected void initializeRequestContext(RequestContext requestContext) {
        TraceUtil.trace3("Entering");
        super.initializeRequestContext(requestContext);

        ViewBeanManager viewBeanManager =
            new ViewBeanManager(requestContext, PACKAGE_NAME);
        ((RequestContextImpl)requestContext).
	    setViewBeanManager(viewBeanManager);
        TraceUtil.trace3("Exiting");
    }

    public String getModuleURL() {
        TraceUtil.trace3("Entering");
        String result = (super.getModuleURL() != null)
            ? super.getModuleURL()
            : DEFAULT_MODULE_URL;

        TraceUtil.trace3("Exiting");
        return result;
    }
}	
