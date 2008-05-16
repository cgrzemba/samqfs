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
<!-- $Id: fs.xsl,v 1.4 2008/05/16 18:39:09 am143972 Exp $ -->

<xsl:stylesheet 
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:fo="http://www.w3.org/1999/XSL/Format"
  xmlns:java="java">
    
  <xsl:output method="xml"/>
  
  <!-- ========================= -->
  <!-- root element: filesystems -->
  <!-- ========================= -->
  
  <!-- Load java.util.ResourceBundle at the beginning -->
  <!-- of the stylesheet to localize literal text -->
  <xsl:variable name="resources"
    select="java:util.ResourceBundle.getBundle(
    'com/sun/netstorage/samqfs/web/resources/Resources')"/>
        
  <xsl:template match='filesystems'>
    <fo:block font-size="12pt">
      <fo:table table-layout="fixed" border-width="1pt" border-style="solid" background-color="#E9EBEC">
        <fo:table-column column-width="4cm"/>
        <fo:table-column column-width="2cm"/>
        <fo:table-column column-width="3cm"/>
        <fo:table-column column-width="3cm"/>
        <fo:table-column column-width="3cm"/>
        <fo:table-body>
          <fo:table-row line-height="12pt" font-weight="bold">
            <fo:table-cell  border-width="1pt" border-style="solid" padding="1mm">
              <fo:block text-align="center">
                <xsl:value-of select="java:getString($resources,'common.columnheader.name')"/>
              </fo:block>
            </fo:table-cell>
            <fo:table-cell border-width="1pt" border-style="solid" padding="1mm">
              <fo:block text-align="center">
                <xsl:value-of select="java:getString($resources,'common.columnheader.mounted')"/>
              </fo:block>
            </fo:table-cell>
            <fo:table-cell border-width="1pt" border-style="solid" padding="1mm">
              <fo:block text-align="center">
                <xsl:value-of select="java:getString($resources,'common.columnheader.hwm')"/>
              </fo:block>
            </fo:table-cell>
            <fo:table-cell border-width="1pt" border-style="solid" padding="1mm">
              <fo:block text-align="center">
                <xsl:value-of select="java:getString($resources,'common.columnheader.lwm')"/>
              </fo:block>
            </fo:table-cell>
            <fo:table-cell border-width="1pt" border-style="solid" padding="1mm">
              <fo:block text-align="center">
                <xsl:value-of select="java:getString($resources,'common.columnheader.hwmExceed')"/>
              </fo:block>
            </fo:table-cell>
          </fo:table-row>
          <xsl:apply-templates select="filesystem"/>
        </fo:table-body>
      </fo:table>
    </fo:block>
  </xsl:template>          
  <!-- ========================= -->
  <!-- child element: filesystem -->
  <!-- ========================= -->
  <xsl:template match="filesystem">     
    <fo:table-row>
      <xsl:if test="hwmExceed &gt; 10">
        <xsl:attribute name="font-weight">bold</xsl:attribute>
      </xsl:if>
      <fo:table-cell border-width="1pt" border-style="solid">
        <fo:block text-align="center"><xsl:value-of select="name"/></fo:block>
      </fo:table-cell>
      <fo:table-cell border-width="1pt" border-style="solid">
        <fo:block text-align="center"><xsl:value-of select="mounted"/></fo:block>
      </fo:table-cell>
      <fo:table-cell border-width="1pt" border-style="solid">
        <fo:block text-align="center"><xsl:value-of select="hwm"/></fo:block>
      </fo:table-cell>
      <fo:table-cell border-width="1pt" border-style="solid">
        <fo:block text-align="center"><xsl:value-of select="lwm"/></fo:block>
      </fo:table-cell>
      <fo:table-cell border-width="1pt" border-style="solid">
        <fo:block text-align="center"><xsl:value-of select="hwmExceed"/></fo:block>
      </fo:table-cell>
    </fo:table-row>
  </xsl:template>
  
</xsl:stylesheet>
