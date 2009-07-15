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

<!-- $Id: MountMediaJobDetails.jsp,v 1.4 2008/12/17 20:18:56 kilemba Exp $ -->
                       
<jsp:root version="1.2"
    xmlns:f="http://java.sun.com/jsf/core"
    xmlns:h="http://java.sun.com/jsf/html"
    xmlns:ui="http://www.sun.com/web/ui"
    xmlns:jsp="http://java.sun.com/JSP/Page">
    <jsp:directive.page contentType="text/html;charset=utf-8" pageEncoding="UTF-8"/>

<f:view>
<f:loadBundle basename="com.sun.netstorage.samqfs.web.resources.Resources" var="samBundle" />
<ui:page>
<ui:html>
<ui:head title="#{samBundle['node.admin.job']}"/>
<ui:body>
<ui:form id="MountMediaJobDetailsForm">
    <ui:breadcrumbs id="breadcrumbs" style="margin-bottom:20px">
       <ui:hyperlink url="Jobs.jsp" text="#{samBundle['admin.jobs.summary.pagetitle']}"/>
       <ui:hyperlink url="Jobs.jsp" text="#{samBundle['job.details.title']}"/>		      
    </ui:breadcrumbs>	  

    <ui:contentPageTitle title="#{samBundle['job.details.title']}"/>

    <h:panelGrid columns="2" style="margin-left:20px" cellspacing="10">
        <ui:label for="jobId" text="#{samBundle['job.details.id']}"/>
        <ui:staticText id="jobId" text="#{JobController.mediaMountJob.jobId}"/>

        <ui:label for="type" text="#{samBundle['job.details.type']}"/>
        <ui:staticText id="type" text="#{JobController.mediaMountJob.type}" converter="JobTypeConverter"/>

        <ui:label for="vsn" text="#{samBundle['job.details.vsn']}"/>
        <ui:staticText id="vsn" text="#{JobController.mediaMountJob.VSNName}"/>

        <ui:label for="mediaType" text="#{samBundle['job.details.mediatype']}"/>
        <ui:staticText id="mediaType" text="#{JobController.mediaMountJob.mediaType}"/>

        <ui:label for="library" text="#{samBundle[job.details.library']}"/>
        <ui:staticText id="library" text="#{JobController.mediaMountJob.libraryName}"/>

        <ui:label for="archivestage" text="#{samBundle['job.details.archivestagerelated']}"/>
        <ui:staticText id="archivestage" text="#{JobController.mediaMountJob.isArchiveMount}"/>

        <ui:label for="pid" text="#{samBundle['job.details.pid']}"/>
        <ui:staticText id="pid" text="#{JobController.mediaMountJob.processId}"/>

        <ui:label for="user" text="#{samBundle['job.details.user']}"/>
        <ui:staticText id="user" text="#{JobController.mediaMountJob.initialitingUsername}"/>

        <ui:label for="initTime" text="#{samBundle['job.details.initializationtime']}"/>
        <ui:staticText id="initTime" text="#{JobController.mediaMountJob.startTime}">
            <f:convertDateTime dateStyle="medium" timeStyle="medium"/>
        </ui:staticText>

        <ui:label for="queuetime" text="#{samBundle['job.details.queuetime']}"/>
        <ui:staticText id="queuetime" text="#{JobController.mediaMountJob.timeInQueue}"/>
    </h:panelGrid>

</ui:form>
</ui:body>
</ui:html>
</ui:page>
</f:view>
</jsp:root>