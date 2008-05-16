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

// ident	$Id: CommonViewBeanBase.java,v 1.47 2008/05/16 18:39:06 am143972 Exp $

package com.sun.netstorage.samqfs.web.util;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBean;
import com.iplanet.jato.view.ViewBeanBase;
import com.iplanet.jato.view.event.ChildContentDisplayEvent;
import com.iplanet.jato.view.event.JspEndChildDisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.web.ui.taglib.WizardWindowTag;
import com.sun.web.ui.taglib.html.CCButtonTag;
import com.sun.web.ui.view.alert.CCAlertInline;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;
import java.io.IOException;
import java.io.Serializable;
import java.util.Iterator;
import java.util.Map;
import javax.servlet.ServletException;
import javax.servlet.jsp.JspException;
import javax.servlet.jsp.tagext.Tag;

/**
 *  This class is the base class for all the viewbeans except wizards and
 *  pop ups.
 */

public class CommonViewBeanBase extends ViewBeanBase {

    /**
     * cc components from the corresponding jsp page(s)...
     */
    public static final String CHILD_COMMON_ALERT    = "Alert";

    /**
     * Refresh Href that is called when the Refresh button is clicked in
     * the masthead frame.
     */
    protected static final String REFRESH_HREF = "RefreshHref";

    private boolean alreadyDeserialized = false;

    /**
     * Constructor
     *
     * @param name of the page (Not needed for HELP button anymore)
     *  -> ViewBeanBase needs a name for the view so pageName is kept here
     * @param page display URL
     * @param name of tab
     */
    protected CommonViewBeanBase(
        String pageName,
        String displayURL) {
        super(pageName);

        // set the address of the JSP page
        setDefaultDisplayURL(displayURL);
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        registerChild(CHILD_COMMON_ALERT, CCAlertInline.class);
        registerChild(REFRESH_HREF, CCHref.class);
    }

    /**
     * Check the child component is valid
     *
     * @param name of child compoment
     * @return the boolean variable
     */
    protected boolean isChildSupported(String name) {
        if (name.equals(CHILD_COMMON_ALERT) ||
            name.equals(REFRESH_HREF)) {
            return true;
        } else {
            return false;
        }
    }

    /**
     * Instantiate each child view.
     *
     * @param name The name of the child view
     * @return View The instantiated child view
     */
    protected View createChild(String name) {
        // Alert Inline
        if (name.equals(CHILD_COMMON_ALERT)) {
            return new CCAlertInline(this, name, null);
        } else if (name.equals(REFRESH_HREF)) {
            return new CCHref(this, name, null);
        } else {
            // Should not come here
            return null;
        }
    }


    protected void deserializePageAttributes() {
        if (!alreadyDeserialized) {
            super.deserializePageAttributes();
        }
        alreadyDeserialized = true;
    }

    public boolean invokeRequestHandler() throws Exception {
        deserializePageAttributes();
        return super.invokeRequestHandler();
    }

    /**
     * since the CCActionTable doesn't recognize 'unknown' tags, use a
     * regular CCWizardWindowTag in the table XML and swap its HTML with
     * that our of special WizardWindowTag here.
     */
    public String endChildDisplay(ChildContentDisplayEvent ccde)
        throws ModelControlException {
        String childName = ccde.getChildName();

        String html = super.endChildDisplay(ccde);
        // if its one of our special wizards
        // i.e. name contains keyword "SamQFSWizard"
        if (childName.indexOf(Constants.Wizard.WIZARD_BUTTON_KEYWORD) != -1 ||
            childName.indexOf(Constants.Wizard.WIZARD_SCRIPT_KEYWORD) != -1) {
            // retrieve the html from a WizardWindowTag
            JspEndChildDisplayEvent event = (JspEndChildDisplayEvent)ccde;
            CCButtonTag sourceTag = (CCButtonTag)event.getSourceTag();
            Tag parentTag = sourceTag.getParent();

            // get the peer view of this tag
            View theView = getChild(childName);

            // instantiate the wizard tag
            WizardWindowTag tag = new WizardWindowTag();
            tag.setBundleID(sourceTag.getBundleID());
            tag.setDynamic(sourceTag.getDynamic());

            // Is this a script wizard tag?
            if (childName.indexOf(
                Constants.Wizard.WIZARD_SCRIPT_KEYWORD) != -1) {
                // This is a script wizard
                tag.setName(childName);
                tag.setWizardType(WizardWindowTag.TYPE_SCRIPT);
            }


            // try to retrieve the correct HTML from the tag
            try {
                html = tag.getHTMLStringInternal(parentTag,
                    event.getPageContext(), theView);
            } catch (JspException je) {
                throw new IllegalArgumentException("Error retrieving the " +
                    "WizardWIndowTag html : " + je.getMessage());
            }
        }
        return html;
    }

