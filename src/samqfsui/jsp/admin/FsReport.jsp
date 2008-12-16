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

// ident	$Id: FsReport.jsp,v 1.6 2008/12/16 00:10:40 am143972 Exp $
--%>

<%@ page info="Index" language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<%@ page import="com.iplanet.jato.RequestManager" %>
<%@ page import="com.sun.netstorage.samqfs.web.admin.FsReportView" %>
<%@ page import="com.sun.netstorage.samqfs.web.admin.FsReportViewBean" %>

<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.admin.FsReportViewBean">

<!-- page header -->
<cc:header
    pageTitle="reports.pagetitle.fsutilization"
    copyrightYear="2006" 
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    onLoad="
        if (parent.serverName != null) {
            parent.setSelectedNode('103', 'FsReport');
        }"
    bundleID="samBundle">

<script language="javascript" src="/samqfsui/js/samqfsui.js"></script>
<script language="javascript" src="/samqfsui/js/admin/FsReport.js"></script>

<style type="text/css">
  div.scroll400by570 { height: 400px; width: 570px; overflow: auto; }
  div.leftmargin { margin-left: 8px; }        
</style>

<!-- the form and its contents -->
<jato:form name="FsReportForm" method="POST">

<!-- alert for feedback -->
<cc:alertinline name="Alert" bundleID="samBundle"/>

<!-- page title -->
<cc:pagetitle name="PageTitle" 
              bundleID="samBundle" 
              pageTitleText="reports.pagetitle.fsutilization" 
              showPageTitleSeparator="true" 
              showPageButtonsTop="false" 
              showPageButtonsBottom="false">

<!-- Split the page to display the report summary table in one half and the -->
<!-- actual report in the other half -->
<br />
<table>
   <tr>
        <td><cc:spacer name="Spacer1" width="15"/></td>
        <td>
          <cc:text name="StaticText" bundleID="samBundle" 
            defaultValue="reports.msg.fsutilization" />         
        </td>
   </tr>
</table>

<br />

<jato:containerView name="FsReportView">           
  <table border="0" width="100%">
    <tr>
      <td width="40%" align="left" valign="top">
        <!-- action table -->
        <cc:actiontable name="reportSummaryTable"
          bundleID="samBundle"
          title="reports.tabletitle.fsutilization"
          selectionType="single" 
          selectionJavascript="toggleDisabledState(this)"
          showAdvancedSortIcon="false"
          showLowerActions="false"
          showPaginationControls="true"
          showPaginationIcon="true"
          showSelectionIcons="true"
          maxRows="25"
          page="1"/>
      </td>
      <td width="60%" align="center" valign="top">
        <%
          // Display the data formatted with the XSL applied to XML
          FsReportView view =  
            (FsReportView) viewBean.getChild(FsReportViewBean.FS_REPORT_VIEW);
          String str = view.getXhtmlString();
                
          if (str != null) {
            out.write(str);
          }
        %>
      </td>
    </tr>
    <tr>
      <td colspan="2">
        <cc:spacer name="Spacer1" width="5"/>
        <cc:href name="SampleFsHref" bundleID="samBundle">
          <cc:text 
            name="StaticText"
            bundleID="samBundle"
            defaultValue="reports.sample.fs"/>
        </cc:href>
        <cc:text name="SampleFsNoHref" bundleID="samBundle" 
            defaultValue="reports.sample.fs"/>
      </td>
    </tr>
  </table>
</jato:containerView>
<cc:hidden name="ServerName" />

</cc:pagetitle>
</jato:form>
</cc:header>
</jato:useViewBean>





























