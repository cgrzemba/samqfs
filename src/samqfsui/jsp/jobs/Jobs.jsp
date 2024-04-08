<!--  SAM-QFS_notice_begin

    CDDL HEADER START

    The contents of this file are subject to the terms of the
    Common Development and Distribution License (the "License").
    You may not use this file except in compliance with the License.

    You can obtain a copy of the license at pkg/OPENSOLARIS.LICENSE
    or https://illumos.org/license/CDDL.
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

<!-- $Id: Jobs.jsp,v 1.7 2008/12/17 20:18:56 kilemba Exp $ -->

<jsp:root version="1.2"
   xmlns:f="http://java.sun.com/jsf/core"
   xmlns:h="http://java.sun.com/jsf/html"
   xmlns:ui="http://www.sun.com/web/ui"
   xmlns:jsp="http://java.sun.com/JSP/Page">
   <jsp:directive.page contentType="text/html;charset=utf-8"
                       pageEncoding="UTF-8"/>
   <f:view>
   <f:loadBundle basename="com.sun.netstorage.samqfs.web.resources.Resources"
                 var="samBundle"/>

   <ui:page>
   <ui:html>
   <ui:head title="#{samBundle['node.admin.jobs']}"/>
   <ui:body onLoad="if (parent.serverName != null) {
                        parent.setSelectedNode('113', 'JobsSummary');
                    }">
   <ui:form id="JobsForm">

    <!-- page alert -->
    <!--
    <ui:alert id="alert"
              rendered="#{JobsBean.showAlert}"
              type="#{JobsBean.alertType}"
              summary="#{JobsBean.alertSummary}"
              detail="#{JobsBean.alertDetails}"/>
    -->

    <ui:contentPageTitle id="pageTitle"
                         title="#{samBundle['admin.jobs.summary.pagetitle']}"/>

    <ui:table id="jobsTable"
              augmentTitle="true"
              itemsText="#{samBundle['node.admin.job']}"
              title="#{samBundle['admin.jobs.table.title']}"
              deselectSingleButton="true"
              deselectSingleButtonOnClick="xxdeselect javascriptxxx"
              style="margin:10px">

    <!-- action buttons -->
    <f:facet name="actionsTop">
    <f:subview id="actionsTop">
        <ui:button id="cancel"
                   text="#{samBundle['common.button.cancel']}"/>
    </f:subview>
    </f:facet>

    <!-- filter -->
    <!--
    <f:facet name="filter">
        <ui:dropDown submitForm="true" id="filter"
                     action="handleFilterChange"
                     items="xxfilter itemsxx"
                     onChange="xxx javascript functionxxx"
                     selected="xxdefault filter namexx" />
    </f:facet>
    -->
    <!-- the table column definition -->
    <ui:tableRowGroup id="jobsRows"
                      selected="#{JobController.select.selectedState}"
                      binding="#{JobController.tableRowGroup}"
                      sourceData="#{JobController.jobList}"
                      sourceVar="jobs">

        <ui:tableColumn id="selection" selectId="radio"
                        sort="#{JobController.select.selectedState}">
            <ui:radioButton id="radio"
                            name="same"
                            selected="#{JobController.select.selected}"
                            selectedValue="#{JobController.select.selectedValue}"/>
        </ui:tableColumn>

        <ui:tableColumn id="id" headerText="#{samBundle['admin.jobs.table.id']}">
            <ui:hyperlink id="idLink" action="#{JobController.handleJobHref}">
                <ui:staticText id="idvalue"  text="#{jobs.value.jobId}"/>
            </ui:hyperlink>
        </ui:tableColumn>

        <ui:tableColumn id="type"
                        headerText="#{samBundle['admin.jobs.table.type']}">
            <ui:staticText id="typeText" text="#{jobs.value.type}" converter="JobTypeConverter"/>
        </ui:tableColumn>

        <ui:tableColumn id="initTime"
                        headerText="#{samBundle['admin.jobs.table.inittime']}">
            <ui:staticText id="initTimeText" text="#{jobs.value.startTime}">
                <f:convertDateTime dateStyle="medium" timeStyle="medium"/>
            </ui:staticText>
        </ui:tableColumn>

        <ui:tableColumn id="description"
                        headerText="#{samBundle['admin.jobs.table.description']}">
            <ui:staticText id="descriptionText" text="#{jobs.value.description}"/>
        </ui:tableColumn>

        <ui:tableColumn id="status"
                        headerText="#{samBundle['admin.jobs.table.status']}">
            <ui:staticText id="statusText" text="#{jobs.value.condition}" converter="JobStatusConverter" />
        </ui:tableColumn>
    </ui:tableRowGroup>
    </ui:table>
   </ui:form>
   </ui:body>
   </ui:html>
   </ui:page>
   </f:view>
</jsp:root>
