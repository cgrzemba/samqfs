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
<!-- $Id: filebyage.xsl,v 1.15 2008/05/16 18:39:10 am143972 Exp $ -->

<xsl:stylesheet 
    version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:svg="http://www.w3.org/2000/svg"
    xmlns:xlink="http://www.w3.org/1999/xlink"
    xmlns:exsl="http://exslt.org/common" extension-element-prefixes="exsl"
    xmlns:SamUtil="com.sun.netstorage.samqfs.web.util.SamUtil"
    xmlns:Capacity="com.sun.netstorage.samqfs.web.util.Capacity"
    xmlns:Math="java.lang.Math"
    xmlns:Long="java.lang.Long">
    
    <!-- Import other sytlesheets -->
    <xsl:import href="/xsl/svg/common.xsl"/>
    <xsl:import href="/xsl/svg/baserange.xsl"/>
    
    <!-- Write the result tree to a serial output file - serialization -->
    <xsl:output 
        method="xml"
        media-type="image/svg+xml"
        encoding="ISO-8859-1" 
        indent="yes"
        omit-xml-declaration="yes"/>      
    
    <!-- Localization/Internationalization - Use SamUtil -->
    
    <!-- Parameters passed to this stylesheet: startTime and endTime -->
    <xsl:param name="startTime" select="1160398800"/>
    <xsl:param name="endTime" select="1162990800"/>
    
    <xsl:template match="/MetricReport">
        <svg width="700" height="700">
            
            <xsl:call-template name="setup-screen"/>
            <xsl:call-template name="display-title"/>
            <xsl:call-template name="display-subtitle"/>
            <xsl:call-template name="display-yaxis-label"/>
            <xsl:call-template name="display-xaxis-ticks"/>
            <xsl:call-template name="display-xaxis-label"/>
            <xsl:call-template name="display-legend">
                <xsl:with-param name="name" select="string('filebyage')"/>
            </xsl:call-template>
            <!-- display graph -->
            <xsl:apply-templates select="filesystem"/>
            
        </svg>
        <!-- construct the imagemap, the caller will split the result to get svg and imagemap -->
        <xsl:text disable-output-escaping="yes">&lt;!-- imageMapSection --&gt;</xsl:text>
        <xsl:call-template name="create-imagemap"/>
    </xsl:template>
    
    <xsl:template match="filesystem">
        
        <!-- Prerequisites -->
        <!-- 1) If date element exists, entries for all ranges are mandatory -->
        <!-- total length of x-Axis = 470 -->
        <!-- x interval is calculated based on date range -->
        <!-- (provided as stylesheet parameter) elements -->

        <xsl:variable name="total" select="count(date)"/>
        
        <!-- If count is 0, display 'no items found' -->
        <xsl:choose>
            <xsl:when test="$total &gt; 0">
                
                <!-- the 5 graphs are drawn using polyline, the x/y coord are -->
                <!-- obtained from value of numBytes and date of each range entry -->
                <!-- date corresponds to x-axis point while the numBytes for that -->
                <!-- date corresponds to y-axis point. e.g. 
                polyline points="80,200 120,154 160,114 200,145 240,144 280,189 320,179"
                fill="none" stroke="#996600" stroke-width="2"
                marker-start="url(#marker)"
                marker-mid="url(#marker)"
                marker-end="url(#marker)"
                -->
        
                <!-- graph 1 -->
                <xsl:text disable-output-escaping="yes">&lt;polyline points='</xsl:text>
                <xsl:for-each select="date/range[@val= '>365']">
                    <xsl:sort select="ancestor::date/@val" order="ascending" data-type="number"/>
                    <xsl:call-template name="fill-coords">
                        <xsl:with-param name="currentDate" select="ancestor::date/@val"/>
                    </xsl:call-template>
                </xsl:for-each>
                <xsl:text disable-output-escaping="yes">
                ' fill='none' stroke='#669966' stroke-width='2' marker-start='url(#marker-669966)' marker-mid='url(#marker-669966)' marker-end='url(#marker-669966)'</xsl:text>
                <xsl:text disable-output-escaping="yes">/&gt;</xsl:text>
                
                <!-- graph 2 -->
                <xsl:text disable-output-escaping="yes">&lt;polyline points='</xsl:text>	
                <xsl:for-each select="date/range[@val= '180-365']">
                    <xsl:sort select="ancestor::date/@val" order="ascending" data-type="number"/>
                    <xsl:call-template name="fill-coords">
                        <xsl:with-param name="currentDate" select="ancestor::date/@val"/>
                    </xsl:call-template>
                </xsl:for-each>
                <xsl:text disable-output-escaping="yes">
                ' fill='none' stroke='#6699CC' stroke-width='2' marker-start='url(#marker-6699CC)' marker-mid='url(#marker-6699CC)' marker-end='url(#marker-6699CC)'</xsl:text>
                <xsl:text disable-output-escaping="yes">/&gt;</xsl:text>
                
                <!-- graph 3 -->
                <xsl:text disable-output-escaping="yes">&lt;polyline points='</xsl:text>	
                <xsl:for-each select="date/range[@val= '90-180']">
                    <xsl:sort select="ancestor::date/@val" order="ascending" data-type="number"/>
                    <xsl:call-template name="fill-coords">
                        <xsl:with-param name="currentDate" select="ancestor::date/@val"/>
                    </xsl:call-template>
                </xsl:for-each>
                <xsl:text disable-output-escaping="yes">
                ' fill='none' stroke='#CC9900' stroke-width='2' marker-start='url(#marker-CC9900)' marker-mid='url(#marker-CC9900)' marker-end='url(#marker-CC9900)'</xsl:text>
                <xsl:text disable-output-escaping="yes">/&gt;</xsl:text>
                
                <!-- graph 4 -->
                <xsl:text disable-output-escaping="yes">&lt;polyline points='</xsl:text>	
                <xsl:for-each select="date/range[@val= '30-90']">
                    <xsl:sort select="ancestor::date/@val" order="ascending" data-type="number"/>
                    <xsl:call-template name="fill-coords">
                        <xsl:with-param name="currentDate" select="ancestor::date/@val"/>
                    </xsl:call-template>
                </xsl:for-each>
                <xsl:text disable-output-escaping="yes">
                ' fill='none' stroke='#000000' stroke-width='2' marker-start='url(#marker-000000)' marker-mid='url(#marker-000000)' marker-end='url(#marker-000000)'</xsl:text>
                <xsl:text disable-output-escaping="yes">/&gt;</xsl:text>
                
                <!-- graph 5 -->
                <xsl:text disable-output-escaping="yes">&lt;polyline points='</xsl:text>	
                <xsl:for-each select="date/range[@val= '0-30']">
                    <xsl:sort select="ancestor::date/@val" order="ascending" data-type="number"/>
                <xsl:call-template name="fill-coords">
                        <xsl:with-param name="currentDate" select="ancestor::date/@val"/>
                    </xsl:call-template>
                </xsl:for-each>
                <xsl:text disable-output-escaping="yes">
                ' fill='none' stroke='#CC0000' stroke-width='2' marker-start='url(#marker-CC0000)' marker-mid='url(#marker-CC0000)' marker-end='url(#marker-CC0000)'</xsl:text>
                <xsl:text disable-output-escaping="yes">/&gt;</xsl:text>
                
            </xsl:when>
            <!-- If no data available -->
            <xsl:otherwise>
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
            </xsl:otherwise>
        </xsl:choose> 
    </xsl:template>
    
    <!-- display title, text for graph title is "Arial", Bold, 16 pt -->
    <!-- Graph title should be center-aligned to the graph, and positioned 10 pixels above subtitle -->
    <xsl:template name="display-title">
        <text style="font-family:arial; font-weight:bold; font-size:16; fill:#666666; letter-spacing:1.5; text-anchor:middle;" 
              x="310" y="20">
            <xsl:value-of select="string(SamUtil:getResourceString('reports.title.type.filebyage'))"/>
        </text>
    </xsl:template>
    
    <!-- display subtitle, text for graph title is "Arial", Plain, 12 pt -->
    <!-- Graph subtitle should be center-aligned to the graph, and positioned 10 pixels above graph -->
    <xsl:template name="display-subtitle">
        <text style="font-family:arial; font-weight:normal; font-size:12; letter-spacing:1.5; text-anchor:middle;" 
              x="310" y="40">
            <xsl:value-of select="string(SamUtil:getResourceString('reports.subtitle.type.filebyage'))"/>
        </text>
    </xsl:template>
    
    <!-- display yaxis label, text for the axis label is "Arial," Bold, 12 pt, #666666 -->
    <!-- Vertical axis labels are rotated 90 degrees to read vertically beside the axis -->
    <xsl:template name="display-yaxis-label">
        <xsl:text disable-output-escaping="yes">&lt;g transform='translate(25,340)'&gt;</xsl:text>
        <g transform="rotate(-90)">
            <text style="font-family:arial; font-weight:bold; font-size:12; letter-spacing:1.5; fill:#666666;">
                <xsl:value-of select="string(SamUtil:getResourceString('reports.filedistribution.filebyage.yaxistext'))"/>
            </text>
        </g>
        <xsl:text disable-output-escaping="yes">&lt;/g&gt;</xsl:text>    
    </xsl:template>
    
    <!-- display xaxis label, text for the axis label is "Arial," Bold, 12 pt, #666666 -->
    <xsl:template name="display-xaxis-label">
        <text style="font-family:arial; font-weight:bold; font-size:12; letter-spacing:1.5; fill:#666666; text-anchor:middle;" 
              x="310" y="445">
            <xsl:value-of select="string(SamUtil:getResourceString('reports.filedistribution.linegraph.xaxistext'))"/>
        </text>
    </xsl:template>
    
</xsl:stylesheet>
