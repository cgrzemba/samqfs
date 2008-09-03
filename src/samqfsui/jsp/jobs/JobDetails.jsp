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

<!-- $Id: JobDetails.jsp,v 1.5 2008/09/03 19:46:03 ronaldso Exp $ -->

<jsp:root version="1.2"
   xmlns:f="http://java.sun.com/jsf/core"
   xmlns:h="http://java.sun.com/jsf/html"
   xmlns:ui="http://www.sun.com/web/ui"
   xmlns:c="http://java.sun.com/jsp/jstl/core"
   xmlns:jsp="http://java.sun.com/JSP/Page">
   <jsp:directive.page contentType="text/html;charset=ISO-8859-1" 
                       pageEncoding="UTF-8"/>
   <f:view>
   <f:loadBundle basename="com.sun.netstorage.samqfs.web.resources.Resources"
                 var="samBundle"/>
                 
   <ui:page>
   <ui:html>
   <ui:head title="#{samBundle['node.admin.job']}"/>
   <ui:body onLoad="if (parent.serverName != null) {
                        parent.setSelectedNode('113', 'JobDetails');
                    }">
   <ui:form id="JobDetailsForm">
       <ui:contentPageTitle title="#{samBundle['job.details.title']}" />
       <h:panelGrid columns="2" style="margin-left:10px" cellspacing="10">
           <ui:label text="#{samBundle['job.details.id']}" for="jobId"/>
           <ui:staticText id="jobId" text="#{JobController.job.jobId}"/>
           
           <ui:label text="#{samBundle['job.details.type']}" for="jobType"/>
           <ui:staticText id="jobType" text="#{JobController.job.type}" />
        
           <ui:label for="url" text="Page URL" />
           <ui:staticText id="url" text="#{JobController.jobDetailsPageUrl}"/>
       </h:panelGrid>

       <!--- redefine the managed bean here for the benefit of the older tags-->
       <jsp:useBean id="JobController"
                    class="com.sun.netstorage.samqfs.web.jobs.JobController"
                    scope="request"/>
       <c:set var="url" value="${JobController.jobDetailsPageUrl}"/>
       <jsp:include page="${url}"/>
   </ui:form>
   </ui:body>
   </ui:html>
   </ui:page>
   </f:view>
</jsp:root>
