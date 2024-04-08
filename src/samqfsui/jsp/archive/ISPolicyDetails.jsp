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

// ident	$Id: ISPolicyDetails.jsp,v 1.14 2008/12/16 00:10:42 am143972 Exp $
--%>

<%@page info="ISPolicyDetails" language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato" %>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>
<%@page import="com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy" %>
<%@page import="com.sun.netstorage.samqfs.web.util.Constants" %>

<jato:useViewBean className="com.sun.netstorage.samqfs.web.archive.ISPolicyDetailsViewBean">

<!-- include helper javascript -->
<script type="text/javascript" src="/samqfsui/js/samqfsui.js"></script>

<%--
<script type="text/javascript" src="/samqfsui/js/archive/ispolicydetails.js">
</script>
--%>

<!-- page header -->
<cc:header pageTitle="archiving.policy.details.headertitle" 
           copyrightYear="2006" 
           baseName="com.sun.netstorage.samqfs.web.resources.Resources" 
           bundleID="samBundle"
           onLoad="
                if (parent.serverName != null) {
                    parent.setSelectedNode('12', 'ISPolicyDetails');
                }
                initializepolicydetailspage(this);">

<!-- the form -->
<jato:form name="ISPolicyDetailsForm" method="POST">

<!-- bread crumbing -->
<cc:breadcrumbs name="BreadCrumb" bundleID="samBundle" />

<!-- alert for feedback -->
<cc:alertinline name="Alert" bundleID="samBundle"/> <br />

<!-- page title -->
<cc:pagetitle name="PageTitle"
              bundleID="samBundle"
              pageTitleText="archiving.policy.details.pagetitle"
              showPageTitleSeparator="true"
              showPageButtonsTop="true"
              showPageButtonsBottom="true">

<div style="text-indent:20px">
<cc:label name="policyDescription" styleLevel="2"/>
</div>

<!--include the copy pagelet for all policies except the allsets policy-->
<cc:includepagelet name="ISPolicyDetailsView"/>
<cc:includepagelet name="ISAllsetsPolicyDetailsView"/>

<%--
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
--%>
<cc:hidden name="maxCopyCount" elementId="maxCopyCount"/>

</cc:pagetitle>
</jato:form>
</cc:header>
</jato:useViewBean>
