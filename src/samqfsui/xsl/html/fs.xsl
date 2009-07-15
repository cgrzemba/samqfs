<?xml version="1.0" encoding="ISO-8859-1"?>

<!--  SAM-QFS_notice_begin

    CDDL HEADER START

    The contents of this file are subject to the terms of the
    Common Development and Distribution License (the "License").
    You may not use this file except in compliance with the License.

    You can obtain a copy of the license at pkg/OPENSOLARIS.LICENSE
    or http://www.opensolaris.org/os/licensing.
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
<!-- $Id: fs.xsl,v 1.13 2008/12/16 00:12:31 am143972 Exp $ -->

<xsl:stylesheet 
    version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:Capacity="com.sun.netstorage.samqfs.web.util.Capacity"
    xmlns:Math="java.lang.Math"
    xmlns:Long="java.lang.Long"
    xmlns:Integer="java.lang.Integer"
    xmlns:SamQFSUtil="com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSUtil"
    xmlns:SamUtil="com.sun.netstorage.samqfs.web.util.SamUtil">
  
    <!-- Import other sytlesheets -->
    <xsl:import href="/xsl/html/common.xsl"/>
 
    <!-- File System Summary -->
    <xsl:template match="filesystems">
        <xsl:variable name="dateVal" select="normalize-space(../@time)"/>
        <xsl:variable name="displayDate" 
        select="SamUtil:getTimeString(Long:parseLong($dateVal))"/>
        <xsl:variable name="title" 
        select="SamUtil:getResourceString('common.title.capacity')"/>
    
        <table class="Tbl">
            <caption class="TblTtlTxt">
                <xsl:choose>
                    <xsl:when test="../@desc">
                        <xsl:value-of select="SamUtil:getResourceString('reports.sample.desc.fs')"/>
                    </xsl:when>
                    <xsl:otherwise>
                        <xsl:value-of select="$title"/><xsl:text> (</xsl:text>
                        <xsl:value-of select="$displayDate"/>
                        <xsl:text>)</xsl:text>
                    </xsl:otherwise>
                </xsl:choose>
                
            </caption>
            <tr> 
                <xsl:copy-of select="$columnheader.name"/>
                <xsl:copy-of select="$columnheader.mounted"/>
                <xsl:copy-of select="$columnheader.total"/>
                <xsl:copy-of select="$columnheader.free"/>
                <xsl:copy-of select="$columnheader.hwm"/>
                <xsl:copy-of select="$columnheader.lwm"/>
                <xsl:copy-of select="$columnheader.hwmExceed"/>
            </tr>
            <!-- If there are no children, then display 'No items found' -->
            <xsl:call-template name="emptyTableCondition">
                <xsl:with-param name="colspan" select="7"/>
            </xsl:call-template>
            <!-- If error, display the samerrmsg -->
            <xsl:call-template name="errorCondition">
                <xsl:with-param name="colspan" select="7"/>
            </xsl:call-template>
      
            <xsl:apply-templates select="fs">
                <xsl:sort select="name"/>
            </xsl:apply-templates>
        </table>
    </xsl:template>
    
    <xsl:template match="fs">
        <!-- convert to appropriate units using the Capacity class  -->
        <!-- in xslt 1.0, you cannot specify that a variable should hold an int/long -->
        <xsl:variable name="capacityWithUnit">
            <xsl:choose>
                <xsl:when test="number(@capacity) &gt; Long:MAX_VALUE">
                    <xsl:value-of select="@capacity"/>
                </xsl:when>
                <xsl:otherwise>        
                    <xsl:value-of select="Capacity:toString(Capacity:new(Long:parseLong(@capacity), 0))"/>
                </xsl:otherwise>    
            </xsl:choose>
        </xsl:variable>
                
        <xsl:variable name="freeWithUnit">
        <xsl:choose>
            <xsl:when test="number(@free) &gt; Long:MAX_VALUE">
                <xsl:value-of select="@free"/>
            </xsl:when>
            <xsl:otherwise>        
                <xsl:value-of select="Capacity:toString(Capacity:new(Long:parseLong(@free), 0))"/>
            </xsl:otherwise>
        </xsl:choose>
        </xsl:variable>
              
        <tr>
            <xsl:call-template name="evenRowAttrCondition"/>
            <td><xsl:value-of select="@name"/></td> 
            <td><xsl:value-of select="@mounted"/></td>
      
            <td align="right">
                <xsl:if test="@mounted = 'Yes'">
                    <xsl:value-of select="$capacityWithUnit"/>
                </xsl:if>
            </td> 
      
            <td align="right">
                <xsl:if test="@mounted = 'Yes'">
                    <xsl:value-of select="$freeWithUnit"/>
                </xsl:if>
            </td> 
              
            <td align="right">
                <xsl:if test="@mounted = 'Yes'">
                    <xsl:value-of select="@hwm"/>
                </xsl:if>
            </td>
              
            <td align="right">
                <xsl:if test="@mounted = 'Yes'">
                    <xsl:value-of select="@lwm"/>
                </xsl:if>
            </td>
 
            <!-- If @timesHwmExceeded does not exist, it must be a QFS/UFS system -->
            <td>
                <xsl:choose>
                    <xsl:when test="@timesHwmExceeded">
                        <!-- if 0, don't display -->
                        <xsl:variable name="count" select="@timesHwmExceeded"/>
                        <xsl:choose>
                            <xsl:when test="$count &gt; 10">
                                <xsl:text disable-output-escaping="yes">
                                    &lt;font color='#FF0000'&gt;&lt;b&gt;</xsl:text>
                                <xsl:value-of select="$count"/>
                                <xsl:text disable-output-escaping="yes">
                                    &lt;/b&gt;&lt;/font&gt;</xsl:text>
                            </xsl:when>
                            <xsl:when test="$count &gt; 0">
                                <xsl:value-of select="$count"/>                       
                            </xsl:when>
                        </xsl:choose>
                    </xsl:when>
                    <xsl:otherwise>
                        <xsl:value-of 
                            select="SamUtil:getResourceString('common.text.notapplicable')"/>
                    
                    </xsl:otherwise>
                </xsl:choose>
            </td> 
        </tr> 
    </xsl:template>

</xsl:stylesheet>
