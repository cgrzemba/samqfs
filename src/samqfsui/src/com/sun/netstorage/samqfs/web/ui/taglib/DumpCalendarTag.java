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
 * or https://illumos.org/license/CDDL.
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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

// ident	$Id: DumpCalendarTag.java,v 1.8 2008/12/16 00:12:25 am143972 Exp $

package com.sun.netstorage.samqfs.web.ui.taglib;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.JspDisplayEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.web.ui.common.CCImage;
import com.sun.web.ui.common.CCStyle;
import com.sun.web.ui.taglib.common.CCTagBase;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.GregorianCalendar;
import javax.servlet.jsp.JspException;
import javax.servlet.jsp.PageContext;
import javax.servlet.jsp.tagext.Tag;

public class DumpCalendarTag extends CCTagBase {

    /**
     * Tag attributes
     */

    // gap between two scheduler properties row
    protected static int rowsGap = 5;

    // gap between scheduler property and value
    protected int colsGap = 10;

    protected static final String
        STYLE_NOT_RETAINED = "color:#003399;background-color:#fff";
    protected static final String
        STYLE_RETAINED = "color:#FFFFFF;background-color:#0000FF";

    // private view members that multiple methods need access to
    protected DumpCalendar dumpCalendar = null;

    /**
     * Default constructor
     */
    public DumpCalendarTag() {
        super();
    }

    public void reset() {
        super.reset();

        // Reset all variables (ignore tag attributes).
        rowsGap = 5;
        colsGap = 10;
    }

    /**
     * Get HTML string.
     *
     * The code in this method is separated out from the getHTMLString method in
     * order to prevent reset() from being called by subclasses. Subclasses
     * should call
     * <code>super.getHTMLStringInternal(Tag, PageContext, View)</code> from
     * their getHTMLStringInternal method.
     *
     * @param parent The Tag instance enclosing this tag.
     * @param pageContext The page context used for this tag.
     * @param view The container view used for this tag. (use a null value)
     * @return The HTML string
     */
    protected String getHTMLStringInternal(Tag parent, PageContext pageContext,
	    View view) throws JspException {

        if (parent == null) {
            throw new IllegalArgumentException("parent cannot be null");
        } else if (pageContext == null) {
            throw new IllegalArgumentException("pageContext cannot be null");
        } else if (view == null) {
            throw new IllegalArgumentException("view cannot be null");
        }

        // See bugtraq ID 4994832.
        super.getHTMLStringInternal(parent, pageContext, view);

        // Save container view and model.
        checkChildType(view, DumpCalendar.class);
        dumpCalendar = (DumpCalendar) view;

        // Initialize tag parameters.
        setParent(parent);
        setPageContext(pageContext);

        // Set the state of the model and evoke the begin display
        // event for the view.
        try {
            dumpCalendar.beginDisplay(new JspDisplayEvent(this, pageContext));
        } catch (ModelControlException e) {
            throw new JspException(e.getMessage());
        }

        // Get start date.  The number of months depend on schema.
        // Number onf months depends on schema.
        int numMonths = dumpCalendar.getNumMonthsForSchema();
        GregorianCalendar date = new GregorianCalendar();
        date.add(Calendar.MONTH, -(numMonths - 1));

        StringBuffer buffer =
            new StringBuffer(DEFAULT_BUFFER_SIZE);

        // create 6-column table
        buffer.append(
            "\n<table border=\"0\" cellspacing=\"1\" cellpadding=\"1\">")
            .append("\n<tr>\n<td colspan=\"6\">")
            .append(getImageHTMLString(CCImage.DOT, 5, 1))
            .append("</td>\n</tr>");

        // create calendar for each month
        // Display 6 months per row
        buffer.append("\n<tr>");
        try {
            for (int i = 0; i < numMonths; i++) {
                buffer.append(getMonthlyHTML(date, dumpCalendar));
                date.add(Calendar.MONTH, 1);
                if ((i + 1) % 6 == 0) {
                    buffer.append("\n</tr>\n<tr>");
                }
            }
        } catch (SamFSException e) {
            buffer.append(SamUtil.getResourceString(
                                        "FSDumpDemo.error.cantDisplayDemo"))
                  .append(e.getMessage());
        }

        buffer.append("\n</tr>\n</table>");

        return buffer.toString();
    }

