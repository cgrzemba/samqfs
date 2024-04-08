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

// ident    $Id: FSMount.js,v 1.7 2008/12/16 00:10:37 am143972 Exp $

/** 
 * This is the javascript file for FS Mount Option Page
 */

    function resetPage() {
        var tf = document.FSMountForm;
        tf.reset();
        var forceIOButton =
            tf.elements["FSMount.MountOptionsView.forceValue"];
        if (forceIOButton != null) {
            disableIOParams(forceIOButton[0].checked ? "on" : "off");
        }
    }

    function isValidNumericData(fieldId, unitId, startValue, startUnit,
            endValue, endUnit) {

        var numValue = trim(fieldId.value);
        var unitValue = unitId;
        if (!isEmpty(unitId))
            unitValue = unitId.options[unitId.selectedIndex].value;
        if (!isEmpty(numValue)) {
            if (!isValidNum(numValue, unitValue, startValue, 
                            startUnit, endValue, endUnit))
                return false;            
        }
        return true;
    }
    function isMultipleOf8KB(fieldId, unitId) {

        var value = trim(fieldId.value);
        var unitValue = unitId.options[unitId.selectedIndex].value
        if (unitValue == "kb") {
            if (value % 8 == 0)
                return true;
            else
                return false;
        } else
            return true;    
    }
    
    function isValidPositiveData(fieldId) {
        var value = trim(fieldId.value);
        if (!isEmpty(value)) {
            if (!isPositiveInteger(value))
                return false;
        }
        return true;
    }
    
    function checkHWMValue(fieldId) {
        return isValidNumericData(fieldId, "", "0", "", "100", "");
    }

    function checkLWMValue(fieldId) {
        var tf = document.FSMountForm;
        var endValue = trim(
                       tf.elements["FSMount.MountOptionsView.hwmValue"].value);
        return isValidNumericData(fieldId, "", "0", "", endValue, "");
    }

    function checkStripeValue(fieldId) {
        return isValidNumericData(fieldId, "", "0", "", "255", "");
    }

    function checkRetriesValue(fieldId) {
        return isValidNumericData(fieldId, "", "0", "", "20000", "");
    }

    function checkmetarefreshValue(fieldId) {
        return isValidNumericData(fieldId, "", "0", "", "60", "");
        // return isValidPositiveData(fieldId);
    }

    function checkMiniblockValue(fieldId) {
        var tf = document.FSMountForm;
        var unitId = tf.elements["FSMount.MountOptionsView.minblockUnit"];
        if (isValidNumericData(fieldId, unitId,
            "16", "kb", "2", "gb"))
            return isMultipleOf8KB(fieldId, unitId);
        return false;
    }

    function checkMaxblockValue(fieldId) {
        var tf = document.FSMountForm;
        var unitId = tf.elements["FSMount.MountOptionsView.maxblockUnit"];
        if (isValidNumericData(fieldId, unitId,
            "16", "kb", "4", "gb"))
            return isMultipleOf8KB(fieldId, unitId);
        return false;
    }

    function checkReadLeaseValue(fieldId) {
        return isValidNumericData(fieldId, "", "15", "", "600", "");
    }

    function checkWriteLeaseValue(fieldId) {
        return isValidNumericData(fieldId, "", "15", "", "600", "");
    }

    function checkAppendLeaseValue(fieldId) {
        return isValidNumericData(fieldId, "", "15", "", "600", "");
    }

    function checkMaxStreamValue(fieldId) {
        return isValidNumericData(fieldId, "", "8", "", "2048", "");
    }

    function checkMinPoolValue(fieldId) {
        return isValidNumericData(fieldId, "", "8", "", "2048", "");
    }

    function checkMaxPartValue(fieldId) {
        var tf = document.FSMountForm;
        return isValidNumericData(fieldId,
            tf.elements["FSMount.MountOptionsView.maxpartUnit"],
            "8", "kb", "2", "gb");
    }

    function checkPartReleaseValue(fieldId) {
        var tf = document.FSMountForm;
        var endValue = trim(
           tf.elements["FSMount.MountOptionsView.maxpartValue"].value);
        var endUnit=tf.elements["FSMount.MountOptionsView.maxpartUnit"];
        var endUnitVal = endUnit.options[endUnit.selectedIndex].value;

        return isValidNumericData(fieldId,
            tf.elements["FSMount.MountOptionsView.partreleaseUnit"],
            "8", "kb", endValue, endUnitVal);
    }
    
    function checkPartStageValue(fieldId) {
        var tf = document.FSMountForm;
        var endValue = trim(
                tf.elements["FSMount.MountOptionsView.maxpartValue"].value);
        var endUnit=tf.elements["FSMount.MountOptionsView.maxpartUnit"];
        var endUnitVal = endUnit.options[endUnit.selectedIndex].value;

        var unitId = tf.elements["FSMount.MountOptionsView.partstageUnit"];
        if (isValidNumericData(fieldId, unitId, "0", "kb",
            endValue, endUnitVal)) {
            return isMultipleOf8KB(fieldId, unitId);
        }
        return false;
    }

    function checkStageRetriesValue(fieldId) {
        return isValidNumericData(fieldId, "", "0", "", "20", "");
    }

    function checkStageWindowValue(fieldId) {
        var tf = document.FSMountForm;
        return isValidNumericData(fieldId,
            tf.elements["FSMount.MountOptionsView.stagewindowUnit"],
            "64", "kb", "2", "gb");
    }

    function checkReadaheadValue(fieldId) {
        var tf = document.FSMountForm;
        var unitId = tf.elements["FSMount.MountOptionsView.readaheadUnit"];
        if (isValidNumericData(fieldId, unitId,
            "0", "kb", "16", "gb"))
            return isMultipleOf8KB(fieldId, unitId);

        return false;
    }

    function checkWritebehindValue(fieldId) {
        var tf = document.FSMountForm;

        var unitId = tf.elements["FSMount.MountOptionsView.writebehindUnit"];
        if (isValidNumericData(fieldId, unitId,
            "8", "kb", "16", "gb"))
            return isMultipleOf8KB(fieldId, unitId);

        return false;
    }

    function checkWritethrottleValue(fieldId) {
        var tf = document.FSMountForm;
        var numValue =
            trim(tf.elements["FSMount.MountOptionsView.writethroValue"].value);
        return isValidNum(numValue,
            tf.elements["FSMount.MountOptionsView.writethroUnit"].value,
            "0", "b", "512", "mb");
    }

    function checkFlushBehindValue(fieldId) {
        var tf = document.FSMountForm;
        var unitId = tf.elements["FSMount.MountOptionsView.flushbehindUnit"];
        if (isValidNumericData(fieldId, unitId,
            "0", "b", "8", "mb")){
            return isMultipleOf8KB(fieldId, unitId);
        }
        return false;
    }

    function checkStageFlushValue(fieldId) {
        var tf = document.FSMountForm;
        var unitId = tf.elements["FSMount.MountOptionsView.stageflushUnit"];
        if (isValidNumericData(fieldId, unitId,
            "0", "b", "8", "mb")){
            return isMultipleOf8KB(fieldId, unitId);
        }
        return false;
    }

    function checkLeaseTimeoValue(fieldId) {
        var value = parseInt(trim(fieldId.value));
        if (value > 15 || value < -1) {
            return false;
        } else {
            return true;
        }
    }
    
    function checkMetadataValue(fieldId) {
        return isValidNumericData(fieldId, "", "0", "", "255", "");
    }

    function checkConsecreadValue(fieldId) {
        return isValidNumericData(fieldId, "", "0", "", "1073741824", "");
    }

    function checkWellreadminValue(fieldId) {
        var tf = document.FSMountForm;
        return isValidNumericData(fieldId,
            tf.elements["FSMount.MountOptionsView.wellreadminUnit"],
            "0", "b", "1", "tb");
    }

    function checkMisreadminValue(fieldId) {
        var tf = document.FSMountForm;
        return isValidNumericData(fieldId,
            tf.elements["FSMount.MountOptionsView.misreadminUnit"],
            "0", "b", "1", "tb");
    }

    function checkConsecwriteValue(fieldId) {
        return isValidNumericData(fieldId, "", "0", "", "1073741824", "");
    }

    function checkWellwriteminValue(fieldId) {
        var tf = document.FSMountForm;
        return isValidNumericData(fieldId,
            tf.elements["FSMount.MountOptionsView.wellwriteminUnit"],
            "0", "b", "1", "tb");
    }

    function checkMiswriteminValue(fieldId) {
        var tf = document.FSMountForm;
        var unitId = tf.elements["FSMount.MountOptionsView.miswriteminUnit"];
        var value  = tf.elements["FSMount.MountOptionsView.miswriteminValue"];
        
        return isValidNumericData(value, unitId,
            "0", "b", "1", "tb");
    }

    function disableIOParams(forceDirectIO) {
        var disabled = (forceDirectIO == "on");

        var formName = "FSMountForm";
        ccSetTextFieldDisabled(
            "FSMount.MountOptionsView.consecreadValue", formName, disabled);
        ccSetTextFieldDisabled(
            "FSMount.MountOptionsView.wellreadminValue", formName, disabled);
        ccSetDropDownMenuDisabled(
            "FSMount.MountOptionsView.wellreadminUnit", formName, disabled);
        ccSetTextFieldDisabled(
            "FSMount.MountOptionsView.misreadminValue", formName, disabled);
        ccSetDropDownMenuDisabled(
            "FSMount.MountOptionsView.misreadminUnit", formName, disabled);
        ccSetTextFieldDisabled(
            "FSMount.MountOptionsView.consecwriteValue", formName, disabled);
        ccSetTextFieldDisabled(
            "FSMount.MountOptionsView.wellwriteminValue", formName, disabled);
        ccSetDropDownMenuDisabled(
            "FSMount.MountOptionsView.wellwriteminUnit", formName, disabled);
        ccSetTextFieldDisabled(
            "FSMount.MountOptionsView.miswriteminValue", formName, disabled);
        ccSetDropDownMenuDisabled(
            "FSMount.MountOptionsView.miswriteminUnit", formName, disabled);
        ccSetRadioButtonDisabled(
            "FSMount.MountOptionsView.dioszeroValue", formName, disabled);
    }

