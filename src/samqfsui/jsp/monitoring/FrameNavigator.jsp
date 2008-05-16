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

// ident	$Id: FrameNavigator.jsp,v 1.5 2008/05/16 19:39:23 am143972 Exp $
--%>
<%@ page language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.monitoring.FrameNavigatorViewBean">

    <script>

        // Use same color as the navigation tree tag
        var selectedColor  = "#CBDCAF";
        var normalColor    = "#FFFFFF";
        var highlightColor = "#F2FFE6";

        function getSelectedID() {
            var selectedID =
                document.FrameNavigatorForm.elements[
                                            "FrameNavigator.SelectedID"].value;
            if (selectedID == "") {
                // default to the first entry initially
                selectedID = "1";
            }
            return selectedID;
        }

        function handleHrefClick(pageID) {
            var serverName =
                document.FrameNavigatorForm.elements[
                                            "FrameNavigator.ServerName"].value;
            var URL = "/samqfsui/monitoring/Frame?SERVER_NAME=" + serverName +
                                                "&PAGE_ID=" + pageID;
            document.getElementById("TR_" + pageID).
                        style.backgroundColor = selectedColor;
            top.frames.Content.location.href = URL;
        }

        function yoke() {
            var selectedID = getSelectedID();
            var trObj = document.getElementById("TR_" + selectedID);
            trObj.style.backgroundColor = selectedColor;
            trObj.style.fontWeight = "bold";
            trObj.style.fontColor = "#000000";
        }

        function changeRowColor(id, mouseOver) {
            var bkgndColor = "";
            var selectedID = getSelectedID();
            if (selectedID == id) {
                bkgndColor = selectedColor;
            } else {
                if (mouseOver) {
                    bkgndColor = highlightColor;
                } else {
                    bkgndColor = normalColor;
                }
            }
            document.getElementById("TR_" + id).
                    style.backgroundColor = bkgndColor;
        }

    </script>
    
    <cc:header
        pageTitle="Monitor.link"
        copyrightYear="2007"
        baseName="com.sun.netstorage.samqfs.web.resources.Resources"
        onLoad="yoke()"
        bundleID="samBundle">
        
        <cc:form name="FrameNavigatorForm" method="post">
            <br />
            <cc:spacer name="Spacer" width="15" />
            <cc:text
                name="StaticTextTitle"
                defaultValue="Monitor.label.areatomonitor"
                escape="false"
                bundleID="samBundle"/>
            <br />
            <table border="0" cellpadding="3" width="100%" cellspacing="3">
                <tr id="TR_1" style="background-color:#FFFFFF;" 
                    onMouseOver="changeRowColor(1, true);"
                    onMouseOut="changeRowColor(1, false);">
                    <td>
                        <cc:spacer name="Spacer" width="15" />
                        <cc:href
                            name="HrefDaemons"
                            title="Monitor.page.daemons.tooltip"
                            onClick="handleHrefClick(1)"
                            bundleID="samBundle">
                            <cc:alarm
                                name="AlarmDaemons" />
                            <cc:text
                                name="StaticText"
                                defaultValue="Monitor.page.daemons"
                                bundleID="samBundle" />
                        </cc:href>
                    </td>
                </tr>
                <tr id="TR_2" style="background-color:#FFFFFF;" 
                    onMouseOver="changeRowColor(2, true);"
                    onMouseOut="changeRowColor(2, false);">
                    <td>
                        <cc:spacer name="Spacer" width="15" />
                        <cc:href
                            name="HrefFS"
                            title="Monitor.page.fs.tooltip"
                            onClick="handleHrefClick(2)"
                            bundleID="samBundle">
                            <cc:alarm
                                name="AlarmFS" />
                            <cc:text
                                name="StaticText"
                                defaultValue="Monitor.page.fs"
                                bundleID="samBundle" />
                        </cc:href>
                    </td>
                </tr>
                <tr id="TR_3" style="background-color:#FFFFFF;" 
                    onMouseOver="changeRowColor(3, true);"
                    onMouseOut="changeRowColor(3, false);">
                    <td>
                        <cc:spacer name="Spacer" width="15" />
                        <cc:href
                            name="HrefCopyFreeSpace"
                            title="Monitor.page.copies.tooltip"
                            onClick="handleHrefClick(3)"
                            bundleID="samBundle">
                            <cc:alarm
                                name="AlarmCopyFreeSpace" />
                            <cc:text
                                name="StaticText"
                                defaultValue="Monitor.page.copies"
                                bundleID="samBundle" />
                        </cc:href>
                    </td>
                </tr>
                <tr id="TR_4" style="background-color:#FFFFFF;" 
                    onMouseOver="changeRowColor(4, true);"
                    onMouseOut="changeRowColor(4, false);">
                    <td>
                        <cc:spacer name="Spacer" width="15" />
                        <cc:href
                            name="HrefLibraries"
                            title="Monitor.page.libraries.tooltip"
                            onClick="handleHrefClick(4)"
                            bundleID="samBundle">
                            <cc:alarm
                                name="AlarmLibraries" />
                            <cc:text
                                name="StaticText"
                                defaultValue="Monitor.page.libraries"
                                bundleID="samBundle" />
                        </cc:href>
                    </td>
                </tr>
                <tr id="TR_5" style="background-color:#FFFFFF;" 
                    onMouseOver="changeRowColor(5, true);"
                    onMouseOut="changeRowColor(5, false);">
                    <td>
                        <cc:spacer name="Spacer" width="15" />
                        <cc:href
                            name="HrefDrives"
                            title="Monitor.page.drives.tooltip"
                            onClick="handleHrefClick(5)"
                            bundleID="samBundle">
                            <cc:alarm
                                name="AlarmDrives" />
                            <cc:text
                                name="StaticText"
                                defaultValue="Monitor.page.drives"
                                bundleID="samBundle" />
                        </cc:href>
                    </td>
                </tr>
                <tr id="TR_6" style="background-color:#FFFFFF;" 
                    onMouseOver="changeRowColor(6, true);"
                    onMouseOut="changeRowColor(6, false);">
                    <td>
                        <cc:spacer name="Spacer" width="15" />
                        <cc:href
                            name="HrefTapeMountQueue"
                            title="Monitor.page.tapemountqueue.tooltip"
                            onClick="handleHrefClick(6)"
                            bundleID="samBundle">
                            <cc:alarm
                                name="AlarmTapeMountQueue" />
                            <cc:text
                                name="StaticText"
                                defaultValue="Monitor.page.tapemountqueue"
                                bundleID="samBundle" />
                        </cc:href>
                    </td>
                </tr>
                <tr id="TR_7" style="background-color:#FFFFFF;" 
                    onMouseOver="changeRowColor(7, true);"
                    onMouseOut="changeRowColor(7, false);">
                    <td>
                        <cc:spacer name="Spacer" width="15" />
                        <cc:href
                            name="HrefQuarantinedVSNs"
                            title="Monitor.page.quarantinedvsn.tooltip"
                            onClick="handleHrefClick(7)"
                            bundleID="samBundle">
                            <cc:alarm
                                name="AlarmQuarantinedVSNs" />
                            <cc:text
                                name="StaticText"
                                defaultValue="Monitor.page.quarantinedvsn"
                                bundleID="samBundle" />
                        </cc:href>
                    </td>
                </tr>
                <tr id="TR_8" style="background-color:#FFFFFF;" 
                    onMouseOver="changeRowColor(8, true);"
                    onMouseOut="changeRowColor(8, false);">
                    <td>
                        <cc:spacer name="Spacer" width="15" />
                        <cc:href
                            name="HrefArCopyQueue"
                            title="Monitor.page.arcopyqueue.tooltip"
                            onClick="handleHrefClick(8)"
                            bundleID="samBundle">
                            <cc:alarm
                                name="AlarmArCopyQueue" />
                            <cc:text
                                name="StaticText"
                                defaultValue="Monitor.page.arcopyqueue"
                                bundleID="samBundle" />
                        </cc:href>
                    </td>
                </tr>
                <tr id="TR_9" style="background-color:#FFFFFF;" 
                    onMouseOver="changeRowColor(9, true);"
                    onMouseOut="changeRowColor(9, false);">
                    <td>
                        <cc:spacer name="Spacer" width="15" />
                        <cc:href
                            name="HrefStagingQueue"
                            title="Monitor.page.stagingqueue.tooltip"
                            onClick="handleHrefClick(9)"
                            bundleID="samBundle">
                            <cc:alarm
                                name="AlarmStagingQueue" />
                            <cc:text
                                name="StaticText"
                                defaultValue="Monitor.page.stagingqueue"
                                bundleID="samBundle" />
                        </cc:href>
                    </td>
                </tr>
            </table>
            
            <cc:hidden name="ServerName" />
            <cc:hidden name="SelectedID" />
        </cc:form>
    </cc:header>
</jato:useViewBean> 
