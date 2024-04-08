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

// ident	$Id: FrameNavigator.jsp,v 1.6 2008/12/16 00:10:51 am143972 Exp $
--%>
<%@ page language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>


<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.util.FrameNavigatorViewBean">
    
<script language="Javascript" src="/samqfsui/js/FrameHelper.js"></script>

<!-- Define the resource bundle -->
<cc:header
    pageTitle="Frame Navigator"
    copyrightYear="2006"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    bundleID="samBundle">
    
<cc:form name="FrameNavigatorForm" method="post">

<div class="Tab3Div">
<table>
    <tr>
        <td>
            <cc:dropdownmenu
                name="ServerMenu"
                bundleID="samBundle"
                onChange="
                    handleServerMenuOnChange(this);
                    return false;"
                type="jump"/>
        </td>
        <td valign="center">
            <cc:spacer
                name="Spacer"
                height="10"
                width="3" />
            <div class="Tab3Lnk">
                <cc:href
                    name="ManageServerHref"
                    onClick="
                        top.location.href='/samqfsui/server/ServerSelection'
                        return false;">
                    <cc:text
                        name="Text"
                        bundleID="samBundle"
                        defaultValue="manage.server" />
                </cc:href>
            </div>
        </td>
    </tr>
</table>
</div>

<br />

<cc:ctree
    name="NavigationTree"
    targetFrame="Content"
    bundleID="samBundle"/>
    
<cc:hidden name="ServerName" />
    
</cc:form>
</cc:header>
</jato:useViewBean> 
