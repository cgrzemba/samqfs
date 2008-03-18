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

// ident	$Id: RemoteFileChooserServlet.java,v 1.6 2008/03/17 14:43:53 am143972 Exp $

package com.sun.netstorage.samqfs.web.remotefilechooser;

import com.iplanet.jato.RequestContext;
import com.iplanet.jato.RequestContextImpl;
import com.iplanet.jato.ViewBeanManager;

import com.sun.web.ui.servlet.common.TagsServletBase;

/**
 * The filechooser servlet.
 *
 */
public class RemoteFileChooserServlet extends TagsServletBase {
    public static final String DEFAULT_MODULE_URL = "../remotefilechooser";

    public static String PACKAGE_NAME =
        getPackageName(RemoteFileChooserServlet.class.getName());

    // Constructor
    public RemoteFileChooserServlet() {
        super();
    }

    protected void initializeRequestContext(RequestContext requestContext) {
        super.initializeRequestContext(requestContext);

        // Set a view bean manager in the request context.  This must be
        // done at the module level because the view bean manager is
        // module specifc.
        ViewBeanManager viewBeanManager =
            new ViewBeanManager(requestContext, PACKAGE_NAME);
        ((RequestContextImpl)requestContext).
            setViewBeanManager(viewBeanManager);
    }

    public String getModuleURL() {
        return DEFAULT_MODULE_URL;
    }
}
