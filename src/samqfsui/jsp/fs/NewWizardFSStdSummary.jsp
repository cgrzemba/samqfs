
<%--
/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at pkg/OPENSOLARIS.LICENSE
 * or https://illumos.org/license/CDDL.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at pkg/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

// ident	$Id: NewWizardFSStdSummary.jsp,v 1.8 2008/12/16 00:10:46 am143972 Exp $

--%>

<%@ page language="java" %> 
<%@ page import="com.iplanet.jato.view.ViewBean" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%> 
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:pagelet>

<cc:i18nbundle id="samBundle"
 baseName="com.sun.netstorage.samqfs.web.resources.Resources" />

<%-- For now assume we're still presenting the components in a table
     which is output by the framework and components are
     in rows and cells
--%>

<!-- inline alart -->
<cc:alertinline name="Alert" bundleID="samBundle" />
<br>

  <table border="0" cellspacing="10" cellpadding="0">
    <tbody>
      <tr>
        <td>
	    <cc:label name="Label" styleLevel="2"
		elementName="fsTypeSelect"
		defaultValue="FSWizard.new.fstype"
		bundleID="samBundle" />
	</td>
        <td>
	    <cc:text name="fsTypeSelect"
		bundleID="samBundle" />
	</td>
      </tr>
      <tr>
        <td>
	    <cc:label name="Label" styleLevel="2"
		elementName="DataField"
		defaultValue="FSWizard.new.selectedata"
		bundleID="samBundle" />
	</td>
        <td>
	    <cc:text name="DataField"
		bundleID="samBundle" escape="false" />
	</td>
      </tr>
      <tr>
        <td>
	    <cc:label name="Label" styleLevel="2"
		elementName="mountValue"
		defaultValue="FSWizard.new.mountlabel"
		bundleID="samBundle" />
	</td>
        <td>
	    <cc:text name="mountValue"
		bundleID="samBundle" />
	</td>
      </tr>
      <tr>
        <td>
	    <cc:label name="Label" styleLevel="2"
		elementName="bootTimeCheckBox"
		defaultValue="FSWizard.new.mountBootLabel"
		bundleID="samBundle" />
	</td>
        <td>
	    <cc:text name="bootTimeCheckBox"
		bundleID="samBundle" />
	</td>
      </tr>
      <tr>
        <td>
	    <cc:label name="Label" styleLevel="2"
		elementName="readOnlyCheckBox"
		defaultValue="FSWizard.new.readOnlyLabel"
		bundleID="samBundle" />
	</td>
        <td>
	    <cc:text name="readOnlyCheckBox"
		bundleID="samBundle" />
	</td>
      </tr>
      <tr>
        <td>
	    <cc:label name="Label" styleLevel="2"
		elementName="noSetUIDCheckBox"
		defaultValue="FSWizard.new.noSetUIDLabel"
		bundleID="samBundle" />
	</td>
        <td>
	    <cc:text name="noSetUIDCheckBox"
		bundleID="samBundle" />
	</td>
      </tr>
      <tr>
        <td>
	    <cc:label name="Label" styleLevel="2"
		elementName="mountAfterCreateCheckBox"
		defaultValue="FSWizard.new.mountAfterCreateLabel"
		bundleID="samBundle" />
	</td>
        <td>
	    <cc:text name="mountAfterCreateCheckBox"
		bundleID="samBundle" />
	</td>
      </tr>
  </tbody>
  </table>

</jato:pagelet>
