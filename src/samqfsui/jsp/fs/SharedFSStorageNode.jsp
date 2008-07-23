<!--  SAM-QFS_notice_begin                                                -->
<!--                                                                      -->
<!--CDDL HEADER START                                                     -->
<!--                                                                      -->
<!--The contents of this file are subject to the terms of the             -->
<!--Common Development and Distribution License (the "License").          -->
<!--You may not use this file except in compliance with the License.      -->
<!--                                                                      -->
<!--You can obtain a copy of the license at pkg/OPENSOLARIS.LICENSE   -->
<!--or http://www.opensolaris.org/os/licensing.                           -->
<!--See the License for the specific language governing permissions       -->
<!--and limitations under the License.                                    -->
<!--                                                                      -->
<!--When distributing Covered Code, include this CDDL HEADER in each      -->
<!--file and include the License file at pkg/OPENSOLARIS.LICENSE.     -->
<!--If applicable, add the following below this CDDL HEADER, with the     -->
<!--fields enclosed by brackets "[]" replaced with your own identifying   -->
<!--information: Portions Copyright [yyyy] [name of copyright owner]      -->
<!--                                                                      -->
<!--CDDL HEADER END                                                       -->
<!--                                                                      -->
<!--Copyright 2008 Sun Microsystems, Inc.  All rights reserved.         -->
<!--Use is subject to license terms.                                      -->
<!--                                                                      -->
<!--  SAM-QFS_notice_end                                                  -->
<!--                                                                      -->

<!-- $Id: SharedFSStorageNode.jsp,v 1.6 2008/07/23 17:38:38 ronaldso Exp $ -->

<jsp:root
    version="1.2"
    xmlns:f="http://java.sun.com/jsf/core"
    xmlns:h="http://java.sun.com/jsf/html"
    xmlns:ui="http://www.sun.com/web/ui"
    xmlns:jsp="http://java.sun.com/JSP/Page">
<jsp:directive.page
    contentType="text/html;charset=ISO-8859-1"
    pageEncoding="UTF-8"/>

<f:view>
<f:loadBundle basename="com.sun.netstorage.samqfs.web.resources.Resources"
              var="samBundle"/>

<ui:page>
<ui:html>
<ui:head title="#{samBundle['SharedFS.pagetitle.sn']}">
    <ui:script url="/js/popuphelper.js"/>
    <ui:script url="/js/fs/SharedFS.js"/>
</ui:head>
<ui:body id="SNBody"
         onLoad="
                if (parent.serverName != null) {
                    parent.setSelectedNode('0', 'SharedFSStorageNode');
                }">
