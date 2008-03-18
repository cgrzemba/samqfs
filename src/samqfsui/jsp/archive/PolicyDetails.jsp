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

// ident	$Id: PolicyDetails.jsp,v 1.11 2008/03/17 14:40:31 am143972 Exp $
--%>

<%@page info="PolicyDetails" language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato" %>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>
<%@page import="com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy" %>
<%@page import="com.sun.netstorage.samqfs.web.util.Constants" %>

<jato:useViewBean className="com.sun.netstorage.samqfs.web.archive.PolicyDetailsViewBean">

<!-- include helper javascript -->
<script type="text/javascript" src="/samqfsui/js/archive/policydetails44.js">
</script>

<!-- page header -->
<cc:header
    pageTitle="archiving.policy.details.headertitle" 
    copyrightYear="2006" 
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    onLoad="
        if (parent.serverName != null) {
            parent.setSelectedNode('12', 'PolicyDetails');
        }"
    bundleID="samBundle">

<!-- the form -->
<jato:form name="PolicyDetailsForm" method="POST">

<!-- bread crumbing -->
<cc:breadcrumbs name="BreadCrumb" bundleID="samBundle" />

<!-- alert for feedback -->
<cc:alertinline name="Alert" bundleID="samBundle"/> <br />

<!-- page title -->
<cc:pagetitle name="PageTitle"
              bundleID="samBundle"
              pageTitleText="archiving.policy.details.pagetitle"
              showPageTitleSeparator="true"
              showPageButtonsTop="false"
              showPageButtonsBottom="false">

<!-- if displaying a default policy, show the view all policies button -->
<table cellspacing="10px">
<tr>
    <td style="wdith:75%;padding-left:20px">
        <cc:text name="policyTypeDescription" 
                 bundleID="samBundle"/>
    </td><td align="right">
        <cc:button name="defaultPolicyViewAllPolicies" 
                    bundleID="samBundle"/>
    </td>
</tr>
</table>

<cc:spacer name="spacer" height="20" width="1"/>

<!-- the pagelets for the policy details tables -->
<cc:includepagelet name="FileMatchCriteriaView"/>
<cc:includepagelet name="CopySettingsView"/>
<cc:includepagelet name="CopyInformationView"/>

<!-- add/remove button helpers -->
<cc:hidden name="isCriteriaDeletable"/>
<cc:hidden name="isCopyDeletable"/>
<cc:hidden name="criteriaDeleteConfirmation"/>
<cc:hidden name="copyDeleteConfirmation"/>

<!--- delete copy/criteria button helpers -->
<cc:hidden name="criteriaNumbers"/>
<cc:hidden name="selectedCriteriaNumber"/>
<cc:hidden name="copyNumbers"/>
<cc:hidden name="selectedCopyNumber"/>
<cc:hidden name="copyMediaTypes"/>
<cc:hidden name="selectedCopyMediaType"/>

</cc:pagetitle>
</jato:form>
</cc:header>
</jato:useViewBean>
