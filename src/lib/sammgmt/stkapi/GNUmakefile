# $Revision: 1.13 $
#    SAM-QFS_notice_begin
#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License (the "License").
# You may not use this file except in compliance with the License.
#
# You can obtain a copy of the license at pkg/OPENSOLARIS.LICENSE
# or https://illumos.org/license/CDDL.
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

DEPTH = ../../../..
LIB_HOME = ../../../src/lib/sammgmt/stkapi

include $(DEPTH)/mk/common.mk

LIB = stkapi 
LIB_SRC = stk.c stk_wrap.c

DEPCFLAGS = -DAPI_DEBUG=syslog -DDEBUG $(METADATA_SERVER) \
		-I. -I.. -I$(INCLUDE) \
		-I$(DEPTH)/src/robots/vendor_supplied/stk/src/h $(THRCOMP)

LIB_LIBS = -L $(DEPTH)/lib/$(OBJ_DIR) -lapi $(LIBSO)/stk

LNOPTS = -a -u -m -x -shared
LNLIBS = -L $(DEPTH)/lib/$(OBJ_DIR)

# use the following for LNOPTS to see much more information from lint
BIGLNOPTS =  -m -x -errchk=%all -errfmt=macro -errsecurity=extended -fd -errtags=yes -erroff=E_SEC_PRINTF_VAR_FMT,E_OLD_STYLE_FUNC_DECL,E_FUNC_RET_ALWAYS_IGNOR -Nlevel=4


include $(DEPTH)/mk/targets.mk

include $(DEPTH)/mk/depend.mk
