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

// ident	$Id: DataClassDetails.jsp,v 1.13 2008/03/17 14:40:30 am143972 Exp $
--%>

<%@page info="DataClassDetails" language="java"%>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato" %>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc" %>

<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.archive.DataClassDetailsViewBean">

<script type="text/javascript" src="/samqfsui/js/popuphelper.js"></script>
<script type="text/javascript" src="/samqfsui/js/archive/DataClassDetails.js">
</script>

<!-- page header -->
<cc:header
    pageTitle="archiving.dataclass.details.headertitle"
    copyrightYear="2006"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    onLoad="
        if (parent.serverName != null) {
            parent.setSelectedNode('11', 'DataClassDetails');
        }
        initializeDataClassAttributes();"
    bundleID="samBundle">

<!-- the form -->
<jato:form name="DataClassDetailsForm" method="POST">

<!-- bread crumbing -->
<cc:breadcrumbs name="BreadCrumb" bundleID="samBundle" />

<!-- alert for feedback -->
<cc:alertinline name="Alert" bundleID="samBundle"/> <br />

<!-- page title -->
<cc:pagetitle
    name="PageTitle"
    bundleID="samBundle"
    pageTitleText="archiving.dataclass.details.pagetitle"
    showPageTitleSeparator="true"
    showPageButtonsTop="true"
    showPageButtonsBottom="true">

<!-- description-->
<div style="text-indent:20px">
<cc:label name="classDescription" styleLevel="2" bundleID="samBundle"/>
</div>

<!-- the property sheet -->
<cc:propertysheet
    name="PropertySheet"
    bundleID="samBundle"
    addJavaScript="true"
    showJumpLinks="false"/>
</cc:pagetitle>

<cc:hidden name="fsname"/>
<cc:hidden name="dumpPath"/>
<cc:hidden name="fsDeletable"/>
<cc:hidden name="fsDeleteConfirmation"/>
<cc:hidden name="psAttributes"/>
<cc:hidden name="fsList"/>
<cc:hidden name="afterDateHiddenField"/>
<cc:hidden name="ServerName" />

</jato:form>
</cc:header>
</jato:useViewBean>
