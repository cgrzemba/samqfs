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

// ident	$Id: NewEditVSNPoolViewBean.java,v 1.25 2008/03/17 14:40:44 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSWarnings;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemArchiveManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.VSNPool;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSUtil;
import com.sun.netstorage.samqfs.web.model.media.BaseDevice;
import com.sun.netstorage.samqfs.web.util.CommonSecondaryViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.html.CCTextField;
import com.sun.web.ui.view.pagetitle.CCPageTitle;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import javax.servlet.ServletException;

/**
 *  This class is the view bean for the New/Edit VSN Pool page
 */
public class NewEditVSNPoolViewBean extends CommonSecondaryViewBeanBase {
    // The "logical" name for this page.
    public static final String PAGE_NAME = "NewEditVSNPool";
    private static final String DEFAULT_DISPLAY_URL =
                                "/jsp/archive/NewEditVSNPool.jsp";

    // children
    public static final String REQUIRED_LABEL = "requiredLabel";
    public static final String NAME = "name";
    public static final String NAME_LABEL = "nameLabel";
    public static final String MEDIA_TYPE = "mediaType";
    public static final String MEDIA_TYPE_LABEL = "mediaTypeLabel";
    public static final String SPECIFY_VSN_LABEL = "specifyVSNLabel";
    public static final String VSN_START_END_RADIO = "vsnStartEndRadio";
    public static final String VSN_RANGE_RADIO = "vsnRangeRadio";
    public static final String START_LABEL = "startLabel";
    public static final String START = "start";
    public static final String END_LABEL = "endLabel";
    public static final String END = "end";
    public static final String VSN_RANGE_LABEL = "vsnRangeLabel";
    public static final String VSN_RANGE = "vsnRange";
    public static final String VSN_RANGE_HELP = "vsnRangeHelp";

    public static final String OPERATION = "OPERATION";
    public static final String NEW_POOL = "NEW";
    public static final String EDIT_POOL = "EDIT";

    // Page Title Attributes and Components.
    private CCPageTitleModel ptModel = null;

    // determines if this is initial load of the page
    private boolean isFirstLoad = false;

    /**
     * Constructor
     */
    public NewEditVSNPoolViewBean() {
        super(PAGE_NAME, DEFAULT_DISPLAY_URL);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        ptModel = createPageTitleModel();
        registerChildren();
        TraceUtil.trace3("Exiting");
    }


    /**
     * Register each child view.
     */
    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        super.registerChildren();
        ptModel.registerChildren(this);

        registerChild(REQUIRED_LABEL, CCLabel.class);
        registerChild(NAME, CCTextField.class);
        registerChild(NAME_LABEL, CCLabel.class);
        registerChild(MEDIA_TYPE, CCDropDownMenu.class);
        registerChild(MEDIA_TYPE_LABEL, CCLabel.class);
        registerChild(SPECIFY_VSN_LABEL, CCLabel.class);
        registerChild(VSN_START_END_RADIO, CCRadioButton.class);
        registerChild(VSN_RANGE_RADIO, CCRadioButton.class);
        registerChild(START_LABEL, CCLabel.class);
        registerChild(START, CCTextField.class);
        registerChild(END_LABEL, CCLabel.class);
        registerChild(END, CCTextField.class);
        registerChild(VSN_RANGE_LABEL, CCLabel.class);
        registerChild(VSN_RANGE, CCTextField.class);
        registerChild(VSN_RANGE_HELP, CCStaticTextField.class);

        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     */
    protected View createChild(String name) {
        if (name.indexOf("Label") != -1) {
            return new CCLabel(this, name, null);
        } else if (name.equals(NAME) ||
                   name.equals(START) ||
                   name.equals(END) ||
                   name.equals(VSN_RANGE)) {
            return new CCTextField(this, name, null);
        } else if (name.equals(MEDIA_TYPE)) {
            return new CCDropDownMenu(this, name, null);
        } else if (name.equals(VSN_START_END_RADIO)) {
            CCRadioButton radio =
                new CCRadioButton(this, VSN_START_END_RADIO, "vsnrange");
            radio.setOptions(new OptionList(new String [] {""},
                                            new String [] {"startend"}));
            return radio;
        } else if (name.equals(VSN_RANGE_RADIO)) {
            CCRadioButton radio =
                new CCRadioButton(this, VSN_START_END_RADIO, "vsnrange");
            radio.setOptions(new OptionList(new String [] {""},
                                            new String [] {"vsnrange"}));
            return radio;
        } else if (name.equals(VSN_RANGE_HELP)) {
            return new CCStaticTextField(this, name, null);
        } else if (super.isChildSupported(name)) {
            return super.createChild(name);
        } else if (name.equals("PageTitle")) {
            return new CCPageTitle(this, ptModel, name);
        } else if (ptModel.isChildSupported(name)) {
            return ptModel.createChild(this, name);
        } else  {
            throw new IllegalArgumentException("Invalid child [" + name + "]");
        }
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");
        super.beginDisplay(event);

        CCDropDownMenu mediaType = (CCDropDownMenu)getChild(MEDIA_TYPE);

        String serverName = getServerName();
        String poolName = "";

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);

