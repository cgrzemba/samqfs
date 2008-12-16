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

<!-- $Id: AddClientsWizard.jsp,v 1.6 2008/12/16 00:10:47 am143972 Exp $ -->


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
<ui:head title="#{msgs['fs.addclients.title']}">
    <ui:script url="/js/samqfsui.js"/>
    <ui:script url="/js/fs/wizards/addclients.js"/>
</ui:head>
<ui:body styleClass="DefBody"
         onLoad="wizOnLoad('AddclientsWizardForm:AddClientsWizard');">

<ui:form id="AddClientsWizardForm">
<ui:masthead id="Masthead" secondary="true"
             productImageURL="/com_sun_web_ui/images/SecondaryProductName.png"
             productImageDescription=""
             productImageWidth="162"
             productImageHeight="40"/>
<ui:wizard id="AddClientsWizard"
           eventListener="#{AddClientsBean.wizardEventListener}"
           title="#{msgs['fs.addclients.title']}"
           onPopupDismiss="dismissWizard();">
    <ui:wizardStep id="selectionMethodPage"
                   summary="#{msgs['fs.addclients.selectionmethod.summary']}"
                   title="#{msgs['fs.addclients.selectionmethod.title']}"
                   help="#{msgs['fs.addclients.selectionmethod.help']}"
                   detail="#{msgs['fs.addclients.selectionmethod.detail']}">

        <ui:alert id="alert"
                  rendered="#{AddClientsBean.alertRendered}"
                  type="#{AddClientsBean.alertType}"
                  summary="#{AddClientsBean.alertSummary}"
                  detail="#{AddClientsBean.alertDetail}" />

        <ui:radioButtonGroup items="#{AddClientsBean.hostSelectionItems}"
                     selected="#{AddClientsBean.selectedMethod}"
                     label="#{msgs['fs.addclients.selectionmethod.method']}" />
    </ui:wizardStep>
    
    <ui:wizardSubstepBranch taken="#{AddClientsBean.selectedMethod == 'hostname'}">
        <ui:wizardStep id="byhostnamePage"
                       summary="#{msgs['fs.addclients.hostname.summary']}"
                       title="#{msgs['fs.addclients.hostname.title']}"
                       help="#{msgs['fs.addclients.hostname.help']}"
                       detail="#{msgs['fs.addclients.hostname.detail']}">
        <ui:alert id="alert"
                  rendered="#{AddClientsBean.alertRendered}"
                  type="#{AddClientsBean.alertType}"
                  summary="#{AddClientsBean.alertSummary}"
                  detail="#{AddClientsBean.alertDetail}" />

            <jsp:include page="AddClientsHostSelection.jsp" />
        </ui:wizardStep>
    </ui:wizardSubstepBranch>
    
    <ui:wizardSubstepBranch taken="#{AddClientsBean.selectedMethod == 'ipaddress'}">
        <ui:wizardStep id="byipaddressPage"
                       summary="#{msgs['fs.addclients.byip.summary']}"
                       title="#{msgs['fs.addclients.byip.title']}"
                       help="#{msgs['fs.addclients.byip.help']}"
                       detail="#{msgs['fs.addclients.byip.detail']}">
        <ui:alert id="alert"
                  rendered="#{AddClientsBean.alertRendered}"
                  type="#{AddClientsBean.alertType}"
                  summary="#{AddClientsBean.alertSummary}"
                  detail="#{AddClientsBean.alertDetail}" />
        
            <jsp:include page="AddClientsHostSelection.jsp" />            
        </ui:wizardStep>
    </ui:wizardSubstepBranch>
    
    <ui:wizardSubstepBranch taken="#{AddClientsBean.selectedMethod == 'file'}">
        <ui:wizardStep id="fromFilePage"
                       summary="#{msgs['fs.addclients.file.summary']}"
                       title="#{msgs['fs.addclients.file.title']}"
                       help="#{msgs['fs.addclients.file.help']}"
                       detail="#{msgs['fs.addclients.file.detail']}">
        <ui:alert id="alert"
                  rendered="#{AddClientsBean.alertRendered}"
                  type="#{AddClientsBean.alertType}"
                  summary="#{AddClientsBean.alertSummary}"
                  detail="#{AddClientsBean.alertDetail}" />
                           
        <h:panelGrid columns="4">
            <ui:label for="filenameText"
                      text="#{msgs['fs.addclients.file.filename']}"/>
            <ui:textField id="filenameText" text="#{AddClientsBean.fileName}"/>

            <!--
            <ui:fileChooser id="filechooser"
                            lookin="#{AddClientsBean.fileBrowserDirectory}"
                            selected="#{AddClientsBean.selectedFile}" />
            <ui:button id="fileChooserButton"
                       primary="true"
                       text="#{msgs['common.button.browse']}"
                       actionListener="#{AddClientsBean.handleChooseFile}"/>
            -->
        </h:panelGrid>
        </ui:wizardStep>
    </ui:wizardSubstepBranch>
    
    <ui:wizardStep id="clientListPage"
                   summary="#{msgs['fs.addclients.clientlist.summary']}"
                   title="#{msgs['fs.addclients.clientlist.title']}"
                   help="#{msgs['fs.addclients.clientlist.help']}"
                   detail="#{msgs['fs.addclients.clientlist.detail']}">
        
        <ui:alert id="alert"
                  rendered="#{AddClientsBean.alertRendered}"
                  type="#{AddClientsBean.alertType}"
                  summary="#{AddClientsBean.alertSummary}"
                  detail="#{AddClientsBean.alertDetail}" />

        <div style="margin-left:10px">
        <ui:editableList id="editableHostList"
                        list="#{AddClientsBean.selectedHosts}"
                        fieldLabel="#{msgs['fs.addclients.hostname.hostname']}"
                        listLabel="#{msgs['fs.addclients.hostname.selectedhosts']}"/>
            <f:facet name="addButton">
                <ui:staticText id="add" rendered="false"/>
            </f:facet>
        </div>

    </ui:wizardStep>
    
    <ui:wizardStep id="mountOptionsPage"
                   summary="#{msgs['fs.addclients.mountoptions.summary']}"
                   title="#{msgs['fs.addclients.mountoptions.title']}"
                   help="#{msgs['fs.addclients.mountoptions.help']}"
                   detail="#{msgs['fs.addclients.mountoptions.detail']}">
        
        <ui:alert id="alert"
                  rendered="#{AddClientsBean.alertRendered}"
                  type="#{AddClientsBean.alertType}"
                  summary="#{AddClientsBean.alertSummary}"
                  detail="#{AddClientsBean.alertDetail}" />

    <table cellspacing="10">
    <tr><td>
        <ui:label for="mountPointText"
                  text="#{msgs['fs.addclients.mountoptions.mountpoint']}"/>
    </td><td>
        <ui:textField id="mountPointText" text="#{AddClientsBean.mountPoint}"/>
    </td></tr>
    
    <tr><td>
    </td><td>
        <ui:checkbox id="mountAfterAdd"
                  selected="#{AddClientsBean.mountAfterAdd}"
                  label="#{msgs['fs.addclients.mountoptions.mountafteradd']}"/>
    </td></tr>
    <tr><td/><td/></tr>
    <tr><td valign="top">
        <ui:label for="mountOptions"
                  text="#{msgs['fs.addclients.mountoptions.options']}"/>
    </td><td valign="top">
        <ui:checkboxGroup id="mountOptions"
                          items="#{AddClientsBean.mountOptionList}"
                          selected="#{AddClientsBean.selectedMountOptions}"/>
    </td></tr>
    
    <tr><td colspan="2" style="margin-top:10px">
        <ui:checkbox id="pmds"
                     label="#{msgs['fs.addclients.mountoptions.pmds']}"
                     selected="#{AddClientsBean.makePMDS}"/>
    </td></tr>
    </table>
    </ui:wizardStep>
    
    <ui:wizardStep id="reviewPage"
                   summary="#{msgs['fs.addclients.review.summary']}"
                   title="#{msgs['fs.addclients.review.title']}"
                   help="#{msgs['fs.addclients.review.help']}"
                   finish="true">

        <ui:alert id="alert"
                  rendered="#{AddClientsBean.alertRendered}"
                  type="#{AddClientsBean.alertType}"
                  summary="#{AddClientsBean.alertSummary}"
                  detail="#{AddClientsBean.alertDetail}" />

        <table cellspacing="10">
            <tr><td colspan="2" valign="top">
            <ui:label for="selectedHosts"
                      text="#{msgs['fs.addclients.hostname.selectedhosts']}"/>
            <ui:listbox id="selectedHosts"
                        items="#{AddClientsBean.selectedHostSummary}"/>
            </td></tr>

            <tr><td>
            <ui:label text="#{msgs['fs.addclients.mountoptions.mountpoint']}"/>
            </td><td>
            <ui:staticText text="#{AddClientsBean.mountPoint}"/>
            </td></tr>

            <tr><td>
            <ui:label text="#{msgs['fs.addclients.mountoptions.mountafteradd']}"/>
            </td><td>
            <ui:staticText text="#{AddClientsBean.mountAfterAddText}"/>
            </td></tr>
            
            <tr><td>
            <ui:label text="#{msgs['fs.addclients.mountoptions.readonly']}"/>
            </td><td>
            <ui:staticText text="#{AddClientsBean.mountReadOnlyText}"/>
            </td></tr>

            <tr><td>
            <ui:label text="#{msgs['fs.addclients.mountoptions.boottime']}"/>
            </td><td>
            <ui:staticText text="#{AddClientsBean.mountAtBootTimeText}"/>
            </td></tr>

            <tr><td>
            <ui:label text="#{msgs['fs.addclients.mountoptions.background']}"/>
            </td><td>
            <ui:staticText text="#{AddClientsBean.mountInTheBackgroundText}"/>
            </td></tr>

            <tr><td>
            <ui:label text="#{msgs['fs.addclients.mountoptions.pmds']}"/>
            </td><td>
            <ui:staticText text="#{AddClientsBean.PMDSText}"/>
            </td></tr>
        </table>
    </ui:wizardStep>

    <ui:wizardStep id="resultsPage"
                   summary="#{msgs['fs.addclients.results.summary']}"
                   title="#{msgs['fs.addclients.results.title']}"
                   help="#{msgs['fs.addclients.results.help']}"
                   results="true">

        <ui:alert id="alert"
                  rendered="#{AddClientsBean.alertRendered}"
                  type="#{AddClientsBean.alertType}"
                  summary="#{AddClientsBean.alertSummary}"
                  detail="#{AddClientsBean.alertDetail}" />

        <div style="margin:40px">
            <ui:hyperlink id="tomhs"
                          rendered="#{AddClientsBean.displayMHSLink}"
                          url="/faces/jsp/fs/MultiHostStatusDisplay.jsp">
                <f:param name="SERVER_NAME" value="#{AddClientsBean.serverName}" />
                <f:param name="SAMQFS_JOB_ID" value="#{AddClientsBean.jobId}" />
                <ui:staticText text="#{msgs['fs.addclients.results.mhs']}"/>
            </ui:hyperlink>
        </div>					  
    </ui:wizardStep>
</ui:wizard>
</ui:form>
</ui:body>
</ui:html>
</ui:page>
</f:view>
</jsp:root>
