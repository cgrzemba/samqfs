# mkhc.awk prepares an '*.hc' file from a '*.h' file.
# The .hc file has the table and data for processing by setfield.c.
# It processes lines in the file between the following statements:
# #ifdef SETFIELD_DEFS
# #endif
#
# 1.  It copies '#include' lines to the output.
# 2.  The line #define STRUCT struct_name table_name identifies the structure
#     and table name for setfield
# 3.  For the other lines, see setfield.h
#
# See "sam/mount.h" for an example.
#
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

BEGIN {
	pname = "mkhc.awk"
	if (ARGC < 2) {
		errors = -1
		print "usage: " pname " file.h [file.hc] [-v setfield_const=\"string\"]"
		exit(2)
	}
	errors = 1
	first = 1
	if (ARGC < 3) {
		out = ARGV[1] "c"
	} else {
		out = ARGV[2]
	}
	if (setfield_const == NULL) {
		setfield_const = "SETFIELD_DEFS"
	}
	skip = 1
}
{
# Do nothing until the text to be processed is reached.
	if ($1 == "#ifdef" && $2 == setfield_const) {
		def_name = ""
		entnum = 0
		errors = 0
		skip = 0
		if (first) {
			first = 0
			printf("/* %s - Do not edit this file.  It is produced from %s */\n",
				out, ARGV[1]) > out
			printf("#include <limits.h>\n") >> out
			printf("#include \"sam/types.h\"\n") >> out
			printf("#include \"sam/setfield.h\"\n") >> out
			next
		}
	}
	if ($1 == "#endif" || $2 == "STRUCT") {
		if ($1 == "#endif") {
			skip = 1
		}
		if (entnum != 0) {
			# print the control table.
			printf("static struct fieldVals %s[] = {\n", ref_name) >> out
			for (i = 0; i < entnum; i++) {
				printf(" { \"%s\", %s, offsetof(struct %s, %s), &%s%d, %s },\n",
					Tname[i], Ttype[i], struct, Tloc[i], ref_name, i,
					TdefFlag[i]) >> out
			}
			printf(" { NULL }};\n") >> out
			entnum = 0
		}
	}
	if (skip)  next


	if ($1 == "#include") {
		print $0 >> out
		next
	}

	if ($2 == "STRUCT") {
		ref_name = struct = $3
		if ($4 != "")  ref_name = $4
		next
	}

	if ($1 == "")  next

	n = split($2, a, "=")
	if (n == 1) {
		Tname[entnum] = $2
		Tloc[entnum] = $2
	}
	else if (n == 2) {
		# name=field form - field in struct has a different name
		Tname[entnum] = a[1]
		Tloc[entnum] = a[2]
	}
	else {
		print $0
		errors++
		next
	}
	Ttype[entnum] = $1
	def = split(Tloc[entnum], a, "+")
	nodupchk = split(Tloc[entnum], b, "-")
	if (def == 2 || nodupchk == 2) {
		# Setting the name of the 'defined' flag
		if (nodupchk == 2) {
			a[1] = b[1]
			a[2] = b[2]
			Ttype[entnum] = Ttype[entnum] "| NO_DUP_CHK"
		}
		TdefFlag[entnum] = a[2]
		Tloc[entnum] = a[1]
	}
	else {
		TdefFlag[entnum] = ""
	}
	if ($6 == "")  $6 = "0"

	if ($1 == "CLEARFLAG") {
		Tloc[entnum] = $3
		printf("static struct fieldFlag %s%d = { (uint32_t)%s, \"%s\", \"%s\", \"%s\" };\n",
			ref_name, entnum, $4, $5, $6, $7) >> out
		}
	else if ($1 == "DEFBITS") {
		printf("static int %s%d;\n", ref_name, entnum) >> out
		Tname[entnum] = "";
		}
	else if ($1 == "DOUBLE") {
		if ($4 == "")  $4 = "DBL_MIN"
		if ($5 == "")  $5 = "DBL_MAX"
		
		printf("static struct fieldDouble %s%d = { %s, %s, %s, %s };\n",
			ref_name, entnum, $3, $4, $5, $6) >> out
		}
	else if ($1 == "ENUM") {
		printf("static struct fieldEnum %s%d = { \"%s\", %s };\n",
			ref_name, entnum, $3, $4) >> out
		}
	else if ($1 == "FLAG") {
		Tloc[entnum] = $3
		printf("static struct fieldFlag %s%d = { (uint32_t)%s, \"%s\", \"%s\", \"%s\" };\n",
			ref_name, entnum, $4, $5, $6, $7) >> out
		}
	else if ($1 == "FLOAT") {
		if ($4 == "")  $4 = "FLT_MIN"
		if ($5 == "")  $5 = "FLT_MAX"
		printf("static struct fieldDouble %s%d = { %s, %s, %s, %s };\n",
			ref_name, entnum, $3, $4, $5, $6) >> out
		}
	else if ($1 == "FSIZE") {
		printf("static struct fieldInt %s%d = { %s, 0, LLONG_MAX, 0 };\n",
			ref_name, entnum, $3) >> out
		}
	else if ($1 == "FUNC") {
		printf("static struct fieldFunc %s%d = { %s, %s, %s, %s };\n",
			ref_name, entnum, $3, $4, $5, $6) >> out
		}
	else if ($1 == "INT" || $1 == "MUL8" || $1 == "PWR2") {
		if ($4 == "")  $4 = "INT_MIN"
		if ($5 == "")  $5 = "INT_MAX"
		printf("static struct fieldInt %s%d = { %s, %s, %s, %s };\n",
			ref_name, entnum, $3, $4, $5, $6) >> out
		}
	else if ($1 == "INT16") {
		if ($4 == "")  $4 = "SHRT_MIN"
		if ($5 == "")  $5 = "SHRT_MAX"
		printf("static struct fieldInt %s%d = { %s, %s, %s, %s };\n",
			ref_name, entnum, $3, $4, $5, $6) >> out
		}
	else if ($1 == "INT64" || $1 == "MULL8") {
		if ($4 == "")  $4 = "LLONG_MIN"
		if ($5 == "")  $5 = "LLONG_MAX"
		printf("static struct fieldInt %s%d = { %s, %s, %s, %s };\n",
			ref_name, entnum, $3, $4, $5, $6) >> out
		}
	else if ($1 == "MEDIA") {
		printf("static struct fieldMedia %s%d = { \"%s\" };\n",
			ref_name, entnum, $3) >> out
		}
	else if ($1 == "SETFLAG") {
		Tloc[entnum] = $3
		printf("static struct fieldFlag %s%d = { (uint32_t)%s, \"%s\", \"%s\", \"%s\" };\n",
			ref_name, entnum, $4, $5, $6, $7) >> out
		}
	else if ($1 == "STRING") {
		if ($3 == "\"\"")  $3 = ""
		printf("static struct fieldString %s%d = { \"%s\", %s };\n",
			ref_name, entnum, $3, $4) >> out
		}
	else if ($1 == "INTERVAL") {
		if ($4 == "")  $4 = "0"
		if ($5 == "")  $5 = "INT_MAX"
		printf("static struct fieldInt %s%d = { %s, %s, %s, %s };\n",
			ref_name, entnum, $3, $4, $5, $6) >> out
		}

	else  next

	entnum++
}

END {
	if (errors != 0) {
		if (errors > 0) {
			printf("%s: %d errors\n", pname, errors)
			exit(1)
		}
	}
}
