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

// ident	$Id: CopyOptions.jsp,v 1.9 2008/03/17 14:40:30 am143972 Exp $
--%>

<%@page info="CopyOptions" language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato" %>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.archive.CopyOptionsViewBean">

<!-- include helper javascript -->
<script type="text/javascript" src="/samqfsui/js/archive/copyoptions.js">
</script>

<!-- page header -->
<cc:header
    pageTitle="archiving.policy.copyoptions.headertitle" 
    copyrightYear="2006" 
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    onLoad="
        if (parent.serverName != null) {
            parent.setSelectedNode('12', 'CopyOptions');
        }"
    bundleID="samBundle">

<!-- the form -->
<jato:form name="CopyOptionsForm" method="POST">

<!-- bread crumbing -->
<cc:breadcrumbs name="BreadCrumb" bundleID="samBundle" />

<!-- alert for feedback -->
<cc:alertinline name="Alert" bundleID="samBundle"/> <br />

<!-- page title -->
<cc:pagetitle name="PageTitle"
              bundleID="samBundle"
              pageTitleText="archiving.policy.copyoptions.pagetitle"
              showPageTitleSeparator="true"
              showPageButtonsTop="true"
              showPageButtonsBottom="true">

<!-- the pagelets for the policy details tables -->
<cc:includepagelet name="DiskCopyOptionsView"/>
<cc:includepagelet name="TapeCopyOptionsView"/>

<cc:hidden name="hardReset"/>

</cc:pagetitle>
</jato:form>
</cc:header>
</jato:useViewBean>
