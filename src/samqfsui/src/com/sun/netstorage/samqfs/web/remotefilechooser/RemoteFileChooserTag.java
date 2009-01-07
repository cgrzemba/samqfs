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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

// ident	$Id: RemoteFileChooserTag.java,v 1.16 2009/01/07 21:27:26 ronaldso Exp $

package com.sun.netstorage.samqfs.web.remotefilechooser;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.HtmlUtil;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.ContainerView;
import com.iplanet.jato.view.DisplayField;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.JspDisplayEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.common.CCAccessible;
import com.sun.web.ui.common.CCBodyContentImpl;
import com.sun.web.ui.common.CCI18N;
import com.sun.web.ui.common.CCImage;
import com.sun.web.ui.common.CCJspWriterImpl;
import com.sun.web.ui.common.CCStyle;
import com.sun.web.ui.model.CCFileChooserModelInterface;
import com.sun.web.ui.model.CCFileChooserTimeInterface;
import com.sun.web.ui.taglib.common.CCDisplayFieldTagBase;
import com.sun.web.ui.taglib.common.CCTagBase;
import com.sun.web.ui.taglib.html.CCButtonTag;
import com.sun.web.ui.taglib.html.CCDropDownMenuTag;
import com.sun.web.ui.taglib.html.CCHiddenTag;
import com.sun.web.ui.taglib.html.CCHrefTag;
import com.sun.web.ui.taglib.html.CCImageTag;
import com.sun.web.ui.taglib.html.CCLabelTag;
import com.sun.web.ui.taglib.html.CCSelectableListTag;
import com.sun.web.ui.taglib.html.CCTextFieldTag;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCOption;
import com.sun.web.ui.view.html.CCSelectableList;
import com.sun.web.ui.view.html.CCTextField;
import java.io.File;
import java.io.FileFilter;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.util.Vector;
import javax.servlet.jsp.JspException;
import javax.servlet.jsp.PageContext;
import javax.servlet.jsp.tagext.BodyContent;
import javax.servlet.jsp.tagext.Tag;

public class RemoteFileChooserTag extends CCTagBase {

    // Attribute type
    protected static final String ATTRIB_TYPE = "type";

    // Attribute allowMultipleSelect
    protected static final
        String ATTRIB_MULTIPLE_SELECT = "allowMultipleSelect";

    // CCFileChooser view.
    protected RemoteFileChooser fileChooser;

    // Current files
    protected File[] files;

    // the "-" string constant
    public static final String HYFEN = "-";

    // the "*" string constant
    public static final String ASTERIX = "*";

    // Index used to create unique element IDs.
    protected int elementIndex = 0;
    /**
     * Default constructor
     */
    public RemoteFileChooserTag() {
        super();
        TraceUtil.initTrace();
        TraceUtil.trace3("Ctor being called");
    }

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Tag handler methods
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    /**
     * Reset cached state to allow for a new rendering of this tag instance.
     * Note that this method only resets class data members; it does not reset
     * tag attribute values.
     */
    public void reset() {
        TraceUtil.trace3("Entering");
        super.reset();

        // Reset all variables (ignore tag attributes).
        fileChooser = null;
        files = null;
        elementIndex = 0;
    }

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Micro component methods
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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

        TraceUtil.trace3("Entering");

        if (parent == null) {
            throw new IllegalArgumentException("parent cannot be null");
        } else if (pageContext == null) {
            throw new IllegalArgumentException("pageContext cannot be null");
        } else if (view == null) {
            throw new IllegalArgumentException("view cannot be null");
        }

        // See bugtraq ID 4994832.
        super.getHTMLStringInternal(parent, pageContext, view);

        checkChildType(view, RemoteFileChooser.class);
        fileChooser = (RemoteFileChooser) view;
        RemoteFileChooserModel model = (RemoteFileChooserModel)
            fileChooser.getModel();
        if (model == null) {
            TraceUtil.trace1("Model is null.");
            throw new IllegalArgumentException();
        }

        // Initialize tag parameters.
        setParent(parent);
        setPageContext(pageContext);
        setAttributes();

        // Evoke the begin display event for the view
        // Set state by evoking the beginDisplay() method of the view.

        try {
            fileChooser.beginDisplay(new JspDisplayEvent(this, pageContext));
        } catch (ModelControlException e) {
            throw new JspException(e.getMessage());
        }

        NonSyncStringBuffer buffer =
            new NonSyncStringBuffer(DEFAULT_BUFFER_SIZE);

        appendStartingHTML(buffer);
        appendServerRow(buffer, model);
        appendHelpMsg1(buffer);
        appendLookinTextField(buffer, model);
        appendFilterField(buffer);
        appendSortMenus(buffer, model);
        appendUpLevelButton(buffer, model);
        appendOpenFolderButton(buffer);
        appendFilesList(buffer, model);
        appendItemsString(buffer, model);
        fileChooser.initPaginationControls();
        appendPaginationControls(buffer, model);
        appendSelectedResource(buffer, model);

