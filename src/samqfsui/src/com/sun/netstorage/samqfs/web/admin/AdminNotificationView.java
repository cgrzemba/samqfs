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

// ident	$Id: AdminNotificationView.java,v 1.30 2008/03/17 14:40:41 am143972 Exp $

package com.sun.netstorage.samqfs.web.admin;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.admin.Notification;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.LogUtil;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.model.CCPropertySheetModel;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCTextField;
import com.sun.web.ui.view.propertysheet.CCPropertySheet;

/**
 *  This class is the view bean for the Admin Notification Summary page
 */
public class AdminNotificationView extends RequestHandlingViewBase {

    private static final String ADD_NEW_SUBSCRIBER  = "AddNewSubscriptionEmail";
    private static final String CHILD_MAIL_LABEL    = "SubscriberLabel";
    private static final String CHILD_DETAIL_LABEL  = "SubscriptionDetail";
    // Dropdown of subscribed mail addresses
    private static final String CHILD_MAIL_LIST  = "SubscriberList";
    private static final String CHILD_MAIL_FIELD = "NewSubscriberField";
    private static final String CHILD_SUBSCRIBER_MENU_HREF  = "SubscriberHref";
    private static final String CHILD_PROPERTY_SHEET = "PropertySheet";
    private static final String CHILD_SERVER_CONFIG = "ServerConfig";

    private CCPropertySheetModel pSheetModel = null;
    private CCPageTitleModel pageTitleModel  = null;

    private boolean isQFSStandAlone = false;

