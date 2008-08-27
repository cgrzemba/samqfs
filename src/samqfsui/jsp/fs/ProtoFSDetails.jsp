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

<!-- $Id: ProtoFSDetails.jsp,v 1.3 2008/08/27 22:17:27 kilemba Exp $ -->


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

<ui:page>
<ui:html>
<ui:head title="#{msgs['protofs.title']}">
    <style>
        .taskhref{font-size:1.25em}                        
    </style>

    <ui:script url="/js/popuphelper.js"/>
    <ui:script url="/js/fs/protofs.js"/>
</ui:head>

<ui:body>
<ui:form id="ProtoFSDetailsForm">
    <ui:breadcrumbs id="breadcrumbs">
        <ui:hyperlink url="/fs/FSSummary" text="#{msgs['FSSummary.title']}">
            <f:param name="SERVER_NAME" value="#{ProtoFSBean.serverName}"/>
        </ui:hyperlink>
        <ui:hyperlink url="ProtoFSDetails.jsp" text="#{msgs['protofs.title']}"/>
    </ui:breadcrumbs>
    <ui:contentPageTitle id="pageTitle" title="#{ProtoFSBean.pageTitleText}"/>

    <div id="instructions" style="margin:10px 10px 0px 10px">
        <ui:staticText id="instruction1"
                       text="#{msgs['protofs.details.instruction1']}"/>
        <br/><br/>
        <ui:staticText id="instruction2"
                       text="#{msgs['protofs.details.instruction2']}"/>
        <br/>
        <ui:legend position="right" style="margin-right:30px"/>
    </div>

    <!-- Task list -->
    <div id="tasks" style="margin:30px 10px 20px 30px">
    
    <span style="margin-left:10px">
    <ui:hyperlink id="aboutHPCHref" action="success" url="HPCFSOverview.jsp">
        <f:param name="SERVER_NAME" value="#{ProtoFSBean.serverName}"/>
        <ui:staticText id="aboutHPCText"
                       text="#{msgs['protofs.details.abouthpc']}"
                       styleClass="taskhref"/>
    </ui:hyperlink>
    </span>

    <div style="margin:30px 0px 30px 0px">
        <ui:hyperlink id="addStorageNodeHref"
                      toolTip="#{msgs['protofs.details.addstoragenode.tooltip']}"
                      action="success">
            <ui:image url="/images/required.gif"/>
            <ui:staticText id="addStorageNodeText"
                           text="#{msgs['protofs.details.addstoragenode']}"
                           styleClass="taskhref"/>
        </ui:hyperlink> <br/>
    
        <div style="margin-left:30px">
            <ui:staticText id="addStorageNodeHelp"
                           rendered="true"
                           text="#{ProtoFSBean.addStorageNodeHelpText}"/>
            <br/>
            <ui:hyperlink id="viewStorageNodes" action="success" rendered="false">
                <ui:staticText text="#{msgs['protofs.details.viewstoragenodes']}"/>
            </ui:hyperlink>
        </div>
    </div>

    <div style="margin:0px 0px 30px 0px">
        <ui:hyperlink id="createFSHref" action="success">
            <ui:image url="/images/required.gif"/>
            <ui:staticText id="createFSText"
                           text="#{msgs['protofs.details.createfs']}"
                           styleClass="taskhref"/>
        </ui:hyperlink>
        <br/>
        <span style="margin-left:30px">
            <ui:staticText id="createFSHelp"
                           rendered="true"
                           text="#{ProtoFSBean.createFSHelpText}"/>
        </span>
    </div>
    
    <ui:hyperlink id="addClientsHref"
                  url="wizards/AddClientsWizard.jsp"
                  onClick="return launchAddClientsWizard(this);"
                  action="success">
        <ui:image url="/images/required.gif"/>
        <ui:staticText id="addClientsText"
                       text="#{msgs['protofs.details.addclients']}"
                       styleClass="taskhref"/>
    </ui:hyperlink><br/>

    <div style="margin-left:30px">
        <ui:staticText id="addClientsHelp"
                       rendered="true"
                       text="#{ProtoFSBean.addClientsHelpText}"/>
        <br/>
        <ui:hyperlink id="viewClients" action="success" rendered="false">
            <ui:staticText text="#{msgs['protofs.details.viewclients']}"/>
        </ui:hyperlink>
    </div>
    
    </div>
    
    <!-- cancel button -->
    <div style="margin:40px 40px 0px 40px;text-align:right">
        <ui:button id="cancel"
                   rendered="false"
                   toolTip="#{msgs['protofs.details.cancel.tooltip']}"
                   text="#{msgs['protofs.details.cancelbutton']}"/>
        <ui:button id="finish"
                   rendered="true"
                   text="#{msgs['common.button.finish']}"/>
    </div>

    <ui:hiddenField id="serverName" value="#{ProtoFSBean.serverName}"/>
</ui:form>
</ui:body>
</ui:html>
</ui:page>
</f:view>
</jsp:root>
