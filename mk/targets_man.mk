# $Revision: 1.8 $

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
# Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
#    SAM-QFS_notice_end
#
#	targets_man.mk - manual/docs target definitions
#
#	The following variables can be defined in a Makefile:
#
#		MAN_SRCS		  	manual source files
#		MAN_RELEASED_SRCS	manual source files targeted for release
#		QFS_RELEASED_SRCS	qfs manual source files targeted for release
#		MIG_RELEASED_SRCS	mig manual source files targeted for release
#		REM_RELEASED_SRCS	rem manual source files targeted for release
#
###############################################################################

STRIP_DIRS = $(strip $(DIRS))
ifneq ($(STRIP_DIRS),)
.PHONY: $(STRIP_DIRS)

$(STRIP_DIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)
endif

.INIT:

all:

clean:	$(STRIP_DIRS)
	-@rm -f *.ps

clobber: clean
clobberall: clobber

depend:

install:	$(STRIP_DIRS)
ifneq ($(MAN_SRCS),)
	@ for f in $(MAN_SRCS); do 									\
		echo $$f; 											\
		$(INSTALL) $(DATAINST) $$f $(MANDEST)/$(DIR)/$$f; 	\
	done
endif

lint:

print_all:	$(STRIP_DIRS)
ifneq ($(MAN_SRCS),)
	@ for f in $(MAN_SRCS); do 							\
		troff -man $$f | dpost -m .95 | lp -o nb; 	\
	done
endif

print:	$(STRIP_DIRS)
ifneq ($(MAN_RELEASED_SRCS),)
	@ banner User Commands | lp -o nb
	@ for f in $(MAN_RELEASED_SRCS); do 				\
		troff -man $$f | dpost -m .95 | lp -o nb; 	\
	done
endif

print_mig:	$(STRIP_DIRS)
ifneq ($(MIG_RELEASED_SRCS),)
	@ banner User Commands | lp -o nb
	@ for f in $(MIG_RELEASED_SRCS); do 				\
		troff -man $$f | dpost -m .95 | lp -o nb; 	\
	done
endif

print_qfs:	$(STRIP_DIRS)
ifneq ($(QFS_RELEASED_SRCS),)
	@ banner User Commands | lp -o nb
	@ for f in $(QFS_RELEASED_SRCS); do 				\
		troff -man $$f | dpost -m .95 | lp -o nb; 	\
	done
endif

print_rem:	$(STRIP_DIRS)
ifneq ($(REM_RELEASED_SRCS),)
	@ banner Admin Commands | lp -o nb
	@ for f in $(REM_RELEASED_SRCS); do                     \
		tbl $$f | troff -man | dpost -m .95 | lp -o nb;     \
	done
endif

ps:		$(STRIP_DIRS)
ifneq ($(MAN_RELEASED_SRCS),)
	@ for f in $(MAN_RELEASED_SRCS); do \
		troff -man $$f | dpost -m .95 > $$f.ps; \
	done
endif

ps_mig:	$(STRIP_DIRS)
ifneq ($(MIG_RELEASED_SRCS),)
	@ for f in $(MIG_RELEASED_SRCS); do \
		troff -man $$f | dpost -m .95 > $$f.ps; \
	done
endif

ps_qfs:	$(STRIP_DIRS)
ifneq ($(QFS_RELEASED_SRCS),)
	@ for f in $(QFS_RELEASED_SRCS); do \
		troff -man $$f | dpost -m .95 > $$f.ps; \
	done
endif

ps_rem:	$(STRIP_DIRS)
ifneq ($(REM_RELEASED_SRCS),)
	@ for f in $(REM_RELEASED_SRCS); do \
		troff -man $$f | dpost -m .95 > $$f.ps; \
	done
endif

pstar: $(STRIP_DIRS) ps
ifneq ($(MAN_RELEASED_SRCS),)
	@ for f in $(MAN_RELEASED_SRCS); do         \
		echo man1/$$f.ps >> ../relsrc.list; \
	done
endif

pstar_mig:	$(STRIP_DIRS)
ifneq ($(MIG_RELEASED_SRCS),)
	@ for f in $(MIG_RELEASED_SRCS); do         \
		echo man1/$$f.ps >> ../relsrc.list; \
	done
endif

pstar_qfs: $(STRIP_DIRS) ps_qfs
ifneq ($(QFS_RELEASED_SRCS),)
	@ for f in $(QFS_RELEASED_SRCS); do         \
		echo man1/$$f.ps >> ../qfsrelsrc.list; \
	done
endif

pstar_rem:	$(STRIP_DIRS) ps_rem
ifneq ($(REM_RELEASED_SRCS),)
	@ for f in $(REM_RELEASED_SRCS); do         \
		echo man1/$$f.ps >> ../qfsrelsrc.list; \
	done
endif

