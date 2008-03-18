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

// ident	$Id: RemoteFileChooserControlTag.java,v 1.13 2008/03/17 14:43:53 am143972 Exp $

package com.sun.netstorage.samqfs.web.remotefilechooser;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.HtmlUtil;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.JspDisplayEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.util.ConversionUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.common.CCI18N;
import com.sun.web.ui.model.CCFileChooserModelInterface;
import com.sun.web.ui.taglib.common.CCTagBase;
import com.sun.web.ui.taglib.html.CCButtonTag;
import com.sun.web.ui.taglib.html.CCHiddenTag;
import com.sun.web.ui.taglib.html.CCTextFieldTag;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCTextField;
import java.io.File;
import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;
import javax.servlet.http.HttpSession;
import javax.servlet.jsp.JspException;
import javax.servlet.jsp.PageContext;
import javax.servlet.jsp.tagext.Tag;

public class RemoteFileChooserControlTag extends CCTagBase {

    protected static final String IMGSRCNAME  = "srcName";
    protected static final String IMGSRCALT   = "srcAlt";
    protected static final String ATTRIB_TYPE = "type";
    protected static final
        String ATTRIB_MULTIPLE_SELECT = "allowMultipleSelect";
    protected static final String ATTRIB_MAX_LENGTH = "maxLength";
    protected static final String ATTRIB_SIZE = "size";
    protected static final int DEFAULT_SIZE = 50;
    protected static final String ATTRIB_READ_ONLY = "readOnly";
    protected static final String ATTRIB_SHOW_TEXT_FIELD = "showTextfield";
    protected static final String ATTRIB_SHOW_CLEAR = "showClearButton";
    protected static final
        String ATTRIB_PARENT_REFRESH_CMD = "parentRefreshCmd";
    protected static final String ATTRIB_ONCLOSE_SCRIPT = "onCloseScript";
    protected static final String ATTRIB_BUTTON_LABEL = "buttonLabel";

    // Set window defaults.
    protected int FCHOOSER_WINDOW_HEIGHT = 700;
    protected int FCHOOSER_WINDOW_WIDTH = 675;
    protected static final String FCHOOSER_WINDOW_NAME = "fileChooser";

    protected RemoteFileChooserControl fileChooser;

    public RemoteFileChooserControlTag() {
        super();
        TraceUtil.initTrace();
        TraceUtil.trace3("RemoteFileChooserControlTag Ctor being called");
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

        TraceUtil.trace3("getHTMLStringInternal() being called");

        if (parent == null) {
            throw new IllegalArgumentException("parent cannot be null");
        } else if (pageContext == null) {
            throw new IllegalArgumentException("pageContext cannot be null");
        } else if (view == null) {
            throw new IllegalArgumentException("view cannot be null");
        }

        // See bugtraq ID 4994832.
        super.getHTMLStringInternal(parent, pageContext, view);

        checkChildType(view, RemoteFileChooserControl.class);
        fileChooser = (RemoteFileChooserControl) view;
        RemoteFileChooserModel model =
                            (RemoteFileChooserModel) fileChooser.getModel();
        if (model == null) {
            throw new IllegalArgumentException();
        }

        // Initialize tag parameters.
        setParent(parent);
        setPageContext(pageContext);
        setAttributes(model);
        TraceUtil.trace3("after setting attributes type =  " + model.getType());
        // Evoke the begin display event for the view
        // Set state by evoking the beginDisplay() method of the view.

        // Store the parent refresh command in the model so that
        // the chooser pop-up will have access to it when it opens
        model.setParentRefreshCmd(getParentRefreshCmd());

        // Store the onCloseScript so that when the popup is closed it knows
        // what function to run.
        model.setOnClose(getOnClose());

        try {
            fileChooser.beginDisplay(new JspDisplayEvent(this, pageContext));
        } catch (ModelControlException e) {
            throw new JspException(e.getMessage());
        }

        NonSyncStringBuffer buffer =
            new NonSyncStringBuffer(DEFAULT_BUFFER_SIZE);

        buffer.append("<!-- beginning of filechooser component -->\n");
        TraceUtil.trace3("before appendFileChooserControl  " + model.getType());
        appendFileChooserControl(buffer, model);
        buffer.append("<!-- end of filechooser component -->\n");

        return buffer.toString();
    }

