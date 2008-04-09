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

// ident	$Id: mediaexpression.js,v 1.2 2008/04/09 20:37:26 ronaldso Exp $


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

    function getSelectedPoolName() {
        return getForm().elements[getViewName() + ".VSNPoolNameField"].value;
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
    function launchEditPoolPopup(expression) {
        // Check Permission
        var hasPermission = getForm().elements[
                            getViewName() + ".hasPermission"].value == "true";
        if (!hasPermission) {
            var noPermissionMsg = getForm().elements[
                                  getViewName() + ".NoPermissionMsg"].value;
            alert(noPermissionMsg);
            return false;
        }

        var params = "&OPERATION=EDIT";
        var poolName = getSelectedPoolName();
        var mediaType = getMediaType();
        params += "&pool_name=" + poolName + "&media_type=" + mediaType;

        if (expression != "") {
            params += "&expression=" + expression;
        }
        var win = launchPopup(POPUP_URI,
                            "edit_vsn_pool",
                            getForm().elements[
                                getViewName() + ".ServerName"].value,
                            SIZE_VOLUME_ASSIGNER,
                            params);

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

        // loop through the check boxes and determine which ones are selected
        for (var i = 0; i < expArray.length; i++) {
            var check_box_name = prefix + i;
            var theCheckBox = theForm.elements[check_box_name];

            if (theCheckBox.checked) {
              selected_exps += expArray[i] + ",";
            }
        }

        // Show error message if nothing is selected
        if (selected_exps.length == 0) {
            alert(theForm.elements[getViewName() + ".NoSelectionMsg"].value);
            return false;
        // Otherwise warn user that he/she is about to delete the selected
        // expressions
        } else {
            // Remove trailing semi-colon
            selected_exps =
                selected_exps.substring(0, selected_exps.length - 1);

            // check if user has selected all expressions, warn user he/she is
            // about to delete the pool if all expressions are selected
            if (existingExpression == selected_exps) {
                if (!window.confirm(getForm().elements[
                    getViewName() + ".deletePoolConfirmation"].value)) {
                    return false;
                }
            } else {
                var result = window.confirm(getForm().elements[
                    getViewName() + ".deleteConfirmation"].value);
                if (!result) {
                    return false;
                }
            }

            theForm.elements[getViewName() + ".SelectedExpression"].value
                = selected_exps;
        }
        return true;
    }

    function launchMatchingVolumePopup(field) {
        var selectedExp = getExpression(field.name);
        var param = '&SAMQFS_SHOW_CONTENT=false' +
            '&SAMQFS_SHOW_LINE_CONTROL=false' +
            '&SAMQFS_STAGE_Q_LIST=false' +
            '&matching_volumes=' + getSelectedPoolName() + ',' + selectedExp;
        launchPopup(
            '/admin/ShowFileContent',
            'content',
            getServerName(),
            SIZE_NORMAL,
            param);
        return false;
    }

    function handleTextExpression(field) {
        return launchEditPoolPopup(getExpression(field.name));
    }

    function handleButtonAdd() {
        return launchEditPoolPopup('');
    }


