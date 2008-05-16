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

// ident	$Id: MonitoringFrameFormat.jsp,v 1.5 2008/05/16 19:39:23 am143972 Exp $
--%>
<%@ page language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>


<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.monitoring.MonitoringFrameFormatViewBean">

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

<script language="Javascript">
    // Initial content page id, set to daemons page ID = 1
    var PAGE_INITIAL = 1;

    // variable to hold javascript refresh interval ID
    var refreshIntervalID = -1;

    var currentNodeID = -1;

    function refresh_frame(frameName) {
        parent.frames[frameName].location.reload();
    }

    // nodeID has to be the same as what is defined in
    // monitoring/FrameNavigator.java
    function dashboard_setSelectedNode(nodeID) {
        currentNodeID = nodeID;

        // now refresh the frame header and set the content page name
        refresh_frame("FrameHeader");
        refresh_frame("FrameNavigator");
    }
    
    function setRefreshRate(value) {
        if (value > 0) {
            if (refreshIntervalID != -1) {
                clearInterval(refreshIntervalID);
            }
            refreshIntervalID =
                setInterval("refresh_frame('Content')", (value*1000));
        } else {
            clearInterval(refreshIntervalID);
        }
    }

    function writeFrameEntry(
        frameID, scrolling, marginwidth, marginheight, passed) {

        // Pass all the request into each of the frame URL
        // serverName is a part of this request
        
        document.write(
            '<frame name="' + frameID + '" scrolling="' + scrolling + '"');
        document.write('marginwidth="' + marginwidth + '"');
        document.write('marginheight="' + marginheight + '"');

        if (frameID == "Content") {
            document.write(
                'src="/samqfsui/monitoring/Frame?' + passed
                + '&PAGE_ID=' + PAGE_INITIAL + '">');
        } else if (frameID == "FrameNavigator") {
            document.write(
                'src="/samqfsui/monitoring/FrameNavigator?' + passed + '">');
        } else if (frameID == "FrameHeader") {
            document.write('noresize ');
            document.write(
                'src="/samqfsui/monitoring/Header?' + passed + '">');
        } else if (frameID == "FrameFooter") {
            document.write('noresize ');
            document.write(
                'src="/samqfsui/monitoring/Footer?' + passed + '">');
        } else {
            document.write('noresize ');
            document.write(
                'src="/samqfsui/monitoring/FrameMasthead?' + passed + '">');
        }
    }
    
    var passed = '';
    if (document.location.search) {
        passed = unescape(document.location.search.substring(1));
    } 

    document.open();
    document.write('<frameset  border="0" frameborder="no" rows="11%,7%,76%,6%">');
    writeFrameEntry('FrameMasthead', 'no', 0, 0, passed);
    writeFrameEntry('FrameHeader', 'no', 0, 0, passed);

    document.write(
    '<frameset border="0" cols="25%,*" frameborder="no" border="0" framespacing="3">');
    writeFrameEntry('FrameNavigator', 'auto', 5, 5, passed);
    writeFrameEntry('Content', 'auto', 0, 0, passed);
    document.write('</frameset>');
    writeFrameEntry('FrameFooter', 'no', 0, 0, passed);
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
