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

// ident	$Id: GrowWizardQFSSummaryPage.jsp,v 1.10 2008/12/16 00:10:45 am143972 Exp $
--%>
<%@ page language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<script type="text/javascript">
    function wizardPageInit() {
        var tf = document.wizWinForm;
        var string = tf.elements[
            "WizardWindow.Wizard.GrowWizardSummaryPageView.NoneSelected"].value;
        WizardWindow_Wizard.setFinishButtonDisabled(string == "true", null);
    }
    WizardWindow_Wizard.pageInit = wizardPageInit;

</script>

<jato:pagelet>

<cc:i18nbundle id="samBundle"
               baseName="com.sun.netstorage.samqfs.web.resources.Resources" />

<cc:alertinline name="Alert" bundleID="samBundle" />

<table border="0" cellspacing="10" cellpadding="0">
    <tbody>
    <tr>
        <td valign="top">
            <cc:label name="LabelMeta"
                      styleLevel="2"
                      elementName="MetadataField"
                      defaultValue="FSWizard.grow.selectedMetadata"
                      bundleID="samBundle" />
        </td>
        <td>
            <cc:selectablelist name="MetadataField"
                               bundleID="samBundle"
                               escape="false" />
        </td>
    </tr>
    <tr>
        <td valign="top">
            <cc:label name="LabelData"
                      styleLevel="2"
                      elementName="DataField"
                      defaultValue="FSWizard.grow.selectedData"
                      bundleID="samBundle" />
        </td>
        <td>
            <cc:selectablelist name="DataField"
                               bundleID="samBundle"
                               escape="false" />
        </td>
    </tr>
    </tbody>
</table>

<cc:hidden name="NoneSelected" />

</jato:pagelet>
