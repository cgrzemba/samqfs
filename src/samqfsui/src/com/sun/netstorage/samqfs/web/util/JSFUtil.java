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

// ident	$Id: JSFUtil.java,v 1.13 2008/12/16 00:12:26 am143972 Exp $

package com.sun.netstorage.samqfs.web.util;

import java.text.MessageFormat;
import java.util.Locale;
import java.util.ResourceBundle;
import java.util.MissingResourceException;
import javax.faces.context.ExternalContext;
import javax.faces.context.FacesContext;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpSession;
import javax.faces.component.UIViewRoot;
import javax.servlet.RequestDispatcher;
import javax.servlet.ServletContext;
import javax.servlet.http.HttpServletResponse;

/**
 *
 * Utility class for the JSF-based pages. This class is to JSF-based pages what
 * SamUtil is to JATO-based pages.
 *
 * NOTE: The methods in this class should only be called from JSF
 * pages. Otherwise, the FacesContext will not be initialized and most
 * if not all of the methods will fail.
 */
public class JSFUtil {
    public static final String SERVER_NAME =
        Constants.PageSessionAttributes.SAMFS_SERVER_NAME;

    public static final String BUNDLE_BASE_NAME =
        "com.sun.netstorage.samqfs.web.resources.Resources";

    // instances of this class are meaningless. Prevent it from being
    // instantiated
    private JSFUtil() {
    }

    /** fore-go  user level concurrency for now */
    // TODO:
    public static String getServerName() {
        String serverName = null;

        // 1. check in the request attribute
        // - same jsf request
        HttpServletRequest request = getRequest();
        serverName = request.getParameter(SERVER_NAME);

        // 2. retrieve from the session
        // - subsequent jsf call
        HttpSession session = request.getSession();
        if (serverName == null) {
            serverName = (String)session.getAttribute(SERVER_NAME);
        } else {
            session.setAttribute(SERVER_NAME, serverName);
        }

        return serverName;
    }

    /**
     * return the http request object for the user request currently being
     * handled.
     */
    public static HttpServletRequest getRequest() {
        HttpServletRequest req = null;
        FacesContext fcontext = FacesContext.getCurrentInstance();
        if (fcontext != null) {
            ExternalContext econtext = fcontext.getExternalContext();

            if (econtext != null) {
                req = (HttpServletRequest)econtext.getRequest();
            }
        }

        // request shouldn't ever be null unless something catastrophic happens
        // mid-stream.
        return req;
    }

    /**
     * convenience method to retrieve the response object associated with the
     * user request currently being handled
     */
    public static HttpServletResponse getResponse() {
        HttpServletResponse response = null;

        FacesContext fcontext = FacesContext.getCurrentInstance();
        if (fcontext != null) {
            ExternalContext econtext = fcontext.getExternalContext();

            if (econtext != null) {
                response = (HttpServletResponse)econtext.getResponse();
            }
        }

        return response;
    }

    /** return the users current locale */
    public static Locale getLocale() {
        HttpServletRequest request = getRequest();
        Locale locale = null;

        // first check in the request object
        if (request != null) {
            locale = request.getLocale();
        }

        // next check the view root
        if (locale == null) {
            UIViewRoot root = FacesContext.getCurrentInstance().getViewRoot();
            if (root != null)
                locale = root.getLocale();
        }

        // finally return the default locale.
        if (locale == null)
            locale = Locale.getDefault();

        return locale;
    }

    // The following methods are using to resolve resource bundle keys in java
    // classes. NOTE: its not necessary to use these methods when resolving
    // keys in JSPs, instead use the JSTL <code><f:loadBundle .../></code> tag.

    public static String getMessage(String key) {
        return getMessage(key, new Object[] {null});
    }

    public static String getMessage(String key, String arg) {
        return getMessage(key, new Object [] {arg});
    }

