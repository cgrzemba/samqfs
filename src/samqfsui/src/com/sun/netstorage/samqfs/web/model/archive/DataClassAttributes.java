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

// ident	$Id: DataClassAttributes.java,v 1.5 2008/12/16 00:12:17 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.archive;

import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSUtil;
import com.sun.netstorage.samqfs.web.util.ConversionUtil;
import java.text.FieldPosition;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Properties;

public class DataClassAttributes {
    // class attribue key symbolic constants
    public static final String CLASS_ID = "classid";
    public static final String AUTO_WORM = "autoworm";
    public static final String ABS_EXP_TIME = "absolute_expiration_time";
    public static final String REL_EXP_TIME = "relative_expiration_time";
    public static final String AUTO_DELETE = "autodelete";
    public static final String DEDUP = "dedup";
    public static final String BITBYBIT = "bitbybit";
    public static final String PERIODIC_AUDIT = "periodicaudit";
    public static final String AUDIT_PERIOD = "auditperiod";
    public static final String LOG_DATA_AUDIT = "log_data_audic";
    public static final String LOG_DEDUP = "log_deduplication";
    public static final String LOG_AUTO_WORM = "log_autoworm";
    public static final String LOG_AUTO_DELETE = "log_autodelete";

    // default attributes. useful when creating a new dataclass
    public static final String DEFAULTS = new StringBuffer("autoworm=N,")
        .append("autodelete=N,dedup=N,bitbybit=N,periodicaudit=none,")
        .append("log_data_audit=N,log_dedup=N,log_autoworm=N,log_autodelete=N")
        .toString();

    // member variables
    private int classId;
    private boolean autoworm;
    private Date absoluteExpirationTime;
    private long relativeExpirationTime;
    private int relativeExpirationTimeUnit;
    private boolean autoDelete;
    private boolean dedup;
    private boolean bitbybit;
    private PeriodicAudit periodicAudit;
    private long auditPeriod;
    private int auditPeriodUnit;
    private boolean logDataAudit;
    private boolean logDeduplication;
    private boolean logAutoWorm;
    private boolean logAutoDeletion;
    private boolean absoluteExpiration;
    private String rawClassAttributes;

    /** default constructor */
    public DataClassAttributes() {
        this(DEFAULTS);
    }

    /**
     * constructor
     */
    public DataClassAttributes(String attrs) {
        this.rawClassAttributes = attrs;

        decode();
    }

    /**
     * convert the instant variable values to a string of the form
     * attribute1=value1,attribute2=value to be passed to the lower tiers
     */
    private String encode() {
        String e = "=", c = ",";
        StringBuffer buffer = new StringBuffer();

        // class id
        if (this.classId != -1) {
            buffer.append(CLASS_ID).append(e)
                  .append(Integer.toString(this.classId));
        }

        // autoworm
        String temp = this.autoworm ? "Y" : "N";
        buffer.append(c).append(AUTO_WORM).append(e).append(temp);

        // expiration time
        if (this.absoluteExpiration) { // absolute expiration time
            SimpleDateFormat dateFormat = new SimpleDateFormat("yyyyMMddHHmm");
            StringBuffer buf = new StringBuffer();
            buf = dateFormat.format(this.absoluteExpirationTime,
                                    buf,
                                    new FieldPosition(0));
            buffer.append(c).append(ABS_EXP_TIME)
                .append(e).append(buf.toString());
        } else { // relative expiration time
            if (relativeExpirationTime != -1) {
            buffer.append(c).append(REL_EXP_TIME)
                .append(e).append(Long.toString(this.relativeExpirationTime))
                .append(SamQFSUtil.getTimeUnitString(
                     this.relativeExpirationTimeUnit));
            }
        }

        // auto delete
        temp = this.autoDelete ? "Y" : "N";
        buffer.append(c).append(AUTO_DELETE).append(e).append(temp);

        // dedup
        temp = this.dedup ? "Y" : "N";
        buffer.append(c).append(DEDUP).append(e).append(temp);

        // bit by bit
        temp = this.bitbybit ? "Y" : "N";
        buffer.append(c).append(BITBYBIT).append(e).append(temp);

        // periodic audit
        buffer.append(c).append(PERIODIC_AUDIT).append(e)
            .append(this.periodicAudit.getStringValue());

        // audit period
        if (this.auditPeriod != -1) {
            buffer.append(c).append(AUDIT_PERIOD).append(e)
                .append(Long.toString(this.auditPeriod))
                .append(SamQFSUtil.getTimeUnitString(this.auditPeriodUnit));
        }

        // logging
        temp = this.logDataAudit ? "Y" : "N";
        buffer.append(c).append(LOG_DATA_AUDIT).append(e).append(temp);

        temp = this.logDeduplication ? "Y" : "N";
        buffer.append(c).append(LOG_DEDUP).append(e).append(temp);

        temp = this.logAutoWorm ? "Y" : "N";
        buffer.append(c).append(LOG_AUTO_WORM).append(e).append(temp);

        temp = this.logAutoDeletion ? "Y" : "N";
        buffer.append(c).append(LOG_AUTO_DELETE).append(e).append(temp);

        return buffer.toString();
    }

