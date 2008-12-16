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

// ident	$Id: AddLibraryNetworkSelectName.jsp,v 1.11 2008/12/16 00:10:49 am143972 Exp $

--%>
<%@ page language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%> 
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:pagelet>

<cc:i18nbundle id="samBundle"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources" />

<cc:alertinline name="Alert" bundleID="samBundle" /><br />
<cc:legend name="Legend" align="right" marginTop="10px" />

<table border="0">
    <tr>
        <td colspan="3">&nbsp;</td>
    </tr>
    <tr>
        <td valign="center" align="left" rowspan="1" colspan="1">
            <cc:label name="Label"
                defaultValue="AddLibrary.label.name"
                bundleID="samBundle"
                showRequired="true" />
        </td>
        <td valign="center" align="left" rowspan="1" colspan="1">
            <cc:textfield name ="nameValue" 
                size="20" 
                maxLength="31"
                bundleID="samBundle"/>
        </td>
    </tr>
    <tr>
        <td valign="center" align="left" rowspan="1" colspan="1">
            <cc:label name="Label"
                defaultValue="AddLibrary.label.paramfile"
                bundleID="samBundle"
                showRequired="true" />
        </td>
        <td valign="center" align="left" rowspan="1" colspan="1">
            <cc:textfield name ="paramValue"
                size="40"
                maxLength="127"
                bundleID="samBundle"/>
        </td>
    </tr>
</table>
</jato:pagelet>
