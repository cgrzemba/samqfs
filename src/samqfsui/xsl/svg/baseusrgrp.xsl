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
<!-- $Id: baseusrgrp.xsl,v 1.11 2008/05/16 18:39:10 am143972 Exp $ -->

<xsl:stylesheet 
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:svg="http://www.w3.org/2000/svg"
  xmlns:SamUtil="com.sun.netstorage.samqfs.web.util.SamUtil"
  xmlns:Capacity="com.sun.netstorage.samqfs.web.util.Capacity"
  xmlns:FileMetricSummaryViewBean="com.sun.netstorage.samqfs.web.admin.FileMetricSummaryViewBean"
  xmlns:Math="java.lang.Math"
  xmlns:Long="java.lang.Long">

  <!-- this is used as a base template for bar graphs to display some info,
  e.g. top 10 users/groups as a series of bar-graphs for a particular date/time.
  Due to space constraints, the information corresponding to only 4 dates can be
  displayed.
  -->

  <xsl:variable name="maxDateCount" select="4"/>
  <xsl:variable name="xIntervalBetweenDates" select="120"/>
  <xsl:variable name="barWidth" select="11"/>
  
  <!-- used for filebyowner and filebygroup -->
  <xsl:template match="filesystem"> <!-- only one filesystem is supported at this time -->
    
    <!-- y coords (in common.xsl) -->
    
    <xsl:variable name="totalDate" select="count(date)"/>
    <!-- If no dates, display no item found -->
    <xsl:if test="$totalDate &lt;= 0">
        <!-- x/y coords are hard-coded -->
        <text style="font-weight:bold; font-size:12; text-anchor:middle; letter-spacing:1.5;" x="330" y="205">
            <xsl:value-of select="SamUtil:getResourceString('common.error.noDataAvailable')"/>
        </text>
        <text style="font-weight:normal; font-size:12; text-anchor:middle;" x="330" y="260">
            <xsl:value-of select="SamUtil:getResourceString('reports.msg.filedistribution.howtocollectfromrecoverypnts')"/>
        </text>
        <text style="font-weight:normal; font-size:12; text-anchor:middle;" x="330" y="310">
            <xsl:value-of select="SamUtil:getResourceString('reports.msg.filedistribution.howtocollectfromlivefs')"/>
        </text>
    </xsl:if>
    
    <!-- Due to space constraints, the bar graphs are displayed for 4 dates -->
    <!-- the dates are sorted in ascending order and the paradigm is to -->
    <!-- always display the graphs for the most current dates -->
    <xsl:variable name="startDatePos">
      <xsl:choose>
        <xsl:when test="$totalDate &gt; $maxDateCount">
          <xsl:value-of select="format-number(($totalDate - $maxDateCount), '#')"/>
        </xsl:when>
        <xsl:otherwise><xsl:value-of select="0"/></xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    
    <!-- Display the dates as x-coordinates along the X-axis -->
    <xsl:call-template name="display-xaxis-dates">
      <xsl:with-param name="startDatePos" select="$startDatePos"/>
    </xsl:call-template>
    
    <!-- bar charts -->
    <xsl:apply-templates select="date">
      <xsl:sort select="@val" order="ascending" data-type="number"/>
      <xsl:with-param name="startDatePos" select="$startDatePos"/>
    </xsl:apply-templates>
  </xsl:template>
  
  <!-- =======================================================          -->
  <!-- generate X axis points i.e. display dates -->
  <!-- date1   date2  date3  date4 (date4 > date3..)  -->
  
  <xsl:template name="display-xaxis-dates">
    <xsl:param name="startDatePos" select="0"/>
    
    <xsl:for-each select="date">
      <xsl:sort select="@val" order="ascending" data-type="number"/>
      
      <xsl:variable name="position" select="position()"/>
      <!-- current position should be greater than startDatePos -->
      <xsl:if test="$position &gt; $startDatePos">
        <!-- get the relative position from the startDatePos -->
        <xsl:variable name="order" select="format-number(($position - $startDatePos - 1), '#')"/>
        <xsl:variable name="dateVal" select="@val"/>
        <xsl:variable name="displayDate"
                      select="FileMetricSummaryViewBean:getDateTimeString(Long:parseLong($dateVal))"/>
                      
        <!-- display the date a little to the right of the first bar -->
        <xsl:variable name="xcoord"
          select="format-number(($minXcoord + ($order * $xIntervalBetweenDates)), '#')"/>
        
        <!-- SVG does not provide for automatic line breaks or word wrapping -->
        <!-- To conserve horizontal space, use tspan to split date and time  -->
        <!-- on two lines, i.e. 10/8/05 on 1st line and 10:00 AM on 2nd line -->
        <xsl:variable name="yline1" select="format-number(($maxYcoord + 15), '#')"/>
        <xsl:variable name="yline2" select="format-number(($maxYcoord + 27), '#')"/>
        <g>
            <text style='font-family:arial;font-weight:normal;font-style:normal;font-size:10;text-anchor:start;'>
                <tspan x='{$xcoord}' y='{$yline1}'>
                    <xsl:value-of select="substring-before($displayDate, ' ')"/>
                </tspan>
                <tspan x='{$xcoord}' y='{$yline2}'>
                    <xsl:value-of select="substring-after($displayDate, ' ')"/>
                </tspan>
            </text>
        </g>
        
        <!-- draw a x-axis tick mark to indicate the point on the scale -->
        <xsl:variable name="ticky2" select="format-number(($maxYcoord + 4), '#')"/>
        <line stroke='#999999' stroke-width='1' 
              x1='{$xcoord}' y1='{$maxYcoord}' 
              x2='{$xcoord}' y2='{$ticky2}'/>
        
        <line stroke='#CCCCCC' stroke-width='1' stroke-dasharray='2' 
              x1='{$xcoord}' y1='{$minYcoord}' 
              x2='{$xcoord}' y2='{$maxYcoord}'/>
        
      </xsl:if>
    </xsl:for-each>
  </xsl:template>
  <!-- =======================================================          -->
  
  <!-- =======================================================          -->
  <!-- Draw the bar graph -->
  <xsl:template match="date">
    <xsl:param name="startDatePos" select="0"/>
    
    <xsl:variable name="position" select="position()"/>
      
    <!-- current position should be greater than startDatePos -->
    <xsl:if test="$position &gt; $startDatePos">
      <!-- get the relative position from the startDatePos -->
      <xsl:variable name="order" select="format-number(($position - $startDatePos - 1), '#')"/>
      <!-- local variables -->
      <xsl:variable name="xcoord4set"
        select="format-number(($minXcoord + ($order * $xIntervalBetweenDates)), '#')"/>
        
      <xsl:for-each select="id">
        <!-- local variables to draw bar graph - rect -->
        <xsl:variable name="barNum" select="position()"/>
        <xsl:variable name="numbytes" select="numBytes"/>
        <xsl:variable name="ycoord">
            <!-- Calculate logarithmic value for bytes -->
            <!-- If numbytes < minY, i.e. 1024, then level at 1k, -->
            <!-- If numbytes > maxY, i.e. 1125899906842624, then level at 1Pb -->
            <xsl:choose>
                <xsl:when test="number($numbytes) &lt;= $minY">
                    <xsl:value-of select="$maxYcoord"/>
                </xsl:when>
                <xsl:when test="number($numbytes) &gt; $maxY">
                    <xsl:value-of select="$minYcoord"/>
                </xsl:when>
                <xsl:otherwise>
                    <xsl:variable name="log2Value" 
                                  select="format-number((Math:log(Long:parseLong($numbytes)) div Math:log(2.0)), '#.###########')"/>
                    <xsl:value-of select="format-number(($maxYcoord - (($log2Value - $logMinY) * $yCoordInterval)), '#.##########')"/>
                </xsl:otherwise>
            </xsl:choose>
        </xsl:variable>

        <xsl:variable name="height" select="format-number(($maxYcoord - $ycoord), '0.##########')"/>
        
        <!-- start draw rect for bar graph -->
        <xsl:text disable-output-escaping="yes">&lt;rect fill='#E9EBEC' stroke='#71838D' x='</xsl:text>
          <xsl:value-of select="format-number(($xcoord4set + (($barNum - 1) * $barWidth)), '#')"/>
        <xsl:text>' y='</xsl:text>
          <xsl:value-of select="$ycoord"/>
        <xsl:text>' height='</xsl:text>
          <xsl:value-of select="$height"/>
        <xsl:text disable-output-escaping="yes">' width='</xsl:text>
          <xsl:value-of select="$barWidth"/>
        <xsl:text disable-output-escaping="yes">'/&gt;</xsl:text>
        <!-- end draw rect for bar graph -->
      
        <!-- display the name of the owner -->
        <xsl:text disable-output-escaping="yes">&lt;g transform='translate(</xsl:text>
        <xsl:value-of select="format-number(($xcoord4set + (($barNum - 1) * $barWidth) + 8), '#')"/>
        <xsl:text disable-output-escaping="yes">,</xsl:text>
        <xsl:value-of select="format-number(($maxYcoord - 10), '#')"/>
        <xsl:text disable-output-escaping="yes">)'&gt;</xsl:text>
        <xsl:text disable-output-escaping="yes">&lt;g transform='rotate(-90)'&gt;</xsl:text>
        <xsl:text disable-output-escaping="yes">&lt;text style='font-weight:bold; font-size:10; fill:#000000; letter-spacing:2; text-anchor:start;'&gt;</xsl:text>
        <!-- if name = unknown, use id instead -->
        <xsl:choose>
            <xsl:when test="name = 'unknown'">
                <xsl:value-of select="@val"/>
            </xsl:when>
            <xsl:otherwise>
                <xsl:value-of select="name"/>
            </xsl:otherwise>
        </xsl:choose>
        
        <xsl:text disable-output-escaping="yes">&lt;/text&gt;&lt;/g&gt;&lt;/g&gt;</xsl:text>  
      </xsl:for-each>
    </xsl:if>
  </xsl:template>
  
  <!-- =======================================================          -->
  <!-- fill the imagemap -->
  <xsl:template match="filesystem/date" mode="imagemap">
    <xsl:param name="totalDate" select="0"/>
    
    <xsl:variable name="position" select="position()"/>
    <!-- only 4 dates can be displayed -->
    <xsl:variable name="startDatePos">
        <xsl:choose>
            <xsl:when test="$totalDate &gt; $maxDateCount">
                <xsl:value-of select="format-number(($totalDate - $maxDateCount), '#')"/>
            </xsl:when>
            <xsl:otherwise>
                <xsl:value-of select="0"/>
            </xsl:otherwise>  
        </xsl:choose>
    </xsl:variable>
            
    <!-- current position should be greater than startDatePos -->
    <xsl:if test="$position &gt; $startDatePos">
      <!-- get the relative position from the startDatePos -->
      <xsl:variable name="order" select="format-number(($position - $startDatePos - 1), '#')"/>
      <!-- local variables -->
      <!-- xcoord for the set of bar graphs corresponding to a single date -->
      <xsl:variable name="xcoord4set"
        select="format-number(($minXcoord + ($order * $xIntervalBetweenDates)), '#')"/>

      <xsl:for-each select="id">
        <!-- local variables -->
        <xsl:variable name="barNum" select="position()"/>
        <xsl:variable name="xleft"
          select="format-number(($xcoord4set + (($barNum - 1) * $barWidth)), '#')"/>
        <!-- if numbytes < 1MB, do not calculate log --> 
        <xsl:variable name="numbytes" select="numBytes"/>
        <xsl:variable name="yupper">
            <!-- Calculate logarithmic value for bytes -->
            <!-- If numbytes < minY, i.e. 1024, then level at 1k, -->
            <!-- If numbytes > maxY, i.e. 1125899906842624, then level at 1Pb -->
            <xsl:choose>
                <xsl:when test="number($numbytes) &lt;= $minY">
                    <xsl:value-of select="$maxYcoord"/>
                </xsl:when>
                <xsl:when test="number($numbytes) &gt; $maxY">
                    <xsl:value-of select="$minYcoord"/>
                </xsl:when>
                <xsl:otherwise>
                    <xsl:variable name="log2Value" 
                                  select="format-number((Math:log(Long:parseLong($numbytes)) div Math:log(2.0)), '#.###########')"/>
                    <xsl:value-of select="format-number(($maxYcoord - (($log2Value - $logMinY) * $yCoordInterval)), '#.##########')"/>
                </xsl:otherwise>
            </xsl:choose>
        </xsl:variable>
        
        <xsl:variable name="onlineCapacity">
          <xsl:choose>
                <xsl:when test="number(numOnlineBytes) &gt; $maxY">
                    <xsl:value-of select="string('> PB')"/>
                </xsl:when>
                <xsl:otherwise>
                    <xsl:value-of select="Capacity:toString(Capacity:new(Long:parseLong(numOnlineBytes), 0))"/>
                </xsl:otherwise>
            </xsl:choose>
        </xsl:variable>
        
        <xsl:variable name="size">
            <xsl:choose>
                <xsl:when test="number(numBytes) &gt; $maxY">
                    <xsl:value-of select="string('> PB')"/>
                </xsl:when>
                <xsl:otherwise>
                    <xsl:value-of select="Capacity:toString(Capacity:new(Long:parseLong(numBytes), 0))"/>
                </xsl:otherwise>
            </xsl:choose>
        </xsl:variable>
                    
        <xsl:variable name="tooltipStr">
            <xsl:value-of select="string(SamUtil:getResourceString('common.text.fileSize', $size))"/>
            <xsl:text> </xsl:text>
            <xsl:value-of select="string(SamUtil:getResourceString('common.text.fileCount', numFiles))"/>
            <xsl:text> </xsl:text>
            <xsl:value-of select="string(SamUtil:getResourceString('common.text.onlineCapacity', $onlineCapacity))"/>
        </xsl:variable>
        
        <!-- imagemap rect -->
        <area shape='rect' 
              coords='{$xleft},{$yupper},{($xleft + $barWidth)},{$maxYcoord}' 
              alt='moreInfo' title='{$tooltipStr}'/>
              
      </xsl:for-each>
    </xsl:if>
  </xsl:template>
  
</xsl:stylesheet>
