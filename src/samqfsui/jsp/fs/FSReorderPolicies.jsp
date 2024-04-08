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

// ident	$Id: FSReorderPolicies.jsp,v 1.19 2008/12/16 00:10:45 am143972 Exp $
--%>

<%@ page info="Index" language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean className="com.sun.netstorage.samqfs.web.fs.FSReorderPoliciesViewBean">

<script language="javascript" src="/samqfsui/js/samqfsui.js"></script>
<script language="javascript" src="/samqfsui/js/popuphelper.js"></script>

<!-- Define the resource bundle, html, head, meta, stylesheet and body tags -->
<cc:header
    pageTitle="FSReorderPolicies.pagetitle"
    copyrightYear="2008"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    onLoad="
        window.opener.document.FSArchivePoliciesForm.target='_self';
        generateUpDownIdiom(); initializePopup(this);"
    bundleID="samBundle">



<script>
    function generateUpDownIdiom() {
        var tf = document.FSReorderPoliciesForm;
        var pf = window.opener.document.FSArchivePoliciesForm;
        var reorderHiddenFieldName =
            "FSArchivePolicies.FSArchivePoliciesView.ReorderNewOrderHiddenField";

        // get the policy string from hidden field
        var policyStr = pf.elements[reorderHiddenFieldName].value;

        // option for add
        var option;

        // parse it, return total number of items
        // delimitor is ","
        var nameArray = policyStr.split(",");
        var arraySize = nameArray.length;

        for (var i = 0; i < arraySize; i++) {
            option = new Option(nameArray[i], nameArray[i]);
            tf.upDownIdiom.options[tf.upDownIdiom.length] = option;
        }
    }

    function moveItem(myIdiom, up) {

        if ( myIdiom.length == -1) {  // If the list is empty
            // should never come to this case
            alert("The up/down idiom is empty!");
        } else {
            var selected = myIdiom.selectedIndex;

            if (selected != -1) {
                // Something is selected
                if ( myIdiom.length != 0 ) {
                    // check if the selected one can be moved.
                    // ignore if it cannot be moved.
                    if (!(up && selected == 0) &&
                        !(!up && selected == myIdiom.length - 1)) {
                        var moveText1, moveValue1, moveText2, moveValue2;

                        // now do the swapping part
                        if (up) {
                            moveText1  = myIdiom[selected-1].text;
                            moveValue1 = myIdiom[selected-1].value;
                        } else {
                            moveText1  = myIdiom[selected+1].text;
                            moveValue1 = myIdiom[selected+1].value;
                        }

                        moveText2 = myIdiom[selected].text;
                        moveValue2 = myIdiom[selected].value;

                        myIdiom[selected].text = moveText1;
                        myIdiom[selected].value = moveValue1;

                        if (up) {
                            myIdiom[selected-1].text = moveText2;
                            myIdiom[selected-1].value = moveValue2;
                            myIdiom.selectedIndex = selected - 1;
                        } else {
                            myIdiom[selected+1].text = moveText2;
                            myIdiom[selected+1].value = moveValue2;
                            myIdiom.selectedIndex = selected+1;
                        }
                    } // ends if the selected one can be moved
                } // ends if the list has more than 1 entry
            } // ends if something has been selected
        } // ends if the list is empty
    }

    function getResultString() {

        var tf = document.FSReorderPoliciesForm;

        var upDownVal = new Array();
        var separator = ',';
        var option    = 0;
        var upDownObj = tf.upDownIdiom;

        while (upDownObj[option]) {
            upDownVal[option] = upDownObj[option].value;
            option++;
        }

        return upDownVal.join(separator);
    }

    function doSubmit() {

        // Form Name
        var tf   = document.FSReorderPoliciesForm;
        tf.elements["FSReorderPolicies.newOrder"].value = getResultString();
        // Element Names in Reorder Policy Page

        // save the new order and submit the form
        return true;
    }

</script>


<!-- Masthead -->
<cc:secondarymasthead name="SecondaryMasthead" bundleID="samBundle" />

<jato:form name="FSReorderPoliciesForm" method="post">

<cc:alertinline name="Alert" bundleID="samBundle" />

<cc:pagetitle name="PageTitle" bundleID="samBundle"
	pageTitleText="FSReorderPolicyCriteria.title"
        showPageTitleSeparator="true"
        showPageButtonsTop="false"
        showPageButtonsBottom="true">

<br />
<center>

<table border="0">

  <tr>
    <td width="50%" rowspan="4">
      <select name="upDownIdiom" multiple
        size="<cc:text name='IdiomSize' />">
      </select>
    </td>
    <td width="50%">&nbsp;</td>
  </tr>

  <tr>
    <td width="50%">
      <cc:button name="Button" bundleID="samBundle"
        defaultValue="FSReorderPolicies.moveUpButton"
        type="default"
        onClick="moveItem(upDownIdiom, true); return false" />
    </td>
  </tr>

  <tr>
    <td width="50%">
      <cc:button name="Button" bundleID="samBundle"
        defaultValue="FSReorderPolicies.moveDownButton"
        type="default"
        onClick="moveItem(upDownIdiom, false); return false" />
    </td>
  </tr>

  <tr>
    <td width="50%">&nbsp;</td>
  </tr>

</table>
</center>

</cc:pagetitle>

<cc:hidden name="newOrder"/>
</jato:form>
</cc:header>
</jato:useViewBean>
