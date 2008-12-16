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

// ident	$Id: SystemMonitoring.jsp,v 1.10 2008/12/16 00:10:50 am143972 Exp $
--%>

<%@ page info="Index" language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.monitoring.SystemMonitoringViewBean">

<jato:form name="SystemMonitoringForm" method="post">

<script language="javascript" src="/samqfsui/js/popuphelper.js"></script>
<script language="javascript">

    function launchSystemMonitorPopUp() {
        var serverName = document.SystemMonitoringForm.elements[
                        "SystemMonitoring.ServerName"].value;

        // IE does not like long window names, and it does not take names
        // with special characters like underscore and hyphen
        var windowName = "";
        if (navigator.appName != "Netscape") {
            windowName = 'mc' + top.consoleNumber;
            top.consoleNumber += 1; 
        } else {
            windowName = 'mc' + serverName;
        }

        launchPopup(
            '/monitoring/MonitoringFrameFormat',
            windowName,
            serverName,
            SIZE_MONITOR_CONSOLE);
        }

</script>

<!-- Define the resource bundle, html, head, meta, stylesheet and body tags -->
<cc:header pageTitle="Monitor.main.pageTitle"
    copyrightYear="2006"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    onLoad="
        if (parent.serverName != null) {
            parent.setSelectedNode('100', 'SystemMonitoring');
        }
        launchSystemMonitorPopUp();"
    bundleID="samBundle">
   
<cc:pagetitle
    name="PageTitle"
    bundleID="samBundle"
    pageTitleText="Monitor.main.pageTitle"
    showPageTitleSeparator="true"
    showPageButtonsTop="false"
    showPageButtonsBottom="false">

<table cellspacing="20" width="50%">
<tr>
    <td>
        <cc:text
            name="Description"
            bundleID="samBundle"
            escape="false"
            defaultValue="Monitor.main.description" />
    </td>
</tr>
<tr>
    <td>
        <cc:button
            name="LaunchButton"
            bundleID="samBundle"
            onClick="launchSystemMonitorPopUp(); return false;"
            defaultValue="Monitor.main.launch" />
    </td>
</tr>
</table>

<cc:hidden name="ServerName" />

</cc:pagetitle>

</cc:header>
</jato:form>
</jato:useViewBean> 
