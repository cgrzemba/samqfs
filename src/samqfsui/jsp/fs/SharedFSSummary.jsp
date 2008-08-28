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

<!-- $Id: SharedFSSummary.jsp,v 1.9 2008/08/28 02:01:34 ronaldso Exp $ -->

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

<ui:page id="page">
<ui:html id="html">
<ui:head id="head" title="#{samBundle['SharedFS.title']}">
    <ui:script url="/js/popuphelper.js"/>
    <ui:script url="/js/fs/SharedFS.js"/>
</ui:head>
<ui:body id="SharedFSBody"
         onLoad="
                if (parent.serverName != null) {
                    parent.setSelectedNode('0', 'SharedFS');
                }">
<ui:form id="SharedFSForm">
    <ui:breadcrumbs id="breadcrumbs" pages="#{SharedFSBean.breadCrumbsSummary}" />
    <ui:tabSet binding="#{SharedFSBean.tabSet}" selected="summary" />
    <ui:alert id="alert"
              rendered="#{SharedFSBean.alertRendered}"
              type="#{SharedFSBean.alertType}"
              summary="#{SharedFSBean.alertSummary}"
              detail="#{SharedFSBean.alertDetail}"/>
    <ui:contentPageTitle
        id="pageTitle"
        title="#{samBundle['SharedFS.title']}">
    <f:facet name="pageActions">
        <ui:panelGroup id="pageActionsGroup">
            <ui:button
                id="ButtonAddClients"
                text="#{samBundle['SharedFS.operation.addclients']}"
                primary="true"
                immediate="true"
                onClick="return launchAddClientsWizard(this);"/>
            <ui:button
                id="ButtonAddSN"
                text="#{samBundle['SharedFS.operation.addsn']}"
                primary="true"
                immediate="true"
                rendered="#{SharedFSBean.showStorageNodes}"
                onClick="launchAddStorageNodeWizard(this); return false;"/>
            <ui:button
                id="ButtonViewPolicies"
                text="#{samBundle['SharedFS.operation.viewpolicies']}"
                primary="true"
                immediate="true"
                rendered="#{SharedFSBean.showArchive}"
                actionListener="#{SharedFSBean.handleViewPolicies}" />
            <ui:dropDown
                id="Menu"
                toolTip="#{samBundle['SharedFS.dropdown.tooltip']}"
                submitForm="true"
                immediate="true"
                forgetValue="true"
                onChange="if (handleSummaryMenu(this) == false) return false;"
                selected="#{SharedFSBean.jumpMenuSelectedOption}"
                items="#{SharedFSBean.jumpMenuOptions}"
                actionListener="#{SharedFSBean.handleJumpMenuSelection}" />
        </ui:panelGroup>
    </f:facet>
    <ui:propertySheet
        id="propertySheet">

    <!-- file system property section -->
    <ui:propertySheetSection
        id="sectionDetails"
        label="">
    <ui:property
        id="propertyTextType"
        labelAlign="left"
        label="#{samBundle['SharedFS.label.type']}"
        noWrap="true"
        overlapLabel="false">
    <ui:staticText id="textType" text="#{SharedFSBean.textType}" />
    </ui:property>
    <ui:property
        id="propertyTextMountPoint"
        labelAlign="left"
        label="#{samBundle['SharedFS.label.mountpt']}"
        noWrap="true"
        overlapLabel="false">
    <ui:staticText id="textMountPoint" text="#{SharedFSBean.textMountPoint}" />
    </ui:property>
    <ui:property
        id="propertyTextCapacity"
        labelAlign="left"
        label="#{samBundle['SharedFS.label.cap']}"
        noWrap="true"
        overlapLabel="false">
    <ui:image id="imageUsage" hspace="0" url="#{SharedFSBean.imageUsage}"/>
    <ui:staticText id="textCapacity" text="#{SharedFSBean.textCapacity}" />
    </ui:property>
    <ui:property
        id="propertyTextHWM"
        labelAlign="left"
        label="#{samBundle['SharedFS.label.hwm']}"
        noWrap="true"
        rendered="#{SharedFSBean.showArchive}"
        overlapLabel="false">
    <ui:staticText id="textHWM" text="#{SharedFSBean.textHWM}" />
    </ui:property>
    <ui:property
        id="propertyTextArchiving"
        labelAlign="left"
        label="#{samBundle['SharedFS.label.archiving']}"
        noWrap="true"
        rendered="#{SharedFSBean.showArchive}"
        overlapLabel="false">
    <ui:staticText id="textArchiving" text="#{SharedFSBean.textArchiving}" />
    </ui:property>
    <ui:markup tag="br" singleton="true"/>
    </ui:propertySheetSection>
    <!-- clients section -->
    <ui:propertySheetSection
        id="sectionClients"
        label="#{SharedFSBean.titleClients}">
    <ui:property
        id="propertyClientHelp"
        labelAlign="left"
        noWrap="false"
        overlapLabel="false"
        helpText="#{samBundle['SharedFS.help.clientsection']}"/>
    <ui:property
        id="propertyClientTable"
        labelAlign="left"
        noWrap="true"
        overlapLabel="false">
    <!-- Client Table -->
    <ui:table id="tableClients">
        <f:facet name="title">
            <ui:staticText text="#{samBundle['SharedFS.title.table.clients']}"/>
        </f:facet>
        <ui:tableRowGroup id="clientRows"
                          binding="#{SharedFSBean.clientTableRowGroup}"
                          sourceData="#{SharedFSBean.clientList}"
                          sourceVar="clients">

            <ui:tableColumn id="colAllClients"
                            headerText="#{samBundle['SharedFS.table.heading.allclients']}"
                            align="center"
                            rowHeader="true">
                <ui:imageHyperlink id="imagehyperlinkClient"
                                   imageURL="/images/client.png"
                                   url="/faces/jsp/fs/SharedFSClient.jsp?filter="
                                   hspace="2"
                                   vspace="0"
                                   immediate="true" />
                <ui:hyperlink id="viewAllClientsLink"
                              url="/faces/jsp/fs/SharedFSClient.jsp"
                              immediate="true"
                              text="#{samBundle['SharedFS.text.viewall']}">
                    <f:param name="filter" value="#{SharedFSBean.paramCriteriaAll}"/>
                </ui:hyperlink>
            </ui:tableColumn>
            <ui:tableColumn id="colOK"
                            headerText="#{samBundle['SharedFS.table.heading.ok']}"
                            align="center"
                            rowHeader="true">
                <ui:hyperlink id="viewOkClientsLink"
                              url="/faces/jsp/fs/SharedFSClient.jsp"
                              immediate="true">
                    <ui:staticText text="#{clients.value.ok}"/>
                    <f:param name="filter" value="#{SharedFSBean.paramCriteriaOk}"/>
                </ui:hyperlink>
            </ui:tableColumn>
            <ui:tableColumn id="colUnmounted"
                            headerText="#{samBundle['SharedFS.table.heading.unmounted']}"
                            align="center"
                            rowHeader="true">
                <ui:hyperlink id="viewUnmountedClientsLink"
                              url="/faces/jsp/fs/SharedFSClient.jsp"
                              immediate="true">
                    <ui:staticText text="#{clients.value.unmounted}"/>
                    <f:param name="filter" value="#{SharedFSBean.paramCriteriaUnmounted}"/>
                </ui:hyperlink>
            </ui:tableColumn>
            <ui:tableColumn id="colDisabled"
                            headerText="#{samBundle['SharedFS.table.heading.disabled']}"
                            align="center"
                            rowHeader="true">
                <ui:hyperlink id="viewDisabledClientsLink"
                              url="/faces/jsp/fs/SharedFSClient.jsp"
                              immediate="true">
                    <ui:staticText text="#{clients.value.off}"/>
                    <f:param name="filter" value="#{SharedFSBean.paramCriteriaOff}"/>
                </ui:hyperlink>
            </ui:tableColumn>
            <ui:tableColumn id="colInError"
                            headerText="#{samBundle['SharedFS.table.heading.inerror']}"
                            align="center"
                            rowHeader="true">
                <ui:hyperlink id="viewErrorClientsLink"
                              url="/faces/jsp/fs/SharedFSClient.jsp"
                              immediate="true">
                    <ui:staticText text="#{clients.value.error}"/>
                    <f:param name="filter" value="#{SharedFSBean.paramCriteriaInError}"/>
                </ui:hyperlink>
            </ui:tableColumn>
            <ui:tableColumn id="colSpacer" spacerColumn="true" width="40%"/>
        </ui:tableRowGroup>
    </ui:table>
    </ui:property>
    </ui:propertySheetSection>
    <!-- storage nodes section -->
    <ui:propertySheetSection
        id="sectionStorageNodes"
        label="#{SharedFSBean.titleStorageNodes}"
        rendered="#{SharedFSBean.showStorageNodes}">
    <ui:property
        id="propertySNHelp"
        labelAlign="left"
        noWrap="false"
        overlapLabel="false"
        helpText="#{samBundle['SharedFS.help.snsection']}"/>
    <ui:property
        id="propertyStorageNodesTable"
        labelAlign="left"
        noWrap="true"
        overlapLabel="false">
    <!-- Storage Node Table -->
    <ui:table id="tableStorageNodes">
        <f:facet name="title">
            <ui:staticText text="#{samBundle['SharedFS.title.table.sns']}"/>
        </f:facet>
        <ui:tableRowGroup id="snsRows"
                          binding="#{SharedFSBean.snTableRowGroup}"
                          sourceData="#{SharedFSBean.storageNodeList}"
                          sourceVar="sns">

            <ui:tableColumn id="colAllClients"
                            headerText="#{samBundle['SharedFS.table.heading.allsns']}"
                            align="center"
                            rowHeader="true">
                <ui:imageHyperlink id="imagehyperlinkSN"
                                   imageURL="/images/storagenode.png"
                                   url="/faces/jsp/fs/SharedFSStorageNode.jsp?filter="
                                   hspace="2"
                                   vspace="0"
                                   immediate="true" />
                <ui:hyperlink id="viewAllSnsLink"
                              url="/faces/jsp/fs/SharedFSStorageNode.jsp"
                              immediate="true"
                              text="#{samBundle['SharedFS.text.viewall']}">
                    <f:param name="filter" value="0"/>
                </ui:hyperlink>
            </ui:tableColumn>
            <ui:tableColumn id="colOK"
                            headerText="#{samBundle['SharedFS.table.heading.ok']}"
                            align="center"
                            rowHeader="true">
                <ui:hyperlink id="viewOkSnssLink"
                              url="/faces/jsp/fs/SharedFSStorageNode.jsp"
                              immediate="true">
                    <ui:staticText text="#{sns.value.ok}"/>
                    <f:param name="filter" value="1"/>
                </ui:hyperlink>
            </ui:tableColumn>
            <ui:tableColumn id="colUnmounted"
                            headerText="#{samBundle['SharedFS.table.heading.unmounted']}"
                            align="center"
                            rowHeader="true">
                <ui:hyperlink id="viewUnmountedSnsLink"
                              url="/faces/jsp/fs/SharedFSStorageNode.jsp"
                              immediate="true">
                    <ui:staticText text="#{sns.value.unmounted}"/>
                    <f:param name="filter" value="2"/>
                </ui:hyperlink>
            </ui:tableColumn>
            <ui:tableColumn id="colDisabled"
                            headerText="#{samBundle['SharedFS.table.heading.disabled']}"
                            align="center"
                            rowHeader="true">
                <ui:hyperlink id="viewDisabledSnsLink"
                              url="/faces/jsp/fs/SharedFSStorageNode.jsp"
                              immediate="true">
                    <ui:staticText text="#{sns.value.off}"/>
                    <f:param name="filter" value="3"/>
                </ui:hyperlink>
            </ui:tableColumn>
            <ui:tableColumn id="colInError"
                            headerText="#{samBundle['SharedFS.table.heading.faults']}"
                            align="center"
                            rowHeader="true">
                <ui:hyperlink id="viewErrorSnsLink"
                              url="/faces/jsp/fs/SharedFSStorageNode.jsp"
                              immediate="true">
                    <ui:staticText text="#{sns.value.error}"/>
                    <f:param name="filter" value="5"/>
                </ui:hyperlink>
            </ui:tableColumn>
            <ui:tableColumn id="colSpacer" spacerColumn="true" width="40%"/>
        </ui:tableRowGroup>
    </ui:table>
    </ui:property>
    </ui:propertySheetSection>
    </ui:propertySheet>
    </ui:contentPageTitle>
    <ui:hiddenField id="Time" value="#{SharedFSBean.timeSummary}"/>
    <ui:hiddenField id="hiddenServerName" value="#{SharedFSBean.hiddenServerName}"/>
    <ui:hiddenField id="hiddenFSName" value="#{SharedFSBean.hiddenFSName}"/>
    <ui:hiddenField id="hiddenMountPoint" value="#{SharedFSBean.hiddenMountPoint}"/>
    <ui:hiddenField id="hiddenIsMDSMounted" value="#{SharedFSBean.hiddenIsMDSMounted}"/>
    <ui:hiddenField id="ConfirmUnmountFS" value="#{SharedFSBean.confirmUnmountFS}"/>
</ui:form>
</ui:body>
</ui:html>
</ui:page>
</f:view>
</jsp:root>
