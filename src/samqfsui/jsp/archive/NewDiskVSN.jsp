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

// ident	$Id: NewDiskVSN.jsp,v 1.16 2008/12/16 00:10:42 am143972 Exp $
--%>

<%@page info="NewDiskVSN" language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>
<%@taglib uri="samqfs.tld" prefix="samqfs"%>

<jato:useViewBean className="com.sun.netstorage.samqfs.web.archive.NewDiskVSNViewBean">

<!-- helper javascript -->
<script type="text/javascript" src="/samqfsui/js/samqfsui.js"></script>
<script type="text/javascript" src="/samqfsui/js/archive/diskvsn.js"></script>
<script type="text/javascript" src="/samqfsui/js/popuphelper.js"></script>

<!-- header -->
<cc:header pageTitle="archiving.diskvsn.newvsn.pagetitle"
           copyrightYear="2006"
           baseName="com.sun.netstorage.samqfs.web.resources.Resources"
           bundleID="samBundle"
           onLoad="initializePopup(this); initDivs();">

<!-- masthead -->
<cc:secondarymasthead name="SecondaryMasthead" bundleID="samBundle"/>

<!-- the form and its contents -->
<jato:form name="NewDiskVSNForm" method="POST">

<!-- feedback alert -->
<cc:alertinline name="Alert" bundleID="samBundle"/>

<!-- page title -->
<cc:pagetitle name="pageTitle"
              bundleID="samBundle"
              pageTitleText="archiving.diskvsn.newvsn.pagetitle"
              showPageTitleSeparator="true"
              showPageButtonsTop="false"
              showPageButtonsBottom="true">

<div id="commonDiv" style="padding-left:10px; padding-top:20px">
<table cellspacing="10" >
<tr><td>
<cc:label name="mediaTypeLabel"
          elementName="mediaType"
          bundleID="samBundle"
          showRequired="true"
          defaultValue="archiving.diskvsn.newvsn.mediatype"/>
</td><td>
<cc:dropdownmenu name="mediaType"
                 bundleID="samBundle"
                 onChange="handleMediaTypeChange(this);"/>
</td></tr>
         
<tr><td>
<cc:label name="vsnNameLabel" 
          elementName="vsnName"
          bundleID="samBundle"
          showRequired="true"
          defaultValue="archiving.diskvsn.newvsn.name"/>
</td><td>
<cc:textfield name="vsnName" maxLength="31"/>
</td></tr>
</table>
</div>

<div id="diskDiv" style="display:; padding-left:10px">
<table cellspacing="10" >
<tr><td>
<cc:label name="vsnHostLabel"
          elementName="vsnHost"
          bundleID="samBundle"
          showRequired="true"
          defaultValue="archiving.diskvsn.newvsn.host"/>
</td><td>
<cc:dropdownmenu name="vsnHost"
                 onChange="handleHostChange(this);"
                 dynamic="true"
                 bundleID="samBundle"/>
</td></tr>

<tr><td>
<cc:label name="vsnPathLabel"
          elementName="filechooser"
          bundleID="samBundle"
          showRequired="true"
          defaultValue="archiving.diskvsn.newvsn.path"/>
</td><td>
<samqfs:remotefilechoosercontrol name="filechooser"
    type="folder"
    multipleSelect="false" />
</td></tr>

<tr><td>
</td><td>
<cc:checkbox name="createPath"
             bundleID="samBundle"
             dynamic="true" 
             label="archiving.diskvsnpath.create.label" />
</td></tr>
</table>
</div>

<div id="honeyCombDiv" style="display:none; padding-left:10px">

<table cellspacing="10" >
<tr><td colspan="2" style="padding-left:20px">
<cc:helpinline type="page">
    <cc:text name="help" 
             defaultValue="archiving.diskvsn.newvsn.hchelp"
             bundleID="samBundle"/>
</cc:helpinline>
</td></tr>

<tr><td>
<cc:label name="dataIPLabel"
          elementName="dataIP"
          bundleID="samBundle"
          showRequired="true"
          defaultValue="archiving.diskvsn.newvsn.dataip"/>
</td><td>
<cc:textfield name="dataIP"/>
</td></tr>

<tr><td>
<cc:label name="portLabel"
          elementName="port"
          bundleID="samBundle"
          showRequired="true"
          defaultValue="archiving.diskvsn.newvsn.port"/>
</td><td>
<cc:textfield name="port"/>
</td></tr>
</table>
</div>

<cc:spacer name="spacer" height="20"/>

<span style="padding-left: 18px">
<cc:text name="browsingRules"
         bundleID="samBundle"/>
</span>

<cc:hidden name="RFCCapableHosts"/>
<cc:hidden name="stateData"/>

</cc:pagetitle>
</jato:form>
</cc:header>
</jato:useViewBean>
