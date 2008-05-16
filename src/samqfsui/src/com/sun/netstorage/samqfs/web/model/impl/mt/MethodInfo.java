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

// ident	$Id: MethodInfo.java,v 1.8 2008/05/16 18:39:03 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.mt;

import java.lang.reflect.Method;
import java.lang.reflect.InvocationTargetException;

/**
 * @see    ThreadPoolMember, ThreadPool, ThreadListener
 * This class encapsultes info about a method to be executed
 * in a ThreadPoolMember thread.
 */
public class MethodInfo {

    Method method;
    Object instance;
    Object[] args;

    public MethodInfo(String metName, Object instance, Object[] args)
        throws NoSuchMethodException {
        method = MethodDB.getMethod(metName, args);
        this.instance = instance;
        this.args = args;
    }

    public MethodInfo(MethodInfo mdi) {
        this.method = mdi.method;
        this.instance = mdi.instance;
        this.args = new Object[mdi.args.length];
        for (int i = 0; i < mdi.args.length; i++)
            this.args[i] = mdi.args[i];
    }

    /**
     * sets the instance on which this method will be called.
     */
    public void setInstance(Object o) {
        instance = o;
    }

    public void setArgs(Object[] o) {
        args = o;
    }

    public Object execute() throws IllegalAccessException,
                     IllegalArgumentException,
                     InvocationTargetException {

        StringBuffer sb = new StringBuffer("MethodInfo::execute() invoking ");
        sb.append(method.getName());
        sb.append(" on target ");
        sb.append(instance);

        return method.invoke(instance, args);
    }


    // ------------------ testing -----------------------------------
    public static void main(String args[]) {
        // add all methods defined in the java.io.PrintStream class
        try {
        MethodDB.init(new String[] { "java.io.PrintStream" });
        MethodDB.dumpAll();
        } catch (ClassNotFoundException cnfe) {
            System.out.println("MI Exception0: " + cnfe); System.exit(-1);
        }

        // now execute a println method on a PrintStream object (System.out)
        MethodInfo mi = null;
        try {
            mi = new MethodInfo("println",
                                System.out,
                                new Object[] {"testing MethodInfo class..."});

        } catch (Exception e1) {
            System.out.println("MI Exception1: " + e1); }
        try {
            System.out.println("*** executing ***");
            mi.execute();
        } catch (Exception e) { System.out.println("MI Exception2: " + e); }
    }

}
