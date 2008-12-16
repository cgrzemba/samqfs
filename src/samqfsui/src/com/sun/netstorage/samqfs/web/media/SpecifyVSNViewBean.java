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

// ident	$Id: SpecifyVSNViewBean.java,v 1.23 2008/12/16 00:12:14 am143972 Exp $

package com.sun.netstorage.samqfs.web.media;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.media.Library;

import com.sun.netstorage.samqfs.web.util.CommonSecondaryViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.LogUtil;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;

import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.html.CCTextField;
import java.io.IOException;
import javax.servlet.ServletException;


/**
 *  This class is the view bean for the Specify VSN pop up page
 */

public class SpecifyVSNViewBean extends CommonSecondaryViewBeanBase {

    // The "logical" name for this page.
    public static final String PAGE_NAME = "SpecifyVSN";
    private static final String DEFAULT_DISPLAY_URL =
        "/jsp/media/SpecifyVSN.jsp";

    // Page Title Attributes and Components.
    private CCPageTitleModel pageTitleModel = null;

    // Child view names (i.e. display fields).
    public static final String CHILD_RADIOBUTTON1 = "RadioButton1";
    public static final String CHILD_RADIOBUTTON2 = "RadioButton2";
    public static final String CHILD_START_TEXT   = "StartText";
    public static final String CHILD_START_TEXTFIELD = "StartTextField";
    public static final String CHILD_END_TEXT = "EndText";
    public static final String CHILD_END_TEXTFIELD = "EndTextField";
    public static final String CHILD_ONEVSN_TEXT = "OneVSNText";
    public static final String CHILD_ONEVSN_TEXTFIELD  = "OneVSNTextField";
    public static final String CHILD_ONEVSN_INSTR_TEXT = "OneVSNInstrText";

    // Child name for Error string that used in Javascript
    public static final String CHILD_HIDDEN_MESSAGES = "HiddenMessages";

    private static OptionList radioOptions1 = new OptionList(
        new String [] {""}, // empty label
        new String [] {"SpecifyVSN.radio1"});

    private static OptionList radioOptions2 = new OptionList(
        new String [] {""}, // empty label
        new String [] {"SpecifyVSN.radio2"});

    /**
     * Constructor
     */
    public SpecifyVSNViewBean() {
        super(PAGE_NAME, DEFAULT_DISPLAY_URL);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        pageTitleModel = createPageTitleModel();
        registerChildren();
        TraceUtil.trace3("Exiting");
    }


    /**
     * Register each child view.
     */
    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        super.registerChildren();
        PageTitleUtil.registerChildren(this, pageTitleModel);
        registerChild(CHILD_RADIOBUTTON1, CCRadioButton.class);
        registerChild(CHILD_RADIOBUTTON2, CCRadioButton.class);
        registerChild(CHILD_START_TEXT, CCLabel.class);
        registerChild(CHILD_START_TEXTFIELD, CCTextField.class);
        registerChild(CHILD_END_TEXT, CCLabel.class);
        registerChild(CHILD_END_TEXTFIELD, CCTextField.class);
        registerChild(CHILD_ONEVSN_TEXT, CCLabel.class);
        registerChild(CHILD_ONEVSN_TEXTFIELD, CCTextField.class);
        registerChild(CHILD_ONEVSN_INSTR_TEXT, CCTextField.class);
        registerChild(CHILD_HIDDEN_MESSAGES, CCHiddenField.class);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     */
    protected View createChild(String name) {
        TraceUtil.trace3(new NonSyncStringBuffer("Entering: name is ").
            append(name).toString());

        View child = null;
        if (super.isChildSupported(name)) {
            child = super.createChild(name);
        } else if (name.equals(CHILD_RADIOBUTTON1)) {
            CCRadioButton myChild = new CCRadioButton(this, name, null);
            myChild.setOptions(radioOptions1);
            child = myChild;
        } else if (name.equals(CHILD_RADIOBUTTON2)) {
            CCRadioButton myChild = new CCRadioButton(this,
            CHILD_RADIOBUTTON1, null);
            myChild.setOptions(radioOptions2);
            child = myChild;
        } else if (name.equals(CHILD_START_TEXTFIELD) ||
            name.equals(CHILD_END_TEXTFIELD) ||
            name.equals(CHILD_ONEVSN_TEXTFIELD) ||
            name.equals(CHILD_ONEVSN_INSTR_TEXT)) {
            child = new CCTextField(this, name, null);
        } else if (name.equals(CHILD_START_TEXT) ||
            name.equals(CHILD_END_TEXT) ||
            name.equals(CHILD_ONEVSN_TEXT)) {
            child = new CCLabel(this, name, null);
        } else if (name.equals(CHILD_HIDDEN_MESSAGES)) {
            child = new CCHiddenField(this, name,
                new NonSyncStringBuffer(
                    SamUtil.getResourceString("SpecifyVSN.errMsg1")).
                    append("###").
                    append(SamUtil.getResourceString("SpecifyVSN.errMsg2")).
                    append("###").
                    append(SamUtil.getResourceString("SpecifyVSN.errMsg3")).
                    append("###").
                    append(SamUtil.getResourceString("SpecifyVSN.errMsg4")).
                    append("###").
                    append(SamUtil.getResourceString("SpecifyVSN.errMsg5")).
                    append("###").
                    append(SamUtil.getResourceString("SpecifyVSN.errMsg6")).
                    append("###").
                    append(SamUtil.getResourceString("SpecifyVSN.errMsg7")).
                    toString());
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            child = PageTitleUtil.createChild(this, pageTitleModel, name);
        } else {
            throw new IllegalArgumentException(new NonSyncStringBuffer(
                "Invalid child name [").append(name).append("]").toString());
        }

        TraceUtil.trace3("Exiting");
        return (View) child;
    }

