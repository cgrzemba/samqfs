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

// ident	$Id: SamWizardManager.java,v 1.9 2008/05/16 18:39:07 am143972 Exp $

package com.sun.netstorage.samqfs.web.wizard;

import java.util.Hashtable;

public class SamWizardManager {

    protected static Hashtable wizardControlMap = new Hashtable();

    protected SamWizardManager() {
    }

    public static int getNewWizardID(String wizardType, String hostName) {
        int id;

        synchronized (wizardControlMap) {
            Hashtable controlTable = (Hashtable)
                wizardControlMap.get(wizardType);
            if (controlTable == null) {
                controlTable = new Hashtable();
                wizardControlMap.put(wizardType, controlTable);
            }

            SamWizardControlData data = (SamWizardControlData)
                controlTable.get(hostName);
            if (data == null) {
                data = new SamWizardControlData();
            }

            id = data.wizardID++;
            controlTable.put(hostName, data);
        }

        return id;
    }

    public static int validateWizardID(
        String wizardType, String hostName, int wizID) {

        int result;

        synchronized (wizardControlMap) {
            Hashtable controlTable = (Hashtable)
                wizardControlMap.get(wizardType);
            if (controlTable == null) {
                controlTable = new Hashtable();
                wizardControlMap.put(wizardType, controlTable);
            }

            SamWizardControlData data = (SamWizardControlData)
                controlTable.get(hostName);
            if (data == null) {
                data = new SamWizardControlData();
            }

            if (wizID < data.activeWizardID) {
                result = -1;
            } else if (wizID > data.activeWizardID) {
                result = 1;
            } else {
                result = 0;
            }
        }

        return result;
    }

    public static int updateActiveWizardID(
        String wizardType, String hostName, int wizID, boolean finishStep) {

        int result;

        synchronized (wizardControlMap) {
            Hashtable controlTable = (Hashtable)
                wizardControlMap.get(wizardType);
            if (controlTable == null) {
                controlTable = new Hashtable();
                wizardControlMap.put(wizardType, controlTable);
            }

            SamWizardControlData data = (SamWizardControlData)
                controlTable.get(hostName);
            if (data == null) {
                data = new SamWizardControlData();
            }

            if (wizID < data.activeWizardID) {
                result = -1;
            } else if (wizID > data.activeWizardID) {
                result = 1;
                if (!finishStep) {
                    data.activeWizardID = wizID;
                }
            } else { // this is the active wizard
                result = 0;
                // make next wizard active
                if (finishStep) {
                    data.activeWizardID = data.wizardID;
                }
            }

            wizardControlMap.put(hostName, data);
        }

        return result;
    }
}
