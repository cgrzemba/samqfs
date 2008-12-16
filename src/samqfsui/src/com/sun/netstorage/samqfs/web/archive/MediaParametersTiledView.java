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

// ident	$Id: MediaParametersTiledView.java,v 1.15 2008/12/16 00:10:55 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.ChildDisplayEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.BufferDirective;
import com.sun.netstorage.samqfs.web.model.archive.GlobalArchiveDirective;
import com.sun.netstorage.samqfs.web.util.Capacity;
import com.sun.netstorage.samqfs.web.util.CommonTiledViewBase;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.view.html.CCCheckBox;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCTextField;
import javax.servlet.http.HttpServletRequest;

/**
 * This is tiled view class for BufferTable in Admin-SetUp page
 */

public class MediaParametersTiledView extends CommonTiledViewBase {

    public MediaParametersTiledView(View parent,
                                    CCActionTableModel model,
                                    String name) {

        super(parent, model, name);
    }

    public void mapRequestParameters(HttpServletRequest request)
        throws ModelControlException {

        super.mapRequestParameters(request);

    }

    public boolean beginMaxArchiveFileSizeDisplay(ChildDisplayEvent event)
        throws ModelControlException {
        TraceUtil.trace3("Entering");

        int index = model.getRowIndex();
        CCTextField value = (CCTextField) getChild("MaxArchiveFileSize", index);

        try {
            String size = getArchiveEditableValue("MaxArchiveFileSize", index);
            // this size is always stored as bytes
            // Now when it is displayed, it has to be displayed with meaningful
            // units. the logic layer only takes in bytes for this field
            // So use Capacity class to do the conversion here and save the unit
            // in the hidden field
            size = (size != null) ? size.trim() : "";
            if (!size.equals("")) {
                Capacity archmax = Capacity.newExactCapacity(
                    Long.parseLong(size), SamQFSSystemModel.SIZE_B);

                value.setValue(String.valueOf(archmax.getSize()));
                String unitStr = String.valueOf(archmax.getUnit());
                model.setValue("MaxArchiveFileSizeHiddenField", unitStr);
            } else {
                value.setValue("");
                model.setValue("MaxArchiveFileSizeHiddenField",
                    new Integer(SamQFSSystemModel.SIZE_MB).toString());
            }

        } catch (SamFSException smfex) {
            SamUtil.processException(smfex,
                                     this.getClass(),
                                     "beginMaxArchiveFileSizeDisplay",
                                     "Fail to set max archive file size value",
                                     getServerName());

            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "ArchiveSetupViewBean.error.failedPopulate",
                smfex.getSAMerrno(),
                smfex.getMessage(),
                getServerName());

        } catch (Exception ex) {
            SamUtil.processException(ex,
                                     this.getClass(),
                                     "beginMaxArchiveFileSizeDisplay",
                                     "Fail to set max archive file size value",
                                     getServerName());

            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "ArchiveSetupViewBean.error.failedPopulate",
                -2017, // samerrno not available
                ex.getMessage(),
                getServerName());
        }
        TraceUtil.trace3("Exiting");
        return true;
    }

    public boolean beginMaxArchiveFileSizeUnitDisplay(ChildDisplayEvent event)
        throws ModelControlException {
        TraceUtil.trace3("Entering");

        int index = model.getRowIndex();
        // init dropdown Maximum Archive File Size Unit
        CCDropDownMenu dropDown =
            (CCDropDownMenu)getChild("MaxArchiveFileSizeUnit", index);
        dropDown.setOptions(new OptionList(
            SelectableGroupHelper.Sizes.labels,
            SelectableGroupHelper.Sizes.values));

        try {
            String unitStr =
                (String) model.getValue("MaxArchiveFileSizeHiddenField");
            if (unitStr != null) {
                dropDown.setValue(unitStr);
            }
        } catch (Exception ex) {
            SamUtil.processException(ex,
                                     this.getClass(),
                                     "beginMaxArchiveFileSizeUnitDisplay",
                                     "Fail to set max archive file size unit",
                                     getServerName());

            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "ArchiveSetupViewBean.error.failedPopulate",
                -2017,
                ex.getMessage(),
                getServerName());
        }
        TraceUtil.trace3("Exiting");
        return true;
    }

    public boolean beginMinSizeForOverflowUnitDisplay(ChildDisplayEvent event)
        throws ModelControlException {

        TraceUtil.trace3("Entering");

        int index = model.getRowIndex();
        // init dropdown MinSizeForOverflow Unit
        CCDropDownMenu dropDown =
            (CCDropDownMenu)getChild("MinSizeForOverflowUnit", index);
        dropDown.setOptions(new OptionList(
            SelectableGroupHelper.Sizes.labels,
            SelectableGroupHelper.Sizes.values));

        try {
            String unitStr =
                (String) model.getValue("MinSizeForOverflowHiddenField");
            if (unitStr != null) {
                dropDown.setValue(unitStr);
            }
        } catch (Exception ex) {
            SamUtil.processException(ex,
                                     this.getClass(),
                                     "beginMinSizeForOverflowUnitDisplay",
                                     "Fail to set min size for overflow unit",
                                     getServerName());

            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "ArchiveSetupViewBean.error.failedPopulate",
                -2017,
                ex.getMessage(),
                getServerName());
        }
        TraceUtil.trace3("Exiting");
        return true;
    }

    public boolean beginMinSizeForOverflowDisplay(ChildDisplayEvent event)
        throws ModelControlException {

        TraceUtil.trace3("Entering");
        int index = model.getRowIndex();
        CCTextField value = (CCTextField) getChild("MinSizeForOverflow", index);
        try {
            // SAM always saves the size in bytes, but when it is displayed
            // to the user, use the appropriate units
            String size = getArchiveEditableValue("MinSizeForOverflow", index);
            TraceUtil.trace3("Min overflow size = " + size);
            // If size is "" or null, set size to -1
            // set size unit to default to MB
            size = (size != null) ? size.trim() : "";
            if (!size.equals(""))	{
                Capacity ovflmin = Capacity.newExactCapacity(
                    Long.parseLong(size), SamQFSSystemModel.SIZE_B);
                value.setValue(String.valueOf(ovflmin.getSize()));
                String unitStr = String.valueOf(ovflmin.getUnit());
                model.setValue("MinSizeForOverflowHiddenField", unitStr);
            } else {
                value.setValue("");
                model.setValue("MinSizeForOverflowHiddenField",
                    new Integer(SamQFSSystemModel.SIZE_MB).toString());
            }
        } catch (SamFSException smfex) {
            SamUtil.processException(smfex,
                                     this.getClass(),
                                     "beginMinSizeForOverflowDisplay",
                           "Failed to set the minimum size for overflow value",
                                     getServerName());

            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "ArchiveSetupViewBean.error.failedPopulate",
                smfex.getSAMerrno(),
                smfex.getMessage(),
                getServerName());

        } catch (Exception ex) {

            SamUtil.processException(ex,
                                     this.getClass(),
                                     "beginMinSizeForOverflowDisplay",
                           "Failed to set the minimum size for overflow value",
                                     getServerName());

            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "ArchiveSetupViewBean.error.failedPopulate",
                -2017,
                ex.getMessage(),
                getServerName());

        }
        TraceUtil.trace3("Exiting");
        return true;
    }

    public boolean beginArchiveBufferLockDisplay(ChildDisplayEvent event)
        throws ModelControlException {

        TraceUtil.trace3("Entering");
        int index = model.getRowIndex();
        CCCheckBox lock = (CCCheckBox) getChild("ArchiveBufferLock", index);
        try {
            lock.setValue(getArchiveEditableValue("ArchiveBufferLock", index));
        } catch (SamFSException smfex) {

            SamUtil.processException(smfex,
                                     this.getClass(),
                                     "beginArchiveBufferLockDisplay",
                                     "Failed to set the lock",
                                     getServerName());

            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "ArchiveSetupViewBean.error.failedPopulate",
                smfex.getSAMerrno(),
                smfex.getMessage(),
                getServerName());

        } catch (Exception ex) {

            SamUtil.processException(ex,
                                     this.getClass(),
                                     "beginArchiveBufferLockDisplay",
                                     "Failed to set the lock",
                                     getServerName());

            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "ArchiveSetupViewBean.error.failedPopulate",
                -2017,
                ex.getMessage(),
                getServerName());
        }

        TraceUtil.trace3("Exiting");
        return true;
    }

    public boolean beginArchiveBufferSizeDisplay(ChildDisplayEvent event) throws
		ModelControlException {

        TraceUtil.trace3("Entering");
        int index = model.getRowIndex();
        CCTextField buffer = (CCTextField) getChild("ArchiveBufferSize", index);
        try {
            String size = getArchiveEditableValue("ArchiveBufferSize", index);
            if (!size.equals("-1")) {
                buffer.setValue(size);
            }
        } catch (SamFSException smfex) {

            SamUtil.processException(smfex,
                                     this.getClass(),
                                     "beginArchiveBufferSizeDisplay",
                                     "Failed to set buffer size",
                                     getServerName());

            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "ArchiveSetupViewBean.error.failedPopulate",
                smfex.getSAMerrno(),
                smfex.getMessage(),
                getServerName());

        } catch (Exception ex) {

            SamUtil.processException(ex,
                                     this.getClass(),
                                     "beginArchiveBufferSizeDisplay",
                                     "Failed to set buffer size",
                                     getServerName());

            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "ArchiveSetupViewBean.error.failedPopulate",
                -2017,
                ex.getMessage(),
                getServerName());
        }
        TraceUtil.trace3("Exiting");
        return true;
    }

    private String getArchiveEditableValue(String type, int index)
		throws ModelControlException, SamFSException {

        TraceUtil.trace3("Entering");
        String value = "";

        GlobalArchiveDirective globalDir =
            ((ArchiveSetUpViewBean)getParentViewBean()).getGlobalDirective();
        if (globalDir == null)
            throw new SamFSException(null, -2015);

        if (type.equals("ArchiveBufferSize")) {
            BufferDirective[] buffers = globalDir.getBufferSize();
            if (buffers.length != 0) {
                BufferDirective buffer = buffers[index];
                if (buffer != null) {
                    value = String.valueOf(buffer.getSize());

                    int mtype = buffer.getMediaType();
                    String mString = SamUtil.getMediaTypeString(mtype);
                }
            }
        } else if (type.equals("ArchiveBufferLock")) {
            BufferDirective[] buffers = globalDir.getBufferSize();
            if (buffers.length != 0) {
                BufferDirective buffer = buffers[index];
                if (buffer != null) {
                    value = (buffer.isLocked()) ? "true" : "false";

                    int mtype = buffer.getMediaType();
                    String mString = SamUtil.getMediaTypeString(mtype);
                }
            } else {
                value = "false";
            }
        } else if (type.equals("MaxArchiveFileSize")) {
            BufferDirective[] buffers = globalDir.getMaxFileSize();
            if (buffers.length != 0) {
                BufferDirective buffer = buffers[index];
                if (buffer != null) {
                    value = buffer.getSizeString();

                    int mtype = buffer.getMediaType();
                    String mString = SamUtil.getMediaTypeString(mtype);
                }
            }
        } else if (type.equals("MinSizeForOverflow")) {
            BufferDirective[] buffers = globalDir.getMinFileSizeForOverflow();
            if (buffers.length != 0) {
                BufferDirective buffer = buffers[index];
                if (buffer != null) {
                    value = buffer.getSizeString();

                    int mtype = buffer.getMediaType();
                    String mString = SamUtil.getMediaTypeString(mtype);
                }
            }
        }
        value = (value != null) ? value.trim() : "";
        TraceUtil.trace3(new NonSyncStringBuffer(
            "Value for ").append(type).append(" is ").append(value).toString());
        TraceUtil.trace3("Exiting");
        return value;
    }

    public boolean beginStageBufferLockDisplay(ChildDisplayEvent event)
        throws ModelControlException {

        TraceUtil.trace3("Entering");
        int index = model.getRowIndex();

        try {
            ((CCCheckBox) getChild("StageBufferLock", index)).setValue(
                getStagerEditableValue("StageBufferLock", index));
        } catch (SamFSException smfex) {
            SamUtil.processException(
                smfex,
                this.getClass(),
                "beginLockDisplay",
                "Failed to retrieve buffer directives",
                getServerName());

            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "ArchiveSetupViewBean.error.failedPopulate",
                smfex.getSAMerrno(),
                smfex.getMessage(),
                getServerName());

        } catch (Exception ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "beginLockDisplay",
                "Failed to retrieve buffer directives",
                getServerName());

            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "ArchiveSetupViewBean.error.failedPopulate",
                -2017,
                ex.getMessage(),
                getServerName());
        }
        TraceUtil.trace3("Exiting");
        return true;
    }

    public boolean beginStageBufferSizeDisplay(ChildDisplayEvent event)
        throws ModelControlException {
        TraceUtil.trace3("Entering");

        int index = model.getRowIndex();

        try {
            String bufferValue =
                getStagerEditableValue("StageBufferSize", index);

            if (bufferValue != null && !bufferValue.equals("-1")) {
                ((CCTextField) getChild("StageBufferSize", index)).
                    setValue(bufferValue);
            }
        } catch (SamFSException smfex) {
            SamUtil.processException(
                smfex,
                this.getClass(),
                "beginStageBuffersizeDisplay",
                "Failed to retrieve buffer directives",
                getServerName());

            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "ArchiveSetupViewBean.error.failedPopulate",
                smfex.getSAMerrno(),
                smfex.getMessage(),
                getServerName());

        } catch (Exception ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "beginStageBuffersizeDisplay",
                "Failed to retrieve buffer directives",
                getServerName());

            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "ArchiveSetupViewBean.error.failedPopulate",
                -2017,
                ex.getMessage(),
                getServerName());
        }
        TraceUtil.trace3("Exiting");
        return true;
    }

    private String getStagerEditableValue(String type, int index)
        throws ModelControlException, SamFSException {

        TraceUtil.trace3("Entering");
        String value = "";
        BufferDirective[] stageBuffers =
            ((ArchiveSetUpViewBean)getParentViewBean()).
            getStageBufferDirectives();
        if (stageBuffers == null || stageBuffers.length == 0) {
            if (type.equals("StageBufferLock")) {
                return "false";
            } else if (type.equals("StageBufferSize")) {
                return "";
            }
        }
        // stageBuffers is not null or empty
        BufferDirective stagebuffer = stageBuffers[index];
        if (stagebuffer != null) {
            if (type.equals("StageBufferLock")) {
                value = (stagebuffer.isLocked()) ? "true" : "false";
            } else if (type.equals("StageBufferSize")) {
                value = String.valueOf(stagebuffer.getSize());
            }
        }
        value = (value != null) ? value.trim() : "";
        TraceUtil.trace3("Exiting");
        return value;
    }

    private String getServerName() {
        return (String)
            ((CommonViewBeanBase)getParentViewBean()).getServerName();
    }
}
