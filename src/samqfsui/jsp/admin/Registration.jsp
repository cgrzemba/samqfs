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

// ident	$Id: Registration.jsp,v 1.4 2008/03/17 14:40:29 am143972 Exp $
--%>

<%@ page info="Index" language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean 
    className="com.sun.netstorage.samqfs.web.admin.RegistrationViewBean">

<!-- Define the resource bundle, html, head, meta, stylesheet and body tags -->
<cc:header
    pageTitle="registration.title"
    copyrightYear="2006"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    onLoad="
        if (parent.serverName != null) {
            parent.setSelectedNode('112', 'Registration');
        }"
    bundleID="samBundle">

<!-- Form -->
<jato:form name="Form" method="post">

<!-- inline alart -->
<cc:alertinline name="Alert" bundleID="samBundle" />

<!-- JavaScript Hidden Info -->
<cc:hidden name="HiddenMessages"/>

<!-- Page title -->
<cc:pagetitle name="PageTitle" 
    bundleID="samBundle"
    pageTitleText="registration.title"
    pageTitleHelpMessage="registration.title.helptext"
    showPageTitleSeparator="true"
    showPageButtonsTop="true"
    showPageButtonsBottom="true">

<!-- PropertySheet -->
<cc:propertysheet 
    name="PropertySheet" 
    bundleID="samBundle" 
    showJumpLinks="true"/>

</cc:pagetitle>

<br>

</jato:form>
</cc:header>
</jato:useViewBean> 

