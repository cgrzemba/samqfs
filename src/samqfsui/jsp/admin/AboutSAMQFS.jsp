<!--  SAM-QFS_notice_begin

    CDDL HEADER START

    The contents of this file are subject to the terms of the
    Common Development and Distribution License (the "License").
    You may not use this file except in compliance with the License.

    You can obtain a copy of the license at pkg/OPENSOLARIS.LICENSE
    or https://illumos.org/license/CDDL.
    See the License for the specific language governing permissions
    and limitations under the License.

    When distributing Covered Code, include this CDDL HEADER in each
    file and include the License file at pkg/OPENSOLARIS.LICENSE.
    If applicable, add the following below this CDDL HEADER, with the
    fields enclosed by brackets "[]" replaced with your own identifying
    information: Portions Copyright [yyyy] [name of copyright owner]

    CDDL HEADER END

    Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
    Use is subject to license terms.

      SAM-QFS_notice_end -->
<!--                                                                      -->

<!-- $Id: AboutSAMQFS.jsp,v 1.6 2008/12/17 20:18:55 kilemba Exp $ -->


<jsp:root version="1.2"
   xmlns:f="http://java.sun.com/jsf/core"
   xmlns:h="http://java.sun.com/jsf/html"
   xmlns:ui="http://www.sun.com/web/ui"
   xmlns:jsp="http://java.sun.com/JSP/Page">
   <jsp:directive.page contentType="text/html;charset=utf-8"
                       pageEncoding="UTF-8"/>

    <style>
        div.txtbg{margin:5px 20px 10px 10px;width:80%}
    </style>

   <f:view>
   <f:loadBundle basename="com.sun.netstorage.samqfs.web.resources.Resources"
                 var="msgs"/>

   <ui:page>
   <ui:html>
   <ui:head title="#{msgs['node.aboutsamqfs']}"/>
   <ui:body onLoad="if (parent.serverName != null) {
                        parent.setSelectedNode('200', 'AboutSAMQFS');
                    }">
   <ui:form id="AboutSAMQFSForm">
        <ui:contentPageTitle id="pageTitle" title="#{msgs['node.aboutsamqfs']}"/>

        <div class="txtbg">
            <ui:staticText text="#{msgs['gettingstarted.about.text1']}"/>
        </div>

        <div class="txtbg">
            <ui:staticText text="#{msgs['gettingstarted.about.text2']}"/>
        </div>

        <div class="txtbg">
            <ui:staticText text="#{msgs['gettingstarted.about.text3']}"/>
        </div>

        <div class="txtbg">
            <ui:hyperlink text="#{msgs['gettingstarted.about.text4']}"
                          onClick="window.open('http://wikis.sun.com/display/SAMQFSDocs/Home');return false;"/>
        </div>
   </ui:form>
   </ui:body>
   </ui:html>
   </ui:page>
   </f:view>
</jsp:root>
