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

// ident	$Id: FileMetricSummary.jsp,v 1.18 2008/12/16 00:10:40 am143972 Exp $
--%>

<%@ page info="Index" language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<%@ page import="com.sun.netstorage.samqfs.web.admin.FileMetricSummaryViewBean" %>
<%@ page import="com.sun.netstorage.samqfs.web.util.Constants"%>

<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.admin.FileMetricSummaryViewBean">

<!-- page header -->
<cc:header
    pageTitle="reports.pagetitle.filedistribution"
    copyrightYear="2006" 
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    onLoad="
        if (parent.serverName != null) {
            parent.setSelectedNode('102', 'FileMetricSummary');
        }"
    bundleID="samBundle"
    preserveFocus="true"
    preserveScroll="true">

<script language="javascript" src="/samqfsui/js/samqfsui.js"></script>
<script language="javascript" src="/samqfsui/js/popuphelper.js"></script>
<script language="javascript" src="/samqfsui/js/admin/FileMetric.js"></script>

<style type="text/css">
div.scrollheight400 { height: 400px; overflow: auto; }
div.leftmargin { margin-left: 8px; }        
</style>

<!-- the form and its contents -->
<jato:form name="FileMetricSummaryForm" method="POST">

<!-- alert for feedback -->
<cc:alertinline name="Alert" bundleID="samBundle"/>

<!-- page title -->
<cc:pagetitle name="PageTitle" 
              bundleID="samBundle" 
              pageTitleText="reports.pagetitle.filedistribution" 
              showPageTitleSeparator="true" 
              showPageButtonsTop="false" 
              showPageButtonsBottom="false">


<table border="0" width="100%">
  <tr>
    <td width="75%">
      <div class="leftmargin">
        <cc:text name="ScheduleText" bundleID="samBundle" escape="false" defaultValue=""/>
      </div>
    </td>
    <td width="25%" valign="top">
      <cc:button name="ScheduleMetricButton" bundleID="samBundle" 
        defaultValue="reports.button.schedule"
        dynamic="true"
        disabled="true"
        title="module3.secondaryMiniButtonTitleEnabled"
        titleDisabled="module3.secondaryMiniButtonTitleDisabled"
        type="secondary"
        onClick="
          launchPopup(
            '/admin/ScheduleFsMetric',
            'scheduleFsMetric',
            getServerKey(),
            SIZE_NORMAL,
            '&amp;SAMQFS_FS_NAME=' + getFsName());
          return false;
          "/>
    </td>
  </tr>
</table>
<table border="0" width="85%">
  <tr>   
    <td width="45%" align="left">
      <cc:spacer name="Spacer1" width="5"/>
      <cc:label
        name="fsnameLabel"
        styleLevel="2"
        defaultValue="reports.fsname.label" 
        bundleID="samBundle"/>
      <cc:dropdownmenu name="fsnameOptions"
        bundleID="samBundle" 
        title="Select the file system"
        escape="false"
        dynamic="true"
        commandChild="fsnameHref"
        type="jump"
        onChange="
          var button = 'FileMetricSummary.ScheduleMetricButton';
          ccSetButtonDisabled(button, 'FileMetricSummaryForm', this.value == '-- Sample --');
        "/>
    </td>
    <td width="15%">
      <cc:spacer name="Spacer1" width="5"/>
      <cc:label 
        name="startDateLabel"
        styleLevel="2"
        defaultValue="common.label.startDate" 
        bundleID="samBundle"/>
      <cc:datetimewindow
        name="startDate"
        showTextInput="true"
        bundleID="samBundle"/>
    </td>
    <td width="15%">
      <cc:spacer name="Spacer1" width="5"/>
      <cc:label 
        name="endDateLabel"
        styleLevel="2"
        defaultValue="common.label.endDate" 
        bundleID="samBundle"/>
      <cc:datetimewindow
        name="endDate"
        showTextInput="true"
        bundleID="samBundle"/>
    </td>
    <td width="10%">
      <cc:button 
        name="TimeRangeButton" 
        bundleID="samBundle" 
        defaultValue="go"
        dynamic="true"
        type="secondaryMini"/>
    </td>   
  </tr>
</table>
<table>
  <tr>
    <td width="180" height="15" valign="top" nowrap="nowrap">
      <cc:spacer name="Spacer1" width="5"/>
      <cc:href name="StorageTierHref" bundleID="samBundle">
        <cc:text 
          name="StaticText"
          bundleID="samBundle"
          defaultValue="reports.type.storagetier"/>
      </cc:href>
      <cc:text name="StorageTierNoHref" bundleID="samBundle" defaultValue=""/>
    </td>
    <td width="400" align="center" valign="top" rowspan="6" colspan="3">
      <img src="
           <%
           String jpgName = viewBean.getJpgFileName();
           if (jpgName == null) {
               out.write("/samqfsui/xsl/svg/error.jpg");
           } else {
               System.out.println("jpg: " + jpgName);
               if (!jpgName.startsWith("/samqfsui")) {
                    String name = new StringBuffer().append("/samqfsui").append(jpgName).toString();
                         out.write(name);
                    } else {
               out.write(jpgName);
                    }
           }
           %>"
      alt="TimeSeries Image" 
      usemap="#mymap"
      width="700"
      height="700"
      border="0"
      align="top"/>
    </td>
  </tr>
  <tr>
    <td width="180" height="15" valign="top" nowrap="nowrap">
      <cc:spacer name="Spacer1" width="5"/>
      <cc:href name="FileByLifeHref" bundleID="samBundle">
        <cc:text 
          name="StaticText"
          bundleID="samBundle"
          defaultValue="reports.type.filebylife"/>
      </cc:href>
      <cc:text name="FileByLifeNoHref" bundleID="samBundle" defaultValue=""/>
    </td>
  </tr>
  <tr>
    <td width="180" height="15" valign="top" nowrap="nowrap">
      <cc:spacer name="Spacer1" width="5"/>
      <cc:href name="FileByAgeHref" bundleID="samBundle">
        <cc:text 
          name="StaticText"
          bundleID="samBundle"
          defaultValue="reports.type.filebyage"/>
      </cc:href>
      <cc:text name="FileByAgeNoHref" bundleID="samBundle" defaultValue=""/>
    </td>
  </tr>
  <tr>
    <td width="180" height="15" valign="top" nowrap="nowrap">
      <cc:spacer name="Spacer1" width="5"/>
      <cc:href name="FileByOwnerHref" bundleID="samBundle">
        <cc:text 
          name="StaticText"
          bundleID="samBundle"
          defaultValue="reports.type.filebyowner"/>
      </cc:href>
      <cc:text name="FileByOwnerNoHref" bundleID="samBundle" defaultValue=""/>
    </td>
  </tr>
  <tr>
    <td width="180" valign="top" rowspan="2" nowrap="nowrap">
      <cc:spacer name="Spacer1" width="5"/>
      <cc:href name="FileByGroupHref" bundleID="samBundle">
        <cc:text 
          name="StaticText"
          bundleID="samBundle"
          defaultValue="reports.type.filebygroup"/>
      </cc:href>
      <cc:text name="FileByGroupNoHref" bundleID="samBundle" defaultValue=""/>
    </td>
  </tr>
</table>

<cc:hidden name="ServerName" />

<map name="mymap">
  <%
    String imageMap = viewBean.getMapString();
    out.write(imageMap);
  %>
</map>

</cc:pagetitle>
</jato:form>
</cc:header>
</jato:useViewBean>





























