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

// ident	$Id: SpaceDelimiterConverter.java,v 1.1 2008/06/11 16:58:01 ronaldso Exp $

package com.sun.netstorage.samqfs.web.util;

import javax.faces.component.UIComponent;
import javax.faces.context.FacesContext;
import javax.faces.convert.Converter;

public class SpaceDelimiterConverter implements Converter {

    /**
     * Thus far implementation of this method is not required.
     *
     * @param context - the current faces context.
     * @param component - the component whose value is to be converted.
     * @param value - the string representation of the current job.
     * @return Integer - the symbolic constant for the current Job type
     */
    public Object getAsObject(FacesContext context,
            UIComponent component,
            String value) {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    public String getAsString(FacesContext context,
            UIComponent component,
            Object value) {

        String [] ipAddresses = ((String) value).split(" ");
        if (ipAddresses.length == 0) {
            return "";
        }

        StringBuffer buf = new StringBuffer();
        for (int i = 0; i < ipAddresses.length; i++){
            if (buf.length() > 0) {
                buf.append("<br />");
            }
            buf.append(ipAddresses[i]);
        }
        return buf.toString();
    }
}
