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

// ident	$Id: AdminNotification.jsp,v 1.28 2008/10/01 22:43:31 ronaldso Exp $
--%> 
<%@ page info="Index" language="java" %> 
<%@ page import="com.iplanet.jato.view.ViewBean" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.admin.AdminNotificationViewBean">

<cc:header pageTitle="emailAlert.title"
    copyrightYear="2006"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    onLoad="
        if (parent.serverName != null) {
            parent.setSelectedNode('38', 'AdminNotification');
        }
        if (document.AdminNotificationForm.elements[
            'AdminNotification.AdminNotificationView.NewSubscriberField'] != null) {
            document.AdminNotificationForm.elements[
                'AdminNotification.AdminNotificationView.NewSubscriberField'].focus();
        }"
    bundleID="samBundle" >
   
<script language="javascript" src="/samqfsui/js/samqfsui.js"></script>
<script language="javascript" src="/samqfsui/js/popuphelper.js"></script>
<script language="javascript"
    src="/samqfsui/js/admin/AdminNotification.js"></script>

<jato:form name="AdminNotificationForm" method="post">

<cc:alertinline name="Alert" bundleID="samBundle" />

<jato:containerView name="AdminNotificationView">

<cc:pagetitle name="PageTitle" 
    bundleID="samBundle"
    pageTitleText="emailAlert.title"
    showPageTitleSeparator="true"
    showPageButtonsTop="true"
    showPageButtonsBottom="true">
    
<!-- Subscriber name/add -->
<table>
  <tr>
    <td><cc:spacer name="Spacer1" width="25"/></td>
    <td>
        <cc:label 
            name="SubscriberLabel"
            styleLevel="2"
            defaultValue="emailAlert.label.existingSubscribers" 
            bundleID="samBundle"/>
        <cc:dropdownmenu name="SubscriberList"
            bundleID="samBundle" 
            title="emailAlert.tooltip.subscriberList"
            escape="false"
            dynamic="true"
            commandChild="SubscriberHref"
            type="jump"/>
        <cc:textfield
            name="NewSubscriberField"
            bundleID="samBundle" 
            maxLength="126"
            disabled="false"
            dynamic="true"
            defaultValue=""
            title="emailAlert.tooltip.newEmail"
            autoSubmit="false" />  
        <cc:helpinline type="field">
            <cc:text name="SubscriptionDetail" bundleID="samBundle" 
                defaultValue="emailAlert.msg.addEmail" />
        </cc:helpinline>
    </td>
  </tr>
</table>

<!-- PropertySheet -->
<cc:propertysheet 
    name="PropertySheet" 
    bundleID="samBundle" 
    showJumpLinks="false"/>
    
<cc:hidden name="ServerConfig"/> <!-- QFS Standalone, 4.5 or Intellistore -->
</cc:pagetitle>

</jato:containerView>

<cc:hidden name="ServerName" />

<cc:hidden name="Messages" />

</jato:form>
</cc:header>
</jato:useViewBean> 
