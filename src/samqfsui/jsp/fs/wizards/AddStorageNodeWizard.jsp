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

<!-- $Id: AddStorageNodeWizard.jsp,v 1.1 2008/07/16 17:09:30 ronaldso Exp $ -->

<jsp:root
    version="1.2"
    xmlns:f="http://java.sun.com/jsf/core"
    xmlns:h="http://java.sun.com/jsf/html"
    xmlns:ui="http://www.sun.com/web/ui"
    xmlns:jsp="http://java.sun.com/JSP/Page">
<jsp:directive.page
    contentType="text/html;charset=ISO-8859-1"
    pageEncoding="UTF-8"/>

<f:view>
    <f:loadBundle basename="com.sun.netstorage.samqfs.web.resources.Resources"
                  var="samBundle"/>

    <ui:page>
        <ui:html>
            <ui:head title="#{samBundle['addsn.title']}">
                <ui:script url="/js/fs/AddStorageNode.js"/>
            </ui:head>
            <ui:body styleClass="DefBdy"
                     onLoad="wizOnLoad('AddStorageNodeForm:AddStorageNodeWizard')">
                <ui:form id="AddStorageNodeForm">
                    <ui:masthead id="Masthead" secondary="true"
                                 productImageURL="/com_sun_web_ui/images/SecondaryProductName.png"
                                 productImageDescription=""
                                 productImageWidth="162"
                                 productImageHeight="40"/>

                    <ui:wizard id="AddStorageNodeWizard"
                               title="#{samBundle['addsn.title']}"
                               onPopupDismiss="handleWizardDismiss();"
                               eventListener="#{AddStorageNodeBean.wizardEventListener}">
                        <!-- Step 1: OverView -->
                        <ui:wizardStep id="step_id_overview"
                                       summary="#{samBundle['addsn.overview.steptitle']}"
                                       title="#{samBundle['addsn.overview.steptitle']}"
                                       detail="#{samBundle['addsn.overview.instruction.1']}">
                            <ui:staticText text="#{samBundle['addsn.overview.instruction.2']}" style="margin-left:2%" />
                            <ui:markup tag="br" singleton="true"/>
                            <h:panelGrid columns="2" style="margin-left:2%;align:left;width:95%">
                                <ui:staticText text="#{samBundle['common.asterisk']}"/>
                                <ui:staticText text="#{samBundle['addsn.overview.instruction.3']}"/>
                                <ui:staticText text="#{samBundle['common.asterisk']}"/>
                                <ui:staticText text="#{samBundle['addsn.overview.instruction.4']}"/>
                                <ui:staticText text="#{samBundle['common.asterisk']}"/>
                                <ui:staticText text="#{samBundle['addsn.overview.instruction.5']}"/>
                                <ui:staticText text="#{samBundle['common.asterisk']}"/>
                                <ui:staticText text="#{samBundle['addsn.overview.instruction.6']}"/>
                            </h:panelGrid>
                        </ui:wizardStep>
                        <!-- Step 2: Specify Host -->
                        <ui:wizardStep id="step_id_specify_host"
                                       summary="#{samBundle['addsn.specifyhost.steptitle']}"
                                       title="#{samBundle['addsn.specifyhost.steptitle']}"
                                       detail="#{samBundle['addsn.specifyhost.instruction']}">
                            <ui:legend id="legend" />
                            <ui:alert id="alert_specifyhost"
                                      rendered="#{AddStorageNodeBean.alertRendered}"
                                      type="#{AddStorageNodeBean.alertType}"
                                      summary="#{AddStorageNodeBean.alertSummary}"
                                      detail="#{AddStorageNodeBean.alertDetail}"/>
                            <ui:markup tag="br" singleton="true"/>
                            <ui:markup tag="p" extraAttributes="style='margin-left:2%'">
                                <ui:label id="labelHostNameIP"
                                          for="fieldHostNameIP"
                                          requiredIndicator="true"
                                          text="#{samBundle['addsn.specifyhost.label.hostnameip']}" />
                                <ui:textField id="fieldHostNameIP"
                                              required="true"
                                              trim="true"
                                              style="width:200px"
                                              validator="#{AddStorageNodeBean.validateHostNameIP}"
                                              text="#{AddStorageNodeBean.textHostNameIP}" />
                            </ui:markup>
                        </ui:wizardStep>
                        <!-- Step 3: Specify Storage Node Group -->
                        <ui:wizardStep id="step_id_specify_group"
                                       summary="#{samBundle['addsn.specifygroup.steptitle']}"
                                       title="#{samBundle['addsn.specifygroup.steptitle']}"
                                       detail="#{samBundle['addsn.specifygroup.instruction']}">
                            <ui:markup tag="br" singleton="true"/>
                            <ui:markup tag="br" singleton="true"/>
                            <ui:radioButton id="radioExisting"
                                            name="groupRadio"
                                            label="#{samBundle['addsn.specifygroup.label.existing']}"
                                            onClick="this.form.submit();"
                                            selected="#{AddStorageNodeBean.selectedExistingGroup}"
                                            style="margin-left:2%"/>
                                <h:panelGrid columns="2" style="margin-left:5%" cellpadding="10">
                                <ui:dropDown id="menuGroup"
                                             selected="#{AddStorageNodeBean.selectedGroup}"
                                             items="#{AddStorageNodeBean.groupSelections}"
                                             disabled="#{AddStorageNodeBean.selectedCreateGroup}"/>
                                <ui:staticText text="help text goes here"
                                               rendered="#{AddStorageNodeBean.selectedExistingGroup}"/>
                            </h:panelGrid>
                            <ui:radioButton id="radioCreate"
                                            name="groupRadio"
                                            label="#{samBundle['addsn.specifygroup.label.create']}"
                                            onClick="this.form.submit();"
                                            selected="#{AddStorageNodeBean.selectedCreateGroup}"
                                            style="margin-left:2%"/>
                        </ui:wizardStep>
                        <!-- Step 3.1 Create New Group -->
                        <ui:wizardSubstepBranch id="SubStepCreateGroup"
                                                taken='#{AddStorageNodeBean.selectedCreateGroup}'>
                            <ui:wizardStep id="step_id_create_group"
                                           summary="#{samBundle['addsn.creategroup.steptitle']}"
                                           title="#{samBundle['addsn.creategroup.steptitle']}"
                                           detail="#{samBundle['addsn.creategroup.instruction']}"
                                           help="help">
                                <ui:alert id="alert_method"
                                          rendered="#{AddStorageNodeBean.alertRendered}"
                                          type="#{AddStorageNodeBean.alertType}"
                                          summary="#{AddStorageNodeBean.alertSummary}"
                                          detail="#{AddStorageNodeBean.alertDetail}"/>
                                <ui:markup tag="p" extraAttributes="style='margin-left:3%'">
                                    <ui:markup tag="br" singleton="true"/>
                                    <ui:label id="idGroupName" text="#{samBundle['addsn.creategroup.label.groupname']}" />
                                    <ui:staticText text="#{AddStorageNodeBean.groupName}" style="margin-left:10" />
                                    <ui:markup tag="br" singleton="true"/>
                                    <ui:markup tag="br" singleton="true"/>
                                    <ui:label id="idDAM" text="#{samBundle['addsn.creategroup.label.dam']}" />
                                    <h:panelGrid columns="1" style="margin-left:5%;align:left;width:90%">
                                        <ui:radioButton id="radioEqualSize"
                                                        name="dauRadio"
                                                        label="#{samBundle['addsn.creategroup.method.equalsize']}"
                                                        onClick="clearTextFields(); this.form.submit();"
                                                        selected="#{AddStorageNodeBean.selectedEqualSize}"/>
                                        <ui:radioButton id="radioSpecifySize"
                                                        name="dauRadio"
                                                        label="#{samBundle['addsn.creategroup.method.specifysize']}"
                                                        onClick="this.form.submit();"
                                                        selected="#{AddStorageNodeBean.selectedSpecifySize}"/>
                                        <h:panelGrid columns="3" style="margin-left:5%;align:left">
                                            <ui:label id="idBlockSize" text="#{samBundle['addsn.creategroup.label.blocksize']}"/>
                                            <ui:textField id="fieldBlockSize"
                                                          trim="true"
                                                          style="width:50px"
                                                          validator="#{AddStorageNodeBean.validateBlockSize}"
                                                          text="#{AddStorageNodeBean.textBlockSize}"
                                                          disabled="#{!AddStorageNodeBean.selectedSpecifySize}">
                                                  <f:validateLongRange minimum="1" />
                                            </ui:textField>
                                            <ui:dropDown id="menuBlockSizeUnit"
                                                         selected="#{AddStorageNodeBean.selectedBlockSizeUnit}"
                                                         items="#{AddStorageNodeBean.blockSizeUnitSelections}"
                                                         disabled="#{!AddStorageNodeBean.selectedSpecifySize}"/>
                                            <ui:label id="idBlockPerDevice" text="#{samBundle['addsn.creategroup.label.blockperdevice']}" />
                                            <ui:textField id="fieldBlockPerDevice"
                                                          trim="true"
                                                          style="width:50px"
                                                          validator="#{AddStorageNodeBean.validateBlockPerDevice}"
                                                          text="#{AddStorageNodeBean.textBlockPerDevice}"
                                                          disabled="#{!AddStorageNodeBean.selectedSpecifySize}">
                                                  <f:validateLongRange minimum="1" />
                                            </ui:textField>
                                            <h:panelGroup/>
                                        </h:panelGrid>
                                        <ui:radioButton id="radioStripedGroup"
                                                        name="dauRadio"
                                                        label="#{samBundle['addsn.creategroup.method.striped']}"
                                                        onClick="clearTextFields(); this.form.submit();"
                                                        selected="#{AddStorageNodeBean.selectedStripedGroup}"/>
                                    </h:panelGrid>
                                </ui:markup>
                            </ui:wizardStep>
                        </ui:wizardSubstepBranch>
                        <!-- Step 4: Select Devices for Storing Metadata -->
                        <ui:wizardStep id="step_id_specify_metadevice"
                                       summary="#{samBundle['addsn.selectmdevice.steptitle']}"
                                       title="#{samBundle['addsn.selectmdevice.steptitle']}"
                                       detail="#{samBundle['addsn.selectmdevice.instruction']}"
                                       help="#{samBundle['addsn.selectmdevice.help']}">
                            <ui:alert id="alert_specifymetadevice"
                                      rendered="#{AddStorageNodeBean.alertRendered}"
                                      type="#{AddStorageNodeBean.alertType}"
                                      summary="#{AddStorageNodeBean.alertSummary}"
                                      detail="#{AddStorageNodeBean.alertDetail}"/>
                            <ui:markup tag="p" extraAttributes="style='margin-left:2%'">
                                <ui:table id="tableSpecifyMeta"
                                          title="#{samBundle['addsn.selectmdevice.table.title']}"
                                          style="margin:10px"
                                          clearSortButton="true"
                                          sortPanelToggleButton="true"
                                          paginateButton="false"
                                          paginationControls="false">

                                    <ui:tableRowGroup id="TableRowsMeta"
                                                      selected="#{AddStorageNodeBean.selectMeta.selectedState}"
                                                      binding="#{AddStorageNodeBean.metaTableRowGroup}"
                                                      sourceData="#{AddStorageNodeBean.metaSummaryList}"
                                                      sourceVar="metaTable">
                                        <!-- Selection Type -->
                                        <ui:tableColumn id="selectionMeta"
                                                        selectId="selectMeta"
                                                        sort="#{AddStorageNodeBean.selectMeta.selectedState}">
                                            <ui:checkbox id="selectMetaBox"
                                                         onClick="setTimeout('initMetaTableRows()', 0);"
                                                         name="checkBoxSelectMeta"
                                                         selected="#{AddStorageNodeBean.selectMeta.selected}"
                                                         selectedValue="#{AddStorageNodeBean.selectMeta.selectedValue}"/>
                                        </ui:tableColumn>
                                        <ui:tableColumn id="colDeviceName"
                                                        headerText="#{samBundle['common.columnheader.name']}"
                                                        align="left"
                                                        valign="top"
                                                        sort="devicePathDisplayString"
                                                        rowHeader="true">
                                            <ui:staticText text="#{metaTable.value.devicePathDisplayString}"/>
                                            <ui:staticText text="#{metaTable.value.devicePath}" visible="false"/>
                                        </ui:tableColumn>
                                        <ui:tableColumn id="colType"
                                                        headerText="#{samBundle['common.columnheader.type']}"
                                                        align="left"
                                                        valign="top"
                                                        sort="diskType"
                                                        rowHeader="true">
                                            <ui:staticText text="#{metaTable.value.diskType}"
                                                           converter="DeviceTypeConverter"/>
                                        </ui:tableColumn>
                                        <ui:tableColumn id="colCapacity"
                                                        headerText="#{samBundle['common.columnheader.capacity']}"
                                                        align="left"
                                                        valign="top"
                                                        sort="capacity"
                                                        rowHeader="true">
                                            <ui:staticText text="#{metaTable.value.capacity}"
                                                           converter="CapacityInMbConverter"/>
                                        </ui:tableColumn>
                                        <ui:tableColumn id="colVendor"
                                                        headerText="#{samBundle['common.columnheader.vendor']}"
                                                        align="left"
                                                        valign="top"
                                                        sort="vendor"
                                                        rowHeader="true">
                                            <ui:staticText text="#{metaTable.value.vendor}"/>
                                        </ui:tableColumn>
                                        <ui:tableColumn id="colProdID"
                                                        headerText="#{samBundle['common.columnheader.prodid']}"
                                                        align="left"
                                                        valign="top"
                                                        sort="productId"
                                                        rowHeader="true">
                                            <ui:staticText text="#{metaTable.value.productId}"/>
                                        </ui:tableColumn>
                                    </ui:tableRowGroup>
                                </ui:table>
                            </ui:markup>
                        </ui:wizardStep>
                        <!-- Step 5: Select Data Devices -->
                        <ui:wizardStep id="step_id_specify_datadevice"
                                       summary="#{samBundle['addsn.selectdatadevice.steptitle']}"
                                       title="#{samBundle['addsn.selectdatadevice.steptitle']}"
                                       detail="#{samBundle['addsn.selectdatadevice.instruction']}"
                                       help="#{samBundle['addsn.selectdatadevice.help']}">
                            <ui:alert id="alert_specifydatadevice"
                                      rendered="#{AddStorageNodeBean.alertRendered}"
                                      type="#{AddStorageNodeBean.alertType}"
                                      summary="#{AddStorageNodeBean.alertSummary}"
                                      detail="#{AddStorageNodeBean.alertDetail}"/>
                            <ui:markup tag="p" extraAttributes="style='margin-left:2%'">
                                <ui:table id="tableSpecifyData"
                                          title="#{samBundle['addsn.selectdatadevice.table.title']}"
                                          style="margin:10px"
                                          clearSortButton="true"
                                          sortPanelToggleButton="true"
                                          paginateButton="false"
                                          paginationControls="false">

                                    <ui:tableRowGroup id="TableRowsData"
                                                      selected="#{AddStorageNodeBean.selectData.selectedState}"
                                                      binding="#{AddStorageNodeBean.dataTableRowGroup}"
                                                      sourceData="#{AddStorageNodeBean.dataSummaryList}"
                                                      sourceVar="dataTable">
                                        <!-- Selection Type -->
                                        <ui:tableColumn id="selectionData"
                                                        selectId="selectData"
                                                        sort="#{AddStorageNodeBean.selectData.selectedState}">
                                            <ui:checkbox id="selectDataBox"
                                                         onClick="setTimeout('initDataTableRows()', 0);"
                                                         name="checkBoxSelectData"
                                                         selected="#{AddStorageNodeBean.selectData.selected}"
                                                         selectedValue="#{AddStorageNodeBean.selectData.selectedValue}"/>
                                        </ui:tableColumn>
                                        <ui:tableColumn id="colDeviceName"
                                                        headerText="#{samBundle['common.columnheader.name']}"
                                                        align="left"
                                                        valign="top"
                                                        sort="devicePathDisplayString"
                                                        rowHeader="true">
                                            <ui:staticText text="#{dataTable.value.devicePathDisplayString}"/>
                                            <ui:staticText text="#{dataTable.value.devicePath}" visible="false"/>
                                        </ui:tableColumn>
                                        <ui:tableColumn id="colType"
                                                        headerText="#{samBundle['common.columnheader.type']}"
                                                        align="left"
                                                        valign="top"
                                                        sort="diskType"
                                                        rowHeader="true">
                                            <ui:staticText text="#{dataTable.value.diskType}"
                                                           converter="DeviceTypeConverter"/>
                                        </ui:tableColumn>
                                        <ui:tableColumn id="colCapacity"
                                                        headerText="#{samBundle['common.columnheader.capacity']}"
                                                        align="left"
                                                        valign="top"
                                                        sort="capacity"
                                                        rowHeader="true">
                                            <ui:staticText text="#{dataTable.value.capacity}"
                                                           converter="CapacityInMbConverter"/>
                                        </ui:tableColumn>
                                        <ui:tableColumn id="colVendor"
                                                        headerText="#{samBundle['common.columnheader.vendor']}"
                                                        align="left"
                                                        valign="top"
                                                        sort="vendor"
                                                        rowHeader="true">
                                            <ui:staticText text="#{dataTable.value.vendor}"/>
                                        </ui:tableColumn>
                                        <ui:tableColumn id="colProdID"
                                                        headerText="#{samBundle['common.columnheader.prodid']}"
                                                        align="left"
                                                        valign="top"
                                                        sort="productId"
                                                        rowHeader="true">
                                            <ui:staticText text="#{dataTable.value.productId}"/>
                                        </ui:tableColumn>
                                    </ui:tableRowGroup>
                                </ui:table>
                            </ui:markup>
                        </ui:wizardStep>
                        <!-- Step 6: Review Selections -->
                        <ui:wizardStep id="step_id_review"
                                       summary="#{samBundle['common.wizard.summary.title']}"
                                       title="#{samBundle['common.wizard.summary.title']}"
                                       detail="#{samBundle['common.wizard.summary.instruction']}"
                                       finish="true">
                            <ui:markup tag="br" singleton="true"/>
                            <h:panelGrid columns="2" style="margin-left:2%" cellpadding="2">
                                <ui:label id="idHostIP"
                                          text="#{samBundle['addsn.specifyhost.label.hostnameip']}" />
                                <ui:staticText text="#{AddStorageNodeBean.textHostNameIP}"
                                               escape="false" />
                                <ui:label id="idGroup"
                                          text="#{samBundle['addsn.creategroup.label.groupname']}" />
                                <ui:staticText text="#{AddStorageNodeBean.textGroup}"
                                               escape="false" />
                                <ui:label id="idNewGroupProp"
                                          text="#{samBundle['addsn.specifygroup.label.newgroupprop']}"
                                          rendered="#{AddStorageNodeBean.selectedCreateGroup}"/>
                                <ui:staticText text="#{AddStorageNodeBean.textNewGroupProp}"
                                               escape="false"
                                               rendered="#{AddStorageNodeBean.selectedCreateGroup}"/>
                                <ui:label id="idMetaDevices"
                                          text="#{samBundle['addsn.label.selectmdevice']}" />
                                <ui:staticText text="#{AddStorageNodeBean.textSelectedMDevice}"
                                               escape="false" />
                                <ui:label id="idDataDevices"
                                          text="#{samBundle['addsn.label.selectdatadevice']}" />
                                <ui:staticText text="#{AddStorageNodeBean.textSelectedDataDevice}"
                                               escape="false" />
                            </h:panelGrid>
                        </ui:wizardStep>
                        <!-- Step 7: View Results -->
                        <ui:wizardStep id="step_id_results"
                                       summary="#{samBundle['common.wizard.viewresult.title']}"
                                       title="#{samBundle['common.wizard.viewresult.title']}"
                                       results="true">
                            <ui:markup tag="p" id="id_Alert" extraAttributes="style='margin:7%'">
                                <ui:alert id="alert_results"
                                          rendered="true"
                                          type="#{AddStorageNodeBean.alertType}"
                                          summary="#{AddStorageNodeBean.alertSummary}"
                                          detail="#{AddStorageNodeBean.alertDetail}"/>
                            </ui:markup>
                            <ui:markup tag="p" id="id_Result" extraAttributes="style='margin:5%'">
                                <ui:staticText text="#{samBundle['addsn.finish.text']}" />
                            </ui:markup>
                        </ui:wizardStep>
                    </ui:wizard>
                </ui:form>
            </ui:body>
        </ui:html>
    </ui:page>
</f:view>
</jsp:root>
