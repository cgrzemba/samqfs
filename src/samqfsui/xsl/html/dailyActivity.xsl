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
<!-- $Id $ -->

<xsl:stylesheet 
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:svg="http://www.w3.org/2000/svg"
  xmlns:xlink="http://www.w3.org/1999/xlink"
  xmlns:SamUtil="com.sun.netstorage.samqfs.web.util.SamUtil"
  xmlns:Capacity="com.sun.netstorage.samqfs.web.util.Capacity"
  xmlns:Math="java.lang.Math"
  xmlns:Long="java.lang.Long">

  <!-- Import other sytlesheets -->
  <xsl:import href="/xsl/html/common.xsl"/>
    
  <!-- Generic Media Summary -->
  <xsl:template match="dailyActivity">
    <xsl:variable name="dateVal" select="normalize-space(../@time)"/>
    <xsl:variable name="displayDate" 
      select="SamUtil:getTimeString(Long:parseLong($dateVal))"/>
    <table align="center">
      <tr>
        <td colspan="2">
          <h2><xsl:value-of select="normalize-space(../@name)"/></h2>
        </td>
      </tr>
      <tr>
        <td align="left">
          <h3><xsl:text>Hostname: </xsl:text><xsl:value-of select="normalize-space(../@host)"/></h3>
        </td>
        <td align="right">
          <h3><xsl:text>Date: </xsl:text><xsl:value-of select="$displayDate"/></h3>
        </td>
      </tr>
    </table>
    
    <br />
    
    <table class="Tbl">
      <caption class="TblTtlTxt">
        <xsl:text>Data Movement Summary</xsl:text>
      </caption>
      <tr>
        <td><xsl:text>Archived File Count: </xsl:text><xsl:value-of select="archive/@numFiles"/></td>
      </tr>
      <tr>
        <td><xsl:text>Staged File Count: </xsl:text><xsl:value-of select="stage/@numFiles"/></td>
      </tr>
      <tr>
        <td><xsl:text>Recycled File Count: </xsl:text><xsl:value-of select="recycle/@numFiles"/></td>      
      </tr>
      <tr>
        <td>
          <xsl:text>Released File Count: </xsl:text><xsl:value-of select="release/@numFiles"/>   
          <xsl:if test="release/@runCount &gt; 0">
            <xsl:text> (Releaser ran </xsl:text>
            <xsl:value-of select="release/@runCount"/>
            <xsl:text> time/times) </xsl:text>
            <xsl:if test="release/@runCount &gt; 5">
              <br />
              <br />
              <xsl:text>
                Check the high and low watermark settings. Frequent releasing will affect the overall performance of the system.
              </xsl:text>
            </xsl:if>
          </xsl:if>
        </td>
      </tr>
    </table>
    
    <br />
    <br />
    
    <table class="Tbl">
      <caption class="TblTtlTxt">
        <xsl:text>Tape Input/Output Activity Summary</xsl:text>
      </caption>
      <!-- convert to appropriate units using the Capacity class  -->
      <!-- in xslt 1.0, you cannot specify that a variable should hold an int/long -->
      <xsl:variable name="write" 
        select="Capacity:toString(Capacity:new(Long:parseLong(string(write/@numBytes)), 0))"/>
      <xsl:variable name="read" 
        select="Capacity:toString(Capacity:new(Long:parseLong(string(read/@numBytes)), 0))"/>
      <xsl:variable name="avgWrite" select="format-number((write/@numBytes div write/@seconds), 0)"/>
      <xsl:variable name="avgRead" select="format-number((read/@numBytes div read/@seconds), 0)"/>
      <tr>
        <td>
          <xsl:value-of select="$write"/>
          <xsl:text> of data was written to tape in </xsl:text>
          <xsl:value-of select="write/@seconds"/>
          <xsl:text> seconds. </xsl:text>
          <xsl:text>Average write speed = </xsl:text>
          <xsl:value-of select="Capacity:toString(Capacity:new(Long:parseLong($avgWrite), 0))"/>
          <xsl:text>/sec.</xsl:text>
        </td>
      </tr>
      <tr>
        <td>
          <xsl:value-of select="$read"/>
          <xsl:text> of data was read from tape in </xsl:text>
          <xsl:value-of select="read/@seconds"/>
          <xsl:text> seconds. </xsl:text>
          <xsl:text>Average read speed = </xsl:text>
          <xsl:value-of select="Capacity:toString(Capacity:new(Long:parseLong($avgRead), 0))"/>
          <xsl:text>/sec.</xsl:text>
        </td>
      </tr>
    </table>
    <br />
    <br />
    <xsl:apply-templates select="vsns"/>
  </xsl:template>
  
  <xsl:template match="vsns">
    <xsl:variable name="totalVsns" select="count(vsn)"/>
    <table class="Tbl">
      <caption class="TblTtlTxt">
        <xsl:text>VSNs Used (</xsl:text><xsl:value-of select="$totalVsns"/><xsl:text>)</xsl:text>
      </caption>
      <xsl:apply-templates select="vsn"/>
    </table>       
  </xsl:template>
  
  <!-- place all vsn into a '8 Column x n Row' Table -->
  <xsl:template match="vsn[(position()-1) mod 5 = 0]">
    <tr>
      <td>
        <xsl:value-of select="@name"/>
        <xsl:if test="@mounts">
          <xsl:text> (</xsl:text><xsl:value-of select="@mounts"/><xsl:text> mounts)</xsl:text>
        </xsl:if>
      </td>
      <xsl:for-each select="following-sibling::*[position() &lt; 5]">
        <td>
          <xsl:if test="position() = last()">
            <xsl:attribute name="colspan"><xsl:value-of select="2"/></xsl:attribute>
          </xsl:if>
          <xsl:value-of select="@name"/>
          <xsl:if test="@mounts">
            <xsl:text> (</xsl:text><xsl:value-of select="@mounts"/><xsl:text> mounts)</xsl:text>
          </xsl:if>
        </td>
      </xsl:for-each>
    </tr>
  </xsl:template>
  
</xsl:stylesheet>
