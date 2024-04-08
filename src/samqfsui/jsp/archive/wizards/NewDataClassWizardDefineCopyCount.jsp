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

// ident	$Id: NewDataClassWizardDefineCopyCount.jsp,v 1.8 2008/12/16 00:10:43 am143972 Exp $
--%>

<%@ page language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%> 
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:pagelet>

<cc:i18nbundle
    id="samBundle"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources" />

<cc:alertinline name="Alert" bundleID="samBundle" />

<cc:legend name="Legend" align="right" marginTop="10px" />

<table>
<tr>
    <td rowspan="7">
        <cc:spacer name="Spacer" width="10" height="1" />
    </td>
    <td>
        <cc:label
             name="PolicyNameText"
             showRequired="true"
             defaultValue="archiving.dataclass.selectpolicyname"
             bundleID="samBundle" />
    </td>
    <td>
        <cc:textfield
            name="PolicyNameTextField"
            elementId="policyNameTextField"
            maxLength="28"
            bundleID="samBundle" />
    </td>
</tr>
<tr>
    <td>
        <cc:label
             name="PolicyDescriptionText"
             defaultValue="archiving.dataclass.selectpolicyname.desc"
             bundleID="samBundle" />
    </td>
    <td>
        <cc:textfield
            name="PolicyDescription"
            elementId="policyDescription"
            size="60"
            maxLength="256"
            bundleID="samBundle" />
    </td>
</tr>
<tr>
    <td>
        <cc:label
             name="CopiesText"
             defaultValue="archiving.dataclass.noofcopies"
             bundleID="samBundle" />
    </td>
    <td>
        <cc:dropdownmenu
            name="CopiesDropDown"
            bundleID="samBundle" />
    </td>
</tr>

<tr><td colspan="2"><cc:spacer name="Spacer" width="10" height="10" /></td></tr>

<tr>
    <td colspan="2">
        <cc:text
             name="Text"
             defaultValue="archiving.dataclass.configpolicytext"
             bundleID="samBundle" />
    </td>
</tr>

<tr>
    <td>
        <cc:label
            name="MigrateFromText"
            defaultValue="archiving.dataclass.migratefrom"
            bundleID="samBundle" />
    </td>
    <td>
        <cc:dropdownmenu
            name="MigrateFromDropDown"
            bundleID="samBundle" />
    </td>
</tr>

<tr>
    <td>
        <cc:label
            name="MigrateToText"
            defaultValue="archiving.dataclass.migrateto"
            bundleID="samBundle" />
    </td>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:dropdownmenu
            name="MigrateToDropDown"
            bundleID="samBundle" />
    </td>
</tr>
</jato:pagelet>
