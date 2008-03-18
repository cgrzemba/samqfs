<?xml version="1.0" encoding="ISO-8859-1"?>

<!--  SAM-QFS_notice_begin                                                -->
<!--                                                                      -->
<!--CDDL HEADER START                                                     -->
<!--                                                                      -->
<!--The contents of this file are subject to the terms of the             -->
<!--Common Development and Distribution License (the "License").          -->
<!--You may not use this file except in compliance with the License.      -->
<!--                                                                      -->
<!--You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE   -->
<!--or http://www.opensolaris.org/os/licensing.                           -->
<!--See the License for the specific language governing permissions       -->
<!--and limitations under the License.                                    -->
<!--                                                                      -->
<!--When distributing Covered Code, include this CDDL HEADER in each      -->
<!--file and include the License file at usr/src/OPENSOLARIS.LICENSE.     -->
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
<!-- $Id: common.xsl,v 1.14 2008/03/17 14:44:04 am143972 Exp $ -->

<xsl:stylesheet 
    version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:exsl="http://exslt.org/common" extension-element-prefixes="exsl"
    xmlns:svg="http://www.w3.org/2000/svg"
    xmlns:xlink="http://www.w3.org/1999/xlink"
    xmlns:SamUtil="com.sun.netstorage.samqfs.web.util.SamUtil"
    xmlns:FileMetricSummaryViewBean="com.sun.netstorage.samqfs.web.admin.FileMetricSummaryViewBean"
    xmlns:Capacity="com.sun.netstorage.samqfs.web.util.Capacity"
    xmlns:Math="java.lang.Math"
    xmlns:Double="java.lang.Double"
    xmlns:Long="java.lang.Long">

  <!-- Although XSLT 2.0 is released, FSM is using XSLT 1.0, released in    -->
  <!-- Nov 99 because the Xalan processor that comes with Java JDK 1.4      -->
  <!-- only supports XSLT 1.0. With XSLT 2.0, the only choice of processors -->
  <!-- is Saxon -->
  
  <!-- Usability guide -->
  <!-- http://xdesign.sfbay.sun.com/projects/swaed/uispec3_0/08-complex.html#8.6 -->
  
  <!-- SVG specification -->
  <!-- http://www.w3.org/TR/SVG/text.html -->
  
  <!-- =======================================================          -->
  <!-- Global variables start                                           -->
  
  <xsl:variable name="maxXcoord" select="550"/>
  <xsl:variable name="minXcoord" select="80"/>
  <xsl:variable name="totalXLen" select="470"/>
  <xsl:variable name="MAX_X_LABEL_POINTS" select="10"/> <!-- for date labels -->
  
  <xsl:variable name="maxYcoord" select="400"/>
  <xsl:variable name="minYcoord" select="55"/>
  <xsl:variable name="totalYLen" select="345"/>
  <!-- max is 1 PB log(2^50) min is 1 MB log(2^20)                        -->
  <!-- 2^50 corresponds to minYcoord i.e. 50                            -->
  <!-- 2^10 corresponds to maxYcoord i.e. 350                           -->
  <!-- Use the following formula to calculate the ycoord                -->
  <!-- $maxYcoord - ($logbase2(bytes) * $yCoordInterval)                -->
  <xsl:variable name="maxY" select="number(1125899906842624)"/>
  <xsl:variable name="minY" select="number(1048576)"/>
  <xsl:variable name="totalYPts" select="30"/> <!-- 2^20 to 2^50 -->
  <xsl:variable name="logMinY" select="20"/> <!-- log (1048576) / log (2) -->
  
  <xsl:variable name="yCoordInterval" 
                select="format-number(($totalYLen div $totalYPts), '#.##########')"/>
  

  <!-- =======================================================          -->
  <!-- Global variables end                                             -->
  
  <!-- =======================================================          -->
  <!-- setup Screen                                                     -->
  <xsl:template name="setup-screen">
      <xsl:param name="width" select="700"/>
      <xsl:param name="height" select="700"/>
    <!-- =======================================================        -->
      <!-- definitions                                                    -->
      <desc>Define marker for data points</desc>
      
      <defs>
          <marker id="marker-669966" markerWidth="2" markerHeight="2" 
                  viewBox="0 0 4 4" orient="0" refX="2" refY="2"
                  markerUnits="strokeWidth">
              <circle cx="2" cy="2" r="2" fill="none" stroke="#669966"/>
          </marker>
      </defs>
      
      <defs>
          <marker id="marker-6699CC" markerWidth="2" markerHeight="2" 
                  viewBox="0 0 4 4" orient="0" refX="2" refY="2"
                  markerUnits="strokeWidth">
              <circle cx="2" cy="2" r="2" fill="none" stroke="#6699CC" />
          </marker>
      </defs>
      
      <defs>
          <marker id="marker-CC9900" markerWidth="2" markerHeight="2" 
                  viewBox="0 0 4 4" orient="0" refX="2" refY="2"
                  markerUnits="strokeWidth">
              <circle cx="2" cy="2" r="2" fill="none" stroke="#CC9900" />
          </marker>
      </defs>
      
      <defs>
          <marker id="marker-000000" markerWidth="2" markerHeight="2" 
                  viewBox="0 0 4 4" orient="0" refX="2" refY="2"
                  markerUnits="strokeWidth">
              <circle cx="2" cy="2" r="2" fill="none" stroke="#000000" />
          </marker>
      </defs>
      
      <defs>
          <marker id="marker-CC0000" markerWidth="2" markerHeight="2" 
                  viewBox="0 0 4 4" orient="0" refX="2" refY="2"
                  markerUnits="strokeWidth">
              <circle cx="2" cy="2" r="2" fill="none" stroke="#CC0000" />
          </marker>
      </defs>
      
      <defs>
          <marker id="marker-0000FF" markerWidth="2" markerHeight="2" 
                  viewBox="0 0 4 4" orient="0" refX="2" refY="2"
                  markerUnits="strokeWidth">
              <circle cx="2" cy="2" r="2" fill="none" stroke="#0000FF" />
          </marker>
      </defs>
    <!-- =======================================================        -->
    <!-- definitions end                                                -->        
    
  
    <!-- draw rect for jpeg -->
    <rect fill='none' x='1' y='1' width='{$width}' height='{$height}'/>
    
    <!-- draw X axis: a horizontal line from x1,y2 with length=totalXLen -->
    <path fill='none' stroke='#999999' stroke-width='1' 
          d='M {$minXcoord},{$maxYcoord} h {$totalXLen}'/>
    
    <!-- draw Y axis: a vertical line from x1,y2 with length=-totalYLen (up direction)-->
    <path fill='none' stroke='#999999' stroke-width='1' 
          d='M {$minXcoord},{$maxYcoord} v -{$totalYLen}'/>
    
    <!-- display y axis points -->
    <xsl:call-template name="display-y-ticks"/>
    
    <!-- @isSample is used to indicate that this is a sample -->
    <!-- If sample, display watermark -->
    <xsl:if test="@isSample">
        <g transform="translate(330, 205)">
        <g transform="rotate(-45)">
            <text style="font-weight:bold; font-size:48; text-anchor:middle; fill:#BEC7CC; letter-spacing:1.5;">
                <xsl:value-of select="string(SamUtil:getResourceString('reports.filedistribution.sample'))"/>
            </text>
        </g>
        </g>
    </xsl:if>
    
 
  </xsl:template>
  <!-- =======================================================          -->
  
  
  <!--  generate y axis tick labels and tick lines -->
  <!-- y axis uses a logarithmic (base 2) scale (2^10 - 2^50) for display -->
  <!-- this function is called recursively when y= 10 to 49 -->
  <xsl:template name="display-y-ticks">
    <xsl:param name="y" select="20"/> <!-- start at 2^20 -->
  
    <xsl:variable name="yPt" select="format-number(($maxYcoord - (($y - $logMinY) * $yCoordInterval)), '#')"/>
    <xsl:variable name="yVal" select="format-number($y, '#')"/>
    <!-- find 2^y -->
    <xsl:variable name="yPow2" select="format-number(Math:pow(2, Double:parseDouble($yVal)), '#')"/>
    
    <!-- convert to appropriate units using the Capacity class  -->
    <!-- in xslt 1.0, you cannot specify that a variable should hold an int/long -->
    <xsl:variable name="capacity" 
      select="Capacity:toString(Capacity:newExactCapacity(Long:parseLong($yPow2), 0))"/>
      
      <xsl:if test="$yVal mod 2 = 0">
          <!-- draw a tick mark to indicate the divisions on the scale -->
          <xsl:variable name="tickx1" select="format-number(($minXcoord - 4), '#')"/>
          <line stroke='#999999' stroke-width='1' 
                x1='{$tickx1}' y1='{$yPt}' x2='{$minXcoord}' y2='{$yPt}'/>
          
          <!-- draw a horizontal tick line across yPt -->
          <xsl:if test="$yVal != 10"> <!-- don't draw the X0 line as dotted -->
              <line stroke='#CCCCCC' stroke-width='1' stroke-dasharray='2' 
                    x1='{$minXcoord}' y1='{$yPt}' 
                    x2='{$maxXcoord}' y2='{$yPt}'/>
          </xsl:if>
      </xsl:if>   
      
      <!-- Use a minimum of 10 pixels distance between the tick labels on axis -->
      <!-- If tick mark labels are on the left of the table, the text aligns right -->
      <xsl:if test="$yVal mod 2 = 0">
          <xsl:variable name="xVal" select="format-number(($minXcoord - 8), '#')"/>
          <text font-family='arial' font-weight='normal' 
                font-style='normal' font-size='10' 
                text-anchor='end' x='{$xVal}' y='{$yPt}'>
              <xsl:value-of select="$capacity"/>
          </text>       
      </xsl:if>
    
    <!-- Call template recursively to display all the y-coords  -->
    <xsl:if test="$y &lt; 50">
      <xsl:variable name="next" select="format-number(($y + 1), '#')"/>
      
      <xsl:call-template name="display-y-ticks">
        <xsl:with-param name="y" select="$next"/>
      </xsl:call-template>
    </xsl:if>
  </xsl:template>
  <!-- =======================================================          -->
  <!--
    display-date - recursive template to display the date along the X-axis, given the
    startTime and dateInterval. The xcoord is calculated using the following formula:
    minXcoord + (n * xcoordInterval), n = 0, 1, 2...MAX_X_LABEL_POINTS
    The X-Axis is a linear display of dates. These dates are not used from the
    XML (data) but are input to the stylesheet via stylesheet parameters, it
    cooresponds to the stard/end dates chosen by the user
    -->
    <xsl:template name="display-xaxis-ticks">
        <xsl:param name="recursiveCount" select="0"/>       
        
        <xsl:variable name="dateInterval"
                      select="format-number((($endTime - $startTime) div $MAX_X_LABEL_POINTS), '#')"/>
        
        <xsl:variable name="xcoordInterval"
                      select="format-number((($maxXcoord - $minXcoord) div $MAX_X_LABEL_POINTS), '#')"/>
  
        <xsl:variable name="xcoord"
                      select="format-number(($minXcoord + ($recursiveCount * $xcoordInterval)), '#')"/>
        
        <xsl:variable name="dateVal"
                      select="format-number(($startTime + ($recursiveCount * $dateInterval)), '#')"/>
        
        
        <xsl:variable name="displayDate"
                      select="FileMetricSummaryViewBean:getDateTimeString(Long:parseLong($dateVal))"/>
        <!-- split into date string and time string (separated by space) -->
        
        <!-- SVG does not provide for automatic line breaks or word wrapping -->
        <!-- To conserve horizontal space, use tspan to split date and time  -->
        <!-- on two lines, i.e. 10/8/05 on 1st line and 10:00 AM on 2nd line -->
        <xsl:variable name="yline1" select="format-number(($maxYcoord + 15), '#')"/>
        <xsl:variable name="yline2" select="format-number(($maxYcoord + 27), '#')"/>
        <g>
            <text style='font-family:arial;font-weight:normal;font-style:normal;font-size:10;text-anchor:middle;'>
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
        
        <xsl:if test="$recursiveCount &gt; 0 and $recursiveCount &lt; $MAX_X_LABEL_POINTS">
            <line stroke='#CCCCCC' stroke-width='1' stroke-dasharray='2' 
                  x1='{$xcoord}' y1='{$minYcoord}' 
                  x2='{$xcoord}' y2='{$maxYcoord}'/>
        </xsl:if>
          
        <!-- Call template recursively to display all the x-coords  -->
        <xsl:if test="$recursiveCount &lt; $MAX_X_LABEL_POINTS and $dateInterval &gt; 0">
            
            <xsl:call-template name="display-xaxis-ticks">
                <xsl:with-param name="recursiveCount"
                                select="format-number(($recursiveCount + 1), '#')"/>                   
            </xsl:call-template>
            
        </xsl:if>
    </xsl:template>
    <!-- =======================================================          -->
  
  
</xsl:stylesheet>
