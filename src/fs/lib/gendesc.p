#!/bin/perl
#
# $Revision: 1.6 $
#
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


$err = 0;

%typelist = &map_init('types.swap');
%defslist = &map_init('types.define');

&defs_decl_print(%defslist);
&defs_list_print(%defslist);

while ( <STDIN> ) {
#	print "i: $_";

	#
	# skip comments and blank lines
	#
	s/#.*//;
	s/^\s*$//;
	if ( /^$/ ) {
		next;
	}

	#
	# look for 'typedef struct <word> {'
	#
	if ( /^typedef\s+struct\s+(\w+)\s+{\s+$/ ) {
		if ( ! exists $defslist{${1} . "_t"}
				|| $defslist{${1} . "_t"} ne 'DEFINE' ) {
			next;
		}

		#
		# a <word> we care about (found in types.define)
		#
		print "START_DESC(${1})\n";
		$typedef = $1;
		while ( <STDIN> ) {
			($type, $name, $count) = ("", "", "");

			#
			# Skip comments, remove whitespace
			# from beginning and end of line,
			# and purge blank lines.
			#
			s/#.*//;
			s/^\s+//;
			s/\s+$//;
			s/\s+/ /g;
			if ( /^$/ ) {
				next;
			}
			s/\s+;/;/;

#			print "(b)line = '$_' ('$typedef')\n";

			# Is this the end of the typedef declaration?
			if ( /^}\s*${typedef}_t\s*;$/ ) {
				print "END_DESC(${typedef}_t);\n\n";
				last;
			}

			# parse the type into $type $name [$count]
			($fulltype, $typename, $name, $count) = ("", "", "", "");

			if ( /^(((struct )|(union )){0,1}(\w+)) (\w+)(\s*\[(.+)]){0,1};$/ ) {
#				   (((       ) (      ))     (   )) (   )(     (   ))
#
#				1: (                                  )
#				2:  (                  )
#				3:   (       )
#				4:		       (      )
#				5:		                     (   )
#				6:		                            (   )
#				7:		                                 (          )
#				8:		                                       (   )
#
				($fulltype, $typename, $name, $count) = ($1, $5, $6, $8);
				if ( ! defined $count) {
					$count = 1;
				}
			} else {
				print "unmatched line: $_";
				$err = 1;
			}
			if ($typelist{$typename} eq 'ATOMIC') {
				print "	ATOMIC(${typedef}_t, $fulltype, $name, $count),\n";
			} elsif ($typelist{$typename} eq 'REMAP') {
				print "	REMAP(${typedef}_t, $fulltype, $name, $count),\n";
			} elsif ($typelist{$typename} eq 'PRIMITIVE') {
				print "	PRIMITIVE(${typedef}_t, $fulltype, $name, $count),\n";
			} elsif ($typelist{$typename} eq 'OBJECT') {
				$d = &desc_name($typename);
# print "typename = ${typename}, d = $d\n";
				print "	OBJECT(${typedef}_t, $fulltype, $d, $name, $count),\n";
			} else {
				print "\nERROR -- unrecognized type '$typename';";
				print "1 = '$1', 2 = '$2', 3 = '$3', 4 = '$4', 5 = '$5', 6 = '$6', 7 = '$7', 8 = '$8'\n\n";
				$err = 1;
			}
		}
	}
}

$err && print "fatal parsing error\n";

#
# Initialize an associative array from the file name passed
# as the argument.  Syntax is:
#
# type = X
#	foo
#	bar
# type = Y
#	xyzzy
#
# which will initialize the associative array to:
# ( (foo, X) (bar, X) (xyzzy, Y) )
#
# Actually, this one initializes it to
# ( (foo, X) (foo_t, X) (bar, X) (bar_t, X) (xyzzy, Y) (xyzzy_t, Y) )
# so that we can look up by either the structure name or the typedef'd
# type.
#
sub map_init {
	local($name) = @_;
	local($type);
	local(%typelist);

	open(FD, $name) || die "can't open $name\n";

	while (<FD>) {
		s/#.*$//;
		if (/^s*type\s*=\s*(\w+)\s*$/) {
			$type = $1;
			next;
		}
		if (/^\s*(\w+)\s*$/) {
			$typelist{$1} = $type;
			$typelist{$1 . "_t"} = $type;
			next;
		}
		if (/^\s*$/) {
			next;
		}
		$err = 1;
		print "Unrecognized directive: $_\n";
	}
	close(FD);

	$err && die "syntax error(s) in $name\n";
	%typelist;
}

#
# Convert a type name to the type of its object descriptor
#
sub desc_name {
	local($key) = @_;
	local($name);

	$name = $key;
	$name =~ s/_t$//;
	$name =~ s/$/_descriptor/;
	$name;
}

#
# Print out a list declaring (in C) all the relevant types.
#
sub defs_decl_print {
	local(%defslist) = @_;
	local($key);

	foreach $key (keys %defslist) {
		if ( $key =~ /_t$/ ) {
			next;
		}
		print "extern struct element_descriptor ${key}_descriptor[];\n";
	}
}

#
# Print out an array of all the relevant type descriptors,
# sorted by name.
#
sub defs_list_print {
	local(%defslist) = @_;
	local(@names);
	local($key);

	@names = keys(%defslist);
	@names = sort(@names);
	print "\n\nSTARTLIST\n";
	foreach $key (@names) {
		if ( $key =~ /_t$/ ) {
			next;
		}
		print "\tLIST(\"${key}\", ${key}_descriptor, sizeof(${key}_t)),\n";
	}
	print "ENDLIST\n\n\n";
}

sub map_print {
	local(%name) = @_;
	local($key);

	foreach $key (keys %name) {
		print "'$key' : '$name{$key}'\n";
	}
	print "\n\n";
}
