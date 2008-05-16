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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

// ident	$Id: NewPolicyWizardSummary.jsp,v 1.9 2008/05/16 19:39:18 am143972 Exp $
--%>
<%@ page language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%> 
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:pagelet>

<cc:i18nbundle
    id="samBundle"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources" />

<tr>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:label
            name="StartingDir"
            defaultValue="NewArchivePolWizard.page1.startDirText"
            bundleID="samBundle" />
    </td>
    <td valign="bottom" align="left" rowspan="1" colspan="1">
        <cc:text
            name="StartingDirTextField" 
            bundleID="samBundle" />
    </td>
</tr>

<tr>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:label
            name="PolicyName"
            defaultValue="NewArchivePolWizard.summary.name"
            bundleID="samBundle" />
    </td>
    <td valign="bottom" align="left" rowspan="1" colspan="1">
        <cc:text
            name="PolicyNameTextField" 
            bundleID="samBundle" />
    </td>
</tr>

<tr>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:label
            name="NumCopies"
            defaultValue="NewArchivePolWizard.summary.numCopies"
            bundleID="samBundle" />
    </td>
    <td valign="bottom" align="left" rowspan="1" colspan="1">
        <cc:text
            name="NumCopiesTextField" 
            bundleID="samBundle" />
    </td>
</tr>

<tr>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:label
            name="MinSizeText"
            defaultValue="NewArchivePolWizard.page2.minSize"
            bundleID="samBundle" />
    </td>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:text
            name="MinimumSizeTextField"
            bundleID="samBundle" />
    </td>
</tr>

<tr>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:label
            name="MaxSizeText"
            defaultValue="NewArchivePolWizard.page2.maxSize"
            bundleID="samBundle" />
    </td>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:text
            name="MaximumSizeTextField"
            bundleID="samBundle" />
    </td>
</tr>

<tr>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:label
            name="NamePatternText"
            defaultValue="NewArchivePolWizard.page2.namePattern"
            bundleID="samBundle" />
    </td>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:text
            name="NamePatternTextField"
            bundleID="samBundle" />
    </td>
</tr>

<tr>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:label
            name="OwnerText"
            defaultValue="NewArchivePolWizard.page2.owner"
            bundleID="samBundle" />
    </td>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:text
            name="OwnerTextField"
            bundleID="samBundle" />
    </td>
</tr>

<tr>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:label
            name="GroupText"
            defaultValue="NewArchivePolWizard.page2.group"
            bundleID="samBundle" />
    </td>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:text
            name="GroupTextField"
            bundleID="samBundle" />
    </td>
</tr>

<tr>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:label
            name="AccessAgeText"
            defaultValue="NewPolicyWizard.defineType.accessAge"
            bundleID="samBundle" />
    </td>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:text
            name="AccessAgeTextFieldWithUnits"
            bundleID="samBundle" />
    </td>
</tr>


<tr>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:label
            name="StageAttText"
            defaultValue="NewArchivePolWizard.page2.stageAttributes"
            bundleID="samBundle" />
    </td>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:text
            name="StageAttTextField"
            bundleID="samBundle" />
    </td>
</tr>

<tr>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:label
            name="ReleaseAttText"
            defaultValue="NewArchivePolWizard.page2.releaseAttributes"
            bundleID="samBundle" />
    </td>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:text
            name="ReleaseAttTextField"
            bundleID="samBundle" />
    </td>
</tr>

<tr>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:label
            name="FsText"
            defaultValue="NewArchivePolWizard.summary.fs"
            bundleID="samBundle" />
    </td>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:selectablelist name="FSDataField"
            bundleID="samBundle" escape="false" />
    </td>
</tr>

</jato:pagelet>
