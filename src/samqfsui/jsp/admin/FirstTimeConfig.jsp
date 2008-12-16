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
// ident	$Id: FirstTimeConfig.jsp,v 1.8 2008/12/16 00:10:40 am143972 Exp $
--%>

<%@page info="FirstTimeConfig" language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>
<%@taglib uri="samqfs.tld" prefix="samqfs"%>

<jato:useViewBean className="com.sun.netstorage.samqfs.web.admin.FirstTimeConfigViewBean">

<script tyep="text/javascript" src="/samqfsui/js/popuphelper.js"></script>
<script type="text/javascript" src="/samqfsui/js/admin/firsttimeconfig.js"></script>

<cc:header
    pageTitle="node.firsttimeconfig"
    copyrightYear="2007"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    onLoad="
        if (parent.serverName != null) {
            parent.setSelectedNode('201', 'FirstTimeConfig');
        }"
    bundleID="samBundle">


<jato:form name="FirstTimeConfigForm">
<cc:alertinline name="Alert" bundleID="samBundle"/>

<cc:pagetitle name="PageTitle"
              bundleID="samBundle"
              pageTitleText="node.firsttimeconfig"
              showPageTitleSeparator="true"
              showPageButtonsTop="fase"
              showPageButtonsBottom="false">

<!-- page level instruction -->
<div style="margin:10px;width:75%">
<cc:text name="instruction" bundleID="samBundle" defaultValue="firsttime.instruction"/>
<cc:href name="moreHref" bundleID="samBundle" onClick="return handleMoreInformation();">
    <cc:image name="moreImg" bundleID="samBundle" border="0" defaultValue="/samqfsui/images/link_arrow.gif"/>
    <cc:text name="moreText" bundleID="samBundle" defaultValue="firsttime.moreinfo"/>
</cc:href>

<cc:legend name="Legend" align="right"/>
</div>

<div style="margin:20px">
<img src="/samqfsui/images/dot.gif"height="1" width="100%" class="ConLin"/>
</div>

<!-- item list -->
<div style="margin-top:20px;margin-left:30px">
<cc:label name="nearline" bundleID="samBundle" defaultValue="firsttime.nearline"/>
<br/>

<div style="margin-left:20px">
    <cc:image name="asteriskImg" defaultValue="/samqfsui/images/asterisk.gif"/>
<cc:href name="addLibraryHref" bundleID="samBundle"
         onClick="return launchAddLibraryWizard();"
         extraHtml="style=\"font-size:1.25em\"">
    <cc:text name="addLibraryText" bundleID="samBundle" defaultValue="firsttime.library.add"/>
</cc:href>
<br/>
<span style="text-indent:25px">
<cc:helpinline>
    <cc:text name="addLibraryHelpText" bundleID="samBundle"/>
</cc:helpinline>
</span>
<cc:spacer name="spacer1" newline="true" height="30"/>

<cc:image name="asteriskImg" defaultValue="/samqfsui/images/asterisk.gif"/>
<cc:href name="importHref" bundleID="samBundle"
         onClick="return importTapeVolumes()"
         extraHtml="style=\"font-size:1.25em\"">
    <cc:text name="importText" bundleID="samBundle" defaultValue="firsttime.tapevsn.import"/>
</cc:href>
<br/>
<span style="text-indent:25px">
<cc:helpinline>
<cc:text name="importHelpText" bundleID="samBundle" />
</cc:helpinline>
</span>
<cc:spacer name="spacer2" newline="true" height="30"/>

<cc:image name="asteriskImg" defaultValue="/samqfsui/images/asterisk.gif"/>
<cc:href name="diskVolumeHref" bundleID="samBundle"
         onClick="return addDiskVolume();"
         extraHtml="style=\"font-size:1.25em\"">
    <cc:text name="diskVolumeText" bundleID="samBundle" defaultValue="firsttime.diskvsn.add"/>
</cc:href>
<br/>
<span style="text-indent:25px">
<cc:helpinline>
<cc:text name="diskVolumeHelpText" bundleID="samBundle" />
</cc:helpinline>
</span>
<cc:spacer name="spacer3" newline="true" height="30"/>

<cc:href name="poolHref" bundleID="samBundle"
         onClick="return addVolumePool();"
         extraHtml="style=\"font-size:1.25em\"">
    <cc:text name="poolText" bundleID="samBundle" defaultValue="firsttime.pools.create"/>
</cc:href>
<span style="text-indent:25px">
<cc:helpinline>
<cc:text name="poolHelpText" bundleID="samBundle" />
</cc:helpinline>
</span>
<cc:spacer name="spacer4" newline="true" height="30"/>
</div>

<cc:label name="osd" bundleID="samBundle" defaultValue="firsttime.osd"/>
<br/>
<div style="margin-left:20px">
<cc:href name="createFSHref" bundleID="samBundle"
         onClick="return createFileSystem();"
         extraHtml="style=\"font-size:1.25em\"">
    <cc:text name="createFSText" bundleID="samBundle" defaultValue="firsttime.fs.create"/>
</cc:href>

<cc:spacer name="spacer4" newline="true" height="30"/>

<cc:href name="rpsHref" bundleID="samBundle"
         onClick="return scheduleRecoveryPoint();"
         extraHtml="style=\"font-size:1.25em\"">
<cc:text name="rpsText" bundleID="samBundle"defaultValue="firsttime.recoverypoint.schedule"/>
</cc:href>
<cc:spacer name="spacer4" newline="true" height="30"/>
</div>

<cc:label name="email" bundleID="samBundle" defaultValue="firsttime.emailalerts"/>
<br/>

<div style="margin-left:20px">
<cc:href name="emailHref" bundleID="samBundle"
         onClick="return setEmailAlerts();"
         extraHtml="style=\"font-size:1.25em\"">
    <cc:text name="emailText" bundleID="samBundle" defaultValue="firsttime.alerts.define"/>
</cc:href>
</div>
</div>

<div style="margin:20px">
<img src="/samqfsui/images/dot.gif" height="1" width="100%" class="ConLin"/>
</div>

<div style="display:none">
<jato:containerView name="FirstTimeConfigView">
    <samqfs:wizardwindow name="addLibraryWizard" />
    <samqfs:wizardwindow name="newFileSystemWizard" />
</jato:containerView>
</div>


<cc:hidden name="serverName"/>
</cc:pagetitle>
</jato:form>
</cc:header>
</jato:useViewBean>