    /*
     * Generate the HTML to create button followed by a textt field.
     * The user will click on this button to launch the filechooser
     * window
     */
    protected void appendFileChooserControl(NonSyncStringBuffer buffer,
        RemoteFileChooserModel model) throws JspException {

        TraceUtil.trace3("appendFileChooserControl() entry");

        CCTextField textField = (CCTextField) fileChooser.getChild(
                            RemoteFileChooserControl.BROWSED_FILE_NAME);
        CCHiddenField hiddenField = (CCHiddenField) fileChooser.getChild(
                              RemoteFileChooserControl.CHILD_PATH_HIDDEN);

        // Get current directory and file name from value in text or hidden
        // field.  Punt if multi select
        String curDir = null;
        String fileName = null;
        if (getMultipleSelect().equals("false")) {
            if (getShowTextField().equals("true")) {
                curDir = (String) textField.getValue();
            } else {
                curDir = (String) hiddenField.getValue();
            }
            // If file chooser, curDir is a fully qualified path name.
            if (curDir != null && model.getType().equals(
                CCFileChooserModelInterface.FILE_CHOOSER)) {
                // Add selected file into model if necessary
                if (model.getSelectedFiles() == null ||
                    model.getSelectedFiles().length == 0) {
                    // Add selected file
                    model.addSelectedFile(curDir);
                }
                // Get parent directory.
                File file = new File(curDir);
                curDir = file.getParent();
            }
        }

        // Set the current directory in the model.  This effects the popup
        // when its displayed.
        model.clearCachedDir();
        if (curDir != null) {
            model.setCurrentDirectory(curDir);
        }

        // Render text field or hidden value
        String fileNameHtml = "";
        String hiddenHtml = "";
        if (getShowTextField().equals("true")) {
            CCTextFieldTag textTag = new CCTextFieldTag();
            textTag.setSize(getSize());
            textTag.setTabIndex(getTabIndex());
            textTag.setAutoSubmit("false");
            textTag.setElementId(getElementId());
            textTag.setMaxLength(getMaxLength());
            textTag.setReadOnly(getReadOnly());

            fileNameHtml =
                    textTag.getHTMLString(getParent(), pageContext, textField);
        } else {
            CCHiddenTag hiddenTag = new CCHiddenTag();
            hiddenHtml =
                hiddenTag.getHTMLString(getParent(),  pageContext, hiddenField);
        }


        // Browse button

        View browserChild =
            fileChooser.getChild(
                RemoteFileChooserControl.BROWSER_SERVER_BUTTON);
        checkChildType(browserChild, CCButton.class);
        CCButton browserButton = (CCButton) browserChild;
        if (model.getType().equals(
            RemoteFileChooserModel.FILE_AND_FOLDER_CHOOSER)) {
            browserButton.setAlt("filechooser.fileChooserTitle");
            browserButton.setTitle("filechooser.fileChooserTitle");
        } else if (model.getType().equals(
            CCFileChooserModelInterface.FOLDER_CHOOSER)) {
            browserButton.setAlt("filechooser.folderChooserTitle");
            browserButton.setTitle("filechooser.folderChooserTitle");
        } else {
            browserButton.setAlt("filechooser.fileChooserTitle");
            browserButton.setTitle("filechooser.fileChooserTitle");
        }
        String browserButtonLabel = getButtonLabel();
        if (browserButtonLabel != null) {
            browserButton.setDisplayLabel(
                                SamUtil.getResourceString(browserButtonLabel));
        } else {
            browserButton.setDisplayLabel(
                            SamUtil.getResourceString("browserWindow.browse"));
        }

        CCButtonTag browserButtonTag = new CCButtonTag();

        // Set micro component values.
        browserButtonTag.setBundleID(CCI18N.TAGS_BUNDLE_ID);
        browserButtonTag.setType(CCButton.TYPE_SECONDARY_MINI);
        browserButtonTag.setTabIndex(getTabIndex());

        // enable dynamic disabling & enabling of the browse button
        browserButtonTag.setDynamic("true");

        /**
         * When the button is clicked it should launch a new
         * browser window and the HTML contained in that
         * window will consist of a secondary masthead tag, an
         * in page filechooser tag that includes the choose file
         * and cancel button.
         *
         * Get the name of the field that will contain the selected file or
         * folder.  This field will be referenced in JavaScript via the choosers
         * "choose" button.  When the text field is visible, this will be the
         * name of the text field.  When the text field is not visible, this
         * will be the name of a hidden field.
         */
        String browseValueFieldName = "";
        if (getShowTextField().equals("true")) {
            browseValueFieldName = textField.getQualifiedName();
        } else {
            browseValueFieldName = hiddenField.getQualifiedName();
        }

        browserButtonTag.setOnClick(
                appendOpenWindowHTML(model, browseValueFieldName) +
                "; return false;");

        String browserButtonHTML = browserButtonTag.getHTMLString(getParent(),
                                           pageContext,
                                           browserButton);

        // Clear server button
        String clearButtonHTML = "";
        if (getShowClearButton() == Boolean.toString(true)) {
            CCButton clearChild = (CCButton)
                fileChooser.getChild(RemoteFileChooserControl.CHILD_CLEAR);
            clearChild.setAlt("browserWindow.clear.toolTip");
            clearChild.setTitle("browserWindow.clear.toolTip");

            CCButtonTag clearButtonTag = new CCButtonTag();
            clearButtonTag.setTabIndex(getTabIndex());

            // When the button is clicked it should clear the contents of the
            // text field or hidden value.  Not too useful when the
            // text field is not visible.
            NonSyncStringBuffer onClk = new NonSyncStringBuffer()
                .append("javascript: this.form.elements['")
                .append(browseValueFieldName)
                .append("'].value = ''; return false;");

            clearButtonTag.setOnClick(onClk.toString());

            clearButtonHTML = clearButtonTag.getHTMLString(getParent(),
                                               pageContext,
                                               clearChild);
        }

        buffer.append(fileNameHtml).append(getSpace())
              .append(hiddenHtml).append(getSpace())
              .append(browserButtonHTML).append(getSpace())
              .append(clearButtonHTML);
    }

