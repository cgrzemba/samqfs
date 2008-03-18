/*
 *
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

// ident	$Id: SAXFilter.java,v 1.4 2008/03/17 14:43:54 am143972 Exp $

package com.sun.netstorage.samqfs.web.server;

import org.xml.sax.SAXException;
import org.xml.sax.helpers.XMLFilterImpl;
import org.xml.sax.Attributes;
import java.util.Stack;

/**
 * This filter sits between the input source (xml reader) and the application
 * handler. It intercepts multiple character events for the same element
 * content and merges them to a single event which it passes on to the
 * SAXHandler for processing. As a secondary function, it also eliminates
 * whitespace character events to prevent them from reaching the handler where
 * they would cause problems.
 */
public  class SAXFilter extends XMLFilterImpl {
    private Stack stack;

    public SAXFilter() {
        super();
    }

    /** instantiate the stack */
    public void startDocument() throws SAXException {
        stack = new Stack();

        super.startDocument();
    }

    /** start of element */
    public void startElement(String uri,
                             String localName,
                             String qName,
                             Attributes attributes)
        throws SAXException {
        // mark the beginning of the element and create an instance of
        // StringBuffer which will be used to merge multiple character envents
        // values to a single character array.
        stack.push(new StringBuffer());

        super.startElement(uri, localName, qName, attributes);
    }

    /** end of element */
    public void endElement(String uri,
                           String localName,
                           String qName)
        throws SAXException {
        // we've reached the end of the element, build a character array
        // and call the next filter's or handler's characters method to
        // process it.
        StringBuffer buf = (StringBuffer)stack.pop();
        if (buf.length() != 0) {
            char [] buffer = buf.toString().toCharArray();
            super.characters(buffer, 0, buffer.length);
        }

        super.endElement(uri, localName, qName);
    }

    /**
     * characters() can be called more than one time with different
     * fragments of the same element body content which in our case can lead to
     * partial version keys. To resolve this, we'll intercept the characters()
     * event and merge all the fragments of the same body content together,
     * then call the handler's character method with the merged character array.
     */
    public void characters(char [] buffer, int start, int length)
        throws SAXException {
        String content = new String(buffer, start, length).trim();

        if (content.length() != 0) {
            StringBuffer buf = (StringBuffer)stack.peek();
            buf.append(content);
        }
    }
}
