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

<!-- $Id: ArchiveScanJobDetails.jsp,v 1.1 2008/05/09 21:08:57 kilemba Exp $ -->
                       
<jsp:root version="1.2"
    xmlns:f="http://java.sun.com/jsf/core"
    xmlns:h="http://java.sun.com/jsf/html"
    xmlns:ui="http://www.sun.com/web/ui"
    xmlns:jsp="http://java.sun.com/JSP/Page">
    <jsp:directive.page contentType="text/html;charset=ISO-8859-1" pageEncoding="UTF-8"/>
        
    <ui:label for="fsname" text="#{samBundle['job.details.fsname']}"/>
    <ui:staticText id="fsname" text="#{JobController.archiveScanJob.fileSystemName}"/>

    <ui:table id="scanJobDataTable"
                title="#{samBundle['job.details.scan.table.title']}"    
                style="margin:10px">
        <ui:tableRowGroup id="scanDataRows"
                            sourceData="#{JobController.scanJobData}"
                            sourceVar="scanData">
            <ui:tableColumn id="fileType" headerText="#{samBundle['job.details.scan.table.filetype']}">
                <ui:staticText text="xyz"/>
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
</jsp:root>
