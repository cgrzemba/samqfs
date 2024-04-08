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

// ident	$Id: FileSystemSelector.jsp,v 1.6 2008/12/16 00:10:51 am143972 Exp $
--%>

<%@page info="FileSystemSelector" language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>
<%@taglib uri="samqfs.tld" prefix="samqfs"%>

<jato:useViewBean className="com.sun.netstorage.samqfs.web.util.FileSystemSelectorViewBean">

<!-- helper javascript -->
<script type="text/javascript" src="/samqfsui/js/samqfsui.js"></script>
<script type="text/javascript" src="/samqfsui/js/popuphelper.js"></script>
<script>

function doSubmit(button) {
    var theForm = button.form;
    var fsName = theForm.elements["FileSystemSelector.fileSystem"].value;
    if (fsName != null) {
        var serverName =
            opener.document.FirstTimeConfigForm.elements["FirstTimeConfig.serverName"].value;
        
        var url = "/samqfsui/fs/RecoveryPointSchedule" +
                  "?SERVER_NAME=" + serverName +
                  "&FS_NAME=" + fsName;

        opener.document.location.href = url;
        window.close();
    }
}
</script>
<!-- header -->
<cc:header pageTitle="firsttime.selector.fs.pagetitle"
           copyrightYear="2007"
           baseName="com.sun.netstorage.samqfs.web.resources.Resources"
           bundleID="samBundle"
           onLoad="initializePopup(this);">

<!-- masthead -->
<cc:secondarymasthead name="SecondaryMasthead" bundleID="samBundle"/>

<!-- the form and its contents -->
<jato:form name="FileSystemSelector" method="POST">

<!-- page title -->
<cc:pagetitle name="pageTitle"
              bundleID="samBundle"
              pageTitleText="firsttime.selector.fs.pagetitle"
              showPageTitleSeparator="true"
              showPageButtonsTop="false"
              showPageButtonsBottom="true">

<table cellspacing="10"><tr><td>
<cc:label name="fileSystemLabel"
          elementName="fileSystem"
          defaultValue="firsttime.selector.fs.label"
          bundleID="samBundle"/>
</td><td>
<cc:dropdownmenu name="fileSystem"/>
</td></tr>
</table>

</cc:pagetitle>
</jato:form>
</cc:header>
</jato:useViewBean>
