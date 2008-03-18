<%--
/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

// ident	$Id: NewEditVSNPool.jsp,v 1.21 2008/03/17 14:40:31 am143972 Exp $
--%>

<%@ page info="NewEditVSNPool" language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%> 
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean className="com.sun.netstorage.samqfs.web.archive.NewEditVSNPoolViewBean">

<!-- Define the resource bundle, html, head, meta, stylesheet and body tags -->
<cc:header pageTitle="NewEditVSNPool.browserPageTitle" copyrightYear="2006"
  baseName="com.sun.netstorage.samqfs.web.resources.Resources"
  bundleID="samBundle"
    onLoad="initializePopup(this);">

<script language="javascript" src="/samqfsui/js/samqfsui.js"></script>
<script language="javascript" src="/samqfsui/js/popuphelper.js"></script>
<script language="javascript" src="/samqfsui/js/archive/vsnpools.js"></script>

<!-- Masthead -->
<cc:secondarymasthead name="SecondaryMasthead" bundleID="samBundle" />

<jato:form name="NewEditVSNPoolForm" method="post">

<!-- feedback alert -->
<cc:alertinline name="Alert" bundleID="samBundle"/>

<cc:pagetitle name="PageTitle" bundleID="samBundle"
        pageTitleText="NewEditVSNPool.pageTitle"
	showPageTitleSeparator="true"
        showPageButtonsTop="false"
        showPageButtonsBottom="true">

<br />

<div align="right">
  <cc:label name="requiredLabel" bundleID="samBundle"
    defaultValue="page.required" styleLevel="3"
    showRequired="true" />
</div>

<table border="0" cellspacing="10" width="100%">

<tr>
  <td>
    <cc:label name="nameLabel" defaultValue="NewEditVSNPool.name"
        showRequired="true"
	bundleID="samBundle" />
  </td>
  <td>
    <cc:textfield name="name" maxLength="31" />
  </td>
</tr>

<tr>
  <td>
    <cc:label name="mediaTypeLabel" defaultValue="NewEditVSNPool.mediatype"
	bundleID="samBundle" />
  </td>
  <td>
    <cc:dropdownmenu name="mediaType" type="standard" bundleID="samBundle"/>
  </td>
</tr>

<tr>
  <td colspan="2">
    <cc:label name="specifyVSNLabel" defaultValue="NewEditVSNPool.specifyvsns"
	bundleID="samBundle" />
  </td>
</tr>

<tr>
  <td valign="top" align="right" rowspan="1" colspan="1">
    <cc:radiobutton name="vsnStartEndRadio"
	elementId="vsnStartEndRadio"
	bundleID="samBundle"
    onClick="handleVSNRadio(this);"
	styleLevel="3" />
  </td>

  <td align="left" nowrap>
  <table cellspacing="0">
  <tr>
   <td align="left">  	 	
    <cc:label name="startLabel" defaultValue="NewEditVSNPool.start"
	bundleID="samBundle" />
   </td>

   <td valign="top" align="left" rowspan="1" colspan="1">
    <cc:textfield name="start" size="25" maxLength="31"
	elementId="start"
	dynamic="true"
    disabled="true"
	bundleID="samBundle" />
   </td>

   <td valign="center" align="left" rowspan="1" colspan="1">
    <cc:label name="endLabel" defaultValue="NewEditVSNPool.end"
	bundleID="samBundle" />
   </td>

   <td valign="top" align="left" rowspan="1" colspan="1">
    <cc:textfield name="end"  size="25" maxLength="31"
	elementId="end"
	dynamic="true"
    disabled="true"
	bundleID="samBundle" />
   </td>
   </tr>
   </table>
   </td>
</tr>	

<tr>
  <td valign="top" align="right" rowspan="1" colspan="1">
    <cc:radiobutton name="vsnRangeRadio"
	elementId="vsnRangeRadio"
	bundleID="samBundle"
    onClick="handleVSNRadio(this);"
	styleLevel="3" />
  </td>

  <td valign="top" align="left" nowrap>
  <table cellspacing="0">
  <tr>
  <td valign="top" align="left">	 	 	
    <cc:label name="vsnRangeLabel"
	defaultValue="NewEditVSNPool.vsnrange"
	bundleID="samBundle" />
    <cc:textfield name="vsnRange" size="50" maxLength="128" 
	elementId="vsnRange"
	dynamic="true"
    disabled="true"
	bundleID="samBundle" />
  </td>
  </tr>	

  <tr>
  <td valign="top" align="left" rowspan="1" colspan="3">
    <cc:helpinline type="field">
      <cc:text name="vsnRangeHelp"
	defaultValue="NewEditVSNPool.vsnRangeHelp"
	bundleID="samBundle" />
    </cc:helpinline>
  </td>
  </tr>
  </table>
  </td>
</tr> 	 	

</table>

</cc:pagetitle>

</jato:form>
</cc:header>
</jato:useViewBean>
