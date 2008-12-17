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

<!-- $Id: MultiHostStatusDisplay.jsp,v 1.4 2008/12/17 20:18:55 kilemba Exp $ -->


<jsp:root version="1.2"
   xmlns:f="http://java.sun.com/jsf/core"
   xmlns:h="http://java.sun.com/jsf/html"
   xmlns:ui="http://www.sun.com/web/ui"
   xmlns:jsp="http://java.sun.com/JSP/Page">
   <jsp:directive.page contentType="text/html;charset=utf-8" pageEncoding="UTF-8"/>

<f:view>
<f:loadBundle basename="com.sun.netstorage.samqfs.web.resources.Resources"
              var="msgs"/>
<ui:head title="#{MultiHoststatusBean.titleText}">
   <ui:script url="/js/samqfsui.js"/>
   <ui:script url="/js/fs/multihoststatus.js"/>

   <ui:meta httpEquiv="refresh" content="10"/>
</ui:head>

<ui:body styleClass="defBody" id="mhs">
<ui:form id="MultiHostStatusForm">
<ui:masthead id="Masthead"
             secondary="true"
             productImageURL="/com_sun_web_ui/images/SecondaryProductName.png"
             productImageDescription=""
             productImageWidth="162"
             productImageHeight="40" />

       <div style="margin:10px">
            <ui:label id="title" text="#{MultiHostStatusBean.titleText}"/>
       </div>
       
       <h:panelGrid columns="2" width="30%" style="margin:10px">
           <ui:label for="totalText" text="#{msgs['fs.multihoststatus.total']}"/>
           <ui:staticText id="totalText" text="#{MultiHostStatusBean.total}"/>
           
           <ui:label for="succeededText" text="#{msgs['fs.multihoststatus.succeeded']}"/>
           <ui:staticText id="succeededText" text="#{MultiHostStatusBean.succeeded}"/>
           
           <ui:label for="failedText" text="#{msgs['fs.multihoststatus.failed']}"/>
           <ui:staticText id="failedText" text="#{MultiHostStatusBean.failed}"/>
           
           <ui:label for="pendingText" text="#{msgs['fs.multihoststatus.pending']}"/>
           <ui:staticText id="pendingText" text="#{MultiHostStatusBean.pending}"/>
       </h:panelGrid>

       <table style="margin:10px">
        <tr><td style="width:50%">
           <ui:label text="#{MultiHostStatusBean.failedHostLabel}"/>
        </td><td>
           <ui:label text="#{msgs['fs.multihoststatus.hosterrordetail']}"/>
        </td></tr>

        <tr><td>
           <ui:listbox id="failedHostList"
                       onChange="return hostWithErrorSelected(this);"
                       items="#{MultiHostStatusBean.failedHostList}"/>
        </td><td style="vertical-align:top">
           <ui:staticText id="hostErrorDetail"/>
        </td></tr>
       </table>

       <div style="margin:30px">
           <ui:button id="closeButton"
                      text="#{msgs['common.button.close']}"
                      onClick="return handleCloseButton(this);"/>
       </div>

   <ui:hiddenField id="serverName" value="#{MultiHostStatusBean.serverName}" />
   <ui:hiddenField id="jobId" value="#{MultiHostStatusBean.jobId}" />
   <ui:hiddenField id="hostError" value="#{MultiHostStatusBean.hostErrorList}"/>
</ui:form>
</ui:body>
</f:view>
</jsp:root>
