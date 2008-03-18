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

// ident	$Id: CopyVSNs.jsp,v 1.13 2008/03/17 14:40:30 am143972 Exp $
--%>

<%@page info="CopyVSNs" language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato" %>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean className="com.sun.netstorage.samqfs.web.archive.CopyVSNsViewBean">

<!-- include helper javascript -->
<script type="text/javascript" src="/samqfsui/js/archive/copyvsns.js"></script>

<!-- tab everything by 30  px -->
<style>
    td.indent{padding-left:30px; text-align:left}
</style>

<!-- page header -->
<cc:header
    pageTitle="archiving.policy.copyvsns.headertitle" 
    copyrightYear="2006" 
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    onLoad="
        if (parent.serverName != null) {
            parent.setSelectedNode('12', 'CopyVSNs');
        }"
    bundleID="samBundle">

<!-- the form -->
<jato:form name="CopyVSNsForm" method="POST">

<!-- bread crumbing -->
<cc:breadcrumbs name="BreadCrumb" bundleID="samBundle" />

<!-- alert for feedback -->
<cc:alertinline name="Alert" bundleID="samBundle"/> <br />

<!-- page title -->
<cc:pagetitle name="PageTitle"
              bundleID="samBundle"
              pageTitleText="archiving.policy.copyvsns.pagetitle"
              showPageTitleSeparator="true"
              showPageButtonsTop="true"
              showPageButtonsBottom="true">

<!-- body -->
<cc:spacer name="spacer" height="10" width="1"/>

<table cellspacing="10"> 
    <tr><td class="indent">
        <cc:label name="mediaTypeLabel" 
                  defaultValue="archiving.mediatype"
                  bundleID="samBundle"
                  elementName="mediaType"/>
    </td><td>
        <cc:dropdownmenu name="mediaType"
                         bundleID="samBundle"
                         onChange="handleMediaTypeChange(this)"/>
    </td></tr>
    
    <tr><td class="indent">
        <cc:label name="vsnsDefinedLabel"
                  defaultValue="archiving.vsnsdefined"
                  bundleID="samBundle"
                  elementName="vsnsDefined"/>
    </td><td>
        <cc:textfield name="vsnsDefined"
                      bundleID="samBundle"
                      size="40"/>
    </td></tr>
    
    <tr><td class="indent">
        <cc:label name="poolDefinedLabel"
                  defaultValue="archiving.pooldefined"
                  bundleID="samBundle"
                  elementName="poolDefined"/>
    </td><td>
        <cc:dropdownmenu name="poolDefined"
                        bundleID="samBundle"/>
    </td></tr>
    <tr><td colspan=2" class="indent">

        <cc:spacer name="spacer1" height="20"/>
        <img src="/com_sun_web_ui/images/other/dot.gif"
             height="1" width="100%" class="ConLin"/>
        </td>
    </tr>
    
    <tr><td class="indent">
        <cc:label name="freeSpaceLabel"
                  defaultValue="archiving.freespace"
                  bundleID="samBundle"
                  elementName="freeSpace"/>
    </td><td>
        <cc:text name="freeSpace"
                 bundleID="samBundle"/>
    </td></tr>
    
    <tr><td class="indent">
        <cc:label name="availableMembersLabel"
                  defaultValue="archiving.availablemembers"
                  bundleID="samBundle"
                  elementName="availableMembers"/>
    </td><td>
        <cc:text name="availableMembers"
                 bundleID="samBundle"/>
    </td></tr>
</table>
<br>
<span style="padding-left:20px">
<cc:text name="message"
         bundleID="samBundle"/>
</span>

<cc:hidden name="hardReset"/>
<cc:hidden name="all_pools"/>

</cc:pagetitle>
</jato:form>
</cc:header>
</jato:useViewBean>