    /**
     * Constructor
     * The view bean calls the RequestDispatcher's forward method passing in
     * the view beans's DISPLAY_URL. This passes control over to the
     * jsp to render its contents. the page title model is created and the
     * children are registered
     */
    public AdminNotificationView(View parent, String name) {
        super(parent, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        pageTitleModel = createPageTitleModel();

        String serverName =
            ((CommonViewBeanBase)getParentViewBean()).getServerName();
        isQFSStandAlone =
            SamUtil.getSystemType(serverName) == SamQFSSystemModel.QFS;
        pSheetModel = createPropertySheetModel();

        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child with the parent view. This won't create the
     * child views, only metadata, containerView will create the child
     * views 'just in time' via call to createChild()
     */
    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        registerChild(CHILD_MAIL_LABEL, CCLabel.class);
        registerChild(CHILD_DETAIL_LABEL, CCLabel.class);
        registerChild(CHILD_MAIL_LIST, CCDropDownMenu.class);
        registerChild(CHILD_MAIL_FIELD, CCTextField.class);
        registerChild(CHILD_SUBSCRIBER_MENU_HREF, CCHref.class);
        registerChild(CHILD_SERVER_CONFIG, CCHiddenField.class);
        registerChild(CHILD_PROPERTY_SHEET, CCPropertySheet.class);
        pSheetModel.registerChildren(this);
        PageTitleUtil.registerChildren(this, pageTitleModel);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Simplest implementation of createChild
     * Call constructor for each child view in multi-conditional statement block
     *
     * @param name The name of the child view
     * @return View The instantiated child view
     */
    protected View createChild(String name) {
        TraceUtil.trace3(new NonSyncStringBuffer(
            "Entering: child view name is ").append(name).toString());
        View child = null;

        if (name.equals(CHILD_PROPERTY_SHEET)) {
            CCPropertySheet psChild =
                            new CCPropertySheet(this, pSheetModel, name);
            child = psChild;

        } else if (pSheetModel != null && pSheetModel.isChildSupported(name)) {
            // Create child from property sheet model.
            child = pSheetModel.createChild(this, name);

        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {

            child = PageTitleUtil.createChild(this, pageTitleModel, name);

        } else if (name.equals(CHILD_MAIL_LABEL) ||
            name.equals(CHILD_DETAIL_LABEL)) {
            child = new CCLabel(this, name, null);

        } else if (name.equals(CHILD_MAIL_LIST)) {
            child = new CCDropDownMenu(this, name, null);
            // options in the drop down list are filled later

        } else if (name.equals(CHILD_MAIL_FIELD)) {
            child = new CCTextField(this, name, null);

        // inline alert
        } else if (name.equals(CHILD_SUBSCRIBER_MENU_HREF)) {
            child = new CCHref(this, name, null);

        } else if (name.equals(CHILD_SERVER_CONFIG)) {
            child = new CCHiddenField(this, name, null);
        } else {
            throw new IllegalArgumentException(
                "Invalid child name [" + name + "]");
        }

        TraceUtil.trace3("Exiting");
        return (View) child;
    }

    /**
     * Called as notification that the JSP has begun its display processing
     * @param event The DisplayEvent
     */
    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");
        // CommonPageTitle has both Submit and Cancel, hide Cancel button
        ((CCButton) getChild("Cancel")).setVisible(false);
        // set onclick action for Sumbit
        NonSyncStringBuffer extraHtml = new NonSyncStringBuffer();
        extraHtml.append("onclick=\"return validateCheckBoxes(); \"");
        ((CCButton) getChild("Submit")).setExtraHtml(extraHtml.toString());

        TraceUtil.trace3("Exiting");
    }

    /**
     * On Submit, check if the mailaddressValue is enabled and populated
     * with an email address. If the mailaddressValue is disabled, then
     * a radio item must be selected, Get the selectedOptions and save
     *
     * alerts are displayed for the following:-
     *  mailaddressValue is enabled but empty or invalid
     *  mailaddressValue is disabled and no radio is selected (cannot happen)
     *
     */
    public void handleSubmitRequest(RequestInvocationEvent event) {
        TraceUtil.trace3("Entering");

        boolean isOverflow = false;
        boolean isDevice = false; // in SAM install only
        boolean isArchive = false; // in SAM install only
        boolean isMedia = false; // in SAM install only
        boolean isRecycler = false; // in SAM install only
        boolean isSnapshot = false; // in SAM install only
        boolean isHwmExceed = false; // in SAM install 4.6 and above
        boolean isAcslsErr = false; // in SAM install 4.6 and above
        boolean isAcslsWarn = false; // in SAM install 4.6 and above
        boolean isDumpWarn = false; // in SAM install 4.6 and above
        boolean isLongWaitTape = false; // not used (intellistor setup)
        boolean isFsInconsistent = false; // not used (intellistor setup)
        boolean isSystemHealth = false; // not used (intellistor setup)

        AdminNotificationViewBean parent =
            (AdminNotificationViewBean)getParentViewBean();
        String serverName = parent.getServerName();

        ((CCDropDownMenu) getChild(CHILD_MAIL_LIST)).restoreStateData();
        ((CCTextField) getChild(CHILD_MAIL_FIELD)).restoreStateData();

        String selectedSubscriber = (String)
            ((CCDropDownMenu) getChild(CHILD_MAIL_LIST)).getValue();
        String newAddress = (String)
            ((CCTextField) getChild(CHILD_MAIL_FIELD)).getValue();
        String address = (String)
            (ADD_NEW_SUBSCRIBER.equals(selectedSubscriber) ?
                newAddress : selectedSubscriber);
        boolean isNew = ADD_NEW_SUBSCRIBER.equals(selectedSubscriber) ?
            true: false;

        try {
            String operation = "";

            if (address == null || address.trim().length() == 0) {
                ((CCLabel) getChild(CHILD_MAIL_LABEL)).setShowError(true);
                throw new SamFSException("AdminNotification.errMsg1");
            }

            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            Notification existNotification =
                sysModel.getSamQFSSystemAdminManager().getNotification(address);

            // get the selected notification subscription for this address
            if (pSheetModel != null) {

                isOverflow  = "true".equals((String)pSheetModel.getValue(
                    "check6")) ? true : false;

                if (!isQFSStandAlone) {
                    isArchive   =
                        "true".equals((String)pSheetModel.getValue("check1"));
                    isMedia =
                        "true".equals((String)pSheetModel.getValue("check2"));
                    isDevice    =
                        "true".equals((String)pSheetModel.getValue("check3"));
                    isRecycler  =
                        "true".equals((String)pSheetModel.getValue("check4"));
                    isSnapshot  =
                        "true".equals((String)pSheetModel.getValue("check5"));
                    isHwmExceed =
                        "true".equals((String)pSheetModel.getValue("check7"));
                    isAcslsErr =
                        "true".equals((String)pSheetModel.getValue("check8"));
                    isAcslsWarn =
                        "true".equals((String)pSheetModel.getValue("check9"));
                    isDumpWarn =
                        "true".equals((String)pSheetModel.getValue("check10"));
                }
            }

            // Check if notification already exists for this mailAddress
            TraceUtil.trace3("Create/Modify email: " + address);

            if (existNotification == null) {
                // atleast one must be selected for new email subscription
                if (!isArchive && !isMedia && !isDevice &&
                    !isRecycler && !isSnapshot && !isOverflow &&
                    !isHwmExceed && !isDumpWarn && !isAcslsErr &&
                    !isAcslsWarn && !isLongWaitTape && !isFsInconsistent &&
                    !isSystemHealth) {

                    throw new SamFSException("AdminNotification.errMsg3");
                }
                // create new notification
                LogUtil.info(this.getClass(), "handleSubmitRequest",
                    new NonSyncStringBuffer().
                        append("create notification start").
                        append(address).toString());

                sysModel.getSamQFSSystemAdminManager().createNotification(
                    "",
                    address,
                    isDevice,
                    isArchive,
                    isMedia,
                    isRecycler,
                    isSnapshot,
                    isOverflow,
                    isHwmExceed,
                    isAcslsErr,
                    isAcslsWarn,
                    isDumpWarn,
                    isLongWaitTape,
                    isFsInconsistent,
                    isSystemHealth);

                LogUtil.info(this.getClass(), "handleSubmitRequest",
                    new NonSyncStringBuffer().
                        append("creating notification complete").
                        append(address).toString());
                // select the newly added email in the subscriber list
                ((CCDropDownMenu) getChild(CHILD_MAIL_LIST)).setValue(address);
                operation = "AdminNotification.msg.new";

            } else {
                // Check if user is choosing to add
                if (isNew == true) {
                    // user cannot add an existing email address
                    throw new SamFSException(
                        SamUtil.getResourceString("AdminNotification.errMsg4",
                            address));
                }
                // modify existing notification
                LogUtil.info(this.getClass(), "handleSubmitRequest",
                    new NonSyncStringBuffer("Edit notification start").
                        append(address).toString());

                existNotification.setDeviceDownNotify(isDevice);
                existNotification.setArchiverInterruptNotify(isArchive);
                existNotification.setReqMediaNotify(isMedia);
                existNotification.setRecycleNotify(isRecycler);
                existNotification.setDumpInterruptNotify(isSnapshot);
                existNotification.setFsNospaceNotify(isOverflow);
                existNotification.setHwmExceedNotify(isHwmExceed);
                existNotification.setAcslsErrNotify(isAcslsErr);
                existNotification.setAcslsWarnNotify(isAcslsWarn);
                existNotification.setDumpWarnNotify(isDumpWarn);
                sysModel.getSamQFSSystemAdminManager().
                    editNotification(existNotification);

                LogUtil.info(this.getClass(), "handleSubmitRequest",
                    new NonSyncStringBuffer("edit notification complete").
                    append(address).toString());

                if (!isArchive && !isMedia && !isDevice &&
                    !isRecycler && !isSnapshot && !isOverflow &&
                    !isHwmExceed && !isDumpWarn && !isAcslsErr &&
                    !isAcslsWarn && !isLongWaitTape &&
                    !isFsInconsistent && !isSystemHealth) {
                    // delete operation
                    ((CCDropDownMenu) getChild(CHILD_MAIL_LIST)).setValue(
                        ADD_NEW_SUBSCRIBER);
                    operation = "AdminNotification.msg.delete";
                } else {
                    ((CCDropDownMenu)
                            getChild(CHILD_MAIL_LIST)).setValue(address);
                    operation = "AdminNotification.msg.edit";
                }
            }
            // clear mail field
            ((CCTextField) getChild(CHILD_MAIL_FIELD)).setValue("");
            SamUtil.setInfoAlert(
                parent,
                parent.CHILD_COMMON_ALERT,
                "success.summary",
                SamUtil.getResourceString(operation, address),
                serverName);
        } catch (SamFSException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "AdminNotificationViewBean()",
                "Failed to save notifications",
                serverName);
            SamUtil.setErrorAlert(
                parent,
                parent.CHILD_COMMON_ALERT,
                "AdminNotification.error.save",
                ex.getSAMerrno(),
                ex.getMessage(),
                serverName);
        }

        parent.forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }


    public void handleSubscriberHrefRequest(RequestInvocationEvent event)
    {
        TraceUtil.trace3("Entering");
        AdminNotificationViewBean parent =
            (AdminNotificationViewBean)getParentViewBean();
        parent.forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }


    public void populateDisplay() throws SamFSException {

        AdminNotificationViewBean parent =
            (AdminNotificationViewBean)getParentViewBean();
        String serverName = parent.getServerName();

        SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
        Notification[] notifications =
            sysModel.getSamQFSSystemAdminManager().getAllNotifications();

        String address = (String) getDisplayFieldValue(CHILD_MAIL_LIST);
        // first time display, set display field value to ADD_NEW_SUBSCRIBER
        address = (address == null) ? ADD_NEW_SUBSCRIBER : address;
        // If address is a variable e.g. $CONTACT, then read only
        if (address.startsWith("$")) {
            ((CCButton) getChild("Submit")).setDisabled(true);
        }

        // populate the list of subscribers in dropdown
        OptionList subscribers = new OptionList();
        // first entry is always "Add New Subscriber
        subscribers.add(
            SamUtil.getResourceString("emailAlert.subscriberList.addNew"),
            ADD_NEW_SUBSCRIBER);

        if (notifications != null && notifications.length > 0) {
            for (int i = 0; i < notifications.length; i++) {
                String email = notifications[i].getEmailAddress();
                subscribers.add(email, email);
            }
        }
        ((CCDropDownMenu) getChild(CHILD_MAIL_LIST)).setOptions(subscribers);
        ((CCDropDownMenu) getChild(CHILD_MAIL_LIST)).setValue(address);

        pSheetModel.clear();
        // hide textbox if not new subscription
        if (!ADD_NEW_SUBSCRIBER.equals(address)) {
            ((CCTextField) getChild(CHILD_MAIL_FIELD)).setVisible(false);
            ((CCLabel)getChild(CHILD_DETAIL_LABEL)).setValue(
                SamUtil.getResourceString("emailAlert.msg.editEmail"));
            Notification selected =
                sysModel.getSamQFSSystemAdminManager().getNotification(address);
            populateSubscription(selected);
        } else {
            ((CCTextField) getChild(CHILD_MAIL_FIELD)).setVisible(true);
        }
        TraceUtil.trace3("Exiting");
    }

    private void populateSubscription(Notification selected)
        throws SamFSException {

        if (selected == null) { // new address
            TraceUtil.trace3("selected = null, is it a new option?");
            throw new SamFSException(null, -1002);
        }
        pSheetModel.clear();

        pSheetModel.setValue("check6",
            String.valueOf(selected.isFsNospaceNotify()));
        if (!isQFSStandAlone) {
            pSheetModel.setValue("check1",
                String.valueOf(selected.isArchiverInterruptNotify()));
            pSheetModel.setValue("check2",
                String.valueOf(selected.isReqMediaNotify()));
            pSheetModel.setValue("check3",
                String.valueOf(selected.isDeviceDownNotify()));
            pSheetModel.setValue("check4",
                String.valueOf(selected.isRecycleNotify()));
            pSheetModel.setValue("check5",
                String.valueOf(selected.isDumpInterruptNotify()));
            pSheetModel.setValue("check7",
                String.valueOf(selected.isHwmExceedNotify()));
            pSheetModel.setValue("check8",
                String.valueOf(selected.isAcslsErrNotify()));
            pSheetModel.setValue("check9",
                String.valueOf(selected.isAcslsWarnNotify()));
            pSheetModel.setValue("check10",
                String.valueOf(selected.isDumpWarnNotify()));
        }
    }

    private CCPropertySheetModel createPropertySheetModel() {

        if (isQFSStandAlone) {
            // for qfsStandAlone, only File System Overflow notification
            ((CCHiddenField)getChild(CHILD_SERVER_CONFIG)).
                setValue(String.valueOf(0));
            return new CCPropertySheetModel(
                RequestManager.getRequestContext().getServletContext(),
                "/jsp/admin/AdminNotificationPropertySheetQFS.xml");
        } else {
            ((CCHiddenField)getChild(CHILD_SERVER_CONFIG)).
                setValue(String.valueOf(3));
            return new CCPropertySheetModel(
                RequestManager.getRequestContext().getServletContext(),
                    "/jsp/admin/AdminNotificationPropertySheet.xml");
        }
    }

    /**
     * Create the pagetitle model
     */
    private CCPageTitleModel createPageTitleModel() {
        TraceUtil.trace3("Entering");
        if (pageTitleModel == null) {
            pageTitleModel = PageTitleUtil.createModel(
                "/jsp/util/CommonPageTitle.xml");
        }
        TraceUtil.trace3("Exiting");
        return pageTitleModel;
    }

}
