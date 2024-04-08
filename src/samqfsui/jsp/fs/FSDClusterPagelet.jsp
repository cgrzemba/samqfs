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

// ident	$Id: FSDClusterPagelet.jsp,v 1.11 2008/12/16 00:10:44 am143972 Exp $ -->
--%>

<%@page info="FSDClusterPagelet" language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:pagelet>
<!-- NOTE: use bundle definition in the parent page -->

<cc:actiontable name="FSDClusterTable"
                bundleID="samBundle"
                title="fs.details.cluster.table.title"
                selectionType="single"
                selectionJavascript="handleFSDClusterTableSelection(this)"
                showAdvancedSortIcon="false"
                showLowerActions="true"
                showPaginationControls="true"
                showPaginationIcon="true"
                showSelectionIcons="true"
                maxRows="25"
                page="1"/>
                
<cc:hidden name="nodeNames"/>
<cc:hidden name="nodeToRemove"/>
<cc:hidden name="mountedNodes"/>
<cc:hidden name="removeConfirmation" bundleID="samBundle"/>
</jato:pagelet>
