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
<!-- $Id: baserange.xsl,v 1.9 2008/12/16 00:12:31 am143972 Exp $ -->

<xsl:stylesheet 
    version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:svg="http://www.w3.org/2000/svg"
    xmlns:SamUtil="com.sun.netstorage.samqfs.web.util.SamUtil"
    xmlns:Capacity="com.sun.netstorage.samqfs.web.util.Capacity"
    xmlns:Math="java.lang.Math"
    xmlns:Long="java.lang.Long">
    
    <!-- =======================================================          -->
    <xsl:template name="fill-coords">
        <xsl:param name="currentDate"/>
        
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
        
        <!-- get the numbytes for the ycoord                    -->
        <xsl:variable name="numbytes" select="numBytes"/>
        
        <!-- Calculate logarithmic value for bytes -->
        <!-- If numbytes < minY, i.e. 1024, then level at 1k, -->
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
                    <xsl:value-of select="format-number(($maxYcoord - (($log2Value - $logMinY) * $yCoordInterval)), '#.##########')"/>
                </xsl:otherwise>
            </xsl:choose>
        </xsl:variable>
        
        <!-- write the x,y coords -->
        <xsl:value-of select="$xcoord"/>
        <xsl:text>,</xsl:text>
        <xsl:value-of select="$ycoord"/>
        <xsl:if test="position() != last()"><xsl:text> </xsl:text></xsl:if>
        
    </xsl:template>
    
    <!-- display legend (conventions) -->
    <xsl:template name="display-legend">
        <xsl:param name="name" select="filebyage"/>
        
        <rect fill='none' stroke='#BEC7CC' x='80' y='460' width='470' height='55'/>
        <text style="font-weight:bold; font-size:12; letter-spacing:1.5;" x="88" y="475">
            <xsl:choose>
                <xsl:when test="$name = 'filebylife'">
                    <xsl:value-of select="string(SamUtil:getResourceString('reports.filedistribution.range.filebylife.desc'))"/>
                </xsl:when>
                <xsl:when test="$name = 'filebyage'">
                    <xsl:value-of select="string(SamUtil:getResourceString('reports.filedistribution.range.filebyage.desc'))"/>
                </xsl:when>
            </xsl:choose>
        </text>
        <g>
            <line stroke="#669966" stroke-width="10" x1="85" y1="488" x2="95" y2="488"/>
            <text style="font-weight:normal; font-size:12; letter-spacing:1.5;" x="100" y="491">
                <xsl:value-of select="string(SamUtil:getResourceString('reports.filedistribution.range.above365days'))"/>
            </text>
            <line stroke="#6699CC" stroke-width="10" x1="185" y1="488" x2="195" y2="488"/>
            <text style="font-weight:normal; font-size:12; letter-spacing:1.5;" x="200" y="491">
                <xsl:value-of select="string(SamUtil:getResourceString('reports.filedistribution.range.180to365days'))"/>
            </text>
            <line stroke="#CC9900" stroke-width="10" x1="315" y1="488" x2="325" y2="488"/>
            <text style="font-weight:normal; font-size:12; letter-spacing:1.5;" x="330" y="491">
                <xsl:value-of select="string(SamUtil:getResourceString('reports.filedistribution.range.90to180days'))"/>
            </text>
            <line stroke="#000000" stroke-width="10" x1="425" y1="488" x2="435" y2="488"/>
            <text style="font-weight:normal; font-size:12; letter-spacing:1.5;" x="440" y="491">
                <xsl:value-of select="SamUtil:getResourceString('reports.filedistribution.range.30to90days')"/>
            </text>
            <line stroke="#CC0000" stroke-width="10" x1="85" y1="504" x2="95" y2="504"/>
            <text style="font-weight:normal; font-size:12; letter-spacing:1.5;" x="100" y="507">
                <xsl:value-of select="string(SamUtil:getResourceString('reports.filedistribution.range.0to30days'))"/>
            </text>
            <xsl:if test="$name = 'filebylife'">
                <line stroke="#0000FF" stroke-width="10" x1="185" y1="504" x2="195" y2="504"/>
                <text style="font-weight:normal; font-size:12; letter-spacing:1.5;" x="200" y="507">
                    <xsl:value-of select="string(SamUtil:getResourceString('reports.filedistribution.range.0days'))"/>
                </text>
            </xsl:if>
        </g>
        <text style="font-weight:normal; font-size:10; letter-spacing:1.5;" x="80" y="525">
            <xsl:value-of select="string(SamUtil:getResourceString('reports.filedistribution.legend.title'))"/>
        </text>
        
    </xsl:template>
    
    <!-- Construct imagemap, iterate through date elements and for each range, -->
    <!-- get the x-y coords and fill the title with additional information     -->
    <xsl:template name="create-imagemap">
        
        <xsl:for-each select="filesystem">
            
            <!-- If there are a large number of date elements, hotspot is useless -->
            <!-- display anyways -->
            <xsl:variable name="total" select="count(date)"/>
            
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
                
                <xsl:for-each select="range">
                    <!-- get the numbytes for the ycoord                    -->
                    <xsl:variable name="numbytes" select="numBytes"/>
                    <!-- Calculate logarithmic value for bytes -->
                    <!-- If numbytes < minY, i.e. 1024, then level at 1k, -->
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
                                <xsl:value-of select="format-number(($maxYcoord - (($log2Value - $logMinY) * $yCoordInterval)), '#.##########')"/>
                            </xsl:otherwise>
                        </xsl:choose>
                    </xsl:variable>
        
                    
                    <!-- convert to appropriate units using the Capacity class  -->
                    <!-- in xslt 1.0, you cannot specify that a variable should hold an int/long -->
                    <!-- do conversion only if < Long.MAX_VALUE(9223372049231779576) -->
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
                        <xsl:value-of select="string(SamUtil:getResourceString('common.text.fileCount', numfiles))"/>
                        <xsl:text> </xsl:text>
                        <xsl:value-of select="string(SamUtil:getResourceString('common.text.onlineCapacity', $onlineCapacity))"/>
                    </xsl:variable>

                    <area shape='circle' coords='{$xcoord},{$ycoord},5' alt='moreInfo' title='{$tooltipStr}'/>
                    
                </xsl:for-each>
            </xsl:for-each>
            
        </xsl:for-each>
    </xsl:template>
    
    <!-- =======================================================          -->
    
</xsl:stylesheet>
