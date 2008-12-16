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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

// ident $Id: ProductRegistrationInfo.java,v 1.7 2008/12/16 00:12:16 am143972 Exp $

package com.sun.netstorage.samqfs.web.model;

import java.util.Properties;

import com.sun.netstorage.samqfs.web.util.ConversionUtil;
import com.sun.netstorage.samqfs.mgmt.SamFSException;

/**
 * this class encapsulates information on the Product (SAM-QFS) registration
 */
public class ProductRegistrationInfo {

    // constant below must match those defined in the server-side code
    final String KEY_SUNLOGIN  = "sun_login";
    final String KEY_NAME  = "name";
    final String KEY_EMAIL = "email";
    final String KEY_ASSET_PREFIX = "asset_prefix"; // = sam-qfs;
    final String KEY_REGISTERED   = "registered"; // = Y/N
    final String KEY_PROXY_ENABLED  = "prxy_enabled"; // = Y/N
    final String KEY_PROXY_PORT = "prxy_port";
    final String KEY_PROXY_HOST = "prxy_host";
    final String KEY_PROXY_AUTH = "prxy_auth"; // = Y/N
    final String KEY_PROXY_USER = "prxy_user";

    // clientPassword and authPassword are not stored

    protected String login, name, emailAddr, proxyHost, proxyUser;
    protected int proxyPort;
    protected boolean registered, proxyEnabled, proxyAuth;

    /** constructors, must call register to actually register the product */
    public ProductRegistrationInfo(
        String login, String name, String emailAddress) {
        setSunLogin(login);
        setName(name);
        setEmailAddress(emailAddress);
        this.registered = true;
    }

    public ProductRegistrationInfo(
        String login, String name, String emailAddress,
        String proxyHost, int proxyPort) {
        this(login, name, emailAddress);
        this.proxyEnabled = true;
        setProxyHostname(proxyHost);
        setProxyPort(proxyPort);
    }

    public ProductRegistrationInfo(
        String login, String name, String emailAddress,
        String proxyHost, int proxyPort,
        String authUser) {
        this(login, name, emailAddress, proxyHost, proxyPort);
        this.proxyAuth = true;
        setProxyUser(authUser);
    }

    public ProductRegistrationInfo(String registrationStr)
        throws SamFSException {
        this(ConversionUtil.strToProps(registrationStr));
    }

    protected ProductRegistrationInfo(Properties props) throws SamFSException {
        decode(props);
    }

    public void decode(Properties props) throws SamFSException {
        if (props == null)
            return;

        login = props.getProperty(KEY_SUNLOGIN);
        name  = props.getProperty(KEY_NAME);
        emailAddr  = props.getProperty(KEY_EMAIL);
        registered =
            "Y".equalsIgnoreCase(props.getProperty(KEY_REGISTERED)) ?
                true : false;
        proxyEnabled =
            "Y".equalsIgnoreCase(props.getProperty(KEY_PROXY_ENABLED)) ?
                true : false;
        proxyPort   =
            ConversionUtil.strToIntVal(props.getProperty(KEY_PROXY_PORT));
        proxyHost   = props.getProperty(KEY_PROXY_HOST);
        proxyAuth   =
            "Y".equalsIgnoreCase(props.getProperty(KEY_PROXY_AUTH)) ?
                true : false;
        proxyUser   = props.getProperty(KEY_PROXY_USER);

    }

    public String encode() {
        StringBuffer buffer = new StringBuffer();
        String e = "=", c = ",";
        buffer.append(KEY_SUNLOGIN).append(e).append(login);
        buffer.append(c).append(KEY_NAME).append(e).append(name);
        buffer.append(c).append(KEY_EMAIL).append(e).append(emailAddr);
        buffer.append(c).append(KEY_ASSET_PREFIX).append(e).append("sam-qfs");

        String reg = registered ? "Y" : "N";
        buffer.append(c).append(KEY_REGISTERED).append(e).append(reg);

        String proxyEnabledAsStr = proxyEnabled ? "Y" : "N";
        buffer.append(c).append(KEY_PROXY_ENABLED).
                append(e).append(proxyEnabledAsStr);

        if (proxyEnabled) {
            buffer.append(c).append(KEY_PROXY_PORT).append(e).append(proxyPort);
            buffer.append(c).append(KEY_PROXY_HOST).append(e).append(proxyHost);
        }

        String proxyAuthAsStr = proxyAuth ? "Y" : "N";
        buffer.append(c).append(KEY_PROXY_AUTH).
                append(e).append(proxyAuthAsStr);
        if (proxyAuth) {
            buffer.append(c).append(KEY_PROXY_USER).append(e).append(proxyUser);
        }

        return buffer.toString();
    }

    public String toString() {
        return encode();
    }

    // getters
    public String getSunLogin()  { return login; }
    public String getName() { return name; }
    public String getEmailAddress() { return emailAddr; }
    public boolean isRegistered() { return registered; }
    public boolean isProxyEnabled() { return proxyEnabled; }
    public int getProxyPort() { return proxyPort; }
    public String getProxyHostname() { return proxyHost; }
    public boolean isProxyAuth() { return proxyAuth; }
    public String getProxyUser() { return proxyUser; }

    // private setters, used by constructor
    private void setSunLogin(String login) { this.login = login; }
    private void setName(String name) { this.name = name; }
    private void setEmailAddress(String addr) { this.emailAddr = addr; }
    private void setProxyPort(int port) { this.proxyPort = port; }
    private void setProxyHostname(String host)  { this.proxyHost = host; }
    private void setProxyUser(String proxyUser) { this.proxyUser = proxyUser; }
}
