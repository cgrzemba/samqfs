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

// ident	$Id: AddLibraryACSLSSummary.jsp,v 1.6 2008/03/17 14:40:37 am143972 Exp $

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
                elementName="LibraryCount"
                defaultValue="AddLibrary.label.numberofvirtuallibs"
                bundleID="samBundle" />
        </td>
        <td>
            <cc:text name="LibraryCount"
                bundleID="samBundle" />
        </td>
    </tr>
    <tr>
        <td>
            <cc:label name="Label" styleLevel="2"
                elementName="ACSLSHostName"
                defaultValue="AddLibrary.selecttype.label.acslshost"
                bundleID="samBundle" />
        </td>
        <td>
            <cc:text name="ACSLSHostName"
                bundleID="samBundle" />
        </td>
    </tr>
    <tr>
        <td>
            <cc:label name="Label" styleLevel="2"
                elementName="ACSLSPortNumber"
                defaultValue="AddLibrary.selecttype.label.acslsport"
                bundleID="samBundle" />
        </td>
        <td>
            <cc:text name="ACSLSPortNumber"
                bundleID="samBundle" />
        </td>
    </tr>
    <tr>
        <td>
            <cc:label name="Label" styleLevel="2"
                elementName="LibrarySerialNumber"
                defaultValue="AddLibrary.label.serialnumber"
                bundleID="samBundle" />
        </td>
        <td>
            <cc:text name="LibrarySerialNumber"
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
    <tr>
        <td>
            <cc:label name="Label" styleLevel="2"
                elementName="AccessIDValue"
                defaultValue="AddLibrary.label.accessid"
                bundleID="samBundle" />
        </td>
        <td>
            <cc:text name="AccessIDValue"
                bundleID="samBundle" />
        </td>
    </tr>
    <tr>
        <td>
            <cc:label name="Label" styleLevel="2"
                elementName="SSIHostValue"
                defaultValue="AddLibrary.label.ssihost"
                bundleID="samBundle" />
        </td>
        <td>
            <cc:text name="SSIHostValue"
                bundleID="samBundle" />
        </td>
    </tr>
    <tr>
        <td>
            <cc:label name="Label" styleLevel="2"
                elementName="UseSecureRPCValue"
                defaultValue="AddLibrary.label.usesecurerpc.colon"
                bundleID="samBundle" />
        </td>
        <td>
            <cc:text name="UseSecureRPCValue"
                bundleID="samBundle" />
        </td>
    </tr>
    <tr>
        <td>
            <cc:label name="Label" styleLevel="2"
                elementName="SSIInetPortValue"
                defaultValue="AddLibrary.label.ssiinetport"
                bundleID="samBundle" />
        </td>
        <td>
            <cc:text name="SSIInetPortValue"
                bundleID="samBundle" />
        </td>
    </tr>
    <tr>
        <td>
            <cc:label name="Label" styleLevel="2"
                elementName="CSIHostPortValue"
                defaultValue="AddLibrary.label.csihostport"
                bundleID="samBundle" />
        </td>
        <td>
            <cc:text name="CSIHostPortValue"
                bundleID="samBundle" />
        </td>
    </tr>
</tbody>
</table>
</jato:pagelet>
