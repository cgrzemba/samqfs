# $Revision: 1.11 $

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

include $(DEPTH)/mk/common.mk

PROG = bld_codes
PROG_SRC = bld_codes.c

LNOPTS = -a -Dlint 
LNLIBS =

include $(DEPTH)/mk/targets.mk

.NOTPARALLEL:

all: 	gen_buildcodes

gen_buildcodes:
	cd $(OBJ_DIR); \
	rm -rf errcodes; \
	./$(PROG)

clean:	clean_more
clean_more:
	@rm -rf errcodes

install: install_more
install_more: 
	$(INSTALL) -d -o $(OWNER) -g $(GROUP) $(VARDEST)/errcodes
	$(INSTALL) -o $(OWNER) -g $(GROUP) errcodes/acl2640  $(VARDEST)/errcodes/acl2640 
	$(INSTALL) -o $(OWNER) -g $(GROUP) errcodes/acl452   $(VARDEST)/errcodes/acl452 
	$(INSTALL) -o $(OWNER) -g $(GROUP) errcodes/adic448  $(VARDEST)/errcodes/adic448 
	$(INSTALL) -o $(OWNER) -g $(GROUP) errcodes/cyg1803  $(VARDEST)/errcodes/cyg1803 
	$(INSTALL) -o $(OWNER) -g $(GROUP) errcodes/dlt2700  $(VARDEST)/errcodes/dlt2700 
	$(INSTALL) -o $(OWNER) -g $(GROUP) errcodes/docstor  $(VARDEST)/errcodes/docstor 
	$(INSTALL) -o $(OWNER) -g $(GROUP) errcodes/exb210   $(VARDEST)/errcodes/exb210 
	$(INSTALL) -o $(OWNER) -g $(GROUP) errcodes/hpoplib  $(VARDEST)/errcodes/hpoplib 
	$(INSTALL) -o $(OWNER) -g $(GROUP) errcodes/ibm3570c $(VARDEST)/errcodes/ibm3570c
	$(INSTALL) -o $(OWNER) -g $(GROUP) errcodes/metd28   $(VARDEST)/errcodes/metd28 
	$(INSTALL) -o $(OWNER) -g $(GROUP) errcodes/metd360  $(VARDEST)/errcodes/metd360 
	$(INSTALL) -o $(OWNER) -g $(GROUP) errcodes/plasmond $(VARDEST)/errcodes/plasmond
	$(INSTALL) -o $(OWNER) -g $(GROUP) errcodes/plasmong $(VARDEST)/errcodes/plasmong
	$(INSTALL) -o $(OWNER) -g $(GROUP) errcodes/sonydms  $(VARDEST)/errcodes/sonydms
	$(INSTALL) -o $(OWNER) -g $(GROUP) errcodes/sonycsm  $(VARDEST)/errcodes/sonycsm
	$(INSTALL) -o $(OWNER) -g $(GROUP) errcodes/speclog  $(VARDEST)/errcodes/speclog 
	$(INSTALL) -o $(OWNER) -g $(GROUP) errcodes/stk97xx  $(VARDEST)/errcodes/stk97xx 

include $(DEPTH)/mk/depend.mk
