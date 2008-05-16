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

// ident	$Id: ISPolicyDetailsTiledView.java,v 1.13 2008/05/16 18:38:51 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.DefaultModel;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.RequestHandlingTiledViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.ChildDisplayEvent;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.iplanet.jato.view.event.TiledViewRequestInvocationEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteria;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteriaCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveVSNMap;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSUtil;
import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PageInfo;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCCheckBox;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.html.CCTextField;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import javax.servlet.ServletException;

public class ISPolicyDetailsTiledView extends RequestHandlingTiledViewBase
                                      implements CopyFields {
    public ISPolicyDetailsTiledView(View parent, String name) {
        super(parent, name);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        setPrimaryModel((DefaultModel)getDefaultModel());
        setMaxDisplayTiles(4);
        registerChildren();

        TraceUtil.trace3("Exiting");
    }

    public void registerChildren() {
        TraceUtil.trace3("Entering");
        registerChild(COPY_NUMBER, CCStaticTextField.class);
        registerChild(COPY_TIME, CCTextField.class);
        registerChild(COPY_TIME_UNIT, CCDropDownMenu.class);
        registerChild(EXPIRATION_TIME, CCTextField.class);
        registerChild(EXPIRATION_TIME_UNIT, CCDropDownMenu.class);
        registerChild(NEVER_EXPIRE, CCCheckBox.class);
        registerChild(RELEASER_BEHAVIOR, CCDropDownMenu.class);
        registerChild(MEDIA_POOL, CCDropDownMenu.class);
        registerChild(SCRATCH_POOL, CCDropDownMenu.class);
        registerChild(AVAILABLE_MEDIA, CCStaticTextField.class);
        registerChild(AVAILABLE_MEDIA_HREF, CCHref.class);
        registerChild(AVAILABLE_MEDIA_STRING, CCHiddenField.class);
        registerChild(ENABLE_RECYCLING, CCCheckBox.class);
        registerChild(ADD_COPY, CCButton.class);
        registerChild(REMOVE_COPY, CCButton.class);
        registerChild(COPY_OPTIONS, CCButton.class);
        registerChild(MEDIA_TYPE, CCDropDownMenu.class);
        registerChild(ASSIGNED_VSNS, CCTextField.class);

        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        if (name.equals(COPY_TIME) ||
            name.equals(EXPIRATION_TIME) ||
            name.equals(ASSIGNED_VSNS)) {
            return new CCTextField(this, name, null);
        } else if (name.equals(COPY_TIME_UNIT) ||
                   name.equals(EXPIRATION_TIME_UNIT) ||
                   name.equals(RELEASER_BEHAVIOR) ||
                   name.equals(MEDIA_POOL) ||
                   name.equals(SCRATCH_POOL) ||
                   name.equals(MEDIA_TYPE)) {
            return new CCDropDownMenu(this, name, null);
        } else if (name.equals(NEVER_EXPIRE) ||
                   name.equals(ENABLE_RECYCLING)) {
            return new CCCheckBox(this, name, "true", "false", false);
        } else if (name.equals(ADD_COPY) ||
                   name.equals(REMOVE_COPY) ||
                   name.equals(COPY_OPTIONS)) {
            return new CCButton(this, name, null);
        } else if (name.equals(COPY_NUMBER) ||
                   name.equals(COPY_DIV) ||
                   name.equals(NO_COPY_DIV) ||
                   name.equals(AVAILABLE_MEDIA)) {
            return new CCStaticTextField(this, name, name);
        } else if (name.equals(AVAILABLE_MEDIA_HREF)) {
            return new CCHref(this, name, null);
        } else if (name.equals(AVAILABLE_MEDIA_STRING)) {
            return new CCHiddenField(this, name, null);
        } else {
            throw new IllegalArgumentException("Invalid child '" + name + "'");
        }
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        TraceUtil.trace3("Entering");
        super.beginDisplay(evt);
        DefaultModel model = (DefaultModel)getPrimaryModel();
        model.setSize(4);

        try {
            populateDropDownMenus();
        } catch (SamFSException sfe) {
            // TODO
        }

        TraceUtil.trace3("Exiting");
    }

    /**
     * locate the ArchivePolCriteriaCopy object that corresponds to the given
     * copyNumber
     */
    private ArchivePolCriteriaCopy getArchivePolCriteriaCopy(int copyNumber) {
        TraceUtil.trace3("Entering");

        ArchivePolCriteriaCopy result = null;

        // does this copy exist?
        List copyList = getAvailableCopyList();
        if (!copyList.contains(new Integer(copyNumber))) {
            return null;
        }

        // get the copy object
        ArchiveCopy theCopy = getArchiveCopy(copyNumber);
        if (theCopy != null) { // copy exists
        ArchivePolicy thePolicy = theCopy.getArchivePolicy();
        if (thePolicy != null) { // policy exists
        ArchivePolCriteria [] criteria = thePolicy.getArchivePolCriteria();
        if (criteria != null && criteria.length > 0) { // criteria list
        // NOTE: we use the very first criteria here
        ArchivePolCriteria theCriteria = criteria[0];
        if (theCriteria != null) { // found the first criteria
        ArchivePolCriteriaCopy [] criteriaCopy =
            theCriteria.getArchivePolCriteriaCopies();
        if (criteriaCopy != null) { // found the list of criteria copies
            // now loop through the criteria copy list and find the copy with
            // the matching copy number
            boolean found = false;
            for (int i = 0; !found && i < criteriaCopy.length; i++) {
                if (copyNumber ==
                    criteriaCopy[i].getArchivePolCriteriaCopyNumber()) {
                    result = criteriaCopy[i];
                    break;
                }
            }
        } // found the list of criteria copies
        } // found the first criteria
        } // criteira list
        } // policy exists
        } // copy exists

        TraceUtil.trace3("Exiting");
        return result;
    }

    // copy div
    public boolean beginCopyDivNameDisplay(ChildDisplayEvent evt)
        throws ModelControlException {
        int tile = getPrimaryModel().getLocation();

        ((CCStaticTextField)getChild(COPY_DIV))
            .setValue(COPY_DIV.concat("-")
            .concat(Integer.toString((tile))));

        return true;
    }

    // no copy div
    public boolean beginNoCopyDivNameDisplay(ChildDisplayEvent evt)
        throws ModelControlException {
        int tile = getPrimaryModel().getLocation();

        ((CCStaticTextField)getChild(NO_COPY_DIV))
            .setValue(NO_COPY_DIV.concat("-")
            .concat(Integer.toString(tile)));

        return true;
    }


    // copy header values
    public boolean beginCopyNumberDisplay(ChildDisplayEvent evt)
        throws ModelControlException {
        int tile = getPrimaryModel().getLocation();

        ((CCStaticTextField)getChild(COPY_NUMBER)).setValue(
            SamUtil.getResourceString("archiving.copynumber",
                                      Integer.toString(1+tile)));

        return true;
    }

    // copy time
    public boolean beginCopyTimeDisplay(ChildDisplayEvent evt)
        throws ModelControlException {
        int tile = getPrimaryModel().getLocation();
        boolean foundCriteriaCopy = true;

        // the tile is zero based while copy numbers are 1 based.
        int copyNumber = 1 + tile;
        long copyTime = 4;
        int copyTimeUnit = SamQFSSystemModel.TIME_MINUTE;

        ArchivePolCriteriaCopy theCriteriaCopy =
            getArchivePolCriteriaCopy(copyNumber);

        if (theCriteriaCopy != null) {
            copyTime = theCriteriaCopy.getArchiveAge();
            copyTimeUnit = theCriteriaCopy.getArchiveAgeUnit();
        }

        String s = copyTime == -1 ? "" : Long.toString(copyTime);
        ((CCTextField)getChild(COPY_TIME)).setValue(s);

        s = copyTimeUnit == -1
            ? Integer.toString(SamQFSSystemModel.TIME_MINUTE)
            : Integer.toString(copyTimeUnit);

        ((CCDropDownMenu)getChild(COPY_TIME_UNIT)).setValue(s);

        // if only copy is available, disable its remove button
        List copyList = getAvailableCopyList();
        if (copyList.size() == 1 &&
            copyList.contains(new Integer(copyNumber))) {
            ((CCButton)getChild(REMOVE_COPY)).setDisabled(true);
        } else {
            ((CCButton)getChild(REMOVE_COPY)).setDisabled(false);
        }

        return true;
    }

    // expiration time
    public boolean beginExpirationTimeDisplay(ChildDisplayEvent evt)
        throws ModelControlException {
        int tile = getPrimaryModel().getLocation();
        boolean foundCriteriaCopy = true;

        // the tile is zero based while copy numbers are 1 based.
        int copyNumber = 1 + tile;
        long expirationTime = -1;
        int expirationTimeUnit = -1;

        ArchivePolCriteriaCopy theCriteriaCopy =
            getArchivePolCriteriaCopy(copyNumber);

        if (theCriteriaCopy != null) {
            expirationTime = theCriteriaCopy.getUnarchiveAge();
            expirationTimeUnit = theCriteriaCopy.getUnarchiveAgeUnit();
        }

        String s = expirationTime == -1 ? "" : Long.toString(expirationTime);
        CCTextField expTime = (CCTextField)getChild(EXPIRATION_TIME);
        expTime.setValue(s);

        s = expirationTimeUnit == -1
            ? Integer.toString(SamQFSSystemModel.TIME_MINUTE)
            : Integer.toString(expirationTimeUnit);
        CCDropDownMenu expTimeUnit =
            (CCDropDownMenu)getChild(EXPIRATION_TIME_UNIT);
        expTimeUnit.setValue(s);

        // set the never expire checkbox
        CCCheckBox neverExpire = (CCCheckBox)getChild(NEVER_EXPIRE);
        if (expirationTime == -1) {
            expTime.setDisabled(true);
            expTimeUnit.setDisabled(true);
            neverExpire.setValue("true");
        } else {
            expTime.setDisabled(false);
            expTimeUnit.setDisabled(false);
            neverExpire.setValue("false");
        }
        return true;
    }

    // never expire
    public boolean __beginNeverExpireDisplay(ChildDisplayEvent evt)
        throws ModelControlException {
        int tile = getPrimaryModel().getLocation();
        boolean foundCriteriaCopy = true;

        // the tile is zero based while copy numbers are 1 based.
        int copyNumber = 1 + tile;
        long expirationTime = 0;
        int expirationTimeUnit = -1;

        ArchivePolCriteriaCopy theCriteriaCopy =
            getArchivePolCriteriaCopy(copyNumber);

        if (theCriteriaCopy != null) {
            expirationTime = theCriteriaCopy.getUnarchiveAge();
            expirationTimeUnit = theCriteriaCopy.getUnarchiveAgeUnit();
        }

        // never expire
        // TODO: sticky value here ... ignore for now
        if (expirationTime == -1) {
            ((CCCheckBox)getChild(NEVER_EXPIRE)).setValue("true");
            ((CCTextField)getChild(EXPIRATION_TIME)).setDisabled(true);
            ((CCDropDownMenu)getChild(EXPIRATION_TIME_UNIT)).setDisabled(true);
        } else {
            ((CCCheckBox)getChild(NEVER_EXPIRE)).setValue("false");
            ((CCTextField)getChild(EXPIRATION_TIME)).setDisabled(false);
            ((CCDropDownMenu)getChild(EXPIRATION_TIME_UNIT)).setDisabled(false);
        }

        return true;
    }

    // media pool
    public boolean beginMediaPoolDisplay(ChildDisplayEvent evt)
        throws ModelControlException {
        int copyNumber = getPrimaryModel().getLocation() + 1;

        String mediaPool = "", scratchPool = "", poolName = "";
        int mediaType = -1;
        ArchiveCopy theCopy = getArchiveCopy(copyNumber);
        if (theCopy != null) {
        ArchiveVSNMap map = theCopy.getArchiveVSNMap();
        if (map != null) {
        String [] pools = SamQFSUtil
            .getStringsFromCommaStream(map.getPoolExpression());
        int length = 0;
        if (pools != null) {
            length = pools.length;
            if (length >= 2) {
                mediaPool = pools[0];
                scratchPool = pools[1];
            } else if (length == 1) {
                mediaPool = pools[0];
            }
            mediaType = map.getArchiveMediaType();
        } // end if pools
        } // end if map
        } // end if the copy

        if (mediaPool != null && mediaPool.length() > 0) {
            mediaPool = mediaPool.concat(":")
                .concat(Integer.toString(mediaType));
        }

        if (scratchPool != null && scratchPool.length() > 0) {
            scratchPool = scratchPool.concat(":")
                .concat(Integer.toString(mediaType));
        }
        ((CCDropDownMenu)getChild(MEDIA_POOL)).setValue(mediaPool);
        ((CCDropDownMenu)getChild(SCRATCH_POOL)).setValue(scratchPool);

        return true;
    }

    // media type
    public boolean beginMediaTypeDisplay(ChildDisplayEvent evt)
        throws ModelControlException {
        int copyNumber = getPrimaryModel().getLocation() + 1;

        return false;
    }

    // directly assigned vsns
    public boolean beginAssignedVSNsDisplay(ChildDisplayEvent evt)
        throws ModelControlException {
        int copyNumber = getPrimaryModel().getLocation() + 1;
        ArchiveCopy theCopy = getArchiveCopy(copyNumber);

        if (theCopy != null) {
            ArchiveVSNMap vsnMap = theCopy.getArchiveVSNMap();
            ((CCTextField)getChild(ASSIGNED_VSNS))
                .setValue(vsnMap.getMapExpression());
        }
        return true;
    }

    // available media
    public boolean beginAvailableMediaDisplay(ChildDisplayEvent evt)
        throws ModelControlException {
        int copyNumber = getPrimaryModel().getLocation() + 1;

        String availableMedia = "";
        ArchiveCopy theCopy = getArchiveCopy(copyNumber);

        int maxPopupLength = 10;
        int maxInlineLength = 20;

        NonSyncStringBuffer buf = new NonSyncStringBuffer();
        int vsnCount = -1;
        if (theCopy != null) {
        ArchiveVSNMap map = theCopy.getArchiveVSNMap();
        if (map != null) {
        try {
        String [] vsn = map.getMemberVSNNames();
        if (vsn != null) {
            vsnCount = vsn.length;
            for (int i = 0; i < vsn.length; i++) {
                buf.append(vsn[i]).append(", ");
            } // end for each vsn
        } // if vsn
        } catch (SamFSException sfe) {
            // TODO:log
        }
        } // if map
        } // if copy

        // all vsns in string
        String allVSNs = buf.toString();
        if (allVSNs.indexOf(",") != -1)
            allVSNs = allVSNs.substring(0, allVSNs.length() -2);

        // get first 15 chars of the allVSNs string and append ' ... N' to it
        // where N is the total number of the available VSNS. The resulting
        // string is what we'll display to the user.
        String display = null;
        if (allVSNs.length() > maxInlineLength) {
            display = allVSNs.substring(0, maxInlineLength);

            display = display.concat(" ... ")
                .concat("(").concat(""+vsnCount).concat(")");
        } else {
            String sc = vsnCount == -1 ? "" : Integer.toString(vsnCount);
            display = allVSNs.concat(" (" + sc).concat(")");
        }

        ((CCStaticTextField)getChild(AVAILABLE_MEDIA)).setValue(display);
        ((CCHiddenField)getChild(AVAILABLE_MEDIA_STRING)).setValue(allVSNs);

        return true;
    }

    // enable recycling
    public boolean beginEnableRecyclingDisplay(ChildDisplayEvent evt)
        throws ModelControlException {
        int copyNumber = getPrimaryModel().getLocation() + 1;

        String enableRecycling = "false";
        ArchiveCopy theCopy = getArchiveCopy(copyNumber);
        if (theCopy != null) {
            enableRecycling = theCopy.isIgnoreRecycle() ? "false" : "true";
        }

        ((CCCheckBox)getChild(ENABLE_RECYCLING)).setValue(enableRecycling);
        return true;
    }


    /**
     * handle the save button
     */
    public boolean save() throws ModelControlException {
        DefaultModel model = (DefaultModel)getPrimaryModel();
        if (model == null) {
            return false;
        }
        model.beforeFirst();

        return true;
    }

    /**
     * populate the non-dynamic drop downs on this page i.e.
     *  - copy time unit
     *  - copy expiration time unit
     *  - releaser behavior
     */
    private void populateDropDownMenus() throws SamFSException {
        // retrieve the server name
        String serverName =
            ((CommonViewBeanBase)getParentViewBean()).getServerName();

        // copy time units
        CCDropDownMenu dropDown =
            (CCDropDownMenu)getChild(COPY_TIME_UNIT);
        dropDown.setOptions(new OptionList(SelectableGroupHelper.Time.labels,
                                           SelectableGroupHelper.Time.values));

        // expiration time units
        dropDown = (CCDropDownMenu)getChild(EXPIRATION_TIME_UNIT);
        dropDown.setOptions(new OptionList(SelectableGroupHelper.Time.labels,
                                           SelectableGroupHelper.Time.values));

        // releaser behavior
        dropDown = (CCDropDownMenu)getChild(RELEASER_BEHAVIOR);
        dropDown.setOptions(new OptionList(
                                SelectableGroupHelper.ReleaseOptions.labels,
                                SelectableGroupHelper.ReleaseOptions.values));

        // populate pool drop downs
        String [][] mediaPool = PolicyUtil.getAllVSNPoolNames(serverName);
        dropDown = (CCDropDownMenu)getChild(MEDIA_POOL);
            dropDown.setOptions(new OptionList(mediaPool[0], mediaPool[1]));

        dropDown = (CCDropDownMenu)getChild(SCRATCH_POOL);
        dropDown.setOptions(new OptionList(mediaPool[0], mediaPool[1]));

        // media type dropdown
        Object [][]mediaTypes = PolicyUtil.getAvailableMediaTypes(serverName);
        dropDown = (CCDropDownMenu)getChild(MEDIA_TYPE);
        dropDown.setOptions(new OptionList((String [])mediaTypes[0],
                                           (String [])mediaTypes[1]));
    }

    // these should be put in a separate  base class when we support the
    // allsets policy
    protected ArchiveCopy getArchiveCopy(int copyNumber) {
        ArchivePolicy thePolicy = getArchivePolicy();
        ArchiveCopy theCopy = null;

        if (thePolicy != null)
            theCopy = thePolicy.getArchiveCopy(copyNumber);

        return theCopy;
    }

    // get the currently managed policy
    protected ArchivePolicy getArchivePolicy() {
        ISPolicyDetailsViewBean parent =
            (ISPolicyDetailsViewBean)getParentViewBean();
        String serverName = parent.getServerName();
        String policyName = parent.getPolicyName();
        ArchivePolicy thePolicy = null;

        try {
            thePolicy = (ArchivePolicy)
                RequestManager.getRequest().getAttribute(ARCHIVE_POLICY);

            if (thePolicy == null) {
                thePolicy = SamUtil.getModel(serverName).
                    getSamQFSSystemArchiveManager().
                    getArchivePolicy(policyName);

                RequestManager.getRequest().
                    setAttribute(ARCHIVE_POLICY, thePolicy);
            }
        } catch (SamFSException sfe) {
            // TODO
        }

        return thePolicy;
    }

    // get a list of all the available copies
    protected List getAvailableCopyList() {
        List availableCopies = (List)
            RequestManager.getRequest().getAttribute(AVAILABLE_COPY_LIST);

        if (availableCopies == null) {
            availableCopies = new ArrayList();

            ArchivePolicy thePolicy = getArchivePolicy();

            ArchiveCopy [] copy = thePolicy.getArchiveCopies();

            for (int i = 0; i < copy.length; i++) {
                availableCopies.add(new Integer(copy[i].getCopyNumber()));
            }

            RequestManager.getRequest().setAttribute(AVAILABLE_COPY_LIST,
                                                     availableCopies);
        }

        return availableCopies;
    }


    // handler for the edit advanced options
    public void handleEditCopyOptionsRequest(RequestInvocationEvent rie)
        throws ServletException, IOException, ModelControlException {
        int tileIndex = ((TiledViewRequestInvocationEvent)rie).getTileNumber();

        CommonViewBeanBase source =
            (CommonViewBeanBase)getParentViewBean();

        DefaultModel model = (DefaultModel)getPrimaryModel();
        model.setLocation(tileIndex);

        String expirationTime = (String)model.getValue(EXPIRATION_TIME);
        TraceUtil.trace3("expiration time for copy : " + tileIndex +
                           " is : " + expirationTime);

        TraceUtil.trace3("selected tile = " + tileIndex);
        TraceUtil.trace3("selected copy number = " + (1+tileIndex));


        int copyNumber = tileIndex + 1;
        ArchiveCopy theCopy = getArchiveCopy(copyNumber);
        ArchiveVSNMap theMap = theCopy.getArchiveVSNMap();
        int mediaType = theMap.getArchiveMediaType();

        CommonViewBeanBase target = (CommonViewBeanBase)
            getViewBean(CopyOptionsViewBean.class);
        source.setPageSessionAttribute(Constants.Archive.COPY_NUMBER,
                                       new Integer(copyNumber));
        source.setPageSessionAttribute(Constants.Archive.COPY_MEDIA_TYPE,
                                       new Integer(mediaType));
        // bread crumb
        BreadCrumbUtil.breadCrumbPathForward(source,
           PageInfo.getPageInfo().getPageNumber(source.getName()));

        source.forwardTo(target);
    }

    private static final String ARCHIVE_POLICY =
        "http_request_archive_policy";
    private static final String AVAILABLE_COPY_LIST =
        "http_request_available_copy_list";
}
