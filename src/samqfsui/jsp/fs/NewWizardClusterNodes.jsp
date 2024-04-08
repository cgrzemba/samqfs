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

// ident	$Id: NewWizardClusterNodes.jsp,v 1.7 2008/12/16 00:10:46 am143972 Exp $
--%>

<%@ page language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%> 
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<script type="text/javascript" src="/samqfsui/js/fs/cluster.js"></script>

<jato:pagelet>

<cc:i18nbundle
    id="samBundle"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources" />

<cc:alertinline name="Alert" bundleID="samBundle" />

<tr><td valign="top">
<cc:label name="clusterNodeLabel"
          bundleID="samBundle"
          elementName="clusterNode"
          defaultValue="FSWizard.new.clusternodes.label"/>
</td><td>
<cc:selectablelist name="clusterNode"
                   size="10"
                   multiple="true"
                   onClick="handleClusterNodeSelection(this);"
                   bundleID="samBundle"/>
</td></tr>
</jato:pagelet>
