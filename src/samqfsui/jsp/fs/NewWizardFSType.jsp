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

// ident	$Id: NewWizardFSType.jsp,v 1.2 2008/10/22 20:57:03 kilemba Exp $
--%>

<%@ page language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%> 
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<script language="javascript" src="/samqfsui/js/samqfsui.js"></script>
<script language="javascript" src="/samqfsui/js/fs/wizards/fstype.js"></script>

<jato:pagelet>

<cc:i18nbundle id="samBundle"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources" />

<cc:legend name="Legend" align="right" marginTop="10px" />

<cc:alertinline name="Alert" bundleID="samBundle" />

<table cellspacing="10">
<tr><td>
<cc:radiobutton name="QFSRadioButton" bundleID="samBundle"/>
</td></tr>

<tr><td style="text-indent:20px">
<cc:checkbox name="HAFSCheckBox"
             bundleID="samBundle"
             dynamic="true"
             onChange="return handleHAFSCheckBox(this);"
             label="FSWizard.new.fstype.hafs"/>
</td></tr>

<tr><td style="text-indent:20px">
<cc:checkbox name="archivingCheckBox"
             bundleID="samBundle"
             dynamic="true"
             onChange="return handleArchivingCheckBox(this);"
             label="FSWizard.new.fstype.archiving"/>
</td></tr>

<tr><td style="text-indent:20px">
<cc:checkbox name="sharedCheckBox"
             bundleID="samBundle"
             dynamic="true"
             onChange="return handleSharedCheckBox(this);"
             label="FSWizard.new.fstype.shared"/>
</td></tr>

<tr><td style="text-indent:40px">
<cc:checkbox name="HPCCheckBox"
             dynamic="true"
             disabled="true"
             bundleID="samBundle"
             onChange="return handleHPCCheckBox(this);"
             label="FSWizard.new.fstype.hpc"/>
</td></tr>

<tr><td style="text-indent:20px">
<cc:checkbox name="matfsCheckBox"
             bundleID="samBundle"
             onChange="return handleMATFSCheckBox(this);"
             dynamic="true"
             label="FSWizard.new.fstype.matfs"/>
</td></tr>

<tr><td>
<cc:radiobutton name="UFSRadioButton" bundleID="samBundle"/>
</td></tr>
</table>

<cc:hidden name="hasArchiveMedia"/>
<cc:hidden name="archiveMediaWarning"/>
</jato:pagelet>

