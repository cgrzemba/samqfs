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

// ident	$Id: AddLibraryACSLSParam.jsp,v 1.8 2008/12/16 00:10:49 am143972 Exp $

--%>
<%@ page language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%> 
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<script type="text/javascript"
    src="/samqfsui/js/media/wizards/AddLibraryACSLSParam.js">
</script>
    
<jato:pagelet>

<cc:i18nbundle id="samBundle"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources" />

<cc:alertinline name="Alert" bundleID="samBundle" />

<cc:legend name="Legend" align="right" marginTop="10px" />

<table border="0">
<tr>
    <td rowspan="2"><cc:spacer name="Spacer" width="10" /></td>
    <td>
        <cc:label
            name="Label"
            defaultValue="AddLibrary.label.acslsserver"
            bundleID="samBundle" />
    </td>
    <td><cc:spacer name="Spacer" width="10" /></td>
    <td>
        <cc:text name="STKHostName" />
    </td>
    <td><cc:spacer name="Spacer" width="20" /></td>
    <td>
        <cc:label
            name="Label"
            defaultValue="AddLibrary.label.portnumber"
            bundleID="samBundle" />
    </td>
    <td><cc:spacer name="Spacer" width="10" /></td>
    <td>
        <cc:text
            name="ACSLSPortNumber"
            defaultValue="" />
    </td>
</tr>
<tr>
    <td colspan="2">
        <cc:label name="Label"
            defaultValue="AddLibrary.label.savesettingto"
            bundleID="samBundle"/>
    </td>
    <td colspan="5">
        <cc:text name ="SaveTo"/>
    </td>
</tr>
</table>
        
<br />

<table border="0">
<tr>
    <td rowspan="10">
        <cc:spacer
            name="Spacer"
            width="10" />
    </td>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:label
            name="Label"
            defaultValue="AddLibrary.label.libraryname"
            showRequired="true"
            bundleID="samBundle" />
    </td>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:textfield
            name="LibraryNameValue"
            size="40" 
            maxLength="127"
            bundleID="samBundle"/>
    </td>   
</tr>

<tr>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:label
            name="Label"
            defaultValue="AddLibrary.label.accessid"
            bundleID="samBundle"/>
    </td>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:textfield name ="AccessIDValue" bundleID="samBundle"/>
    </td>
</tr>

<tr>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:label
            name="Label"
            defaultValue="AddLibrary.label.ssihost"
            bundleID="samBundle"/>
    </td>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:textfield
            name ="SSIHostValue"
            bundleID="samBundle"/>
    </td>
</tr>

<tr>
    <td valign="center" align="left" rowspan="1" colspan="2">
        <cc:checkbox
            name="UseSecureRPC"
            label="AddLibrary.label.usesecurerpc"
            title="AddLibrary.tooltip.usesecurerpc"
            onClick="changeComponentState(this)"
            bundleID="samBundle"/>
        <cc:spacer name="Spacer" height="20" />
    </td>
</tr>

<tr>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:spacer name="Spacer" width="20" />
        <cc:label
            name="Label"
            defaultValue="AddLibrary.label.ssiinetport"
            bundleID="samBundle"/>
    </td>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:textfield
            name ="SSIInetPortValue"
            disabled="true"
            dynamic="true"
            bundleID="samBundle"/>
    </td>
</tr>

<tr>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:spacer name="Spacer" width="20" />
        <cc:label
            name="Label"
            defaultValue="AddLibrary.label.csihostport"
            bundleID="samBundle"/>
    </td>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:textfield
            name ="CSIHostPortValue"
            disabled="true"
            dynamic="true"
            bundleID="samBundle"/>
    </td>
</tr>

</table>
</span>
</jato:pagelet>
