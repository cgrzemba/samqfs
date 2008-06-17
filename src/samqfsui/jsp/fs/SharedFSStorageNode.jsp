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

<!-- $Id: SharedFSStorageNode.jsp,v 1.2 2008/06/17 16:04:27 ronaldso Exp $ -->

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
            <ui:head title="#{samBundle['SharedFS.pagetitle.sn']}"/>
            <ui:body>
                <ui:form id="SharedFSStorageNodeForm">
                    <ui:breadcrumbs id="breadcrumbs" pages="#{SharedFSBean.breadCrumbsStorageNode}" />
                    <ui:tabSet binding="#{SharedFSBean.tabSet}" selected="storagenode" />
                    <ui:contentPageTitle
                        id="pageTitle"
                        title="#{samBundle['SharedFS.pagetitle.sn']}">
                    </ui:contentPageTitle>
                </ui:form>
            </ui:body>
        </ui:html>
    </ui:page>
</f:view>
</jsp:root>
