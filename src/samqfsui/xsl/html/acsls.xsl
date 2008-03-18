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
<!-- $Id: acsls.xsl,v 1.11 2008/03/17 14:44:03 am143972 Exp $ -->

<xsl:stylesheet 
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:Long="java.lang.Long"
  xmlns:SamUtil="com.sun.netstorage.samqfs.web.util.SamUtil">

  <!-- Import other sytlesheets -->
  <xsl:import href="/xsl/html/common.xsl"/>

  <!-- acsls summary -->
  <xsl:template match="acsls">
    <!-- ignore ../@name as it is currently not meaningful -->
    <xsl:variable name="title" select="SamUtil:getResourceString('reports.type.desc.acsls')"/>
    
    <xsl:variable name="dateVal" select="normalize-space(../@time)"/>
    <xsl:variable name="displayDate" 
      select="SamUtil:getTimeString(Long:parseLong($dateVal))"/>
    <table align="center">
      <tr>
        <td width="100%">
          <h2>
            <xsl:choose>
              <xsl:when test="../@desc">
                <xsl:value-of select="SamUtil:getResourceString('reports.sample.desc.acsls')"/>
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="$title"/><xsl:text> (</xsl:text><xsl:value-of select="$displayDate"/><xsl:text>)</xsl:text>
              </xsl:otherwise>
            </xsl:choose>
          </h2>
        </td>
      </tr>
    </table>

    <xsl:apply-templates select="drives"/>
    <xsl:apply-templates select="accessedVolumes"/>
    <xsl:apply-templates select="enteredVolumes"/>
    <xsl:apply-templates select="locks"/>
    <xsl:apply-templates select="scratchPools"/>
  </xsl:template>
  
  <!-- acsls drive summary -->
  <xsl:template match="drives">
    <table class="Tbl"> 
      <caption class="TblTtlTxt">
        <xsl:value-of select="SamUtil:getResourceString('reports.acsls.status.drives')"/>
      </caption>
      <tr> 
        <xsl:copy-of select="$columnheader.drive"/>
        <xsl:copy-of select="$columnheader.acs"/>
        <xsl:copy-of select="$columnheader.lsm"/>
        <xsl:copy-of select="$columnheader.panel"/>
        <xsl:copy-of select="$columnheader.serial"/>
        <xsl:copy-of select="$columnheader.type"/>
        <xsl:copy-of select="$columnheader.status"/>
      </tr> 
      <!-- If there are no children, then display 'No items found' -->
      <xsl:call-template name="emptyTableCondition"><xsl:with-param name="colspan" select="7"/></xsl:call-template>
      <xsl:apply-templates select="drive">
        <xsl:sort select="driveId"/>
      </xsl:apply-templates>
    </table>
  </xsl:template>                 
  <xsl:template match="drive">
    <tr>
      <xsl:call-template name="evenRowAttrCondition"/>
      <td><xsl:value-of select='@driveId'/></td>
      <td><xsl:value-of select='@acsId'/></td>
      <td><xsl:value-of select='@lsmId'/></td>
      <td><xsl:value-of select='@panelId'/></td>
      <td><xsl:value-of select='@serial'/></td>
      <td><xsl:value-of select='@type'/></td>
      <td><xsl:value-of select='@state'/></td>
    </tr>
  </xsl:template>
  <!-- accessed volumes -->
  <xsl:template match='accessedVolumes'>
    <br />
    <br />
    <table class="Tbl"> 
      <caption class="TblTtlTxt">
        <xsl:value-of select="SamUtil:getResourceString('reports.acsls.status.accessedvolumes')"/>
      </caption>
      <tr>
        <xsl:copy-of select="$columnheader.vsn"/>
        <xsl:copy-of select="$columnheader.acs"/>
        <xsl:copy-of select="$columnheader.lsm"/>
        <xsl:copy-of select="$columnheader.panel"/>
        <xsl:copy-of select="$columnheader.row"/>
        <xsl:copy-of select="$columnheader.col"/>
        <xsl:copy-of select="$columnheader.pool"/>
        <xsl:copy-of select="$columnheader.media"/>
        <xsl:copy-of select="$columnheader.type"/>
        <xsl:copy-of select="$columnheader.status"/>
      </tr>
      <!-- If there are no children, then display 'No items found' -->
      <xsl:call-template name="emptyTableCondition"><xsl:with-param name="colspan" select="10"/></xsl:call-template>
      <xsl:apply-templates select="volume">
        <xsl:sort select="volId"/>
      </xsl:apply-templates>
    </table>
  </xsl:template>
  <xsl:template match='volume'>
    <tr>
      <xsl:call-template name="evenRowAttrCondition"/>
      <td><xsl:value-of select='@name'/></td>
      <td><xsl:value-of select='@acsId'/></td>
      <td><xsl:value-of select='@lsmId'/></td>
      <td><xsl:value-of select='@panelId'/></td>
      <td><xsl:value-of select='@row'/></td>
      <td><xsl:value-of select='@col'/></td>
      <td><xsl:value-of select='@poolId'/></td>      
      <td><xsl:value-of select='@mtype'/></td>
      <td><xsl:value-of select='@volType'/></td>
      <td><xsl:value-of select='@status'/></td>
    </tr>
  </xsl:template>

  <xsl:template match='enteredVolumes'>
    <br />
    <br />
    <table class="Tbl"> 
      <caption class="TblTtlTxt">
        <xsl:value-of select="SamUtil:getResourceString('reports.acsls.status.enteredvolumes')"/>
      </caption>
      <tr>
        <xsl:copy-of select="$columnheader.vsn"/>
        <xsl:copy-of select="$columnheader.acs"/>
        <xsl:copy-of select="$columnheader.lsm"/>
        <xsl:copy-of select="$columnheader.panel"/>
        <xsl:copy-of select="$columnheader.row"/>
        <xsl:copy-of select="$columnheader.col"/>
        <xsl:copy-of select="$columnheader.pool"/>
        <xsl:copy-of select="$columnheader.media"/>
        <xsl:copy-of select="$columnheader.type"/>
        <xsl:copy-of select="$columnheader.status"/>
      </tr>
      <!-- If there are no children, then display 'No items found' -->
      <xsl:call-template name="emptyTableCondition"><xsl:with-param name="colspan" select="10"/></xsl:call-template>
      <xsl:apply-templates select="volume">
        <xsl:sort select="volId"/>
      </xsl:apply-templates>
    </table>
  </xsl:template>   
  <xsl:template match="volume">
    <tr>
      <xsl:call-template name="evenRowAttrCondition"/>
      <td><xsl:value-of select='@name'/></td>
      <td><xsl:value-of select='@acsId'/></td>
      <td><xsl:value-of select='@lsmId'/></td>
      <td><xsl:value-of select='@panelId'/></td>
      <td><xsl:value-of select='@row'/></td>
      <td><xsl:value-of select='@col'/></td>
      <td><xsl:value-of select='@poolId'/></td>      
      <td><xsl:value-of select='@mtype'/></td>
      <td><xsl:value-of select='@volType'/></td>
      <td><xsl:value-of select='@status'/></td>
    </tr>
  </xsl:template>   
  <!-- ACSLS scratch pool summary -->
  <xsl:template match='scratchPools'>    
    <br />
    <br />
    <table class="Tbl"> 
      <caption class="TblTtlTxt">
        <xsl:value-of select="SamUtil:getResourceString('reports.acsls.status.scratchpools')"/>
      </caption>
      <tr>
        <xsl:copy-of select="$columnheader.pool"/>
        <xsl:copy-of select="$columnheader.lwm"/>
        <xsl:copy-of select="$columnheader.hwm"/>
        <xsl:copy-of select="$columnheader.overflow"/>
      </tr>
      <!-- If there are no children, then display 'No items found' -->
      <xsl:call-template name="emptyTableCondition"><xsl:with-param name="colspan" select="4"/></xsl:call-template>
      <xsl:apply-templates select="scratchPool"/>
    </table>
  </xsl:template>  
  <xsl:template match="scratchPool">
    <tr> 
      <xsl:call-template name="evenRowAttrCondition"/> 
      <td><xsl:value-of select='@poolId'/></td> 
      <td><xsl:value-of select='@lwm'/></td> 
      <td><xsl:value-of select='@hwm'/></td> 
      <td><xsl:value-of select='@overflow'/></td> 
    </tr>
  </xsl:template>                     
  <!-- ACSLS lock summary -->
  <xsl:template match="locks">
    <br />
    <br />
    <table class="Tbl"> 
      <caption class="TblTtlTxt">
        <xsl:value-of select="SamUtil:getResourceString('reports.acsls.status.locks')"/>
      </caption>
      <tr>
        <xsl:copy-of select="$columnheader.lock"/>
        <xsl:copy-of select="$columnheader.acs"/>
        <xsl:copy-of select="$columnheader.lsm"/>
        <xsl:copy-of select="$columnheader.panel"/>
        <xsl:copy-of select="$columnheader.drive"/> 
        <xsl:copy-of select="$columnheader.type"/>
        <xsl:copy-of select="$columnheader.duration"/>
        <xsl:copy-of select="$columnheader.pending"/>
        <xsl:copy-of select="$columnheader.user"/>
        <xsl:copy-of select="$columnheader.status"/>
      </tr>
      <!-- If there are no children, then display 'No items found' -->
      <xsl:call-template name="emptyTableCondition"><xsl:with-param name="colspan" select="10"/></xsl:call-template>
      <xsl:apply-templates select="lock"/>
    </table>
  </xsl:template>   
  <xsl:template match='lock'> 
    <tr>
      <xsl:call-template name="evenRowAttrCondition"/>
      <!-- identity is obtained from c-api, convert it to acs, lsm, panel etc. -->
      <xsl:variable name="idStr"><xsl:value-of select="@identify"/></xsl:variable>
      <td><xsl:value-of select='lockId'/></td>
      <td><xsl:value-of select="substring($idStr, 1, 1)"/></td> 
      <td><xsl:value-of select="substring($idStr, 3, 1)"/></td> 
      <td><xsl:value-of select="substring($idStr, 5, 1)"/></td> 
      <td><xsl:value-of select="substring($idStr, 7, 1)"/></td>
      <td><xsl:value-of select='@type'/></td>
      <td><xsl:value-of select='@duration'/></td>
      <td><xsl:value-of select='@pending'/></td>
      <td><xsl:value-of select='@userId'/></td>
      <td><xsl:value-of select='@status'/></td>
    </tr>
    </xsl:template>
</xsl:stylesheet>
