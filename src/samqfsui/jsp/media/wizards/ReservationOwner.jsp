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

// ident	$Id: ReservationOwner.jsp,v 1.11 2008/12/16 00:10:49 am143972 Exp $
--%>

<%@ page language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%> 
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<script language="javascript"
    src="/samqfsui/js/media/wizards/ReservationOwner.js">
</script>

<jato:pagelet>

<cc:i18nbundle id="samBundle"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources" />

<cc:alertinline name="Alert" bundleID="samBundle" /><br />

<table border="0" width="80%">
<tr>
    <td>
        <cc:radiobutton name="RadioButton1"
            elementId="radioowner"
            onClick="
                if (this.id == 'radioowner') {
                    setFieldEnable('owner');
                    this.form.elements[
                        'WizardWindow.Wizard.ReservationOwnerView.ownerValue'].
                        focus();
                }
            " />
    </td>

    <td align="left" nowrap>
        <cc:text name="ownerText"
            defaultValue="ReservationOwner.radio1"
            bundleID="samBundle" />
    </td>

    <td align="left" rowspan="1" colspan="1">
        <cc:textfield name="ownerValue"
            elementId="ownerValue"
            dynamic="true"
            bundleID="samBundle"
            maxLength="31" />
    </td>
</tr>

<tr>
    <td>
        <cc:radiobutton name="RadioButton2"
            elementId="radiogroup"
            onClick="
                if (this.id == 'radiogroup') {
                    setFieldEnable('group');
                    this.form.elements[
                        'WizardWindow.Wizard.ReservationOwnerView.groupValue'].
                        focus();
                }" />
    </td>

    <td align="left" nowrap>
        <cc:text
            name="groupText"
            defaultValue="ReservationOwner.radio2"
            bundleID="samBundle" />
    </td>

    <td>
        <cc:textfield
            name="groupValue"
            elementId="groupValue"
            dynamic="true"
            bundleID="samBundle"
            maxLength="31" />
    </td>
</tr>

<tr>
    <td>
        <cc:radiobutton name="RadioButton3"
            elementId="radiodir"
            onClick="
                if (this.id == 'radiodir') {
                    setFieldEnable('dir');
                    this.form.elements[
                        'WizardWindow.Wizard.ReservationOwnerView.dirValue'].
                        focus();
                }" />
    </td>

    <td align="left" nowrap>
        <cc:text name="dirText"
            defaultValue="ReservationOwner.radio3"
            bundleID="samBundle" />
    </td>
    <td colspan="3">
        <cc:textfield name="dirValue"
            elementId="dirValue"
            dynamic="true"
            bundleID="samBundle"
            maxLength="31" />
    </td>
</tr>
</table>

</jato:pagelet>
