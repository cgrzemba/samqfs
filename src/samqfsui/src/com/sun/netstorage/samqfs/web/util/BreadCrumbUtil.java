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

// ident	$Id: BreadCrumbUtil.java,v 1.15 2008/12/16 00:12:25 am143972 Exp $

package com.sun.netstorage.samqfs.web.util;

import com.iplanet.jato.view.ViewBean;
import com.sun.web.ui.model.CCBreadCrumbsModel;

public class BreadCrumbUtil {

    /**
     * Used to set the page session attribute of the breadcrumbs
     * going forward.
     * Used when not clicking on a breadcrumb link
     * DO NOT USE THIS METHOD if you are writing any new code.  Use the one
     * without the targetView parameter, and use the CommonViewBeanBase::
     * forwardTo() wrapper in conjunction.
     */

    public static void breadCrumbPathForward(
        ViewBean sourceView,
        ViewBean targetView,
        Integer pageId) {

        Integer [] intArray = (Integer []) sourceView.getPageSessionAttribute(
            Constants.SessionAttributes.PAGE_PATH);
        TraceUtil.trace3("Got the page session attribute from the sourceView");

        // this is the starting page, so just create an
        // array, adding the pageId as the only element
        if (intArray == null) {
            TraceUtil.trace3("intArray is null");
            targetView.setPageSessionAttribute(
                Constants.SessionAttributes.PAGE_PATH,
                new Integer [] { pageId });
        } else {
            TraceUtil.trace3("Copy the array going forward");
            Integer [] newIntArray = new Integer[intArray.length + 1];
            int i = 0;
            for (i = 0; i < intArray.length; i++) {
                newIntArray[i] = intArray[i];
            }
            newIntArray[i] = pageId;
            targetView.setPageSessionAttribute(
                Constants.SessionAttributes.PAGE_PATH,
                newIntArray);
        }
    }

    /**
     * Set the BreadCrumb component while going forward (appending links)
     * @param sourceView - View Bean where the page is forwarding from
     * @param pageID - See PageInfo class
     * @return none
     */
    public static void breadCrumbPathForward(
        ViewBean sourceView,
        Integer pageId) {

        Integer [] intArray = (Integer []) sourceView.getPageSessionAttribute(
            Constants.SessionAttributes.PAGE_PATH);
        TraceUtil.trace3("Got the page session attribute from the sourceView");

        // this is the starting page, so just create an
        // array, adding the pageId as the only element
        if (intArray == null) {
            TraceUtil.trace3("intArray is null");
            sourceView.setPageSessionAttribute(
                Constants.SessionAttributes.PAGE_PATH,
                new Integer [] { pageId });
        } else {
            TraceUtil.trace3("Copy the array going forward");
            Integer [] newIntArray = new Integer[intArray.length + 1];
            int i = 0;
            for (i = 0; i < intArray.length; i++) {
                newIntArray[i] = intArray[i];
            }
            newIntArray[i] = pageId;
            sourceView.setPageSessionAttribute(
                Constants.SessionAttributes.PAGE_PATH,
                newIntArray);
        }
    }

    /**
     * Pre-condition: A valid pageId is given, and will be present
     * in the array
     * Used when clicking on the breadcrumb links.
     * DO NOT USE THIS METHOD if you are writing any new code.  Use the one
     * without the targetView parameter, and use the CommonViewBeanBase::
     * forwardTo() wrapper in conjunction.
     */
    public static void breadCrumbPathBackward(
        ViewBean sourceView,
        ViewBean targetView,
        Integer pageId,
        String displayValue) {

        // need to check if the breadcrumb that was clicked on
        // is the same as another one in the trail
        int displayValueInt = Integer.parseInt(displayValue);
        Integer [] intArray = (Integer []) sourceView.getPageSessionAttribute(
            Constants.SessionAttributes.PAGE_PATH);
        int start = 0;

        // only look at last Constants.BreadCrumb.MAX_LINKS in the array
        if (intArray.length > Constants.BreadCrumb.MAX_LINKS) {
            start = intArray.length - Constants.BreadCrumb.MAX_LINKS;
        }

        // detect which breadcrumb the user clicked on
        // there can be two breadcrumbs in the path with the same name
        int savedIndex = -1;
        int myCount = -1;
        for (int i = start; i < intArray.length; i++) {
            if (intArray[i].intValue() == pageId.intValue()) {
                myCount++;
                if (myCount == displayValueInt) {
                    savedIndex = i;
                    break;
                }
            }
        }

        if (savedIndex == 0) {
            targetView.removePageSessionAttribute(
                Constants.SessionAttributes.PAGE_PATH);
            return;
        }
        // otherwise copy the chunk of the array to the page session
        Integer [] newIntArray = new Integer [savedIndex];
        for (int i = 0; i < savedIndex; i++) {
            newIntArray[i] = intArray[i];
        }
        targetView.setPageSessionAttribute(
            Constants.SessionAttributes.PAGE_PATH,
            newIntArray);
    }


