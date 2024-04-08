<%--
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
 * or https://illumos.org/license/CDDL.
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

// ident	$Id: AddLibraryDirectSummary.jsp,v 1.12 2008/12/16 00:10:49 am143972 Exp $

--%>

<%@ page language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%> 
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:pagelet>

<cc:i18nbundle id="samBundle"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources" />

<table border="0" cellspacing="10" cellpadding="0">
<tbody>
    <tr>
        <td colspan="2">
            <cc:alertinline name="Alert" bundleID="samBundle" /><br />
        </td>
    </tr>
    <tr>
        <td>
            <cc:label name="Label" styleLevel="2"
                elementName="attachedValue"
                defaultValue="AddLibrary.label.attachedtype"
                bundleID="samBundle" />
        </td>
        <td>
            <cc:text name="attachedValue"
                bundleID="samBundle" />
        </td>
    </tr>
    <tr>
        <td>
            <cc:label name="Label" styleLevel="2"
                elementName="vendorIDValue"
                defaultValue="AddLibrary.label.vendorID"
                bundleID="samBundle" />
        </td>
        <td>
            <cc:text name="vendorIDValue"
                bundleID="samBundle" />
        </td>
    </tr>
    <tr>
        <td>
            <cc:label name="Label" styleLevel="2"
                elementName="productIDValue"
                defaultValue="AddLibrary.label.productID"
                bundleID="samBundle" />
        </td>
        <td>
            <cc:text name="productIDValue"
                bundleID="samBundle" />
        </td>
    </tr>
    <tr>
        <td>
            <cc:label name="Label" styleLevel="2"
                elementName="nameValue"
                defaultValue="AddLibrary.label.name"
                bundleID="samBundle" />
        </td>
        <td>
            <cc:text name="nameValue"
                bundleID="samBundle" />
        </td>
    </tr>
    <tr>
        <td>
            <cc:label name="Label" styleLevel="2"
                elementName="mediaTypeValue"
                defaultValue="AddLibrary.label.mediatype"
                bundleID="samBundle" />
        </td>
        <td>
            <cc:text name="mediaTypeValue"
                bundleID="samBundle" />
        </td>
    </tr>
</tbody>
</table>

</jato:pagelet>
