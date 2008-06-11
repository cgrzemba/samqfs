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
// ident	$Id: OneProtoFS.jsp,v 1.2 2008/06/11 21:16:18 kilemba Exp $
--%>

<%@page info="CommonTasks" language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc" %>

<jato:pagelet>
     <cc:helpinline>
        <cc:text name="ProtoFSHelpText"
                 bundleID="samBundle"/>
    </cc:helpinline>
    <cc:spacer name="spacer1" newline="true" height="30"/>
    <cc:image name="asteriskImg" defaultValue="/samqfsui/images/asterisk.gif"/>
    <cc:href name="addStorageNodeHref"
             bundleID="samBundle"
             onClick="return addStorageNode();">
        <cc:text name="addStorageNodeText"
                 bundleID="samBundle"
                 defaultValue="firsttime.storagenode.add"/>
    </cc:href>
    
    <cc:spacer name="spacer2" newline="true" height="30"/>
    <cc:image name="asteriskImg" defaultValue="/samqfsui/images/asterisk.gif"/>
    <cc:href name="createFSonMDSHref"
             bundleID="samBundle"
             onClick="return createFSOnMDS();">
        <cc:text name="createFSonMDSText"
                 bundleID="samBundle"
                 defaultValue="firsttime.fsonmds.create"/>
    </cc:href>
</jato:pagelet>
