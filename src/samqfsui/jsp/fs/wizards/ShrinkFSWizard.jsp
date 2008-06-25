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

<!-- $Id: ShrinkFSWizard.jsp,v 1.1 2008/06/25 21:03:56 ronaldso Exp $ -->

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
            <ui:head title="#{samBundle['fs.shrink.title']}" />
            <ui:body>
                <ui:form id="ShrinkFSForm">
                    <ui:masthead id="Masthead" secondary="true"
                                 productImageURL="/com_sun_web_ui/images/SecondaryProductName.png"
                                 productImageDescription=""
                                 productImageWidth="162"
                                 productImageHeight="40"/>

                    <ui:wizard id="ShrinkFSWizard" title="#{samBundle['fs.shrink.title']}"
                               eventListener="#{ShrinkFSBean.wizardEventListener}">
                        <!-- Step 1: Select Storage to Exclude -->
                        <ui:wizardStep id="step_id_selectstorage"
                                       summary="#{samBundle['fs.shrink.selectstorage.steptitle']}"
                                       title="#{samBundle['fs.shrink.selectstorage.steptitle']}"
                                       detail="#{samBundle['fs.shrink.selectstorage.instruction.1']}"
                                       help="#{samBundle['fs.shrink.selectstorage.help']}">
                            <h:panelGrid columns="1" style="margin:10px">
                                <ui:staticText text="#{samBundle['fs.shrink.selectstorage.instruction.2']}"/>
                            </h:panelGrid>
                            <ui:markup tag="br" singleton="true"/>
                            <ui:markup tag="p" extraAttributes="style='margin-left:2%'">
                                <ui:table id="tableExclude"
                                          title="#{ShrinkFSBean.tableTitleExclude}"
                                          style="margin:10px"
                                          paginateButton="false"
                                          paginationControls="false">

                                    <ui:tableRowGroup id="TableRowsExclude"
                                                      selected="#{ShrinkFSBean.selectExclude.selectedState}"
                                                      binding="#{ShrinkFSBean.excludeTableRowGroup}"
                                                      sourceData="#{ShrinkFSBean.excludeSummaryList}"
                                                      sourceVar="excludeTable">
                                        <!-- Selection Type -->
                                        <ui:tableColumn id="selection"
                                                        selectId="select"
                                                        sort="#{ShrinkFSBean.selectExclude.selectedState}">
                                            <ui:radioButton id="selectExclude"
                                                            onClick="setTimeout('initExcludeTableRows()', 0);"
                                                            name="radioSelect"
                                                            selected="#{ShrinkFSBean.selectExclude.selected}"
                                                            selectedValue="#{ShrinkFSBean.selectExclude.selectedValue}"/>
                                        </ui:tableColumn>
                                        <ui:tableColumn id="colDeviceName"
                                                        headerText="#{samBundle['fs.shrink.selectstorage.heading.name']}"
                                                        align="left"
                                                        valign="top"
                                                        sort="name"
                                                        rowHeader="true">
                                            <ui:staticText text="#{excludeTable.value.devicePathDisplayString}"/>
                                            <ui:staticText text="#{excludeTable.value.devicePath}" visible="false"/>
                                        </ui:tableColumn>
                                        <ui:tableColumn id="colType"
                                                        headerText="#{samBundle['fs.shrink.selectstorage.heading.type']}"
                                                        align="left"
                                                        valign="top"
                                                        sort="type"
                                                        rowHeader="true">
                                            <ui:staticText text="#{excludeTable.value.diskType}"
                                                           converter="DeviceTypeConverter"/>
                                        </ui:tableColumn>
                                        <ui:tableColumn id="colCapacity"
                                                        headerText="#{samBundle['fs.shrink.selectstorage.heading.capacity']}"
                                                        align="left"
                                                        valign="top"
                                                        sort="cap"
                                                        rowHeader="true">
                                            <ui:staticText text="#{excludeTable.value.capacity}"
                                                           converter="CapacityInMbConverter"/>
                                        </ui:tableColumn>
                                        <ui:tableColumn id="colVendor"
                                                        headerText="#{samBundle['fs.shrink.selectstorage.heading.vendor']}"
                                                        align="left"
                                                        valign="top"
                                                        sort="vendor"
                                                        rowHeader="true">
                                            <ui:staticText text="#{excludeTable.value.vendor}"/>
                                        </ui:tableColumn>
                                        <ui:tableColumn id="colProdID"
                                                        headerText="#{samBundle['fs.shrink.selectstorage.heading.prodid']}"
                                                        align="left"
                                                        valign="top"
                                                        sort="prodId"
                                                        rowHeader="true">
                                            <ui:staticText text="#{excludeTable.value.productId}"/>
                                        </ui:tableColumn>
                                    </ui:tableRowGroup>
                                </ui:table>
                            </ui:markup>
                        </ui:wizardStep>
                        <!-- Step 2: Specify how to move data off the device -->
                        <ui:wizardStep id="step_id_method"
                                       summary="#{samBundle['fs.shrink.method.steptitle']}"
                                       title="#{samBundle['fs.shrink.method.steptitle']}"
                                       detail="#{samBundle['fs.shrink.method.instruction.1']}"
                                       help="#{samBundle['fs.shrink.method.help']}">
                            <h:panelGrid columns="1" style="margin:10px">
                                <ui:staticText text="#{samBundle['fs.shrink.method.instruction.2']}" />
                                <ui:staticText text="#{samBundle['fs.shrink.method.instruction.3']}" />
                                <ui:staticText text="#{samBundle['fs.shrink.method.instruction.4']}" />
                            </h:panelGrid>
                            <ui:markup tag="br" singleton="true"/>
                            <ui:markup tag="p" extraAttributes="style='margin-left:2%'">
                            <ui:markup tag="br" singleton="true"/>
                            <ui:radioButton id="radioRelease"
                                            name="methodRadio"
                                            label="#{samBundle['fs.shrink.method.radio.release']}"
                                            rendered="#{ShrinkFSBean.renderedMethodRelease}"
                                            selected="#{ShrinkFSBean.selectedMethodRelease}"/>
                            <ui:markup tag="br" singleton="true"/>
                            <ui:radioButton id="radioDistribute"
                                            name="methodRadio"
                                            label="#{samBundle['fs.shrink.method.radio.distribute']}"
                                            rendered="#{ShrinkFSBean.renderedMethodDistribute}"
                                            selected="#{ShrinkFSBean.selectedMethodDistribute}"/>
                            <ui:markup tag="br" singleton="true"/>
                                <ui:radioButton id="radioMove"
                                                name="methodRadio"
                                                label="#{samBundle['fs.shrink.method.radio.move']}"
                                                rendered="#{ShrinkFSBean.renderedMethodMove}"
                                                selected="#{ShrinkFSBean.selectedMethodMove}"/>
                            </ui:markup>
                        </ui:wizardStep>
                        <!-- Step 2.1 Specify a device to store the data -->
                        <ui:wizardSubstepBranch id="SubStepSpecifyDevice"
                                                taken='#{ShrinkFSBean.renderedSubStep}'>
                            <ui:wizardStep id="step_id_specifydevice"
                                           summary="#{samBundle['fs.shrink.specifydevice.steptitle']}"
                                           title="#{samBundle['fs.shrink.specifydevice.steptitle']}"
                                           detail="#{samBundle['fs.shrink.specifydevice.instruction.1']}"
                                           help="#{samBundle['fs.shrink.specifydevice.help']}">
                                <ui:markup tag="p" extraAttributes="style='margin-left:2%'">
                                    <ui:table id="tableAvailable"
                                              title="#{samBundle['fs.shrink.specifydevice.tabletitle']}"
                                              style="margin:10px"
                                              paginateButton="false"
                                              paginationControls="false">

                                        <ui:tableRowGroup id="TableRowsAvailable"
                                                          selected="#{ShrinkFSBean.selectAvailable.selectedState}"
                                                          binding="#{ShrinkFSBean.availableTableRowGroup}"
                                                          sourceData="#{ShrinkFSBean.availableSummaryList}"
                                                          sourceVar="availableTable">
                                            <!-- Selection Type -->
                                            <ui:tableColumn id="selection"
                                                            selectId="select"
                                                            sort="#{ShrinkFSBean.selectAvailable.selectedState}">
                                                <ui:radioButton id="selectAvailable"
                                                                onClick="setTimeout('initExcludeTableRows()', 0);"
                                                                name="radioSelectAvail"
                                                                selected="#{ShrinkFSBean.selectAvailable.selected}"
                                                                selectedValue="#{ShrinkFSBean.selectAvailable.selectedValue}"/>
                                            </ui:tableColumn>
                                            <ui:tableColumn id="colDeviceName"
                                                            headerText="#{samBundle['fs.shrink.specifydevice.heading.name']}"
                                                            align="left"
                                                            valign="top"
                                                            sort="name"
                                                            rowHeader="true">
                                                <ui:staticText text="#{availableTable.value.devicePathDisplayString}"/>
                                                <ui:staticText text="#{availableTable.value.devicePath}" visible="false"/>
                                            </ui:tableColumn>
                                            <ui:tableColumn id="colType"
                                                            headerText="#{samBundle['fs.shrink.specifydevice.heading.type']}"
                                                            align="left"
                                                            valign="top"
                                                            sort="type"
                                                            rowHeader="true">
                                                <ui:staticText text="#{availableTable.value.diskType}"
                                                               converter="DeviceTypeConverter"/>
                                            </ui:tableColumn>
                                            <ui:tableColumn id="colCapacity"
                                                            headerText="#{samBundle['fs.shrink.specifydevice.heading.capacity']}"
                                                            align="left"
                                                            valign="top"
                                                            sort="cap"
                                                            rowHeader="true">
                                                <ui:staticText text="#{availableTable.value.capacity}"
                                                               converter="CapacityInMbConverter"/>
                                            </ui:tableColumn>
                                            <ui:tableColumn id="colVendor"
                                                            headerText="#{samBundle['fs.shrink.specifydevice.heading.vendor']}"
                                                            align="left"
                                                            valign="top"
                                                            sort="vendor"
                                                            rowHeader="true">
                                                <ui:staticText text="#{availableTable.value.vendor}"/>
                                            </ui:tableColumn>
                                            <ui:tableColumn id="colProdID"
                                                            headerText="#{samBundle['fs.shrink.specifydevice.heading.prodid']}"
                                                            align="left"
                                                            valign="top"
                                                            sort="prodId"
                                                            rowHeader="true">
                                                <ui:staticText text="#{availableTable.value.productId}"/>
                                            </ui:tableColumn>
                                        </ui:tableRowGroup>
                                    </ui:table>
                                </ui:markup>
                            </ui:wizardStep>
                        </ui:wizardSubstepBranch>
                        <!-- Step 3:Specify Shrink Options -->
                        <ui:wizardStep id="step_id_options"
                                       summary="#{samBundle['fs.shrink.options.steptitle']}"
                                       title="#{samBundle['fs.shrink.options.steptitle']}"
                                       detail="#{samBundle['fs.shrink.options.instruction.1']}"
                                       help="#{samBundle['fs.shrink.options.help']}">
                            <ui:legend id="legend" />
                            <ui:markup tag="p" extraAttributes="style='margin-left:2%'">
                                <ui:label id="labelLogFile"
                                          for="fieldLogFile"
                                          requiredIndicator="true"
                                          text="#{samBundle['fs.shrink.options.label.logfile']}" />
                                <ui:textField id="fieldLogFile"
                                              required="true"
                                              trim="true"
                                              style="width:200px"
                                              validator="#{ShrinkFSBean.validateLogFile}"
                                              text="#{ShrinkFSBean.textLogFile}" />
                                <ui:markup tag="br" singleton="true"/>
                                <ui:markup tag="br" singleton="true"/>
                                <ui:radioButton id="radioDefault"
                                                name="shrinkOptionRadio"
                                                label="#{samBundle['fs.shrink.options.radio.default']}"
                                                onClick="this.form.submit();"
                                                selected="#{ShrinkFSBean.selectedOptionDefault}"/>
                                <ui:markup tag="br" singleton="true"/>
                                <ui:radioButton id="radioCustom"
                                                name="shrinkOptionRadio"
                                                label="#{samBundle['fs.shrink.options.radio.custom']}"
                                                onClick="this.form.submit();"
                                                selected="#{ShrinkFSBean.selectedOptionCustom}"/>
                            </ui:markup>
                            <ui:markup tag="p" id="customSettings" extraAttributes="style='margin-left:4%'" rendered="#{ShrinkFSBean.selectedOptionCustom}">
                                <ui:checkbox id="boxDisplayName"
                                             name="DisplayNameBox"
                                             selected="#{ShrinkFSBean.displayName}"
                                             label="#{samBundle['fs.shrink.options.checkbox.display']}" />
                                <ui:markup tag="br" singleton="true"/>
                                <ui:checkbox id="boxDryRun"
                                             name="DryRunBox"
                                             selected="#{ShrinkFSBean.dryRun}"
                                             label="#{samBundle['fs.shrink.options.checkbox.dryrun']}" />
                                <ui:markup tag="br" singleton="true"/>
                                <ui:checkbox id="boxStageBack"
                                             name="StageBackBox"
                                             selected="#{ShrinkFSBean.stageBack}"
                                             label="#{samBundle['fs.shrink.options.checkbox.bringonline']}" />
                                <ui:markup tag="br" singleton="true"/>
                                <h:panelGrid columns="2" style="margin-left:4%">
                                    <ui:label id="idBlockSize" for="blockSize" text="#{samBundle['fs.shrink.options.label.blocksize']}" />
                                    <h:panelGroup>
                                        <ui:dropDown id="menuBlockSize"
                                                     selected="#{ShrinkFSBean.selectedBlockSize}"
                                                     items="#{ShrinkFSBean.blockSizeSelections}"/>
                                        <ui:staticText text="#{samBundle['fs.shrink.options.text.mb']}"
                                                       style="margin-left:5px"/>
                                    </h:panelGroup>
                                    <ui:label id="idStreams"
                                              for="streams"
                                              text="#{samBundle['fs.shrink.options.label.streams']}" />
                                    <ui:textField id="fieldStreams"
                                                  trim="true"
                                                  text="#{ShrinkFSBean.textStreams}">
                                        <f:validateLongRange maximum="128" minimum="1" />
                                    </ui:textField>
                                    <h:panelGroup/>
                                    <ui:helpInline id="inlineHelpStreams"
                                                   type="field"
                                                   text="#{samBundle['fs.shrink.options.help.streams']}" />
                                </h:panelGrid>
                            </ui:markup>
                        </ui:wizardStep>
                        <!-- Step 4: Review Selections -->
                        <ui:wizardStep id="step_id_review"
                                       summary="#{samBundle['common.wizard.summary.title']}"
                                       title="#{samBundle['common.wizard.summary.title']}"
                                       detail="#{samBundle['common.wizard.summary.instruction']}"
                                       finish="true">
                            <h:panelGrid columns="2" style="margin-left:2%">
                                <ui:label id="idSelectStorage"
                                          text="#{samBundle['fs.shrink.review.text.selectstorage']}" />
                                <ui:staticText text="#{ShrinkFSBean.summarySelectStorage}" />
                                <ui:label id="idMethod"
                                          text="#{samBundle['fs.shrink.review.text.method']}" />
                                <ui:staticText text="#{ShrinkFSBean.summaryMethod}" />
                                <ui:label id="idSpecifyDevice"
                                          rendered="#{ShrinkFSBean.renderedSubStep}"
                                          text="#{samBundle['fs.shrink.review.text.specifydevice']}" />
                                <ui:staticText text="#{ShrinkFSBean.summarySpecifyDevice}"
                                               rendered="#{ShrinkFSBean.renderedSubStep}" />
                                <ui:label id="idOptions"
                                          text="#{samBundle['fs.shrink.review.text.options']}" />
                                <ui:staticText text="#{ShrinkFSBean.summaryOptions}" />
                            </h:panelGrid>
                        </ui:wizardStep>
                        <!-- Step 5: View Results -->
                        <ui:wizardStep id="step_id_results"
                                       summary="#{samBundle['common.wizard.viewresult.title']}"
                                       title="#{samBundle['common.wizard.viewresult.title']}"
                                       results="true">
                            <ui:markup tag="p" id="id_Alert" extraAttributes="style='margin:7%'">
                                <ui:alert id="alert"
                                          rendered="true"
                                          type="#{ShrinkFSBean.alertType}"
                                          summary="#{ShrinkFSBean.alertSummary}"
                                          detail="#{ShrinkFSBean.alertDetail}"/>
                            </ui:markup>
                            <ui:markup tag="p" id="id_Result" extraAttributes="style='margin:5%'">
                                <ui:staticText text="#{samBundle['fs.shrink.result.text']}" />
                            </ui:markup>
                        </ui:wizardStep>
                    </ui:wizard>
                </ui:form>
            </ui:body>
        </ui:html>
    </ui:page>
</f:view>
</jsp:root>
