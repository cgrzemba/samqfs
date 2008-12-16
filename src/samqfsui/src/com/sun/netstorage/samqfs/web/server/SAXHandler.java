/*
 *
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

// ident	$Id: SAXHandler.java,v 1.13 2008/12/16 00:12:24 am143972 Exp $

package com.sun.netstorage.samqfs.web.server;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import java.io.InputStream;
import java.util.HashMap;
import javax.servlet.ServletContext;
import javax.xml.parsers.SAXParser;
import javax.xml.parsers.SAXParserFactory;

import org.xml.sax.Attributes;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;
import org.xml.sax.helpers.DefaultHandler;

/**
 * Parse VersionHighlightContent.xml file using JAXP and SAX.
 * Insert contents into a data structure and eventually enter into
 * an action table model for the Version Highlight Page
 */

public class SAXHandler extends DefaultHandler {

    private int key = 0;
    private static HashMap myHashMap;
    private String featureType;
    private String featureName;
    private String serverVersion;
    private String versionNumber;
    private String versionStatus;
    private NonSyncStringBuffer versionInfo;
    private HighlightInfo highlightInfo;

    /**
     * nowServing is a variable to identify which tag the reader
     * is currently reading.
     *
     * 0 => Feature Name
     * 1 => Version Number
     * 2 => Version Status
     */

    private int nowServing = -1;


    public static void parseIt() throws SamFSException {

        DefaultHandler handler = new SAXHandler();
        SAXParserFactory factory = SAXParserFactory.newInstance();

        try {
            SAXParser saxParser = factory.newSAXParser();
            String path = "/jsp/server/VersionHighlightContent.xml";

            ServletContext sc =
                RequestManager.getRequestContext().getServletContext();
            InputStream is = sc.getResourceAsStream(path);
            InputSource isource = new InputSource(is);

            SAXFilter filter = new SAXFilter();
            filter.setContentHandler(handler);
            filter.setErrorHandler(handler);
            filter.setParent(saxParser.getXMLReader());

            filter.parse(isource);
            // saxParser.parse(isource, handler);
        } catch (Throwable t) {
            TraceUtil.trace1(
                "Error occurred while parsing version highlight version file." +
                " Reason: " + t.getMessage());
            throw new SamFSException(null, -2550);
        }
    }

    public void startDocument() throws SAXException {
        TraceUtil.trace2("Start parsing version highlight XML file ...");

        // initialize the hashMap
        if (myHashMap == null) {
            myHashMap = new HashMap();
        } else {
            myHashMap.clear();
        }
    }

    public void endDocument() throws SAXException {
        TraceUtil.trace2("Done parsing version highlight XML file.");
    }

    public void startElement(
        String namespaceURI,
        String simpleName,
        String qualifiedName,
        Attributes attrs) throws SAXException {

        print(new NonSyncStringBuffer("StartElement: qualifiedName is ").
            append(qualifiedName).toString());

        if (qualifiedName.equals("feature")) {
            if (attrs != null && attrs.getValue(0) != null) {
                featureType = attrs.getValue(0);
            } else {
                featureType = "";
            }

            // Start of a feature, re-initialize all fields
            featureName = "";
            versionNumber = "";
            versionStatus = "";
            serverVersion = "";
            versionInfo = new NonSyncStringBuffer();
            highlightInfo = null;
            nowServing = -1;

        } else if (qualifiedName.equals("feature-name")) {
            nowServing = 0;
        } else if (qualifiedName.equals("version-number")) {
            nowServing = 1;
        } else if (qualifiedName.equals("version-status")) {
            nowServing = 2;
        } else if (qualifiedName.equals("server-version")) {
            nowServing = 3;
        }
    }

    public void endElement(
        String namespaceURI,
        String simpleName,
        String qualifiedName) throws SAXException {

        print("endElement is called!");

        // reset nowServing
        nowServing = -1;

        if (qualifiedName.equals("feature")) {

            featureType = (featureType == null) ? "" : featureType.trim();
            featureName = (featureName == null) ? "" : featureName.trim();
            serverVersion = (serverVersion == null) ? "" : serverVersion.trim();

            // Save the Highlight Info
            highlightInfo =
                new HighlightInfo(
                    featureType,
                    featureName,
                    serverVersion,
                    versionInfo.toString());

            // Push into the HashMap
            myHashMap.put(new Integer(key++), highlightInfo);
        } else if (qualifiedName.equals("version")) {
            if (versionInfo.length() != 0) {
                versionInfo.append("###");
            }
            versionInfo.append(versionNumber).append(",").append(versionStatus);
        }
    }

    public void characters(
        char buf[], int offset, int length) throws SAXException {

        String content = new String(buf, offset, length).trim();

        print(new NonSyncStringBuffer("Characters: nowServing is ").
            append(nowServing).toString());
        print(new NonSyncStringBuffer("Characters: content is ").
            append(content).toString());

        switch (nowServing) {

            // Feature Name
            case 0:
                featureName = content;
                break;

            // Version Number
            case 1:
                versionNumber = content;
                break;

            // Version Status
            case 2:
                versionStatus = content;
                break;

            // Minimum Support Server Version
            case 3:
                serverVersion = content;
                break;

            default:
                break;
        }
    }

    private void print(String printMessage) {
        TraceUtil.trace3(printMessage);
    }

    public static HashMap getHashMap() {
        return myHashMap;
    }
}
