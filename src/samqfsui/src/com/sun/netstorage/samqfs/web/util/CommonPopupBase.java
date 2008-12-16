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

// ident	$Id: CommonPopupBase.java,v 1.10 2008/12/16 00:12:25 am143972 Exp $

package com.sun.netstorage.samqfs.web.util;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.model.CCPropertySheetModel;
import com.sun.web.ui.view.html.CCHiddenField;
import javax.servlet.http.HttpServletRequest;


// For an example of the type of JSP page that works with this view bean,
// set the change directory or filter settings jsp pages under util.
public abstract class CommonPopupBase extends CommonSecondaryViewBeanBase {

    // Models
    protected CCPageTitleModel pageTitleModel = null;
    protected CCPropertySheetModel propertySheetModel = null;

    // Child View Names used to set Javascript Values
    public static final String CHILD_PARENT_FORM_NAME = "parentFormName";
    public static final
        String CHILD_PARENT_RETURN_VALUE_OBJ_NAME = "parentReturnValueObjName";
    public static final String CHILD_PARENT_SUBMIT_CMD = "parentSubmitCmd";
    public static final String CHILD_PROMPT_TEXT = "promptText";

    private String parentFormName;
    private String returnValueObjName;
    private String submitCmd;
    private String promptText;
    private String pageTitleText;
    private String loadValue;

    // Page session attribute keys / URL parameters
    public static final String PARENT_FORM_NAME = "parentFormName";
    public static final
        String PARENT_RETURN_VALUE_OBJ_NAME  = "parentReturnValueObjName";
    public static final String PARENT_SUBMIT_CMD = "parentSubmitCmd";
    public static final String PAGE_TITLE_TEXT   = "pageTitleText";
    public static final String PROMPT_TEXT = "promptText";
    public static final String LOAD_VALUE  = "loadValue";

    private String pageName;