<ui:form id="SharedFSStorageNodeForm">
    <ui:breadcrumbs id="breadcrumbs" pages="#{SharedFSBean.breadCrumbsStorageNode}" />
    <ui:tabSet binding="#{SharedFSBean.tabSet}" selected="storagenode" />
    <ui:alert id="alert"
              rendered="#{SharedFSBean.alertRendered}"
              type="#{SharedFSBean.alertType}"
              summary="#{SharedFSBean.alertSummary}"
              detail="#{SharedFSBean.alertDetail}"/>
    <ui:contentPageTitle
        id="pageTitle"
        title="#{samBundle['SharedFS.pagetitle.sn']}">

    <ui:markup tag="br" singleton="true"/>
    <ui:staticText id="textInstruction" text="#{samBundle['SharedFS.help.sn.pageinstruction']}" style="margin:10px"/>
    <ui:markup tag="br" singleton="true"/>

    <ui:table id="tableStorageNodeSummary"
              title="#{SharedFSBean.snTableTitle}"
              style="margin:10px"
              paginateButton="true"
              paginationControls="true"
              clearSortButton="true"
              deselectMultipleButton="true"
              selectMultipleButton="true"
              sortPanelToggleButton="true">

        <!-- Table Button Top -->
        <f:facet name="actionsTop">
            <f:subview id="actionsTop">
                <ui:button id="buttonAdd"
                           text="#{samBundle['common.button.add']}"
                           onClick="launchAddStorageNodeWizard(this); return false;"/>
                <ui:button id="buttonRemove"
                           text="#{samBundle['common.button.remove']}"
                           onClick="if (handleButtonRemove(0) == false) return false;"
                           actionListener="#{SharedFSBean.handleRemoveStorageNode}"/>
                <ui:dropDown
                    id="Menu"
                    submitForm="true"
                    forgetValue="true"
                    onChange="if (handleSnDropDownMenu(this) == false) return false;"
                    selected="#{SharedFSBean.snTableSelectedOption}"
                    items="#{SharedFSBean.snTableMenuOptions}"
                    actionListener="#{SharedFSBean.handleSnTableMenuSelection}" />
            </f:subview>
        </f:facet>
        <!-- Table Button Buttom -->
        <f:facet name="actionsBottom">
            <f:subview id="actionsBottom">
                <ui:button id="buttonAdd"
                           text="#{samBundle['common.button.add']}"
                           onClick="alert('Coming Soon'); return false;"/>
                <ui:button id="buttonRemove"
                           text="#{samBundle['common.button.remove']}"
                           onClick="if (handleOperation(0, true) == false) return false;"
                           actionListener="#{SharedFSBean.handleRemoveStorageNode}"/>
                <ui:dropDown
                    id="Menu"
                    submitForm="true"
                    forgetValue="true"
                    onChange="if (handleSnDropDownMenu(this) == false) return false;"
                    selected="#{SharedFSBean.snTableSelectedOption}"
                    items="#{SharedFSBean.snTableMenuOptions}"
                    actionListener="#{SharedFSBean.handleSnTableMenuSelection}" />
            </f:subview>
        </f:facet>

        <!-- filter -->
        <f:facet name="filter">
            <ui:dropDown id="filter"
                         submitForm="true"
                         immediate="true"
                         actionListener="#{SharedFSBean.handleSnFilterChange}"
                         items="#{SharedFSBean.snTableFilterOptions}"
                         selected="#{SharedFSBean.snTableFilterSelectedOption}" />
        </f:facet>

        <ui:tableRowGroup id="snTableRows"
                          selected="#{SharedFSBean.selectStorageNode.selectedState}"
                          binding="#{SharedFSBean.snSummaryTableRowGroup}"
                          sourceData="#{SharedFSBean.snSummaryList}"
                          sourceVar="snTable">
            <!-- Selection Type -->
            <ui:tableColumn id="selection"
                            selectId="select"
                            sort="#{SharedFSBean.selectStorageNode.selectedState}">
                <ui:checkbox id="select"
                             onClick="setTimeout('initSnTableRows()', 0);"
                             name="checkBoxSelect"
                             selected="#{SharedFSBean.selectStorageNode.selected}"
                             selectedValue="#{SharedFSBean.selectStorageNode.selectedValue}"/>
            </ui:tableColumn>
            <ui:tableColumn id="colHostName"
                            headerText="#{samBundle['SharedFS.sn.table.heading.hostname']}"
                            align="left"
                            valign="top"
                            sort="name"
                            rowHeader="true">
                <ui:staticText text="#{snTable.value.name}"/>
            </ui:tableColumn>
            <ui:tableColumn id="colFault"
                            headerText="#{samBundle['SharedFS.sn.table.heading.fault']}"
                            align="left"
                            valign="top"
                            sort="faultString"
                            rowHeader="true">
                <ui:image id="imageFault" hspace="5" url="#{snTable.value.faultIcon}"/>
                <ui:staticText text="#{snTable.value.faultString}"/>
            </ui:tableColumn>
            <ui:tableColumn id="colUsage"
                            headerText="#{samBundle['SharedFS.sn.table.heading.usage']}"
                            align="left"
                            valign="top"
                            sort="usage"
                            rowHeader="true">
                <ui:image id="imageUsage" hspace="5" url="#{snTable.value.usageBar}"/>
                <ui:staticText text="#{snTable.value.usage}" visible="false"/>
                <ui:staticText text="#{snTable.value.capacityStr}"/>
            </ui:tableColumn>
            <ui:tableColumn id="colIScsiId"
                            headerText="#{samBundle['SharedFS.sn.table.heading.iscsiid']}"
                            align="left"
                            valign="top"
                            sort="iscsiid"
                            rowHeader="true">
                <ui:staticText text="#{snTable.value.iscsiid}"/>
            </ui:tableColumn>
            <ui:tableColumn id="colStatus"
                            headerText="#{samBundle['SharedFS.sn.table.heading.status']}"
                            align="left"
                            valign="top"
                            sort="statusString"
                            rowHeader="true">
                <ui:image id="imageStatus" hspace="2" url="#{snTable.value.statusIcon}"/>
                <ui:staticText text="#{snTable.value.statusString}"/>
            </ui:tableColumn>
        </ui:tableRowGroup>
    </ui:table>
    </ui:contentPageTitle>
    <ui:hiddenField id="Time" value="#{SharedFSBean.timeStorageNode}"/>
    <ui:hiddenField id="NoMultipleOp" value="#{SharedFSBean.noMultipleOpMsg}"/>
    <ui:hiddenField id="NoneSelected" value="#{SharedFSBean.noneSelectedMsg}"/>
    <ui:hiddenField id="ConfirmRemoveSn" value="#{SharedFSBean.confirmRemoveSn}"/>
    <ui:hiddenField id="ConfirmDisableSn" value="#{SharedFSBean.confirmDisableSn}"/>
    <ui:hiddenField id="ConfirmUnmountSn" value="#{SharedFSBean.confirmUnmountSn}"/>
    <ui:hiddenField id="ConfirmClearFaultSn" value="#{SharedFSBean.confirmClearFaultSn}"/>
    <ui:hiddenField id="hiddenServerName" value="#{SharedFSBean.hiddenServerName}"/>
    <ui:hiddenField id="hiddenFSName" value="#{SharedFSBean.hiddenFSName}"/>
</ui:form>
</ui:body>
</ui:html>
</ui:page>
</f:view>
</jsp:root>
