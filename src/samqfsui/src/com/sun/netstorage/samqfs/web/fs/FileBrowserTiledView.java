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

// ident	$Id: FileBrowserTiledView.java,v 1.6 2008/03/17 14:43:34 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.web.util.CommonTiledViewBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.Filter;
import java.io.IOException;
import javax.servlet.ServletException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.ChildDisplayEvent;
import com.sun.web.ui.model.CCActionTableModel;
import com.iplanet.jato.view.event.TiledViewRequestInvocationEvent;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.web.ui.view.html.CCImageField;
import java.io.File;

public class FileBrowserTiledView extends CommonTiledViewBase {

    private static final String ICON_DISK_CACHE = "IconDiskCache";

    public FileBrowserTiledView(
        View parent,
        CCActionTableModel model,
        String name) {
        super(parent, model, name);

        // Set the primary model to evoke JATO's TiledView behavior,
        // which will automatically add the current model row index to
        // each qualified name.
        setPrimaryModel(model);
    }

    public void handleNameHrefRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {

        model.setRowIndex(
            ((TiledViewRequestInvocationEvent)rie).getTileNumber());
        String selectedDirectory = getDisplayFieldStringValue("NameHref");
        FileBrowserViewBean parentViewBean =
            (FileBrowserViewBean) getParentViewBean();
        String currentDirectory = parentViewBean.getCurrentDirectory();

        // Only happens if current directory is not "/"
        if ("fs.filebrowser.uponelevel".equals(selectedDirectory)) {
            currentDirectory = upOneLevel(currentDirectory);
        } else if ("/".equals(currentDirectory)) {
            currentDirectory = currentDirectory.concat(selectedDirectory);
        } else {
            currentDirectory = currentDirectory.concat(File.separator)
                .concat(selectedDirectory);
        }

        // Reset filter if it is set.  User wants to see the content of the
        // directory that matches the filter.  It makes no sense to carry over
        // the filter criteria to the next level.
        parentViewBean.removePageSessionAttribute(parentViewBean.FILTER_VALUE);

        parentViewBean.setCurrentDirectory(currentDirectory);
        parentViewBean.forwardTo(getRequestContext());
    }

    private String upOneLevel(String currentDirectory) {
        String [] dirArray = currentDirectory.split(File.separator);
        StringBuffer buf = new StringBuffer();
        for (int i = 0; i < dirArray.length - 1; i++) {
            // Ignore leading slashes
            if ("".equals(dirArray[i])) {
                continue;
            }
            buf.append(File.separator);
            buf.append(dirArray[i]);
        }
        return buf.toString();
    }

    /**
     * Method to append alt text to images due to 508 compliance
     */
    public boolean beginIconDiskCacheDisplay(ChildDisplayEvent event)
        throws ModelControlException {
        CCImageField myImageField = (CCImageField) getChild(ICON_DISK_CACHE);

        String onlineStatus = (String) myImageField.getValue();
        String altString = null;
        if (Constants.Image.ICON_ONLINE.equals(onlineStatus)) {
            altString =
                SamUtil.getResourceString("fs.filebrowser.online");
        } else if (Constants.Image.ICON_PARTIAL_ONLINE.equals(onlineStatus)) {
            altString =
                SamUtil.getResourceString("fs.filebrowser.partialonline");
        } else if (Constants.Image.ICON_OFFLINE.equals(onlineStatus)) {
            altString =
                SamUtil.getResourceString("fs.filebrowser.offline");
        } else {
            altString = "";
        }

        myImageField.setAlt(altString);
        myImageField.setTitle(altString);

        return true;
    }

    /**
     * Method to append alt text to images due to 508 compliance
     */
    public boolean beginIconCopy1Display(ChildDisplayEvent event)
        throws ModelControlException {

        for (int i = 1; i <= 4; i++) {
            CCImageField myImageField =
                (CCImageField) getChild("IconCopy".concat(Integer.toString(i)));
            setAltText(myImageField, i);
        }

        return true;
    }

    private void setAltText(CCImageField myImageField, int copyNumber) {
        String imageString = (String) myImageField.getValue();
        imageString = imageString == null ? "" : imageString.trim();

        if (imageString.length() == 0 ||
            Constants.Image.ICON_BLANK.equals(imageString)) {
            // no icon
            myImageField.setAlt("");
            myImageField.setTitle("");
            return;
        }

        int imageBase = -1;
        final int DISK_BASE = 0;
        final int HONEYCOMB_BASE = 1;
        final int TAPE_BASE = 2;

        if (imageString.indexOf(Constants.Image.ICON_DISK_PREFIX) != -1) {
            imageBase = DISK_BASE;
        } else if (imageString.indexOf(Constants.Image.ICON_HONEYCOMB) != -1) {
            imageBase = HONEYCOMB_BASE;
        } else {
            imageBase = TAPE_BASE;
        }

        boolean damaged =
            imageString.indexOf(Constants.Image.ICON_DAMAGED_SUFFIX) != -1;

        // Construct alt text
        StringBuffer buf = new StringBuffer(
            imageBase == DISK_BASE ?
                SamUtil.getResourceString(
                    "fs.filebrowser.alttext.disk",
                    new String [] {
                        Integer.toString(copyNumber)}) :
                imageBase == TAPE_BASE ?
                    SamUtil.getResourceString(
                        "fs.filebrowser.alttext.tape",
                        new String [] {
                            Integer.toString(copyNumber)}) :
                    SamUtil.getResourceString(
                        "fs.filebrowser.alttext.honeycomb",
                        new String [] {
                            Integer.toString(copyNumber)}));

        if (damaged) {
            buf.append(" - ").append(
                SamUtil.getResourceString("fs.filebrowser.alttext.damaged"));
        }

        myImageField.setAlt(buf.toString());
        myImageField.setTitle(buf.toString());
    }
}
