
<%--
/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

// ident	$Id: GrowWizardQFSSummaryPage.jsp,v 1.7 2008/03/17 14:40:33 am143972 Exp $
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

<table border="0" cellspacing="10" cellpadding="0">
<tbody>
<tr>
<td>
<cc:alertinline name="Alert" bundleID="samBundle" /><br />
</td>
</tr>
  <tr>
	<td>
      <cc:label name="Label" styleLevel="2"
          elementName="MetadataField"
          defaultValue="FSWizard.grow.selectedMetadata"
          bundleID="samBundle" />
    </td>
    <td>
      <cc:selectablelist name="MetadataField"
          bundleID="samBundle" escape="false" />
    </td>
  </tr>

  <tr>
    <td>
      <cc:label name="Label" styleLevel="2"
          elementName="DataField"
          defaultValue="FSWizard.grow.selectedData"
          bundleID="samBundle" />
    </td>
    <td>
      <cc:selectablelist name="DataField"
          bundleID="samBundle" escape="false" />
    </td>
  </tr>

  <tr>
    <td>
	  <cc:label name="Label" styleLevel="2"
          elementName="DumpField"
		  defaultValue="FSWizard.grow.commitORsave"
          bundleID="samBundle" />
	</td>
    <td>
	  <cc:text name="DumpField"
          bundleID="samBundle" />
	</td>
  </tr>

</tbody>
</table>

</jato:pagelet>
