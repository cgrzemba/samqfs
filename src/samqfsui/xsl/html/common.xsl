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
<!-- $Id: common.xsl,v 1.7 2008/03/17 14:44:03 am143972 Exp $ -->

<xsl:stylesheet 
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:SamUtil="com.sun.netstorage.samqfs.web.util.SamUtil">

  <!-- every even row of a different color -->
  <xsl:template name="evenRowAttrCondition">
    <xsl:if test="position() mod 2 = 0">
      <xsl:attribute name="bgcolor">#E9EBEC</xsl:attribute>
    </xsl:if>
  </xsl:template>

  <!-- If there are no children, then display 'No items found' -->
  <xsl:template name="emptyTableCondition">
    <xsl:param name="colspan" select="1"/>
    <xsl:if test="count(*)=0">
      <tr>
        <td>
          <xsl:attribute name="colspan"><xsl:value-of select="$colspan"/></xsl:attribute>
          <xsl:copy-of select="$noitems"/>
        </td>
      </tr>
    </xsl:if>
  </xsl:template>
  
  <!-- If errors are encountered, display the error message -->
  <xsl:template name="errorCondition">
    <xsl:param name="colspan" select="1"/>
    <xsl:if test="error">
      <tr>
        <td>
          <xsl:attribute name="colspan"><xsl:value-of select="$colspan"/></xsl:attribute>
          <xsl:value-of select="error"/>
        </td>
      </tr>
    </xsl:if>
  </xsl:template>
  
  <!-- column header from resource bundle -->
  <xsl:variable name="columnheader.name">
    <th class="TblColHdr" align="left" scope="col" nowrap="nowrap"><span class="TblHdrTxt">
      <xsl:value-of select="SamUtil:getResourceString('common.columnheader.name')"/>
    </span></th>
  </xsl:variable>
  <xsl:variable name="columnheader.mounted">
    <th class="TblColHdr" align="left" scope="col" nowrap="nowrap"><span class="TblHdrTxt">
      <xsl:value-of select="SamUtil:getResourceString('common.columnheader.mounted')"/>
    </span></th>
  </xsl:variable>
  <xsl:variable name="columnheader.hwm">
    <th class="TblColHdr" align="left" scope="col" nowrap="nowrap"><span class="TblHdrTxt">
      <xsl:value-of select="SamUtil:getResourceString('common.columnheader.hwm')"/>
    </span></th>
  </xsl:variable>
  <xsl:variable name="columnheader.lwm">
    <th class="TblColHdr" align="left" scope="col" nowrap="nowrap"><span class="TblHdrTxt">
      <xsl:value-of select="SamUtil:getResourceString('common.columnheader.lwm')"/>
    </span></th>
  </xsl:variable>
  <xsl:variable name="columnheader.hwmExceed">
    <th class="TblColHdr" align="left" scope="col" nowrap="nowrap"><span class="TblHdrTxt">
      <xsl:value-of select="SamUtil:getResourceString('common.columnheader.hwmExceed')"/>
    </span></th>
  </xsl:variable>
  <xsl:variable name="columnheader.drive">
    <th class="TblColHdr" align="left" scope="col" nowrap="nowrap"><span class="TblHdrTxt">
      <xsl:value-of select="SamUtil:getResourceString('common.columnheader.drive')"/>
    </span></th>
  </xsl:variable>
  <xsl:variable name="columnheader.serial">
    <th class="TblColHdr" align="left" scope="col" nowrap="nowrap"><span class="TblHdrTxt">
      <xsl:value-of select="SamUtil:getResourceString('common.columnheader.serial')"/>
    </span></th>
  </xsl:variable> 
  <xsl:variable name="columnheader.vsn">
    <th class="TblColHdr" align="left" scope="col" nowrap="nowrap"><span class="TblHdrTxt">
      <xsl:value-of select="SamUtil:getResourceString('common.columnheader.vsn')"/>
    </span></th>
  </xsl:variable>
  <xsl:variable name="columnheader.acs">
    <th class="TblColHdr" align="left" scope="col" nowrap="nowrap"><span class="TblHdrTxt">
      <xsl:value-of select="SamUtil:getResourceString('common.columnheader.acs')"/>
    </span></th>
  </xsl:variable>
  <xsl:variable name="columnheader.lsm">
    <th class="TblColHdr" align="left" scope="col" nowrap="nowrap"><span class="TblHdrTxt">
      <xsl:value-of select="SamUtil:getResourceString('common.columnheader.lsm')"/>
    </span></th>
  </xsl:variable>
  <xsl:variable name="columnheader.panel">
    <th class="TblColHdr" align="left" scope="col" nowrap="nowrap"><span class="TblHdrTxt">
      <xsl:value-of select="SamUtil:getResourceString('common.columnheader.panel')"/>
    </span></th>
  </xsl:variable>
  <xsl:variable name="columnheader.row">
    <th class="TblColHdr" align="left" scope="col" nowrap="nowrap"><span class="TblHdrTxt">
      <xsl:value-of select="SamUtil:getResourceString('common.columnheader.row')"/>
    </span></th>
  </xsl:variable>
  <xsl:variable name="columnheader.col">
    <th class="TblColHdr" align="left" scope="col" nowrap="nowrap"><span class="TblHdrTxt">
      <xsl:value-of select="SamUtil:getResourceString('common.columnheader.col')"/>
    </span></th>
  </xsl:variable>
  <xsl:variable name="columnheader.pool">
    <th class="TblColHdr" align="left" scope="col" nowrap="nowrap"><span class="TblHdrTxt">
      <xsl:value-of select="SamUtil:getResourceString('common.columnheader.pool')"/>
    </span></th>
  </xsl:variable>
  <xsl:variable name="columnheader.media">
    <th class="TblColHdr" align="left" scope="col" nowrap="nowrap"><span class="TblHdrTxt">
      <xsl:value-of select="SamUtil:getResourceString('common.columnheader.media')"/>
    </span></th>
  </xsl:variable>
  <xsl:variable name="columnheader.type">
    <th class="TblColHdr" align="left" scope="col" nowrap="nowrap"><span class="TblHdrTxt">
      <xsl:value-of select="SamUtil:getResourceString('common.columnheader.type')"/>
    </span></th>
  </xsl:variable>
  <xsl:variable name="columnheader.status">
    <th class="TblColHdr" align="left" scope="col" nowrap="nowrap"><span class="TblHdrTxt">
      <xsl:value-of select="SamUtil:getResourceString('common.columnheader.status')"/>
    </span></th>
  </xsl:variable>
  <xsl:variable name="columnheader.overflow">
    <th class="TblColHdr" align="left" scope="col" nowrap="nowrap"><span class="TblHdrTxt">
      <xsl:value-of select="SamUtil:getResourceString('common.columnheader.overflow')"/>
    </span></th>
  </xsl:variable>
  <xsl:variable name="columnheader.duration">
    <th class="TblColHdr" align="left" scope="col" nowrap="nowrap"><span class="TblHdrTxt">
      <xsl:value-of select="SamUtil:getResourceString('common.columnheader.duration')"/>
    </span></th>
  </xsl:variable>
  <xsl:variable name="columnheader.pending">
    <th class="TblColHdr" align="left" scope="col" nowrap="nowrap"><span class="TblHdrTxt">
      <xsl:value-of select="SamUtil:getResourceString('common.columnheader.pending')"/>
    </span></th>
  </xsl:variable>
  <xsl:variable name="columnheader.user">
    <th class="TblColHdr" align="left" scope="col" nowrap="nowrap"><span class="TblHdrTxt">
      <xsl:value-of select="SamUtil:getResourceString('common.columnheader.user')"/>
    </span></th>
  </xsl:variable>
  <xsl:variable name="columnheader.lock">
    <th class="TblColHdr" align="left" scope="col" nowrap="nowrap"><span class="TblHdrTxt">
      <xsl:value-of select="SamUtil:getResourceString('common.columnheader.lock')"/>
    </span></th>
  </xsl:variable>
  <xsl:variable name="columnheader.count">
    <th class="TblColHdr" align="left" scope="col" nowrap="nowrap"><span class="TblHdrTxt">
      <xsl:value-of select="SamUtil:getResourceString('common.columnheader.count')"/>
    </span></th>
  </xsl:variable>
  <xsl:variable name="columnheader.free">
    <th class="TblColHdr" align="left" scope="col" nowrap="nowrap"><span class="TblHdrTxt">
      <xsl:value-of select="SamUtil:getResourceString('common.columnheader.free')"/>
    </span></th>
  </xsl:variable>
  <xsl:variable name="columnheader.used">
    <th class="TblColHdr" align="left" scope="col" nowrap="nowrap"><span class="TblHdrTxt">
      <xsl:value-of select="SamUtil:getResourceString('common.columnheader.used')"/>
    </span></th>
  </xsl:variable>
  <xsl:variable name="columnheader.total">
    <th class="TblColHdr" align="left" scope="col" nowrap="nowrap"><span class="TblHdrTxt">
      <xsl:value-of select="SamUtil:getResourceString('common.columnheader.total')"/>
    </span></th>
  </xsl:variable>
  <xsl:variable name="noitems">
    <xsl:value-of select="SamUtil:getResourceString('common.noitemsfound')"/>
  </xsl:variable> 
</xsl:stylesheet>
