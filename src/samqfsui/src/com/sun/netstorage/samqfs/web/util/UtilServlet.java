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

// ident	$Id: UtilServlet.java,v 1.14 2008/10/02 03:00:27 ronaldso Exp $


package com.sun.netstorage.samqfs.web.util;

import com.iplanet.jato.RequestContext;
import com.iplanet.jato.RequestContextImpl;
import com.iplanet.jato.ViewBeanManager;
import com.sun.netstorage.samqfs.web.model.SamQFSAppModel;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSSystemModelImpl;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSUtil;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.util.HashMap;
import javax.servlet.ServletConfig;
import javax.servlet.ServletContext;
import javax.servlet.ServletException;

/**
 * Util Servlet class is for administration module
 */
public class UtilServlet extends AppServletBase {
    /**
     * Default module URL name
     */
    public static final String DEFAULT_MODULE_URL = "../util";

    /**
     * Package name for the Samfs/Qfs servlet
     */
    public static String PACKAGE_NAME =
        getPackageName(UtilServlet.class.getName());

    /**
     * Constructor
     */
    public UtilServlet() {
        super();
    }

    /**
     * initialize the servlet. Parse the host.conf file if it has not been
     * parsed yet
     */
    public void init(ServletConfig config) throws ServletException {
        super.init(config);

        initSystemModels(config.getServletContext());
    }

    /** if the host.conf file has not been parsed, parse it */
    protected synchronized void initSystemModels(ServletContext sc) {
        /* if the host.conf file has not been parsed yet, parse it here */
        HashMap hostModelMap =
            (HashMap)sc.getAttribute(Constants.sc.HOST_MODEL_MAP);
        if (hostModelMap == null) {
            hostModelMap = new HashMap();
            try {
                String realPath =
                    sc.getRealPath(SamQFSAppModel.hostFileLocation);
                File file = new File(realPath);

                BufferedReader in =
                    new BufferedReader(new FileReader(file));

                String hostName = null;
                if (in != null) {
                    while ((hostName = in.readLine()) != null) {
                        if (SamQFSUtil.isValidString(hostName)) {
                            hostModelMap.put(
                                hostName,
                                new SamQFSSystemModelImpl(hostName));
                        }
                    } // end while
                } // end if
                in.close();
            } catch (Exception e) {
                TraceUtil.trace1(
                    "Error reading host.conf : " + e.getMessage());
            }
        }

        // save the host model map
        sc.setAttribute(Constants.sc.HOST_MODEL_MAP, hostModelMap);
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
} // end UtilServlet
