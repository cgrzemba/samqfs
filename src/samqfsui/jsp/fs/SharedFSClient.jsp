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

<!-- $Id: SharedFSClient.jsp,v 1.1 2008/06/11 16:57:59 ronaldso Exp $ -->

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
            <ui:head title="#{samBundle['SharedFS.pagetitle.client']}"/>
            <ui:body>
                <ui:form id="SharedFSClientForm">
                    <ui:breadcrumbs id="breadcrumbs" pages="#{SharedFSBean.breadCrumbsClient}" />
                    <ui:tabSet binding="#{SharedFSBean.tabSet}" selected="client" />
                    <ui:alert id="alert"
                              rendered="#{SharedFSBean.alertRendered}"
                              type="#{SharedFSBean.alertType}"
                              summary="#{SharedFSBean.alertSummary}"
                              detail="#{SharedFSBean.alertDetail}"/>
                    <ui:contentPageTitle
                        id="pageTitle"
                        title="#{samBundle['SharedFS.pagetitle.client']}">

                        <f:verbatim><![CDATA[<br><br>]]></f:verbatim>
                        <ui:staticText id="textInstruction" text="#{samBundle['SharedFS.help.client.pageinstruction']}" style="margin:10px"/>
                        <f:verbatim><![CDATA[<br><br>]]></f:verbatim>

                        <ui:table id="tableClientSummary"
                                  title="#{samBundle['SharedFS.title.table.clients']}"
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
                                               onClick="alert('Coming Soon');"/>
                                    <ui:button id="buttonRemove"
                                               text="#{samBundle['common.button.remove']}"
                                               actionListener="#{SharedFSBean.handleRemoveClient}"/>
                                    <ui:dropDown
                                        id="Menu"
                                        submitForm="true"
                                        immediate="true"
                                        forgetValue="true"
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
                                               onClick="alert('Coming Soon');"/>
                                    <ui:button id="buttonRemove"
                                               text="#{samBundle['common.button.remove']}"
                                               actionListener="#{SharedFSBean.handleRemoveClient}"/>
                                    <ui:dropDown
                                        id="Menu"
                                        submitForm="true"
                                        immediate="true"
                                        forgetValue="true"
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
                                             action="handleFilterChange"
                                             items="#{SharedFSBean.clientTableFilterOptions}"
                                             onChange="if (filterMenuChanged() == false) return false"
                                             selected="#{SharedFSBean.clientTableFilterSelectedOption}" />
                            </f:facet>

                            <ui:tableRowGroup id="clientTableRows"
                                              binding="#{SharedFSBean.clientSummaryTableRowGroup}"
                                              sourceData="#{SharedFSBean.clientSummaryList}"
                                              sourceVar="clientTable">
                                <!-- Selection Type -->
                                <ui:tableColumn id="selection"
                                                selectId="select"
                                                sort="#{SharedFSBean.selectClient.selectedState}">
                                    <ui:checkbox id="select"
                                                 name="same"
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
                                <ui:tableColumn id="colArch"
                                                headerText="#{samBundle['SharedFS.client.table.heading.arch']}"
                                                align="left"
                                                valign="top"
                                                sort="arch"
                                                rowHeader="true">
                                    <ui:staticText text="#{clientTable.value.arch}"/>
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
                                <ui:tableColumn id="colOS"
                                                headerText="#{samBundle['SharedFS.client.table.heading.os']}"
                                                align="left"
                                                valign="top"
                                                sort="OS"
                                                rowHeader="true">
                                    <ui:staticText text="#{clientTable.value.OS}"/>
                                </ui:tableColumn>
                                <ui:tableColumn id="colMounted"
                                                headerText="#{samBundle['SharedFS.client.table.heading.mounted']}"
                                                align="left"
                                                valign="top"
                                                sort="mounted"
                                                rowHeader="true">
                                    <ui:staticText text="#{clientTable.value.mounted}"
                                                   escape="false"
                                                   converter="TimeInSecConverter"/>
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
                </ui:form>
            </ui:body>
        </ui:html>
    </ui:page>
</f:view>
</jsp:root>
