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

// ident	$Id: ImportVSN.jsp,v 1.8 2008/03/17 14:40:36 am143972 Exp $
--%>

<%@ page info="Index" language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.media.ImportVSNViewBean">
    
    <script language="javascript"
            src="/samqfsui/js/media/ImportVSN.js"></script>
    
    <!-- Define the resource bundle, html, head, meta, stylesheet and body tags -->
    <cc:header
        pageTitle="ImportVSN.browsertitle"
        copyrightYear="2006"
        baseName="com.sun.netstorage.samqfs.web.resources.Resources"
        onLoad="
        if (parent.serverName != null) {
        parent.setSelectedNode('21', 'ImportVSN');
        }
        toggleDisabledState();"
        bundleID="samBundle">
        
        <jato:form name="ImportVSNForm" method="post">
            
            <!-- Bread Crumb component -->
            <cc:breadcrumbs name="BreadCrumb" bundleID="samBundle" />
            
            <cc:alertinline name="Alert" bundleID="samBundle" />
            
            <cc:pagetitle
                name="PageTitle"
                bundleID="samBundle"
                pageTitleText="ImportVSN.pagetitle"
                showPageTitleSeparator="true"
                showPageButtonsTop="false"
                showPageButtonsBottom="true">
                
                <br />
                
                <table>
                    <tr>
                        <td rowspan="9">
                            <cc:spacer name="spacer" height="1" width="5"/>
                        </td>
                        <td>
                            <cc:text
                                name="StaticText"
                                defaultValue="ImportVSN.text.filterby"
                                escape="false"
                                bundleID="samBundle" />
                            <cc:spacer name="spacer" height="5" width="5"/>
                        </td>
                    </tr>
                    <tr>
                        <td colspan="6">
                            <cc:radiobutton
                                name="None"
                                bundleID="samBundle"
                                elementId="none"
                                onClick="
                                if (this.id == 'none') {
                                setFieldEnable('none');
                            }"/>
                        </td>
                    </tr>
                    <tr>
                        <td colspan="3">
                            <cc:radiobutton
                                name="ScratchPool"
                                elementId="pool"
                                bundleID="samBundle"
                                onClick="
                                if (this.id == 'pool') {
                                setFieldEnable('pool');
                                this.form.elements[
                                'ImportVSN.ScratchPoolMenu'].
                                focus();
                            }" />
                        </td>
                        <td colspan="3">
                            <cc:dropdownmenu
                                name="ScratchPoolMenu"
                                dynamic="true"
                                disabled="true"
                                bundleID="samBundle"
                                type="standard"/>
                        </td>
                    </tr>
                    <tr>
                        <td>
                            <cc:radiobutton
                                name="VSNRange"
                                elementId="range"
                                bundleID="samBundle"
                                onClick="
                                if (this.id == 'range') {
                                setFieldEnable('range');
                                this.form.elements[
                                'ImportVSN.StartField'].
                                focus();
                            }" />
                        </td>
                        <td rowspan="4">
                            <cc:spacer name="Spacer" width="3" height="1" />
                        </td>
                        <td align="right">
                            <cc:text
                                name="StaticText"
                                defaultValue="ImportVSN.text.start"
                                bundleID="samBundle" />
                        </td>
                        <td>
                            <cc:textfield
                                name="StartField"
                                dynamic="true"
                                disabled="true"
                                maxLength="6"
                                size="7"/>
                        </td>
                        <td align="right">
                            <cc:text
                                name="StaticText"
                                defaultValue="ImportVSN.text.end"
                                bundleID="samBundle" />
                        </td>
                        <td>
                            <cc:textfield
                                name="EndField"
                                dynamic="true"
                                disabled="true"
                                maxLength="6"
                                size="7" />
                        </td>
                    </tr>
                    <tr>
                        <td colspan="3">
                            <cc:radiobutton
                                name="RegEx"
                                elementId="regex"
                                bundleID="samBundle"
                                onClick="
                                if (this.id == 'regex') {
                                setFieldEnable('regex');
                                this.form.elements[
                                'ImportVSN.RegExField'].
                                focus();
                            }" />
                        </td>
                        <td colspan="3">
                            <cc:textfield
                                name="RegExField"
                                size="50"
                                dynamic="true"
                                disabled="true" />
                        </td>
                    </tr>
                    <tr>
                        <td>
                            <br />
                            <cc:button
                                name="FilterButton"
                                defaultValue="ImportVSN.button.filter"
                                bundleID="samBundle" />
                        </td>
                    </tr>
                </table>
                
                <br />
                
                <cc:includepagelet name="ImportVSNView"/>
                
            </cc:pagetitle>
        </jato:form>
    </cc:header>
    
</jato:useViewBean> 
