<%--
/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at pkg/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at pkg/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

// ident    $Id: Overview.jsp,v 1.3 2008/05/16 19:39:24 am143972 Exp $
--%>

<html>
<head>
<title>Overview</title>
</head>
<link rel="stylesheet" 
      type="text/css" 
      href="/com_sun_web_ui/css/css_ns6up.css" />
<meta name="Copyright" 
      content="Copyright &copy; 2007 Sun Microsystems, Inc. All Rights Reserved. Use is subject to license terms." />
<link rel="shortcut icon" 
      href="/com_sun_web_ui/images/favicon/favicon.ico" 
      type="image/x-icon">
</link>
</head>

<body class="DefBdy">

<!-- masthead -->
<table title="" class="MstSecTbl" width="100%" cellspacing="0" cellpadding="0">
<tbody><tr><td><div class="MstDivSecTtl">
<img name="Overview.SecondaryMasthead.ProdNameImage" 
     src="/samqfsui/com_sun_web_ui/images/SecondaryProductName.png" 
     alt="File System Manager" height="40" width="183" />
</div></td></tr>
</tbody>
</table>

<div class="TtlTxtDiv">
<h1 class="TtlTxt">Overview</h1>
</div>

<div id="content" style="font-family:sans-serif;font-size:14px;margin:0px 5px 0px 10px">
<p>
The File System Manager lets you configure and monitor Sun StorageTek SAM-QFS. SAM is the Storage and Archive Manager and QFS is a high performance file system. Together, they provide an integrated system for transparently managing user data across multiple tiers of storage.
</p>

<p>
For an archiving QFS, SAM is enabled and the disks of the file system represent the primary tier of storage. All data access occurs in the primary tier of storage.
</p>

<p>
Based on user-configurable policies, SAM automatically creates archive copies of files to the archive tier which is made up of disk and tape archive volumes.
</p>

<p>
SAM maintains free space in the primary tier by releasing the data for files that have been archived. The releaser begins to release files when the percentage of space used in the file system reaches the high water mark. It will continue to release files until it reaches the low water mark.
</p>

<p>
If a user or application accesses a file that is currently not in the primary tier of storage, SAM automatically stages the file back in to the primary tier.
</p>

<p>
All of this activity is transparent to users of the system and is configured through policies.
</p>

<p>
Life Cycle of a File in SAM-QFS: 
</p>
<img src="/samqfsui/images/sam_lifecycle.jpg"/>
</div>

<div id="closebutton" style="text-ident:30px">
<table cellspacing="10" cellpadding="10">
<tr><td style="text-align:center">
<input name="close" type="submit" class="Btn2" value="Close" onclick="window.close();" onmouseover="javascript: if (this.disabled==0) this.className='Btn2Hov'" onmouseout="javascript: if (this.disabled==0) this.className='Btn2'" onblur="javascript: if (this.disabled==0) this.className='Btn2'" onfocus="javascript: if (this.disabled==0) this.className='Btn2Hov'" />
</td></tr>
</table>
</div>
</body>
</html>
