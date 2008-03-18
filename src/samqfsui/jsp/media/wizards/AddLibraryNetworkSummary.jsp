<%--
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

// ident	$Id: AddLibraryNetworkSummary.jsp,v 1.9 2008/03/17 14:40:37 am143972 Exp $

--%>

<%@ page language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%> 
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:pagelet>

<cc:i18nbundle id="samBundle"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources" />

<cc:alertinline name="Alert" bundleID="samBundle" /><br />

<table border="0" cellspacing="10" cellpadding="0">
<tbody>
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
                elementName="ManagementTool"
                defaultValue="AddLibrary.label.driver"
                bundleID="samBundle" />
        </td>
        <td>
            <cc:text name="ManagementTool"
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
                elementName="paramValue"
                defaultValue="AddLibrary.label.paramfile"
                bundleID="samBundle" />
        </td>
        <td>
            <cc:text name="paramValue"
                bundleID="samBundle" />
        </td>
    </tr>
</tbody>
</table>
</jato:pagelet>