        return buffer.toString();
    }

    /*
     * Generate the starting HTML that contains the label etc.
     */
    protected void appendStartingHTML(NonSyncStringBuffer buffer)
        throws JspException {

        TraceUtil.trace3("Entering");

        String title = HtmlUtil.escape(getTagMessage("filechooser.title"));
        String summary = HtmlUtil.escape(getTagMessage("filechooser.summary"));
        buffer.append("<div class=\"")
            .append(CCStyle.FILECHOOSER_CONMGN)
            .append("\" > \n")
            .append("<table title=\"\" border=\"0\" cellspacing=\"0\"")
            .append(" cellpadding=\"0\"")
            .append(" summary=\"" + summary + "\">\n");
    }

    /*
     * Generate the HTML to create server prompt
     */
    protected void appendServerRow(NonSyncStringBuffer buffer,
        RemoteFileChooserModel model) throws JspException {

        TraceUtil.trace3("Entering");

        // add the hidden tag that contains the file name
        CCHiddenTag hiddenTag = new CCHiddenTag();
        String fileNameHtml = hiddenTag.getHTMLString(getParent(), pageContext,
            fileChooser.getChild(RemoteFileChooser.ENTER_FLAG));

        String value =
            HtmlUtil.escape(getTagMessage("filechooser.serverPrompt"));
        buffer.append("<tr>\n")
            .append("<td>")
            .append(getImageHTMLString(CCImage.DOT, 20, 1))
            .append("</td>\n </tr>\n")
            .append("<tr>\n<td><span class=\"")
            .append(CCStyle.FILECHOOSER_LABEL_TXT)
            .append("\" > ")
            .append(value)
            .append("</span>")
            .append("&nbsp;&nbsp;")
            .append("<span class=\"")
            .append(CCStyle.FILECHOOSER_NAME_TXT)
            .append("\" > ")
            .append(HtmlUtil.escape(model.getServerName()))
            .append("</span>")
            .append(fileNameHtml)
            .append("</td></tr>\n")
            .append(appendEmptyLine(null, "1", "10"));
    }

    /*
     * Generate the HTML to create a lookinText field
     */
    protected void appendLookinTextField(NonSyncStringBuffer buffer,
        RemoteFileChooserModel model) throws JspException {

        TraceUtil.trace3("Entering");

        CCTextFieldTag lookInTextTag = new CCTextFieldTag();
        CCTextField lookInText = (CCTextField)
            fileChooser.getChild(RemoteFileChooser.LOOK_IN_TEXTFIELD);
        String textData = model.getCurrentDirectory();
        TraceUtil.trace3("textData = " + textData);

        if (textData == null) {
            textData = model.getHomeDirectory();
        }
        TraceUtil.trace3("textData = " + textData);
        lookInText.setValue(textData);

        lookInTextTag.setSize("40");
        lookInTextTag.setExtraHtml("style=\"width:422px\"");
        lookInTextTag.setTabIndex(getTabIndex());

        CCHref href = new CCHref((ContainerView) lookInText.getParent(),
            RemoteFileChooser.LOOK_IN_COMMAND_HREF, null);

        // Get Javascript command to set action URL.
        String actionURLJavascript = getActionURLJavascript(
            (ContainerView) lookInText.getParent(),
            RemoteFileChooser.LOOK_IN_COMMAND_HREF);

        // append javascript event handler so that the form can be
        // submitted when the user enters data in this text field
        // and hits the enter button

        getTextFieldScript(actionURLJavascript, lookInTextTag,
            RemoteFileChooser.ENTER_KEY_PRESSED);

        buffer.append("\n<tr>\n<td> <table title=\"\" ")
            .append("border=\"0\" cellpadding=\"0\"")
            .append("cellspacing=\"0\">")
            .append(appendEmptyLine("3", "1", "3"))
            .append("<tr>\n<td nowrap=\"nowrap\">")
            .append(getLabel(lookInText.getParent(), lookInText.getName(),
                "filechooser.lookin"))
            .append("</td>")
            .append(getDotImage(null, "10", "1"))
            .append("<td>")
            .append(lookInTextTag.getHTMLString(
                getParent(), pageContext, lookInText))
            .append("</td>\n</tr>\n");
    }

    /*
     * Generate the HTML to create a filter field
     */
    protected void appendFilterField(NonSyncStringBuffer buffer)
        throws JspException {

        TraceUtil.trace3("Entering");

        CCTextFieldTag filterTextTag = new CCTextFieldTag();
        CCTextField filterText = (CCTextField)
            fileChooser.getChild(RemoteFileChooser.FILE_TYPE_FILTER);

        filterText.setValue(ASTERIX, false);
        int size = 18;
        if (isNav4()) {
            size = 14;
        } else if (isNav6up()) {
            size = 32;
        }

        filterTextTag.setSize(Integer.toString(size));
        filterTextTag.setTabIndex(getTabIndex());
        CCHref href = new CCHref((ContainerView) filterText.getParent(),
            RemoteFileChooser.LOOK_IN_COMMAND_HREF, null);

        // Get Javascript command to set action URL.
        String actionURLJavascript = getActionURLJavascript(
            (ContainerView) filterText.getParent(),
            RemoteFileChooser.LOOK_IN_COMMAND_HREF);

        // append javascript event handler so that the form can be
        // submitted when the user enters data in this text field
        // and hits the enter button

        getTextFieldScript(actionURLJavascript, filterTextTag,
            RemoteFileChooser.ENTER_KEY_PRESSED);

        buffer.append(appendEmptyLine("3", "1", "5"))
            .append("<tr>\n<td nowrap=\"nowrap\">")
            .append(getLabel(filterText.getParent(), filterText.getName(),
                "filechooser.filterOn"))
            .append("</td>")
            .append(getDotImage(null, "10", "1"))
            .append("<td>")
            .append(filterTextTag.getHTMLString(
                getParent(), pageContext, filterText))
            .append("</td>\n</tr>\n</table></td>\n</tr>");
    }

    /*
     * Generate the javascript to submit the form when the user enters
     * data in the "Look In" text field or the "Filter On" text field.
     */
    protected void getTextFieldScript(String actionURLJavascript,
        CCDisplayFieldTagBase tag, String enterType) {

        // set a hidden field to indicate that the enter key has been
        // pressed.

        CCHiddenField flag = (CCHiddenField)
            fileChooser.getChild(RemoteFileChooser.ENTER_FLAG);
        String flagName = flag.getQualifiedName();
        NonSyncStringBuffer jsBuffer =
            new NonSyncStringBuffer(DEFAULT_BUFFER_SIZE);

        jsBuffer.append("var enter = elements['")
            .append(flagName)
            .append("'];")
            .append("enter.value='")
            .append(enterType)
            .append("'; ");

        NonSyncStringBuffer scriptBuffer =
            new NonSyncStringBuffer(DEFAULT_BUFFER_SIZE);

        String javaScript = "javascript:";
        actionURLJavascript =
            actionURLJavascript.substring(javaScript.length());

        scriptBuffer.append("javascript: var keycode = -1; ");

        // Different browsers handle events quite differently.
        // ON NS4.7 its the onChange event that we need to capture
        // whereas on other browsers we need to capture the onKeyPress
        // event.

        // Netscape 4.7x on Windows and Unix
        if (isNav4()) {
            scriptBuffer.append("if (event) keycode = event.which;")
                .append("if (keycode != 0) return true; else {")
                .append(jsBuffer.toString() + actionURLJavascript)
                .append("}");
            tag.setOnChange(scriptBuffer.toString());

        // IE 5 and higher
        } else if (isIe5up()) {
            scriptBuffer.append("if (window.event) keycode = ")
                .append("window.event.keyCode;")
                .append("else if (event) keycode = event.which; ")
                .append("if (keycode != 13) return true; else {")
                .append(jsBuffer.toString())
                .append(actionURLJavascript)
                .append(" return false;}");
            tag.setOnKeyPress(scriptBuffer.toString());

        // Netscape 6 and higher
        } else {
            scriptBuffer.append("if (window.event) keycode = ")
                .append("window.event.keyCode;")
                .append("else if (event) keycode = event.which;")
                .append("if (keycode != 13) return true; else {")
                .append(jsBuffer.toString() + actionURLJavascript)
                .append("}");
            tag.setOnKeyPress(scriptBuffer.toString());
        }
    }

    /*
     * Generate the HTML to display the sort drop down box
     */

    protected void appendSortMenus(NonSyncStringBuffer buffer,
        RemoteFileChooserModel model) throws JspException {

        TraceUtil.trace3("Entering");

        CCDropDownMenuTag sortFieldTag = new CCDropDownMenuTag();
        sortFieldTag.setBundleID(CCI18N.TAGS_BUNDLE_ID);
        sortFieldTag.setTabIndex(getTabIndex());

        // Adding code so that the Listbox selected index is set to
        // -1 (none selected) when the user sorts the list. Otherwise
        // the old index remains when the page realoads but the "Open
        // Folder" button is inactive. Since the onChange event cannot
        // invoked when the page is reloaded this is being done as an
        // alternative to maintain consistency.

        CCSelectableList fileList = (CCSelectableList)
            fileChooser.getChild(RemoteFileChooser.FILE_LIST);
        String fileListName = fileList.getQualifiedName();
        NonSyncStringBuffer jsBuffer =
            new NonSyncStringBuffer(DEFAULT_BUFFER_SIZE);
        jsBuffer.append("var select = elements['")
            .append(fileListName)
            .append("'];\n")
            .append("select.selectedIndex = -1; \n");
        sortFieldTag.setOnChange(jsBuffer.toString());

        CCDropDownMenu sortFieldView = (CCDropDownMenu)
            fileChooser.getChild(RemoteFileChooser.SORT_MENU);

        CCHref href = new CCHref((ContainerView) sortFieldView.getParent(),
            RemoteFileChooser.TIME_TO_SORT_HREF, null);

        buffer.append("<tr>\n")
            .append("<td> <table title=\"\" width=\"100%\" border=\"0\"")
            .append(" cellpadding=\"0\" cellspacing=\"0\">")
            .append(appendEmptyLine("3", "1", "10"))
            .append("<tr>\n");

        buffer.append("<td align=\"left\">")
            .append(getLabel(sortFieldView.getParent(), sortFieldView.getName(),
                "filechooser.sortBy"))
            .append("&nbsp;&nbsp;")
            .append(sortFieldTag.getHTMLString(
                getParent(), pageContext, sortFieldView))
            .append("</td>");
    }

    /**
     * Append the "Up One Level" HTML button.
     */
    protected void appendUpLevelButton(NonSyncStringBuffer buffer,
        RemoteFileChooserModel model) throws JspException {

        TraceUtil.trace3("Entering");

        // Get the html string for "Open Folder" mini button

        CCTextField lookInText = (CCTextField)
            fileChooser.getChild(RemoteFileChooser.LOOK_IN_TEXTFIELD);
        String lookInTextName = lookInText.getQualifiedName();

        View child = fileChooser.getChild(RemoteFileChooser.MOVE_UP_BUTTON);
        checkChildType(child, CCButton.class);
        CCButton button = (CCButton) child;

        CCButtonTag buttonTag = new CCButtonTag();

        // Set micro component values.
        buttonTag.setBundleID(CCI18N.TAGS_BUNDLE_ID);
        buttonTag.setType(CCButton.TYPE_SECONDARY_MINI);
        buttonTag.setTabIndex(getTabIndex());
        String formName = getFormName();

        button.setAlt("filechooser.upOneLevelTitle");
        button.setTitle("filechooser.upOneLevelTitle");
        buffer.append(getDotImage(null, "10", "1"))
            .append("<td align=\"right\" nowrap=\"nowrap\">")
            .append(buttonTag.getHTMLString(getParent(), pageContext, button))
            .append(" ");
    }

    protected void appendOpenFolderButton(NonSyncStringBuffer buffer)
        throws JspException {

        TraceUtil.trace3("Entering");

        // Get the html string for "Open Folder" mini button

        View child1 =
            fileChooser.getChild(RemoteFileChooser.OPEN_FOLDER_BUTTON);
        checkChildType(child1, CCButton.class);
        CCButton button1 = (CCButton) child1;

        CCButtonTag buttonTag = new CCButtonTag();
        buttonTag.setTabIndex(getTabIndex());

        // Set micro component values.
        buttonTag.setBundleID(CCI18N.TAGS_BUNDLE_ID);
        buttonTag.setType(CCButton.TYPE_SECONDARY_MINI);
        buttonTag.setDynamic("true");
        button1.setDisabled(true);

        button1.setAlt("filechooser.openFolderTitle");
        button1.setTitle("filechooser.openFolderTitle");
        button1.setTitleDisabled("filechooser.openFolderTitle");

        buffer.append(
        buttonTag.getHTMLString(getParent(), pageContext, button1))
            .append("</td>\n</tr>\n</table>\n</td>\n</tr>")
            .append(appendEmptyLine(null, "1", "5"));
    }

    /*
     * Generate the HTML to display file list
     */

    protected void appendFilesList(NonSyncStringBuffer buffer,
        RemoteFileChooserModel model) throws JspException {

        TraceUtil.trace3("Entering");

        // Get the html string for file list
        CCSelectableListTag selectTag = new CCSelectableListTag();
        selectTag.setSize(String.valueOf(model.getFileListBoxHeight()));
        String titleKey = null;
        if (model.getType().equals(CCFileChooserModelInterface.FILE_CHOOSER)) {
            titleKey = "filechooser.listTitleFile";
        } else {
            titleKey = "filechooser.listTitleFolder";
        }
        selectTag.setTitle(getTagMessage(titleKey));
        selectTag.setDisableStyleOnly("true");
        CCSelectableList fileList = (CCSelectableList)
            fileChooser.getChild(RemoteFileChooser.FILE_LIST);

        CCHref href = new CCHref((ContainerView) fileList.getParent(),
            RemoteFileChooser.DB_CLICK_COMMAND_HREF, null);

        // Add a HREF command child to handle the event where a
        // user presses the enter key when the focus is on the
        // Listbox.

        CCHref enterHref = new CCHref((ContainerView) fileList.getParent(),
            RemoteFileChooser.ENTER_COMMAND_HREF, null);

        // Get Javascript command to set action URL when user hits enter.
        String actionURLJavascript = getActionURLJavascript(
            (ContainerView) fileList.getParent(),
            RemoteFileChooser.ENTER_COMMAND_HREF);

        // append javascript event handler so that the form can be
        // submitted when the user hits enter

        getTextFieldScript(actionURLJavascript, selectTag,
            RemoteFileChooser.ENTER_KEY_PRESSED_INLISTBOX);

        // create the StringBuffer to be passed to generate the
        // disable javascript when DBL clicking a file

        NonSyncStringBuffer dblClickDisableScript =
            new NonSyncStringBuffer(DEFAULT_BUFFER_SIZE);
        dblClickDisableScript.append("ondblclick=\"");

        // Get Javascript command to set action URL when the user DBL clicks.
        String dblClickJavascript = getActionURLJavascript(
            (ContainerView) fileList.getParent(),
            RemoteFileChooser.DB_CLICK_COMMAND_HREF);

        // append the onChange event handler
        String onChangeJavascript = setFileListContents(dblClickDisableScript);
        if (onChangeJavascript != null) {
            selectTag.setOnChange(onChangeJavascript);
        }

        dblClickDisableScript.append(dblClickJavascript).append("\"");
        // append the dblClick event handler
        fileList.setExtraHtml(dblClickDisableScript.toString());

        selectTag.setMonospace("true");
        selectTag.setEscape("false");

        buffer.append("\n<tr>\n<td>")
            .append(selectTag.getHTMLString(getParent(), pageContext, fileList))
            .append("</td>\n</tr>")
            .append(appendEmptyLine(null, "1", "5"));
    }

    // Generate the HTML to display the items string.
    // For example, "25 Items" or "25 - 50 of 1000"
    // indicating total or paginated rows.
    protected void appendItemsString(NonSyncStringBuffer buffer,
        RemoteFileChooserModel model) throws JspException {

        int rows = model.getTotalItems();
        int pageSize = model.getPageSize();

        buffer.append("<tr>\n")
            .append("<td> <table title=\"\" width=\"100%\" border=\"0\"")
            .append(" cellpadding=\"0\" cellspacing=\"0\">")
            .append("\n<tr>");

        buffer.append("<td align=\"left\" nowrap=\"nowrap\"><span class=\"")
            .append(CCStyle.FILECHOOSER_LABEL_TXT)
            .append("\" > ");

        // Append title augment.
        if (rows > pageSize) {
            int currentPage = model.getCurrentPage();
            int firstRow = currentPage * pageSize + 1;
            int lastRow  = (currentPage + 1) * pageSize;
            if (lastRow > rows) {
                lastRow = rows;
            }
            buffer.append(SamUtil.getResourceString(
                "browser.paginatedItems", new String[] {
                    Integer.toString(firstRow),
                    Integer.toString(lastRow),
                    Integer.toString(rows)}));
        } else {
            buffer.append(SamUtil.getResourceString(
                "browser.items", new String[] {
                    Integer.toString(rows)}));
        }
        buffer.append("</span></td>");
    }

    // Append pagination controls.
    protected void appendPaginationControls(NonSyncStringBuffer buffer,
        RemoteFileChooserModel model) throws JspException {

        TraceUtil.trace3("Entering");

        RemoteFileChooser chooser = (RemoteFileChooser) fileChooser;

        // Get micro components.
        View child = fileChooser.getChild(
            RemoteFileChooser.CHILD_PAGINATION_PAGE_TEXTFIELD);
        CCTextFieldTag textFieldTag = new CCTextFieldTag();

        // Set micro component values.
        textFieldTag.setSize("3");
        textFieldTag.setAutoSubmit("false");
        textFieldTag.setTabIndex(getTabIndex());

        buffer.append("<td align=\"right\" nowrap=\"nowrap\">");

        // Append pagination controls.
        buffer.append(getIconHTMLString(
                RemoteFileChooser.CHILD_PAGINATION_FIRST_HREF,
                RemoteFileChooser.CHILD_PAGINATION_FIRST_IMAGE))
            .append(getIconHTMLString(
                RemoteFileChooser.CHILD_PAGINATION_PREV_HREF,
                RemoteFileChooser.CHILD_PAGINATION_PREV_IMAGE))
            .append(getDotImageHtmlString(1, 5))
            .append("<span")
            .append(" class=\"")
            .append(CCStyle.TABLE_PAGINATION_TEXT)
            .append("\"")
            .append(">")
            .append(SamUtil.getResourceString("browser.page"))
            .append("</span>")
            .append("&nbsp;\n")
            .append(textFieldTag.getHTMLString(getParent(), pageContext, child))
            .append("\n")
            .append("<span")
            .append(" class=\"")
            .append(CCStyle.FILECHOOSER_LABEL_TXT)
            .append("\"")
            .append(">")
            .append(getTagMessage("table.paginationRows",
                new String[] {Integer.toString(model.getTotalPages())}))
            .append("</span>")
            .append(getButtonHTMLString(
                RemoteFileChooser.CHILD_PAGINATION_GO_BUTTON))
            .append(getDotImageHtmlString(1, 5))
            .append(getIconHTMLString(
                RemoteFileChooser.CHILD_PAGINATION_NEXT_HREF,
                RemoteFileChooser.CHILD_PAGINATION_NEXT_IMAGE))
            .append(getIconHTMLString(
                RemoteFileChooser.CHILD_PAGINATION_LAST_HREF,
                RemoteFileChooser.CHILD_PAGINATION_LAST_IMAGE))
            .append("</td>\n</tr>\n</table>\n</td>\n</tr>")
            .append(appendEmptyLine(null, "1", "10"));
    }

    /**
     * This method appends the selected file/folder text field in the file
     * chooser component. It must satisfy the following UI properties:
     * If a file is selected in the list, display the name of the file
     * in the Selected File text field. If multiple files are selected,
     * display all of the files names separated by commas. Do not display
     * folder names in the Selected File text field. Replace existing text
     * in the Selected File text field only when the user selects a new
     * file name from the list or types a new file name. No other action
     * should replace existing text in the Selected File text field.
     *
     * Allow the user to specify a new path in the Selected File text
     * field. After the user enters the new path and hits enter, update
     * the list and the Look In text field to reflect the new path. If a
     * user types a path that begins with '/' or '\', take that as the
     * full path name. Otherwise, append what the user types to the path
     * in the Look In text field.
     *
     * Allow the user to specify a file name in the Selected File text
     * field by typing a full path, or without a path if the file is
     * contained in the current Look In folder. Once the user types in a
     * valid file name and presses the Enter key, the window should react
     * appropriately: pop-up windows should close and return the file name
     * to the application while inline file choosers should return the file
     * name, clear the Selected file text field, and either close or await
     * additional user input at the application designer's discretion.
     * If the file chooser is in a pop-up window, set the keyboard focus
     * inside the Selected File text field when the pop-up window
     * initially comes up.
     *
     * The create folder button will create a directory with the name provided.
     * It does not create anything if the value in selected resource is a
     * path.
     */
    protected void appendSelectedResource(NonSyncStringBuffer buffer,
        RemoteFileChooserModel model) throws JspException {

        TraceUtil.trace3("Entering");

        // Text tag
        CCTextFieldTag selectFileTextTag = new CCTextFieldTag();
        CCTextField selectFileText = (CCTextField)
            fileChooser.getChild(RemoteFileChooser.FILE_NAME_TEXT);
        String styleHtml = null;
        String labelKey = null;
        if (model.getType().equals(
            RemoteFileChooserModel.FILE_AND_FOLDER_CHOOSER)) {
            labelKey = SamUtil.getResourceString("browser.selectedFolder");
            styleHtml = "style=\"width:386px\"";
            // Put name of selected file in text field (single select only)
            if (!model.multipleSelect() &&
                model.getSelectedFiles() != null &&
                model.getSelectedFiles().length > 0) {
                File file = new File(model.getSelectedFiles()[0]);
                String name = file.getName();
                boolean isFile = false;
                try {
                    isFile = model.isFile(model.getSelectedFiles()[0]);
                } catch (SamFSException e) {
                }
                if (name != null && isFile) {
                    selectFileText.setValue(name);
                }
            }
        } else if (model.getType().equals(
            CCFileChooserModelInterface.FILE_CHOOSER)) {

            labelKey = SamUtil.getResourceString("browser.selectedFile");
            styleHtml = "style=\"width:390px\"";

            // Put name of selected file in text field (single select only)
            if (!model.multipleSelect() &&
                 model.getSelectedFiles() != null &&
                 model.getSelectedFiles().length > 0) {
                File file = new File(model.getSelectedFiles()[0]);
                String name = file.getName();
                if (name != null) {
                    selectFileText.setValue(name);
                }
            }
        } else {
            labelKey = SamUtil.getResourceString("browser.selectedFolder");
            styleHtml = "style=\"width:386px\"";
        }
        if (model.multipleSelect()) {
            selectFileTextTag.setSize("40");
        } else {
            styleHtml = null;
            selectFileTextTag.setSize("30");
        }
        if (styleHtml != null) {
            selectFileTextTag.setExtraHtml(styleHtml);
        }
        selectFileTextTag.setTabIndex(getTabIndex());

        CCHref href = new CCHref((ContainerView) selectFileText.getParent(),
            RemoteFileChooser.SELECTED_FILE_COMMAND_HREF, null);

        // Get Javascript command to set action URL.
        String actionURLJavascript = getActionURLJavascript(
            (ContainerView) selectFileText.getParent(),
            RemoteFileChooser.SELECTED_FILE_COMMAND_HREF);

        // append javascript event handler so that the form can be
        // submitted when the user enters data in this text field
        // and hits the enter button

        getTextFieldScript(actionURLJavascript, selectFileTextTag,
            RemoteFileChooser.SELECTED_FILE_COMMAND_HREF);


        buffer.append("\n<tr>\n<td> <table title=\"\" ")
            .append("border=\"0\" cellpadding=\"0\"")
            .append("cellspacing=\"0\">")
            .append("<tr>\n<td nowrap=\"nowrap\">")
            .append(getLabel(
                selectFileText.getParent(), selectFileText.getName(), labelKey))
            .append("</td>")
            .append(getDotImage(null, "10", "1"))
            .append("<td>")
            .append(selectFileTextTag.getHTMLString(getParent(),
                pageContext, selectFileText));
        if (model.getType().equals(
            CCFileChooserModelInterface.FOLDER_CHOOSER) ||
            model.getType().equals(
            RemoteFileChooserModel.FILE_AND_FOLDER_CHOOSER)) {
            buffer.append("</td>")
                .append(getDotImage(null, "10", "1"))
                .append("<td>")
                .append(getButtonHTMLString(
                                    RemoteFileChooser.CHILD_CREATE_FOLDER));
        }
        buffer.append("</td>\n</tr>\n</table>")
            .append("</td>\n</tr>\n")
            .append(appendEmptyLine(null, "1", "30"))
            .append("</table>\n</div>\n");
    }

    /**
     * This method appends a help message in small font to
     * the filechooser layout.
     */
    protected void appendHelpMsg1(NonSyncStringBuffer buffer)
        throws JspException {

        TraceUtil.trace3("Entering");

        // Append alert icon html.

        String helpMsg =
            HtmlUtil.escape(getTagMessage("filechooser.enterKeyHelp"));
        buffer.append("<tr><td>")
            .append("<span class=\"")
            .append(CCStyle.HELP_FIELD_TEXT)
            .append("\" > ")
            .append(helpMsg)
            .append("</span>")
            .append("</td></tr>\n");
    }


    /**
     * Return the HTML equivalent of a single table row of empty space.
     */
    protected String appendEmptyLine(String colSpan, String wd, String ht) {

        StringBuffer emptyLine = new StringBuffer();
        emptyLine.append("<tr>\n")
            .append(getDotImage(colSpan, wd, ht))
            .append("</tr>\n");

        return emptyLine.toString();
    }

    /**
     * Return the HTML equivalent of a single table data of empty space.
     */
    protected String getDotImage(String colSpan, String wd, String ht) {

        StringBuffer dotImageBuffer = new StringBuffer();

        if (colSpan != null) {
            dotImageBuffer.append("<td colspan=\"")
                .append(colSpan)
                .append("\" > ");
        } else {
            dotImageBuffer.append("<td>");
        }
        dotImageBuffer
            .append(getImageHTMLString(CCImage.DOT, ht, wd))
            .append("</td>\n");
        return dotImageBuffer.toString();
    }

    /**
     * Return the HTML equivalent of one blank space.
     * If the browser is NS4.7x the "." is used to create spacing
     * in the file selection list.
     */
    protected String getSpace() {

        String space = null;
        if (isNav4()) {
            space = ".";
        } else {
            space = "&nbsp;";
        }

        return space;
    }

    /**
     * This method returns the HTML string containing the selectable list
     * of files/folders in the specified format (name + space + size +
     * lastModified) as well as the Javascript to handle the onchange event
     * associated with this listbox. The list of files/folders is first
     * created by creating an optionlist of files and setting this in the
     * CCSelectableList. Once this is done the createJavaScriptForFileList()
     * method is called to generate the javascript to handle the onchange
     * event.
     */
    protected String setFileListContents(NonSyncStringBuffer dblClickBuffer) {

        TraceUtil.trace3("Entering");

        int fileNameLen = 40;
        int fileSizeLen = 10;
        int fileDateLen = 8;

        String space = getSpace();

        Vector selectableValuesVec = new Vector();
        Vector selectableNamesVec = new Vector();
        Vector fileOrFolderVec = new Vector();
        RemoteFileChooserModel model =
            (RemoteFileChooserModel) fileChooser.getModel();
        String type = model.getType();
        FileFilter filter = null;

        CCTextField filterText = (CCTextField)
            fileChooser.getChild(RemoteFileChooser.FILE_TYPE_FILTER);

        String filterString = (String) filterText.getValue();
        filter = (FileFilter) model.instantiateFilter(filterString);

        CCTextField lookInText = (CCTextField)
            fileChooser.getChild(RemoteFileChooser.LOOK_IN_TEXTFIELD);
        String location = (String) lookInText.getValue();
        TraceUtil.trace3("Location " + location);
        // the files are returned in a sorted order
        // hence, no need to sort them again

        try {
            if (model.canRead(location)) {
                files = model.getFiles(location);
            }
        } catch (Exception e) {
           // do nothing
           // the appropriate alert message is created in the view
           // class.
        }

        model.setCurrentDirectory(location);
        OptionList optionList = new OptionList();

        // flag to indicate if files exist in the listbox
        boolean filesExist = false;
        Locale locale = model.getLocale();
        if (locale == null) {
            locale = CCI18N.getTagsLocale(pageContext.getRequest());
        }

        if (files != null && files.length > 0) {
            SimpleDateFormat dateFormat =
                new SimpleDateFormat(model.getDateFormat(), locale);

            SimpleDateFormat timeFormat = null;
            String tmFormat = null;
            if (model instanceof CCFileChooserTimeInterface) {
                tmFormat = ((CCFileChooserTimeInterface)model).getTimeFormat();
            } else {
                tmFormat = "HH:mm";
            }
            timeFormat = new SimpleDateFormat(tmFormat, locale);

            for (int i = 0; i < files.length; i++) {
                String name = files[i].getName();
                String value = new Integer(i).toString();
                boolean bSelectable = false;
                boolean bIsDirectory = files[i].isDirectory();
                boolean disabled = false;

                if (!bIsDirectory) {
                    if (filter.accept(files[i])) {
                        bSelectable = true;
                        // if folderchooser then files should look as
                        // if they are disabled.
                        TraceUtil.trace3(
                            "File choser type is " + model.getType());

                        if (model.getType().equals(
                            CCFileChooserModelInterface.FOLDER_CHOOSER)) {
                            disabled = true;
                        }
                    } else {
                        continue;
                    }
                } else {
                    // all folders are selectable
                    bSelectable = true;
                }

                if (bSelectable) {
                    selectableValuesVec.addElement(value);
                    selectableNamesVec.addElement(name);
                    if (bIsDirectory) {
                        fileOrFolderVec.addElement(
                            CCFileChooserModelInterface.FOLDER_CHOOSER);
                    } else {
                        fileOrFolderVec.addElement(
                            CCFileChooserModelInterface.FILE_CHOOSER);
                    }
                }

                if (bIsDirectory) {
                    name += File.separator;
                }

                name = getDisplayString(name, fileNameLen, space);
                String size = Long.toString(files[i].length());
                size = getDisplayString(size, fileSizeLen, space);

                // RemoteFile.lastModified() returns time in seconds
                Date modifiedDate = new Date(files[i].lastModified() * 1000);
                String date = dateFormat.format(modifiedDate);
                String time = timeFormat.format(modifiedDate);
                NonSyncStringBuffer buffer =
                    new NonSyncStringBuffer(DEFAULT_BUFFER_SIZE);
                buffer.append(name)
                    .append(space)
                    .append(space)
                    .append(space)
                    .append(size)
                    .append(space)
                    .append(space)
                    .append(date)
                    .append(space)
                    .append(time);

                // create a CCOption object using the value, label and
                // disabled state flag and add it to the OptionList object

                CCOption option =
                    new CCOption(buffer.toString(), value, disabled);

                optionList.add(option);
                filesExist = true;
            }
        }

        if (!filesExist) {
            // no files or directories exist
            // A line with "--------" has to be added to make the list box
            // have a width
            String label = "";
            String value = "0";
            int len = fileNameLen + fileSizeLen + fileDateLen + 6;
            for (int i = 0; i < len; i++) {
                label += HYFEN;
            }
            CCOption option = new CCOption(label, "0", true);
            optionList.add(option);
        }

        CCSelectableList fileList = (CCSelectableList)
            fileChooser.getChild(RemoteFileChooser.FILE_LIST);

        CCTextField selectedFiles = (CCTextField)
            fileChooser.getChild(RemoteFileChooser.FILE_NAME_TEXT);

        // unset the button if no files selected

        CCButton openBtn = (CCButton)
            fileChooser.getChild(RemoteFileChooser.OPEN_FOLDER_BUTTON);

        fileList.setOptions(optionList);

        // set multiple to true based on tag attribute value
        fileList.setMultiple(model.multipleSelect());
        return createJavaScriptForFileList(
            fileList, selectedFiles, openBtn,
            selectableValuesVec, selectableNamesVec, fileOrFolderVec,
            dblClickBuffer);
    }

    /**
     * This method returns the string of size maxLen by padding the
     * appropriate amount of spaces next to str.
     */
    protected String getDisplayString(String str, int maxLen, String space) {
        int length = str.length();
        if (length < maxLen) {
            int spaceCount = maxLen - length;
            StringBuffer displayStringBuffer = new StringBuffer(str);
            for (int j = 0; j < spaceCount; j++) {
                displayStringBuffer.append(space);
            }
            str = displayStringBuffer.toString();
        } else if (length > maxLen) {
            int shownLen = maxLen - 3;
            if (isNav4()) {
                shownLen += 3;
            }

            str = str.substring(0, shownLen);
            if (!isNav4()) {
                str += "...";
            }
        }

        return str;
    }

    /**
     * Create the javascript for file list to handle onchange
     */
    protected String createJavaScriptForFileList(CCSelectableList fileList,
        CCTextField selectedFiles, CCButton openBtn,
        Vector values, Vector names, Vector types,
        NonSyncStringBuffer dblClickBuffer) {

        int size = values.size();
        String fileListName = fileList.getQualifiedName();
        String textBoxName = selectedFiles.getQualifiedName();
        String buttonName = openBtn.getQualifiedName();

        RemoteFileChooserModel model =
            (RemoteFileChooserModel) fileChooser.getModel();

        // generate the JavaScript that will disable DBL clicks
        // on files in a folderchooser.

        dblClickBuffer.append("javascript: var types = new Array(")
            .append(size)
            .append(");\n");
        for (int i = 0; i < size; i++) {
            dblClickBuffer.append("types[")
                .append(i)
                .append("] = '")
                .append(types.elementAt(i))
                .append("';\n");
        }
        dblClickBuffer.append("var select = elements['")
            .append(fileListName)
            .append("'];\n")
            .append("if (select.selectedIndex == -1) {\n")
            .append("    return false; \n}\n")
            .append("if ((types[select.selectedIndex] == 'file') &&")
            .append(" (types[select.selectedIndex] != '")
            .append(model.getType())
            .append("')) { \n")
            .append("return false;\n}\n");


        // The user gets to choose the child name, label, tooltip for
        // the control button that is placed at the bottom of the
        // filechooser tag. The following few lines of code disables
        // this button based on certain conditions. For example, the
        // button would be disabled if the user selects a file but
        // the choose button is meant to choose a folder.

        NonSyncStringBuffer jsBuffer =
            new NonSyncStringBuffer(DEFAULT_BUFFER_SIZE);

        jsBuffer.append("javascript: var values = new Array(")
            .append(size)
            .append(");")
            .append("var names = new Array(")
            .append(size)
            .append(");")
            .append("var types = new Array(")
            .append(size)
            .append(");\n");
        for (int i = 0; i < size; i++) {
            jsBuffer.append("values[")
                .append(i)
                .append("] = '")
                .append(values.elementAt(i))
                .append("';")
                .append("names[")
                .append(i)
                .append("] = '")
                .append(names.elementAt(i))
                .append("';")
                .append("types[")
                .append(i)
                .append("] = '")
                .append(types.elementAt(i))
                .append("';\n");
        }

        // The following code disabled the open folder button
        // based on the following situations:
        // a) user selects more than one from the list of files/folders
        // b) user selects a file
        // c) user does not select anything

        // This javascript also performs the following additional
        // function. Add the file/folder to the selected file/folder
        // textfield if its of the appropriate type and has been selected.

        jsBuffer.append("var select = elements['")
            .append(fileListName)
            .append("'];\n")
            .append("ccSetButtonDisabled('")  	// enable open folder button
            .append(buttonName)			// by default
            .append("', '")
            .append(getFormName())
            .append("', false);\n")
            .append("if (select.selectedIndex == -1) {\n")	// disable
            .append("    ccSetButtonDisabled('")
            .append(buttonName)
            .append("', '")
            .append(getFormName())
            .append("', true); return false;\n}\n")
            .append("var j; var k; var text = ''; ")
            .append("var count = 0; var s = false;\n")
            .append("if (types[select.selectedIndex] == 'file') { \n")
            .append("s=true;\n}\n")
            .append("for (j = 0; j < select.options.length; j++) {\n")
            .append("if (select.options[j].selected){ \n")
            .append("count++;\n")
            .append("if (types[j]=='")
            .append(model.getType())
            .append("') {\n")
            .append("\tif (names[j].indexOf(':') != -1){\n")
            .append("\t\tvar tokens = names[j].split(':');\n")
            .append("\t\tvar newName = tokens[0];\n")
            .append("\t\tfor (k = 1; k < tokens.length; k++) {\n")
            .append("\t\t\tnewName = newName + '\\\\:' + tokens[k];\n\t\t}\n")
            .append("\t\tnames[j] = newName;\n\t}\n")
            .append("\tif ((text == null) || (text == '')) {\n")
            .append("\t\ttext = names[j]; \n")
            .append("\t} else {\n")
            .append("\t\ttext = text + ':' + names[j];\n")
            .append("} } } } \n")
            .append("var selText = elements['")
            .append(textBoxName)
            .append("'];\n")
            .append("if (text != '') { \n selText.value=text;\n} ")
            .append("else { \n selText.value='';\n} \n")
            .append("if ((count > 1) || (s)) {\n")
            .append("    ccSetButtonDisabled('")
            .append(buttonName)
            .append("', '")
            .append(getFormName())
            .append("', true);\n}\n")
            .append(" return false;");

        return jsBuffer.toString();
    }

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Tag attribute methods
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    /**
     * Set type of the file chooser.
     *
     * @param value "file" or "folder".
     */
    public void setType(String value) {

        if (value != null
            && !(value.toLowerCase().equals(
                CCFileChooserModelInterface.FILE_CHOOSER))
            && !(value.toLowerCase().equals(
                CCFileChooserModelInterface.FOLDER_CHOOSER))
            && !(value.toLowerCase().equals(
                RemoteFileChooserModel.FILE_AND_FOLDER_CHOOSER))) {

            return;
        }
        setValue(ATTRIB_TYPE, value);
    }

    /**
     * Get the type of the file chooser.
     *
     * @return the type of the file chooser.
     */
    public String getType() {
        return (getValue(ATTRIB_TYPE) != null)
            ? (String) getValue(ATTRIB_TYPE)
            : CCFileChooserModelInterface.FILE_CHOOSER;
    }

    /**
     * Set flag indicating if multiple files/folders can be selected
     *
     * @param value "true" or "false".
     */
    public void setMultipleSelect(String value) {
        if (isInvalidValue(value)) {
            return;
        }
        setValue(ATTRIB_MULTIPLE_SELECT, value);
    }

    /**
     * Get flag indicating if multiple files/folders can be selected
     *
     * @return allow selection of multiple files/folders or not
     */
    public String getMultipleSelect() {
        return (getValue(ATTRIB_MULTIPLE_SELECT) != null)
            ? (String) getValue(ATTRIB_MULTIPLE_SELECT)
            : "false";
    }

    /**
     * Helper method to set tag attributes.
     */
    protected void setAttributes() {

        RemoteFileChooserModel model =
            (RemoteFileChooserModel) fileChooser.getModel();

        if (model.getType() == null) {
            model.setType(getType());
        }

        if (!model.isMultipleSelectSet()) {
            boolean result =
                Boolean.valueOf(getMultipleSelect()).booleanValue();
            model.setMultipleSelect(result);
        }
    }

    protected boolean isInvalidValue(String value) {
        return (value != null
                && !(value.toLowerCase().equals("true"))
                && !(value.toLowerCase().equals("false")));
    }

    /**
     * Helper method to get label HTML strings associated with the
     * given element name.
     */
    protected String getLabel(View view, String name, String value)
        throws JspException {

        CCLabel child =
            new CCLabel((ContainerView) view, name + ".Label", value);
        CCLabelTag ccLabelTag = new CCLabelTag();

        // Set micro-component values.
        ccLabelTag.setBundleID(CCI18N.TAGS_BUNDLE_ID);
        ccLabelTag.setElementName(name);
        NonSyncStringBuffer buffer = new NonSyncStringBuffer();
        buffer.append(ccLabelTag.getHTMLString(
            getParent(), pageContext, child));
        return buffer.toString();
    }

    // Helper method to get href HTML strings.
    protected String getHrefHTMLString(HrefAttributes attrs)
        throws JspException {
        // Get micro components.
        View child = fileChooser.getChild(attrs.name);
        CCHrefTag tag = new CCHrefTag();

        // Set micro component values.
        tag.setBundleID(attrs.bundleID);
        tag.setDefaultValue(attrs.value);
        tag.setStyleClass(attrs.styleClass);
        tag.setTitle(attrs.title);
        tag.setTarget(attrs.target);
        tag.setOnClick(attrs.onClick);
        tag.setOnKeyPress(attrs.onKeyPress);
        tag.setQueryParams(attrs.queryParams);
        tag.setSubmitFormData(attrs.submitFormData);
        tag.setTabIndex(getTabIndex());
        tag.setIsPopup(attrs.isPopup);

        // Set element ID, if required.
        setElementId(tag, child);

        try {
            // Create BodyContent object.
            BodyContent bodyContent = new CCBodyContentImpl(
                new CCJspWriterImpl(null, 100, false));

            // Set body content.
            bodyContent.print(attrs.body);
            tag.setBodyContent(bodyContent);
        } catch (IOException e) {
        }

        // Create buffer to store strings.
        NonSyncStringBuffer buffer = new NonSyncStringBuffer(K);

        // Append new line for readability. Note: New line characters
        // can cause line wraps; thus, it's conditional.
        if (attrs.newLine) {
            buffer.append("\n");
        }

        buffer.append(tag.getHTMLString(getParent(), pageContext, child));

        return buffer.toString();
    }

    // Helper method to get icon HTML strings for the given child names.
    protected String getIconHTMLString(String hrefName, String imageName)
        throws JspException {

        HrefAttributes hrefAttrs = new HrefAttributes();
        ImageAttributes imageAttrs = new ImageAttributes();

        // Set attributes
        hrefAttrs.name = hrefName;
        imageAttrs.name = imageName;

        return getIconHTMLString(hrefAttrs, imageAttrs);
    }

    // Helper method to get icon HTML strings for the given child names.
    protected String getIconHTMLString(HrefAttributes hrefAttrs,
        ImageAttributes imageAttrs) throws JspException {
        // Get HTML string.
        String html = getImageHTMLString(imageAttrs);

        // Get micro components.
        View child = fileChooser.getChild(imageAttrs.name);
        String value = (String) ((DisplayField) child).getValue();

        // Do not use HREFs for disabled pagination icons.
        if (!(value.equals(CCImage.TABLE_PAGINATION_FIRST_DISABLED)
            || value.equals(CCImage.TABLE_PAGINATION_PREV_DISABLED)
            || value.equals(CCImage.TABLE_PAGINATION_NEXT_DISABLED)
            || value.equals(CCImage.TABLE_PAGINATION_LAST_DISABLED)
            || value.equals(CCImage.TABLE_SCROLL_PAGE_DISABLED))) {
            // Get HTML string.
            hrefAttrs.body = html;
            html = getHrefHTMLString(hrefAttrs);
        }

        return html;
    }

    // Helper method to get image HTML strings.
    protected String getImageHTMLString(ImageAttributes attrs)
        throws JspException {
        // Get micro components.
        View child = fileChooser.getChild(attrs.name);
        CCImageTag tag = new CCImageTag();

        // Set micro component values.
        tag.setBundleID(attrs.bundleID);
        tag.setDefaultValue(attrs.src);
        tag.setTitle(attrs.title);
        tag.setAlign(isNav4() ? "absmiddle" : attrs.align);
        tag.setAlt(attrs.alt);
        tag.setHeight(attrs.height);
        tag.setWidth(attrs.width);
        tag.setBorder(attrs.border);

        // Create buffer to store strings.
        NonSyncStringBuffer buffer = new NonSyncStringBuffer(K);

        // Append new line for readability. Note: New line characters
        // can cause line wraps; thus, it's conditional.
        if (attrs.newLine) {
            buffer.append("\n");
        }

        buffer.append(tag.getHTMLString(getParent(), pageContext, child));

        return buffer.toString();
    }

    // Helper method to get DOT image HTML strings.
    protected String getDotImageHtmlString(int height, int width) {
        // Create buffer to store strings.
        NonSyncStringBuffer buffer = new NonSyncStringBuffer("\n");
        buffer.append(getImageHTMLString(CCImage.DOT, height, width));

        return buffer.toString();
    }

    // Helper method to get button HTML strings for the given child name.
    protected String getButtonHTMLString(String name) throws JspException {
        ButtonAttributes buttonAttrs = new ButtonAttributes();
        buttonAttrs.name = name;
        return getButtonHTMLString(buttonAttrs);
    }

    // Helper method to get button HTML strings.
    protected String getButtonHTMLString(ButtonAttributes buttonAttrs)
        throws JspException {
        // Get micro components.
        CCButton child = (CCButton) fileChooser.getChild(buttonAttrs.name);
        CCButtonTag tag = new CCButtonTag();

        // Set micro component values.
        tag.setBundleID(buttonAttrs.bundleID);
        tag.setDefaultValue(buttonAttrs.value);
        tag.setTitle(buttonAttrs.title);
        tag.setOnClick(buttonAttrs.onClick);
        tag.setTabIndex(getTabIndex());

        // Create buffer to store strings.
        NonSyncStringBuffer buffer = new NonSyncStringBuffer("\n");
        buffer.append(tag.getHTMLString(getParent(), pageContext, child));

        return buffer.toString();
    }


    // Set unique element ID. For some elements, both element name and
    // ID are required to maintain focus when a page is submitted.
    // Since this tag outputs elements which use the same name (e.g.,
    // sort icons), a unique element ID is required. Without this
    // attribute, the first element found will get focus.
    private void setElementId(CCTagBase tag, View view) {
        // Use the qualified name container view.
        String elementId = view.getQualifiedName() + ".Id" + elementIndex++;

        // Set element ID.
        if (view instanceof CCAccessible) {
            ((CCAccessible) view).setElementId(elementId);
        } else {
            tag.setElementId(elementId);
        }
    }

    // Data structure for button attributes.
    protected class ButtonAttributes {
        public String bundleID = CCI18N.TAGS_BUNDLE_ID;
        public String name  = null;
        public String value = null;
        public String title = null;
        public String onClick = null;

        public ButtonAttributes() {
        }
    }

    // Data structure for href attributes.
    protected class HrefAttributes {
        public String bundleID = CCI18N.TAGS_BUNDLE_ID;
        public String name  = null;
        public String value = null;
        public String styleClass = null;
        public String title = null;
        public String target = null;
        public String onClick = null;
        public String onKeyPress = null;
        public String queryParams = null;
        public String body = null;
        public String submitFormData = "true";
        public boolean newLine = true;
        public String isPopup = null;

        public HrefAttributes() {
        }
    }

    // Data structure for image attributes.
    protected class ImageAttributes {
        public String bundleID = CCI18N.TAGS_BUNDLE_ID;
        public String name = null;
        public String src  = null;
        public String title = null;
        public String align = "top";
        public String alt = null;
        public String height = null;
        public String width = null;
        public String border = "0";
        public boolean newLine = true;

        public ImageAttributes() {
        }
    }
}
