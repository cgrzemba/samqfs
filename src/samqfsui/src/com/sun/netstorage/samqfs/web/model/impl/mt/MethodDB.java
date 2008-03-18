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

// ident	$Id: MethodDB.java,v 1.7 2008/03/17 14:43:50 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.mt;

import java.util.Hashtable;
import java.util.Vector;
import java.util.Iterator;
import java.lang.reflect.Method;

/**
 * @see    MethodInfo
 * This class encapsultes info about all the methods which
 * can be executed in a ThreadPoolMember thread.
 */
public class MethodDB {

    /**
     *  this hashtable stores all methods keyed by the method name;
     *  if two methods have the same name, the value will be a Vector
     *  of Method objects, otherwise it is just one Method object.
     */
    protected static Hashtable methods;

    /** initialize (populate) the DB */
    public static void init(String[] interfaces)
        throws ClassNotFoundException {
        methods = new Hashtable();
        Method[] methodSet;
        for (int i = 0; i < interfaces.length; i++) {
            methodSet = Class.forName(interfaces[i]).getDeclaredMethods();
            for (int j = 0; j < methodSet.length; j++)
                addMethod(methodSet[j]);
        }
    }

    public static Method getMethod(String methodName, Object[] args)
    throws NoSuchMethodException {
        if (args == null) args = new Object[] {};
        Method met = null;
        if (methods.containsKey(methodName)) {
            Object val = methods.get(methodName);
            if (val instanceof Method)
                met = (Method) val;
            else if ((met = matchMethod(args, (Vector)val)) == null)
                throw new NoSuchMethodException(methodName);
        } else
            throw new NoSuchMethodException(methodName);
        return met;
    }


    // ------------------- PROTECTED METHODS ------------------------

    protected static void addMethod(Method newMet) {
        String name = newMet.getName();
        if (methods.containsKey(name)) {
            Object val = methods.get(name);
            if (val instanceof Vector)
                ((Vector) val).add(newMet);
            else {
                Vector vec = new Vector();
                vec.add(val);
                vec.add(newMet);
                methods.remove(name);
                methods.put(name, vec);
            }
        } else
            methods.put(name, newMet);
    }


    // args is non-null; if no arguments, args has length = 0.
    protected static Method matchMethod(Object[] args, Vector mets) {
        Method method = null;
        boolean found = false;
        Class[] paramTypes; // retrieved from the DB
        Class c;
        int i;
        Iterator methods = mets.iterator();
        while ((!found) && methods.hasNext()) {
            method = (Method) methods.next();
            paramTypes = method.getParameterTypes();
            if (args.length != paramTypes.length) // missmatch
                continue;
            found = true;
            for (i = 0; (i < paramTypes.length) && found; i++)
                if (paramTypes[i].isPrimitive()) {
                try {
                    // get the underlying primitive type for args[i]
                    c = (Class)
                    args[i].getClass().getField("TYPE").get(null);
                    // check if same as paramTypes[i]
                    found = c.isAssignableFrom(paramTypes[i]);
                } catch (Exception e) {
                    // args[i] is not a wrapper class for a primitive type
                    found = false;
                }
                } else
                    if (!(paramTypes[i].isInstance(args[i])))
                        found = false;
            if ((i == 0) && (args.length != 0)) found = false;
        }
        if (found)
            return method;
        else
            return null;
    }


    // ------------------ testing -----------------------------------

    public static void dumpAll() {
        java.util.Enumeration keys = methods.keys();
        System.out.println("\n*** known methods:\n" +
            "method name                full signature\n" +
            "---------------------------------------------");
        while (keys.hasMoreElements()) {
            String key = (String) keys.nextElement();
            Object val = methods.get(key);
            if (val instanceof Method)
                System.out.println(key + "\t - " + (Method) val);
            else { Vector vec = (Vector) val;
            for (int i = 0; i < vec.size(); i++)
                System.out.println(key + "\t > " + (Method) vec.elementAt(i));
            }
        }
    }

    public static void main(String args[]) {
        try {
            init(new String[] {"java.util.Collection",
            "com.sun.netstorage.sve.util.ThreadListener"});
        } catch (ClassNotFoundException cnfe) {
            System.out.println("MetDB: " + cnfe);
        }
        dumpAll();

        try {
            System.out.println("\n*** Looking for toArray() ...");
            Method m1 = getMethod("toArray", null);
            System.out.println("Method found: " + m1);
            System.out.println("\n*** Looking for toArray(Object[]) ...");
            Method m2 = getMethod("toArray",
                new Object[] {new String[] {"o1", "o2"}});
            System.out.println("Method found: " + m2);
        } catch (NoSuchMethodException nsme) { System.out.println(nsme); }
    }

}
