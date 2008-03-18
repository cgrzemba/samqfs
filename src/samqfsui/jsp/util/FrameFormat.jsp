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

// ident	$Id: FrameFormat.jsp,v 1.8 2008/03/17 14:40:40 am143972 Exp $
--%>
<%@ page language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>


<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.util.FrameFormatViewBean">

<!-- Define the resource bundle -->  
<cc:i18nbundle
    id="samBundle"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources" />

<html>
    <head>
        <title>
            <cc:text
                name="BrowserTitle"
                bundleID="samBundle" />
        </title>
    </head>
<script language="Javascript" src="/samqfsui/js/FrameHelper.js"></script>
<script language="Javascript">

    var serverName = "";
    var destination = "";

    // for monitoring console (to pop up different windows for different server)
    var consoleNumber = 0;
    
        // Figure out if user is coming from the Server Selection Page
    // If yes, set the server name and set where user wants to go

    if (document.location.search != "") {
        // Come from Server Selection Page with the Server Name,
        // DO NOT Need to fetch from FrameFormat.whichServer();
        serverName = getLocationParam(document.location.search, "SERVER_NAME");
        destination = getLocationParam(document.location.search, "DESTINATION");
    } else {
        serverName = "<cc:text name="ServerName" />";
        if (serverName == "&lt;EMPTY&gt;") {
            // GO to Server Selection Page
            window.location.href='/samqfsui/server/ServerSelection';
        }
    }

    document.open();
    document.write('<frameset  border="2" rows="105,*">');
    writeFrameEntry('FrameHeader', 'no', 0, 0, serverName, destination);

    document.write(
    '<frameset cols="20%,*" frameborder="yes" border="2" framespacing="3">');
    writeFrameEntry('FrameNavigator', 'auto', 5, 5, serverName, destination);
    writeFrameEntry('Content', 'auto', 0, 0, serverName, destination);
    document.write('</frameset>');
    document.write('</frameset>');
    document.close();
</script>

<noframes>
    <body>
        <p><cc:text
                name="Text"
                bundleID="samBundle"
                defaultValue="noframes" /></p>
    </body>
</noframes>
</html>
</jato:useViewBean> 
