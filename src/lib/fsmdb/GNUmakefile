# $Revision: 1.9 $
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
LIB_HOME = $(DEPTH)/src/lib/fsmdb

include $(DEPTH)/mk/common.mk

LIB = fsmdb
LIB_SRC = \
	fsmdb_api.c \
	xlate.c

DEPCFLAGS = -I. -I$(INCLUDE) $(THRCOMP) -DDEBUG -D_REENTRANT

LIB_LIBS += 	-L$(DEPTH)/lib/$(OBJ_DIR) -ldoor -lgen -lrt -lnsl $(LIBSO) 

LNOPTS = -a -u -m -x
LNLIBS = -L$(DEPTH)/lib/$(OBJ_DIR)

# use the following for LNOPTS to get more detail from lint
#LNOPTS =  $(DEPCFLAGS) -m -x -errchk=%all -errfmt=macro -errsecurity=extended -fd -errtags=yes -erroff=E_SEC_PRINTF_VAR_FMT,E_OLD_STYLE_FUNC_DECL,E_FUNC_RET_ALWAYS_IGNOR,E_H_C_CHECK0,E_H_C_CHECK1,E_H_C_CHECK2,E_SEC_CREAT_WITHOUT_EXCL -Nlevel=4 -Ncheck=%all


include $(DEPTH)/mk/targets.mk

include $(DEPTH)/mk/depend.mk
