/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

// ident    $Id: TasksSectionTag.java,v 1.3 2008/03/17 14:43:55 am143972 Exp $

package com.sun.netstorage.samqfs.web.ui.taglib;

import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.View;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.web.ui.taglib.common.CCTagBase;
import java.util.Iterator;
import javax.servlet.jsp.JspException;
import javax.servlet.jsp.PageContext;
import javax.servlet.jsp.tagext.Tag;

public class TasksSectionTag extends CCTagBase {
    private static final String STYLE_HEADER = "TskPgeSbHdr";
    private static final String STYLE_BOTTOM_SPACE = "TskPgeBtmSpc";
    private static final String STYLE_BACKGROUND_TD = "TskPgeBckgrTd";
    private static final String STYLE_LEFT_TD = "TskPgeTskLftTd";
    private static final String STYLE_CENTER_TD = "TskPgeTskCntrTd";
    private static final String STYLE_RIGHT_TD = "TskPgeTskRghtTd";
    private static final String STYLE_TEXT_BACKGROUND = "TskPgeTxtBg";
    private static final String STYLE_TEXT_BACKGROUND_OVER = "TskPgeTxtBgOvr";
    private static final String STYLE_LEFT_BTM = "TskPgeTskLftBtm";
    private static final String STYLE_LEFT_TP = "TskPgeTskLftTp";
    private static final String STYLE_RIGHT_BTM = "TskPgeTskRghtBtm";
    private static final String STYLE_RIGHT_TP = "TskPgeTskRghtTp";
    private static final String STYLE_RIGHT_BORDER = "TskPgeTskRghtBrdr";
    private static final String STYLE_SUBSECTION = "TskPgeTskPdng";
    private static final String STYLE_INFO_PANEL = "TskPgeInfPnl";
    private static final String STYLE_INFO_HEADER = "TskPgeHdr";
    private static final String STYLE_INFO_CONTENT = "TskPgeCnt";
    private static final String TOGGLE = "/samqfsui/images/right_toggle.gif";
    private static final String TOGGLE_WIDTH = "29";
    private static final String TOGGLE_HEIGHT = "21";
    private static final String
        TOGGLE_EMPTY = "/samqfsui/images/right_toggle_empty.gif";
    private static final String TOGGLE_EMPTY_WIDTH = "12";
    private static final String TOGGLE_EMPTY_HEIGHT = "21";
    private static final String SPACER = "/samqfsui/images/task_spacer.gif";
    private static final String CLOSE = "/samqfsui/images/close.gif";

    public TasksSectionTag() {
        super();
    }

    protected String getHTMLStringInternal(Tag parent,
                                           PageContext pageContext,
                                           View view) throws JspException {

        if (parent == null) {
            throw new IllegalArgumentException("parent cannot be null");
        } else if (pageContext == null) {
            throw new IllegalArgumentException("pageContext cannot be null");
        } else if (view == null) {
            throw new IllegalArgumentException("view cannot be null");
        }

        TasksSectionModel model = ((TasksSection)view).getTasksSectionModel();

        // See bugtraq ID 4994832.
        super.getHTMLStringInternal(parent, pageContext, view);

        // Initialize tag parameters.
        setParent(parent);
        setPageContext(pageContext);

        NonSyncStringBuffer buffer = new NonSyncStringBuffer(K);

        buffer.append("<span class=\""+STYLE_HEADER+"\">")
            .append(SamUtil.getResourceString(model.getName())+"</span>\n");

        Iterator it = model.getSubsections().iterator();
        while (it.hasNext()) {
            appendSubsection(buffer, (TaskSubsection)it.next());
        }

        it = model.getSubsections().iterator();
        while (it.hasNext()) {
            appendSubsectionHelpContent(buffer, (TaskSubsection)it.next());
        }

        return buffer.toString();
    }

