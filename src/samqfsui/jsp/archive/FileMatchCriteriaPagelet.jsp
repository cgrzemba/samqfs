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

// ident	$Id: FileMatchCriteriaPagelet.jsp,v 1.7 2008/12/16 00:10:42 am143972 Exp $
--%>

<%@page info="FileMatchCriteraPagelet" language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:pagelet>
<cc:i18nbundle id="samBundle"
         baseName="com.sun.netstorage.samqfs.web.resource.Resources"/>

<cc:actiontable name="FileMatchCriteriaTable"
                bundleID="samBundle"
                title="archiving.policy.criteria.infotable.title"
                selectionType="single"
                selectionJavascript="handleFileMatchCriteriaTableSelection(this)"
                showAdvancedSortIcon="false"
                showLowerActions="false"
                showPaginationControls="true"
                showPaginationIcon="true"
                showSelectionIcons="true"
                maxRows="25"
                page="1"/>

<cc:spacer name="spacer" height="25" width="1"/>

</jato:pagelet>
