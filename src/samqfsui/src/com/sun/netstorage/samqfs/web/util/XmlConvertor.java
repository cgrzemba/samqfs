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

// ident	$Id: XmlConvertor.java,v 1.12 2008/03/17 14:43:57 am143972 Exp $

package com.sun.netstorage.samqfs.web.util;

import com.iplanet.jato.RequestManager;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.StringReader;
import java.util.Enumeration;
import java.util.Properties;
import javax.servlet.ServletContext;
import javax.servlet.ServletException;
import javax.xml.transform.Result;
import javax.xml.transform.Source;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerException;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.URIResolver;
import javax.xml.transform.sax.SAXResult;
import javax.xml.transform.stream.StreamResult;
import javax.xml.transform.stream.StreamSource;
import org.apache.batik.transcoder.TranscoderException;
import org.apache.batik.transcoder.TranscoderInput;
import org.apache.batik.transcoder.TranscoderOutput;
import org.apache.batik.transcoder.image.JPEGTranscoder;
import org.apache.fop.apps.Driver;

/**
 * XMLConvertor is a utility class that converts XML data to XHTML, SVG, PDF
 * The stylesheet is provided as input
 *
 * This uses the Apache Batik 1.6 open source technology to convert SVG to JPEG
 * URL for the open source technology:- http://xmlgraphics.apache.org/batik/
 * License for the open source technology:-
 * Apache License, Version 2.0, January 2004
 * See Review #: 5148 (https://opensourcereview.east.sun.com/)
 * Review filed by Kelly Silverthorne (Program Manager)
 */
public class XmlConvertor {

    /* Renders a JPEG image from an SVG document */
    public static void convertSvg2Jpeg(
        File svgFile,
        String jpegDest) throws TranscoderException, IOException {

        // create the transcoder input
        String svgURI = svgFile.toURL().toString();
        TranscoderInput input = new TranscoderInput(svgURI);
        convertSvg2Jpeg(input, jpegDest);
    }

    /* Renders a JPEG image from an SVG document */
    public static void convertSvg2Jpeg(
        String svgString,
        String jpegDest) throws TranscoderException, IOException {

        // Write the string to a file
        ServletContext sc =
            RequestManager.getRequestContext().getServletContext();

        String svgFileName =
            new StringBuffer().append("/tmp/svg.svg").toString();
        File inFile = new File(sc.getRealPath(svgFileName));
        FileWriter in = new FileWriter(inFile);
        in.write(svgString);
        in.close();

        // InputStream svgInputStream = sc.getResourceAsStream(svgFileName);
        String svgURI =
            new File(sc.getRealPath(svgFileName)).toURL().toString();
        TranscoderInput input = new TranscoderInput(svgURI);

        // create the transcoder input
        // StringReader reader = new StringReader(svgString);
        // TranscoderInput input = new TranscoderInput(svgInputStream);

        convertSvg2Jpeg(input, jpegDest);
    }

    /* Renders a JPEG image from an SVG TranscoderInput */
    private static void convertSvg2Jpeg(
        TranscoderInput svgInput,
        String jpegDest) throws TranscoderException, IOException {

        // create a JPEG transcoder
        JPEGTranscoder t = new JPEGTranscoder();

        // Inorder to support distributions with Java 1.4 or Java 1.5, and
        // support the respective XML parsers, we have to provide the parser
        // class names via transcoder hints. This is due to the implementation
        // limitation in Batik 1.6 XMLReaderFactory. See CR 6498640.
        //
        // In java.specification.version=1.5, Parser =
        // com.sun.org.apache.xerces.internal.jaxp.SAXParserImpl
        // In java.specification.version=1.4, Parser =
        // org.apache.crimson.jaxp.SAXParserImpl
        String jVersion = System.getProperty("java.specification.version");
        String parserClassName = "org.apache.crimson.jaxp.SAXParser"; // default
        if (jVersion != null) {
            if (jVersion.equalsIgnoreCase("1.5")) {
                parserClassName =
                    "com.sun.org.apache.xerces.internal.parsers.SAXParser";
            }
        }
        t.addTranscodingHint(
            JPEGTranscoder.KEY_XML_PARSER_CLASSNAME,
            parserClassName);

        // set the transcoding hints
        t.addTranscodingHint(JPEGTranscoder.KEY_QUALITY, new Float(.8));
        // t.addTranscodingHint(JPEGTranscoder.KEY_HEIGHT, new Float(700.0));
        // create the transcoder output
        jpegDest = RequestManager.getRequestContext()
            .getServletContext().getRealPath(jpegDest);
        OutputStream ostream = new FileOutputStream(jpegDest);

        TranscoderOutput output = new TranscoderOutput(ostream);
        // save the image
        t.transcode(svgInput, output);
        // flush and close the stream then exit
        ostream.flush();
        ostream.close();
    }


    public static String convert2Svg(
        String xmlString,
        String xslFile,
        Properties xslParams) throws TransformerException, IOException {
        ServletContext sc =
            RequestManager.getRequestContext().getServletContext();

        return transform(new StreamSource(new StringReader(xmlString)),
                         new StreamSource(sc.getResourceAsStream(xslFile)),
                         xslParams);
    }

    /**
     * Renders an XML file into XHTML by applying a stylesheet
     * Use Xalan to apply stylesheet to the XML data by way of applying TraX
     *
     * @param xmlString  XML input string
     * @param xslString  XSLT input string
     *
     * @throws TransformerException IOException
     */
    public static String convert2Xhtml(
        String xmlString,
        File xslFile) throws TransformerException, IOException {

        return (transform(new StreamSource(new StringReader(xmlString)),
            new StreamSource(xslFile)));
    }

