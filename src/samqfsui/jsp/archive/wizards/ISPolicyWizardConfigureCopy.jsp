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

// ident	$Id: ISPolicyWizardConfigureCopy.jsp,v 1.6 2008/12/16 00:10:43 am143972 Exp $
--%>

<%@ page language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%> 
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<script language="javascript"
    src="/samqfsui/js/archive/wizards/ISPolicyWizardConfigureCopy.js">
</script>

<script langugage="javascript">
    WizardWindow_Wizard.pageInit = wizardPageInit;
</script>

<jato:pagelet>

<cc:i18nbundle
    id="samBundle"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources" />

<cc:alertinline name="Alert" bundleID="samBundle" />

<cc:legend name="Legend" align="right" marginTop="10px" />

<table>
<tr>
    <td rowspan="9">
        <cc:spacer name="Spacer" width="10" height="1" />
    </td>
    <td>
        <cc:label
             name="CopyTimeText"
             showRequired="true"
             defaultValue="archiving.dataclass.wizard.copytime"
             bundleID="samBundle" />
    </td>
    <td>
        <cc:textfield
            name="CopyTimeTextField"
            elementId="CopyTimeTextField"
            size="5"
            maxLength="4"
            bundleID="samBundle" />
        <cc:dropdownmenu
            name="CopyTimeDropDown"
            bundleID="samBundle" />
    </td>
</tr>
<tr>
    <td>
        <cc:label
             name="ExpirationTimeText"
             defaultValue="archiving.dataclass.wizard.expirationtime"
             bundleID="samBundle" />
    </td>
    <td>
        <cc:textfield
            name="ExpirationTimeTextField"
            size="5"
            maxLength="4"
            dynamic="true"
            bundleID="samBundle" />
        <cc:dropdownmenu
            name="ExpirationTimeDropDown"
            dynamic="true"
            bundleID="samBundle" />
    </td>
</tr>
<tr>
    <td>&nbsp;</td>
    <td>
        <cc:checkbox
            name="NeverExpireCheckBox"
            label="archiving.dataclass.wizard.neverexpire"
            onClick="handleExpireCheckBoxClick(this);"
            bundleID="samBundle" />
    </td>
</tr>
<tr><td colspan="2"><cc:spacer name="Spacer" width="1" height="20" /></td></tr>
<tr>
    <td>
        <cc:label
            name="MediaPoolText"
            showRequired="true"
            defaultValue="archiving.dataclass.wizard.mediapool"
            bundleID="samBundle" />
    </td>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:dropdownmenu
            name="MediaPoolDropDown"
            onChange="populateScratchPoolDropDown()"
            bundleID="samBundle" />
    </td>
</tr>
<tr>
    <td>
        <cc:label
            name="ScratchPoolText"
            defaultValue="archiving.dataclass.wizard.scratchpool"
            bundleID="samBundle" />
    </td>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:dropdownmenu
            name="ScratchPoolDropDown"
            dynamic="true"
            bundleID="samBundle" />
    </td>
</tr>
<tr><td colspan="2"><cc:spacer name="Spacer" width="1" height="20" /></td></tr>

<!-- Remove in CIS, pending decision on Point Product
<tr>
    <td>
        <cc:label
            name="CopyMigrationToText"
            defaultValue="archiving.dataclass.migratefrom"
            bundleID="samBundle" />
    </td>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:dropdownmenu
            name="CopyMigrateToDropDown"
            bundleID="samBundle" />
    </td>
</tr>
 -->
<tr>
    <td>
        <cc:label
            name="EnableRecyclingText"
            defaultValue="archiving.dataclass.wizard.enablerecycling"
            bundleID="samBundle" />
    </td>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:checkbox
            name="EnableRecyclingCheckBox"
            label=""
            bundleID="samBundle" />
    </td>
</tr>

<cc:hidden name="PoolInfo" />
<cc:hidden name="SelectedScratchPool" />


</jato:pagelet>
