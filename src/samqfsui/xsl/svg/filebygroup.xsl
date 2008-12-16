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
<!--Copyright 2009 Sun Microsystems, Inc.  All rights reserved.         -->
<!--Use is subject to license terms.                                      -->
<!--                                                                      -->
<!--  SAM-QFS_notice_end                                                  -->
<!--                                                                      -->
<!-- $Id: filebygroup.xsl,v 1.13 2008/12/16 00:12:31 am143972 Exp $ -->

<xsl:stylesheet 
    version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:svg="http://www.w3.org/2000/svg"
    xmlns:SamUtil="com.sun.netstorage.samqfs.web.util.SamUtil"
    xmlns:Capacity="com.sun.netstorage.samqfs.web.util.Capacity"
    xmlns:Math="java.lang.Math"
    xmlns:Long="java.lang.Long">
    
    <!-- Import other sytlesheets -->
    <xsl:import href="/xsl/svg/common.xsl"/>
    <xsl:import href="/xsl/svg/baseusrgrp.xsl"/>
    
    <!-- Write the result tree to a serial output file - serialization    -->
    <xsl:output 
        method="xml"
        media-type="image/svg+xml"
        encoding="ISO-8859-1" 
        indent="yes"
        omit-xml-declaration="yes"/>      
    
    <!-- Localization/Internationalization - Use SamUtil                  -->
    
    <!-- Parameters passed to this stylesheet: startTime and endTime -->
    <xsl:param name="startTime" select="1113339900"/>
    <xsl:param name="endTime" select="1162501201"/>
    <!-- the startTime and endTime are ignored for now, only the last four
    dates for which data is available is displayed -->
    
    <!-- Main start                                                       -->
    <xsl:template match="/MetricReport">
        <svg width="700" height="700">
            
            <xsl:call-template name="setup-screen"/>
            <xsl:call-template name="display-title"/>
            <xsl:call-template name="display-subtitle"/>
            <xsl:call-template name="display-yaxis-label"/>
            <xsl:call-template name="display-xaxis-label"/>
            <!-- x-axis tick labels are displayed in the filesystem template -->
            <!-- display graph -->
            <xsl:apply-templates select="filesystem"/>
            
        </svg>
        <!-- construct the imagemap -->
        <xsl:variable name="totalDate" select="count(filesystem/date)"/>
        <xsl:text disable-output-escaping="yes">&lt;!-- imageMapSection --&gt;</xsl:text>
        <xsl:apply-templates select="filesystem/date" mode="imagemap">
            <xsl:sort select="@val" order="ascending" data-type="number"/>
            <xsl:with-param name="totalDate" select="$totalDate"/>
        </xsl:apply-templates>
    </xsl:template>
    
    <!-- display title, text for graph title is "Arial", Bold, 16 pt -->
    <!-- Graph title should be center-aligned to the graph, and positioned 10 pixels above subtitle -->
    <xsl:template name="display-title">
        <text style="font-family:arial; font-weight:bold; font-size:16; fill:#666666; letter-spacing:1.5; text-anchor:middle;" 
              x="310" y="20">
            <xsl:value-of select="string(SamUtil:getResourceString('reports.title.type.filebygroup'))"/>
        </text>
    </xsl:template>
    
    <!-- display subtitle, text for graph title is "Arial", Plain, 12 pt -->
    <!-- Graph subtitle should be center-aligned to the graph, and positioned 10 pixels above graph -->
    <xsl:template name="display-subtitle">
        <text style="font-family:arial; font-weight:normal; font-size:12; letter-spacing:1.5; text-anchor:middle;" 
              x="310" y="40">
            <xsl:value-of select="string(SamUtil:getResourceString('reports.subtitle.type.filebygroup'))"/>
        </text>
    </xsl:template>
    
    <!-- display yaxis label, text for the axis label is "Arial," Bold, 12 pt, #666666 -->
    <!-- Vertical axis labels are rotated 90 degrees to read vertically beside the axis -->
    <xsl:template name="display-yaxis-label">
        <xsl:text disable-output-escaping="yes">&lt;g transform='translate(25,340)'&gt;</xsl:text>
        <g transform="rotate(-90)">
            <text style="font-family:arial; font-weight:bold; font-size:12; letter-spacing:1.5; fill:#666666;">
                <xsl:value-of select="string(SamUtil:getResourceString('reports.filedistribution.filebygroup.yaxistext'))"/>
            </text>
        </g>
        <xsl:text disable-output-escaping="yes">&lt;/g&gt;</xsl:text>    
    </xsl:template>
    
    <!-- display xaxis label, text for the axis label is "Arial," Bold, 12 pt, #666666 -->
    <xsl:template name="display-xaxis-label">
        <text style="font-family:arial; font-weight:bold; font-size:12; letter-spacing:1.5; fill:#666666; text-anchor:middle;" 
              x="310" y="445">
            <xsl:value-of select="string(SamUtil:getResourceString('reports.filedistribution.bargraph.xaxistext'))"/>
        </text>
    </xsl:template>
    
</xsl:stylesheet>
