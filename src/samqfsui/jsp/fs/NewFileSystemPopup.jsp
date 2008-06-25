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

// ident	$Id: NewFileSystemPopup.jsp,v 1.1 2008/06/25 23:23:25 kilemba Exp $
--%>

<%@ page info="Index" language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>
<%@taglib uri="samqfs.tld" prefix="samqfs"%>

<jato:useViewBean className="com.sun.netstorage.samqfs.web.fs.NewFileSystemPopupViewBean">

<!-- include helper javascript -->
<script type="text/javascript" src="/samqfsui/js/samqfsui.js"></script>
<script type="text/javascript" src="/samqfsui/js/popuphelper.js"></script>
<script type="text/javascript" src="/samqfsui/js/fs/wizards/newfs.js"></script>
<style>
    #helpDiv {display:block;margin-top:5px;border-top:1px solid #CCC}
    #contentDiv {padding-top:0px;margin-top:5px;background-image:none;border-top:1px solid #CCC}
    #buttonContentDiv {text-align:right;margin:20px 10px 0px 0px}
</style>
    
<!-- Define the resource bundle, html, head, meta, stylesheet and body tags -->
<cc:header
    pageTitle="fs.newfs.popup.browsertitle"
    copyrightYear="2008"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    bundleID="samBundle">

<!-- masthead -->
<cc:secondarymasthead name="SecondaryMasthead" bundleID="samBundle"/>

<jato:form name="NewFileSystemPopupForm" method="post">

<cc:alertinline name="Alert" bundleID="samBundle" />

<!-- begin page title -->
<!--
<cc:pagetitle name="PageTitle" bundleID="samBundle"
        pageTitleText="fs.newfs.popup.title"
        showPageTitleSeparator="true"
        showPageButtonsTop="false"
        showPageButtonsBottom="false">
-->
<div>
<cc:label name="pageTittleLabel"
          styleLevel="1"
          bundleID="samBundle"
          defaultValue="fs.newfs.popup.title"/>

</div>
<!-- end page title -->

<!-- Help Section -->
<div id="helpDiv" class="WizStp">
<div style="margin:5px">
<cc:label name="helpLabel" bundleID="samBundle"
          defaultValue="fs.newfs.popup.help"/>
<cc:spacer name="spacer" height="10" newline="true"/>

<cc:label name="fstypeLabel" bundleID="samBundle"
          defaultValue="fs.newfs.popup.help.fstype.label"/>
<br/>
<cc:text name="fstypeText" bundleID="samBundle"
         defaultValue="fs.newfs.popup.help.fstype.text"/>
<cc:spacer name="spacer" height="10" newline="true"/>

<cc:label name="archivingLabel" bundleID="samBundle"
          defaultValue="fs.newfs.popup.help.archiving.label"/>
<br/>
<cc:text name="archivingText" bundleID="samBundle"
         defaultValue="fs.newfs.popup.help.archiving.text"/>
<cc:spacer name="spacer" height="10" newline="true"/>

<cc:label name="sharedLabel" bundleID="samBundle"
          defaultValue="fs.newfs.popup.help.shared.label"/>
<br/>
<cc:text name="sharedText" bundleID="samBundle"
         defaultValue="fs.newfs.popup.help.shared.text"/>
<cc:spacer name="spacer" height="10" newline="true"/>

<cc:label name="hpcLabel" bundleID="samBundle"
          defaultValue="fs.newfs.popup.help.hpc.label"/>
<br/>
<cc:text name="hpcText" bundleID="samBundle"
         defaultValue="fs.newfs.popup.help.hpc.text"/>
<cc:spacer name="spacer" height="10" newline="true"/>

<cc:label name="hpcFSNameLabel" bundleID="samBundle"
          defaultValue="fs.newfs.popup.help.fsname.label"/>
<br/>
<cc:text name="hpcFSNameText" bundleID="samBundle"
         defaultValue="fs.newfs.popup.help.fsname.text"/>
</div>     
</div>

<!-- content section -->
<div id="contentDiv" class="WizBdy">
<div style="margin:5px;margin-left:10px">
<cc:text name="instructionText" bundleID="samBundle"
         defaultValue="fs.newfs.popup.instruction"/>
<cc:spacer name="spacer" height="30" newline="true"/>

<cc:label name="fsnameLabel" bundleID="samBundle"
          defaultValue="fs.newfs.popup.fstype"/>
<cc:spacer name="spacer" height="20" newline="true"/>

<cc:radiobutton name="QFSRadioButton" bundleID="samBundle"/>

<div style="margin-left:30px">
<cc:checkbox name="HAFSCheckBox"
             bundleID="samBundle"
             dynamic="true"
             onChange="return handleHAFSCheckBox(this);"
             label="fs.newfs.popup.hafs"/>
<cc:spacer name="spacer" height="20" newline="true"/>

<cc:checkbox name="archivingCheckBox"
             bundleID="samBundle"
             dynamic="true"
             onChange="return handleArchivingCheckBox(this);"
             label="fs.newfs.popup.archiving"/>
<cc:spacer name="spacer" height="20" newline="true"/>

<cc:checkbox name="sharedCheckBox"
             bundleID="samBundle"
             onChange="return handleSharedCheckBox(this);"
             label="fs.newfs.popup.shared"/>

<div style="margin:20px;margin-left:30px">
<cc:checkbox name="HPCCheckBox"
             dynamic="true"
             disabled="true"
             bundleID="samBundle"
             onChange="return handleHPCCheckBox(this);"
             label="fs.newfs.popup.hpc"/>
<div style="margin:20px;margin-left:30px">
<cc:label name="hpcFSNameLabel"
          bundleID="samBundle"						
          elementName="FSName"
          defaultValue="fs.newfs.popup.hpc.fsname"/> 
<cc:textfield name="FSName" dynamic="true" disabled="true"/>         
</div>
</div>
</div>
<cc:radiobutton name="UFSRadioButton" bundleID="samBundle"/>
</div>
</div>

<!-- buttons section -->
<div id="buttonDiv" class="WizBtn">
<div style="margin-right:10px">
<img src="/samqfsui/images/dot.gif" height="1" width="100%" class="ConLin"/>
</div>

<div id="buttonContentDiv">
    <cc:button name="Cancel"
               bundleID="samBundle"
               onClick="return handleNewFileSystemPopupCancel();"
               defaultValue="common.button.cancel"/>

    <cc:spacer name="spacer" width="10"/>
    <cc:button name="Ok"
               bundleID="samBundle"
               onClick="return handleNewFileSystemPopupOk();"
               defaultValue="common.button.ok"/>
</div>
</div>
</cc:pagetitle>
<cc:hidden name="hasArchiveMedia"/>
<cc:hidden name="serverName"/>
<cc:hidden name="messages" />
<cc:hidden name="existingFSNames"/>

<!-- wizard -->
<div style="display:none">
<jato:containerView name="NewFileSystemPopupView">
    <samqfs:wizardwindow name="newFileSystemWizard"/>
</jato:containerView>
</div>
</jato:form>
</cc:header>
</jato:useViewBean>

