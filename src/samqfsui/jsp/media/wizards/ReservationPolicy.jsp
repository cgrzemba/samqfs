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

// ident	$Id: ReservationPolicy.jsp,v 1.6 2008/05/16 19:39:22 am143972 Exp $
--%>

<%@ page language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%> 
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<script type="text/javascript"
    src="/samqfsui/js/media/wizards/ReservationPolicy.js"></script>
<script type="text/javascript">
    WizardWindow_Wizard.pageInit = wizardPageInit;
</script>

<jato:pagelet>

<cc:i18nbundle id="samBundle"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources" />

<cc:alertinline name="Alert" bundleID="samBundle" />
<br />


<tr>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:actiontable
            name="SelectPolicyTable"
            bundleID="samBundle"
            title="ReservationPolicy.tabletitle"
            selectionType="single"
            showAdvancedSortIcon="false"
            showLowerActions="false"
            showPaginationControls="false"
            showPaginationIcon="false"
            showSelectionIcons="true" />
    </td>
</tr>

<cc:hidden name="errorOccur" />

</jato:pagelet>