    /**
     * Set the BreadCrumb component while going backward (removing links)
     * @param sourceView - View Bean where the page is forwarding from
     * @param pageID - See PageInfo class
     * @displayValue -
     * @return none
     */
    public static void breadCrumbPathBackward(
        ViewBean sourceView,
        Integer pageId,
        String displayValue) {

        // need to check if the breadcrumb that was clicked on
        // is the same as another one in the trail
        int displayValueInt = Integer.parseInt(displayValue);
        Integer [] intArray = (Integer []) sourceView.getPageSessionAttribute(
            Constants.SessionAttributes.PAGE_PATH);
        int start = 0;

        // only look at last Constants.BreadCrumb.MAX_LINKS in the array
        if (intArray.length > Constants.BreadCrumb.MAX_LINKS) {
            start = intArray.length - Constants.BreadCrumb.MAX_LINKS;
        }

        // detect which breadcrumb the user clicked on
        // there can be two breadcrumbs in the path with the same name
        int savedIndex = -1;
        int myCount = -1;
        for (int i = start; i < intArray.length; i++) {
            if (intArray[i].intValue() == pageId.intValue()) {
                myCount++;
                if (myCount == displayValueInt) {
                    savedIndex = i;
                    break;
                }
            }
        }

        if (savedIndex == 0) {
            sourceView.removePageSessionAttribute(
                Constants.SessionAttributes.PAGE_PATH);
            return;
        }
        // otherwise copy the chunk of the array to the page session
        Integer [] newIntArray = new Integer [savedIndex];
        for (int i = 0; i < savedIndex; i++) {
            newIntArray[i] = intArray[i];
        }
        sourceView.setPageSessionAttribute(
            Constants.SessionAttributes.PAGE_PATH,
            newIntArray);
    }

    /**
     * Retrieves an array back displaying only the maximum number of
     * breadcrumb links to be displayed (Constants.BreadCrumb.MAX_LINKS)
     */
    public static Integer[] getBreadCrumbDisplay(Integer [] intArray) {
        if (intArray.length <= Constants.BreadCrumb.MAX_LINKS) {
            return intArray;
        }

        // make a new array only having a maximum of
	// Constants.BreadCrumb.MAX_LINKS in the array
        Integer newIntArray [] = new Integer[Constants.BreadCrumb.MAX_LINKS];

        int start = intArray.length - Constants.BreadCrumb.MAX_LINKS;
        int j = 0;
        for (int i = start; i < intArray.length; i++) {
            newIntArray[j++] = intArray[i];
        }
        return newIntArray;
    }

    /**
     * Creates the breadcrumbs.
     */
    public static void createBreadCrumbs(
        ViewBean view,
        String breadCrumbName,
        CCBreadCrumbsModel model) {

        Integer [] previousPages = (Integer []) view.getPageSessionAttribute(
            Constants.SessionAttributes.PAGE_PATH);

        if (previousPages == null) {
            TraceUtil.trace3("previousPages is null");
            return;
        }

        // get all of the breadcrumb information for the links
        PageInfo pageInfo = PageInfo.getPageInfo();

        // Only look at the last Constants.BreadCrumb.MAX_LINKS in the array
        Integer [] previousPagesDisplay =
            BreadCrumbUtil.getBreadCrumbDisplay(previousPages);

        if (TraceUtil.isOnLevel3()) {
            for (int k = 0; k < previousPagesDisplay.length; k++) {
                TraceUtil.trace3(new StringBuffer().append(
                    "previousPagesDisplay[").append(k).append("] is ").append(
                    previousPagesDisplay[k].intValue()).toString());
            }
        }

        // add the breadcrumbs to the model
        // each breadcrumb has a unique Integer identifier
        // which is used to retrieve the breadcrumb information
        // for the page
        for (int i = 0; i < previousPagesDisplay.length; i++) {
            int j = previousPagesDisplay[i].intValue();
            if (i != 0) {
                model.appendRow();
            }
            model.setValue(
                CCBreadCrumbsModel.COMMANDFIELD,
                pageInfo.getPagePath(j).getCommandField());
            model.setValue(CCBreadCrumbsModel.LABEL,
                pageInfo.getPagePath(j).getLabel());
                model.setValue(CCBreadCrumbsModel.MOUSEOVER,
                    pageInfo.getPagePath(j).getMouseOver());

            // need to check if this value is in the array
            // if it is not, give a value of 0, otherwise
            // see how many times it occurs and give the link
            model.setValue(CCBreadCrumbsModel.HREF_VALUE,
                Integer.toString(BreadCrumbUtil.inPagePath(
                    previousPagesDisplay, j, i)));
        }
    }

    /**
     * Tells how many times a link occurs in the breadcrumb.
     */
    public static int inPagePath(
        Integer [] previousPagesDisplay,
        int i,
        int index) {

        int counter = 0;
        for (int j = 0; j < index; j++) {
            if (previousPagesDisplay[j].intValue() == i) {
                counter++;
            }
        }
        return counter;
    }
}
