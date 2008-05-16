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

// ident	$Id: NewCriteriaSummary.jsp,v 1.6 2008/05/16 19:39:18 am143972 Exp $
--%>

<%@ page language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%> 
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:pagelet>

<cc:i18nbundle
    id="samBundle"
     baseName="com.sun.netstorage.samqfs.web.resources.Resources" />

<tr>
<td valign="center" align="left" rowspan="1" colspan="2">
<cc:alertinline name="Alert" bundleID="samBundle" />
</td>
</tr>

<tr><td>
<cc:label name="startingDirLabel"
          bundleID="samBundle"
          defaultValue="archiving.startingdir.colon"/>
</td><td>
<cc:text name="startingDirText" bundleID="samBundle"/>
</td</tr>

<tr><td>
<cc:label name="namePatternLabel"
          bundleID="samBundle"
          defaultValue="archiving.namepattern.colon"/>
</td><td>
<cc:text name="namePatternText" bundleID="samBundle"/>
</td</tr>

<tr><td>
<cc:label name="minSizeLabel"
          bundleID="samBundle"
          defaultValue="archiving.minimumsize.colon"/>
</td><td>
<cc:text name="minSizeText" bundleID="samBundle"/>
</td</tr>

<tr><td>
<cc:label name="maxSizeLabel"
          bundleID="samBundle"
          defaultValue="archiving.maximumsize.colon"/>
</td><td>
<cc:text name="maxSizeText" bundleID="samBundle"/>
</td</tr>

<tr><td>
<cc:label name="ownerLabel"
          bundleID="samBundle"
          defaultValue="archiving.owner.colon"/>
</td><td>
<cc:text name="ownerText" bundleID="samBundle"/>
</td</tr>

<tr><td>
<cc:label name="groupLabel"
          bundleID="samBundle"
          defaultValue="archiving.group.colon"/>
</td><td>
<cc:text name="groupText" bundleID="samBundle"/>
</td</tr>

<tr><td>
<cc:label name="accessAgeLabel"
          bundleID="samBundle"
          defaultValue="archiving.accessage.colon"/>
</td><td>
<cc:text name="accessAgeText" bundleID="samBundle"/>
</td</tr>


<tr><td>
<cc:label name="stagingLabel"
          bundleID="samBundle"
          defaultValue="archiving.staging.colon"/>
</td><td>
<cc:text name="stagingText" bundleID="samBundle"/>
</td</tr>

<tr><td>
<cc:label name="releasingLabel"
          bundleID="samBundle"
          defaultValue="archiving.releasing.colon"/>
</td><td>
<cc:text name="releasingText" bundleID="samBundle"/>
</td</tr>

<tr><td>
<cc:label name="archiveAgeLabel"
          bundleID="samBundle"
          defaultValue="archiving.archiveage.colon"/>
</td><td>
<cc:text name="archiveAgeText" bundleID="samBundle" escape="false" />
</td</tr>

<tr><td>
<cc:label name="unarchiveAgeLabel"
          bundleID="samBundle"
          defaultValue="archiving.unarchiveage.colon"/>
</td><td>
<cc:text name="unarchiveAgeText" bundleID="samBundle" escape="false" />
</td</tr>

<tr><td>
<cc:label name="releaseOptionsLabel"
          bundleID="samBundle"
          defaultValue="archiving.releaseoptions.colon"/>
</td><td>
<cc:text name="releaseOptionsText" bundleID="samBundle" escape="false" />
</td</tr>

<tr><td>
<cc:label name="applyFSLabel"
          bundleID="samBundle"
          defaultValue="archiving.applyfilesystems.colon"/>
</td><td>
<cc:text name="applyFSText" bundleID="samBundle" escape="false" />
</td</tr>

<tr><td>
<cc:label name="saveMethodLabel" 
          bundleID="samBundle"
          defaultValue="archiving.criteria.wizard.save.how"/>
</td><td>
<cc:text name="saveMethodText" bundleID="saveBundle"/>
</jato:pagelet>
