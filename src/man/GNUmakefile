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

DEPTH = ../..

include $(DEPTH)/mk/common.mk

DIRS = 	man1 \
		man1m \
		man3 \
		man3x \
		man4 \
		man5 \
		man7

include $(DEPTH)/mk/targets_man.mk

pstar:	pstar-top
pstar_mig: pstar_mig-top
pstar_qfs: pstar_qfs-top
pstar_rem: pstar_rem-top

pstar-top:
	/bin/rm -f ./ps.tar ./relsrc.list
	/bin/touch ./ps.tar
	/bin/tar chf ./ps.tar `/bin/cat ./relsrc.list`

pstar_mig-top:
	/bin/rm -f ./migps.tar ./migrelsrc.list
	/bin/touch ./migps.tar
	/bin/tar chf ./migps.tar `/bin/cat ./migrelsrc.list`

pstar_qfs-top:
	/bin/rm -f ./qfsps.tar ./qfsrelsrc.list
	/bin/touch ./qfsps.tar
	/bin/tar chf ./qfsps.tar `/bin/cat ./qfsrelsrc.list`

pstar_rem-top:
	/bin/rm -f ./remps.tar ./remrelsrc.list
	/bin/touch ./remps.tar
	/bin/tar chf ./remps.tar `/bin/cat ./remrelsrc.list`

include $(DEPTH)/mk/depend.mk