    /**
     * convert the comma separated value pair string provided by the c-api.
     */
    private void decode() {
        try {
            Properties props =
                ConversionUtil.strToProps(this.rawClassAttributes);

            // class id
            String temp = props.getProperty(CLASS_ID);
            if (temp != null) {
                this.classId = Integer.parseInt(temp.trim());
            }

            // auto worm
            temp = props.getProperty(AUTO_WORM);
            temp = temp != null ? temp.trim() : "";
            this.autoworm = temp.equals("Y") ? true : false;

            // expiration time
            temp = props.getProperty(ABS_EXP_TIME);
            if (temp != null) {
                this.absoluteExpirationTime =
                    new SimpleDateFormat("yyyyMMddHHmm").parse(temp.trim());

                this.absoluteExpiration = true;
            }

            temp = props.getProperty(REL_EXP_TIME);
            if (temp != null) {
                this.relativeExpirationTime =
                    SamQFSUtil.getLongValSecond(temp.trim());
                this.relativeExpirationTimeUnit =
                    SamQFSUtil.getTimeUnitInteger(temp.trim());

                this.absoluteExpiration = false;
            }

            // auto delete
            temp = props.getProperty(AUTO_DELETE);
            temp = temp != null ? temp.trim() : "";
            this.autoDelete = temp.equals("Y") ? true : false;

            // dedup
            temp = props.getProperty(DEDUP);
            temp = temp != null ? temp.trim() : "";
            this.dedup = temp.equals("Y") ? true : false;

            // bit by bit
            temp = props.getProperty(BITBYBIT);
            temp = temp != null ? temp.trim() : "";
            this.bitbybit = temp.equals("Y") ? true : false;

            // periodic audit
            temp = props.getProperty(PERIODIC_AUDIT);
            temp = temp != null ? temp.trim() : "";
            if (temp.equals(PeriodicAudit.DISK.getStringValue())) {
                this.periodicAudit = PeriodicAudit.DISK;
            } else if (temp.equals(PeriodicAudit.ALL.getStringValue())) {
                this.periodicAudit = PeriodicAudit.ALL;
            } else { // TODO: same as NONE? check with PG
                this.periodicAudit = PeriodicAudit.NONE;
            }

            // audit period
            temp = props.getProperty(AUDIT_PERIOD);
            temp = temp != null ? temp.trim() : "";
            this.auditPeriod = SamQFSUtil.getLongValSecond(temp);
            this.auditPeriodUnit = SamQFSUtil.getTimeUnitInteger(temp);

            // loggging
            // data audit
            temp = props.getProperty(LOG_DATA_AUDIT);
            temp = temp != null ? temp.trim() : "";
            this.logDataAudit = temp.equals("Y") ? true : false;

            // deduplication
            temp = props.getProperty(LOG_DEDUP);
            temp = temp != null ? temp.trim() : "";
            this.logDeduplication = temp.equals("Y") ? true : false;

            // auto worm
            temp = props.getProperty(LOG_AUTO_WORM);
            temp = temp != null ? temp.trim() : "";
            this.logAutoWorm = temp.equals("Y") ? true : false;

            // auto deletion
            temp = props.getProperty(LOG_AUTO_DELETE);
            temp = temp != null ? temp.trim() : "";
            this.logAutoDeletion = temp.equals("Y") ? true : false;
        } catch (Exception e) {
            // NOTE: These are most number format exceptions which we can't
            // do much about anyway. Drop the value in such cases
        }
    }

    public String toString() {
        return encode();
    }

    // setters
    public void setClassId(int classid) {this.classId = classid; }
    public void setAutoWormEnabled(boolean aw) {this.autoworm = aw; }
    public void setAbsoluteExpirationTime(Date exp) {
        this.absoluteExpirationTime = exp;
    }
    public void setRelativeExpirationTime(long exp) {
        this.relativeExpirationTime = exp;
    }
    public void setRelativeExpirationTimeUnit(int unit) {
        this.relativeExpirationTimeUnit = unit;
    }
    public void setAutoDeleteEnabled(boolean b) {this.autoDelete = b; }
    public void setDedupEnabled(boolean b) {this.dedup = b; }
    public void setBitbybitEnabled(boolean b) {this.bitbybit = b; }
    public void setPeriodicAudit(PeriodicAudit p) {this.periodicAudit = p; }
    public void setAuditPeriod(long l) {this.auditPeriod = l; }
    public void setAuditPeriodUnit(int u) {this.auditPeriodUnit = u; }
    public void setLogDataAuditEnabled(boolean b) {this.logDataAudit = b; }
    public void setLogDeduplicationEnabled(boolean b) {
        this.logDeduplication = b;
    }
    public void setLogAutoWormEnabled(boolean b) {this.logAutoWorm = b; }
    public void setLogAutoDeletionEnabled(boolean b) {
        this.logAutoDeletion = b;
    };
    public void setAbsoluteExpirationEnabled(boolean b) {
        this.absoluteExpiration = b;
    }


    // getters
    public boolean isAbsoluteExpirationTime() {
        return absoluteExpiration;
    }
    public int getClassId() {return this.classId; }
    public boolean isAutoWormEnabled() {return this.autoworm; }
    public Date getAbsoluteExpirationTime() {
        return this.absoluteExpirationTime;
    }
    public long getRelativeExpirationTime() {
        return this.relativeExpirationTime;
    }
    public int getRelativeExpirationTimeUnit() {
        return this.relativeExpirationTimeUnit;
    }
    public boolean isAutoDeleteEnabled() {return this.autoDelete; }
    public boolean isDedupEnabled() {return this.dedup; }
    public boolean isBitbybitEnabled() {return this.bitbybit; }
    public PeriodicAudit getPeriodicAudit() {return this.periodicAudit; }
    public long getAuditPeriod() {return this.auditPeriod; }
    public int getAuditPeriodUnit() {return this.auditPeriodUnit; }
    public boolean isLogDataAuditEnabled() {return this.logDataAudit; }
    public boolean isLogDeduplicationEnabled() {return this.logDeduplication; }
    public boolean isLogAutoWormEnabled() {return this.logAutoWorm; }
    public boolean isLogAutoDeletionEnabled() {return this.logAutoDeletion; }
}
