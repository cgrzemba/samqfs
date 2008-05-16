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

// ident	$Id: SunPlexManagerPagelet.jsp,v 1.4 2008/05/16 19:39:20 am143972 Exp $ 
--%>

<%@page info="SunPlexManagerPagelet" language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:pagelet>
<!-- NOTE: use bundle definition in the parent page -->

<cc:i18nbundle id="samBundle"
               baseName="com.sun.netstorage.samqfs.web.resources.Resources"/>

<table cellspacing="10" style="width: 100%">
<tr><td colspan="2">
<img src="/com_sun_web_ui/images/other/dot.gif" 
     height="1" width="100%" class="ConLin"/>
</td></tr>
<tr><td colspan="2">
<cc:label name="SunPlexLabel" bundleID="samBundle" 
          defaultValue="fs.cluster.sunplex.label"/>
<cc:spacer name="spacer1" height="1" width="20"/>
<cc:href name="SunPlexManagerLink" bundleID="samBundle">
    <cc:text name="SunPlexManagerLinkText" bundleID="samBundle"/>
</cc:href>
<cc:text name="SunPlexManagerDescription" bundleID="samBundle"/>
</td></tr>
</table>


</jato:pagelet>
