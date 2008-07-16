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

<!-- $Id: AddClientsHostSelection.jsp,v 1.1 2008/07/16 23:47:28 kilemba Exp $ -->


<jsp:root version="1.2"
   xmlns:f="http://java.sun.com/jsf/core"
   xmlns:h="http://java.sun.com/jsf/html"
   xmlns:ui="http://www.sun.com/web/ui"
   xmlns:jsp="http://java.sun.com/JSP/Page">
   <jsp:directive.page contentType="text/html;charset=ISO-8859-1" 
                       pageEncoding="UTF-8"/>

      <div style="margin-left:10px">              
        <h:panelGrid columns="3" style="vertical-align:top;margin-left:10px">
            <ui:label for="hostNammeText"
                      text="#{msgs['fs.addclients.hostname.hostname']}" />
            <ui:textField id="hostNameText" />
            <ui:button id="add"
                       onClick="return handleAddButton(this);"
                       text="#{msgs['common.button.add']}" />
                       
            <ui:label for="selectedHostList"
                      text="#{msgs['fs.addclients.hostname.selectedhosts']}" />
            <ui:listbox id="selectedHostList"
                        items="#{AddClientsBean.hostList}" />
            <ui:button id="remove"
                       onClick="return handleRemoveButton(this);"
                       text="#{msgs['common.button.remove']}" />
        </h:panelGrid>
    </div>

</jsp:root>
