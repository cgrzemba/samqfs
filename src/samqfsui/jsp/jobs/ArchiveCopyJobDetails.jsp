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

<!-- $Id: ArchiveCopyJobDetails.jsp,v 1.5 2008/12/17 20:18:56 kilemba Exp $ -->

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
<ui:form id="ArchiceCopyDetailsForm">
    <ui:breadcrumbs id="breadcrumbs" style="margin-bottom:20px">
       <ui:hyperlink url="Jobs.jsp" text="#{samBundle['admin.jobs.summary.pagetitle']}"/>
       <ui:hyperlink url="Jobs.jsp" text="#{samBundle['job.details.title']}"/>		      
    </ui:breadcrumbs>	  

    <ui:contentPageTitle title="#{samBundle['job.details.title']}"/>

    <h:panelGrid columns="2" style="margin:10px" cellspacing="10">
    <ui:label text="#{samBundle['job.details.id']}" for="jobId"/>
    <ui:staticText id="jobId" text="#{JobController.job.jobId}"/>

    <ui:label text="#{samBundle['job.details.type']}" for="jobType"/>
    <ui:staticText id="jobType" text="#{JobController.job.type}" converter="JobTypeConverter"/>

    <ui:label for="fsname" text="#{samBundle['job.details.fsname']}"/>
    <ui:staticText id="fsname" text="#{JobController.archiveCopyJob.fileSystemName}"/>

    <ui:label for="policyname" text="#{samBundle['job.details.copy.policyname']}"/>
    <ui:staticText id="policyname" text="#{JobController.archiveCopyJob.policyName}"/>

    <ui:label for="copynumber" text="#{samBundle['job.details.copy.copynumber']}"/>
    <ui:staticText id="copynumber" text="#{JobController.archiveCopyJob.copyNumber}"/>

    <ui:label for="vsn" text="#{samBundle['job.details.copy.vsn']}"/>
    <ui:staticText id="vsn" text="#{JobController.archiveCopyJob.VSNName}"/>

    <ui:label for="mediatype" text="#{samBundle['job.details.copy.mediatype']}"/>
    <ui:staticText id="mediatype" text="#{JobController.archiveCopyJob.mediaType}"/>

    <ui:label for="inittime" text="#{samBundle['job.details.copy.initializationtime']}"/>
    <ui:staticText id="inittime" text="#{JobController.archiveCopyJob.startTime}"/>

    <ui:label for="totalfiles" text="#{samBundle['job.details.copy.totalfiles']}"/>
    <ui:staticText id="totalfiles" text="#{JobController.archiveCopyJob.totalNoOfFilesToBeCopied}"/>

    <ui:label for="datavolume" text="#{samBundle['job.details.copy.datavolume']}"/>
    <ui:staticText id="datavolume" text="#{JobController.archiveCopyJob.dataVolumeToBeCopied}"/>
    </h:panelGrid>
</ui:form>
</ui:body>
</ui:html>
</ui:page>
</f:view>
</jsp:root>
