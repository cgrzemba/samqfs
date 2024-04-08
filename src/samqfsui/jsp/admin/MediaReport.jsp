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

// ident	$Id: MediaReport.jsp,v 1.7 2008/12/16 00:10:40 am143972 Exp $
--%>

<%@ page info="Index" language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<%@ page import="com.sun.netstorage.samqfs.web.admin.MediaReportView" %>
<%@ page import="com.sun.netstorage.samqfs.web.admin.MediaReportViewBean" %>

<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.admin.MediaReportViewBean">

<!-- page header -->
<cc:header
    pageTitle="reports.pagetitle.media"
    copyrightYear="2006" 
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    onLoad="
        if (parent.serverName != null) {
            parent.setSelectedNode('101', 'MediaReport');
        }"
    bundleID="samBundle">

<script language="javascript" src="/samqfsui/js/samqfsui.js"></script>
<script language="javascript" src="/samqfsui/js/popuphelper.js"></script>
<script language="javascript" src="/samqfsui/js/admin/MediaReport.js"></script>

<!-- the form and its contents -->
<jato:form name="MediaReportForm" method="POST">

<!-- alert for feedback -->
<cc:alertinline name="Alert" bundleID="samBundle"/>

<!-- page title -->
<cc:pagetitle name="PageTitle" 
  bundleID="samBundle" 
  pageTitleText="reports.pagetitle.media" 
  showPageTitleSeparator="true" 
  showPageButtonsTop="false" 
  showPageButtonsBottom="false">

<br />
<table>
  <tr>
    <td><cc:spacer name="Spacer1" width="15"/></td>
    <td>
      <cc:text name="StaticText" bundleID="samBundle" 
        defaultValue="reports.msg.media" />
    </td>
  </tr>
</table>

<br />

<jato:containerView name="MediaReportView">           
  <cc:actiontable name="reportSummaryTable"
    bundleID="samBundle"
    title="reports.tabletitle.media"
    selectionType="single"
    selectionJavascript="toggleDisabledState(this)"
    showAdvancedSortIcon="false"
    showLowerActions="false"
    showPaginationControls="true"
    showPaginationIcon="true"
    showSelectionIcons="true"
    maxRows="5"
    page="1"/>

  <br />
   
  <table border="0" width="100%">
    <tr>
      <td><cc:spacer name="Spacer1" width="5"/></td>
      <td align="center" valign="top" colspan="2">     
        <%
          // Display the data formatted with the XSL applied to XML
          MediaReportView view = (MediaReportView) viewBean.getChild(
              MediaReportViewBean.MEDIA_REPORT_VIEW);
          String str = view.getXhtmlString();
                
          if (str != null) {
            out.write(str);
          }
        %>
      </td>
      <td><cc:spacer name="Spacer1" width="5"/></td>
    </tr>
    <tr>
      <td><cc:spacer name="Spacer1" width="5"/></td>
      <td>
        <cc:href name="SampleMediaHref" bundleID="samBundle">
          <cc:text
            name="StaticText"
            bundleID="samBundle"
            defaultValue="reports.sample.media"/>
        </cc:href>
        <cc:text name="SampleMediaNoHref" bundleID="samBundle" 
            defaultValue="reports.sample.media"/>
      </td>        
      <td align="left">
        <cc:href name="SampleStkHref" bundleID="samBundle">
          <cc:text 
            name="StaticText"
            bundleID="samBundle"
            defaultValue="reports.sample.stk"/> <!-- display optional -->
        </cc:href>
        <cc:text name="SampleStkNoHref" bundleID="samBundle" 
            defaultValue="reports.sample.stk"/>
      </td>
      <td><cc:spacer name="Spacer1" width="5"/></td>
    </tr>  
  </table>
</jato:containerView>
<cc:hidden name="ServerName" />

</cc:pagetitle>
</jato:form>
</cc:header>
</jato:useViewBean>
