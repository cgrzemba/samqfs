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

// ident	$Id: NewCopySummary.jsp,v 1.3 2008/12/16 00:10:43 am143972 Exp $
--%>
<%@ page language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:pagelet>

<cc:i18nbundle
    id="samBundle"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources" />

<br />

<table>
<tr>
    <td rowspan="4">
        <cc:spacer name="Spacer" width="10"/>
    </td>
    <td>
        <cc:label
            name="LabelArchiveAge"
            bundleID="samBundle"
            defaultValue="archiving.archiveage.label"/>
    </td>
    <td>
        <cc:text name="TextArchiveAge" bundleID="samBundle"/>
    </td>
</tr>
<tr>
    <td>
        <cc:label
            name="LabelMediaType"
            bundleID="samBundle"
            defaultValue="archiving.mediatype.label"/>
    </td>
    <td>
        <cc:text name="TextMediaType" bundleID="samBundle"/>
    </td>
</tr>
<tr>
    <td>
        <cc:label
            name="LabelIncludeVolumes"
            bundleID="samBundle"
            defaultValue="archiving.copy.includevol"/>
    </td>
    <td>
        <cc:text name="TextIncludeVolumes" bundleID="samBundle"/>
    </td>
</tr>
<tr>
    <td>
        <cc:label
            name="LabelOther"
            bundleID="samBundle"
            defaultValue="archiving.copy.otherinfo"/>
    </td>
    <td>
        <cc:text name="TextOther" bundleID="samBundle"/>
    </td>
</tr>
</table>

</jato:pagelet>
