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

<!-- $Id: SharedFSClient.jsp,v 1.18 2008/12/17 20:18:55 kilemba Exp $ -->

<jsp:root
    version="1.2"
    xmlns:f="http://java.sun.com/jsf/core"
    xmlns:h="http://java.sun.com/jsf/html"
    xmlns:ui="http://www.sun.com/web/ui"
    xmlns:jsp="http://java.sun.com/JSP/Page">
<jsp:directive.page contentType="text/html;charset=utf-8" pageEncoding="UTF-8"/>

<f:view>
<f:loadBundle basename="com.sun.netstorage.samqfs.web.resources.Resources"
              var="samBundle"/>

<ui:page>
<ui:html>
<ui:head title="#{samBundle['SharedFS.pagetitle.client']}">
    <ui:script url="/js/popuphelper.js"/>
    <ui:script url="/js/fs/SharedFS.js"/>
    <ui:script url="/js/fs/SharedFSClientFilterPanel.js"/>
</ui:head>
<ui:body onLoad="
                if (parent.serverName != null) {
                    parent.setSelectedNode('0', 'SharedFSMembers');
                }">
<ui:form id="SharedFSClientForm">
    <ui:breadcrumbs id="breadcrumbs" pages="#{SharedFSBean.breadCrumbsClient}" />
    <ui:tabSet binding="#{SharedFSBean.tabSet}" selected="client" />
    <ui:alert id="alert"
              rendered="#{SharedFSBean.alertRendered}"
              type="#{SharedFSBean.alertType}"
              summary="#{SharedFSBean.alertSummary}"
              detail="#{SharedFSBean.alertDetail}">
        <f:facet name="alertLink">
            <ui:imageHyperlink id="imageLink"
                               rendered="#{SharedFSBean.alertLinkRendered}"
                               icon="HREF_LINK"
                               target="#{SharedFSBean.alertLinkTarget}"
                               url="#{SharedFSBean.alertLinkURL}"
                               text="#{SharedFSBean.alertLinkText}"
                               toolTip=""
                               onMouseOver=""
                               onMouseOut=""
                               onClick="javascript: var win = window.open('','#{SharedFSBean.alertLinkTarget}','height=500,width=750,top='+((screen.height-(screen.height/1.618))-(500/2))+',left='+((screen.width-650)/2)+',resizable'); win.focus();"/>
        </f:facet>
    </ui:alert>
    <ui:contentPageTitle
        id="pageTitle"
        title="#{SharedFSBean.clientPageTitle}">

    <h:panelGrid columns="1" style="margin:10px">
        <ui:staticText id="textInstruction" text="#{samBundle['SharedFS.client.instruction']}"/>
    </h:panelGrid>

    <ui:table id="tableClientSummary"
              title="#{SharedFSBean.clientTableTitle}"
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
                           disabled="#{!SharedFSBean.hasPermission}"
                           onClick="return launchAddClientsWizard(this);"/>
                <ui:button id="buttonRemove"
                           text="#{samBundle['common.button.remove']}"
                           disabled="#{!SharedFSBean.hasPermission}"
                           onClick="if (handleButtonRemove() == false) return false;"
                           actionListener="#{SharedFSBean.handleRemoveClient}"/>
                <ui:dropDown
                    id="MenuTop"
                    submitForm="true"
                    onChange="if (handleClientDropDownMenu(this) == false) return false;"
                    selected="#{SharedFSBean.clientTableSelectedOption}"
                    items="#{SharedFSBean.clientTableMenuOptions}"
                    actionListener="#{SharedFSBean.handleTableMenuSelection}" />
            </f:subview>
        </f:facet>
        <!-- Table Button Buttom -->
        <f:facet name="actionsBottom">
            <f:subview id="actionsBottom">
                <ui:button id="buttonAdd"
                           text="#{samBundle['common.button.add']}"
                           disabled="#{!SharedFSBean.hasPermission}"
                           onClick="return launchAddClientsWizard(this);"/>
                <ui:button id="buttonRemove"
                           text="#{samBundle['common.button.remove']}"
                           disabled="#{!SharedFSBean.hasPermission}"
                           onClick="if (handleButtonRemove() == false) return false;"
                           actionListener="#{SharedFSBean.handleRemoveClient}"/>
                <ui:dropDown
                    id="MenuBottom"
                    submitForm="true"
                    onChange="if (handleClientDropDownMenu(this) == false) return false;"
                    selected="#{SharedFSBean.clientTableSelectedOption}"
                    items="#{SharedFSBean.clientTableMenuOptions}"
                    actionListener="#{SharedFSBean.handleTableMenuSelection}" />
            </f:subview>
        </f:facet>

        <!-- filter -->
        <f:facet name="filter">
            <ui:dropDown id="filter"
                         submitForm="true"
                         immediate="true"
                         actionListener="#{SharedFSBean.handleFilterChange}"
                         items="#{SharedFSBean.clientTableFilterOptions}"
                         onChange="if (!handleFilterChange(this)) return false;"
                         selected="#{SharedFSBean.clientTableFilterSelectedOption}" />
        </f:facet>
        <f:facet name="filterPanel">
            <f:subview id="filterPanel">
                <jsp:include page="SharedFSClientFilterPanel.jsp"/>
            </f:subview>
        </f:facet>
        <ui:tableRowGroup id="clientTableRows"
                          selected="#{SharedFSBean.selectClient.selectedState}"
                          binding="#{SharedFSBean.clientSummaryTableRowGroup}"
                          sourceData="#{SharedFSBean.clientSummaryList}"
                          sourceVar="clientTable">
            <!-- Selection Type -->
            <ui:tableColumn id="selection"
                            selectId="select"
                            sort="#{SharedFSBean.selectClient.selectedState}">
                <ui:checkbox id="select"
                             onClick="setTimeout('initClientTableRows()', 0);"
                             name="checkBoxSelect"
                             selected="#{SharedFSBean.selectClient.selected}"
                             selectedValue="#{SharedFSBean.selectClient.selectedValue}"/>
            </ui:tableColumn>
            <ui:tableColumn id="colHostName"
                            headerText="#{samBundle['SharedFS.client.table.heading.hostname']}"
                            align="left"
                            valign="top"
                            sort="name"
                            rowHeader="true">
                <ui:staticText text="#{clientTable.value.name}"/>
            </ui:tableColumn>
            <ui:tableColumn id="colHostType"
                            headerText="#{samBundle['SharedFS.client.table.heading.type']}"
                            align="left"
                            valign="top"
                            sort="typeString"
                            rowHeader="true">
                <ui:staticText text="#{clientTable.value.typeString}"/>
            </ui:tableColumn>
            <ui:tableColumn id="colIPAddress"
                            headerText="#{samBundle['SharedFS.client.table.heading.ipaddress']}"
                            align="left"
                            valign="top"
                            sort="IPAddressesStr"
                            rowHeader="true">
                <ui:staticText text="#{clientTable.value.IPAddressesStr}"
                               escape="false"
                               converter="SpaceDelimiterConverter"/>
            </ui:tableColumn>
            <ui:tableColumn id="colStatus"
                            headerText="#{samBundle['SharedFS.client.table.heading.status']}"
                            align="left"
                            valign="top"
                            sort="statusString"
                            rowHeader="true">
                <ui:image id="imageStatus" hspace="2" url="#{clientTable.value.statusIcon}"/>
                <ui:staticText text="#{clientTable.value.statusString}"/>
            </ui:tableColumn>
        </ui:tableRowGroup>
    </ui:table>
    </ui:contentPageTitle>
    <ui:hiddenField id="Time" value="#{SharedFSBean.timeClient}"/>
    <ui:hiddenField id="NoMultipleOp" value="#{SharedFSBean.noMultipleOpMsg}"/>
    <ui:hiddenField id="NoneSelected" value="#{SharedFSBean.noneSelectedMsg}"/>
    <ui:hiddenField id="ConfirmRemove" value="#{SharedFSBean.confirmRemove}"/>
    <ui:hiddenField id="ConfirmDisable" value="#{SharedFSBean.confirmDisable}"/>
    <ui:hiddenField id="ConfirmUnmount" value="#{SharedFSBean.confirmUnmount}"/>
    <ui:hiddenField id="hiddenServerName" value="#{SharedFSBean.hiddenServerName}"/>
    <ui:hiddenField id="hiddenFSName" value="#{SharedFSBean.hiddenFSName}"/>
</ui:form>
</ui:body>
</ui:html>
</ui:page>
</f:view>
</jsp:root>
