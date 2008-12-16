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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

// ident	$Id: NewMediaReportPagelet.jsp,v 1.8 2008/12/16 00:10:40 am143972 Exp $
--%>
<%@ page language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>


<script language="javascript" src="/samqfsui/js/popuphelper.js"></script>
<script language="javascript" src="/samqfsui/js/samqfsui.js"></script>
<script language="javascript">
    
    function setStateCheckBoxes(state) {
        var myForm = document.NewReportForm;
        var prefix = "NewReport.NewReportView.check";

        for (var i = 1; i < 14; i++) {
            var checkbox_name = prefix + i;
            var checkbox = myForm.elements[checkbox_name];
            checkbox.checked = state;
        }
        return false;
    }
  
</script>

<jato:pagelet>
    <cc:i18nbundle
        id="samBundle"
        baseName="com.sun.netstorage.samqfs.web.resource.Resources"/>
        
    <cc:propertysheet
        name="PropertySheet"
        bundleID="samBundle"
        showJumpLinks="false"/>
    
</jato:pagelet>
