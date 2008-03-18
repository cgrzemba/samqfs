
#    SAM-QFS_notice_begin
#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License (the "License").
# You may not use this file except in compliance with the License.
#
# You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
# or http://www.opensolaris.org/os/licensing.
# See the License for the specific language governing permissions
# and limitations under the License.
#
# When distributing Covered Code, include this CDDL HEADER in each
# file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

BEGIN {
	FS = "|";
	MsgGrp = 0;
}
{
if ($1 == "")  continue;
if ($1 ~ /^\$/) continue;
i = int($1 / 1000);
if (i != MsgGrp) {
	if (MsgGrp != 0)  print "};";
	MsgGrp = i;
	print "static char *" $2 "Msgs[] = {";
	print " { \"\" },";
	GN[MsgGrp] = $2;
	Gnum[MsgGrp] = 1;
	nextin = (i * 1000) + 1;
} else {
	while (nextin++ < $1) { print " { NULL },"; Gnum[MsgGrp]++; }
	print " { \"" $2 "\" },"
	nextin = $1 + 1;
	Gnum[MsgGrp]++;
}
}
END {
	print "};";
	print "struct MsgsTable DevlogMsgsTable[] = {"
	print " { 0, NULL },"
	for (i = 1; i < MsgGrp + 1; i++)  print " { "Gnum[i] ", " GN[i] "Msgs },"
	print "};"
	print "int DevlogMsgsTableNum = " MsgGrp + 1 ";";
}
