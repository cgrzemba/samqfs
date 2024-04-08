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

// ident	$Id: Footer.jsp,v 1.4 2008/12/16 00:10:50 am143972 Exp $
--%>

<%@ page language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.monitoring.FooterViewBean">
    
<cc:header
    pageTitle="" 
    copyrightYear="2007"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    bundleID="samBundle">

<table border="0" width="100%">
    <tr>
        <td align="right" valign="top">
            <cc:button name="CloseButton"
                       defaultValue="common.button.close"
                       bundleID="samBundle"
                       onClick="top.close();" />
        </td>
    </tr>
</table>
</cc:header>
</jato:useViewBean> 