    public static String getMessage(String key, Object [] arg) {
        // try to open the resource bundle
        ResourceBundle bundle =
	    ResourceBundle.getBundle(BUNDLE_BASE_NAME, getLocale());
        if (bundle == null) {
            throw new NullPointerException("Could not locate the resource "
					   + "bundle '"
					   + BUNDLE_BASE_NAME
					   + "'");
            // TODO: log this and return the key
        }

        // try to resolve the given key.
        String message = null;
        try {
            message = bundle.getString(key);
        } catch (MissingResourceException mre) {
            TraceUtil.trace1("Missing Resource Exception caught! key: " + key);
            LogUtil.error("Missing Resource Exception caught! key: " + key);
            return key;
        } catch (NullPointerException nullEx) {
            // code falls into this case if the key is not found
            return key;
        }
        return formatMessage((message != null ? message : key), arg);
    }

    /**
     * format a string message by substituting the provided arguemnts.
     */
    public static String formatMessage(String msg, Object [] arg) {
        String formattedMessage = null;
        if (arg != null && arg.length != 0) {
            try {
                MessageFormat messageFormat = new MessageFormat(msg);
                formattedMessage = messageFormat.format(arg);
            } catch (NullPointerException npe) {
                TraceUtil.trace1(
                    "Null Pointer Exception caught in formatMessage:" +
                    "msg=" + msg, npe);
            }
        }

        return formattedMessage == null ? msg : formattedMessage;
    }

    /** for now store values in session */
    public static void setAttribute(String name, Object value) {
        HttpSession session = getRequest().getSession();
        if (name != null)
            session.setAttribute(name, value);
    }

    /** for now, store values in session */
    public static Object getAttribute(String name) {
        HttpSession session = getRequest().getSession();
        Object value = null;

        if (name != null)
            value = session.getAttribute(name);

        return value;
    }

    public static void removeAttribute(String name) {
        HttpSession session = getRequest().getSession();

        if (name != null)
            session.removeAttribute(name);
    }

    /**
     * Used by JSF page when it's needed to forward to a JATO based page, or
     * used by a JSF page to redirect to another JSF page when it's appropriate.
     * @param url
     * @param params
     */
    public static void redirectPage(String url, String params) {
        FacesContext context = FacesContext.getCurrentInstance();
        ExternalContext ec = context.getExternalContext();
        HttpServletRequest request =
            (HttpServletRequest) ec.getRequest();
        HttpServletResponse response =
            (HttpServletResponse) ec.getResponse();

        params = params == null ? "" : "?" + params;
        // Get the context path.
        String targetURL = request.getContextPath() + url + params;

        try {
            TraceUtil.trace2("Forward to URL: " + targetURL);
            // Redirect to the particular JATO page
            response.sendRedirect(targetURL);

            // Skip the JSF cycle.
            context.responseComplete();

        } catch (Exception se) {
            // handle the exception
        }
    }

    /**
     * Used by a JATO based page to forward to JSF based page.
     * @param vb
     * @param url
     * @param params
     */
    public static void forwardToJSFPage(
            CommonViewBeanBase vb, String url, String params) {
        HttpServletRequest request = vb.getRequestContext().getRequest();
        HttpServletResponse response =
            vb.getRequestContext().getResponse();

        params = params == null ? "" : "?" + params;
        String targetURL = url + params;

        TraceUtil.trace2("Forward to URL: " + targetURL);

        ServletContext sc = vb.getRequestContext().getServletContext();
        RequestDispatcher rd = sc.getRequestDispatcher(targetURL);

        try {
            rd.forward(request, response);
        } catch (Exception se) {
            TraceUtil.trace1(
            "Exception caught while forwarding back to JSF Page " + url,
                se);
            se.printStackTrace();
            System.out.println(
                "Exception caught while forwarding back to JSF Page. url: " +
                url + " params: " + params);
            vb.forwardTo(vb.getRequestContext());
        }
    }
}
