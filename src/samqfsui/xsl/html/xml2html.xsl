<?xml version="1.0" encoding="ISO-8859-1"?>

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
<!-- $Id: xml2html.xsl,v 1.11 2008/12/16 00:12:31 am143972 Exp $ -->

<xsl:stylesheet 
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:java="java"
  xmlns:SamUtil="com.sun.netstorage.samqfs.web.util.SamUtil">
    
  <!-- Import other sytlesheets -->
  <xsl:import href="/xsl/html/fs.xsl"/>
  <xsl:import href="/xsl/html/acsls.xsl"/>
  <xsl:import href="/xsl/html/media.xsl"/>
  
  <!-- Write the result tree to a serial output file - serialization -->
  <xsl:output 
    method="html"
    doctype-public="-//W3C//DTD XHTML 1.0 Transitional//EN"
    doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd"
    encoding="UTF-8" 
    indent="yes" 
    omit-xml-declaration="no"/>      
         
  <xsl:template match='/'> 
    <!-- the reports are getting appended as a part of a html body, so -->
    <!-- do not add html head and body tags -->
    <!-- Use CSS from Lockhart -->
    <xsl:apply-templates select="report"/>
  </xsl:template>
    
  <xsl:template match="report">
    <xsl:apply-templates select="filesystems"/>
    <xsl:apply-templates select="acsls"/>
    <xsl:apply-templates select="media"/>
    <xsl:apply-templates select="dailyActivity"/>
  </xsl:template>
  
</xsl:stylesheet>
