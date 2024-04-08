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

// ident	$Id: WizardWindowTag.java,v 1.13 2008/12/16 00:12:25 am143972 Exp $

package com.sun.netstorage.samqfs.web.ui.taglib;

import com.iplanet.jato.util.HtmlUtil;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBean;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.common.CCDebug;
import com.sun.web.ui.common.CCI18N;
import com.sun.web.ui.model.CCWizardWindowModelInterface;
import com.sun.web.ui.servlet.wizard.WizardWindowViewBean;
import com.sun.web.ui.taglib.html.CCButtonTag;
import com.sun.web.ui.taglib.wizard.CCWizardWindowTag;
import com.sun.web.ui.view.wizard.CCWizardWindow;
import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;
import java.util.StringTokenizer;
import javax.servlet.jsp.JspException;
import javax.servlet.jsp.PageContext;
import javax.servlet.jsp.tagext.Tag;

/**
 * this replacement tag for lockhart's CCWizardWindowTag splits the wizard URL
 * into three components: the base URL, the default parameters defined by the
 * wizard window model, and client-side user provided parameters. This allows
 * developers to append request parameters to the wizard URL.
 *
 * To enable this functionality, the name of the component must start with the
 * value of the Constants.WIZARD_BUTTON_KEYWORD.
 *
 * Another way to use this tag is to have it generate the script needed to
 * launch a wizard, but not display a button.  This script can be called by
 * other components, thereby allowing us to launch wizards from componentst that
 * aren't buttons.  The script will be named "launch<wizard name>Wizard" where
 * <wizard name> is the name of the wizard tag (child name) (therefore, be sure
 * component name only includes characters that can be included in function
 * names). To enable this functionality, the name of the component must start
 * with the value of the Constants.WIZARD_SCRIPT_KEYWORD.
 */
public class WizardWindowTag extends CCButtonTag {
    public static final String PASS_PAGE_SESSION = "wizPassPageSession";

    // key to a js function that will be called on the client-side to provide
    // client-side parameters to be passed to the wizard
    public static final String CLIENTSIDE_PARAM_JSFUNCTION =
        "clientsideParameterJSFunction";

    public static final int TYPE_BUTTON = 0;
    public static final int TYPE_SCRIPT = 1;
    private int wizardType = TYPE_BUTTON;
    private String ampersand = new String();

    protected static final String URI = "/wizard/WizardWindow";
    protected static final String WINDOW_NAME = "wizardWindow";

    protected CCWizardWindow theView = null;
    protected CCWizardWindowModelInterface theModel  = null;

    /**
     * constructor
     */
    public WizardWindowTag() {
        super();
        CCDebug.initTrace();
    }

    /**
     * clear the tag's member variables
     */
    public void reset() {
        super.reset();
        theView = null;
        theModel = null;
    }

    public void setWizardType(int type) {
        if (type == TYPE_BUTTON ||
            type == TYPE_SCRIPT) {
            this.wizardType = type;
        } else {
            this.wizardType = TYPE_BUTTON;
            TraceUtil.trace2("Invalid wizard type:  " + String.valueOf(type) +
                             ".  Using button type instead.");
        }
    }

    public int getWizardType() {
        return this.wizardType;
    }

    /**
     * this is the main method that generates the tag's HTML output
     */
    public String getHTMLStringInternal(Tag parent,
                                        PageContext pageContext,
                                        View view) throws JspException {
        // make sure all the parameters are valid
        String msg = null;
        if (parent == null) {
            msg = "Tag parameter is null";
            CCDebug.trace1(msg);
            throw new IllegalArgumentException(msg);
        } else if (pageContext == null) {
            msg = "PageContext parameter is null";
            CCDebug.trace1(msg);
            throw new IllegalArgumentException(msg);
        } else if (view == null) {
            msg = "View parameter is null";
            CCDebug.trace1(msg);
            throw new IllegalArgumentException(msg);
        }

        // verify is that the view passed is of the right type
        checkChildType(view, CCWizardWindow.class);
        theView = (CCWizardWindow)view;
        theModel = (CCWizardWindowModelInterface)theView.getModel();

        // init tag
        setParent(parent);
        setPageContext(pageContext);
        setAttributes();

        // holds the response string
        StringBuffer buffer = new StringBuffer();

        if (this.wizardType == TYPE_SCRIPT) {
            ampersand = "&";
        } else {
            // Needs to use entity because of placement in HTML
            ampersand = "&amp;";
        }
        // check for the client-side javascript method to be called to set
        // the client-side parameters. - default to 'getClientParams()'
        String jsMethod = (String)
            theModel.getValue(CLIENTSIDE_PARAM_JSFUNCTION);
        if (jsMethod == null)
            jsMethod = "getClientParams";

        // retrieve the various parts of the 'window.open(...)' call
        String url = getWizardURL();
        String defaultParameters = getDefaultWizardParameters();
        String windowGeometry = getWindowGeometry();
        String windowName = (new StringBuffer(WINDOW_NAME))
                            .append("_")
                            .append(HtmlUtil.getUniqueValue())
                            .toString();

        // Set the Onclick attribute
        StringBuffer onClick = new StringBuffer();
        if (this.wizardType == TYPE_SCRIPT) {
            onClick.append("var url='");
        } else {
            onClick.append("javascript: var url='");
        }
        onClick.append(url)
               .append("'; var defaultParams='")
               .append(defaultParameters)
               .append("'; var geometry='")
               .append(windowGeometry)
               .append("'; var windowName='")
               .append(windowName)
               .append("'; var csParams=")
               .append(jsMethod)
               .append("();")
               .append("var fullURL = url; if (csParams != null) {")
               .append(" fullURL += csParams + '")
               .append(ampersand)
               .append("';}")
               .append(" fullURL += defaultParams;")
               .append(" var win = window.open(fullURL, windowName, geometry);")
               .append("win.focus();return true;");

        // the tags payload
        buffer.append("\n<!-- Start Wizard Tag -->\n");

        // Show button or only output script?
        if (wizardType == TYPE_SCRIPT) {
            buffer.append("<script language='javascript'>\n")
                  .append("\tfunction launch")
                  .append(getName())
                  .append("Wizard() {\n\t\t")
                  .append(onClick.toString())
                  .append("\n\t}\n")
                  .append("\n</script>\n");
        } else {
            setOnClick(onClick.toString());
            buffer.append(
                super.getHTMLStringInternal(parent, pageContext, theView));
        }
        buffer.append("\n<!-- End Wizard Tag -->\n");



        return buffer.toString();
    }

