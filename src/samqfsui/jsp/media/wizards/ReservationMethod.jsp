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

// ident	$Id: ReservationMethod.jsp,v 1.7 2008/05/16 19:39:22 am143972 Exp $

--%>

<%@ page language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%> 
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:pagelet>

<cc:i18nbundle id="samBundle"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources" />

<cc:alertinline name="Alert" bundleID="samBundle" /><br />

<tr>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:checkbox name="CheckBox1"
            bundleID="samBundle"
            styleLevel="3"
            label="ReservationMethod.checkbox1" />
    </td>
</tr>

<tr>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:checkbox name="CheckBox2"
            bundleID="samBundle"
            styleLevel="3"
            label="ReservationMethod.checkbox2" />
    </td>
</tr>

<tr>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:checkbox name="CheckBox3"
            bundleID="samBundle"
            styleLevel="3"
            label="ReservationMethod.checkbox3" />
    </td>
</tr>

</jato:pagelet>
