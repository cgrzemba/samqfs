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

// ident	$Id: copyvsns.js,v 1.7 2008/04/03 02:21:38 ronaldso Exp $

    var origMenuValue = "";

    function getForm() {
        return document.forms[0];
    }

    function getPageName() {
        var strArray = getForm().action.split("/");
        return strArray[strArray.length - 1];
    }

    function getViewName() {
        return getPageName() + ".MediaExpressionView";
    }

    function getTableName() {
        return "MediaExpressionTable";
    }

    function saveMenuValue(field) {
        origMenuValue = field.value;
    }

    /* handler for the media type drop down */
    function handleMediaTypeChange(field) {
        var formName = field.form.name;
        var theForm = field.form;
        var resetMsgArr =
            theForm.elements[getPageName() + ".ResetMessage"].value.split(";");

        // Change from undefined to a media type
        if (origMenuValue == "-999") {
            if (!confirm(resetMsgArr[1])) {
                field.value = origMenuValue;
                return false;
            }
        } else {
            if (!confirm(resetMsgArr[0])) {
                field.value = origMenuValue;
                return false;
            }
        }

        // pop new pool windows, reset menu to original value just in case if
        // user clicks cancel in the new pool pop up.  The menu will be
        // presented with new media pool after a pool is successfully created.
        field.value = origMenuValue;

        launchCopyVSNExtensionPopup(true);
        return false;
    }

    function getMediaType() {
        return getForm().elements[getViewName() + ".MediaType"].value;
    }

    function getTileNumberFromName(fieldName) {
        if (fieldName == null)
            return -1;

        var a1 = fieldName.split("[");
        var a2 = a1[1].split("]");
        var tileNumber = a2[0];

        return tileNumber;
    }

    function getExpression(fieldName) {
        var expValue =
            getForm().elements[getViewName() +  ".Expressions"].value;
        var expArray = expValue.split(",");
        var index = getTileNumberFromName(fieldName);
        return expArray[index];
    }

    /**
     * @Override
     */
    function launchEditExpressionPopup(expression) {
        // Check Permission
        var hasPermission = getForm().elements[
                            getViewName() + ".hasPermission"].value == "true";
        if (!hasPermission) {
            var noPermissionMsg = getForm().elements[
                                  getViewName() + ".NoPermissionMsg"].value;
            alert(noPermissionMsg);
            return false;
        }

        var params = "&OPERATION=EDIT_EXP";
        var mediaType = getMediaType();
        var policyName = getForm().elements[
                            getPageName() + ".PolicyName"].value;
        var copyNumber = getForm().elements[
                            getPageName() + ".CopyNumber"].value;
        params += "&media_type=" + mediaType +
                  "&policy_info=" + policyName + "." + copyNumber;

        if (expression != "") {
            params += "&expression=" + expression;
        }
        var win = launchPopup("/archive/NewEditVSNPool",
                            "edit_exp",
                            getForm().elements[
                                getViewName() + ".ServerName"].value,
                            SIZE_VOLUME_ASSIGNER,
                            params);

        return false;
    }

     function launchMatchingVolumePopup(field) {
        var selectedExp = getExpression(field.name);
        var param = '&SAMQFS_SHOW_CONTENT=false' +
            '&SAMQFS_SHOW_LINE_CONTROL=false' +
            '&SAMQFS_STAGE_Q_LIST=false' +
            '&matching_volumes=' + ',' + selectedExp;
        launchPopup(
            '/admin/ShowFileContent',
            'content',
            getServerName(),
            SIZE_NORMAL,
            param);
        return false;
    }

    function launchCopyVSNExtensionPopup(resetType) {
        var mediaType =
            resetType ? -1 : getMediaType();
        var policyName = getForm().elements[
                            getPageName() + ".PolicyName"].value;
        var copyNumber = getForm().elements[
                            getPageName() + ".CopyNumber"].value;
        var param = "&SAMQFS_media_type=" + mediaType +
                  "&SAMQFS_policy_info=" + policyName + "." + copyNumber;
        launchPopup(
            '/archive/CopyVSNExtension',
            'content',
            getServerName(),
            SIZE_VOLUME_ASSIGNER,
            param);
        return false;
    }

    function isPool(field) {
        var tileNumber = getTileNumberFromName(field.name);
        var poolBufArray =
            getForm().elements[getViewName() + ".poolBoolean"].value.split(",");
        return poolBufArray[tileNumber] == "true";
    }

    function handleTextExpression(field) {
        if (!isPool(field)) {
            launchEditExpressionPopup(getExpression(field.name));
            return false;
        } else {
            // Otherwise go to pool details page
            return true;
        }
    }

    function handleButtonAdd() {
        launchCopyVSNExtensionPopup(false);
        return false;
    }

    function handleButtonDelete() {
        var prefix = getViewName() + "." +
                     getTableName() + ".SelectionCheckbox";
        var theForm = getForm();
        var existingExpression =
            theForm.elements[getViewName() + ".Expressions"].value;
        var expArray = existingExpression.split(",");
        var selected_exps = "";
        var selected_pools = "";
        var allChecked = true;

        // loop through the check boxes and determine which ones are selected
        for (var i = 0; i < expArray.length; i++) {
            var check_box_name = prefix + i;
            var theCheckBox = theForm.elements[check_box_name];
            var poolBuf = getForm().elements[getViewName() + ".poolBoolean"].
                                                            value.split(",");
            if (theCheckBox.checked) {
                if (poolBuf[i] == "true") {
                    selected_pools += expArray[i] + ",";
                } else {
                    selected_exps += expArray[i] + ",";
                }
            } else {
                allChecked = false;
            }
        }

        // Show error message if nothing is selected
        if (selected_exps.length == 0 && selected_pools.length == 0) {
            alert(theForm.elements[getViewName() + ".NoSelectionMsg"].value);
            return false;
        // Otherwise warn user that he/she is about to delete the selected
        // expressions
        } else {
            // Remove trailing semi-colon
            selected_exps =
                selected_exps.length == 0 ?
                    "" :
                    selected_exps.substring(0, selected_exps.length - 1);
            selected_pools =
                selected_pools.length == 0 ?
                    "" :
                    selected_pools.substring(0, selected_pools.length - 1);

            // check if user has selected all expressions, warn user he/she is
            // about to delete the pool if all expressions are selected
            if (allChecked) {
                alert(theForm.elements[
                        getPageName() + ".DeleteAllMessage"].value);
                return false;
            } else {
                var result = window.confirm(getForm().elements[
                    getViewName() + ".deleteConfirmation"].value);
                if (!result) {
                    return false;
                }
            }
            theForm.elements[getViewName() + ".SelectedExpression"].value
                = selected_exps;
            theForm.elements[getViewName() + ".SelectedPool"].value
                = selected_pools;
        }

return true;
    }