    private CCPageTitleModel createPageTitleModel() {
        TraceUtil.trace3("Entering");

        if (pageTitleModel == null) {
            pageTitleModel = PageTitleUtil.createModel(
                "/jsp/media/SpecifyVSNPageTitle.xml");
        }
        pageTitleModel.setPageTitleText(SamUtil.getResourceString(
            "SpecifyVSN.pageTitle",
            new String [] {getLibraryName(), getServerName()}));

        TraceUtil.trace3("Exiting");
        return pageTitleModel;
    }

    public void handleSubmitRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");

        String start  = null, end = null;
        String selection = (String) getDisplayFieldValue(CHILD_RADIOBUTTON1);

        // assume tokens size is always 2, verification happens in client side
        if (selection.equals("SpecifyVSN.radio1")) {
            start = ((String) getDisplayFieldValue(
                        CHILD_START_TEXTFIELD)).trim();
            end   = ((String) getDisplayFieldValue(
                        CHILD_END_TEXTFIELD)).trim();
        } else {
            start = ((String) getDisplayFieldValue(
                        CHILD_ONEVSN_TEXTFIELD)).trim();
        }

        try {
            String libName = getLibraryName();
            Library myLibrary = MediaUtil.getLibraryObject(
                getServerName(), libName);

            // if One VSN is chosen, end is null
            SamUtil.doPrint(new NonSyncStringBuffer().append(
                "Importing VSNs: start is ").append(start).append(
                " end is ").append(end).toString());

            LogUtil.info(this.getClass(), "handleSubmitRequest",
                new NonSyncStringBuffer().append("Start importing in ").
                append(libName).toString());

            myLibrary.importVSNInNWALib(start, end);

            LogUtil.info(this.getClass(), "handleSubmitRequest",
                new NonSyncStringBuffer().append("Done importing ").append(
                libName).toString());

            SamUtil.setInfoAlert(
                getParentViewBean(),
                CommonSecondaryViewBeanBase.ALERT,
                "success.summary",
                SamUtil.getResourceString(
                    "LibrarySummary.action.import", libName),
                getServerName());

            setSubmitSuccessful(true);

        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "handleSubmitHrefRequest",
                "Failed to import VSN",
                getServerName());
            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonSecondaryViewBeanBase.ALERT,
                "LibrarySummary.error.import",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                getServerName());
        }

        getParentViewBean().forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    private String getLibraryName() {
        // first check the page session
        String libraryName = (String) getPageSessionAttribute(
            Constants.PageSessionAttributes.LIBRARY_NAME);

        // second check the request
        if (libraryName == null) {
            libraryName = RequestManager.getRequest().getParameter(
                Constants.PageSessionAttributes.LIBRARY_NAME);

            if (libraryName != null) {
                setPageSessionAttribute(
                    Constants.PageSessionAttributes.LIBRARY_NAME,
                    libraryName);
            }

            if (libraryName == null) {
                throw new IllegalArgumentException("Library Name not supplied");
            }
        }

        return libraryName;
    }
}
