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

// ident	$Id: ShowPopUpWindowViewBeanBase.java,v 1.8 2008/05/16 19:39:26 am143972 Exp $

package com.sun.netstorage.samqfs.web.admin;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.sun.netstorage.samqfs.web.util.CommonSecondaryViewBeanBase;
import com.sun.web.ui.common.CCClientSniffer;
import com.sun.web.ui.view.html.CCTextArea;

/**
 *  This class is the view bean base for all pop up windows that are used to
 *  show the file/status content with a text area.  This class should not be
 *  used directly but to extend from it.
 */

public abstract class ShowPopUpWindowViewBeanBase
    extends CommonSecondaryViewBeanBase {

    protected static final String TEXT_AREA = "TextArea";

    private static int NORMAL_TEXT_AREA_COL_SIZE_FOR_MOZILLA = 120;
    private static int NORMAL_TEXT_AREA_ROW_SIZE_FOR_MOZILLA = 27;
    private static int NORMAL_TEXT_AREA_COL_SIZE_FOR_IE = 140;
    private static int NORMAL_TEXT_AREA_ROW_SIZE_FOR_IE = 27;

    private static int SMALL_TEXT_AREA_COL_SIZE_FOR_MOZILLA = 80;
    private static int SMALL_TEXT_AREA_ROW_SIZE_FOR_MOZILLA = 8;
    private static int SMALL_TEXT_AREA_COL_SIZE_FOR_IE = 100;
    private static int SMALL_TEXT_AREA_ROW_SIZE_FOR_IE = 8;

    // boolean to keep track if the show pop up window is in size NORMAL or
    // SMALL.  This is needed to adjust the text area for different browsers.
    protected boolean isWindowSizeNormal = true;

    /**
     * Constructor
     */
    public ShowPopUpWindowViewBeanBase(String pageName, String displayURL) {
        super(pageName, displayURL);
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        super.registerChildren();
        registerChild(TEXT_AREA, CCTextArea.class);
    }

    /**
     * Check the child component is valid
     *
     * @param name of child compoment
     * @return the boolean variable
     */
    protected boolean isChildSupported(String name) {
        if (name.equals(TEXT_AREA)) {
            return true;
        } else {
            return super.isChildSupported(name);
        }
    }
    /**
     * Instantiate each child view.
     *
     * @param name The name of the child view
     * @return View The instantiated child view
     */
    protected View createChild(String name) {
        View child = null;
        if (name.equals(TEXT_AREA)) {
            child =  new CCTextArea(this, name, null);
        } else if (super.isChildSupported(name)) {
            child = super.createChild(name);
        } else {
            throw new IllegalArgumentException(new StringBuffer(
                "Invalid child name [").append(name).append("]").toString());
        }

        return (View) child;
    }

    protected void setWindowSizeNormal(boolean isNormal) {
        isWindowSizeNormal = isNormal;
    }

    /**
     * Called as notification that the JSP has begun its display processing
     * @param event The DisplayEvent
     */
    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        CCTextArea myTextArea = (CCTextArea) getChild(TEXT_AREA);
        boolean isIE = new CCClientSniffer(
            RequestManager.getRequestContext().getRequest()).isIe();
        // adjust textArea to a bigger size if isShowContent is true
        if (isWindowSizeNormal) {
            if (isIE) {
                myTextArea.setCols(NORMAL_TEXT_AREA_COL_SIZE_FOR_IE);
                myTextArea.setRows(NORMAL_TEXT_AREA_ROW_SIZE_FOR_IE);
            } else {
                myTextArea.setCols(NORMAL_TEXT_AREA_COL_SIZE_FOR_MOZILLA);
                myTextArea.setRows(NORMAL_TEXT_AREA_ROW_SIZE_FOR_MOZILLA);
            }
        } else {
            if (isIE) {
                myTextArea.setCols(SMALL_TEXT_AREA_COL_SIZE_FOR_IE);
                myTextArea.setRows(SMALL_TEXT_AREA_ROW_SIZE_FOR_IE);
            } else {
                myTextArea.setCols(SMALL_TEXT_AREA_COL_SIZE_FOR_MOZILLA);
                myTextArea.setRows(SMALL_TEXT_AREA_ROW_SIZE_FOR_MOZILLA);
            }
        }
    }
}
