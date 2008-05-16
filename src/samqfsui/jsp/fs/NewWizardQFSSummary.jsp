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

// ident	$Id: NewWizardQFSSummary.jsp,v 1.22 2008/05/16 19:39:20 am143972 Exp $

--%>

<%@ page language="java" %> 
<%@ page import="com.iplanet.jato.view.ViewBean" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%> 
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:pagelet>

<cc:i18nbundle id="samBundle"
 baseName="com.sun.netstorage.samqfs.web.resources.Resources" />

<!-- inline alart -->
<cc:alertinline name="Alert" bundleID="samBundle" />
<br>

<table border="0" cellspacing="10" cellpadding="0">
    <tbody>
        <tr>
            <td>
                <cc:label name="Label" styleLevel="2"
                   elementName="fsNameValue"
                   defaultValue="FSWizard.new.fsnameLabel"
                   bundleID="samBundle" />
            </td>
            <td>
                <cc:text name="fsNameValue"
                  bundleID="samBundle" />
            </td>
        </tr>
        <tr>
            <td>
                <cc:label name="Label" styleLevel="2"
                   elementName="fsTypeSelect"
                   defaultValue="FSWizard.new.fstype"
                   bundleID="samBundle" />
            </td>
            <td>
                <cc:text name="fsTypeSelect"
                  bundleID="samBundle" />
            </td>
        </tr>
        <tr>
            <td>
                <cc:label name="Label" styleLevel="2"
                   elementName="qfsSelect"
                   defaultValue="FSWizard.new.qfsselection"
                   bundleID="samBundle" />
            </td>
            <td>
                <cc:text name="qfsSelect"
                  bundleID="samBundle" />
            </td>
        </tr>
        
        <tr>
            <td>
                <cc:label name="Label" styleLevel="2"
                   elementName="MetadataField"
                   defaultValue="FSWizard.new.selectedmetadata"
                   bundleID="samBundle" />
            </td>
            <td>
                <cc:selectablelist name="MetadataField"
                            bundleID="samBundle" escape="false" />
            </td>
        </tr>
        
        <tr>
            <td>
                <cc:label name="Label" styleLevel="2"
                   elementName="DataField"
                   defaultValue="FSWizard.new.selectedata"
                   bundleID="samBundle" />
            </td>
            <td>
                <cc:selectablelist name="DataField"
                            bundleID="samBundle" escape="false" />
            </td>
        </tr>
        
        <tr>
            <td>
                <cc:label name="Label" styleLevel="2"
                   elementName="DAUDropDown"
                   defaultValue="FSWizard.new.daulabel"
                   bundleID="samBundle" />
            </td>
            <td>
                <cc:text name="DAUDropDown"
                  bundleID="samBundle" />
            </td>
        </tr>
        <tr>
            <td>
                <cc:label name="Label" styleLevel="2"
                   elementName="mountValue"
                   defaultValue="FSWizard.new.mountlabel"
                   bundleID="samBundle" />
            </td>
            <td>
                <cc:text name="mountValue"
                  bundleID="samBundle" />
            </td>
        </tr>
        <tr>
            <td>
                <cc:label name="Label" styleLevel="2"
                   elementName="bootTimeCheckBox"
                   defaultValue="FSWizard.new.mountBootLabel"
                   bundleID="samBundle" />
            </td>
            <td>
                <cc:text name="bootTimeCheckBox"
                  bundleID="samBundle" />
            </td>
        </tr>
        
        <tr>
            <td>
                <cc:label name="Label" styleLevel="2"
                          elementName="mountOptionCheckBox"
                          defaultValue="FSWizard.new.mountOptionLabel"
                          bundleID="samBundle" />
            </td>
            <td>
                <cc:text name="mountOptionCheckBox"
                         bundleID="samBundle" />
            </td>
        </tr>
        
        <tr>
            <td>
                <cc:label
                    name="Label"
                    styleLevel="2"
                    elementName="optimizeForOracle"
                    defaultValue="FSWizard.new.optimizeForOracle"
                    bundleID="samBundle" />
            </td>
            <td>
                <cc:text
                    name="optimizeForOracle"
                    bundleID="samBundle" />
            </td>
        </tr>
        
        <tr>
            <td>
                <cc:label name="wmLabel" styleLevel="2"
                          elementName="hwmValue"
                          defaultValue="FSWizard.new.hwmlabel"
                          bundleID="samBundle" />
            </td>
            <td>
                <cc:text name="hwmValue"
                         bundleID="samBundle" />
            </td>
        </tr>
        
        <tr>
            <td>
                <cc:label name="wmLabel" styleLevel="2"
                          elementName="lwmValue"
                          defaultValue="FSWizard.new.lwmlabel"
                          bundleID="samBundle" />
            </td>
            <td>
                <cc:text name="lwmValue"
                         bundleID="samBundle" />
            </td>
        </tr>
        
        <tr>
            <td>
                <cc:label name="Label" styleLevel="2"
                   elementName="stripeValue"
                   defaultValue="FSWizard.new.stripelabel"
                   bundleID="samBundle" />
            </td>
            <td>
                <cc:text name="stripeValue"
                  bundleID="samBundle" />
            </td>
        </tr>
        <tr>
            <td>
                <cc:label name="Label" styleLevel="2"
                   elementName="traceDropDown"
                   defaultValue="FSWizard.new.tracelabel"
                   bundleID="samBundle" />
            </td>
            <td>
                <cc:text name="traceDropDown"
                  bundleID="samBundle" />
            </td>
        </tr>
        
    </tbody>
</table>

</jato:pagelet>
