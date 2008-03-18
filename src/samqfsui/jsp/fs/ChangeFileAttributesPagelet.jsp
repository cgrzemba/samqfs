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

// ident	$Id: ChangeFileAttributesPagelet.jsp,v 1.5 2008/03/17 14:40:32 am143972 Exp $ 
--%>

<%@ page info="Index" language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:pagelet>

<cc:i18nbundle
    id="samBundle"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"/>

<cc:alertinline name="Alert" bundleID="samBundle" />

<cc:pagetitle
    name="PageTitle" 
    bundleID="samBundle"
    pageTitleText="<insert title>"
    showPageTitleSeparator="true"
    showPageButtonsTop="false"
    showPageButtonsBottom="true">

<br />

<table style="margin-left:10px">
<tr>
    <td colspan="2">
        <cc:checkbox
            name="Recursive"
            styleLevel="2"
            label="fs.filedetails.recursive"
            bundleID="samBundle"/>
        <br />
    </td>
</tr>
<tr>
    <td colspan="2">
        <cc:radiobutton
            name="Radio"
            styleLevel="2"
            bundleID="samBundle"
            onClick="handleRadioSelection(this)"/>
    </td>
</tr>
<tr>
    <td rowspan="2">
        <cc:spacer
            name="Spacer"
            width="20"
            height="20" />
    </td>
    <td>
        <cc:radiobutton
            name="SubRadio"
            styleLevel="3"
            dynamic="true"
            bundleID="samBundle"/>
    </td>
</tr>
<tr>
    <td>
        <div id="releaseDiv">
            <cc:checkbox
                name="PartialRelease"
                bundleID="samBundle"
                label="fs.filedetails.releasing.partial"
                dynamic="true"
                onClick="handlePartialRelease(this)"
                styleLevel="3" />
            <cc:spacer
                name="Spacer"
                width="5"
                height="2" />
            <cc:label
                name="Label"
                bundleID="samBundle"
                defaultValue="fs.filedetails.releasing.partial.size" />
            <cc:textfield
                name="PartialReleaseSize"
                size="5"
                maxLength="5"
                dynamic="true"
                disabled="true"
                bundleID="samBundle" />
            <br />
            <cc:spacer
                name="Spacer"
                width="5"
                height="2" />
            <cc:helpinline type="field">
                <cc:text
                    name="HelpText"
                    defaultValue="fs.filedetails.releasing.releaseSize.help"
                    bundleID="samBundle" />
            </cc:helpinline>
        </div>
    </td>
</tr>

<tr>
    <td colspan="2">
        <br />
        <cc:button
            name="Submit"
            bundleID="samBundle" 
            defaultValue="common.button.submit"
            type="primary" />
    </td>
</tr>
</table>


</cc:pagetitle>

</jato:pagelet>
