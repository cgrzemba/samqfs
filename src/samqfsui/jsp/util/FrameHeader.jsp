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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

// ident	$Id: FrameHeader.jsp,v 1.7 2008/12/16 00:10:51 am143972 Exp $
--%>
<%@ page language="java" %>
<%@ page import="com.iplanet.jato.view.ViewBean" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>


<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.util.FrameHeaderViewBean">

<script language="Javascript" src="/samqfsui/js/FrameHelper.js"></script>
<script language="Javascript">

    function doRefreshTarget() {
        var targetForm = getContentFrameForm();

        // Skip JSF pages, all old JATO pages do not have form ID
        if (targetForm.id == "") {

            // Command Child Name
            var commandChildName = getContentFramePageName() + ".RefreshHref";

            // Set Form action URL and submit
            targetForm.action = targetForm.action + "?" + 
                commandChildName + "=&jato.pageSession=" + 
                targetForm.elements[
                   "<%=ViewBean.PAGE_SESSION_ATTRIBUTE_NVP_NAME %>"].value;
        }

        targetForm.submit();
    }
    
</script>

<!-- Define the resource bundle -->
<cc:header
    pageTitle=""
    copyrightYear="2006"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources" 
    bundleID="samBundle">
    
<cc:form name="FrameHeaderForm" method="post">
    
<!-- Masthead -->
<cc:primarymasthead name="Masthead" bundleID="samBundle" />

<cc:hidden name="PreferenceHidden" />

</cc:form>   
</cc:header>

</jato:useViewBean> 