    /**
     * Append the javascript/HTML that would be required to open
     * a new window to display the filechooser tag.
     */
    protected String appendOpenWindowHTML(RemoteFileChooserModel model,
                                          String textFieldName)
        throws JspException {

        TraceUtil.trace3("appendOpenWindowHTML() being called");

        // Get the context path and prepend it to the fileshooser popup
        // viewbean.

        String model_key = String.valueOf(System.currentTimeMillis());
        String cPath = getRequestContext().getRequest().getContextPath();
        String fileChooserURL =
            cPath + "/remotefilechooser/RemoteFileChooserWindow?model_key=" +
            model_key;

        HttpSession session =
            getRequestContext().getRequest().getSession();

        session.setAttribute(model_key, model);
        model.setFieldName(textFieldName);
        String image = getMessage(model.getProductNameSrc());
        String imageAlt = getMessage(model.getProductNameAlt());
        session.setAttribute(IMGSRCNAME, image);
        session.setAttribute(IMGSRCALT, imageAlt);

        // return onClick event Javascript.
        if ((isNav4())) {
            FCHOOSER_WINDOW_HEIGHT = 780;
            FCHOOSER_WINDOW_WIDTH = 675;
        } else if (isIe5up()) {
            FCHOOSER_WINDOW_HEIGHT = 675;
            FCHOOSER_WINDOW_WIDTH = 650;
        }

        // return onClick event Javascript.
        return getOpenWindowJavascript(
            fileChooserURL,
            FCHOOSER_WINDOW_NAME + "_" + HtmlUtil.getUniqueValue(),
            FCHOOSER_WINDOW_HEIGHT, FCHOOSER_WINDOW_WIDTH,
            "scrollbars=yes,resizable");
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
     * Set type of the file chooser.
     *
     * @param value "file" or "folder" or "fileAndFolder".
     */
    public void setType(String value) {

        TraceUtil.trace3("Setting value to " + value);
        TraceUtil.trace3(
            "Added " + RemoteFileChooserModel.FILE_AND_FOLDER_CHOOSER);
        if (value != null
            && !(value.toLowerCase().equals(
                CCFileChooserModelInterface.FILE_CHOOSER))
            && !(value.toLowerCase().equals(
                CCFileChooserModelInterface.FOLDER_CHOOSER))
            && !(value.toLowerCase().equals(
                RemoteFileChooserModel.FILE_AND_FOLDER_CHOOSER))) {
                    TraceUtil.trace3("Not a recognised type");
            return;
        }
        TraceUtil.trace3("Setting " + ATTRIB_TYPE + " to " + value);
        setValue(ATTRIB_TYPE, value);
        TraceUtil.trace3("Type = " + getValue(ATTRIB_TYPE));
    }

    /**
     * Get the type of the file chooser.
     *
     * @return the type of the file chooser.
     */
    public String getType() {
        TraceUtil.trace3("type is " + getValue(ATTRIB_TYPE));
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
     * Set the maximum length for entires in the text field.  Setting an
     * invalid max length is like setting none.
     *
     * @param value A positive integer number.
     */
    public void setMaxLength(String value) {
        int val = 0;
        try {
            val = ConversionUtil.strToIntVal(value);
            if (val < 0) {
                return;
            }
        } catch (SamFSException e) {
            // Not a number.
            return;
        }
        setValue(ATTRIB_MAX_LENGTH, new Integer(val));
    }

    /**
     * Get the maximum length for entires in the text field
     *
     * @return allow selection of multiple files/folders or not
     */
    public String getMaxLength() {
        Object obj = getValue(ATTRIB_MAX_LENGTH);
        String retVal = "";
        if (obj != null) {
            Integer intVal = (Integer) obj;
            retVal = intVal.toString();
        }
        return retVal;
    }

    /**
     * Set the size for the text field
     *
     * @param value A positive integer number.
     */
    public void setSize(String value) {
        int val = DEFAULT_SIZE;
        try {
            val = ConversionUtil.strToIntVal(value);
            if (val < 0) {
                val = DEFAULT_SIZE;
            }
        } catch (SamFSException e) {
            // Not a number
        }
        setValue(ATTRIB_SIZE, new Integer(val));
    }

    /**
     * Get the size for the text field
     *
     * @return the size
     */
    public String getSize() {
        Object obj = getValue(ATTRIB_SIZE);
        String retVal = "";
        if (obj != null) {
            Integer intVal = (Integer) obj;
            retVal = intVal.toString();
        } else {
            retVal = String.valueOf(DEFAULT_SIZE);
        }
        return retVal;
    }

    /**
     * Set the read-only attribute for the text field
     *
     * @param value true or false
     */
    public void setReadOnly(String value) {
        boolean val = false;
        if (value.equals(Boolean.toString(true))) {
            val = true;
        }
        setValue(ATTRIB_READ_ONLY, new Boolean(val));
    }

    /**
     * Get the read only attribute for the text field
     *
     * @return true or false
     */
    public String getReadOnly() {
        Boolean val = (Boolean) getValue(ATTRIB_READ_ONLY);
        String retVal = Boolean.toString(false);
        if (val != null && val.booleanValue()) {
            retVal = val.toString();
        }
        return retVal;
    }

    /**
     * If true, shows the Clear button which is useful in clearing the value
     * if the text field is read only.
     *
     * @param value true or false
     */
    public void setShowClearButton(String value) {
        boolean val = false;
        if (value.equals(Boolean.toString(true))) {
            val = true;
        }
        setValue(ATTRIB_SHOW_CLEAR, new Boolean(val));
    }

    /**
     * Get the read only attribute for the text field
     *
     * @return true or false
     */
    public String getShowClearButton() {
        Boolean val = (Boolean) getValue(ATTRIB_SHOW_CLEAR);
        String retVal = Boolean.toString(false);
        if (val != null && val.booleanValue()) {
            retVal = val.toString();
        }
        return retVal;
    }

    /**
     * Shows or hides the text field in the file chooser control.
     *
     * @param value true or false
     */
    public void setShowTextField(String value) {
        boolean val = true;
        if (value.equals(Boolean.toString(false))) {
            val = false;
        }
        setValue(ATTRIB_SHOW_TEXT_FIELD, new Boolean(val));
    }

    /**
     * Gets the current visibility setting for the text field.
     *
     * @return true or false
     */
    public String getShowTextField() {
        Boolean val = (Boolean) getValue(ATTRIB_SHOW_TEXT_FIELD);
        String retVal = Boolean.toString(true);
        if (val != null && !val.booleanValue()) {
            retVal = val.toString();
        }
        return retVal;
    }

    /**
     * When parentRefreshCmd is set, the parent page will be refreshed when the
     * chooser's "choose" button is clicked and the chooser pop-up is closed.
     * As part of the refresh, the handler for the indicated command object
     * will be run.  For example, if this property = "postChooserCmd", then the
     * handlePostChooserCmdRequest method will be called on the parent view
     * after "choose" is clicked.  If this property is not set,
     * than the parent page was not be refreshed.
     *
     * @param value true or false
     */
    public void setParentRefreshCmd(String value) {
        if (value != null && value.length() == 0) {
            value = null;
        }
        setValue(ATTRIB_PARENT_REFRESH_CMD, value);
    }

    /**
     * Returns the parent refresh command, if any.  If null,
     * there is no parent refreshed command in the parent will not be refreshed
     * when the chooser closes.
     *
     */
    public String getParentRefreshCmd() {
        return (String) getValue(ATTRIB_PARENT_REFRESH_CMD);
    }

    /**
     * When onClose is set, the javascript specified in the
     * property will be run when the popup closes using the "Choose" button".
     * The script won't run when the popup is closed with "Cancel".
     */
    public void setOnClose(String value) {
        if (value != null && value.length() == 0) {
            value = null;
        }
        setValue(ATTRIB_ONCLOSE_SCRIPT, value);
    }

    public String getOnClose() {
        return (String) getValue(ATTRIB_ONCLOSE_SCRIPT);
    }

    /**
     * Sets the label on the chooser button.  If null, the default Lockhart
     * label will be used.
     */
    public void setButtonLabel(String value) {
        if (value != null && value.length() == 0) {
            value = null;
        }
        setValue(ATTRIB_BUTTON_LABEL, value);
    }

    /**
     *
     */
    public String getButtonLabel() {
        return (String) getValue(ATTRIB_BUTTON_LABEL);
    }

    /**
     * Helper method to set tag attributes.
     */
    protected void setAttributes(RemoteFileChooserModel model) {
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

    protected String getOpenWindowJavascript(String url, String name,
        int height, int width, String features) {

        if (url == null) {
            throw new IllegalArgumentException("url cannot be null");
        }

        NonSyncStringBuffer buffer = new NonSyncStringBuffer(K);
        String urlAnchor = "";

        // Append query string com_sun_web_ui_popup=true
        try {
            if (url.length() > 0 && !url.startsWith("javascript")) {
                NonSyncStringBuffer urlBuffer = new NonSyncStringBuffer(K);

                // Append delimeter.
                if (url.indexOf("?") == -1) {
                    urlBuffer.append("?");
                } else {
                    urlBuffer.append("&amp;");
                }

                urlBuffer.append(URLEncoder.encode(POPUP_WINDOW, encoding))
                    .append("=")
                    .append(URLEncoder.encode("true", encoding));

                // Insert before existing URL anchor.
                int index = url.indexOf("#");
                if (index == -1) {
                    url = url.concat(urlBuffer.toString());
                } else {
                    NonSyncStringBuffer tmpBuffer = new NonSyncStringBuffer(K);
                    tmpBuffer.append(url.substring(0, index -1));
                    tmpBuffer.append(urlBuffer.toString());
                    url = tmpBuffer.toString();
                    urlAnchor = url.substring(index);
                }
            }
        } catch (UnsupportedEncodingException e) {
            TraceUtil.trace3(encoding + " encoding is not supported.");
        }

        buffer.append("javascript:var url='")
            .append(url)
            .append("'; urlAnchor='")
            .append(urlAnchor)
            .append("'; var fullURL=url; ")
            .append("var rfcParams=getFileChooserParams(this); ")
            .append("if (rfcParams != null) { fullURL += rfcParams; } ")
            .append("fullURL += urlAnchor; ")
            .append("var win = window.open(fullURL,'")
            .append(name)
            .append("','")
            .append("height=")
            .append(height)
            .append(",width=")
            .append(width)
            .append(",top='+((screen.height-(screen.height/1.618))-(")
            .append(height)
            .append("/2))+',left='+((screen.width-")
            .append(width)
            .append(")/2)+'");

        if (features != null) {
            buffer.append(",").append(features);
        }

        buffer.append("')");

        if (getClientSniffer().getUserAgentMajor() >= 4) {
            buffer.append(";win.focus()");
        }

        return buffer.toString();
    }
}
