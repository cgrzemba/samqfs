<%--
/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

// ident	$Id: CopyParams.jsp,v 1.1 2008/04/29 17:08:06 ronaldso Exp $

--%>
<%@ page language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<script type="text/javascript">

    function wizardPageInit() {
        WizardWindow_Wizard.setNextButtonDisabled(false, null);
        WizardWindow_Wizard.setPreviousButtonDisabled(false, null);
    }
    
    function handleMenuChange(menu) {
        var menuValue = menu.value;
        menuValue = menuValue.substring(0,7);

        if (menuValue == "##ANY##") {
            var checkBoxObj = menu.form.elements[
                                "WizardWindow.Wizard.CopyParamsView.Reserved"];
            checkBoxObj.checked = true;
        }
    }

</script>

<style>
    td.indent10px{padding-left:10px;text-align:left}
</style>

<jato:pagelet>
    <cc:i18nbundle
        id="samBundle"
        baseName="com.sun.netstorage.samqfs.web.resources.Resources"/>

    <cc:legend name="Legend" align="right" marginTop="10px" />

    <cc:alertinline name="Alert" bundleID="samBundle"/>

    <br /><br />
    <table>
    <tr>
        <td colspan="2">
            <cc:text name="StaticText"
                     defaultValue="archiving.copy.mediaparam.instruction.1"
                     bundleID="samBundle" />
            <br /><br />
        </td>
    </tr>
    <tr>
        <td valign="top">
            <cc:spacer name="Spacer" width="5" />
            <cc:label name="LabelArchiveAge"
                      bundleID="samBundle"
                      elementName="archiveAge"
                      showRequired="true"
                      defaultValue="archiving.archiveage.label"/>
        </td>
        <td>
            <cc:textfield name="archiveAge"/>
            <cc:dropdownmenu name="archiveAgeUnit"
                             bundleID="samBundle" />
            <cc:text name="StaticText"
                     defaultValue="archiving.copy.mediaparam.text.1"
                    bundleID="samBundle" />
            <br /><br />
            <br /><br />
        </td>
    </tr>
    <tr>
        <td colspan="2">
            <cc:text name="VolumeInstruction"
                     defaultValue="archiving.copy.mediaparam.instruction.2"
                     bundleID="samBundle" />
        <br /><br />
    </tr>
    <tr>
        <td valign="top">
            <cc:spacer name="Spacer" width="5" />
            <cc:label name="LabelArchiveMedia"
                      bundleID="samBundle"
                      elementName="ArchiveMediaMenu"
                      defaultValue="archiving.archivemedia.label"/>
        </td>
        <td>
            <cc:dropdownmenu name="ArchiveMediaMenu"
                             escape="false"
                             onChange="handleMenuChange(this)"
                             bundleID="samBundle" />
            <br /><br />
        </td>
    </tr>
    <tr>
        <td colspan="2">
            <cc:spacer name="Spacer" width="10" />
            <cc:checkbox name="Reserved"
                         label="archiving.copy.mediaparam.reserved"
                        bundleID="samBundle" />
       </td>
    </tr>
    </table>

</jato:pagelet>
