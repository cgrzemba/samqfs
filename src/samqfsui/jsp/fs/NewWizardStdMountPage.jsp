
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

// ident	$Id: NewWizardStdMountPage.jsp,v 1.10 2008/12/16 00:10:46 am143972 Exp $

--%>
<%@ page language="java" %> 
<%@ page import="com.iplanet.jato.view.ViewBean" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%> 
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<script type="text/javascript">
  function wizardPageInit() {
      var disabled = false;
        var tf = document.wizWinForm;
        var string = tf.elements["WizardWindow.Wizard.NewWizardStdMountView.errorOccur"];
      var error = string.value;
      if (error == "exception")
        disabled = true;
      WizardWindow_Wizard.setNextButtonDisabled(disabled, null);
      WizardWindow_Wizard.setPreviousButtonDisabled(disabled, null);
   }

    function customNextClicked() {
        // display the disk discovery message
        var theForm = document.wizWinForm;
        var message = theForm.elements["WizardWindow.Wizard.NewWizardStdMountView.discoveryMessage"].value;
        setTimeout(alert(message), 7000);

        return true;
    }
   WizardWindow_Wizard.pageInit = wizardPageInit;
   WizardWindow_Wizard.nextClicked = customNextClicked;
</script>

<jato:pagelet>

<cc:i18nbundle id="samBundle"
 baseName="com.sun.netstorage.samqfs.web.resources.Resources" />

<%-- For now assume we're still presenting the components in a table
     which is output by the framework and components are
     in rows and cells
--%>

<div align="right">
  <cc:label name="Label" bundleID="samBundle"
    defaultValue="page.required" styleLevel="3"
    showRequired="true" />
</div>

<tr>
  <td>
    <cc:alertinline name="Alert" bundleID="samBundle" />
  </td>
</tr>

<tr>
  <td valign="top" align="left" rowspan="1" colspan="1">
    <cc:label name="Label" defaultValue="FSWizard.new.mountlabel"
        bundleID="samBundle" showRequired="true"/>
  </td>
  <td valign="center" align="left" rowspan="1" colspan="1">
    <cc:textfield name ="mountValue" bundleID="samBundle" maxLength="127"/>
  </td>
</tr>

<tr>
  <td></td>
  <td>
    <cc:helpinline type="field">
        <cc:text name="createText" bundleID="samBundle"
            defaultValue="samqfsui.fs.wizards.new.mountPage.createMountLabel" />
    </cc:helpinline>
  </td>
</tr>

<tr>
  <td></td>
  <td>
    <cc:checkbox name="bootTimeCheckBox"
      bundleID="samBundle"
      styleLevel="3"
      label="samqfsui.fs.wizards.new.mountPage.mountBootLabel"
    />
  </td>
</tr>

<tr>
  <td></td>
  <td>
    <cc:checkbox name="readOnlyCheckBox"
      bundleID="samBundle"
      styleLevel="3"
      label="samqfsui.fs.wizards.new.mountPage.readOnlyLabel"
    />
  </td>
</tr>

<tr>
  <td></td>
  <td>
    <cc:checkbox name="noSetUIDCheckBox"
      bundleID="samBundle"
      styleLevel="3"
      label="samqfsui.fs.wizards.new.mountPage.noSetUIDLabel"
    />
  </td>
</tr>

<tr>
  <td></td>
  <td>
    <cc:checkbox name="mountAfterCreateCheckBox"
      bundleID="samBundle"
      styleLevel="3"
      label="samqfsui.fs.wizards.new.mountPage.mountAfterCreateLabel"
    />
  </td>
</tr>
<cc:hidden name="errorOccur" />
<cc:hidden name="discoveryMessage" />
</jato:pagelet>
