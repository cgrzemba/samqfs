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
<!--Copyright 2009 Sun Microsystems, Inc.  All rights reserved.         -->
<!--Use is subject to license terms.                                      -->
<!--                                                                      -->
<!--  SAM-QFS_notice_end                                                  -->
<!--                                                                      -->

<!-- $Id: ArchiveScanJobDetails.jsp,v 1.4 2008/12/16 00:10:47 am143972 Exp $ -->
                       
<jsp:root version="1.2"
    xmlns:f="http://java.sun.com/jsf/core"
    xmlns:h="http://java.sun.com/jsf/html"
    xmlns:ui="http://www.sun.com/web/ui"
    xmlns:jsp="http://java.sun.com/JSP/Page">
    <jsp:directive.page contentType="text/html;charset=ISO-8859-1" pageEncoding="UTF-8"/>

<f:view>
<f:loadBundle basename="com.sun.netstorage.samqfs.web.resources.Resources" var="samBundle" />
<ui:page>
<ui:html>
<ui:head title="#{samBundle['node.admin.job']}"/>
<ui:body>
<ui:form id="ArchiveScanJobDetailsForm">
    <ui:breadcrumbs id="breadcrumbs" style="margin-bottom:20px">
       <ui:hyperlink url="Jobs.jsp" text="#{samBundle['admin.jobs.summary.pagetitle']}"/>
       <ui:hyperlink url="Jobs.jsp" text="#{samBundle['job.details.title']}"/>		      
    </ui:breadcrumbs>	  

    <ui:contentPageTitle title="#{samBundle['job.details.title']}"/>

    <h:panelGrid columns="2" style="margin-left:20px" cellspacing="10">
    <ui:label text="#{samBundle['job.details.id']}" for="jobId"/>
    <ui:staticText id="jobId" text="#{JobController.job.jobId}"/>

    <ui:label text="#{samBundle['job.details.type']}" for="jobType"/>
    <ui:staticText id="jobType" text="#{JobController.job.type}" converter="JobTypeConverter"/>
    </h:panelGrid>

    <ui:table id="scanJobDataTable" title="#{samBundle['job.details.scan.table.title']}" style="margin:10px">
        <ui:tableRowGroup id="scanDataRows" sourceData="#{JobController.scanJobData}" sourceVar="scanData">
            <ui:tableColumn id="fileType" headerText="#{samBundle['job.details.scan.table.filetype']}">
                <ui:staticText text="#{scanData.value.fileTypeLabel}"/>
            </ui:tableColumn>

            <ui:tableColumn id="totalfiles" headerText="#{samBundle['job.details.scan.table.totalfiles']}">
                <ui:staticText text="#{scanData.value.totalNoOfFiles}"/>
            </ui:tableColumn>

            <ui:tableColumn id="totalspace" headerText="#{samBundle['job.details.scan.table.totalspace']}">
                <ui:staticText text="#{scanData.value.totalConsumedSpace}"/>
            </ui:tableColumn>

            <ui:tableColumn id="currentfiles" headerText="#{samBundle['job.details.scan.table.currentfiles']}">
                <ui:staticText text="#{scanData.value.noOfCompletedFiles}"/>
            </ui:tableColumn>

            <ui:tableColumn id="currentspace" headerText="#{samBundle['job.details.scan.table.currentspace']}">
                <ui:staticText text="#{scanData.value.currentConsumedSpace}"/>
            </ui:tableColumn>
        </ui:tableRowGroup>
    </ui:table>

</ui:form>
</ui:body>
</ui:html>
</ui:page>
</f:view>
</jsp:root>
