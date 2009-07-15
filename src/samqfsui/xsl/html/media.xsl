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
<!-- $Id: media.xsl,v 1.14 2008/12/16 00:12:31 am143972 Exp $ -->

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
  
  <!-- Generic Media Summary -->
  <xsl:template match="media">
    <!-- ignore ../@name as it is currently not meaningful -->
    <xsl:variable name="title" select="SamUtil:getResourceString('reports.type.desc.media')"/>
    <xsl:variable name="dateVal" select="normalize-space(../@time)"/>
    
    <xsl:variable name="displayDate" 
      select="SamUtil:getTimeString(Long:parseLong($dateVal))"/>
    <table align="center">
      <tr>
        <td align="center" width="100%">
          <h2>
            <xsl:choose>
              <xsl:when test="../@desc"> <!-- for sample -->
                <xsl:value-of select="SamUtil:getResourceString('reports.sample.desc.media')"/>
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="$title"/><xsl:text> (</xsl:text><xsl:value-of select="$displayDate"/><xsl:text>)</xsl:text>
              </xsl:otherwise>
            </xsl:choose>
          </h2>
        </td>        
      </tr>
    </table>
       
    <xsl:apply-templates select="errorVsns"/>
    <xsl:apply-templates select="copyUtilization"/>
    <xsl:apply-templates select="blankVsns"/>
    <xsl:apply-templates select="accessedVsns"/>
    <xsl:apply-templates select="pools"/>
    <xsl:apply-templates select="vsnSummary"/>
    
  </xsl:template>
  
  
  <!-- errorVsns: -->
  <!-- The status of VSNs are provided as bit-values, convert to Strings -->
  <xsl:template match="errorVsns">
    <table class="Tbl">  
      <caption class="TblTtlTxt">
        <xsl:value-of select="SamUtil:getResourceString('common.title.vsnsRequireOperatorAttn')"/>
      </caption>
      <tr> 
        <xsl:copy-of select="$columnheader.name"/>
        <xsl:copy-of select="$columnheader.type"/>
        <xsl:copy-of select="$columnheader.status"/>
      </tr>
      <xsl:call-template name="emptyTableCondition"><xsl:with-param name="colspan" select="3"/></xsl:call-template>
      
      <xsl:for-each select="vsn">
          
        <xsl:variable name="typeStr" select="SamUtil:getMediaTypeString(@type)"/>
        <xsl:variable name="statusStr" select="SamUtil:getStatusString(Integer:parseInt(@status))"/>
        
        <tr>
          <td><xsl:value-of select="@name"/></td>
          <td><xsl:value-of select="$typeStr"/></td>
          <!-- statusStr might have <br> tags, don't convert it to &lt;br&gt; -->
          <td><xsl:value-of select="$statusStr" disable-output-escaping="yes"/></td>
        </tr>
        
      </xsl:for-each>
    </table>
  </xsl:template>
  
  <xsl:template match="blankVsns">
    <br />
    <br />  
    <table class="Tbl">  
      <caption class="TblTtlTxt">
        <xsl:value-of select="SamUtil:getResourceString('vsn.status.blank')"/>
      </caption>
      <tr> 
        <xsl:copy-of select="$columnheader.name"/>
        <xsl:copy-of select="$columnheader.type"/>
      </tr>
      <xsl:call-template name="emptyTableCondition"><xsl:with-param name="colspan" select="2"/></xsl:call-template>
      
      <xsl:for-each select="vsn">
          
        <xsl:variable name="typeStr" select="SamUtil:getMediaTypeString(@type)"/>

        <tr>
          <td><xsl:value-of select="@name"/></td>
          <td><xsl:value-of select="$typeStr"/></td>
        </tr>
      </xsl:for-each>

    </table>
  </xsl:template>
 
  <xsl:template match="accessedVsns">
    <br />
    <br />  
    <table class="Tbl">  
      <caption class="TblTtlTxt">
        <xsl:value-of select="SamUtil:getResourceString('vsn.status.inuse')"/>
      </caption>
      <tr> 
        <xsl:copy-of select="$columnheader.name"/>
        <xsl:copy-of select="$columnheader.type"/>
      </tr>
      <xsl:call-template name="emptyTableCondition"><xsl:with-param name="colspan" select="2"/></xsl:call-template>
      
      <xsl:for-each select="vsn">
        <!-- <xsl:sort select="vsn/@name" order="ascending" data-type="text"/> -->
   
        <xsl:variable name="typeStr" select="SamUtil:getMediaTypeString(@type)"/>
        
        <tr>
          <td><xsl:value-of select="@name"/></td>
          <td><xsl:value-of select="$typeStr"/></td>
        </tr>
      </xsl:for-each>
    </table>
  </xsl:template>
  
  <xsl:template match="copyUtilization">
    <br />
    <br />  
    <table class="Tbl">  
      <caption class="TblTtlTxt">
        <xsl:value-of select="SamUtil:getResourceString('common.title.copyUtilization')"/>
      </caption>
      <tr> 
        <xsl:copy-of select="$columnheader.name"/>
        <xsl:copy-of select="$columnheader.type"/>        
        <xsl:copy-of select="$columnheader.total"/>
        <xsl:copy-of select="$columnheader.free"/>
        <xsl:copy-of select="$columnheader.used"/>
      </tr>
      <xsl:call-template name="emptyTableCondition"><xsl:with-param name="colspan" select="5"/></xsl:call-template>
      
      <xsl:for-each select="copy">
        
        <xsl:variable name="typeStr" select="SamUtil:getMediaTypeString(@type)"/>
        <!-- convert to appropriate units using the Capacity class  -->
        <!-- in xslt 1.0, you cannot specify that a variable should hold an int/long -->
        <!-- capacity is given in kb -->
        <xsl:variable name="capacityWithUnit" 
          select="Capacity:toString(Capacity:new(Long:parseLong(@capacity), 1))"/>
        <xsl:variable name="freeWithUnit" 
          select="Capacity:toString(Capacity:new(Long:parseLong(@free), 1))"/>
          
        <tr>
          <td><xsl:value-of select="@name"/></td>
          <td><xsl:value-of select="$typeStr"/></td>
          <td align="right"><xsl:value-of select="$capacityWithUnit"/></td>
          <td align="right"><xsl:value-of select="$freeWithUnit"/></td>
          <td align="right"><xsl:value-of select="@usage"/></td>
        </tr>

      </xsl:for-each>
    </table>
  </xsl:template>
  
  <xsl:template match="vsnSummary">
    <br />
    <br />  
    <table class="Tbl"> 
      <caption class="TblTtlTxt">
        <xsl:value-of select="SamUtil:getResourceString('common.title.vsnSummary')"/>
      </caption>
      <tr> 
        <xsl:copy-of select="$columnheader.type"/>
        <xsl:copy-of select="$columnheader.count"/>
        <xsl:copy-of select="$columnheader.total"/>
        <xsl:copy-of select="$columnheader.free"/>
      </tr>
      <xsl:call-template name="emptyTableCondition"><xsl:with-param name="colspan" select="4"/></xsl:call-template>
      
      <xsl:for-each select="vsn">
        
        <xsl:variable name="typeStr" select="SamUtil:getMediaTypeString(@type)"/>
        <!-- convert to appropriate units using the Capacity class  -->
        <!-- in xslt 1.0, you cannot specify that a variable should hold an int/long -->
        <!-- capacity is given as bytes for disk vsns, but as kb for tape vsns -->
        <xsl:variable name="capacityWithUnit">
            <xsl:choose>
                <xsl:when test="@type = 'dk'">
                    <xsl:value-of select="Capacity:toString(Capacity:new(Long:parseLong(@capacity), 0))"/>
                </xsl:when>
                <xsl:otherwise>
                    <xsl:value-of select="Capacity:toString(Capacity:new(Long:parseLong(@capacity), 1))"/>
                </xsl:otherwise>           
            </xsl:choose>
        </xsl:variable>
        <xsl:variable name="freeWithUnit">
            <xsl:choose>
                <xsl:when test="@type = 'dk'">
                    <xsl:value-of select="Capacity:toString(Capacity:new(Long:parseLong(@free), 0))"/>
                </xsl:when>
                <xsl:otherwise> 
                    <xsl:value-of select="Capacity:toString(Capacity:new(Long:parseLong(@free), 1))"/>
                </xsl:otherwise>
            </xsl:choose>
        </xsl:variable>
          
        <tr>
          <xsl:call-template name="evenRowAttrCondition"/>
          <td><xsl:value-of select="$typeStr"/></td>
          <td align="right"><xsl:value-of select="@count"/></td>     
          <td align="right"><xsl:value-of select="$capacityWithUnit"/></td>
          <td align="right"><xsl:value-of select="$freeWithUnit"/></td>  
        </tr>
      </xsl:for-each>
    </table>
  </xsl:template>
      
  <xsl:template match="pools">
    <br />
    <br />
    <table class="Tbl"> 
      <caption class="TblTtlTxt">
        <xsl:value-of select="SamUtil:getResourceString('common.title.archiverPoolUtilization')"/>
      </caption> 
      <tr> 
        <xsl:copy-of select="$columnheader.name"/>
        <xsl:copy-of select="$columnheader.type"/>
        <xsl:copy-of select="$columnheader.count"/>
        <xsl:copy-of select="$columnheader.free"/> 
      </tr>
      <xsl:call-template name="emptyTableCondition"><xsl:with-param name="colspan" select="4"/></xsl:call-template>
      <xsl:apply-templates select="pool"/>
    </table>
  </xsl:template>
  
  <xsl:template match="pool">
    
    <xsl:variable name="typeStr" select="SamUtil:getMediaTypeString(@type)"/>
    <!-- convert to appropriate units using the Capacity class  -->
    <!-- in xslt 1.0, you cannot specify that a variable should hold an int/long -->
    <!-- capacity is given in kb -->
    <xsl:variable name="freeWithUnit" 
      select="Capacity:toString(Capacity:new(Long:parseLong(@free), 1))"/>
        
    <tr> 
      <xsl:call-template name="evenRowAttrCondition"/>
      <td><xsl:value-of select="normalize-space(@name)"/></td> 
      <td><xsl:value-of select="$typeStr"/></td> 
      <td align="right"><xsl:value-of select="@count"/></td> 
      <td align="right"><xsl:value-of select="$freeWithUnit"/></td>  
    </tr>

  </xsl:template>
  
</xsl:stylesheet>