    private void appendSubsection(NonSyncStringBuffer buffer,
                                  TaskSubsection subsection) {

        String theId = subsection.getID();

        buffer.append("<table width=\"100%\" border=\"0\" ")
                .append("cellspacing=\"0\" ")
        .append("cellpadding=\"0\" class=\""+STYLE_BOTTOM_SPACE+"\">\n")
        .append("<tbody><tr>\n")
        .append("<td class=\""+STYLE_BACKGROUND_TD+"\">")
        .append("<table width=\"100%\" border=\"0\" cellspacing=\"0\" ")
        .append("cellpadding=\"0\">\n")
        .append("<tbody><tr>\n")
        .append("<td width=\"2%\" valign=\"bottom\" class=\"")
        .append(STYLE_LEFT_TD + "\" > \n")
        .append("<img alt=\"\" id=\"gif"+theId+"\" src=\""+SPACER+"\" "+
                "width=\"12\" height=\"8\" /></td>\n")
        .append("<td width=\"100%\" class=\""+STYLE_CENTER_TD+"\">");

        String url = subsection.getURL();
        if ((url == null) || (url.length() == 0)) {
            url = "#";
        }

        buffer.append(
            "<a href=\""+url+"\" class=\""+STYLE_TEXT_BACKGROUND+"\" ");

        if (subsection.getTarget() != null) {
            buffer.append("target=\""+subsection.getTarget()+"\"");
        }

        if (subsection.getOnClick() != null) {
                buffer.append("onclick=\""+subsection.getOnClick()+"\"");
        }

        buffer.append(
            "onmouseover=\"this.className='"+
            STYLE_TEXT_BACKGROUND_OVER+"'\" onfocus=\"this.className='"+
            STYLE_TEXT_BACKGROUND_OVER+"'\" onmouseout=\"this.className='"+
            STYLE_TEXT_BACKGROUND+"'\" onblur=\"this.className='"+
            STYLE_TEXT_BACKGROUND+"'\"> <span class=\""+STYLE_LEFT_BTM+
            "\"></span><span class=\""+STYLE_LEFT_TP+"\"></span>"+
            "<span class=\""+STYLE_RIGHT_BTM+"\"></span>"+
            "<span class=\""+STYLE_RIGHT_TP+"\"></span> "+
            "<span class=\""+STYLE_RIGHT_BORDER+"\"></span>"+
            "<span class=\""+STYLE_SUBSECTION+"\">"+
            SamUtil.getResourceString(subsection.getName()) +
            "</span> </a></td>");

        String id = subsection.getID();

        if (subsection.containsHelp()) {
            String title = SamUtil
                .getResourceString("commontasks.clickToDisplayHelp");

            buffer.append("<td width=\"3%\" align=\"right\" ")
                .append("valign=\"top\" ")
        .append("class=\""+STYLE_RIGHT_TD+"\" ><a href=\"#\" ")
        .append("onclick=\"test("+id+"); ")
        .append("event.cancelBubble = true; ")
        .append("return false;\" onmouseover=\"hoverImg("+id+"); ")
        .append("event.cancelBubble = true;\" ")
        .append("onmouseout=\"outImg("+id+"); ")
        .append("event.cancelBubble = true;\" ")
        .append("onfocus=\"hoverImg("+id+"); ")
        .append("event.cancelBubble = true;\" ")
        .append("onblur=\"outImg("+id+"); ")
        .append("event.cancelBubble = true;\" ")
        .append("id=\"i"+id+"\"><img alt=\""+title+"\" id=\"togImg"+id+
                "\" src=\""+TOGGLE+"\" width=\""+TOGGLE_WIDTH+"\" height=\""+
                TOGGLE_HEIGHT+"\"  border=\"0\"  title=\""+
                title+"\"/></a></td>");

        } else {
            buffer.append("<td width=\"3%\" align=\"right\" ")
                .append("valign=\"top\" ")
        .append("class=\""+STYLE_RIGHT_TD+"\" >")
        .append("<img alt=\"\" id=\"Img"+id+"\" src=\""+
            TOGGLE_EMPTY+"\" width=\""+TOGGLE_EMPTY_WIDTH+
            "\" height=\""+TOGGLE_EMPTY_HEIGHT+
            "\"  border=\"0\" /></a></td>");
        }

        buffer.append("</tr>\n</tbody></table></td>\n</tr>\n</tbody></table>");
    }

    private void appendSubsectionHelpContent(NonSyncStringBuffer buffer,
                                             TaskSubsection subsection) {

        String id = subsection.getID();
        String title =
            SamUtil.getResourceString("commontasks.clickToCloseHelp");

        buffer.append("<div id=\"info"+id+"\" ")
        .append("onclick=\"showDiv("+id+"); ")
        .append("event.cancelBubble = true;\" class=\""+
                STYLE_INFO_PANEL+
                        "\"><div><a href=\"#\" id=\"close"+id+"\" "+
                        "onclick=\"closeAll("+id+
                        "); event.cancelBubble = true;"+
                        "return false;\">")
        .append("<img alt=\""+title+"\" title=\""+title+"\" src=\""+CLOSE)
        .append("\" border=\"0\" />")
        .append("</a></div>\n");

        if (subsection.getHelpTitle() != null) {
            buffer.append("<p> <span class=\"")
                .append(STYLE_INFO_HEADER)
                .append("\">")
                .append(SamUtil.getResourceString(subsection.getHelpTitle()))
                .append("<br/><br/></span>");
        }

        if (subsection.getHelpContent() != null) {
            buffer.append("<span class=\"")
                .append(STYLE_INFO_CONTENT)
                .append("\">")
                .append(SamUtil.getResourceString(subsection.getHelpContent()))
                .append("</span> </p>");
        }

        buffer.append("\n</div>");
    }
}
