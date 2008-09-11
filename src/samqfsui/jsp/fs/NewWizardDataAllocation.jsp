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

// ident	$Id: NewWizardDataAllocation.jsp,v 1.1 2008/09/11 05:28:50 kilemba Exp $
--%>

<%@ page language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%> 
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<script language="javascript" src="/samqfsui/js/samqfsui.js"></script>
<script language="javascript" src="/samqfsui/js/fs/wizards/blockallocation.js">
</script>

<jato:pagelet>

<cc:i18nbundle id="samBundle"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources" />

<cc:legend name="Legend" align="right" marginTop="10px" />

<cc:alertinline name="Alert" bundleID="samBundle" />
<br/>
<table cellspacing="10" cellpadding="10">
<tr><td>
    <cc:label name="objectGroupLabel"
              bundleID="samBundle"
              defaultValue="FSWizard.new.dataallocation.groups"/>
</td><td>
    <cc:textfield name="objectGroupText" size="3"/>
</td></tr>

<tr><td>
    <cc:label name="blockSizeLabel"
              bundleID="samBundle"
              defaultValue="FSWizard.new.blockallocation.blocksize"/>
</td><td nowrap>
    <div id="blocksizetext">
    <cc:textfield name="blockSizeText" size="3" />
    <cc:dropdownmenu name="blockSizeUnit" bundleID="samBundle" />
    </div>
</td></tr>

<tr><td>
    <cc:label name="blocksPerDeviceLabel"
              bundleID="samBundle"
              defaultValue="FSWizard.new.blockallocation.blocksperdevice" />
</td><td>
    <cc:textfield name="blocksPerDeviceText" size="3" />
</td></tr>
</table>
</jato:pagelet>
