# $Revision: 1.6 $
#    SAM-QFS_notice_begin
#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License (the "License").
# You may not use this file except in compliance with the License.
#
# You can obtain a copy of the license at pkg/OPENSOLARIS.LICENSE
# or http://www.opensolaris.org/os/licensing.
# See the License for the specific language governing permissions
# and limitations under the License.
#
# When distributing Covered Code, include this CDDL HEADER in each
# file and include the License file at pkg/OPENSOLARIS.LICENSE.
# If applicable, add the following below this CDDL HEADER, with the
# fields enclosed by brackets "[]" replaced with your own identifying
# information: Portions Copyright [yyyy] [name of copyright owner]
#
# CDDL HEADER END
#
# Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
#    SAM-QFS_notice_end

DEPTH = ../../..
LIB_HOME = $(DEPTH)/src/lib/csn

include $(DEPTH)/mk/common.mk

LIB = csn
LIB_SRC = \
	scrkd_curl.c \
	scrkd_xml_string.c \
	scrkd_xml.c \
	scrkd_lib.c 


CURL_INCLUDES = $(DEPTH)/src/lib/curl/V7/7.15.4/include

SSL_CONF_INCLUDES = $(DEPTH)/src/lib/ssl/V9/9.7/$(DB_ARCH)/include
SSL_INCLUDES = $(DEPTH)/src/lib/ssl/V9/9.7/include

LIB_INCLUDES = $(DEPTH)/include/

DEPCFLAGS = -DAPI_DEBUG=syslog -DDEBUG \
		-I. -I$(CURL_INCLUDES) -I$(SSL_CONF_INCLUDES) -I$(SSL_INCLUDES) -I$(LIB_INCLUDES) $(THRCOMP)


LIB_LIBS = -L$(DEPTH)/lib/$(OBJ_DIR) -lsamut $(STATIC_OPT) -lcrypto -lssl \
	-lcurl $(DYNAMIC_OPT) -lsocket -ldl $(LIBSO)


LNOPTS = -a -u -m -x
LNLIBS = -L$(DEPTH)/lib/$(OBJ_DIR)

# use the following for LNOPTS to get more detail from lint
#LNOPTS =  $(DEPCFLAGS) -m -x -errchk=%all -errfmt=macro -errsecurity=extended -fd -errtags=yes -erroff=E_SEC_PRINTF_VAR_FMT,E_OLD_STYLE_FUNC_DECL,E_FUNC_RET_ALWAYS_IGNOR,E_H_C_CHECK0,E_H_C_CHECK1,E_H_C_CHECK2,E_SEC_CREAT_WITHOUT_EXCL -Nlevel=4 -Ncheck=%all




include $(DEPTH)/mk/targets.mk

include $(DEPTH)/mk/depend.mk