    /**
     * returns <protocol>://<server>:<port>/<app>/wizard/WizardWindow?
     */
    protected String getWizardURL() {
        StringBuffer url = new StringBuffer();
        String path = getRequestContext().getRequest().getContextPath();
        url.append(path)
           .append(URI)
           .append("?");

        return url.toString();
    }

    /**
     * returns CCWizardWindowModelInterface instance parameters in the form of
     * of name value i.e. pageName=xyz
     */
    protected String getDefaultWizardParameters() {
        StringBuffer buffer = new StringBuffer();

        // remember the form name
        theModel.setValue(theModel.WIZARD_BUTTON_FORM, getFormName());

        // add the popup parameter
        theModel.setValue(POPUP_WINDOW, "true");

        String params =
            theModel.toRequestParametersString(WizardWindowViewBean.PAGE_NAME);
        buffer.append(encodeParams(params));

        String extraParams = theModel.toExtraRequestParameters();
        buffer.append(encodeParams(extraParams));

        // append the JATO page session is appropriate
        Boolean appendPageSession =
            (Boolean)theModel.getValue(PASS_PAGE_SESSION);

        if (appendPageSession != null && appendPageSession.booleanValue()) {
            buffer.append(WizardWindowViewBean.PAGE_NAME)
                .append(".")
                .append(WizardWindowViewBean.CHILD_CMD_FIELD)
                .append("=")
                .append(ampersand)
                .append(ViewBean.PAGE_SESSION_ATTRIBUTE_NVP_NAME)
                .append("=");
            String pageSessionString = ((ViewBean)theView.getParentViewBean())
                .getPageSessionAttributeString(false);
            buffer.append(pageSessionString);
        } else {
            buffer.append(WizardWindowViewBean.PAGE_NAME)
                .append(".")
                .append(WizardWindowViewBean.CHILD_CMD_FIELD)
                .append("=");
        }
        return buffer.toString();

    }

    /**
     * return such window geometrical features as the height, width, top, left,
     * scrollbars, and resizable
     */
    protected String getWindowGeometry() {
        StringBuffer geometry = new StringBuffer();

        // determine the height and width of the wizard window
        Integer h = (Integer)theModel.getValue(theModel.WINDOW_HEIGHT);
        Integer w = (Integer)theModel.getValue(theModel.WINDOW_WIDTH);

        int height = h == null ? theModel.PIXEL_WINDOW_HEIGHT : h.intValue();
        int width = w == null ? theModel.PIXEL_WINDOW_WIDTH : w.intValue();

        // if using netscape 4 as client enable scrollbars
        String useScrollBars = isNav4() ? "yes" : "no";

        geometry.append("height=")
                .append(height)
                .append(", width=")
                .append(width)
                .append(", top='+((screen.height-(screen.height/1.618))-(")
                .append(height)
                .append("/2))+'")
                .append(", left='+((screen.width-")
                .append(width)
                .append(")/2)+'")
                .append("scrollbars=")
                .append(useScrollBars)
                .append(",resizable");

        return geometry.toString();
    }

    /**
     * UTF8 encode the Request Parameters skipping the '&' and '=' characters
     */
    protected String encodeParams(String params) {
        StringBuffer buffer = new StringBuffer();

        if (params != null && params.length() > 0) {
            StringTokenizer st1 = new StringTokenizer(params, "&");

            while (st1.hasMoreTokens()) {
                StringTokenizer st2 = new StringTokenizer(st1.nextToken(), "=");

                try {
                    if (st2.hasMoreTokens()) {
                        // encode and append the name part
                        buffer.append(URLEncoder.encode(st2.nextToken(),
                                                        CCI18N.UTF8_ENCODING));
                    }

                    // encode and append the value part
                    if (st2.hasMoreTokens()) {
                        buffer.append("=")
                              .append(URLEncoder.encode(st2.nextToken(),
                                                        CCI18N.UTF8_ENCODING))
                              .append(ampersand);
                    }
                } catch (UnsupportedEncodingException uee) {
                }
            } // end while
        } // end if params

        return buffer.toString();
    }
}
