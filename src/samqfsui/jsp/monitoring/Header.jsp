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

// ident	$Id: Header.jsp,v 1.7 2008/12/16 00:10:50 am143972 Exp $
--%>

<%@ page language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.monitoring.HeaderViewBean">
    
    <script language="javascript">
    
    function handleAutoRefreshMenu(menu) {
        // desire auto refresh value in seconds
        var interval = menu.value;
        top.setRefreshRate(interval);
    }

    </script>
    
    <cc:header
        pageTitle="" 
        copyrightYear="2007"
        baseName="com.sun.netstorage.samqfs.web.resources.Resources"
        bundleID="samBundle">
        <style>
            .DefBdy{background:#EEEEEE}
        </style>
        
        <cc:form name="HeaderForm" method="post">
            
            <table border="0" width="100%">
                <tr>
                    <td align="left">
                        <cc:spacer name="Spacer" width="10" />
                        <cc:label name="Label"
                                  defaultValue="Monitor.label.server"
                                  styleLevel="1"
                                  bundleID="samBundle" />
                        <cc:spacer name="Spacer" width="2" />
                        <cc:text name="ServerName" escape="false" />
                    </td>
                    <td align="right">
                        <cc:label name="Label" bundleID="samBundle"
                                  defaultValue="Monitor.label.repeat" />
                        <cc:spacer name="Spacer" width="10" />
                        <cc:dropdownmenu
                            name="AutoRefresh"
                            bundleID="samBundle"
                            onChange="handleAutoRefreshMenu(this)"/>
                        <cc:spacer name="Spacer" width="10" />
                        <cc:button
                            name="RefreshButton"
                            bundleID="samBundle"
                            defaultValue="Monitor.button.refresh"
                            onClick="top.refresh_frame('Content');
                                     return false;"
                            title="Monitor.button.refresh.alt"
                            type="icon" />
                    </td>
                </tr>
            </table>
        </cc:form>
    </cc:header>
</jato:useViewBean> 
