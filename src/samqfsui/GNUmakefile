# $Id: GNUmakefile,v 1.12 2008/12/16 00:10:34 am143972 Exp $

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

DEPTH = ../..

include $(DEPTH)/mk/common.mk

include $(DEPTH)/mk/targets.mk

all: samqfsui

samqfsui:
	if [ z$(MAKECMDGOALS) != zclean -a z$(MAKECMDGOALS) != zclobber -a z$(MAKECMDGOALS) != zclobberall ]; then ( JAVA_HOME=$(JBASE) ; export JAVA_HOME; $(ANT) -Dobj.dir=$(OBJ_DIR) -Djni.lib.dir=$(DEPTH)/lib/$(OBJ_DIR) dist ) fi

include $(DEPTH)/mk/depend.mk
