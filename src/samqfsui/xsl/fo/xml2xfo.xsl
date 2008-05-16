<?xml version="1.0" encoding="ISO-8859-1"?>

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
<!-- $Id: xml2xfo.xsl,v 1.4 2008/05/16 18:39:09 am143972 Exp $ -->

<xsl:stylesheet 
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:fo="http://www.w3.org/1999/XSL/Format"
  xmlns:java="java">
  
  <xsl:output method="xml"/>
  
  <!-- Import other sytlesheets -->
  <xsl:import href="fsfo.xsl"/>
  
  <!-- ========================= -->
  <!-- root element: report -->
  <!-- ========================= -->
  
  <!-- Load java.util.ResourceBundle at the beginning -->
  <!-- of the stylesheet to localize literal text -->
  <xsl:variable name="resources"
    select="java:util.ResourceBundle.getBundle(
    'com/sun/netstorage/samqfs/web/resources/Resources')"/>
        
  <xsl:template match='/report'>
    <fo:root xmlns:fo="http://www.w3.org/1999/XSL/Format">
      <fo:layout-master-set>
        <fo:simple-page-master master-name="simple">
          <fo:region-body margin="1in"/>
        </fo:simple-page-master>
      </fo:layout-master-set>
      <fo:page-sequence master-reference="simple">
        <fo:flow flow-name="xsl-region-body">
          <fo:block font-size="16pt" font-weight="bold" space-after="5mm">
            <xsl:value-of select="@name"/>
            <xsl:text> - </xsl:text>
            <xsl:value-of select="@time"/>
          </fo:block>
          <xsl:apply-templates select="filesystems"/>
          <xsl:apply-templates select="acsls"/>
          <xsl:apply-templates select="media"/>
        </fo:flow>
      </fo:page-sequence>
    </fo:root>
  </xsl:template>       
</xsl:stylesheet>