    protected String getMonthlyHTML(GregorianCalendar date,
                                    DumpCalendar dumpCalendar)
    throws SamFSException {

        StringBuffer buffer =
            new StringBuffer(DEFAULT_BUFFER_SIZE);

        // monthly header
        SimpleDateFormat monthFmt = new SimpleDateFormat("MMMMM yyyy");
        buffer.append("\n<td align=\"center\" valign=\"top\">")
            .append("<div class=\"")
            .append(CCStyle.DATE_TIME_SELECT_DIV)
            .append("\">\n<b>")
            .append(monthFmt.format(date.getTime()))
            .append("</b></div>");

        // monthly calendar
        buffer.append("\n<div class=\" ")
            .append(CCStyle.DATE_TIME_CALENDAR_DIV)
            .append("\" > \n")
            .append("<table class=\" ")
            .append(CCStyle.DATE_TIME_CALENDAR_TABLE)
            .append("\"")
            .append("width=\"100%\"")
            .append("cellspacing=\"1\" ")
            .append(" cellpadding=\"1\" border=\"0\">\n");

        // 2/13/2005 is a Sunday.  2/14/2005 is a Monday.
        // Get one as the start of the week.
        GregorianCalendar now = new GregorianCalendar();
        int dayOfWeek = now.get(Calendar.DAY_OF_WEEK);
        int firstDayOfWeek = now.getFirstDayOfWeek();
        if (dayOfWeek != firstDayOfWeek) {
            now.add(Calendar.DAY_OF_WEEK,  firstDayOfWeek - dayOfWeek);
        }

        // Day of week header
        SimpleDateFormat dayFmt = new SimpleDateFormat("E");
        buffer.append("<tr>\n");
        for (int i = 0; i < 7; i++) {
            buffer.append("<th align=\"center\" scope=\"col\"><span class=\"")
                .append(CCStyle.DATE_TIME_DAY_HEADER)
                .append("\" > ")
                .append(dayFmt.format(now.getTime()).charAt(0))
                .append("</span></th>\n");
            now.add(Calendar.DAY_OF_MONTH,  1);
        }
        buffer.append("</tr>\n");

        // append days here
        date.set(Calendar.DAY_OF_MONTH, 1);
        int maxDay = date.getActualMaximum(Calendar.DAY_OF_MONTH);
        int firstDay = date.getFirstDayOfWeek();
        int day = date.get(Calendar.DAY_OF_WEEK);
        int offset = day - firstDay;
        int dd = 0;

        while (dd < maxDay) {
            buffer.append("\n<tr>");
            for (int i = 0; i < 7; i++) {
                buffer.append("<td align=\"center\" style=\"");
                if (offset > 0 || dd >= maxDay) {
                    // Days before first of the month and after last of the
                    // month are shown white
                    buffer.append(STYLE_NOT_RETAINED).
                        append("\" > ").
                        append("</td>");
                    offset--;
                } else {
                    // Days on which snapshots are retained are colored.
                    // Non-retained days are white.
                    date.set(Calendar.DAY_OF_MONTH, dd + 1);
                    String style = "";
                    if (dumpCalendar.isSnapshotRetainedOnDay(
                        date,
                        new GregorianCalendar())) {
                        style = STYLE_RETAINED;
                    } else {
                        style = STYLE_NOT_RETAINED;
                    }

                    buffer.append(style)
                        .append("\" > ")
                        .append(dd + 1)
                        .append("</td>");
                    dd++;
                }
            }
            buffer.append("</tr>");
        }

        buffer.append("\n</table>\n</div>\n</td>");
        return buffer.toString();
    }

}
