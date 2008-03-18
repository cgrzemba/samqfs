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

// ident	$Id: FilterSettings.jsp,v 1.10 2008/03/17 14:40:40 am143972 Exp $
--%>
<%@ page language="java" %>
<%@ page import="com.iplanet.jato.view.ViewBean" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>


<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.util.FilterSettingsViewBean">

<!-- Define the resource bundle, html, head, meta, stylesheet and body tags -->
<cc:header
    name="pageHeader"
    pageTitle="FSRestore.filterSnapshot.pageTitle" 
    copyrightYear="2006"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources" 
    bundleID="samBundle"
    onLoad="init(); return checkForSubmit();">
    
<script language="javascript">

    var theFormObj;
    var theFormName;
    var lastDateRangeValue = "";

    function init() {
        // When enter is pressed, click OK
        document.onkeypress = onKeyPressEnter;
        
        // Initialize components
        theFormObj = document.FilterSettingsForm;
        theFormName = theFormObj.name;
        var dateRangeObj = theFormObj.elements["FilterSettings.fileDateRange"];
        // Radio button at element 0 is "inTheLast".
        // Radio button at element 1 is "between".
        if (dateRangeObj[0].checked == true) {
            // Passing in "dateRangeObj[0].value" does not always work...
            // Something wierd about the way objects are initialized
            selectDateRangeButton("inTheLast");
        } else if (dateRangeObj[1].checked == true) {
            selectDateRangeButton("between");
        } else {
            selectDateRangeButton("");
        }
        var dateTypeObj = theFormObj.elements["FilterSettings.fileDateType"];
        selectDateType(dateTypeObj.value);
        
        theFormObj.elements["FilterSettings.fileNamePatternValue"].focus();
                
    }    

    function selectDateType(whichType) {
        var fileDateRange = "FilterSettings.fileDateRange";
        if (whichType == "blank") {
            ccSetRadioButtonDisabled(fileDateRange, theFormName, true);
            selectDateRangeButton("");
        } else {
            ccSetRadioButtonDisabled(fileDateRange, theFormName, false);
            selectDateRangeButton(lastDateRangeValue);
        }
    }

    function selectDateRangeButton(whichButton) {
        var fileDateNum = "FilterSettings.fileDateNum";
        var fileDateUnit = "FilterSettings.fileDateUnit";
        var fileDateStart = "FilterSettings.fileDateStart";
        var fileDateEnd = "FilterSettings.fileDateEnd";

        // Disable all
        ccSetTextFieldDisabled(fileDateNum, theFormName, true);
        ccSetTextFieldDisabled(fileDateUnit, theFormName, true);
        ccSetTextFieldDisabled(fileDateStart, theFormName, true);   
        ccSetTextFieldDisabled(fileDateEnd, theFormName, true);

        // Enable something?
        if (whichButton == "inTheLast") {
            ccSetTextFieldDisabled(fileDateNum, theFormName, false);
            ccSetTextFieldDisabled(fileDateUnit, theFormName, false);
            lastDateRangeValue = whichButton;
        } else if (whichButton == "between") {
            ccSetTextFieldDisabled(fileDateStart, theFormName, false);   
            ccSetTextFieldDisabled(fileDateEnd, theFormName, false);
            lastDateRangeValue = whichButton;
        }
    }

    function clearForm() {
        theFormObj.elements["FilterSettings.fileNamePatternValue"].value = "";
        theFormObj.elements["FilterSettings.fileSizeGreaterValue"].value = "";
        theFormObj.elements["FilterSettings.fileSizeGreaterUnit"].value = "kb";
        theFormObj.elements["FilterSettings.fileSizeLessValue"].value = "";
        theFormObj.elements["FilterSettings.fileSizeLessUnit"].value = "kb";
        theFormObj.elements["FilterSettings.fileDateType"].value = "blank";
        theFormObj.elements["FilterSettings.fileDateRange"][0].checked = false;
        theFormObj.elements["FilterSettings.fileDateRange"][1].checked = false;
        theFormObj.elements["FilterSettings.fileDateNum"].value = "";
        theFormObj.elements["FilterSettings.fileDateUnit"].value = "days";
        theFormObj.elements["FilterSettings.fileDateStart"].value = "";
        theFormObj.elements["FilterSettings.fileDateEnd"].value = "";
        theFormObj.elements["FilterSettings.ownerValue"].value = "";
        theFormObj.elements["FilterSettings.groupValue"].value = "";
        // Objects not present in 4.4
        var obj = theFormObj.elements["FilterSettings.isDamagedValue"];
        if (obj != null) {
            obj.value = "blank";
        }
        obj = theFormObj.elements["FilterSettings.isOnlineValue"];
        if (obj != null) {
            obj.value = "blank";
        }
        
        init();
        
    }
    
    // On an OK click, the order of events is:
    //  1.  Run OK button handler which validates entries, stores in Filter object,
    //      and puts serialzied filter object in hidden object of page and sets submitNow object.
    //  2.  Refresh popup page.  
    //  3.  On load, check for "true" in the submitNow object.
    //      If "true" is there, put in parent page, refresh parent and close popup.
    //      If nothing is there, we are not ready to close yet.
    // This method  does #3 above.
    function checkForSubmit() {
        // Check whether to submit now.
        var submitNow = theFormObj.elements["FilterSettings.submitNow"].value;
        if (submitNow == null || submitNow != "true") {
            return true;  // Continue with page load.  Probably first time or refresh on user error.
        }
                    
        // Need to return filter.  Get forms
        var parentFormName = theFormObj.elements["FilterSettings.parentFormName"].value;
        var parentFormObj   = window.opener.document.forms[parentFormName];

        // Save filter in parent form.  
        var parentReturnObjName = theFormObj.elements["FilterSettings.parentReturnValueObjName"].value;
        var parentReturnObj = parentFormObj.elements[parentReturnObjName];
        var returnValue = theFormObj.elements["FilterSettings.returnValue"].value;
        parentReturnObj.value = returnValue;

        // Refresh parent page.
        var parentSubmitCmd = theFormObj.elements["FilterSettings.parentSubmitCmd"].value;
        parentFormObj.action = parentFormObj.action + "?" + parentSubmitCmd;
        parentFormObj.submit();
        
        window.close();
        return false;
    } 

    function onKeyPressEnter(theEvent) {
        if (theEvent != null) {
            // Mozilla support
            if (theEvent.which == 13) {
                // Enter key pressed.  
                clickOK();
            }
        } else if (event != null) {
            // IE support
            if (event.keyCode == 13) {
                // Enter key pressed.  
                clickOK();
            }
        }
    }
    
    function clickOK() {
        var okButton = theFormObj.elements["FilterSettings.OKButton"];
        okButton.click();    
    }
</script>

<!-- Masthead -->
<cc:secondarymasthead name="SecondaryMasthead" bundleID="samBundle" />

<jato:form name="FilterSettingsForm" method="post">

<cc:pagetitle
    name="PageTitle"
    bundleID="samBundle"
    showPageTitleSeparator="true"
    showPageButtonsTop="false"
    showPageButtonsBottom="true">

<br />
<cc:alertinline name="Alert" bundleID="samBundle" />

<cc:propertysheet name="PropertySheet" 
              bundleID="samBundle" 
              showJumpLinks="false"/>


</cc:pagetitle>

<cc:hidden name="parentFormName" />
<cc:hidden name="parentReturnValueObjName" />
<cc:hidden name="parentSubmitCmd" />

</jato:form>
</body>
</cc:header>
</jato:useViewBean> 
