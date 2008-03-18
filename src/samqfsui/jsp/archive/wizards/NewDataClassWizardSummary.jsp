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

// ident	$Id: NewDataClassWizardSummary.jsp,v 1.8 2008/03/17 14:40:32 am143972 Exp $
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
            name="Label"
            defaultValue="archiving.dataclass.name"
            bundleID="samBundle" />
    </td>
    <td valign="bottom" align="left" rowspan="1" colspan="1">
        <cc:text
            name="DataClassName" 
            bundleID="samBundle" />
    </td>
</tr>

<tr>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:label
            name="Label"
            defaultValue="archiving.dataclass.desc"
            bundleID="samBundle" />
    </td>
    <td valign="bottom" align="left" rowspan="1" colspan="1">
        <cc:text
            name="Description" 
            bundleID="samBundle" />
    </td>
</tr>

<tr>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:label
            name="Label"
            defaultValue="archiving.policy.name.colon"
            bundleID="samBundle" />
    </td>
    <td valign="bottom" align="left" rowspan="1" colspan="1">
        <cc:text
            name="PolicyName" 
            bundleID="samBundle" />
    </td>
</tr>

<tr>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:label
            name="Label"
            defaultValue="archiving.policy.description.colon"
            bundleID="samBundle" />
    </td>
    <td valign="bottom" align="left" rowspan="1" colspan="1">
        <cc:text
            name="PolicyDescription"
            bundleID="samBundle" />
    </td>
</tr>

<tr>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:label
            name="Label"
            defaultValue="archiving.startingdir.colon"
            bundleID="samBundle" />
    </td>
    <td valign="bottom" align="left" rowspan="1" colspan="1">
        <cc:text
            name="StartingDir" 
            bundleID="samBundle" />
    </td>
</tr>

<tr>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:label
            name="Label"
            defaultValue="archiving.numOfCopies.colon"
            bundleID="samBundle" />
    </td>
    <td valign="bottom" align="left" rowspan="1" colspan="1">
        <cc:text
            name="NumCopies" 
            bundleID="samBundle" />
    </td>
</tr>

<tr>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:label
            name="Label"
            defaultValue="archiving.minimumsize.colon"
            bundleID="samBundle" />
    </td>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:text
            name="MinSize"
            bundleID="samBundle" />
    </td>
</tr>

<tr>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:label
            name="Label"
            defaultValue="archiving.maximumsize.colon"
            bundleID="samBundle" />
    </td>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:text
            name="MaxSize"
            bundleID="samBundle" />
    </td>
</tr>

<tr>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:label
            name="Label"
            defaultValue="archiving.namepattern.colon"
            bundleID="samBundle" />
    </td>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:text
            name="NamePattern"
            bundleID="samBundle" />
    </td>
</tr>

<tr>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:label
            name="Label"
            defaultValue="archiving.owner.colon"
            bundleID="samBundle" />
    </td>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:text
            name="Owner"
            bundleID="samBundle" />
    </td>
</tr>

<tr>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:label
            name="Label"
            defaultValue="archiving.group.colon"
            bundleID="samBundle" />
    </td>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:text
            name="Group"
            bundleID="samBundle" />
    </td>
</tr>

<tr>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:label
            name="Label"
            defaultValue="archiving.accessage.colon"
            bundleID="samBundle" />
    </td>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:text
            name="AccessAge"
            bundleID="samBundle" />
    </td>
</tr>

<tr>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:label
            name="Label"
            defaultValue="archiving.dataclass.wizard.copyinfo"
            bundleID="samBundle" />
    </td>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:text
            name="CopyInfo"
            escape="false"
            bundleID="samBundle" />
    </td>
</tr>

</jato:pagelet>