    /** return the name of the server currently being managed */
    public String getServerName() {
        String serverName = (String) getParentViewBean().
            getPageSessionAttribute(Constants.
                                    PageSessionAttributes.SAMFS_SERVER_NAME);
        if (serverName != null) {
            return serverName;
        }

        // Retrieve the server name from the hidden field in the navigator
        // frame when user clicks on the nodes in the tree
        // We cannot get the server name from the request here so I choose to
        // save the server name in the Navigator Frame and retrieve it from
        // there.  DO NOT attempt to retrieve the server name from the change
        // server drop down menu as it may not exist in a CIS setup.
        ViewBean navigationViewBean = getViewBean(FrameNavigatorViewBean.class);
        CCHiddenField field = (CCHiddenField)
            navigationViewBean.getChild(FrameNavigatorViewBean.SERVER_NAME);
        String value = (String) field.getValue();

        setPageSessionAttribute(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME,
            value);
        return value;
    }

    /**
     * Copies over the "server name" attribute, the page path and those
     * starting with Constants.PageSessionAttributes.PREFIX.
     *
     * @param pageSessionAttributesMap the page session map containing the
     * page session attributes to copy
     *
     * @param copyAttributes An array containing extra keys of the map entries
     * to copy.  These attributes will be copied in addition to
     * the standard ones mentioned above.  May be null if no extra copies
     * are desired.
     */
    protected void encodeSAMAttributes(Map pageSessionAttributesMap,
                                       String[] copyAttributes) {

        // Copy server name
        setPageSessionAttribute(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME,
            (Serializable) pageSessionAttributesMap.get(
                         Constants.PageSessionAttributes.SAMFS_SERVER_NAME));

        // Copy page path
        setPageSessionAttribute(
            Constants.SessionAttributes.PAGE_PATH,
            (Integer []) pageSessionAttributesMap.get(
                            Constants.SessionAttributes.PAGE_PATH));

        // Copy requested attributes
        if (copyAttributes != null) {
            for (int i = 0; i < copyAttributes.length; i++) {
                Serializable value = (Serializable)
                                pageSessionAttributesMap.get(copyAttributes[i]);
                setPageSessionAttribute(copyAttributes[i], value);
            }
        }

        // Copy all attributes that start with
        // Constants.PageSessionAttributes.PREFIX
        Iterator it = pageSessionAttributesMap.keySet().iterator();
        while (it.hasNext()) {
            String key = (String) it.next();

            /* Check if this is an attribute we are looking for */
            if (key.startsWith(Constants.PageSessionAttributes.PREFIX)) {
                Serializable value = (Serializable)
                                        pageSessionAttributesMap.get(key);
                setPageSessionAttribute(key, value);
            }
        }
    }

    public void forwardTo(ViewBean targetViewBean) {
        ((CommonViewBeanBase) targetViewBean).
            encodeSAMAttributes(getPageSessionAttributes(), null);
        ((CommonViewBeanBase) targetViewBean).forwardTo(getRequestContext());
    }

    /**
     * Performs a forward, carrying with it the page session attributes
     * whose keys are included in the pageSessionAttrubtes string array
     * in addition to the page session attrubtes normally carried forward
     * with calls to forwardTo.
     *
     * @param targetViewBean The destination view bean being forwarded to.
     * @param pageSessionAttributes An array keys for the page session
     * attributes to carry foward into the targetViewBean.  Do not include
     * those page session attrubtes that are normally carried forward with
     * calls to forwardTo.
     */
    public void forwardToWithAttributes(CommonViewBeanBase targetViewBean,
                                        String[] pageSessionAttributes) {
        targetViewBean.encodeSAMAttributes(getPageSessionAttributes(),
                                           pageSessionAttributes);
        targetViewBean.forwardTo(getRequestContext());

    }

    public void handleRefreshHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        getParentViewBean().forwardTo(getRequestContext());
    }
}
