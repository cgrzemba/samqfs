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

// ident	$Id: NewReport.jsp,v 1.5 2008/12/16 00:10:40 am143972 Exp $
--%>
<%@ page language="java" %> 
<%@ page import="com.iplanet.jato.view.ViewBean" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.admin.NewReportViewBean">

<!-- Define the resource bundle, html, head, meta, stylesheet and body tags -->
<cc:header
    pageTitle="reports.create.title"
    copyrightYear="2006"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources" 
    onLoad="initializePopup(this);"
    bundleID="samBundle">

<script language="javascript" src="/samqfsui/js/popuphelper.js"></script>
<script language="javascript" src="/samqfsui/js/samqfsui.js"></script>
<script language="javascript">
    
    function handleRadioOnClick() {
        // Form Name
        var tf   = document.NewReportForm;    
        // Command Child Name, href defined in ViewBean
        var commandChildName = "NewReport.reportTypeRadioHref";
        var radioSelection = this.value;
        if (radioSelection != "Report of file system capacity and utilization") {
        
            // Set Form action URL and submit
            tf.action = tf.action + "?" + 
                commandChildName +
                "=&jato.pageSession=" + 
                tf.elements["<%=ViewBean.PAGE_SESSION_ATTRIBUTE_NVP_NAME %>"].value;
            tf.submit();
        }
        ccSetButtonDisabled('NewReport.Submit', 'NewReport', false);
    }

</script>

<!-- Masthead -->
<cc:secondarymasthead name="SecondaryMasthead" bundleID="samBundle" />

<jato:form name="NewReportForm" method="post">

<cc:alertinline name="Alert" bundleID="samBundle" />

<cc:pagetitle
    name="PageTitle"
    bundleID="samBundle"
    pageTitleText="reports.create.pagetitle"
    showPageTitleSeparator="true"
    showPageButtonsTop="false"
    showPageButtonsBottom="true">
      
    <br />

    <!-- Pick type of report, Use onClick event handler to populate pagelet -->
    <table>
        <tr>
            <td><cc:spacer name="Spacer1" width="25" /></td>  
            <td nowrap="true">
                <cc:radiobutton 
                    name="reportTypeRadio"
                    bundleID="samBundle"
                    styleLevel="3"
                    dynamic="true"
                    onClick="handleRadioOnClick();" />
            </td>
        </tr>     
    </table>
    
    <cc:includepagelet name="NewReportView"/>
    
</cc:pagetitle>

</jato:form>
</cc:header>
</jato:useViewBean> 
