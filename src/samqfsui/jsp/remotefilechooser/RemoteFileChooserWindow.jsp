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

// ident	$Id: RemoteFileChooserWindow.jsp,v 1.10 2008/12/16 00:10:50 am143972 Exp $
--%>
<%@ page language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%> 
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>
<%@taglib uri="samqfs.tld" prefix="sam"%>

<%
    String srcName = (request.getSession().getAttribute("srcName") != null)
	? (String) request.getSession().getAttribute("srcName")
	: "";

    String srcAlt = (request.getSession().getAttribute("srcAlt") != null)
	? (String) request.getSession().getAttribute("srcAlt")
	: "";
%>

<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.remotefilechooser.RemoteFileChooserWindowViewBean">

<!-- Header -->
<cc:header name="Header"
     pageTitle="filechooser.title"
     copyrightYear="2006"
     baseName="com.sun.web.ui.resources.Resources"
     bundleID="tagBundle"
     onLoad="checkAndClose();"
     preserveFocus="true"
     preserveScroll="true"
     isPopup="true">

<script type="text/javascript">
    function checkAndClose() {
        size = window.opener.document.forms[0].elements.length;
        numElements = document.remoteFileChooserForm.elements.length;
        var returnValFieldName = 
            document.remoteFileChooserForm.elements['RemoteFileChooserWindow.fieldname'].value;
        var fileListObj = document.remoteFileChooserForm.elements['RemoteFileChooserWindow.filelist'];
        var fileList = fileListObj.value;
        var parentRefreshCmdObj = document.remoteFileChooserForm.elements['RemoteFileChooserWindow.parentRefreshCmd'];
        var parentRefreshCmd = parentRefreshCmdObj.value;
        var onCloseScript = document.remoteFileChooserForm.elements['RemoteFileChooserWindow.onCloseScript'].value;

        for (i=0; i < size; i++) {
            if (fileList == "") {
                break;
            }
            if (window.opener.document.forms[0].elements[i].name == returnValFieldName) {
                // Put chosen stuff in parent form
                window.opener.document.forms[0].elements[i].value = fileList;
                fileListObj.value = "";

                // Run client side script in parent window
                if (onCloseScript != "") {
                    eval(onCloseScript);
                }
                                
                // If a parent refresh command was provided, then refresh the parent page
                // otherwise, just close the window.
                if (parentRefreshCmd != "") {
                    window.opener.document.forms[0].action = 
                        window.opener.document.forms[0].action + "?" + parentRefreshCmd;
                    window.opener.document.forms[0].submit();
                }
                window.close();
            }
        }
    }
</script>

<cc:form name="remoteFileChooserForm" method="post">

<!-- Masthead -->
<cc:secondarymasthead name="Masthead" src="<%=srcName %>" alt="<%=srcAlt %>" />

<!-- Alert -->
<cc:alertinline name="Alert" bundleID="tagBundle" />

<cc:pagetitle name="PageTitle" bundleID="tagBundle"
    pageTitleText="filechooser.title"
    showPageTitleSeparator="true"
    showPageButtonsTop="false"
    showPageButtonsBottom="true">

<sam:remotefilechooser name="FileChooser" />

<!-- hidden field to set the list of files selected -->
<cc:hidden name="filelist" />
<cc:hidden name="fieldname" />
<cc:hidden name="parentRefreshCmd" />
<cc:hidden name="onCloseScript" />
<script type="text/javascript">
    if (!is_ie) {
        // except in ie, onload call to checkAndClose doesn't close win, call again
        checkAndClose();
    }
</script>

</cc:pagetitle>
</cc:form>
</cc:header>
</jato:useViewBean>
