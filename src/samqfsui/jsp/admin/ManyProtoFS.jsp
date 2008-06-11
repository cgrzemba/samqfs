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
// ident	$Id: ManyProtoFS.jsp,v 1.2 2008/06/11 21:16:18 kilemba Exp $
--%>

<%@page info="CommonTasks" language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc" %>

<jato:pagelet>
<div style="margin-top:10px;margin-bottom:10px;width:50%">
    <cc:helpinline>
        <cc:text name="ProtoFSHelpText"
                 bundleID="samBundle"
                 defaultValue="firsttime.manyprotofs.found"/>
    </cc:helpinline>
</div>

    <jato:tiledView name="ProtoFSTiledView">
        <cc:href name="ProtofsHref"
                 bundleID="samBundle"
                 onClick="return handleProtoFS(this);">
            <cc:text name="ProtofsText" bundleID="samBundle"/>
        </cc:href>
        <cc:spacer name="spacer1" newline="true" height="30"/>
    </jato:tiledView>
</jato:pagelet>
