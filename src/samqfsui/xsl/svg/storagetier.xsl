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
<!-- $Id: storagetier.xsl,v 1.17 2008/12/16 00:12:32 am143972 Exp $ -->

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
    <xsl:import href="/xsl/svg/common.xsl"/>
    
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
    <xsl:param name="hwm" select="0"/> <!-- will be set by caller -->
    
    <xsl:variable name="TIER_TYPE_DISKCACHE" select="0"/>
    <xsl:variable name="TIER_TYPE_DISKARCHIVE" select="1"/>
    <xsl:variable name="TIER_TYPE_HONEYCOMB" select="2"/>
    <xsl:variable name="TIER_TYPE_TAPE" select="3"/>
    <xsl:variable name="OFFSET_TAPE" select="3"/>
    <xsl:variable name="OFFSET_DISKARCHIVE" select="number(-3)"/>
    
    <xsl:template match="/MetricReport">
        <svg width="700" height="700">
            
            <xsl:call-template name="setup-screen"/>
            <xsl:call-template name="display-title"/>
            <xsl:call-template name="display-subtitle"/>
            <xsl:call-template name="display-yaxis-label"/>
            <xsl:call-template name="display-xaxis-ticks"/>
            <xsl:call-template name="display-xaxis-label"/>
            <xsl:call-template name="display-legend"/>
            <!-- display graph -->
            <xsl:apply-templates select="filesystem"/>
            
        </svg>
        <!-- construct the imagemap, the caller will split the result to get svg and imagemap -->
        <xsl:text disable-output-escaping="yes">&lt;!-- imageMapSection --&gt;</xsl:text>
        <xsl:call-template name="create-imagemap"/>
    </xsl:template>
    
    
    <xsl:template match="filesystem">
        
        <!-- Prerequisite -->
        <!-- If date element exists, entries for all ranges are mandatory -->
        <!-- x interval is calculated based on startTime and endTime -->
       
        <xsl:variable name="total" select="count(date)"/>
        <!-- If count is 0, display 'no items found' -->
        <xsl:choose>
            <xsl:when test="$total &gt; 0">
                
                                
                <!-- the graph is drawn using the polyline shape, the x/y coord are -->
                <!-- obtained from the value of numBytes and date of each tier entry -->
                <!-- date corresponds to x-axis point while the numBytes for that -->
                <!-- date corresponds to y-axis point. e.g. 
                polyline points="80,200 120,154 160,114 200,145 240,144 280,189 320,179"
                fill="none" stroke="#996600" stroke-width="1"
                marker-start="url(#markerCircle)"
                marker-mid="url(#markerCircle)"
                marker-end="url(#markerCircle)"
                -->
        
                <!-- graph 2 -->
                <xsl:text disable-output-escaping="yes">&lt;polyline points='</xsl:text>	
                <xsl:for-each select="date">
                    <xsl:sort select="@val" order="ascending" data-type="number"/>
                    <xsl:call-template name="fill-coords">
                        <xsl:with-param name="currentDate" select="@val"/>
                        <xsl:with-param name="numbytes" select="diskArchive"/>
                        <xsl:with-param name="tiertype" select="$TIER_TYPE_DISKARCHIVE"/>
                    </xsl:call-template>
                </xsl:for-each>
                <xsl:text disable-output-escaping="yes">
                ' fill='none' stroke='#6699CC' stroke-width='4' marker-start='url(#marker-6699CC)' marker-mid='url(#marker-6699CC)' marker-end='url(#marker-6699CC)'</xsl:text>
                <xsl:text disable-output-escaping="yes">/&gt;</xsl:text>
                
                <!-- graph 3 -->
                <xsl:text disable-output-escaping="yes">&lt;polyline points='</xsl:text>	
                <xsl:for-each select="date">
                    <xsl:sort select="@val" order="ascending" data-type="number"/>
                    <xsl:call-template name="fill-coords">
                        <xsl:with-param name="currentDate" select="@val"/>
                        <xsl:with-param name="numbytes" select="tapeArchive"/>
                        <xsl:with-param name="tiertype" select="$TIER_TYPE_TAPE"/>
                    </xsl:call-template>
                </xsl:for-each>
                <xsl:text disable-output-escaping="yes">
                ' fill='none' stroke='#669966' stroke-width='2' marker-start='url(#marker-669966)' marker-mid='url(#marker-669966)' marker-end='url(#marker-669966)'</xsl:text>
                <xsl:text disable-output-escaping="yes">/&gt;</xsl:text>
                
                <!-- graph 4 for Honeycomb -->
                <xsl:text disable-output-escaping="yes">&lt;polyline points='</xsl:text>	
                <xsl:for-each select="date">
                    <xsl:sort select="@val" order="ascending" data-type="number"/>
                    <xsl:call-template name="fill-coords">
                        <xsl:with-param name="currentDate" select="@val"/>
                        <xsl:with-param name="numbytes" select="honeycombArchive"/>
                        <xsl:with-param name="tiertype" select="$TIER_TYPE_HONEYCOMB"/>
                    </xsl:call-template>
                </xsl:for-each>
                <xsl:text disable-output-escaping="yes">
                ' fill='none' stroke='#CC9900' stroke-width='2' marker-start='url(#marker-CC9900)' marker-mid='url(#marker-CC9900)' marker-end='url(#marker-CC9900)'</xsl:text>
                <xsl:text disable-output-escaping="yes">/&gt;</xsl:text>

                <!-- graph 1 -->
                <xsl:text disable-output-escaping="yes">&lt;polyline points='</xsl:text>
                <xsl:for-each select="date">
                    <xsl:sort select="@val" order="ascending" data-type="number"/>
                    <xsl:call-template name="fill-coords">
                        <xsl:with-param name="currentDate" select="@val"/>
                        <xsl:with-param name="numbytes" select="diskCache"/>
                    </xsl:call-template>
                </xsl:for-each>
                <xsl:text disable-output-escaping="yes">
                ' fill='none' stroke='#000000' stroke-width='2' marker-start='url(#marker-000000)' marker-mid='url(#marker-000000)' marker-end='url(#marker-000000)'</xsl:text>
                <xsl:text disable-output-escaping="yes">/&gt;</xsl:text>
                
                <!-- display the high water mark threshold for SAM systems -->
                <xsl:if test="number($hwm) &gt; $minY and number($hwm) &lt; $maxY">
                    <xsl:variable name="hwmlogValue" 
                                  select="format-number((Math:log(Long:parseLong($hwm)) div Math:log(2.0)), '#.###########')"/>
                    <xsl:variable name="hwmycoord"
                                  select="format-number(($maxYcoord - (($hwmlogValue - $logMinY) * $yCoordInterval)), '#.##########')"/>
                    
                    <path fill='none' 
                          stroke='#CC0000' stroke-width='1' stroke-dasharray='4' 
                          d='M {$minXcoord},{$hwmycoord} h {$totalXLen}'/>                   
                </xsl:if>
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
    
    <!-- get the x/y coord for the path along the graph -->
    <xsl:template name="fill-coords">
        <xsl:param name="currentDate"/>
        <xsl:param name="numbytes"/>
        <xsl:param name="tiertype" select="$TIER_TYPE_DISKCACHE"/>
        
        <!-- offset the graphs from each other to allow better visibility -->
        <xsl:variable name="yoffset">
            <xsl:choose>
                <xsl:when test="$tiertype = $TIER_TYPE_DISKARCHIVE">
                    <xsl:value-of select="number($OFFSET_DISKARCHIVE)"/>
                </xsl:when>
                <xsl:when test="$tiertype = $TIER_TYPE_TAPE">
                    <xsl:value-of select="number($OFFSET_TAPE)"/>
                </xsl:when>
                <xsl:otherwise>
                    <xsl:value-of select="number(0)"/>
                </xsl:otherwise>
            </xsl:choose>
        </xsl:variable>
        
        <xsl:variable name="xcoord">
            <xsl:choose>
                <xsl:when test="$endTime = $startTime">
                    <xsl:value-of select="$minXcoord"/>
                </xsl:when>
                <xsl:otherwise>
                    <xsl:value-of select="format-number(($minXcoord + (($totalXLen div ($endTime - $startTime)) * ($currentDate - $startTime))), '0.##')"/>
                </xsl:otherwise>      
            </xsl:choose>
        </xsl:variable>
        
        <!-- Calculate logarithmic value for bytes -->
        <!-- If numbytes < minY, i.e. 1048576, then level at 1M, -->
        <!-- If numbytes > maxY, i.e. 1125899906842624, then level at 1Pb -->
        <xsl:variable name="ycoord">
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
                    <xsl:value-of select="format-number(($maxYcoord - (($log2Value - $logMinY) * $yCoordInterval) + $yoffset), '#.##########')"/>
                </xsl:otherwise>
            </xsl:choose>
        </xsl:variable>
        
        <!-- write the x,y coords -->
        <xsl:value-of select="$xcoord"/>
        <xsl:text>,</xsl:text>
        <xsl:value-of select="$ycoord"/>
        <xsl:if test="position() != last()"><xsl:text> </xsl:text></xsl:if>
    </xsl:template>
    
    <xsl:template name="create-imagemap">
        
        <xsl:for-each select="filesystem">
            
            <!-- Hotspot is not useful if large number of points, but show anyway -->
            
            <xsl:for-each select="date">
                <xsl:sort select="@val" order="ascending" data-type="number"/>
                
                <xsl:variable name="xcoord">
                    <xsl:choose>
                        <xsl:when test="$endTime = $startTime">
                            <xsl:value-of select="$minXcoord"/>
                        </xsl:when>
                        <xsl:otherwise>
                            <xsl:value-of select="format-number(($minXcoord + (($totalXLen div ($endTime - $startTime)) * (@val - $startTime))), '0.##')"/>
                        </xsl:otherwise>      
                    </xsl:choose>
                </xsl:variable>
                
                <xsl:call-template name="drawHotSpot">
                    <xsl:with-param name="numbytes" select="tapeArchive"/>
                    <xsl:with-param name="xcoord" select="$xcoord"/>
                    <xsl:with-param name="tiertype" select="$TIER_TYPE_TAPE"/>
                </xsl:call-template>
                
                <xsl:call-template name="drawHotSpot">
                    <xsl:with-param name="numbytes" select="diskArchive"/>
                    <xsl:with-param name="xcoord" select="$xcoord"/>
                    <xsl:with-param name="tiertype" select="$TIER_TYPE_DISKARCHIVE"/>
                </xsl:call-template>
                
                <xsl:call-template name="drawHotSpot">
                    <xsl:with-param name="numbytes" select="diskCache"/>
                    <xsl:with-param name="xcoord" select="$xcoord"/>
                </xsl:call-template>
                
                <xsl:call-template name="drawHotSpot">
                    <xsl:with-param name="numbytes" select="honeycombArchive"/>
                    <xsl:with-param name="xcoord" select="$xcoord"/>
                    <xsl:with-param name="tiertype" select="$TIER_TYPE_HONEYCOMB"/>
                </xsl:call-template>
                
            </xsl:for-each>        
            
        </xsl:for-each>
    </xsl:template>
    
    <xsl:template name="drawHotSpot">
        <xsl:param name="numbytes"/>
        <xsl:param name="xcoord"/>
        <xsl:param name="tiertype" select="$TIER_TYPE_DISKCACHE"/>
        
        <!-- offset the graphs from each other to allow better visibility -->
        <xsl:variable name="yoffset">
            <xsl:choose>
                <xsl:when test="$tiertype = $TIER_TYPE_DISKARCHIVE">
                    <xsl:value-of select="number($OFFSET_DISKARCHIVE)"/>
                </xsl:when>
                <xsl:when test="$tiertype = $TIER_TYPE_TAPE">
                    <xsl:value-of select="number($OFFSET_TAPE)"/>
                </xsl:when>
                <xsl:otherwise>
                    <xsl:value-of select="number(0)"/>
                </xsl:otherwise>
            </xsl:choose>
        </xsl:variable>
        
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
                    <xsl:value-of select="format-number(($maxYcoord - (($log2Value - $logMinY) * $yCoordInterval) + $yoffset), '#.##########')"/>
                </xsl:otherwise>
            </xsl:choose>
        </xsl:variable>
        
        <!-- convert to appropriate units using the Capacity class  -->
        <!-- in xslt 1.0, you cannot specify that a variable should hold an int/long -->
        <!-- do conversion only if < 1 PB, Capacity class does not handle larger values -->
        <xsl:variable name="capacity">
            <xsl:choose>
                <xsl:when test="number($numbytes) &gt; $maxY">
                    <xsl:value-of select="string('> PB')"/>
                </xsl:when>
                <xsl:otherwise>
                    <xsl:value-of select="Capacity:toString(Capacity:new(Long:parseLong($numbytes), 0))"/>
                </xsl:otherwise>
            </xsl:choose>
        </xsl:variable>
        
        <xsl:variable name="tooltipStr" select="string(SamUtil:getResourceString('common.text.fileSize', $capacity))"/>
        
        <area shape='circle' coords='{$xcoord},{$ycoord},5' alt='moreInfo' title='{$tooltipStr}'/>
        
        
    </xsl:template>
    
    <!-- display legend (conventions) -->
    <xsl:template name="display-legend">
        <rect fill='none' stroke='#BEC7CC' x='80' y='460' width='470' height='40'/>
        <g>
            <line stroke="#669966" stroke-width="10" x1="85" y1="472" x2="95" y2="472"/>
            <text style="font-weight:normal; font-size:12; letter-spacing:1.5;" x="100" y="475">
                <xsl:value-of select="SamUtil:getResourceString('reports.filedistribution.storagetier.tapearchive')"/>
            </text>
            <line stroke="#6699CC" stroke-width="10" x1="205" y1="472" x2="215" y2="472"/>
            <text style="font-weight:normal; font-size:12; letter-spacing:1.5;" x="220" y="475">
                <xsl:value-of select="SamUtil:getResourceString('reports.filedistribution.storagetier.diskarchive')"/>
            </text>
            <line stroke="#CC9900" stroke-width="10" x1="315" y1="472" x2="325" y2="472"/>
            <text style="font-weight:normal; font-size:12; letter-spacing:1.5;" x="330" y="475">
                <xsl:value-of select="SamUtil:getResourceString('reports.filedistribution.storagetier.honeycombarchive')"/>
            </text>
            <line stroke="#000000" stroke-width="10" x1="85" y1="488" x2="95" y2="488"/>
            <text style="font-weight:normal; font-size:12; letter-spacing:1.5;" x="100" y="491">
                <xsl:value-of select="SamUtil:getResourceString('reports.filedistribution.storagetier.diskcache')"/>
            </text>
            <!-- <line stroke="#CC0000" stroke-width="10" x1="205" y1="488" x2="215" y2="488"/>-->
            <!--<text style="font-weight:normal; font-size:12; letter-spacing:1.5;" x="220" y="491">-->
            <!--    <xsl:value-of select="SamUtil:getResourceString('reports.filedistribution.storagetier.hwm')"/>-->
            <!--</text>-->
        </g>
        <text style="font-weight:normal; font-size:10; letter-spacing:1.5;" x="80" y="510">
            <xsl:value-of select="SamUtil:getResourceString('reports.filedistribution.legend.title')"/>
        </text>
    </xsl:template>
    
    <!-- display title, text for graph title is "Arial", Bold, 16 pt -->
    <!-- Graph title should be center-aligned to the graph, and positioned 10 pixels above subtitle -->
    <xsl:template name="display-title">
        <text style="font-family:arial; font-weight:bold; font-size:16; fill:#666666; letter-spacing:1.5; text-anchor:middle;" 
              x="310" y="20">
            <xsl:value-of select="string(SamUtil:getResourceString('reports.title.type.storagetier'))"/>
        </text>
    </xsl:template>
    
    <!-- display subtitle, text for graph title is "Arial", Plain, 12 pt -->
    <!-- Graph subtitle should be center-aligned to the graph, and positioned 10 pixels above graph -->
    <xsl:template name="display-subtitle">
        <text style="font-family:arial; font-weight:normal; font-size:12; letter-spacing:1.5; text-anchor:middle;" 
              x="310" y="40">
            <xsl:value-of select="string(SamUtil:getResourceString('reports.subtitle.type.storagetier'))"/>
        </text>
    </xsl:template>
    
    <!-- display yaxis label, text for the axis label is "Arial," Bold, 12 pt, #666666 -->
    <!-- Vertical axis labels are rotated 90 degrees to read vertically beside the axis -->
    <xsl:template name="display-yaxis-label">
        <xsl:text disable-output-escaping="yes">&lt;g transform='translate(25,340)'&gt;</xsl:text>
        <g transform="rotate(-90)">
            <text style="font-family:arial; font-weight:bold; font-size:12; letter-spacing:1.5; fill:#666666;">
                <xsl:value-of select="string(SamUtil:getResourceString('reports.filedistribution.storagetier.yaxistext'))"/>
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
