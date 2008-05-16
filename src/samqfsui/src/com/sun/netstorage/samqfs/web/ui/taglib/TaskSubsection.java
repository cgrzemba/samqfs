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

// ident	$Id: TaskSubsection.java,v 1.4 2008/05/16 18:39:06 am143972 Exp $


package com.sun.netstorage.samqfs.web.ui.taglib;

public class TaskSubsection {
    private String _name;
    private String _onclick;
    private String _url;
    private String _target;
    private String _id;
    private boolean _containsHelp;
    private String _helpTitle;
    private String _helpContent;

    public TaskSubsection(String id) {
        _id = id;
    }

    public String getID() {
        return _id;
    }

    public void setName(String name) {
        _name = name;
    }

    public String getName() {
        return _name;
    }

    public void setURL(String url) {
        _url = url;
    }

    public String getURL() {
        return _url;
    }

    public void setTarget(String target) {
        _target = target;
    }

    public String getTarget() {
        return _target;
    }

    public void setOnClick(String onclick) {
        _onclick = onclick;
    }

    public String getOnClick() {
        return _onclick;
    }

    public boolean containsHelp() {
        return _containsHelp;
    }

    public void setHelp(String helpTitle, String helpContent) {
        _containsHelp = (helpTitle != null) || (helpContent != null);

        _helpTitle = helpTitle;
        _helpContent = helpContent;
    }

    public String getHelpTitle() {
        return _helpTitle;
    }

    public String getHelpContent() {
        return _helpContent;
    }
}
