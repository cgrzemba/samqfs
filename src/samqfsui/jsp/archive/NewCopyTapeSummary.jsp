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

// ident	$Id: NewCopyTapeSummary.jsp,v 1.5 2008/03/17 14:40:31 am143972 Exp $
--%>
<%@ page language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%> 
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:pagelet>

<cc:i18nbundle
    id="samBundle"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources" />
    
<tr>
    <td>
        <cc:label
            name="ArchiveAgeLabel"
            bundleID="samBundle"
            defaultValue="NewArchivePolWizard.page3.archiveAge"/>
    </td>
    <td>
        <cc:text name="ArchiveAge" bundleID="samBundle"/>
    </td>
</tr>
<tr>
    <td>
        <cc:label
            name="ArchiveTypeLabel"
            bundleID="samBundle"
            defaultValue="NewArchivePolWizard.page3.archiveMedType"/>
    </td>
    <td>
        <cc:text name="ArchiveType" bundleID="samBundle"/>
    </td>
</tr>
<tr>
    <td>
        <cc:spacer
            name="Spacer"
            height="1"
            width="10" />
        <cc:label
            name="MediaTypeLabel"
            bundleID="samBundle"
            defaultValue="NewArchivePolWizard.page3.mediaType"/>
    </td>
    <td>
        <cc:text name="MediaType" bundleID="samBundle"/>
    </td>
</tr>
<tr>
    <td>
        <cc:spacer
            name="Spacer"
            height="1"
            width="10" />
        <cc:label
            name="VSNPoolNameLabel"
            bundleID="samBundle"
            defaultValue="NewArchivePolWizard.page3.vsnPoolName"/>
    </td>
    <td>
        <cc:text name="VSNPoolName" bundleID="samBundle"/>
    </td>
</tr>
<tr>
    <td>
        <cc:spacer
            name="Spacer"
            height="1"
            width="10" />
        <cc:label
            name="SpecifyVSNLabel"
            bundleID="samBundle"
            defaultValue="NewArchivePolWizard.page3.specifyVSN"/>
    </td>
    <td>
        <cc:text name="SpecifyVSN" bundleID="samBundle"/>
    </td>
</tr>
<tr>
    <td>
        <cc:label
            name="ReserveLabel"
            bundleID="samBundle"
            defaultValue="NewArchivePolWizard.page4.reserve"/>
    </td>
    <td>
        <cc:text name="Reserve" bundleID="samBundle"/>
    </td>
</tr>
<tr>
    <td>
        <cc:label
            name="OfflineCopyLabel"
            bundleID="samBundle"
            defaultValue="NewArchivePolWizard.page4.offlineCopy"/>
    </td>
    <td>
        <cc:text name="OfflineCopy" bundleID="samBundle"/>
    </td>
</tr>
<tr>
    <td>
        <cc:label
            name="MaxDriveLabel"
            defaultValue="NewArchivePolWizard.page4.driveMax"
            bundleID="samBundle" />
    </td>
    <td>
        <cc:text
            name="MaxDrive"
            bundleID="samBundle" />
    </td>
</tr>
<tr>
    <td>
        <cc:label
            name="MinDriveLabel"
            defaultValue="NewArchivePolWizard.page4.driveMin"
            bundleID="samBundle" />
    </td>
    <td>
        <cc:text
            name="MinDrive"
            bundleID="samBundle" />
    </td>
</tr>
<tr>
    <td>
        <cc:label
            name="SaveMethodLabel" 
            bundleID="samBundle"
            defaultValue="archiving.copy.wizard.save.how"/>
    </td>
    <td>
        <cc:text name="SaveMethod" bundleID="saveBundle"/>
    </td>
</tr>

</jato:pagelet>
