<!--  SAM-QFS_notice_begin

    CDDL HEADER START

    The contents of this file are subject to the terms of the
    Common Development and Distribution License (the "License").
    You may not use this file except in compliance with the License.

    You can obtain a copy of the license at pkg/OPENSOLARIS.LICENSE
    or http://www.opensolaris.org/os/licensing.
    See the License for the specific language governing permissions
    and limitations under the License.

    When distributing Covered Code, include this CDDL HEADER in each
    file and include the License file at pkg/OPENSOLARIS.LICENSE.
    If applicable, add the following below this CDDL HEADER, with the
    fields enclosed by brackets "[]" replaced with your own identifying
    information: Portions Copyright [yyyy] [name of copyright owner]

    CDDL HEADER END

    Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
    Use is subject to license terms.

      SAM-QFS_notice_end -->
<!--                                                                      -->

<!-- $Id: SharedFSClientFilterPanel.jsp,v 1.3 2008/12/17 20:18:55 kilemba Exp $ -->

<jsp:root
    version="1.2"
    xmlns:f="http://java.sun.com/jsf/core"
    xmlns:h="http://java.sun.com/jsf/html"
    xmlns:ui="http://www.sun.com/web/ui"
    xmlns:jsp="http://java.sun.com/JSP/Page">
<jsp:directive.page contentType="text/html;charset=utf-8" pageEncoding="UTF-8"/>

<ui:staticText id="instruction"
               text="#{samBundle['common.filter.instruction']}"
               style="margin:10px"/>
<ui:markup tag="br" singleton="true"/>

<h:panelGrid id="myTable" columns="2" border="0" style="margin:10px">
    <ui:markup tag="div" id="div_crit1_1" style="visibility:visible">
        <ui:label id="labelCriteria1"
                  text="#{samBundle['common.label.criteria.1']}" />
        <ui:dropDown id="menuFilterItems1"
                     selected="#{SharedFSFilterBean.selectedMenuFilterItem1}"
                     onChange="handleMenuFilterItemsChange(this)"
                     items="#{SharedFSFilterBean.menuFilterItems}"/>
        <ui:dropDown id="menuFilterCondition1"
                     selected="#{SharedFSFilterBean.selectedMenuFilterCondition1}"
                     items="#{SharedFSFilterBean.menuFilterConditions}"/>
     </ui:markup>
     <ui:markup tag="div" id="div_crit1_2" style="visibility:visible">
        <ui:textField id="text1"
                      text="#{SharedFSFilterBean.text1}"
                      style="visibility:visible"
                      trim="true" />
        <ui:dropDown id="menu1"
                     items="#{SharedFSFilterBean.menuStatus}"
                     selected="#{SharedFSFilterBean.menu1}"
                     style="visibility:hidden"/>
        <ui:button id="addButton1"
                   text="#{samBundle['common.button.plus']}"
                   onClick="handleAddButton(this); return false;"
                   primary="true"
                   mini="true" />
        <ui:button id="removeButton1"
                   text="#{samBundle['common.button.minus']}"
                   onClick="handleRemoveButton(this); return false;"
                   primary="true"
                   mini="true" />
    </ui:markup>
    <ui:markup tag="div" id="div_crit2_1" style="visibility:hidden">
        <ui:label id="labelCriteria2"
                  text="#{samBundle['common.label.criteria.2']}" />
        <ui:dropDown id="menuFilterItems2"
                     selected="#{SharedFSFilterBean.selectedMenuFilterItem2}"
                     onChange="handleMenuFilterItemsChange(this)"
                     items="#{SharedFSFilterBean.menuFilterItems}"/>
        <ui:dropDown id="menuFilterCondition2"
                     selected="#{SharedFSFilterBean.selectedMenuFilterCondition2}"
                     items="#{SharedFSFilterBean.menuFilterConditions}"/>
     </ui:markup>
     <ui:markup tag="div" id="div_crit2_2" style="visibility:hidden">
        <ui:textField id="text2"
                      text="#{SharedFSFilterBean.text2}"
                      style="visibility:hidden"
                      trim="true" />
        <ui:dropDown id="menu2"
                     items="#{SharedFSFilterBean.menuStatus}"
                     selected="#{SharedFSFilterBean.menu2}"
                     style="visibility:hidden"/>
        <ui:button id="addButton2"
                   text="#{samBundle['common.button.plus']}"
                   onClick="handleAddButton(this); return false;"
                   primary="true"
                   mini="true" />
        <ui:button id="removeButton2"
                   text="#{samBundle['common.button.minus']}"
                   onClick="handleRemoveButton(this); return false;"
                   primary="true"
                   mini="true" />
    </ui:markup>
    <ui:markup tag="div" id="div_crit3_1" style="visibility:hidden">
        <ui:label id="labelCriteria3"
                  text="#{samBundle['common.label.criteria.3']}" />
        <ui:dropDown id="menuFilterItems3"
                     selected="#{SharedFSFilterBean.selectedMenuFilterItem3}"
                     onChange="handleMenuFilterItemsChange(this)"
                     items="#{SharedFSFilterBean.menuFilterItems}"/>
        <ui:dropDown id="menuFilterCondition3"
                     selected="#{SharedFSFilterBean.selectedMenuFilterCondition3}"
                     items="#{SharedFSFilterBean.menuFilterConditions}"/>
     </ui:markup>
     <ui:markup tag="div" id="div_crit3_2" style="visibility:hidden">
        <ui:textField id="text3"
                      text="#{SharedFSFilterBean.text3}"
                      style="visibility:hidden"
                      trim="true" />
        <ui:dropDown id="menu3"
                     items="#{SharedFSFilterBean.menuStatus}"
                     selected="#{SharedFSFilterBean.menu3}"
                     style="visibility:hidden"/>
        <ui:button id="addButton3"
                   text="#{samBundle['common.button.plus']}"
                   onClick="handleAddButton(this); return false;"
                   primary="true"
                   mini="true" />
        <ui:button id="removeButton3"
                   text="#{samBundle['common.button.minus']}"
                   onClick="handleRemoveButton(this); return false;"
                   primary="true"
                   mini="true" />
    </ui:markup>
</h:panelGrid>
<ui:markup tag="div" styleClass="TblPnlBtnDiv">
    <ui:button id="submit"
               mini="true"
               primary="true"
               text="#{samBundle['common.button.ok']}"
               onClick="handleOkButton()"
               actionListener="#{SharedFSFilterBean.okButtonClicked}"/>
    <ui:button id="cancel"
               mini="true"
               onClick="toggleFilterPanel(); return false;"
               text="#{samBundle['common.button.cancel']}"/>
</ui:markup>

<!-- For javascript purpose -->
<ui:hiddenField id="condition1" value="#{SharedFSFilterBean.condition1}"/>
<ui:hiddenField id="condition2" value="#{SharedFSFilterBean.condition2}"/>
<ui:hiddenField id="menuContent1" value="#{SharedFSFilterBean.menuContent1}"/>
<ui:hiddenField id="menuContentValue1" value="#{SharedFSFilterBean.menuContentValue1}"/>
<ui:hiddenField id="menuContent2" value="#{SharedFSFilterBean.menuContent2}"/>
<ui:hiddenField id="menuContentValue2" value="#{SharedFSFilterBean.menuContentValue2}"/>
<ui:hiddenField id="errmsg_atleast" value="#{SharedFSFilterBean.errMsgAtLeast}" />
<ui:hiddenField id="errmsg_atmost" value="#{SharedFSFilterBean.errMsgAtMost}" />
<ui:hiddenField id="submitValue" value="#{SharedFSFilterBean.submitValue}" />

</jsp:root>
