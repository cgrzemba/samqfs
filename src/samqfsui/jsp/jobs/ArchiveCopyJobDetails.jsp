<!--  SAM-QFS_notice_begin                                                -->
<!--                                                                      -->
<!--CDDL HEADER START                                                     -->
<!--                                                                      -->
<!--The contents of this file are subject to the terms of the             -->
<!--Common Development and Distribution License (the "License").          -->
<!--You may not use this file except in compliance with the License.      -->
<!--                                                                      -->
<!--You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE   -->
<!--or http://www.opensolaris.org/os/licensing.                           -->
<!--See the License for the specific language governing permissions       -->
<!--and limitations under the License.                                    -->
<!--                                                                      -->
<!--When distributing Covered Code, include this CDDL HEADER in each      -->
<!--file and include the License file at usr/src/OPENSOLARIS.LICENSE.     -->
<!--If applicable, add the following below this CDDL HEADER, with the     -->
<!--fields enclosed by brackets "[]" replaced with your own identifying   -->
<!--information: Portions Copyright [yyyy] [name of copyright owner]      -->
<!--                                                                      -->
<!--CDDL HEADER END                                                       -->
<!--                                                                      -->
<!--Copyright 2008 Sun Microsystems, Inc.  All rights reserved.           -->
<!--Use is subject to license terms.                                      -->
<!--                                                                      -->
<!--  SAM-QFS_notice_end                                                  -->
<!--                                                                      -->

<!-- $Id: ArchiveCopyJobDetails.jsp,v 1.1 2008/05/09 21:08:57 kilemba Exp $ -->

<jsp:root version="1.2"
    xmlns:f="http://java.sun.com/jsf/core"
    xmlns:h="http://java.sun.com/jsf/html"
    xmlns:ui="http://www.sun.com/web/ui"
    xmlns:jsp="http://java.sun.com/JSP/Page">
    <jsp:directive.page contentType="text/html;charset=ISO-8859-1" pageEncoding="UTF-8"/>

    <ui:label for="fsname" text="#{samBundle['job.details.fsname']}"/>
    <ui:staticText id="fsname" text="#{JobController.archiveCopyJob.fileSystemName}"/>

    <ui:label for="policyname" text="#{samBundle['job.details.policyname']}"/>
    <ui:staticText id="policyname" text="#{JobController.archiveCopyJob.policyName}"/>

    <ui:label for="copynumber" text="#{samBundle['job.details.copynumber']}"/>
    <ui:staticText id="copynumber" text="#{JobController.archiveCopyJob.copyNumber}"/>

    <ui:label for="vsn" text="#{samBundle['job.details.vsn']}"/>
    <ui:staticText id="vsn" text="#{JobController.archiveCopyJob.VSNName}"/>

    <ui:label for="mediatype" text="#{samBundle['job.details.mediatype']}"/>
    <ui:staticText id="mediatype" text="#{JobController.archiveCopyJob.mediaType}"/>

    <ui:label for="inittime" text="#{samBundle['job.details.initializationtime']}"/>
    <ui:staticText id="inittime" text="#{JobController.archiveCopyJob.startTime}"/>

    <ui:label for="totalfiles" text="#{samBundle['jobs.details.totalfiles']}"/>
    <ui:staticText id="totalfiles" text="#{JobController.archiveCopyJob.totalNoOffilesToBeCopied}"/>

    <ui:label for="datavolume" text="#{samBundle['job.details.datavolume']}"/>
    <ui:staticText id="datavolume" text="#{JobController.archiveCopyJob.dataVolumeToBeCopied}"/>
</jsp:root>






