    public static String convert2Xhtml(String xmlString, InputStream is)
        throws TransformerException, IOException {

        return (transform(new StreamSource(new StringReader(xmlString)),
                          new StreamSource(is)));
    }

    /**
     * Renders an XML file into XHTML by applying a stylesheet
     * Use Xalan to apply stylesheet to the XML data by way of applying TraX
     *
     * @param xmlFile name of XML input file
     * @param xslFile name of XSLT input file
     *
     * @throws TransformerException IOException
     */
    public static String convert2Xhtml(
        File xmlFile,
        File xslFile) throws TransformerException, IOException {

        return (transform(new StreamSource(xmlFile),
            new StreamSource(xslFile)));
    }

    public static String convert2Xhtml(InputStream xml, InputStream xsl)
        throws TransformerException, IOException {
        return transform(new StreamSource(xml), new StreamSource(xsl));
    }

    /**
     * Renders an XML file into a PDF file by applying a stylesheet
     * that converts the XML to XSL-FO.  The PDF is saved to a file.
     *
     * @param xmlString  XML input string
     * @param xslString  XSLT input string
     * @param pdfDest    destination file name
     *
     * @throws ServletException if a servlet-related error occurs
     *
     * First, the XML is fed to an XSLT processor with an appropriate stylesheet
     * in order to produce another XML document which uses the XSL-FO namespace
     * and is intended for an XSL-FO formatter.
     * The second stage is to feed the output of the first stage to the XSL-FO
     * formatter, which can then produce the end product: a printable document,
     * styled for visual presentation.
     */
    public static void convert2Pdf(
        String xmlString,
        File xslFile,
        String pdfDest) throws TransformerException, IOException {

        generatePdf(new StreamSource(new StringReader(xmlString)),
            new StreamSource(xslFile),
            new File(pdfDest));

    }

    /**
     * Renders an XML file into a PDF file by applying a stylesheet
     * that converts the XML to XSL-FO.  The PDF is saved to a file.
     *
     * @param xmlString  XML input string
     * @param xslString  XSLT input string
     * @param pdfDest    destination file name
     *
     * @throws ServletException if a servlet-related error occurs
     *
     * First, the XML is fed to an XSLT processor with an appropriate stylesheet
     * in order to produce another XML document which uses the XSL-FO namespace
     * and is intended for an XSL-FO formatter.
     * The second stage is to feed the output of the first stage to the XSL-FO
     * formatter, which can then produce the end product: a printable document,
     * styled for visual presentation.
     */
    public static void convert2Pdf(
        File xmlFile,
        File xslFile,
        String pdfDest) throws TransformerException, IOException {

        generatePdf(new StreamSource(xmlFile),
            new StreamSource(xslFile),
            new File(pdfDest));

    }

    private static void generatePdf(
        StreamSource xmlSource,
        StreamSource xslSource,
        File pdfFile) throws TransformerException, IOException {

        Driver driver = new Driver();
        // configure renderer
        driver.setRenderer(Driver.RENDER_PDF);
        // Step 1: Convert the XML to XSL-FO
        OutputStream out;
        // Save the fo in a file
        out = new java.io.FileOutputStream(pdfFile);
        driver.setOutputStream(out);
        // Setup XSLT
        TransformerFactory factory = TransformerFactory.newInstance();
        Transformer transformer = factory.newTransformer(xslSource);

        // Resulting SAX events (the generated FO) must be piped through to FOP
        Result res = new SAXResult(driver.getContentHandler());

        // Start XSLT transformation and FOP processing
        transformer.transform(xmlSource, res);
    }

    /**
     * Use Xalan to apply stylesheet to the XML data by way of applying TraX
     * no arguments to XSL
     */
    private static String transform(
        StreamSource xmlSource,
        StreamSource xslSource) throws TransformerException {

        return (transform(xmlSource, xslSource, null /* no args to xsl */));

    }
    /**
     * Use Xalan to apply stylesheet to the XML data by way of applying TraX
     * pass input arguments to stylesheet via Properties
     */
    private static String transform(
        StreamSource xmlSource,
        StreamSource xslSource,
        Properties xslParams) throws TransformerException {

        URIResolver resolver = new URIResolverImpl();
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        StreamResult streamResult = new StreamResult(out);
        TransformerFactory tfactory = TransformerFactory.newInstance();
        tfactory.setURIResolver(resolver);
        Transformer transformer = tfactory.newTransformer(xslSource);
        transformer.setURIResolver(resolver);
        // passing parameters to XSLT, the param has to be declared in the XSLT
        // arguments to the XSLT are via properties
        if (xslParams != null) {
            Enumeration e = xslParams.propertyNames();
            while (e.hasMoreElements()) {
                String key = (String)e.nextElement();
                transformer.setParameter(key, xslParams.getProperty(key));
            }
        }
        transformer.transform(xmlSource, streamResult);

        return new String(out.toByteArray());
    }

}

class URIResolverImpl implements URIResolver {
    public Source resolve(String href, String base)
        throws TransformerException {

        ServletContext sc =
            RequestManager.getRequestContext().getServletContext();
        StreamSource source = new
            StreamSource(sc.getResourceAsStream(href));
        return source;
    }
}