    /**
     * Constructor
     */
    public CommonPopupBase(String pageName, String defaultDisplayUrl) {
        super(pageName, defaultDisplayUrl);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        this.pageName = pageName;

        // When the page is first displayed, there will be  a bunch of url
        // parameters.  These will be stored in page session attyributes.
        // Upon refresh, the parameters will not be available, so we
        // will obtain the data from the page session attributes.

        // Get url parameters
        HttpServletRequest httprq =
            RequestManager.getRequestContext().getRequest();
        parentFormName = (String) httprq.getParameter(PARENT_FORM_NAME);
        if (parentFormName != null) {
            setPageSessionAttribute(PARENT_FORM_NAME,  parentFormName);
        } else {
            parentFormName = (String) getPageSessionAttribute(PARENT_FORM_NAME);
        }

        returnValueObjName =
            (String) httprq.getParameter(PARENT_RETURN_VALUE_OBJ_NAME);
        if (returnValueObjName != null) {
            setPageSessionAttribute(
                PARENT_RETURN_VALUE_OBJ_NAME, returnValueObjName);
        } else {
            returnValueObjName =
                (String) getPageSessionAttribute(PARENT_RETURN_VALUE_OBJ_NAME);
        }

        submitCmd = (String) httprq.getParameter(PARENT_SUBMIT_CMD);
        if (submitCmd != null) {
            setPageSessionAttribute(PARENT_SUBMIT_CMD, submitCmd);
        } else {
            submitCmd = (String) getPageSessionAttribute(PARENT_SUBMIT_CMD);
        }

        // Page title text and prompt text are resource keys.
        String pageTitleTextKey = (String) httprq.getParameter(PAGE_TITLE_TEXT);
        if (pageTitleTextKey != null) {
            setPageSessionAttribute(PAGE_TITLE_TEXT, pageTitleTextKey);
        } else {
            pageTitleTextKey =
                (String) getPageSessionAttribute(PAGE_TITLE_TEXT);
        }
        pageTitleText = SamUtil.getResourceString(pageTitleTextKey);

        String promptTextKey = (String) httprq.getParameter(PROMPT_TEXT);
        if (promptTextKey != null) {
            setPageSessionAttribute(PROMPT_TEXT, promptTextKey);
        } else {
            promptTextKey = (String) getPageSessionAttribute(PROMPT_TEXT);
        }
        promptText = SamUtil.getResourceString(promptTextKey);

        // Load value is passed in by parent form.
        // It can also be set by the popup when refreshing the popup
        // (error during entry validation)
        loadValue = (String) httprq.getParameter(LOAD_VALUE);
        if (loadValue != null && loadValue.length() > 0) {
            // Value was passed in by parent
            setPageSessionAttribute(LOAD_VALUE, loadValue);
        } else {
            // Not passed in by parent, or this is a refresh.
            // Get from previous load if any.
            loadValue = (String) getPageSessionAttribute(LOAD_VALUE);
        }
        pageTitleModel = PageTitleUtil.createModel(getPageTitleXmlFile());
        propertySheetModel =
            PropertySheetUtil.createModel(getPropSheetXmlFile());
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    abstract public String getPageTitleXmlFile();
    abstract public String getPropSheetXmlFile();

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        super.registerChildren();
        registerChild(CHILD_PARENT_FORM_NAME, CCHiddenField.class);
        registerChild(CHILD_PARENT_RETURN_VALUE_OBJ_NAME, CCHiddenField.class);
        registerChild(CHILD_PARENT_SUBMIT_CMD, CCHiddenField.class);
        PageTitleUtil.registerChildren(this, pageTitleModel);
        PropertySheetUtil.registerChildren(this, propertySheetModel);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     *
     * @param name The name of the child view
     * @return View The instantiated child view
     */
    protected View createChild(String name) {
        TraceUtil.trace3("Entering");

        if (super.isChildSupported(name)) {
            TraceUtil.trace3("Exiting");
            return super.createChild(name);
        } else if (name.equals(CHILD_PARENT_FORM_NAME) ||
                   name.equals(CHILD_PARENT_RETURN_VALUE_OBJ_NAME) ||
                   name.equals(CHILD_PARENT_SUBMIT_CMD)) {
            TraceUtil.trace3("Exiting");
            return new CCHiddenField(this, name, this.parentFormName);
        // PageTitle Child
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            TraceUtil.trace3("Exiting");
            return PageTitleUtil.createChild(this, pageTitleModel, name);
        } else if (PropertySheetUtil.isChildSupported(propertySheetModel,
                                                      name)) {
            TraceUtil.trace3("Exiting");
            return PropertySheetUtil.createChild(
                                              this, propertySheetModel, name);
        } else {
            TraceUtil.trace3("Exiting");
            throw new IllegalArgumentException(
                "Invalid child name [" + name + "]");
        }
    }

    public String getLoadValue() {
        return this.loadValue;
    }

    /**
     * Called as notification that the JSP has begun its display processing
     * @param event The DisplayEvent
     */
    public void beginDisplay(DisplayEvent event) {
        TraceUtil.trace3("Entering");
        TraceUtil.trace3("Exiting");

        try {
            // store parent data in hidden objects for use later by
            // client side script
            if (this.parentFormName == null ||
                this.parentFormName.length() == 0) {
                // Development error
                throw new SamFSException(SamUtil.getResourceString(
                     "CommonPopup.error.urlParamError", "parentFormName"));
            }
            setDisplayFieldValue(CHILD_PARENT_FORM_NAME, this.parentFormName);
            if (this.returnValueObjName == null ||
                this.returnValueObjName.length() == 0) {
                // Development error
                throw new SamFSException(SamUtil.getResourceString(
                                "CommonPopup.error.urlParamError",
                                "parentReturnValueObjName"));
            }
            setDisplayFieldValue(CHILD_PARENT_RETURN_VALUE_OBJ_NAME,
                                 this.returnValueObjName);
            if (this.submitCmd == null ||
                this.submitCmd.length() == 0) {
                // Development error
                throw new SamFSException(SamUtil.getResourceString(
                                "CommonPopup.error.urlParamError",
                                "parentSubmitCmd"));
            }
            setDisplayFieldValue(CHILD_PARENT_SUBMIT_CMD,
                                 this.submitCmd);

            if (this.pageTitleText != null) {
                pageTitleModel.setPageTitleText(
                            SamUtil.getResourceString(this.pageTitleText));
            }
            if (this.promptText != null) {
                pageTitleModel.setPageTitleHelpMessage(
                            SamUtil.getResourceString(this.promptText));
            }
        } catch (SamFSException e) {
            SamUtil.setErrorAlert(
                this,
                ALERT,
                "CommonPopup.error.cantDisplay",
                e.getSAMerrno(),
                e.getMessage(),
                "");
        }

        TraceUtil.trace3("Exiting");
    }

}
