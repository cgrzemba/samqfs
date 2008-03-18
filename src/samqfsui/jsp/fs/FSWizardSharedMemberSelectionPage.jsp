
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

// ident	$Id: FSWizardSharedMemberSelectionPage.jsp,v 1.15 2008/03/17 14:40:33 am143972 Exp $
--%>
<%@ page language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%> 
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<script language="javascript"
    src="/samqfsui/js/fs/wizards/FSWizardSharedMemberSelectionPage.js">
</script>

<jato:pagelet>

<cc:i18nbundle
    id="samBundle"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources" />

<cc:alertinline name="Alert" bundleID="samBundle" />

<br />

<table>
<tr>
    <td rowspan="5">
        <cc:spacer name="Spacer" width="10" />
    </td>
    <td>
        <cc:label
            name="metaServerLabel"
            defaultValue="FSWizard.sharedMemberSelectionPage.metaServer"
            bundleID="samBundle" />
    </td>
    <td rowspan="5">
        <cc:spacer name="Spacer" width="10" />
    </td>
    <td>
        <cc:text name="metaServer" bundleID="samBundle" />
    </td>
</tr>
<tr>
    <td>
        <cc:label
            name="metaServerVerLabel"
            defaultValue="FSWizard.sharedMemberSelectionPage.metaServerVer"
           bundleID="samBundle" />
    </td>
    <td>
        <cc:text name="metaServerVer" bundleID="samBundle" />
    </td>
</tr>
<tr>
    <td>
        <cc:label
            name="metaServerArchLabel"
            defaultValue="FSWizard.sharedMemberSelectionPage.metaServerArch"
            bundleID="samBundle" />
    </td>
    <td>
        <cc:text name="metaServerArch" bundleID="samBundle" />
    </td>
</tr>
<tr>
    <td>
        <cc:label
            name="ipLabel"
            defaultValue="FSWizard.sharedMemberSelectionPage.ipLabel"
            bundleID="samBundle" />
    </td>
    <td>
        <cc:dropdownmenu
            name="serverIP"
            bundleID="samBundle"
            onChange="populateSecondaryIPDropDown();"
            type="standard"/>
    </td>
</tr>
<tr>
    <td>
        <cc:label
            name="secIpLabel"
            defaultValue="FSWizard.sharedMemberSelectionPage.secIpLabel"
            bundleID="samBundle" />
    </td>
    <td>
        <cc:dropdownmenu
            name="secServerIP"
            bundleID="samBundle"
            dynamic="true"
            type="standard"/>
    </td>
</tr>
</table>

<br />
<br />

<cc:actiontable
    name="SharedMemberSelectionTable"
    bundleID="samBundle"
    title="FSWizard.sharedMemberSelectionTable.title"
    selectionType="none"
    selectionJavascript="handleTableSelection(this)"
    showAdvancedSortIcon="false"
    showLowerActions="false"
    showPaginationControls="false"
    showPaginationIcon="false"
    showSelectionIcons="true"/>

<cc:hidden name="HiddenMessage" />
<cc:hidden name="errorOccur" />
<cc:hidden name="ipAddresses"/>
<cc:hidden name="selectedSecondaryIP"/>

</jato:pagelet>
