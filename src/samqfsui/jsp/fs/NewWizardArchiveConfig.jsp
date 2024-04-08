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

// ident	$Id: NewWizardArchiveConfig.jsp,v 1.11 2008/12/16 00:10:46 am143972 Exp $
--%>

<%@page info="NewWizardArchiveConfig" language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%> 
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<style>
    td.indent{text-indent: 30px}
</style>

<script language="javascript" src="/samqfsui/js/samqfsui.js"></script>
<script language="javascript" src="/samqfsui/js/fs/wizards/archiveconfig.js">
</script>

<jato:pagelet>
<cc:i18nbundle
    id="samBundle"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"/>

<cc:alertinline name="Alert" bundleID="samBundle"/>

<table>
    <tr>
        <td>
            <cc:radiobutton name="policyTypeExisting" 
                            bundleID="samBundle" 
                            onClick="policyTypeChange(this);"/>
        </td>
        <td>
            <cc:dropdownmenu name="existingPolicyName"
                             bundleID="samBundle"
                             dynamic="true"/>
        </td>
    </tr>
    <tr>
        <td colspan="2">
            <cc:radiobutton name="policyTypeNew" 
                            bundleID="samBundle" 
                            onClick="policyTypeChange(this);"/>
        </td>
    </tr>
    <tr>
        <td colspan="2" style="text-indent: 20px">
           <cc:label name="policyTypeNewLabel"
                     defaultValue="FSWizard.new.archiving.new"
                     bundleID="samBundle"/>
        </td>
    </tr>
</table>

<table id="newPolicyTable">
    <tr><td colspan="3" class="indent">
            <div id="newPolicyDiv">
                <table cellspacing="10">
                    <tr><td colspan="3" class="indent">
                            <cc:label name="policyNameLabel" 
                                      bundleID="samBundle" 
                                      elementName="newPolicyName"
                                      defaultValue="FSWizard.new.archiving.policyname"/>
                            <cc:dropdownmenu name="newPolicyName" bundleID="samBundle"/>
                    </td></tr>
                    <tr><td  class="indent">
                        </td><td>
                            <cc:label name="copyOneLabel" 
                                      bundleID="samBundle"
                                      defaultValue="FSWizard.new.archiving.copyone"/>
                        </td><td>
                            <cc:checkbox name="enableCopyTwo"
                                         bundleID="samBundle"
                                         onClick="enableCopyTwoChange(this);"
                                         label="FSWizard.new.archiving.copytwo"/>
                    </td></tr>
                    
                    <tr><td class="indent">
                            <cc:label name="archiveAgeLabel"
                                      bundleID="samBundle"
                                      defaultValue="FSWizard.new.archiving.archiveage"/>
                        </td><td nowrap>
                            <cc:textfield name="archiveAgeOne" 
                                      size="5"/>
                            <cc:dropdownmenu name="archiveAgeOneUnit" bundleID="samBundle"/>
                        </td><td>
                            <cc:textfield name="archiveAgeTwo" 
                                          size="5"
                                          dynamic="true"
                                          disabled="true"/>
                            <cc:dropdownmenu name="archiveAgeTwoUnit" 
                                             dynamic="true"
                                             bundleID="samBundle"/>
                    </td></tr>
                    
                    <tr><td class="indent">
                            <cc:label name="mediaLabel"
                                      bundleID="samBundle"
                                      defaultValue="FSWizard.new.archiving.media"/>
                        </td><td>
                            <cc:dropdownmenu name="mediaOne" bundleID="samBundle"/>
                        </td><td>
                            <cc:dropdownmenu name="mediaTwo" 
                                             dynamic="true"
                                             bundleID="samBudle"/>
                    </td></tr>
                </table>
                
                <div>
                    <table cellspacing="10"><tr><td class="indent">
                    <cc:button name="showCopy3and4" 
                               bundleID="samBundle"
                               dynamic="true"
                               disabled="true"
                               onClick="return handleShowCopy3and4(this);"
                               defaultValue="FSWizard.new.archiving.copy3and4.show"/>
                    <cc:hidden name="copy3and4Status" defaultValue="hidden"/>
                    </td></tr></table>
                </div>

                <div id="copy3and4Div" style="display:none">                
                    <table cellspacing="10">

                        <tr><td  class="indent">
                        </td><td>
                            <cc:checkbox name="enableCopyThree" 
                                      bundleID="samBundle"
                                      onClick="enableCopyThreeChange(this);"
                                      label="FSWizard.new.archiving.copythree"/>
                        </td><td>
                            <cc:checkbox name="enableCopyFour"
                                         bundleID="samBundle"
                                         dynamic="true"
                                         onClick="enableCopyFourChange(this);"
                                         label="FSWizard.new.archiving.copyfour"/>
                        </td></tr>
                    
                        <tr><td class="indent">
                            <cc:label name="archiveAgeLabel"
                                      bundleID="samBundle"
                                      defaultValue="FSWizard.new.archiving.archiveage"/>
                        </td><td nowrap>
                            <cc:textfield name="archiveAgeThree" 
                                          dynamic="true"
                                          disabled="true"
                                          size="5"/>
                            <cc:dropdownmenu name="archiveAgeThreeUnit" 
                                             dynamic="true"
                                             disabled="true"
                                             bundleID="samBundle"/>
                        </td><td>
                            <cc:textfield name="archiveAgeFour" 
                                          size="5"
                                          dynamic="true"
                                          disabled="true"/>
                            <cc:dropdownmenu name="archiveAgeFourUnit" 
                                             dynamic="true"
                                             disabled="true"
                                             bundleID="samBundle"/>
                        </td></tr>
                    
                        <tr><td class="indent">
                            <cc:label name="mediaLabel"
                                      bundleID="samBundle"
                                      defaultValue="FSWizard.new.archiving.media"/>
                        </td><td>
                            <cc:dropdownmenu name="mediaThree"
                                             dynamic="true"
                                             disabled="true"
                                             bundleID="samBundle"/>
                        </td><td>
                            <cc:dropdownmenu name="mediaFour" 
                                             dynamic="true"
                                             disabled="true"
                                             bundleID="samBundle"/>
                        </td></tr>
                    </table>
                </div>

                <table>    
                    <tr><td colspan="3" class="indent">
                            <cc:checkbox name="noRelease" 
                                         bundleID="samBundle"
                                         label="FSWizard.new.archiving.norelease"/>
                    </td></tr>
                </table>
            </div>
    </td></tr>
</table>

<table>
    <tr>
        <td colspan="3" style="text-indent:10px">
            <cc:label name="logFileLabel"
                      bundleID="samBundle"
                      defaultValue="FSWizard.new.archiving.logfile"/>
            <cc:textfield name="logFile" size="45"/>
        </td>
    </tr>
</table>

<cc:hidden name="errMsg"/>
<cc:hidden name="ptype"/>
<cc:hidden name="errorOccur"/>
</jato:pagelet>