            int [] mediaTypes = sysModel.getSamQFSSystemMediaManager().
                getAvailableArchiveMediaTypes();

            String [] labels = new String[mediaTypes.length];
            String [] values = new String[mediaTypes.length];

            for (int i = 0; i < mediaTypes.length; i++) {
                labels[i] = SamUtil.getMediaTypeString(mediaTypes[i]);
                values[i] = Integer.toString(mediaTypes[i]);
            }

            mediaType.setOptions(new OptionList(labels, values));

            if (!isNewVSNPool()) {
                ptModel.setPageTitleText("NewEditVSNPool.edit.pageTitle");

                poolName = getVSNPoolName();

                CCTextField name = (CCTextField)getChild(NAME);
                if (isFirstLoad) {
                    loadPoolDetails(serverName, poolName);
                }
                // always disable media type and name if editing a pool
                name.setDisabled(true);
                mediaType.setDisabled(true);
                name.setValue(poolName);
            } else {
                ptModel.setPageTitleText("NewEditVSNPool.new.pageTitle");
            }

            // enable list field if range set.
            // especially important for the initial page load of creating new
            // vsn pool.
            String rs = getDisplayFieldStringValue(VSN_START_END_RADIO);
            if ("vsnrange".equals(rs))
                ((CCTextField)getChild(VSN_RANGE)).setDisabled(false);
        } catch (SamFSException smfex) {
            SamUtil.processException(smfex,
                                     this.getClass(),
                                     "beginDisplay",
                                     "Exception occurred within framework",
                                     serverName);

            ((CCButton)getChild("Submit")).setDisabled(true);

            throw new ModelControlException(smfex.getMessage() != null ?
               smfex.getMessage() : "Exception occurred within framework");
        }

        TraceUtil.trace3("Exiting");
    }

    private CCPageTitleModel createPageTitleModel() {
        return new CCPageTitleModel(
           RequestManager.getRequestContext().getServletContext(),
           "/jsp/archive/NewEditVSNPoolPageTitle.xml");
    }

    /**
     *  determine if we are creating a new vsn pool or editing an existing one
     *
     * @return boolean - true if creating a new pool else false
     *
     */
    protected boolean isNewVSNPool() {
        String operation = (String)getPageSessionAttribute(OPERATION);

        if (operation == null) {
            operation = RequestManager.getRequest().getParameter(OPERATION);
            setPageSessionAttribute(OPERATION, operation);

            // if operation is in the request, then this is the initial page
            // load.
            isFirstLoad = true;
        }
        return NEW_POOL.equals(operation);
    }

    /**
     * return the name of the pool being edited. This method is and should
     * only be called when editing an existing pool.
     *
     * @return poolName - name of the pool
     *
     */
    protected String getVSNPoolName() {
        String poolName =
            (String)getPageSessionAttribute(Constants.Archive.VSN_POOL_NAME);
        if (poolName == null) {
            poolName = RequestManager.
                getRequest().getParameter(Constants.Archive.VSN_POOL_NAME);
            setPageSessionAttribute(Constants.Archive.VSN_POOL_NAME, poolName);
        }

        return poolName;
    }

    /**
     * validate user input
     *
     * @return errors - a list of error messages
     *
     */
    protected List validate() throws SamFSException {
        List errors = new ArrayList();

        SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());
        SamQFSSystemArchiveManager archiveManager =
            sysModel.getSamQFSSystemArchiveManager();

        String poolName = null;
        int mediaType = BaseDevice.MTYPE_DISK;

        // prevent successive calls to isNewVSNPool
        boolean newPool = isNewVSNPool();

        if (newPool) {
            poolName = getDisplayFieldStringValue(NAME);
            poolName = poolName == null? "" : poolName.trim();

            String s = getDisplayFieldStringValue(MEDIA_TYPE);
            // shouldn't be null, but default to disk anyway
            mediaType =
                s == null ? BaseDevice.MTYPE_DISK : Integer.parseInt(s);
        } else {
            poolName = getVSNPoolName();
        }

        // check for null
        if (poolName.equals("")) {
            errors.add(SamUtil.getResourceString("NewEditVSN.namenull"));
            ((CCLabel)getChild(NAME_LABEL)).setShowError(true);
        } else {
            // check for well-formedness
            if (!SamUtil.isValidFSNameString(poolName)) {
                errors.add(SamUtil.getResourceString("NewEditVSN.invalidname"));
                ((CCLabel)getChild(NAME_LABEL)).setShowError(true);
            }

            // check for uniqueness
            String [] allPoolNames = archiveManager.getAllPoolNames();

            // prevent looping through pool names  while editing an existing
            // pool
            boolean found = !newPool;
            for (int i = 0; i < allPoolNames.length && !found; i++) {
                if (poolName.equals(allPoolNames[i])) {
                    found = true;
                }
            }
            if (found && newPool) {
                errors.add(SamUtil.getResourceString("NewEditVSN.nameinuse",
                                                     poolName));
                ((CCLabel)getChild(NAME_LABEL)).setShowError(true);
            }
        }

        // determine if to use range or start & end
        String radio = getDisplayFieldStringValue(VSN_START_END_RADIO);
        boolean isRange = "vsnrange".equals(radio) ? true : false;

        String expression = "";
        if (isRange) {
            // validate range
            String range = getDisplayFieldStringValue(VSN_RANGE);
            range = range == null ? "" : range.trim();
            if (range.equals("")) {
                errors.add(
                     SamUtil.getResourceString("NewEditVSN.rangenull"));
                ((CCLabel)getChild(VSN_RANGE_LABEL)).setShowError(true);
            } else {
                expression = range;
            }
        } else {
            // validate start - end combination
            boolean valid = true;

            String start = getDisplayFieldStringValue(START);
            String end = getDisplayFieldStringValue(END);
            start = start == null ? "" : start.trim();
            end = end == null ? "" : end.trim();

            if (start.equals("")) {
                errors.add(
                SamUtil.getResourceString("NewEditVSN.invalidstart"));
                ((CCLabel)getChild(START_LABEL)).setShowError(true);

                valid = false;
            }

            if (end.equals("")) {
                errors.add(
                     SamUtil.getResourceString("NewEditVSN.invalidend"));
                ((CCLabel)getChild(END_LABEL)).setShowError(true);

                valid = false;
            }

            if (valid) {
                expression = SamQFSUtil.createExpression(start, end);
                if (expression == null) {
                    errors.add(
                    SamUtil.getResourceString("NewEditVSN.invalidstartend"));
                    ((CCLabel)getChild(START_LABEL)).setShowError(true);
                    ((CCLabel)getChild(END_LABEL)).setShowError(true);
                }
            }
        }

        // if valid, save the pool and return else return without saving
        if (errors.size() == 0) {
            if (isNewVSNPool()) {
                archiveManager.createVSNPool(poolName, mediaType, expression);
            } else {
                VSNPool thePool = archiveManager.getVSNPool(poolName);
                if (thePool == null)
                    throw new SamFSException(null, -2010);

                thePool.setMemberVSNs(thePool.getMediaType(), expression);
            }

            // set confirmation alert message
        }
        return errors;
    }

    /**
     * initial populating of the contianer view
     */
    protected void loadPoolDetails(String serverName, String poolName)
        throws SamFSException {
        SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());

        VSNPool thePool  =
            sysModel.getSamQFSSystemArchiveManager().getVSNPool(poolName);

        if (thePool == null)
            throw new SamFSException(null, -2010);

        String vsnExpression = thePool.getVSNExpression();

        ((CCTextField)getChild(NAME)).setValue(poolName);
        ((CCDropDownMenu)getChild(MEDIA_TYPE)).
            setValue(Integer.toString(thePool.getMediaType()));

        CCTextField range = (CCTextField)getChild(VSN_RANGE);
        CCTextField start = (CCTextField)getChild(START);
        CCTextField end = (CCTextField)getChild(END);
        CCRadioButton radio = (CCRadioButton)getChild(VSN_START_END_RADIO);

        if (vsnExpression != null) {
            range.setValue(vsnExpression);
            radio.setValue("vsnrange");
            start.setDisabled(true);
            end.setDisabled(true);
            range.setDisabled(false);
        } else {
            range.setDisabled(true);
            radio.setValue("startend");
            start.setDisabled(false);
            end.setDisabled(false);
        }
    }

    /**
     * handler for the submit button. Creates the new pool or saves the
     * changes to an existing pool.
     *
     * @param evt
     * @throws ServletException
     * @throws IOException
     */
    public void handleSubmitRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {

        String serverName = getServerName();

        try {
            List errors = validate();

            if (errors.size() > 0) {
                NonSyncStringBuffer buffer = new NonSyncStringBuffer();
                Iterator it = errors.iterator();

                while (it.hasNext()) {
                    buffer.append((String)it.next()).append("<br>");
                }

                SamUtil.setErrorAlert(this,
                                      ALERT,
                                      "-2010",
                                      -2010,
                                      buffer.toString(),
                                      serverName);
            } else {
                String poolName = isNewVSNPool() ?
                    getDisplayFieldStringValue(NAME) : getVSNPoolName();
                SamUtil.setInfoAlert(
                    this,
                    ALERT,
                    "success.summary",
                    isNewVSNPool() ?
                        SamUtil.getResourceString(
                            "archiving.vsnpool.new.success", poolName) :
                        SamUtil.getResourceString(
                            "archiving.vsnpool.edit.success", poolName),
                    serverName);
                setSubmitSuccessful(true);
            }
        } catch (SamFSWarnings sfw) {
            SamUtil.setWarningAlert(this,
                                     ALERT,
                                     "ArchiveConfig.warning.summary",
                                     "ArchiveConfig.warning.detail");

            setSubmitSuccessful(true);
        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                            getClass(),
                            "handleSubmitRequest",
                            "Error validating user input",
                            serverName);

            SamUtil.setErrorAlert(this,
                            ALERT,
                            "-2010",
                            sfe.getSAMerrno(),
                            sfe.getMessage(),
                            serverName);

        }

        forwardTo(getRequestContext());
    }

    /**
     * safety method to prevent users from seeing a stack trace should the
     * javascript malfunction.
     *
     * @param evt
     * @throws ServletException
     * @throws IOException
     */
    public void handleCancelRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        forwardTo(getRequestContext());
    }
}
