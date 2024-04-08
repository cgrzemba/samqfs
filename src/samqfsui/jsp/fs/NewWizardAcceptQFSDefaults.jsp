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

// ident	$Id: NewWizardAcceptQFSDefaults.jsp,v 1.3 2008/12/16 00:10:46 am143972 Exp $
--%>

<%@ page language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%> 
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<script language="javascript" src="/samqfsui/js/samqfsui.js"></script>
<script>
    function wizardPageInit() {
        var disabled = false;
        var tf = document.wizWinForm;
        var string = tf.elements["WizardWindow.Wizard.NewWizardAcceptQFSDefaultsView.errorOccur"];
        // var error = string.value;
        if ((string != null) && (string.value == "exception"))
            disabled = true;
        WizardWindow_Wizard.setNextButtonDisabled(disabled, null);
        WizardWindow_Wizard.setPreviousButtonDisabled(disabled, null);
   }
   
    function customNextClicked() {
        // display the disk discovery message
        var theForm = document.wizWinForm;
        var message = theForm.elements["WizardWindow.Wizard.NewWizardAcceptQFSDefaultsView.discoveryMessage"].value;
        setTimeout(alert(message), 7000);

        return true;
    }
   WizardWindow_Wizard.pageInit = wizardPageInit;
   WizardWindow_Wizard.nextClicked = customNextClicked;
</script>

<jato:pagelet>

<cc:i18nbundle
    id="samBundle"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources" />

<cc:legend name="Legend" align="right" marginTop="10px" />

<cc:alertinline name="Alert" bundleID="samBundle" />
<br/>

<table cellspacing="10" cellpadding="10">
<tr><td>
    <cc:label name="metadataLabel"
              bundleID="samBundle"
              defaultValue="FSWizard.new.qfsdefaults.metadatastorage.label"/>
</td><td>
    <cc:text name="metadataText" bundleID="samBundle" />
</td></tr>
<tr><td>
    <cc:label name="allocationMethodLabel"
              bundleID="samBundle"
              defaultValue="FSWizard.new.qfsdefaults.allocationmethod.label"/>
</td><td>
    <cc:text name="allocationMethodText" bundleID="samBundle" />
</td></tr>
<tr><td>
    <cc:label name="blockSizeLabel"
              bundleID="samBundle"
              defaultValue="FSWizard.new.qfsdefaults.blocksize.label" />
</td><td>
    <cc:text name="blockSizeText" bundleID="samBundle" />
</td></tr>
<tr><td>
    <cc:label name="blocksPerDeviceLabel"
              bundleID="samBundle"
              defaultValue="FSWizard.new.qfsdefaults.blocksperdevice.label" />
</td><td>
    <cc:text name="blocksPerDeviceText" bundleID="samBundle" />
</td></tr>
<tr><td colspan="2">
    <cc:radiobutton name="acceptChangeRadioButton" bundleID="samBundle"/>
</td></tr>
</table>
<cc:hidden name="discoveryMessage"/>
</jato:pagelet>
