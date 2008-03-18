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

// ident	$Id: PropertySheetUtil.java,v 1.9 2008/03/17 14:43:57 am143972 Exp $

package com.sun.netstorage.samqfs.web.util;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.view.ContainerView;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ContainerViewBase;

import com.sun.web.ui.model.CCPropertySheetModel;
import com.sun.web.ui.view.propertysheet.CCPropertySheet;

/*
 * This is the Helper Class to create the PropertySheet component.
 */

public class PropertySheetUtil {

    public static final String CHILD_PROPERTY_SHEET    = "PropertySheet";

   /** Creates a new instance of PropertySheetUtil */
    public PropertySheetUtil() {
    }

    public static CCPropertySheetModel createModel(String xmlFile) {
        // Construct a propertySheet model using XML stringa
        CCPropertySheetModel propertySheetModel = new CCPropertySheetModel(
            RequestManager.getRequestContext().getServletContext(),
            xmlFile);
        return propertySheetModel;
    }

    public static CCPropertySheetModel createModelFromString(
        String xmlString) {
        // Construct a propertySheet model using XML stringa
        CCPropertySheetModel propertySheetModel =
            new CCPropertySheetModel(xmlString);
        return propertySheetModel;
    }

    public static boolean isChildSupported(
        CCPropertySheetModel model,
        String name) {

        if (name.equals(CHILD_PROPERTY_SHEET)) {
            return true;
        } else if (model != null && model.isChildSupported(name)) {
            return true;
        } else {
            return false;
        }
    }

    public static void registerChildren(
        ContainerViewBase containerView,
        CCPropertySheetModel model) {
        containerView.registerChild(
            CHILD_PROPERTY_SHEET, CCPropertySheet.class);
        model.registerChildren(containerView);
    }

    public static View createChild(
        ContainerView view,
        CCPropertySheetModel model,
        String name) {

        // Propertysheet child
        if (name.equals(CHILD_PROPERTY_SHEET)) {
            return new CCPropertySheet(view, model, name);
        // Create child from propertysheet model.
        } else if (model.isChildSupported(name)) {
            return model.createChild(view, name);
        } else {
            throw new IllegalArgumentException(
                "Invalid child name [" + name + "]");
        }
    }
}
