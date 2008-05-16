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

// ident	$Id: Metric.java,v 1.10 2008/05/16 18:39:00 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.fs;

import com.iplanet.jato.RequestManager;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.util.XmlConvertor;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.util.Properties;
import javax.servlet.ServletContext;
import javax.xml.transform.TransformerException;
import org.apache.batik.transcoder.TranscoderException;

public class Metric {

    // fs metric types - should be in sync with defn in file_metrics_report.h
    public static final int TYPE_FILESBYAGE    = 0;
    public static final int TYPE_FILESBYLIFE   = 1;
    public static final int TYPE_FILESBYOWNER  = 2;
    public static final int TYPE_FILESBYGROUP  = 3;
    public static final int TYPE_STORAGETIER   = 4;

    private int type = 0;
    private String jpgFileName = "/samqfsui/xsl/svg/sample.jpg";
    private String imageMapString = "";
    private long startTime;
    private long endTime;
    private String rawXml;

    public static int convType2Int(String type) {
        if (type.equals("storagetier")) {
            return TYPE_STORAGETIER;
        } else if (type.equals("filebylife")) {
            return TYPE_FILESBYLIFE;
        } else if (type.equals("filebyage")) {
            return TYPE_FILESBYAGE;
        } else if (type.equals("filebyowner")) {
            return TYPE_FILESBYOWNER;
        } else if (type.equals("filebygroup")) {
            return TYPE_FILESBYGROUP;
        } else {
            return TYPE_STORAGETIER; // default
        }
    }

    public static String convType2Str(int type) {
        switch (type) {
            case TYPE_STORAGETIER: return ("storagetier");
            case TYPE_FILESBYLIFE: return ("filebylife");
            case TYPE_FILESBYAGE: return ("filebyage");
            case TYPE_FILESBYOWNER: return ("filebyowner");
            case TYPE_FILESBYGROUP: return ("filebygroup");
            default: return "";
        }
    }

    public Metric(String rawXml, int type) {
        this.rawXml = rawXml;
        this.type = type;

        ServletContext sc =
            RequestManager.getRequestContext().getServletContext();

        String xmlFileName = new StringBuffer().append("/tmp/xml").toString();
        try {
            FileWriter in =
                new FileWriter(new File(sc.getRealPath(xmlFileName)));
            in.write(rawXml);
            in.close();
        } catch (IOException ioe) {
            ioe.printStackTrace();
        }


    }

    public Metric(String rawXml, int type, long startTime, long endTime) {
        this(rawXml, type);
        this.startTime = startTime;
        this.endTime = endTime;

    }

    public void createJpg(String jpgFileName, String xslFileName)
        throws SamFSException {

        Properties xslParams = new Properties();
        xslParams.setProperty("startTime", String.valueOf(startTime));
        xslParams.setProperty("endTime", String.valueOf(endTime));

        // populate the SVG and imagemap
        try {
            TraceUtil.trace3("start converting xml to svg");
            String str = XmlConvertor.convert2Svg(rawXml,
                                                  xslFileName,
                                                  xslParams);

            TraceUtil.trace3("done converting xml to svg");

            String [] strArr = str.split("<!-- imageMapSection -->");
            String svgStr = strArr[0];
            if (strArr[1] != null) {
                this.imageMapString = strArr[1];
            }
            TraceUtil.trace3("start converting svg to jpeg");
            XmlConvertor.convertSvg2Jpeg(svgStr, jpgFileName);
            TraceUtil.trace3("done converting svg to jpeg");

        } catch (IOException ioEx) {
            throw new SamFSException(ioEx.getLocalizedMessage());
        } catch (TranscoderException tcEx) {
            throw new SamFSException(tcEx.getLocalizedMessage());
        } catch (TransformerException trEx) {
            throw new SamFSException(trEx.getLocalizedMessage());
        }

    }

    public int getType() { return this.type; }
    public String getJpgFileName() { return this.jpgFileName; }
    public String getImageMapString() { return this.imageMapString; }

}
