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

<!-- $Id: HPCFSOverview.jsp,v 1.2 2008/09/17 23:33:23 kilemba Exp $ -->


<jsp:root version="1.2"
   xmlns:f="http://java.sun.com/jsf/core"
   xmlns:h="http://java.sun.com/jsf/html"
   xmlns:ui="http://www.sun.com/web/ui"
   xmlns:jsp="http://java.sun.com/JSP/Page">
   <jsp:directive.page contentType="text/html;charset=ISO-8859-1" 
                       pageEncoding="UTF-8"/>


<f:view>
<f:loadBundle basename="com.sun.netstorage.samqfs.web.resources.Resources"
              var="msgs" />

<%-- TODO: Link this page from first time config instead of removing it --%>
<ui:page>
<ui:html>
<ui:head title="#{msgs['hpcoverview.title']}"/>
<ui:body>
<ui:form id="HPCFSOverviewForm">
    <ui:breadcrumbs>
        <ui:hyperlink url="/fs/FSSummary" text="#{msgs['FSSummary.title']}">
            <f:param name="SERVER_NAME" value="#{ProtoFSBean.serverName}"/>
        </ui:hyperlink>
        <ui:hyperlink url="ProtoFSDetails.jsp" text="#{msgs['protofs.title']}">
            <f:param name="SERVER_NAME" value="#{ProtoFSBean.serverName}"/>
        </ui:hyperlink>
        <ui:hyperlink url="HPCFSOverview.jsp" text="#{msgs['hpcoverview.title']}"/>
    </ui:breadcrumbs>

    <div style="margin:0px 20px 0px 0px;text-align:right">
        <ui:button id="backButton"
                   action="success"
                   rendered="false"
                   immediate="true"
                   text="#{msgs['hpcoverview.backbutton']}"/>
    </div>


    <ui:contentPageTitle id="pageTitle"
                         title="#{msgs['hpcoverview.page.title']}"/>

    <div style="margin:10px 10px 0px 10px">
        <ui:staticText id="instruction1"
                       text="#{msgs['hpcoverview.instruction1']}"/>
        <br/><br/>
        <ui:staticText id="instruction2"
                       text="#{msgs['hpcoverview.instruction2']}"/>
    </div>

    <!-- need a caption for this diagram -->
    <ui:image url="/images/hpc_overview.png"/>

</ui:form>
</ui:body>
</ui:html>
</ui:page>
</f:view>
</jsp:root>

