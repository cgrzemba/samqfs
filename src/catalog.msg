$ *      SAM-FS catalog.msg
$ *
$ *    SAM-QFS_notice_begin
$ *
$ * CDDL HEADER START
$ *
$ * The contents of this file are subject to the terms of the
$ * Common Development and Distribution License (the "License")
$ * You may not use this file except in compliance with the License.
$ *
$ * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
$ * or http://www.opensolaris.org/os/licensing.
$ * See the License for the specific language governing permissions
$ * and limitations under the License.
$ *
$ * When distributing Covered Code, include this CDDL HEADER in each
$ * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
$ * If applicable, add the following below this CDDL HEADER, with the
$ * fields enclosed by brackets "[]" replaced with your own identifying
$ * information: Portions Copyright [yyyy] [name of copyright owner]
$ *
$ * CDDL HEADER END
$ *
$ * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
$ * Use is subject to license terms.
$ *
$ *    SAM-QFS_notice_end
$ *
$ pragma ident "$Revision: 1.585 $"

$ *************************************************************************
$ *		Modification guidelines
$ *
$ *		It is good practice to not reuse message numbers, rather use a new
$ *		message and mark the old message as "** NOT USED **", while retaining
$ *		the old text. Cleanups of the "NOT USED" should be part of a release
$ *		process.
$ *
$ *		Making identical changes to the message catalog on the intended and
$ *		forward (future) release trees can prevent numbering conflicts from
$ *		occurring.
$ *
$ *		Do not include comment lines consisting of a single "$".  Linux
$ *		considers this an error.  Either leave the whole line blank or
$ *		use "$ *".
$ *
$ *		Hyphens can be used to skip message text. If the first character of
$ *		a message is a "-", all the text up to and including leading spaces
$ *		is skipped. Example (all the spaces before "this text" are also
$ *		skipped):
$ *		-thistextisskipped       this text is not
$ *
$ * The following comments also appear before message 18000.
$ The messages in the range 18000 - 18020 are sent via SendCustMsg()
$ In addition to these, there are other messages which have been identified as
$ events for which a SNMP trap is to be thrown. They are also sent using
$ SendCustMsg()
$ They may be preceded by an optional delivery code string that begins with
$ '-'.  The letters may be in any order and may be one of the following:
$     N - Send message to notify executable file.
$     S - Send message to syslog
$     V - Send trap notification.

$ The first letter of the syslog priority codes -
$     Emerg, alert, crit, err, warning, notice, info, debug
$ The default action is: -Si
$ Example: -SNw  sends a "warning" to both syslog and the notify executable.
$ With the 'V' option, the class and subclass information for that event
$ is given as -wV[Class:Subclass]
$ e.g.
$ 4010 -wVN[Archiver:NoVolAvail] No volumes available for Archive Set %s
$ A trap notification would be sent; class= Archiver, Subclass= NoVolAvail
$ *************************************************************************

$   set assignment:     set 1       All SAM-FS source code messages
$                       set 2-256   Not-assigned
$set 1
$quote "


$  Catalog Index
$  ---------------------
$  old          (1-3016)
$  archiver     (4000s)
$  cmd          (5000s)
$  gui          (6000s)
$  samu         (7000s)
$  releaser     (8000s)
$  robots       (9000s)
$  rpc          (10000s)
$  sam-amld     (11000s)
$  lib-samfs    (12000s)
$  utility      (13000s)
$  lib-sam      (14000s)
$  lib-rm-media (15000s)
$  lib-misc     (16000s)
$  samlogd      (17000s)
$  sam-fsd      (17200s)
$  catalog      (18000s)
$  stager       (19000s)
$  misc         (20000s)
$  d2format		(21000s)
$  rft/remote	(22000s)
$  mount_samfs	(23000s)


$  Old Messages  (1-3016)
$  ===================================================================
2 "    more"
28 %#x blocks transferred
73 %c: Unrecognized option.
76 %d bytes written.\n
77 %d does not support moves.\n
79 %d is not a robotic device.\n
80 %d is not a valid equipment ordinal.\n
81 %d is not tape device.\n
89 %s allocation failed on (%s)
91 %s dau map read failed on (%s)
96 %s is not an .inodes file
100 %s may be run only by super-user.\n
101 %s mode\n
102 %s may be set only by super-user.\n
103 %s read failed on (%s)
109 %s write failed on (%s)
111 %s(%d): badly formatted media.
113 %s(%d): capacity check failed.
114 %s(%d): did not become ready.
116 %s(%d): Existing labels not ANSI.
117 %s(%d): existing volid %s does not match new label %s. Use the same \
volid or specify the -erase option.
118 %s(%d): Format data not valid
119 %s(%d): format failed.
120 %s(%d): Volume is not formatted
122 %s(%d): not a d2.
125 %s(%d): state not valid.
127 %s(%d): Unable to re-open drive:%m.
128 %s(%d): unable to determine position.
129 %s(%d): Unable to erase.
131 ** NOT USED ** %s(%d): Unable to label media from slot %d.
132 %s(%d): Unable to label media.
133 %s(%d): Label of media fails [Old VSN %s, New VSN %s].
134 %s(%d): Unable to label tape.
135 %s(%d): unable to open %s for format.
136 %s(%d): unable to position.
137 %s(%d): unable to re-open after format.
138 %s(%d): Unable to re-open:%m.
139 %s(%d): Unable to read old labels.
140 %s(%d): Unable to rewind tape.
141 %s(%d): Unable to write labels on media from slot %d, on eq %d
142 %s(%d): Unable to write labels.
144 %s(%d):driver blksize is not power of two.
147 ** NOT USED ** %s(%d):Unable to label tape from slot %d.
149 %s(%d:%s): bad block size %#x setting to %#x.
150 %s(%d:%s): bad blocksize (not power of 2) %#x setting to %#x.
152 %s(%d:%s): did not return capacity
153 %s(%d:%s): Error during stage (inode %d.%d) : archive header is not valid
157 %s(%d:%s): get position failed.
160 %s(%d:%s): maxblk not valid
161 -eSV[Dev:NoSpace]%s(%d:%s): No space left.
162 %s(%d:%s): position not valid.
163 sam_wait_space: %s: File system full - %s.
164 No space on %s %s
167 %s(%d:%s): Stage error: Checksum miscompare:%s.
168 %s(%d:%s): Stage error: Data miscompare.
174 %s(%d:%s): Unable to determine position.
176 %s(%d:%s): unable to position.
179 %s(%d:%s):buffer size not multiple of sector size.
184 %s(%d:%s):Rewind failed.
186 %s(%d:%s):Stage error reading removable media:%s.
190 %s(%d:%s):unable to get status:%m.
192 %s/%s: Out of memory for directory name cache, current size \
of cache is %d bytes
206 %s: %s family is missing or duplicated.\n
207 %s: %s must be block special file.\n
208 ** NOT USED ** %s: Bad magic number in dump file
211 %s: Cannot attach shared memory segment: %s\n
212 %s: cannot chdir()
213 %s: cannot open()
215 %s: cannot creat()
216 %s: Cannot create dump file
221 %s: ** NOT USED **  Cannot malloc space for directory stack.
222 %s: Cannot mkdir()
225 %s: Cannot open dump file
226 %s: Cannot read directory
227 %s: Cannot read link info.
228 %s: Cannot read resource file.
229 %s: cannot sam_stat
230 %s: cannot sam_stat()
237 %s: ejected_media: equipment %s: not present.\n
238 %s: ejected_media: equipment %s: out of range.\n
240 %s: ** NOT USED **  File is now damaged
241 %s: ** NOT USED **  File was already damaged prior to dump
244 %s: Header record read error
245 %s: Header record write error
248 %s: Invalid argument for restore
249 %s: Invalid flag: %s\n
251 %s: Invalid media type (%s).\n
254 %s: Invalid VSN length.\n
256 %s: No family set device for eq=%d set=%s
259 %s: Not a SAM-FS file.
268 %s: Pathname too long
272 %s: SAM-FS is not running.\n
273 %s: stat() id mismatch: expected: %d.%d, got %d.%d, entry not processed
281 %s: Unknown media type.
282 %s: unknown option\n
283 %s: Unrecognized mode (0x%x)
290 %s: ** NOT USED **  Warning! File is already damaged.
292 %s: ** NOT USED **  Warning! File will be damaged.
305 ** Not Active **
314 -m must be specified if using -v
315 .blocks read failed on eq %d
316 .blocks write failed on eq %d
317 .inodes block at offset 0x%llx is in system area
318 .inodes file (%s) restored\n
319 .inodes pointer not set
320 512 byte sector size not supported
321 512 byte sector size not supported, equipment %d
323 : media is write protected
329 Aborting dump_cat.\n
331 Access denied.
339 acs_query_pool failed: %s\n
340 acs_query_pool: count error: %d.\n
341 acs_query_pool: status error: %s\n
342 acs_query_pool: wrong pool %d.\n
343 acs_query_scratch failed: %s\n
344 acs_query_scratch: No scratch volumes.\n
348 acs_response(qu_scr): wrong sequence.\n
349 acs_response(qu_scr):failure %s
350 acs_response(set_scr): wrong sequence.\n
351 acs_response(set_scr):failure %s
352 acs_response: wrong sequence
353 acs_response:failure %s\n
354 acs_set_scratch failed: %s
370 Additional sense bytes:
373 ** NOT USED **  ALERT:  %d large blocks are free and allocated\n
374 ** NOT USED **  ALERT:  %d small blocks are free and allocated\n
375 ALERT:  %s-F will mark ino %d %s\n
376 ALERT:  %sino %d marked %s\n
378 ** NOT USED **
379 ** NOT USED **
380 ALERT:  ino %d,\tdirectory read failed on eq %d\n
381 ** NOT USED **
382 ** NOT USED **
383 ** NOT USED **
395 " and barcodes are %s order"
399 append_tape(%d:%s): EOF1 not found at %#x. Retry.
400 append_tape(%d:%s): found Record-TM-EOD.
401 append_tape(%d:%s): position after R-TM-TM-EOD (%#x) \
is < supplied position (%#x)
402 append_tape(%d:%s): position after TM (%#x) \
is < supplied position (%#x)
403 append_tape(%d:%s): position after TM-TM-TM (%#x) \
is < supplied position (%#x)
404 append_tape(%d:%s): position at EOF-TM-TM (%#x) \
is <  supplied position (%#x)
405 append_tape(%d:%s): skipping to EOD.
406 append_tape(%d:%s): Media Error : unable to find EOD.
408 append_tape(%d:%s): Media Error : unable to find EOD.
409 append_tape(%d:%s): Media Error : unable to skip EOD.
410 append_tape(%d:%s):Found EOV1 %#x.
417 ** NOT USED ** archdone files %9s
423 Archive file %d, %lld byte%s written
440 Archiver state file memory version mismatch.  Is: %d, should be: %d
445 Argument error.
448 assign fsn %d to resource
452 Attempt to label active device (%d).
454 Attempt to label robotic device (%d).
459 Attempting first unmount of file system(s).\n
461 Attempting second unmount of file system(s).\n
464 Attempting to lock shared memory.\n
466 attempting to read label information
467 Attempting to kill daemons.\n
468 attended
487 audit_eod (-e) is not allowed for the historian.\n
492 available
495 Backup superblock write failed for %s
496 backwards
498 Bad arch_status: 0x%x %s
499 Bad catalog file, in excess of 1 gigabyte size.\n
500 Bad device type in request file:%s
502 Bad ino %d, ino_count = %d\n
507 Bad media unloaded and door unlocked
510 Bad mt device name %s
517 barcode may only be specified for the historian.\n
519 barcodes
521 barcodes_low
523 ** NOT USED ** base: 0x%x
524 become ready
525 becoming ready
533 Blocksize must be one of 16, 32, 64, 128, 256, 512, 1024 or 2048.\n
534 ** NOT USED **
535 ** NOT USED **
538 Both read-only and read-write specified.\n
545 Buffersize not even multiple of inode size.\n
550 calibrating
556 Cannot create response FIFO
560 Cannot open cmd FIFO
567 Cannot attach master shared memory segment
568 Cannot attach shared memory segment\n
571 Cannot audit device (%s)
572 cannot chdir to \"..\" %s
573 Cannot chown %s
574 Cannot create %s
577 Cannot create thread: errno %d retval %d
579 Cannot determine current ID for %s
580 cannot determine cwd.
581 Cannot export volume from (%s).
582 Cannot find master shared memory segment.
583 Cannot find ordinal 0 for filesystem %s
584 Cannot find pathname for filesystem %s inum/gen %lu/%ld\n
586 Cannot get .inodes block at offset 0x%llx
587 Cannot get cwd
590 cannot getcwd()
592 Cannot import for robot (%s).
593 Cannot ioctl(F_IDFILE) %s
594 Cannot ioctl(F_IDSCF) %s: %s
596 cannot label read only media
597 Cannot link %s to %s
599 Cannot malloc devp.
600 Cannot malloc for path %d
601 Cannot malloc indirect block.
602 Cannot malloc ino array.
603 Cannot malloc mcf_file.
604 Cannot malloc ndevp.
605 Cannot malloc space for reading directory %s
606 Cannot map device type %d for %s
607 Cannot mkdir %s
608 Cannot chdir %s
609 Cannot mmap %s
610 Cannot mmap %s: length %d
611 Cannot munmap %s
612 Cannot exec %s

$	message 613 is the standard message used for open() system call
$	failures. The %s parameter is the file name used on the call.
$	This message typically is used as part of a call to error(), which
$	will include an errno string diagnostic.

613 Cannot open %s
614 Cannot close %s
615 Cannot write %s
616 Cannot stat %s
617 Cannot unlink %s
618 Cannot chown %s
619 Cannot open kernel memory.
620 Filesystem \"%s\" not found.
621 Cannot open \".\"
622 Cannot opendir %s
623 Cannot closedir %s
627 cannot read rminfo %s
628 Cannot realloc for directory names %d
629 cannot request %s
630 cannot request: operation not supported on shared client
632 Cannot sam_readrminfo %s
633 cannot sam_stat %s
634 Cannot sam_vsn_stat %s
635 Cannot setfa %s
640 ** NOT USED ** cannot specify -n with -a or -p.
641 cannot specify -w with -a, -d, or -n.
642 cannot specify more than one copy
643 -a, -n are mutually exclusive.
644 ** NOT USED ** cannot specify -w with -d or -n.
645 Cannot truncate %s: length %d
646 Cannot unload device (%s).
653 capacity at EOD returned as 0
654 Capacity of media (%d:%s) (%#llx), larger than UINT_MAX.
656 capacity: %.8lx\n
675 checking for EOF label
686 cleaning cartridge
705 closing opened optical file
711 ** NOT USED ** Command not available with your site's license.
712 This command must be invoked as sammkfs, samgrowfs or samfsinfo, not %s
713 Command:
715 %s: must only be invoked as one of:\n
722 Configuration error
729 copy must be: 1 <= c <= %d
737 Corrupt samfsdump file.  name length %d.
739 %s: inode version incorrect - skipping.
740 Corrupt samfsdump file. symlink name length %d
741 Could not determine path
743 Could not rewind:(%d:%s).
744 Could not write file mark:(%d:%s).
746 Could not open kernel files.
747 Could not read memory %llx
749 Could not read rootvfs vfs pointer at %x
754 Could not read SAM-FS mount table at %x
758 Could not read SAM-FS vfs entry at %x
759 Could not read SAM-FS work queue 0 at %x
760 Could not read SAM-FS work queue 1 at %x
761 Could not read samgt entry at %x
762 Could not read sam_global_tbl pointer at %x
763 Could not read vfs entry at %x
764 Could not retrieve command response
766 count %d
769 count:    %d\n
770 count: %d
780 create_tape_eof - nothing written
781 create_tape_eof(%d:%s): Unable to write EOF.
784 creating EOF-Tape mark
788 current:
789 debug:
797 Dau map read failed on eq %d
798 Dau map read failed on eq %d
799 Dau map write failed on (%s)
800 Dau map write failed on eq %d
801 Dau map write failed on eq %2d, (%s)
802 Dau map write failed on eq %d
813 default:
836 Delaying for media changer
845 ** NOT USED ** Destination slot %d is already occupied.\n
846 ** NOT USED ** Destination slot %d is not a valid slot.\n
847 ** NOT USED ** Destination slot %d is not available for move.\n
851 determine capacity
853 Device %d is not ready.\n
854 Device %d not defined.
855 Device %d: exported_volume is %s\n
856 Device %s not defined.
857 Device %s is not manually loaded.
859 Device %s not found.
860 Device (%d:%s) positioned to %#x after reset.
861 Device (%s) cannot be itemized.
862 Device being offed eq %d
863 Device being offed eq %d.\n
864 Device cannot be labeled.
865 Device cannot be opened (%s)
868 Device configuration:
871 Device has %s
872 Device is type default. 'Update /kernel/drv/st.conf'.
873 Device name:
874 Device not a robot (%s).
875 Device not found (%s)
876 Device not off.
877 Device not on.
881 Device not specified.
882 Device number out of range (0 < %d < %d)
884 Device off or down (%s).
887 Device table: eq: %d
899 Did not become ready, check log
900 Did not find tape mark at EOD(%d:%s).
901 differ:
906 Direct I/O timed out
908 ** NOT USED ** directories    %9s
909 directories marked damaged: %d\n
910 directories marked offline: %d\n
916 disk-d-0-0 ms %s %s %d %d %d unused unused\n
933 Display control commands:
935 Displays:
938 Do not specify 'c' option.
939 Do not specify 'O' option.
940 Do not specify 'r' option.
941 Do not specify 't' option.
952 drive
953 Drive %d did not become ready.
967 "  drive\n"
972 Dump created:%s
986 Either copy or VSN/media must be specified
996 Empty sector
997 Empty sector %.8x (%d)
1002 end of tape detected
1008 eod is at %#x
1010 EOT detected
1017 Equipment for filesystem %s is not all present
1026 ** NOT USED ** ALERT:  Error count>=%d:ino=%d, count=%d, free=0x%x, type=0x%x, 0x%x.%d\n
1027 error during close: %#x
1028 error during close: %s
1029 Error during consistency check
1031 Error during floating point conversion of %s.\n
1032 Error during initialization
1033 Error during label scan
1034 error during open: %#x
1035 error during open: %s
1039 Error in indirect blocks of .inodes (%s) file
1040 Error in layout of .inodes (%s) file
1041 Error in mnttab format entry %d.\n
1042 error in options, no options changed.\n
1043 error in position
1044 error in required size
1045 Error in size argument.\n
1046 Line %d slot is negative or exceeds catalog file size. Entry ignored.\n
1047 error in version
1048 Error mapping symbol %s: %s
1049 Error mapping symbol -%s-:%s.
1054 Error on line %d(capacity or space). Entry ignored.\n
1055 error on line %d(type).  Entry ignored.\n
1057 error on line %d(volser).  Entry ignored.\n
1058 ** NOT USED ** error on line %d(slot or MID).  Entry ignored.\n
1059 error on line %d(vsn).  Entry ignored.\n
1063 Error reading %s: %s
1064 Error reading .inodes (%s) file
1066 Error reading dump file.
1068 Error reading old labels, eq %d
1070 Error restoring .inodes file
1071 Error scanning equipment %d
1085 Existing VSN not specified for relabel.
1086 Did not unload, check log
1087 Drive contains bad media - check log
1088 Cannot sam_vsn_stat; file path is NULL.
1089 Cannot sam_segment_vsn_stat %s (copy %d, segment %d)
1090 Ioctl call to stat %s (%d, %d) failed.
1091 Ioctl call to stat %s, segment %d (%d, %d) failed.
1092 File %s has segment size %d.

$	message 1113 is the standard message used for open() system call
$	failure of the command FIFO.
$	This message typically is used as part of a call to perror(), which
$	will include an errno string diagnostic.

1113 Unable to open command fifo:
1114 Command FIFO write failed
1144 file identifier longer than %d characters
1146 file is not on a SAM-FS filesystem
1150 file name > %d chars, use shorter file name or -f option
1160 File system %s not available.
1170 File system samfs is not loaded.\n
1172 file type is not regular or symlink, type = %x\n
1175 files marked damaged: %d\n
1176 files marked offline: %d\n
1180 Filesystem %s is not mounted.
1186 Find EOD-No tape mark at EOD
1190 finding EOD - backspace at EOD
1191 finding EOD - BSF at ?TM-EOD
1192 finding EOD - BSF at EOD
1194 finding EOD - BSR at ?TM-EOD
1195 finding EOD - BSR at TM-EOD
1196 finding EOD - BSR at TM-TM-EOD
1197 Finding EOD - checking for EOF record
1198 finding EOD - found EOF record - backspacing
1199 finding EOD - found R-TM-EOD - skipping to EOD
1200 finding EOD - found TM-TM-TM-EOD at %#x
1201 finding EOD - FSF
1202 finding EOD - not EOF1 record - skipping TM
1203 finding EOD - position at EOD is %#x
1204 finding EOD - rewinding
1205 finding EOD - skip to EOD
1228 forward
1232 found ?-TM-EOD
1234 found EOF1 at %#x
1235 found EOV1 at %#x
1236 found R-TM-TM-EOD at %#x
1244 Attemped read of re-labeled media %s.
1251 get position failed
1252 get position returned not valid
1270 group identifier longer than %d characters
1274 Header record error
1275 Header record error: Bad magic number
1276 Header record error: Bad version number (%d)
1295 high
1296 High (%d) must be greater than low (%d) and less than 100\n
1297 high requires parameter.\n
1319 illegal character in tape vsn
1328 Import to GRAU, STK and IBM libraries must be by vsn or \
scratch pool(STK only).\n
1336 Improper display selection %s
1337 In inquiry.conf: line %d is missing an open quote on \
the %d parameter.\n
1339 Incorrect device type %s.
1375 Ino %d read failed on eq %d
1376 inode %d.%d: Cannot ioctl(F_IDTIME)
1377 Inode size GT SAM_ISIZE (%x)
1380 Inodes processed: %d\n
1381 INQUIRE command on eq %d, failed: %m
1383 Inquiry.conf file missing.\n
1384 inquiry.conf: line %d, product id greater than 8 characters.\n
1385 inquiry.conf: line %d, vendor id greater than 8 characters.\n
1395 Invalid algorithm specified.
1401 Invalid argument r.
1405 Invalid characters in old VSN.
1406 Invalid characters in VSN.
1408 Invalid command (%s)
1416 Invalid media
1417 Invalid media in equipment %d
1421 Invalid n_vsns field %d for copy %d of %s
1423 Invalid output file path
1425 Invalid path to audit
1431 Invalid sector offset (%s)
1442 Ioctl(DKIOCGVTOC) failed on (%s)
1443 Ioctl(DKIOCINFO) failed on (%s)
1460 Issued %s
1485 Labeled %s, blocksize %d
1486 Labeled %s, blocksize %d%s
1487 labeled - range %#llx-%#llx
1489 labels are %s
1490 labels written
1548 llseek: sector %.8x (%d)
1557 load: %d is not a removable media device.\n
1559 load: %d is not in unavailable state.\n
1566 ** NOT USED ** load: Slot %d is a cleaning cartridge.\n
1572 load: Unknown argument %s.\n
1573 Loading
1589 low
1590 low requires parameter.\n
1591 Low water mark must be between 0 and 100.\n
1599 mailbox
1600 "  mailbox\n"
1606 malloc: %s\n
1626 Superblock size=%lld exceeds mcf eq %d size=%lld
1627 mcf eq %d has duplicate ordinal %d in filesystem %s
1628 mcf eq %d, ordinal %d, is not present in filesystem %s
1630 Volume (%d:%s) bad block size %#x setting to %d.
1631 media already labeled
1632 Media already labeled with %s.
1634 " MEDIA ERROR"
1639 media is not formatted
1640 media needs format
1641 media needs formatting
1643 media type may only be specified for the historian.\n
1644 Media type mismatch, line %d.  Entry ignored.\n
1645 media type must be supplied for the historian.\n
1647 media unloaded and door unlocked
1652 Memory exhausted
1660 Superblock on eq %d at block %d missing magic and/or name
1661 Mismatching eq (%d) for filesystem %s is present
1664 Missing EOF: %s, %s.
1665 Missing lower [upper] inode numbers for -I
1673 Mode sense failed
1688 Mount point:
1699 Mount: family set equipment not found.\n
1700 ** NOT USED **  mount: SAM-FS group name not found.\n
1701 ** NOT USED **  mount: SAM-FS user name not found.\n
1705 " mounted at %s"
1709 mount_samfs: open mnttab lock
1710 mount_samfs: Unable to obtain mnttab lock.
1711 mount_samfs: Can't open mount lock
1712 mount_samfs: Can't get mount lock
1734 name:     %s\n
1735 Namelist lookup error.
1737 " NEEDS AUDIT"
1767 NO LABEL
1770 No media for storage slot %d.
1771 No members in filesystem %s.
1773 No memory for directory buffer %d
1778 No memory for vsns
1785 No robot in system matches command line argument '%s'.
1788 No SAM-FS vfs entry found
1790 No sense data available
1792 No space left on media
1804 Not a disk eq number in a family set (%s).
1805 Not a disk family set device (%s).
1806 Not a robot device (%s).
1810 ** NOT USED **  Not all directory times will be set correctly.
1812 " not archiving"
1820 Not ready or read-only media in device (%d).
1823 NOTICE: %lld bytes can be reclaimed\n
1824 NOTICE: Reclaimed %lld bytes\n
1832 number of vsns greater than %d
1839 ** NOT USED **  offline files  %9s
1840 Old VSN %s does not match media VSN %s.
1841 Old VSN %s does not match unknown media VSN.
1842 d2format: %s out of range.
1843 Old VSN must be 31 characters or less.
1844 Old VSN must be 6 characters or less.
1845 One of c, r, O or t options must be specified
1853 Open command required
1855 open failed - retrying
1856 Open failed on (%s)
1857 open failed(%d)-%m - retrying %d more times.
1876 optical options not valid for this media
1885 options:
1887 ord  eq           capacity              space   device\n
1891 Out of memory for a directory name.
1892 Out of memory for a directory named %s.
1896 Overriding mcf entry.\n
1898 owner identifier longer than %d characters
1901 page %d/%d
1908 partial requires parameter.\n
1909 Partition %d is undefined on (%s)
1922 Position with FSR %d
1923 Pos-err(%d:%s):wanted %#x, got %#x.
1924 position %s (%#x:%#x)-(%#x:%#x)
1925 Position after TM (%#x) is < supplied position (%#x)
1927 position after TM-TM-TM-EOD (%#x) is < supplied position (%#x)
1928 Position at EOD is %#x
1929 position at EOF-TM-TM-EOD (%#x) is < supplied position (%#x)
1930 Position at EOF1-TM-TM (%#x) is < supplied position (%#x)
1931 position at EOF1-TM-TM is %#x
1932 position at R-TM-TM-EOD (%#x) is < supplied position (%#x)
1933 Position error:wanted %#x, got %#x
1934 position failed
1937 Positioned at %#x
1938 positioned to %#x after reset
1939 Positioning error:(%d:%s) to %#x.
1940 Positioning error:(%d:%s).
1941 Positioning to %#x
1942 Positioning to %#x-failed, check log
1943 Positioning- BSF
1944 Positioning- BSR
1946 Possible bus reset
1947 Possible bus reset (%d:%s).
1948 Position at EOD %#x
1950 press ready button
1951 Press Return to continue\n
1954 Preview data not available
1957 Preview table full.
1969 Problems skipping records(%d:%s) (%#x,%#x,%#x:%c%c).
1972 process labels
1973 Process labels: Read labels%s
1974 Process labels: Rewinding%s
1998 Raw device not found for eq %d
2004 read
2007 Read capacity failed
2010 ** NOT USED **
2011 ** NOT USED **
2012 ** NOT USED **
2015 Read incomplete, len=%x (%d)
2016 read position error:(%d:%s) to %#x.
2017 read position: preposterous position(%d:%s).
2023 read: sector %.8x (%d)
2040 Reconcile catalog.
2045 Recorded File Name : %s
2048 Recover from position error: Positioning to %#x
2049 Recover from position error: Rewinding
2051 Recovery: Rewinding
2053 Recycler finished.
2054 Recycling cannot occur due to the above errors.
2055 Recycling did not complete due to errors.
2074 Refresh interval %s, must be >= 0
2075 Refresh off.
2076 Refresh on, delay = %d.
2079 ** NOT USED ** regular files  %9s
2085 Releasing %s\n
2090 Removable media I/O activity: %s
2092 Removable media status: %s
2096 Remove stale preview %s
2125 Returned --\n
2128 Rewinding
2133 Robot %s catalog missing.
2134 Robot %s has no drive for this media.
2139 Robot name:
2141 Robot VSN catalog by %s: eq %d
2142 Robot VSN catalog: eq: %d\tcount: %d\n
2162 Runtime interface not found.
2196 sam-amld not initializing.
2197 sam-amld process died. Status = %#x\n
2198 sam-amld found error(s) in defaults.conf.  Check /var/adm/messages, fix and restart\n
2210 samdev: cannot create %s:
2211 samdev: cannot create symbolic link: %s -> %s:
2212 samdev: cannot stat %s:
2213 samdev: Unable to lstat: %s
2214 samdev: warning: No samst devices found.\n
2221 System call changed to %d in %s\n
2244 SAM-FS is not running.\n
2247 sam_stat error for %s
2250 Superblock count (%d) mismatch with mcf count (%d) in filesystem %s
2251 scan(%d): Unformatted D2 media.
2252 Superblock for eq %d: system %d != computed len %d
2254 Scratch pool only supported on STK.\n
2255 Scratch pools and specifying vsn are mutually exclusive.\n
2260 Direct io timed out (%d).
2265 Second pass\n
2266 Second unmount %s, %s
2272 ** NOT USED **
2310 setting driver block size
2313 %s: %d is not a removable media device.\n
2314 %s: %d is not a valid equipment ordinal.\n
2315 %s: Can only set down devices off.\n
2317 %s: Device %d is open by another process.\n
2321 set_state: Unknown state %s.\n
2322 set_state: Cannot down device, can only off device.\n
2325 Shared memory locked.\n
2333 Signal received %d
2339 size: %d
2344 skip to EOD failed: attempting recovery-Rewind
2345 skip to EOD failed: attempting recovery-skip EOD
2346 skip to EOD failed: recovery failed
2347 Media Error : skip to EOD returned error
2348 %s is not a valid value for %s.\n
2349 Skipping %s: not home
2350 %s is not a valid equipment ordinal.\n
2351 skipping to EOD
2352 slot     barcode          count use flags         ty vsn
2353 slot           label time count use flags         ty vsn
2354 slot          access time count use flags         ty vsn
2355 ** NOT USED ** %s is not a valid slot.\n
2356 ** NOT USED ** Slot %d is not a valid slot number.\n
2357 ** NOT USED ** Slot %d is not occupied.\n
2358 Index not in preview.
2359 Slot %d not in catalog
2360 VSN modifier must be 1 or 2 for optical media.
2361 D2 VSN requires a partition id modifier.
2362 Slot must be specified for robot device %s.
2363 ** NOT USED ** Slot or vsn required for this device.\n
2364 Catalog initialization failed!
2365 " SLOT VACANT"
2374 ** NOT USED ** Source slot %d is not a valid slot number.\n
2375 ** NOT USED ** Source slot %d is not available for move.\n
2376 ** NOT USED ** Source slot %d is not occupied.\n
2381 space:    %.8lx\n
2386 Spin down  %s
2387 Spin up
2388 spun down
2394 Stage request canceled by fs
2398 Staging %s\n
2400 Staging data not available
2404 Ready for data transfer
2419 Status %d on system(%s)
2437 Superblock information record size GT L_SBINFO (%x)
2438 Superblock ordinal record size GT L_SBORD (%x)
2439 Superblock read failed on eq %d
2440 Superblock write failed for %s on eq %d
2444 Symbol rootvfs not found
2446 Symbol samfs_vfsops not found
2447 Symbol samgt not found
2453 System call change failed in %s\n
2457 System error return from read of samfsdump file
2459 system is running in %s mode.\n
2460 system is running with labels = %s\n
2465 Tape device %d is default type. Update '/kernel/drv/st.conf'.
2467 Tape on (%d:%s) bad block size(not power of 2) %#x setting to %d.
2468 Tape path is not sym-link - %s.
2469 tape vsn longer than 6 characters
2470 D2 tape vsn longer than 10 characters
2479 The IBM3494 only supports automatic drive cleaning.\n
2481 The LSCrmtsam package is not installed:%s.
2482 The LSCstk package must be installed to use scratch pools
2483 The shared object library %s cannot be loaded: %s
2491 ** NOT USED ** This machine lacks a SAM-FS license: %s aborts
2499 time:     %s
2506 TIME: pos(%d:%s) %#x to %#x: %d seconds.
2507 TIME: pos-F(%d:%s) %#x to %#x: %d seconds
2509 TIME: rewind(%d): %d seconds
2510 TIME: rewind(%d:%s): %d seconds.
2522 Too many entries in inquiry.conf: 400 max.\n
2523 Too many filesystems found:  Max is %d.
2524 Too many vsn's, max is %d
2526 total %s kilobytes       = %lld\n
2527 total %s kilobytes free  = %lld\n
2529 ** NOT USED ** Trace options:
2530 ** NOT USED ** Trace output to %s\n
2553 unable to find EOD
2556 Unable to attach master shared memory segment
2558 Unable to attach master shared memory segment: %s\n
2559 Unable to attach preview shared memory segment.\n
2560 Unable to attach preview shared memory segment
2563 Unable to BSF-1 during EOD search
2564 Media Error : Unable to BSF-1 during EOD search(%d:%s).
2567 Unable to BSF-l EOD search
2568 Media Error : Unable to BSR-2 EOD search(%d:%s)
2584 Unable to determine capacity (%d:%s) Device does not support function
2585 Unable to determine capacity (%d:%s):Invalid data.
2586 Unable to determine capacity:(%d:%s)
2588 Unable to determine position (%d:%s)
2589 Unable to erase disk, eq %d
2593 Unable to find master shared memory segment.
2594 Unable to find master shared memory segment: %s\n
2596 Unable to find mount point %s.\n
2597 Unable to find preview shared memory segment.\n
2604 unable to FSF during EOD search
2607 Unable to get samst device for non-/dev/rmt/ device:%s.
2608 unable to get samst name for non st device:%s.
2609 Unable to get status for device %s:%m.
2615 Unable to label media
2616 ** NOT USED ** Unable to label media from slot %d
2623 Unable to lock shared memory.\n
2625 Unable to malloc space for buffers (%m).
2627 Unable to malloc space for copy of shared memory segment.\n\
Using real segment.\n
2628 Unable to malloc space for pathname (%m).
2629 Unable to malloc space for release list (%m).
2632 Unable to map catalog %s: %s.\n
2636 Unable to nice %m
2642 Unable to obtain staging resources.
2643 -eSV[Dev:CannotOpen] Unable to open
2644 Unable to open %d for mount:%m.
2650 Unable to open catalog %s: %s.\n
2658 Unable to open eq %d:%m
2659 Unable to open for spinup
2660 Unable to open inodes file (%m).
2669 unable to open to write labels: %#x
2670 unable to open to write labels: %s
2671 Unable to open to write labels
2674 Unable to read sym-link - %s:%m.
2675 Unable to recover from error
2676 Media Error : Unable to recover from error(%d:%s).
2677 Media Error : Unable to recover(%d:%s).
2679 Unable to release disk space to %d%% on %s.
2680 unable to reopen for labels
2688 unable to skip EOD
2700 Unable to start thread (%m).
2708 Unable to stat %s: %s.\n
2724 Label write failed for %s on eq %d
2726 unattended
2728 unavailable
2738 Unknown archive media 0x%x: %s (copy %d)
2739 ** NOT USED ** Unknown archive trace option: %s
2740 Cannot get archiver shared memory segment
2741 Cannot attach archiver shared memory segment
2742 Archiver not active
2743 unknown debug option %s\n
2744 Unknown device type in mount (%d).
2745 Unknown device, eq %d (\"%s\"), device type (%s).\n
2746 Unknown error
2753 unknown media
2754 Unknown media type.\n
2755 Unknown media type %s.\n
2756 Unknown media type %x (%d:%s).
2758 Unknown media type line %d.  Entry ignored.\n
2760 Unknown option %s.\n
2764 Unknown state change
2768 Unknown media type (%s)
2769 Unlabeled %s
2782 unload: %d is not loaded.\n
2787 unloaded
2788 unloaded and door unlocked
2789 unloading
2790 Unloading %s
2797 Unmounting %s, %s
2798 Non-numeric blocksize %s.
2799 Unrecognized argument %s.
2821 Unsupported media in stage request (%d).
2825 Update /etc/opt/SUNWsamfs/inquiry.conf (see inquiry.conf(4).
2828 Upper limit of blocksize for this device is %d rounded down \
to a power of two
2834 user info longer than %d characters
2836 User information must be 127 characters or less.
2842 Vendor/product %s.\n
2871 VSN %s not found in robot.\n
2872 VSN and index inconsistent.
2873 VSN and/or barcode must be supplied for import into the historian.\n
2874 VSN catalog not present.
2879 vsn longer than %d characters
2880 " VSN MISSING"
2881 VSN must be 31 characters or less.
2882 VSN must be 6 characters or less.
2884 VSN not found.
2885 VSN not specified.
2890 wait ready-no media present
2897 waiting for drive to become ready
2903 Waiting for response to timeout
2904 Waiting for response to timeout.
2910 Warning: Only %d volumes left in pool.\n
2915 weight requires parameter.\n
2929 ** NOT USED **
2930 ** NOT USED **
2931 ** NOT USED **
2936 write label - rewinding
2937 write label - wait ready
2938 write label: erase started %s, %d hour time limit
2939 write label: format started %s, %d hour time limit
2940 write label: format succeeded: rewinding
2944 Write tape mark
2958 writing EO%c at %#x
2959 writing labels
2960 writing labels: hdr1
2961 writing labels: hdr2
2962 writing labels: vol1
2963 writing Tape mark-Tape mark
2966 Wrong shared memory segment.\n
2967 wt_labels(%d): Unable to open: %#x.
2968 wt_labels(%d): Unable to open: %s.
2969 wt_labels: Unknown device type (%d).
2975 You must be root to set the position
2976 File system ioctl (SC_position) failed.
2977 WORM media loaded.
2978 Unable to determine if WORM or RW media loaded.
2979 Media is WORM, drive not WORM capable.
2980 Read Position near %#x-failed, check log
2981 Locate to %#x-failed, check log
2982 Drive is in CD-ROM emulation mode.
2983 clean failed


3038 cannot chdir to %s
3039 cannot read directory %s
3040 cannot close directory %s
3041 : Unknown error %d
3042 number of positions greater than %d
3043 number of vsns %d does not match number of position %d
3049 skipping to eod
3052 Attempt to stage inode %d from re-labeled media %s.
3053 %s(%d:%s): Rewind failed.
3054 %s(%d:%s):Medium error at %#x.
3055 %s(%d:%s):Device error at %#x.
3056 TIME: to ready(%d): about %d seconds.
3058 TIME: Skip-eod-F(%d:%s): %d seconds.
3059 TIME: skip-eod(%d:%s): %d seconds.
3060 append_tape(%d:%s): Media Error : Unable to BSF-1 during EOD search.
3061 append_tape(%d:%s): Media Error : unable to FSF during EOD search.
3062 Finding EOD - BSF at TM-EOD
3063 unable to BSF-2 during EOD search
3064 append_tape(%d:%s): Media Error : unable to BSF-2 EOD search.
3066 No memory for mount points (need %d bytes): %s
3067 No media changer devices were found.
3068 No memory for device catalogs (%d): %s
3088 Cannot open directory %s
3091 putting name length
3092 putting pathname
3093 putting base inode
3094 putting resource info
3095 putting link length
3096 putting link resolution name
$  WARNING: DO NOT PUT ANY MESSAGE ABOVE THIS LINE.
$  WARNING: THIS SECTION IS AN ORPHANED SECTION.



$  archiver     (4000s)
$ ===================================================================
$ The messages in the range 4000 - 4100 are sent via SendCustMsg()
$ They may be preceded by an optional delivery code string that begins with
$ '-'.  The letters may be in any order and may be one of the following:
$     N - Send message to notify executable file.
$     S - Send message to syslog
$ The first letter of the syslog priority codes -
$     Emerg, alert, crit, err, warning, notice, debug
$ The default action is: -Si
$ Example: -SNw  sends a "warning" to both syslog and the notify executable.

$ 4000 ** NOT USED ** -e Fatal OS call error
$ 4001 ** NOT USED ** -c Out of memory
4002 -wSV[Archiver:NoRemMedia] No removable media devices found.  No archiving will be performed.
4003 Archiver shutdown by signal %s
$ 4004 ** NOT USED ** -wS No VSNs available for archiving.
$ 4005 ** NOT USED ** -wS No drives found.  No archiving will be performed.
4006 -wS No file systems found.  No archiving will be performed.
4007 Restarting
4008 -aV[Archiver:NotUniq] Another sam-archiverd is running
4009 Filesystem not mounted.  Cannot archive.

4010 -wNV[Archiver:NoVolAvail] No volumes available for Archive Set %s
$ 4011 ** NOT USED ** -wN No space available on Archive Set %s
4012 archiver configuration file changed, rereading
4013 -wNV[Archiver:FileTooBig1] No space available for Archive Set %s - File %s too large for volume overflow on remaining volumes
4014 -wNV[Archiver:FileTooBig2] No space available for Archive Set %s - File %s too large for any remaining volume
4015 -wNV[Archiver:FileTooBig3] No space available for Archive Set %s - Joined files too large for any remaining volume
4016 Waiting for archiving to complete
$ 4020 ** NOT USED ** -eNSV[Archiver:Died] %s[%d] died - Not restarting
$ 4021 ** NOT USED ** Unknown external message command %d
$ 4022 ** NOT USED ** -wNS Cannot create trace file %s
4023 -wNSV[Archiver:CmdErr] Errors in archiver directives - no archiving will be done.
4024 -wNSV[Archiver:OutstandingReq] ArchReq %s queue timeout: %s
$ 4026 ** NOT USED ** -eS Archive write failed %s: %s - %s %s:
4027 -wS %s[%d] exit status %04x
4028 -wV[Archiver:CpyOffline] Cannot archive file: offline with all copies damaged.  %s
$ 4029 ** NOT USED ** -Sw Notify file %s not accessible: %s
$ 4030 ** NOT USED ** -Sw Execution of notify file %s failed: %s
$ 4031 ** NOT USED ** -Sw Notify file %s returned exit status 0x%04x
$ 4032 ** NOT USED ** -wS Reserved VSNs file %s not found
$ 4033 ** NOT USED ** Reading reserved VSNs file
4034 Warning in line %d: %s %s
4035 %d warnings in reserved VSNs file %s
4036 1 warning in reserved VSNs file %s
4037 See '%s' for details.
$ 4038 ** NOT USED ** OS call error
$ 4039 ** NOT USED ** -iS Reserved VSN %s %s relabeled.  Was reserved to %s
$ 4040 ** NOT USED ** Cannot create new Reserved VSNs file %s
4041 -iS Reserved VSNs file backed up to %s
$ 4042 ** NOT USED ** -iS Reserved VSNs file replaced
4043 -iS Volume full: %s.%s: %s bytes written
4044 -iS Overflowing to volume %d: %s.%s
4045 sam-archiverd may be started only by sam-fsd
4046 parent died
$ 4047 ** NOT USED ** File system %s not available
4048 -iS Set bad media for %s.%s
4049 -wS %s[%d] exited, not restarting
4050 -wS Failed to label disk volume %s, %s.
4051 Path name of the disk archive directory is not available
4052 Path name of the disk archive directory does not exist
4053 Failed to open/create disk archive directory
$ File System Manager uses 4054 instead of 4028, custmsg has code to prevent
$ flooding of events
4054 One or more files cannot be archived: File is offline with all copies damaged. Please see the log files for additional information
4055 -iS Set read only for %s.%s
4056 Fatal error on disk volume seqnum file: %s (%s/%s)
4057 STK 5800 client error. Initialization failed, hcerr %d - %s
4058 STK 5800 client '%s:%d' error. Session create failed, hcerr %d - %s
4059 STK 5800 client '%s:%d' error. I/O failed, hcerr %d - %s
4060 Disk archive seqnum number file does not exist, %s - no archiving will be done.

$ Operator messages
4300 Waiting for %s
$ 4301 ** NOT USED ** Archiver command file changed - waiting for archiving to complete
$ 4302 ** NOT USED ** Scanning directory %s
4303 Scanning inodes %s
4304 Scheduling archives
$ 4305 ** NOT USED ** Waiting for files to be archived
$ 4306 ** NOT USED ** Sleeping until %s
4307 Waiting for volume %s
4308 Copying file %s
4309 Entering archive information for %d files, %d left
$ 4310 ** NOT USED ** Staging files
$ 4311 ** NOT USED ** Composing archive files
$ 4312 ** NOT USED ** Waiting for VSNs to be available
4313 Waiting for resources
$ 4314 ** NOT USED ** Recomposing archive files
4315 Divide request for %d drives
4316 Join files
4317 Separate offline files
$ 4318 ** NOT USED ** Separate files by owners
4319 Sort files
$ 4320 ** NOT USED ** Separate files to fit %s
$ 4321 ** NOT USED ** Delaying for %d seconds
$ 4322 ** NOT USED ** Waiting for files to be on line
$ 4323 ** NOT USED ** Waiting for all files to be on line
$ 4324 ** NOT USED ** Separate segments - ArchReq %s
$ 4325 ** NOT USED ** Staging file %5d of %5d from volume %s.%s
$ 4326 ** NOT USED ** Waiting for file %s %5d of %5d to be on line from volume %s.%s
4327 Initializing
4328 Archiving files
4329 Archive set drives busy
4330 Reserved volume busy
4331 Available volume busy
4332 No drives available
4333 No volumes available
$ 4334 ** NOT USED ** Overflow volumes busy
4335 Drives busy
4336 Idle
4337 Not active
4338 Filesystem defined "archive = off".  Cannot archive.
4339 Archiving disabled - see sam log
4340 Waiting until %s for filesystem unmount
4343 Copy finished
4344 File too large for volume overflow on remaining volumes
4345 File too large for any remaining volume
4346 Joined files too large for any remaining volume
4347 All drives in use
4348 Staging - file %d (%d) from %s.%s %s/%s
4349 -wN Stage volume %s.%s unavailable
4350 Shared reader file system.  Cannot archive.
4351 Shared file system client.  Cannot archive.
4352 Waiting for :arrun %s
4353 Waiting for :arrun fs.%s
4354 No disk library
4355 Start archive at %s
4356 Waiting until %s to scan %s
4357 Scanning %s
4358 sam-arcopy fatal error - see sam-log
4359 sam-arcopy error
4360 stage volume
4361 scheduler error
4362 Archive scanning was disabled (noarscan) for this filesystem.  Cannot archive.
4363 Monitoring file system activity.
4364 Cannot archive - no rules.
4365 Inode scan paused for file system activity.

$ Archiver configuration file processing messages
4400 Cannot open archiver configuration file
$ 4401 ** NOT USED ** Reading archiver command file
$ 4402 ** NOT USED ** Error in line %d
$ 4403 ** NOT USED ** Warning in line %d
$ 4404 ** NOT USED ** Error in line %d: %s %s
$ 4405 ** NOT USED ** No definition for archive set
$ 4406 ** NOT USED ** Delay value missing
4407 Volume reserved by
4408 Invalid reserve time
$ 4409 ** NOT USED ** Reserved VSN relabeled
4410 Media specification missing
4411 Library name missing
4412 File system name missing
4413 Interval value missing
4414 Log file name missing
4415 Notify file name missing
$ 4416 ** NOT USED ** Media specification missing
$ 4417 ** NOT USED ** Trace file name missing
4418 '%s' is not a valid archiver directive
$ 4419 ** NOT USED ** Drive count missing
4420 '%s' is not a valid archive set parameter
$ 4421 ** NOT USED ** Group name missing
$ 4422 ** NOT USED ** Maximum size value missing
$ 4423 ** NOT USED ** Minimum size value missing
$ 4424 ** NOT USED ** File name missing
$ 4425 ** NOT USED ** User name missing
4426 '%s' is not a valid file property
4427 No file properties defined
4428 Copy not allowed for '%s'
4429 Duplicate archive copy number
$ 4430 ** NOT USED ** %s must be >= 0 and <= %d seconds
4431 Invalid media
4432 Invalid archive set name
4433 Archive copy number missing
4434 Invalid archive copy number
4435 Archive copy number must be 1 <= n <= %d
4436 Archive set %s not defined
$ 4437 ** NOT USED ** Join method missing
4438 Too many age parameters
4439 '%s' must precede any 'fs =' directive
$ 4440 (-release | -stage)     no_archive
4440 '%s' not allowed for '%s'
$ 4441 ** NOT USED ** Invalid %s size
4442 Incorrect VSN expression
$ 4443 ** NOT USED ** Archmax size missing
4444 Unknown library %s
4445 Invalid drive count
4446 Too many drives requested. Maximum is %d
4447 No preceding '%s' statement
$ 4448              -release  -norelease
$ 4448 ** NOT USED ** Cannot have both %s and %s
4449 Unknown file system
4450 No archive set parameters were defined
$ 4451 ** NOT USED ** Cannot access trace file %s: %s
$ 4452 ** NOT USED ** Unknown trace option: %s
4453 VSN specification missing
4454 Archive set name is too long (maximum %d)
4455 Path missing
4456 Path must be relative
4457 Invalid path
4458 File properties duplicate those in line %d
$ 4459 ** NOT USED ** Name expression too complicated
4460 Incorrect file name expression
$ 4461 ** NOT USED ** No arguments allowed
4462 '%s' missing
$              archmax | drives ...
4463 Duplicate %s definition
4464 Group name not in group file
4465 User name not in password file
$ 4466 ** NOT USED ** Unterminated string
$ 4467 ** NOT USED ** String is too long
$ 4468 ** NOT USED ** 1 error in '%s'
$ 4469 ** NOT USED ** %d errors in '%s'
4470 1 archive set has no volumes defined
4471 %d archive sets have no volumes defined
4472 1 archive set has no volumes available
4473 %d archive sets have no volumes available
4474 '%s' is not a valid copy option
4475 Release attributes missing
4476 Stage attributes missing
4477 Invalid release attribute %c
4478 Invalid stage attribute %c
$ 4479 ** NOT USED ** Invalid %s method
$ 4480 ** NOT USED ** 1 error in default configuration
$ 4481 ** NOT USED ** %d errors in default configuration
$ 4482 ** NOT USED ** Error in default
$ 4483 ** NOT USED ** Unable to create default configuration
$ 4484 ** NOT USED ** Reading reserved VSNs file
$ 4485 ** NOT USED ** Volume not found
4486 Archive set name missing
$ 4487 ** NOT USED ** VSN reserved by %s
4488 VSN pool name is too long (maximum %d)
4489 Duplicate VSN pool name %s
4490 VSN pool name missing
4491 VSN pool %s not defined
4492 Pool media does not match archive set
4493 Different media previously specified
$ 4494 ** NOT USED ** Reserve method missing
4495 Owner name missing
4496 File system name missing
4497 Cannot access %s file %s
$ 4498 ** NOT USED ** Recycle dataquantity missing
$ 4499 ** NOT USED ** Recycle hwm value missing
$ 4500 ** NOT USED ** Recycle mail address missing
$ 4501 ** NOT USED ** Recycle minimum gain value missing
$ 4502 ** NOT USED ** Recycler mail address is too long.  Maximum length is %d characters
$ 4503 ** NOT USED ** Invalid %s value
$ 4504 ** NOT USED ** %s value must be between 0 and 100%%
4505 '%s' parameters must be defined first
$ 4506 ** NOT USED ** Priority parameter name missing
4507 '%s' is not a valid priority name
4508 Priority parameter value missing
$ 4509 ** NOT USED ** Priority parameter value must be between %G and %G
$ 4510 ** NOT USED ** Ovflmin size missing
$ 4511 ** NOT USED ** Sort method missing
$ 4512 ** NOT USED ** Drivemin size missing
$ 4513 ** NOT USED ** Directory name missing
4514 Reserve time missing
$ 4515 ** NOT USED ** Recycle vsn count missing
$ 4516 ** NOT USED ** Archage specification missing
$ 4517 ** NOT USED ** Archive age specification must be 'access' or 'modify'
$ 4518 ** NOT USED ** Bufsize value missing
$ 4519 ** NOT USED ** Invalid bufsize value '%s'
$ 4520 ** NOT USED ** bufsize must be >= 2 and <= 32
4521 bufsize option must be 'lock'
4522 Disk archive VSN missing
4523 Disk archive VSN '%s' not defined
$ 4524 ** NOT USED ** Cannot define removable media for a disk archive
$ 4525 ** NOT USED ** No archiving for filesystem %s
$ 4526 ** NOT USED ** Reserve method missing
$ 4527 ** NOT USED ** Invalid %s
$ 4528 ** NOT USED ** Offline copy method missing
4529 %s has no volumes defined
$ 4530 ** NOT USED ** Drive count missing
4531 Name longer than %d characters
4532 No media available for default assignment
4533 Cannot assign disk to archive set in VSN associations section
$ 4534 ** NOT USED ** Disk archiving not allowed for metadata
4535 Cannot define disk archive when removable media is defined
4536 Logfile name longer than %d characters
4537 Examine value missing
4538 Invalid %s value ('%s')
4539 Fatal disk archiving error: %s (%s:%s) errno= %d
4540 '%s' is not allowed with disk archive
4541 '-disk_archive' params obsolete.\nAssign disk volumes to archive sets in the VSN Associations or Pools section using 'dk' as media type.
4542 State missing
4543 Timeout specification missing
4544 Timeout value missing
4545 Invalid recycling option for disk archive set
4546 Invalid recycling option for removable media archive set
4547 'archmax' is not a valid option for STK 5800 disk archiving
4548 Background interval value missing
4549 Background time value missing
4550 Invalid background time

$ Archiver lister output
$ 4600 * NOT USED ** %s: too many arguments
4601 usage:
4602 Filesystems
4603 You must be root to examine the file system.
4604 Not mounted
4605 *None*
$ 4606 ** NOT USED ** Trace output to
$ 4607 ** NOT USED ** Trace options
4608 Notify file
4609 Archive media
4610 not available
4611 Archive libraries
4612 drives_available
4613 archive_drives
4614 Archive file selections
4615 Filesystem
4616 Logfile
4617 Metadata
4618 Archive sets
4619 No volumes defined
4620 Catalog
4621 Volume overflow not selected
4622 Inodes allocated
4623 Free inodes
$ 4624 ** NOT USED ** Errors in archiver command file
$ 4625 ** NOT USED ** Bad media
$ 4626 ** NOT USED ** Write protected
$ 4627 ** NOT USED ** Read only
$ 4628 ** NOT USED ** Recycle
4629 VSN pools
4630 Priorities
$ 4631 ** NOT USED ** Data directory
4632 Recycling
4633 Interval
4634 Examine
4635 capacity:
4636 space:
4637 reserved:
4638 mount:
$ 4639 ** NOT USED ** Destination not available.
4640 media:
4641 (by default)
4642 Volumes:  all (by default)
4643 copy:
4644 Device:
4645 path:
4646 Volumes:
4647 Total space available:
4648 slot:
4649 Dictionary
4650 Cannot create archive detail file:
4651 Read timeout
4652 Request timeout
4653 Stage timeout
4654 Write timeout

$ showqueue messages
4800 GetFsStatus() failed
4801 No samfs filesystems
4802 Cannot access queue file %s %s
4803 Archive requests
4804 GetFsInfo(%s) failed
4805 Filesystem %s:
4806 no_archive
4807 not mounted
4808 No archive requests
$ 4809 ** NOT USED ** ArchReq %s version mismatch.  Is: %d, should be: %d
4810 Invalid ArchReq %s
4811 Scan list
4812 none
4813 empty
4814 Scanning
4815 full
4816 Archive Set file not available

$ archiverd and arfind control messages
4900 Unknown directive '%s'
4901 Unknown '%s' identifier '%s'
4902 %s value missing
4903 Invalid %s value
4904 %s is not a directory
4905 Invalid ArchReq - %s
4906 ArchReq %s not found

$  cmd          (5000s)
$ ===================================================================
5001 getting pathname
5002 getting base inode
5003 getting new resource info
5004 getting link length
5005 getting link resolution name
5006 getting inode extension
5008 putting vsn array
5010 %s: cannot open() following mkdir()
5011 %s: cannot write() link %s
5013 %s: Header record error: Bad version number (%d)
5014 %s: Unsupported revision for removable media info
5015 %s: cannot write() removable media information
5016 %s: cannot F_IDSTAT, %d.%d
5017 %s: File is now damaged, stale archive copy exists
5018 %s: Warning! File will be damaged, stale archive copy exists.
5019 %s: Dump file name not specified (-f)
5020 : Last undamaged offline copy
5021 Source copy and Destination copy must be specified
5022 %s: cannot read segment file.
5023 tape options not valid for this media
5024 %s: F_IDMVA failed, inode %d.%d
5025 %s: cannot read acl information
5026 %s: cannot write acl information
5027 putting acl array length
5028 putting acl array
5029 getting acl array length
5030 getting acl array
5031 %s: Warning! File is inconsistent, inconsistent archive copy exists.
5032 %s: File has inconsistent copy.
5033 %s: Warning! File is inconsistent.
5034 %s: segment index is empty or invalid, size %d.
5035 %s: %d of %d segments are missing, cannot dump file.
5036 StorADE support is on.
5037 StorADE support is off.
5038 usage: samstorade [-r hostname] [-t timeout] [-s xml_message] [-d]


$  gencmd

5100 Setting segment attributes for %s\n
5101 Setting file attributes for %s\n
5102 -l and -L are mutually exclusive
5103 allocahead must be greater than 1 megabyte
5104 segment size must be greater than 1 megabyte
5105 Bad or missing -A argument
5106 Bad or missing -l or -L argument
5107 %s is a directory but -M not specified
5108 cannot specify -n with -a.
5109 Bad or missing %c argument
5110 -e and -E are mutually exclusive
5111 You must be root to set data verification
5112 Data verification error %d for %s

$  odlabel/tplabel

$  gui          (6000s)
$ ===================================================================
6000 SAM-FS is not running.  Unable to start libmgr.
6001 SAM-FS was stopped, libmgr exiting.

$  samu       (7000s)
$ ===================================================================

7000 SAM-FS is not running.
7001 SAM-FS is not running - type q to quit.
7002 Unknown devlog event: %s
7003 "    more (ctrl-f)"
7004 base: 0x%llx
7005 You must supply a base-16 address for the C command.
7006 Not a SAM-FS file system or not mounted (%s).
7007 Not a valid inode number (%s).
7008 Invalid equipment %s for this command.
7009 Cannot execute command %s for device %s.
7010 Cannot change disk device state from client.

$  samu command help screen   (7050s)
$ ===================================================================

7050 eq value
7051 File System commands - SAM:
7052 File System commands - I/O:
7053 Set number of consecutive dio reads
7054 Set size of well-formed dio reads
7055 Set size of ill-formed dio reads
7056 Set number of consecutive dio writes
7057 Set size of well-formed dio writes
7058 Set size of ill-formed dio writes
7059 Turn on directio mode
7060 Turn off directio mode
7061 Turn on setuid capability
7062 Turn off setuid capability
7063 Turn on software RAID mode
7064 Turn off software RAID mode
7065 Set stripe width
7066 Set sync_meta mode
7067 Set flush behind value in kilobytes
7068 Turn on hwm archiver start
7069 Turn off hwm archiver start
7070 Set maximum partial size in kilobytes
7071 Set partial stage-ahead point in kilobytes
7072 Set stage flush behind size in kilobytes
7073 Set direct stage size in kilobytes
7074 Set number of stage retries
7075 Set outstanding write size in kilobytes
7076 Set append lease time
7077 Turn on multihost read/write
7078 Turn off multihost read/write
7079 Set minimum allocation size
7080 Set maximum allocation size
7081 Set meta stripe width
7082 Turn on qwrite mode
7083 Turn off qwrite mode
7084 Set read lease time
7085 Set write lease time
7086 Set multireader invalidate cache delay
7087 Turn on NFS async
7088 Turn off NFS async
7089 Turn on refresh at eof mode
7090 Turn off refresh at eof mode
7091 File System commands - QFS:
7092 File System commands - multireader:
7093 File System commands - shared fs:
7094 Enable Application Based Recovery
7095 Disable Application Based Recovery
7096 Enable Directed Mirror Reads
7097 Disable Directed Mirror Reads
7098 Turn on dio sparse zeroing
7099 Turn off dio sparse zeroing


7100 Device commands:
7101 Robot commands:
7102 Archiver commands:
7103 Miscellaneous commands:
7104 Media types:
7105 File System commands - miscellaneous:
7106 Stager commands:
7107 eq interval
7108 [-e] eq[:slot[:side]]
7109 eq:slot[:side]
7110 eq
7111 eq [option ...]
$ 7112 ** NOT USED ** eq contig
$ 7113 ** NOT USED ** eq partial
7114 eq high low
7115 [-f] mt.vsn
7116 [-f] eq:slot
$ 7117 ** NOT USED ** [option ...]
7118 vsn [index]
7119 mountpoint
7120 [interval]
7121 address
7122 [filename]
7123 ""
7124 [filesystem]
7125 [media]
7126 [eq]
7127 [address]
7128 [inode]
7129 [mountpoint]

7130 Set device logging options
7131 Down device
7132 Idle device
$ 7133 ** NOT USED ** Set maximum readahead/writebehind in units of 16k blocks
7134 Set size to remain online in kilobytes
7135 Turn off device
7136 Turn on device
7137 Make device read only
7138 Set high and low release thresholds
7139 Make device unavailable
7140 Unload device
7141 Audit slot or library
7142 Import cartridge from mailbox
7143 Export cartridge to mailbox
7144 Load cartridge in drive
7145 Restart archiver
7146 Start archiving
$ 7147 ** NOT USED ** Set trace options
7148 Clear load request
7149 Select a mount point
7150 Open device
7151 Exit from samu
7152 Set display refresh interval in seconds
7153 Read sector from device
7154 Snapshot screen to file
7155 shell-command
7156 Run command in a shell
7157 Set maximum readahead in kilobytes
7158 Set maximum writebehind in kilobytes
7159 [filesystem]
7160 Select a filesystem name (ex samfs1)
7161 Set shared fs meta cache timeout
7162 [dk | rm | fs.fsname]
7163 Idle archiving
7164 Stop archiving
7165 Turn off file system tracing
7166 Turn on file system tracing
7167 daemon[.variable] value
7168 mt.vsn
7169 Clear stage request
7170 [fs.fsname]
7171 Trace archiver
7172 fsname[.dir | ..inodes][int]
7173 Scan filesystem
7174 Soft restart archiver
7175 Idle staging
7176 Start staging
7177 fsname.[* | arname]
7178 Remove ArchReq(s)
7179 pid priority
7180 Set priority in preview queue
7181 Bad priority
7182 pid %d not found
7183 volume +flag | -flag
7184 Set or clear disk volume flags
7185 Set shared fs lease relinquish timeout
7186 Set default WORM retention time
7187 Set shared fs minimum threads count
7188 Set access time (atime) update mode
7189 Disable allocation on partition
7190 Enable allocation on partition
7191 Add eq to mounted file system
7192 Remove eq1; move to eq2 if specified
7193 Release eq and mark files offline
7194 eq1 [eq2]

$  samu display help screen   (7200s)
$ ===================================================================

7200 Archiver status
7201 Device configuration
7202 Help information
7203 Usage information
7204 Mass storage status
7205 Staging status
7206 Optical disk status
7207 Removable media load requests
7208 Removable media
7209 Device status
7210 Tape drive status
7211 Robot catalog
7212 Archiver state file
7213 Memory
7214 Optical disk label
7215 Inode
7216 Preview shared memory
7217 Shared memory tables
7218 Shared memory
7219 File system parameters
7220 SAM-Remote
7221 Sector data
7222 SCSI sense data
7223 Device table
7224 Kernel statistics
7225 Staging queue
$ 7226 ** NOT USED ** Stager status
7227 Pending stage queue
7228 File systems
7229 Daemon trace controls

7230 "Hot Keys:"
7231 "    q\tQuit"
7232 "    :\tEnter command"
7233 "    sp\tRefresh display"
7234 "    ^f\tPage display forward"
7235 "    ^b\tPage display backward"
7236 "    ^d\tHalf-page display forward"
7237 "    ^u\tHalf-page display backward"
7238 "    ^k\tAdvance display format"
7239 "    ^l\tRefresh display (clear)"
7240 "    ^r\tToggle refresh"
7241 "    ^i\tShow details (selected displays)"
7242 "    /\tSearch for VSN (v display)"
7243 "    %%\tSearch for barcode (v display)"
7244 "    $\tSearch for slot (v display)"

7249 Shared clients

7250 "Media types:"
7251 "    all\tAll media types"
7252 "    tp\ttape"
7253 "    d2\tAmpex DST310 (D2) tape"
7254 "    d3\tSTK SD-3 tape"
7255 "    dt\t4mm digital tape (DAT)"
7256 "    i7\tIBM 3570 tape"
7257 "    ib\tIBM 3590 tape"
7258 "    lt\tdigital linear tape (DLT)"
7259 "    se\tSTK 9490 tape"
7260 "    so\tSony DTF tape"
7261 "    st\tSTK 3480 tape"
7262 "    vt\tMetrum VHS tape"
7263 "    xt\tExabyte 8mm tape"
7264 "    od\toptical"
7265 "    mo\t5 1/4 in. erasable optical disk"
7266 "    wo\t5 1/4 in. WORM optical disk"
7267 "    o2\t12 in. WORM optical disk"
7268 "    sg\tSTK 9840 tape"
7269 "    at\tSony AIT tape"
7270 "    fd\tFujitsu M8100 128track tape"
7271 "    li\tIBM 3580, Seagate Viper 200 (LTO)"
7272 "    sf\tSTK T9940 tape"
7273 "    xm\tExabyte Mammoth-2 8mm tape"
7274 "    sa\tSony Super AIT tape"
7275 "    m2\tIBM 3592 tape"

$  samu other displays (7300s)
$ ===================================================================

7300 copy
7301 "ty   eq state   device_name\t\t\t   fs family_set"
7302 "ty     eq state   device_name\t\t\t     fs status"
7303 "ty   eq  status      act  use  state \tvsn"
7304 format
7305 incore
7306 "No usage data"
7307 "Remote server"
7308 "Remote client"
7309 "Unknown Remote device"
7310 message
7311 Client
7312 unknown
7313 cache size
7314 cache left
7315 max file size
7316 min file size
7317 wait for cache
7318 bypass cache
7319 abort no cache
7320 cache action
7321 client index
7322 network block size
7323 flags
7324 no-cache
7325 connected
7326 wait-cache
7327 Client IPv

7328 "              label  time    capacity      space  blocksize barcode"
7329 "index type pid     priority     rb   flags     wait count  vsn"
7330 count
7331 all
7332 "index type pid     user         rb   flags     wait count  vsn"
7333 *** NOT USED *** "ty      eq  status       use state ord"
7334 "  capacity      free    ra  part high low"
7335 "ty   eq    pid  file_size  staged_in  fseq        ino   position    offset"
7336 both
7337 manual
7338 robot
7339 priority

7340 "slot       "
7341 "count      "
7342 "% used     "
7343 "VSN        "
7344 "access time"
7345 "barcode    "
7346 "label time "
7347 "           "
7348 "         reservation time reserved archive set/owner/filesystem location"
7349 port

7350 Stage queue for media type
7351 files queued
7352 "index ty   eq    pid  file_size  fseq        ino   position     offset"
7353 Archive: %d  files: %d (%d)  bytes: %9s
7354 barcode %s not found
7355 "Search for VSN: "
7356 "Search for barcode: "
7357 VSN %s not found
7358 "Search for slot: "
7359 Slot %s not found

7360 Preview queue for media type
7362 "index ty prev    pid  file_size  fseq        ino   position     offset vsn"
7363 *** NOT USED *** "ty     eq  state           device_name      status high low mountpoint server"
7364 regular files
7365 offline files
7366 archdone files
7367 directories
7368 total
7369 Archiver daemon is not running.

7370 Stager daemon is not running.
7371 Log output to: %s
7372 *** NOT USED *** Stage errors
7373 ** NOT USED ** Stage request: %s.%s
7374 No active requests.
7375 "ty     length  fseq        ino   position     offset vsn"
7376 volumes %d files %d
7377 Staging queue by media type: %s
7378 Pending stage queue by media type: %s
7379 active
7380 pending
7381 ** NOT USED ** Staging queues
7382 ty pid    user         status     wait files vsn
7383 kernel statistics not available
7384 module: %s  name: %s instance: %d class: %s
7385 Unknown kstat data type %d
7386 Unknown kstat type %d
7387 Active Services
7388 spm_query_services failed: %d: %s
7389 Registered services for host '%s':
7390 "  %d service(s) registered."
7391 %s %s: capacity %s bytes space %s bytes, usage %3d%%
7392 "library   "
7393 totals
7394 filesystem
7395 hostid: %x  OS name: %s  Architecture: %s CPUs: %d (%d online)
7396 client
7397 server
7398 Stage request %d: %s.%s
7399 Staging queues starting at %d
7400 Disk volume %s not found
7401 Unrecognized disk volume flag '%c'
7402 No statistics available if examine = noscan
7403 Use 'archiver -f [-n filesystem]' to get statistics.
7404 maxactive %ld curr %ld

7410 Server
7411 Server IPv
7412 "ty      eq   state          device_name      status high low mountpoint server"
7413 "ty      eq  status       use state  ord"

7420 "%s is shared, server is %s, %d clients %d max"
7421 "ord hostname              seqno nomsgs status   config  conf1  flags"

$  releaser     (8000s)
$ ===================================================================
8000 -wSV[Releaser:CmdErr] Run \"%s\" manually for details and correct problem.
8001 -wS Filesystem \"%s\" not mounted.
8002 Releaser shutdown by SIGINT
8003 -wV[Releaser:CmdErr] Errors in \"%s\" commands.

8020 Error converting \"%s\" to \"%s\"
8021 Value \"%f\" for \"%s\" must be >= 0.0 and <= 1.0
8022 Cannot mix weight_age and any of weight_age_access, weight_age_modify, or weight_age_residence
8023 -wS %s

$  robots       (9000s)
$ ===================================================================
9001 %s(%d): not started: device off or down.
9002 %s: not running: device off or down
9003 -eV[Dev:MChangerFull] can not clear drive, media changer full
9004 %s(%d): Media changer full.
9006 can not clear drive, move failed
9007 %s(%d): Move failed.
9008 reading barcode
9009 waiting for media changer
9010 drive cleared
9011 clearing transport
9012 -eV[Dev:MChangerFull] can not clear transport, media changer full
9013 transport cleared
9014 clear import/export element
9015 -eV[Dev:MChangerFull] can not clear import/export element, media changer full
9016 can not clear import/export element, move failed
9017 import/export element cleared
9018 catalog name not set
9019 %s: Catalog name not set.
9020 unable to open catalog
9021 unable to mmap catalog
9022 initializing elements
9023 duplicate labels, see sam-log
9024 ** NOT USED ** Duplicate label(%d):%s slots %d and %d attempting to export slots %d
9025 needs cleaning
9026 no cleaning cartridge available
9027 %s:(%d): No cleaning cartridge available.
9028 %s:(%d): No cleaning cartridge found.
9029 unable to load cleaning cartridge, move failed
9030 waiting for cleaning cycle
9031 attempt to unload cleaning cartridge
9032 unable to unload cleaning cartridge
9033 %s(%d): Unable to unload cleaning cartridge.
9034 drive has been cleaned
9035 cleaning cartridge in slot %d has expired
9036 ** NOT USED ** Cleaning cartridge in slots %d has expired.
9037 ** NOT USED ** Cleaning cartridge in slots %d has expired: exporting.
9038 import/export not supported
9039 %s: Import/export not supported.
9040 no empty storage slots for import
9041 ** NOT USED ** %s: No empty slots for import.
9042 no empty export elements
9043 %s: No empty export elements.
9044 vsn not found
9045 ** NOT USED ** Slot number is out of range
9046 equipment not found
9047 unknown request
9048 ** NOT USED ** slot not occupied or unavailable
9049 unable to find media
9050 media is in a drive and the drive is busy
9051 export hopper full
9052 %s: Export hopper full.
9053 place media in import area
9054 remove media from export area
9055 ** NOT USED ** %s: Imported media has a duplicate barcode (%s) with slot %d, exporting new media.
9056 imported media has duplicate barcode
9057 %s: Imported media has no barcode.
9058 import failed, check import/export hopper
9059 import halted, check log
9060 ** NOT USED ** %s: Imported media duplicates vsn %s in slot %d, exporting new media.
9061 unloading drives for export
9062 ** NOT USED ** imported media (slot %d) has no barcode
9063 imported media (slot %d) has barcode %s
9064 media changer reports no access
9065 initializing
9066 flip media
9067 unloading %s
9068 dismount %s/mount %s
9069 mounting %s
9070 mount of %s failed
9071 error on test unit ready
9072 Error on test unit ready.
9073 error on initialize element
9074 Error on initialize element.
9075 mechanical hindrance
9076 Mechanical hindrance.
9077 needs operator attention
9078 Unable to clear transport.
9079 move from %s to %s %s
9080 storage
9081 drive
9082 import/export
9083 transport
9084 invert
9085 unknown
9086 exchange from %s to %s %s to %s %s
9087 move complete
9088 exchange complete
9089 %s: Media changer does not support move.
9090 %s: Slot (%d) is out of range.
9091 %s: Source equals destination.
9092 ** NOT USED ** %s: Source slot(%d) is not occupied.
9093 ** NOT USED ** %s: Destination slot(%d) is occupied.
9094 ** NOT USED ** %s: Source slot(%d) is unavailable.
9095 ** NOT USED ** %s: Destination slot(%d) is unavailable.
9096 %s: Slot (%d) is not occupied.
9097 %s: Out of Cleaner Volumes.
9098 %s: Automated Operational State.
9099 %s: Paused Operational State.
9100 %s: Manual Mode Operational State.
9101 %s: Degraded Operation.
9102 %s: Safety Enclose Interlock Open.
9103 %s: Vision system Non-Operational.
9104 %s: Offline.
9105 %s: Intervention Required.
9106 %s: Library Manager Check 1 Condition.
9107 %s: All Storage Cells Full.
9108 %s: Dual Write Disabled.
9109 %s: Environmental Alert -- smoke detected.
9110 %s: Managed Manual Mode Operational State.
9111 %s: Duplicate volser found %s, exporting.
9112 %s: Catalog full, exporting %s.
9113 Audit not supported on ibm349x.
9114 %s: Setting device (%d) off due to LMC error.
9115 %s: Library reports device %#x(%s) as not available.
9116 %s: Library reports device %#x(%s) as not installed.
9117 %s: Unable to open configuration file(%s): %m.
9118 Unable to open configuration file
9119 %s: access id too long - ignored.
9120 %s: Syntax error in stk configuration file line %d.
9121 %s: STK configuration error line %d: Index error.
9122 %s: ACS specification error line %d.
9123 %s: ACS range error line %d.
9124 %s: LSM specification error line %d.
9125 %s: LSM range error line %d.
9126 %s: PANEL specification error line %d.
9127 %s: PANEL range error line %d.
9128 %s: DRIVE specification error line %d.
9129 %s: DRIVE range error line %d.
9130 %s: Unknown keyword(%s) line %d.
9131 %s: Drive %d set down due to configuration error.
9132 %s: Cannot find aci drive name line %d.
9133 %s: CAP specification error line %d.
9143 Capacity for media type index %d is %dK.
9144 %s: No entry for drive %d in configuration file.
9145 not in configuration file
9146 dismount %s
9147 Drive %d is a shared drive.
9150 %s: Catalog size inconsistent with number of entries.
9151 waiting for helper to start
9152 helper running as pid %d
9153 drive set down due to ACS reported state
9154 %s: Drive (%d) set down: ACS reported state.
9155 waiting for %d drive(s) to initialize
9156 %s: Waiting for %d drive(s) to initialize.
9157 %s: %d drive(s) did not initialize.
9158 Audit not supported on stk.
9159 %s: Setting device (%d) down due to ACS error.
9160 ** NOT USED ** %s: vsn already exists in catalog at slot %d.
9161 ** NOT USED ** %s: barcode already exists in catalog at slot %d.
9162 ** NOT USED ** %s: no empty storage slots.
9163 %s: %s in not home.
9164 %s: vsn (%s) not found.
9165 %s: Device(%d) not found.
9166 %s: Device(%d) is empty.
9167 %s: Volume from slot %d is mounted and drive(%d) is busy.
9168 close import/export door
9169 unable to lock import/export door
9170 %s: Unable to lock import/export door.
9171 Audit not supported on IBM.
9172 %s: Setting device (%d) off due to ACL error.
9173 device off due to ACL error
9174 %s: Drive (%d) access denied.
9175 drive access denied
9176 %s: Syntax error in aci configuration file line %d.
9178 fatal error during initialization, see log
9179 %s: Drive name (%s) too long line %d.
9180 %s: Cannot find drive(%s) line %d.
9181 %s: No acidrive entry for drive(%s) %d.
9182 %s: Client or server not defined.
9183 %s: Setting API environment %s.
9194 api response error
9195 %s: errno %d, d_error %d.
9196 protocol error
9197 d_errno(%d): Range error 0-%d.
9198 %s: Add to catalog failed: %s: %s.
9199 Audit not supported on GRAU.
9200 unable to allocate catalog space
9201 device is empty
9202 %s: Media changer reports %d drives, mcf has %d.
9203 %s: Unable to unload drive(%d).
9204 %s: Move media failed: Media changer in unknown state.
9205 idle
9206 empty
9207 %s: Setting device (%d) off.
9208 writing label %s on %s
9209 %s: Volume on %d, barcode %s: Not labeled.
9210 %s: Volume on %d: Not labeled.
9211 %s: Slot %d is write protected, unavailable, read only, not present, or not SAM media.
9212 Slot %d is write protected, unavailable, read only, not present, or not SAM media.
9213 %s: Cannot label media in slot %d: Waiting for audit.
9214 Cannot label media in slot %d - waiting for audit
9215 %s: Cannot label cleaning cartridge in slot %d.
9216 cannot label cleaning cartridge
9217 -iSV[Dev:CannotLabel] %s(%d): Unable to label media from slot %d, barcode %s: Readonly.
9218 unable to label media from slot %d, barcode %s - readonly
9219 -iSV[Dev:CannotLabel] %s(%d): Unable to label media from slot %d: Readonly.
9220 unable to label media from slot %d - readonly
9221 %s(%d): Volume from slot %d is already labeled.
9222 media from slot %d is already labeled
9223 %s(%d): Cannot label: VSN %s is flagged as bad media.
9224 cannot label - %s is flagged as bad media
9225 %s(%d): Invalid state change.
9226 %s(%d): Incorrect state.
9227 %s: Label request for slot %d: Slot is empty or busy.
9228 slot is empty or media is busy
9229 %s: Barcode label request: slot %d has no barcode.
9230 %s: %s is already mounted on %d.
9231 media has no barcode
9232 slot contains a cleaning cartridge
9233 -iSV[Dev:DuplicateVSN] %s: Label request for %s would duplicate vsn at slot %d.
9234 request would duplicate vsn at slot %d
9235 %s: Slot %d is empty.
9236 %s: Slot %d is a cleaning cartridge.
9237 media is already mounted
9238 %s: Cannot find device %d.
9239 cannot find device
9240 %s: Device %d: State must be set to unavail.
9241 state must be set to unavail
9242 %s: Unload library not supported.
9243 unload library not supported
9244 %s: Slot not found in drive.
9245 Slot not found in drive
9246 unable to create catalog
9247 Historian is running in attended mode.
9248 Historian is running in unattended mode.
9249 Historian has switched to attended mode.
9250 Historian has switched to unattended mode.
9251 waiting for catalog to increase
9252 Must be placed in random mode.
9253 catalog size inconsistent
9254 Error in configuration file
9255 import complete
9300 No device for media type: %s.
9301 -cSV[Dev:DevDown] Device %d:  State set to DOWN by SAM-FS.  Check device log.
9302 SAM-FS attempted to set device %d to off.
9303 Device %d, VSN %s:  Bad media.  Check device log.
9304 Device %d:  State set to ON.
9305 -eV[Dev:CannotRelabel] Can not relabel WORM media.
9306 transport not cleared
9307 move failed
9308 Clear Operation Timeout [%d/%d]
9309 media VSN %s at slot %d not same as old VSN %s
9310 %s(%d): Volume VSN %s at slot %d not same as old VSN %s
9311 -cSV[Dev:DevDown] User attempted to set device %d to DOWN.
9322 Device %d:  State set to OFF by user.  Check device log.
9323 Cannot label: %s
9324 %s(%d): %s
9325 Drive supports media as read-only, setting write protect
9326 No volume information available for view media.
9327 SONY device %d has requested a RECOVER operation.
9328 Data loss may result.
9329 Contact hardware vendor on how to proceed.


$ The following messages have been added from dev_log.msg
$ The long term goal is to have all messages only in catalog.msg
$ These messages have been added to support SendCustMsg

9330 -eV[Dev:LibFull] Cannot clear drive, library full
9331 -eV[Dev:LibFull] Cannot clear robot %d, library full
9332 -eV[Dev:LibFull] Cannot clear mailbox %d, library full
9333 -eV[Dev:LibFull] Import failed: library full
9334 -eV[Dev:LibFull] Library full
9335 -eV[Dev:CannotLabel] Unable to label
9336 -eV[Dev:CannotLabel] Unable to label: Old VSN %s, New VSN %s
9337 -aV[Dev:CannotRewind] Unable to rewind
9338 -eV[Dev:CannotRewind] Unable to rewind: Old VSN %s, New VSN %s
9339 -eV[Dev:CannotErase] Unable to erase
9340 -eV[Dev:CannotSkip] Unable to open (%d) or skip to EOM (%d)
9341 -eV[Dev:CannotGetCapacity] Unable to determine capacity
9342 -eV[Dev:CannotGetCapacity] Unable to determine capacity: invalid data'
9343 -eV[Dev:CannotGetCapacity] Unable to determine capacity (Log Sense command failed)
9344 -eV[Dev:CannotGetCapacity] Unable to determine capacity: does not support function
9345 -eV[Dev:CannotUnload] Unable to unload drive: move failed
9346 -eV[Dev:CannotUnload] Unable to unload drive
9347 -eV[Dev:NoCCartridge] No cleaning cartridge available
9348 -eV[Dev:FailSpinup] Spin Up fails

9349 ** NOT USED **
9350 Slot %d in eq %d is empty; empty slot can not be in drive.
9351 Slot %d in eq %d is occupied; media can not be occupied in its slot and in the drive at the same time.
9352 Volume is in drive %d that is off or down.
9353 Volume (%d:%d) in drive %d set to Unavailable.
9354 ** NOT USED ** Unable to gain access to SAM-Remote clients.
9357 Unable to determine media type. Library daemon exiting.
9358 -eV[Dev:DriveDown] Drive set to down, cannot determine if WORM or RW media loaded.
9359 -eV[Dev:CannotLabel] Unable to relabel used VolSafe media
9360 Mount of %s failed, drive %d downed.
9454 %s: Drive (%d) set down: PSC reported state.
9459 %s: Setting device (%d) down due to PSC error.
9400 -eV[Dev:LoadInterrupt] SAM-FS needs VSN %s to be manually loaded or imported
9401 -eV[Dev:DriveDown] Drive set to down, drive is in CD-ROM emulation mode.
$  rpc          (10000s)
$ ===================================================================


$  sam-amld     (11000s)
$ ===================================================================
11001 SAM-FS has shutdown, shared memory invalid
$ 11002 ** NOT USED ** %s: Unable to resolve filename (%s): %s.\n
$ 11003 ** NOT USED ** %s: Path to defaults.conf too long. Exiting.
$ 11004 ** NOT USED ** defaults.conf: Bad value(%s) for %s: line %d: Exiting.
$ 11005 ** NOT USED ** Error in /etc/name_to_sysnum for samsys line %d, exiting.
$ 11006 ** NOT USED ** defaults.conf: Unknown value(%s) for %s: line %d. Exiting.
11007 sam-amld may be started only by sam-fsd
11008 Add catalog, vsn not supplied.
11009 defaults.conf: Keyword missing: line %d. Exiting.
11010 defaults.conf: Value missing: line %d. Exiting.
11011 defaults.conf: Must specify %s media type: line %d. Exiting.
11012 defaults.conf: Operator group %s not found: Exiting.
11013 defaults.conf: Unknown device type(%s): line %d. Exiting.
11014 License prevents labeling more media.
11015 defaults.conf: Unknown keyword(%s): line %d. Exiting.
11016 Waiting for file system to resync.
11017 License prevents importing more media.
11018 ** NOT USED ** License: unable to open license file
11019 ** NOT USED ** License file format incorrect
11020 ** NOT USED ** License data incorrect
11021 ** NOT USED ** License: hostid does not match license
11022 ** NOT USED ** License expires %s
11023 ** NOT USED ** License never expires for %d slots
11024 ** NOT USED ** License - D E M O   S Y S T E M - %d media slots
11025 ** NOT USED ** License for %d slots, expires %s
11026 ** NOT USED ** License will expire %s
11027 ** NOT USED ** Slots (%d) allowed by license used up
11028 ** NOT USED ** Only %d slots available until license exceeded
11029 ** NOT USED ** Licensed slots equals number of occupied slots
11030 ** NOT USED ** Number of slots exceeds license by %d
11031 ** NOT USED ** License expired
11032 ** NOT USED ** License will expire %s
11033 ** NOT USED ** License key is inconsistent
11034 ** NOT USED ** License is unavailable
11035 ** NOT USED ** License: Cannot get host id from system
11036 ** NOT USED ** License: License expires in less than 1 hour
11037 ** NOT USED ** License: License expires in less than 1 day
11038 ** NOT USED ** License: License expires in less than 1 week
11039 ** NOT USED ** License: License expires in less than 30 days
11040 ** NOT USED ** License:  %s is licensed for %d slots, using %d.  License is suspended.
11041 ** NOT USED ** License: License never expires.
11042 ** NOT USED ** License: License file modification time is impossible.
11043 ** NOT USED ** License: License does not support SAM-remote server.
11044 ** NOT USED ** License: Removing pseudo device from device table.
11045 ** NOT USED ** License: License does not support SAM-remote client.
11046 ** NOT USED ** License: License does not support Migration Toolkit
11047 %s: %s exited with status %d (%s)
11048 %s: %s(%d) exited with status %d (%s)
11049 %s: %s will not be restarted
11050 License: License does not support 'ma' file system: %s aborts
11051 Cannot make device log directory
$ 11052 ** NOT USED ** No samsys entry in /etc/name_to_sysnum, exiting.
11053 ** NOT USED ** Mount: License has expired or is corrupted; mount failed.\n
11054 defaults.conf: Obsolete keyword (%s): line %d. \
	  Move to /etc/opt/SUNWsamfs/samfs.cmd.\n
11055 sam-amld:  the directory /var/adm does not exist.  Create it and retry.
11056 sam-amld:  /var/adm/log is not a directory.  Fix and retry.
11057 sam-amld:  Could not create /var/adm/log.  Fix and retry.
11058 hostid = %x\n
11059 ** NOT USED ** expiration date = %s\n
11060 ** NOT USED ** Demo license expires %s\n
11061 ** NOT USED ** License never expires\n
11062 ** NOT USED ** Remote sam server feature enabled\n
11063 ** NOT USED ** Remote sam client feature enabled\n
11064 ** NOT USED ** Migration toolkit feature enabled\n
11065 ** NOT USED ** Fast file system feature enabled\n
11066 ** NOT USED ** Robot type %s is
11067 ** NOT USED ** present and licensed\n
11068 ** NOT USED ** not licensed\n
11069 ** NOT USED ** licensed\n
11070 ** NOT USED ** not present and not licensed\n
11071 ** NOT USED **     %d %s slots
11072 ** NOT USED ** QFS stand alone %sfeature enabled\n
11073 ** NOT USED ** Data base feature enabled\n
11074 ** NOT USED ** Direct media access feature enabled\n
11075 ** NOT USED ** Foreign tape support enabled\n
11076 XX-UNUSED-XX Shared SAN filesystem support enabled\n
11077 Segment feature enabled\n
11078 %s (pid %d) dumped core\n
11079 Shared filesystem support enabled\n
11080 SAN API support enabled\n
11081 License: License does not support 'shared' file system: %s aborts
11082 ** NOT USED ** License: Cannot read license file
11083 ** NOT USED ** License:  %s is licensed for %d %s slots, using %d.  License is suspended.
11084 ** NOT USED ** License: License expires %d days after file system mkfs.
11085 ** NOT USED ** License expires in less than %d days.
11086 WORM fs support enabled\n
11087 ** NOT USED ** License expires %d days after file system mkfs.


$  lib-samfs    (12000s)
$ ===================================================================
12000 %s: %s has invalid devices.\n
12001 %s: %s has no devices.\n
12002 'device to mount' %s must match 'family set' %s.
12003 %s: %s family is duplicated (check %s).\n
12004 %s: %s family is missing from mcf (check %s).\n
12010 %s: bad token '%s' at line %d of %s
12011 %s: %d error(s) in command file %s
12012 %s: error converting '%s' to %s at line %d of %s
12013 %s: out of range value %s for command %s at line %d of %s
12014 %s: invalid string \"%s\" for command %s at line %d of %s
12015 %s: malformed token at line %d of %s
12016 %s: unknown keyword at line %d of %s
12017 %s: extra tokens at line %d of %s
12018 %s: missing value at line %d of %s
12019 %s: %s in command file at line %d.
12020 %s: Cannot get sam-fsd lock
12021 Configuring file system


$  lib-fs    (12500s)
$ ===================================================================
12500 XX-UNUSED-XX Host path name '%s' exceeds %d characters
12501 XX-UNUSED-XX Invalid server '%s' priority '%s'
12502 XX-UNUSED-XX Invalid host '%s' stager priority '%s'
12503 XX-UNUSED-XX Host '%s' unrecognized keyword '%s'
12504 XX-UNUSED-XX Too many host entries (%d max)
12505 XX-UNUSED-XX Duplicate host name '%s'
12506 XX-UNUSED-XX Server '%s' has server priority zero
12507 XX-UNUSED-XX Multiple hosts ('%s') declared server

12510 Unexpected character in hosts file
12512 Couldn't allocate memory to scan hosts file
12513 Couldn't allocate memory for hosts file processing
12514 Unexpected token in optional 'server' field
12515 Multiple metadata servers declared
12516 Unexpectedly short hosts file line
12517 Unexpected tokens after optional 'server' field
12518 Duplicate host name
12519 Duplicate host IP address
12520 Host name too long
12521 Field too long
12522 Unexpected character in host address list
12523 Unexpected character in numeric field
12524 Unexpected character in host name
12525 ReadCfg %s failed
12526 gethostname failed - %s

$  utility      (13000s)
$ ===================================================================
13001 Usage: %s %s\n
$ 13002 ** NOT USED ** Error in mcf file (%s) at line (%d):
13003 Invalid equipment ordinal (%s)
$ 13004 Invalid device type (%s)
$ 13005 Only one historian allowed (%s)
$ 13006 Invalid device state (%s)
$ 13007 Catalog path name exceeds %d characters (%s)
$ 13008 Excess input
$ 13009 First 3 parameters are required
$ 13010 Equipment ordinal (%d) already in use
$ 13011 Raw special file path missing
$ 13012 Family set name missing
$ 13013 Family set (%s) already in use by equipment (%d)
$ 13014 Catalog file name missing
13015 Device type mismatch eq %d: mcf = %s, inq = %s\n
13016 slot    access_time  count  use  ty vsn\n
13017 Usage: %s %s nnn\n
13018 %s: Incorrect number of arguments\n
13019 %s: Numeric field contains non-numeric character: %s\n
13020 unable to fstat() catalog %s
13021 %s: equipment number must be positive: %s\n
13022 Device path name exceeds %d characters (%s)
13023 Usage: %s devlog eq [events...]\n
13024 %s: %s: equipment %s: out of range.\n
13025 %s: %s: equipment %s: not present.\n
13026 %s: unknown devlog event: %s\n
13027 Cannot open mcf file (%s)
13028 Seek failed on eq %d at block 0x%llx
13029 Write failed on eq %d at block 0x%llx, length = %d
13030 Could not read VTOC or EFI label on %s: %d
13031 file ...
13032 %s %s %ld files, %lld bytes, %ld damaged copies\n
13033 with a sample rate of
13034 sef
13035 %s: equipment %d is not tape device.\n
13100 Invalid samstorade setting '%s'
13101 Resync sam-amld succeeded.
13102 Invalid sef equipment ordinal '%s'
13103 Equipment ordinal %d not tape
13104 Invalid sef setting '%s'
13106 Invalid tapeclean equipment ordinal '%s'
13107 Invalid tapeclean equipment ordinal '%s'
13108 Equipment ordinal %d not tape
13109 Invalid tapeclean setting '%s'
13110 Invalid tapealert equipment ordinal '%s'
13111 Invalid tapealert equipment ordinal '%s'
13112 Equipment ordinal %d not tape or media changer
13113 Invalid tapealert setting '%s'
13114 Invalid sef equipment ordinal '%s'
13115 Tapeclean feature requires 'autoclean' option specified
13200 %s requires parameter.\n
13201 %s must be greater than %d and less than %d.\n
13202 Usage: set_state [-w] state eq\n
13203 Usage: %s [-w] -e n | -vsn xxx eq\n
13204 %s error: %s\n
13205 %s: -e and -n are mutually exclusive.\n
13206 set_state error: %s\n
13207 Usage: import [-v volser | -s pool -c num -l ] [-b barcode][-m type] [-e | -n] eq\n
$ 13208 ** NOT USED ** mount option '%s' is not supported for ms filesystem.
13209 %s is not supported for standalone (QFS) filesystem.
13210 %s and %s are mutually exclusive.
13211 ** NOT USED ** %s: File data cannot be recovered (file is marked damaged).
13212 ** NOT USED ** %s: File data was not recoverable prior to dump (file is marked damaged).
13213 ** NOT USED ** %s: File data was previously not recoverable (file is marked damaged).
13214 ** NOT USED ** %s: File data will not be recoverable (file will be marked damaged).
13215 %s: -l option only allowed if importing by pools.\n
13216 %s: SAM-FS is running.  Shut it down and try again.\n
13218 ** NOT USED ** Slot not specified.
13219 Must specify slot for D2 media.
13220 Cannot malloc block list space
13221 %s:  No devices provided for scan.\n
13222 Couldn't open '%s'
13223 fstat of '%s' failed
13224 '%s' is not a block or char special.
13225 Could not seek on device '%s'
13226 Could not read from device '%s'
13227 Device '%s' doesn't have a QFS superblock (SBLK).\n
13228 Device '%s' doesn't have a QFS superblock (MAGIC).\n
13229 host table malloc failed.
13230 DKIOC malloc failed.
13231 Ioctl DKIOCINFO of '%s' failed
13232 VTOC malloc failed.
13233 Could not get VTOC of '%s'
13234 Device '%s' has a QFS superblock.\n
13235 Devname or SBLK malloc failed.
13236 %d QFS devices found.\n
13237 Host table count out of range (%d).\n
13238 Unrecognized device type:  %#x
13239 Usage: %s mount-point inode-number...\n
13240 Inode is free, inode %d.\n
13241 No name returned from id_to_path, inode %d.\n
13242 %s: mount point path name cannot exceed %d characters.\n
13243 %s: Unknown option.\n
13244 You must be root to run %s\n
13245 unable to read inode (via ioctl) %d
13246 %s: Unknown tapealert flag: %s\n
13247 %s: no change, eq %d must be down for TapeAlert setup.\n
13248 %s: no change, eq %d does not support TapeAlert.\n
13249 %s: equipment %d is not tape or media changer device.\n
13250 %s: Invalid sef interval: %s\n
13251 supported
13252 unsupported
13253 on
13254 off
13255 tapealert
13256 not applicable
13257 device
13258 and
13259 current

$ utility/samchaid
$ =================
13260 Usage: %s [-fhR] admid file ...\n
13261 %s: unknown admin set ID: %s\n
13262 %s: Can't get current directory %s\n
13263 %s: Can't chdir back to '%s'\n

$  utility/samfsck and bcheck (13275)
$ ===================================================================
13274 ALERT:  %s: mcf has missing data devices (%d/%d)
13275 ALERT:  %s: mcf has missing metadata devices (%d/%d)
13276 block %s (%lld.%d) is a file system label block
13277 block %s (%lld.%d) is a file system primary super block
13278 block %s (%lld.%d) is a file system alternate super block
13279 block %s (%lld.%d) is a file system allocation map block
13280 block %s (%lld.%d) is a file system block
13281 block %s (%lld.%d) is a free data block
13282 block %s (%lld.%d) is a data block for .inodes containing %d - %d
13283 block %s (%lld.%d) is a data block for inode %d.%d
13284 block %s (%lld.%d) is a data block for directory inode %d.%d
13285 block %s (%lld.%d) is an indirect block for inode %d.%d
13286 block %s (%lld.%d) is an invalid block number
13287 block %s (%lld.%d) is an invalid ordinal
13288 block %s (%lld.%d) usage is unknown
13289 ALERT:  ino %d,\tblock 0x%llx ord %d dt %d mismatch type %d\n
$ 13290-13299 are open to add more messages
13300 eq %d bn=%.8llx allocated in bit maps, but free\n
13301 NOTICE: Orphan ino:        -F will move ino %d to lost+found\n
13302 NOTICE: Cannot process orphans. Please create ./lost+found directory\n
13303 NOTICE: Orphan ino:        ino %d moved to lost+found\n
13304 ** NOT USED **
13305 ** NOT USED **
13306 NOTICE: Orphan processing stopped due to full lost+found. \n\
		Increase lost+found and rerun.\n
13307 NOTICE: directory ino %d,\tstale directory entry %s removed\n
13308 NOTICE: directory ino %d,\t-F will remove stale directory entry %s\n
13309 ALERT:  ino %d,\tdirectory write failed on eq %d\n
13310 NOTICE: Orphan processing stopped due to offline directory. \n\
		Mount filesystem and stage all directories.\n
13311 First pass\n
13312 NOTICE: Reclaimed %lld meta bytes\n
13313 NOTICE: %lld meta bytes can be reclaimed\n
13314 NOTICE: Orphan ino:        ino %d cannot be moved to missing lost+found directory.\n
13315 ALERT:  Allocation block free ord=%d bn=%.8llx. Run samfsck -F to repair\n
13316 ALERT:  Superblock on eq %d at block %d missing magic and/or name\n
13317 ALERT:  Basic checks of .inodes and/or .blocks failed\n\
		File system may be critically damaged.\n
13318 ALERT:  %d large blocks are free (can be allocated), \
but also allocated to inodes.\n
13319 ALERT:  %d small blocks are free (can be allocated), \
but also allocated to inodes.\n
13320 ALERT:  %d meta blocks are free (can be allocated), \
but also allocated to inodes.\n
13321 NOTICE: Orphan ino:        -F will free empty ino %d\n
13322 NOTICE: Orphan ino:        empty ino %d moved to free list\n
13323 ALERT:  ino %d,\tInvalid directory, offset 0x%llx, size=0x%llx\n
13324 ALERT:  ino %d,\tInvalid directory dotdot entry, parent ino=%d\n
13325 Results are NOT accurate -- the filesystem %s is mounted.
13326 Must run samfsck from the metadata server %s
13327 ** NOT USED **
13328 NOTICE: Superblock space counts are incorrect, \t-F will correct.\n
13329 ALERT:  ino %d,\tInvalid directory dot entry, parent ino=%d\n
13330 ** NOT USED **
13331 NOTICE: %d directory entries hashed.\n
13332 NOTICE: %d directory entries missing hash. Both -F and -G needed to correct.\n
13333 NOTICE: Offline directory: ino %d has been removed\n
13334 NOTICE: Offline directory: -F will remove ino %d\n
13335 NOTICE: Offline segment index: ino %d has been removed\n
13336 NOTICE: Offline segment index: -F will remove ino %d\n
13337 NOTICE: Orphan processing stopped due to offline segment index. \n\
		Mount filesystem and stage all segment indices.\n
13338 Cannot malloc inode\n
13339 NOTICE:\t-F will convert WORM inodes to version 2 format\n
13340 NOTICE:\t%s: failed to convert inode %d.%d to version 2 format\n
13341 Third pass\n
13342 ALERT:  ino %d.%d,\tInvalid validation header, offset 0x%llx, size=0x%llx\n
13343 ALERT:  ino %d.%d,\tInvalid segment inode\n
13344 ALERT:  ino %d.%d,\tInvalid segment index\n
13345 NOTICE:  ino %d.%d,\tInvalid segment inode, parent ino: %d.%d\n
13346 NOTICE:  segment ino %d.%d,\tRepaired inode mode from 0x%x to 0x%x\n
13347 NOTICE:  ino %d.%d,\t-F will repair mode from 0x%x to 0x%x\n
13348 NOTICE:  segment ino %d.%d,\tRepaired inode %s from %d to %d\n
13349 NOTICE:  ino %d.%d,\t-F will repair %s from %d to %d\n
13350 NOTICE:  Orphan ino:        -F will free nlink 0 ino %d\n
13351 NOTICE:  Orphan ino:        nlink 0 ino %d moved to free list\n
13352 ALERT:  ino %d.%d,\tInvalid base inode\n
13353 ALERT:  ino %d.%d,\tInvalid inode extension\n
13354 ** NOT USED **
13355 NOTICE:  ino %d.%d,\tDirectory is damaged\n
13356 NOTICE:  ino %d.%d,\tSegment index is damaged\n
13357 NOTICE:  ino %d.%d,\tDirectory size is 0\n
13358 NOTICE:  ino %d.%d,\tSegment index size is 0\n
13359 ALERT:  directory ino %d,\tRepaired zero directory block off=0x%llx\n
13360 ALERT:  directory ino %d,\t-F will repair zero directory block off=0x%llx\n
13361 NOTICE: ino %d.%d,\tRepaired link count from %d to %d\n
13362 NOTICE: ino %d.%d,\t-F will repair link count from %d to %d\n
13363 NOTICE: ino %d.%d,\tRepaired parent id to %d.%d\n
13364 NOTICE: ino %d.%d,\t-F will repair parent id to %d.%d\n
13365 ALERT:  Zero block number in .inodes file at ino %d\n
13366 NOTICE: ino %d.%d,\tRepaired block count from %d to %d\n
13367 NOTICE: ino %d.%d,\t-F will repair block count from %d to %d\n
13368 NOTICE: ino %d.%d,\tReleased excess data block 0x%llx eq %d\n
13369 NOTICE: ino %d.%d,\t-F will release excess data block 0x%llx eq %d\n
13370 NOTICE: ino %d.%d,\tReleased excess indirect block 0x%llx eq %d\n
13371 NOTICE: ino %d.%d,\t-F will release excess indirect block 0x%llx eq %d\n
13372 ALERT:  directory ino %d.%d,\tRepaired invalid directory block off=0x%llx\n
13373 ALERT:  directory ino %d.%d,\t-F will repair invalid directory block off=0x%llx\n
13374 ALERT:  directory ino %d.%d,\tInvalid entry at in=0x%x, off=0x%llx\n
13375 ** NOT USED ** NOTICE: directory ino %d.%d,\tStale/invalid directory entry %s\n
13376 ALERT:  Could not malloc %s quota table\n
13377 ALERT:  quota malloc failed\n
13378 ALERT:  %s quota file has odd size; length %lld should be %lld
13379 ** NOT USED ** NOTICE:  Updating %s quota file offset %lld\n
13380 NOTICE:  %s quota %d online files,  %lld should be %lld
13381 NOTICE:  %s quota %d total files,   %lld should be %lld
13382 NOTICE:  %s quota %d online blocks, %lld should be %lld
13383 NOTICE:  %s quota %d total blocks,  %lld should be %lld
13384 ** NOT USED ** ALERT:  Update of %s quota file ord/block %d/%lld len %d failed\n
13385 ** NOT USED **
13386 ALERT:  ino %d,\tblock 0x%llx exceeds capacity on eq %d\n
13387 ALERT:  ino %d,\tblock 0x%llx in system area on eq %d\n
13388 ALERT:  ino %d,\tError reading indirect block 0x%llx on eq %d\n
13389 ALERT:  ino %d,\tInvalid indirect block 0x%llx eq %d\n
13390 Read failed in .inodes on eq %d at block 0x%llx
13391 Read failed on eq %d at block 0x%llx
13392 ALERT:  ino %d,\tblock 0x%llx ord %d exceeds max ordinal %d\n
13393 ** NOT USED ** ALERT:  ino %d,\tInvalid lost+found directory block 0x%llx eq %d, offset 0x%llx\n
13394 ** NOT USED ** ALERT:  ino %d,\tInvalid lost+found directory entry at block 0x%llx eq %d, offset 0x%llx\n
13395 ALERT:  ino %d.%d,\tsegment index read failed on eq %d at block 0x%llx\n
13396 ALERT:  ino %d.%d,\tsegment index write failed on eq %d at block 0x%llx\n
13397 Write failed in .inodes on eq %d at block 0x%llx
13398 Write failed on eq %d at block 0x%llx
13399 eq %d bn=%.8llx free in bit maps, but allocated\n



$  utility/sammkfs (13400)
$ ===================================================================
13401 Stripe size (%d) and allocation (%d) are not a multiple
13402 failed to allocate %d inodes on eq %d"
13403 Ioctl(DKIOCGGEOM) failed on (%s)
13404 Meta device must be ordinal 0 on filesystem %s.
13405 Striped group eq %d size differs from other members for filesystem %s.
13406 failed to get %d allocation units on eq %d
13407 Cannot getrlimit.
13408 Cannot open more than %d files.
13409 Cannot setrlimit:RLIMIT_NOFILE.
13410 %s (ma) has no meta devices
13411 %s has no meta devices
13412 ** NOT USED **
13413 -%c option only valid on sammkfs command.
13414 -%c option not valid on a standalone (Sun QFS) package.
13415 no options valid on samfsinfo command.
13416 %s needs to be run on an unmounted filesystem.
13417 capacity:      %.16llx          DAU:      %5d\n
13418 space:         %.16llx\n
13419 meta capacity: %.16llx          meta DAU: %5d\n
13420 meta space:    %.16llx\n
13421 filesystem %s is mounted.
13422 device %s is mounted.
13423 device %s does not match raw device %s.
13424 No new devices added to filesystem %s.
13425 ** NOT USED **
13426 Allocation %d must be an 8k multiple.
13427 Building '%s' will destroy the contents of devices:\n
13428 Do you wish to continue? [y/N]
13430 Not building '%s'.  Exiting.\n
13431 Growing '%s' will destroy the contents of devices:\n
13432 Not growing '%s'.  Exiting.\n
13433 Cannot grow Sun QFS filesytem %s without adding metadata partitions.
13434 -l option is only valid with sammkfs -r option.
13435 ** NOT USED **
13436 Creating an old format file system disallows some file system features\n
13437 Please see '%s'\nfor a list of the affected features\n
13438 Creating a new file system prevents use with 4.6 or earlier releases\n
13439 Use the -P option on sammkfs to create a 4.6 compatible file system\n
13440 Too many devices (%d total) added to filesystem %s.
13441 Sun QFS and Sun SAM-QFS Installation and Configuration Guide
13442 Root directory has no archive copy, %s -r fails
13443 Number of volumes mismatched, %s -r fails
13444 Inode version inconsistent with superblock version, %s -r fails
13445 A Sun QFS filesystem with md device(s) is not supported on this superblock version
13446 Cannot change striped group for eq %d
13447 %s: %s is a shared filesystem; use -S option.
13448 %s: %s is not a shared filesystem; -S option specified.
13449 ** NOT USED **
13450 Can't store FS %s hosts file into FS (check hosts file size).
13451 No server declared in shared hosts file
13452 Cannot grow Sun QFS filesytem %s without adding a metadata partition of %d 1k blocks
13453 Cannot run sammkfs from a client
13454 Stripe group elements of g%d not contiguous; reorder mcf file entries
13455 ** NOT USED **
13456 %s: %s: One or more partitions exceeds %d TB in size\n
13457 %s: SAM-QFS allows a maximum of 16 TB per partition\n
13458 %s: file system %s will not mount on 32 bit Solaris and\n
13459 %s: some earlier versions of Solaris\n
13460 %s (md) allocation = %d, must be 16, 32, or 64
13461 %s (ma) allocation = %d, must be >= 8 and <= 65528
13462 %s: Unrecognized argument.
13463 %s: No family set provided.
13464 The -%c argument requires a positive integer to follow
13465 Write Once Read Many partitions detected <%d of %d>
13466 name:     %s       version:     %s%10s
13467     shared
13468     opt mask 0x%x
13469 mm_ord device not found for index %d/ord %d
13470 Omitting the -V option will create filesystem '%s'.\n
13471 Omitting the -V option will extend filesystem '%s'.\n
13472 FS %s built with non-native byte order\n
13473 FS %s: not a recognized QFS file system type\n
13474 WORM files present\n
13475 WORM (strict) files present\n
13476 WORM (lite) files present\n
13477 WORM (Emulation mode) files present\n
13478 WORM (Emulation lite mode) files present\n
13479 Warning: Host count (%d) > SAM_MAX_SHARED_HOSTS (%d)
13480 Multiple servers declared in shared hosts file
13481 FS %s: Filesystem has no object data partition
13482 FS %s: Cannot create object inode %d
13483 WORM V2 files present\n
13484 FEATURE: Aligned Maps\n

$  utility/csd	(13500)
$ ===================================================================
13500 Cannot write to samfsdump file
13501 Could only write %d bytes to samfsdump file, tried %d, %s
13502 Corrupt samfsdump file.  read(%d) returned %d bytes, %s
13503 System error return from read of samfsdump file, %s
13504 Error reading from file %s
13505 Error writing to data file %s
13506 Unable to sam_stat() file %s
13507 XX-UNUSED-XX Unable to open file <%s> to dump data
13508 Improper invocation of SAM-FS csd utility %s
13509 Invalid buffer size, must be >= %d and <= %d, specified in 512b units
13510 Invalid blocking factor, must be >= 0 and <= %d, specified in 512b units
13511 XX-UNUSED-XX Cannot malloc buffer space %d bytes
13512 %s: File data cannot be recovered (file is marked damaged).
13513 %s: File data was not recoverable prior to dump (file is marked damaged).
13514 %s: File data was previously not recoverable (file is marked damaged).
13515 %s: File data will not be recoverable (file will be marked damaged).
13516 Corrupt samfsdump file. File header error <0x%x/%d>
13517 Corrupt samfsdump file. File tar header error.
13518 %s: Cannot malloc space for directory/segment index stack.
13519 Not all directory or segment index times will be set correctly.
13520 Maximum -X (excluded directory) entries exceeded, max. is %d.
13521 -X (excluded directory) entry <%s> invalid
13522 Segmented file has %d of %d stale archive copies.
13523 Segmented file has %d of %d without archive copies.
13524 Cannot chmod directory %s
13525 putting segment inode
13526 %s: Cannot malloc space for directory permission stack.
13527 %s: Cannot malloc space for directory permission path.
13528 Not all directory permissions will be set correctly.
13529 %s: Volume is not in dump format.
13530 Unable to open file <%s> to dump data, skipping.
13531 %s: segment inode byte swap error - skipping.
13532 %s: inode byte swap error - skipping.
13533 Resource file v%d byte swap error.
13534 ACL byte swap error.
13535 VSN byte swap error.
13536 file header byte swap error.
13537 Maximum -I (include file) entries exceeded, max. is %d.
13538 -I (include file) entry <%s> invalid
13539 %s: Cannot malloc space for hard link table.

$  utility/robot and catalog commands
$ ===================================================================

13600 Volume specification error %s
$  chmed
13610 Usage: %s arguments.   Use '%s -flags' for details
13611 Invalid option %s
13612 Invalid capacity %s
13613 Invalid space %s
13614 Invalid count %s
13615 Invalid flag in %s
13616 Catalog change error %s
$ label
13620 Cannot label volume in historian.
13621 VSN not blank for new label.
$ move
13630 Invalid destination slot
13631 Destination slot %d is in use
13632 Failed to get information for the device with eq ord %u.
13633 once
13634 on required before interval

$  utility/samsharefs cmd
$ ===================================================================
13700 Usage:\n\t%s [-hqRu] [-s host] fs_name
13701 Incorrect usage.
13702 -d/-u options incompatible:  Use -U <hostsfile>.
13703 XX-UNUSED-XX
13704 XX-UNUSED-XX %s:  -s and -A options are incompatible.
13705 XX-UNUSED-XX %s:  -s and -C options are incompatible.
13706 Unknown extra arguments.
13707 Host %s cannot be a metadata server for FS %s.
13708 Host failover already pending on FS %s.

13710 Usage:\n\t%s -R[hqu] [-s host] fs_name\n\t%s [-Rhq] fs_name
13711 Only metadata host can change metadata server
13712 Host file changes allowed only on unmounted FSes

13720 Cannot find FS %s
13721 Input file processing failed.
13722 Host '%s' not found in host table.
13723 XX-UNUSED-XX %s:  Unknown host table device.
13724 XX-UNUSED-XX %s:  Device %s isn't FS root partition.
13725 XX-UNUSED-XX %s:  File system on %s is not shared.
13726 XX-UNUSED-XX %s:  Cannot allocate memory for hosts file.
13727 XX-UNUSED-XX %s:  No SAM filesystem on %s.
13728 Unrecognized version in host file (%d).
13729 XX-UNUSED-XX %s:  Host table too large (%d/%d bytes).
13730 XX-UNUSED-XX %s:  Host table count out of range (%d).
13731 Client cannot access FS %s host information.
13732 Host table length out of range (%d).
13733 Cannot convert hosts file (%s/%d).
13734 Host table too large.
13735 Cannot get FS %s information.
13736 -R option is not valid while FS %s is mounted.

13740 XX-UNUSED-XX %s:  Cannot open device '%s'
13741 XX-UNUSED-XX %s:  Cannot llseek to superblock on '%s' (%d)
13742 XX-UNUSED-XX %s:  superblock read failed on '%s'
13743 XX-UNUSED-XX %s:  sam_sethost call failed on FS %s
13744 XX-UNUSED-XX %s:  Cannot llseek to hosts file on '%s'
13745 XX-UNUSED-XX %s:  Cannot read host file on '%s'
13746 XX-UNUSED-XX %s:  Cannot llseek to hosts file on '%s' (%d)
13747 XX-UNUSED-XX %s:  Cannot write host file on '%s'
13748 XX-UNUSED-XX %s:  Host file close on '%s' failed (%d)
13749 sam_shareops(%s, SHARE_OP_WAKE_SHAREDAEMON, 0) failed
13750 Cannot read hosts file -- %s
13751 Cannot write hosts file -- %s
13752 sam_gethost call failed on FS %s
13753 sam_sethost call failed on FS %s
13754 Filesystem %s not mounted
13755 Cannot get local host name
13756 Cannot get shared server name
13757 FS %s: Host %s already server
13758 sam_shareops(%s, SHARE_OP_INVOL_FAILOVER, %d) failed
13759 File system %s is not mounted on new server

13760 %s:  Retrying %s operation
13761 Host '%s' ordinal (%d) reassigned to '%s'
13762 Server '%s' reassigned in new hosts file
13763 Host ordinals may not be reassigned on active FS
13764 Cannot write hosts file -- foreign byte order.
13765 Warning: Host count (%d) > SAM_MAX_SHARED_HOSTS (%d)
13766 Host '%s' ordinal (%d) removed or reassigned.


$  utility/samfsconfig (continued) (13800)
$ ===================================================================
13800 Invalid shared hosts file on device '%s'.
13801 Device '%s' has a (byte-reversed) QFS superblock.\n
13802 Host file information for family set '%s'\n
13803 Version: %d    Generation: %d    Count: %d\n
13804 Length: %d    Server: %d\n
13805 Family Set '%s' Created %s
13806 Foreign byte order (super-blocks byte-reversed).\n
13807 zoned-off or missing metadata device\n
13808 Missing slices\n
13809 Ordinal %d\n


$  utility/samfsck and bcheck (continued) (13900)
$ ===================================================================
13900 NOTICE: ino %d.%d,\tRepaired inode version from %d to %d\n
13901 NOTICE: ino %d.%d,\t-F will repair inode version from %d to %d\n
13902 Read of shared fs hosts data failed
13903 Bad hosts data -- see samsharefs(1M) to fix
13904 getHostName() failed -- can't get local hostname
13905 NOTICE: Offline directory: ino %d.%d detected\n
13906 NOTICE: Offline segment index: ino %d.%d detected\n
13907 NOTICE: ino %d.%d,\tRepaired file size from %lld to %lld\n
13908 NOTICE: ino %d.%d,\t-F will repair file size from %lld to %lld\n
13909 Filesystem %s slice %d/eq %d:  fsck bits = %s
13910 Filesystem %s superblock:  fsck bits = %s
13911 NOTICE: Filesystem %s requires fsck
13912 NOTICE: ino %d.%d,\tflags repaired\n
13913 NOTICE: ino %d.%d,\tflags incorrect, -F will repair\n
13914 ALERT:  ino %d.%d,\tInvalid lost+found directory validation header at block 0x%llx, eq %d, offset 0x%llx\n
13915 ALERT:  ino %d.%d,\tInvalid lost+found directory entry at block 0x%llx, eq %d, offset 0x%llx\n
13916 Unrecognized option flag '%c'.
13917 ALERT:  Invalid system inode %d with size %lld\n
13918 ALERT:  ino %d.%d,\tFreed inode with size %lld\n
13919 ALERT:  ino %d.%d,\t-F will free inode with size %lld\n
13920 You must be root to run %s\n
13921 Usage: %s fs-name block-number...\n
13922 %s: Configuration error.\n
13925 -S/-U options not supported for Version 1 FS
13926 -S/-U options require -F option
13927 shared_fs_convert: get_inode(HOST_INO) error
13928 shared_fs_convert: hosts file inode problem
13929 shared_fs_convert: hosts file ordinal extent err (%d != 0)
13930 shared_fs_convert: hosts file extent err (%#x)
13931 shared_fs_convert: FS already shared
13932 shared_fs_convert: FS already non-shared
13933 %s quota file is not a plain file
13934 %s quota file is segmented
13935 %s quota file is offline
13936 Cannot malloc quota block buffer
13937 Quota file %s read failed on eq %d, ord/block = %d/%lld len %d
13938 Updating quota file %s, indices %ld - %ld
13939 Quota file %s write failed on eq %d, ord/block %d/%lld len %d
13940 Quota file %s DAU beyond EOF (offset=%lld, EOF=%lld, bn=%lld)
13941 %s:  no quota record for index %d
13942 INFO:  FS %s last repaired: %s
13943 INFO:  FS %s repaired:\n\tstart:  %s\tfinish: %s
13944 NOTICE:  Updating inode %d.%d ext attr flags (%#x->%#x)\n
13945 NOTICE:  Would update inode %d.%d ext attr flags (%#x->%#x)\n
$ 13946 ** NOT USED **
13947 NOTICE:  inode %d.%d HLP extension inode %d.%d n_ids (%d) out-of-range [0..%d]\n
13948 NOTICE:  inode %d.%d HLP extension inode %d.%d slot %d: bad entry (%d.%d)\n
13949 ALERT:  Directory inode %d.%d has HLP extension inode attached (%d.%d)\n
13950 NOTICE:  inode %d.%d HLP extension inode %d.%d slot %d points to non-directory inode %d.%d\n
13951 NOTICE:  inode %d.%d HLP extension inode %d.%d has entry %x.%x in slot >= n_ids (%d/%d)\n
13952 ALERT:  inode %d.%d: extension inode (%d.%d) has unknown type: %x\n
13953 NOTICE:  inode %d.%d shows %d links, %d parents\n
13954 NOTICE:  inode %d.%d, setting parent to ino %d.%d\n
13955 NOTICE:  updating HLP extension inode %d.%d; copying %d parent IDs\n
13956 NOTICE:  clearing HLP extension inode %d.%d.\n
13957 ALERT:  inode %d.%d: %d parents unclaimed\n
13960 ** NOT USED ** -S/-U options require QFS or SAM-QFS (ma) file system
13961 Missing mount point or family set name argument.
13962 illegal -o option flag -- %c
13963 -o option flags must be separated by a comma.
13964 Too many arguments.
13965 The -n/-N and -y/-Y options are mutually exclusive.
13966 The -n/-N and -F options are mutually exclusive.
13967 The -S/-U options require the -F option.
13968 Stopped.
13969 \"%s\" is not an absolute path name.
13970 Can't find filesystem name for mount point \"%s\".
13971 %s:  %sino %d marked %s\n
13972 %s:  %s-F will mark ino %d %s\n
13973 %s: gethostname failed
13974 %s: gethostname(): name too short
13975 %s: gethostname(): name too long
13976 %s: Read of %s failed
13977 %s: Bad hostname in %s: name too short
13978 %s: Bad hostname in %s: name too long
13979 %s: Hostname '%s': Bad character ('%c')
13980 %s: File %s, hostname '%s': Bad character ('%c')
13981 NOTICE:  ino %d.%d,\tFile size %lld exceeds maximum allowed for configured DAU size.\n
13982 No log present\n
13983 Empty log found\n
13984 ALERT: Log contains transactions; mount filesystem writeable to replay them\n
13985 NOTICE: Superblock points to a bad log; freeing blocks\n
13986 NOTICE: Superblock points to a bad log, \t-F will correct\n
13987 ALERT: Cannot malloc log buffer\n
13988 ALERT: Log extent read failed on eq %d\n
13989 INFO: Log extent # %d - startbn %d %d 1K blocks\n
13990 ALERT: Log head and tail read failed on eq %d\n
13991 ALERT: error %d counting block %d ord %d\n
13992 NOTICE: logbno in superblock out of range %d %d\n
13993 NOTICE: bad cksum for log extent_block_t block %d %d\n
13994 NOTICE: First block out of range in log extent # %d - startbn %d %d 1K blocks\n
13995 NOTICE: Last block out of range in log extent # %d - lastbn %d \n
13996 INFO: Log header bad bn %d od_chksum %d od_head_ident %d od_tail_ident %d\n
13997 ALERT: Unexpected state; valid log blocks but qfs_rolled invalid %d\n
13998 %s: gethostname(): bad character
13999 %s: Hostname has bad character in %s 

$  lib-sam      (14000s)
$ ===================================================================
$ readcfg.c
14000 %d errors in '%s'
14001 1 error in '%s'
14002 Unterminated string
14003 String is too long
14004 Error in line %d
14005 '%s' is not a recognized directive name
14006 No value allowed for '%s'
14007 '=' missing
14008 Missing '%s' value
$ 14009 ** NOT USED ** Missing '%s' value (NULL pointer)
14010 Cannot open configuration file '%s': %s
14011 Configuration file '%s' is empty.
14012 Reading '%s'.
14013 Error reading master configuration file '%s'.

$ custmsg.c
14080 -e Fatal OS call error
14081 -c Out of memory
14082 OS call error
14083 Error logging system event: %s
14084 nvlist_alloc failed: %s

$ diskvols.c
14090 -Sw Sam rft connection to '%s' failed.  Check if the SAM-RFT server process is running on the remote host.

$ setfield.c
14100 Invalid value name ('%s')
14101 Invalid '%s' value ('%s')
14102 '%s' value is out of range %lld to %lld
14103 '%s' value must be greater than %lld
14104 '%s' value must be less than %lld
14105 '%s' value is out of range %g to %g
14106 '%s' value must be greater than %g
14107 '%s' value must be less than %g
14108 Invalid '%s' media ('%s')
14109 '%s' value is too long (maximum %d)
14110 '%s' value must be a power of 2
14111 '%s' value must be a multiple of 8
14112 Duplicate '%s' definition
14113 Missing '%s' value (NULL pointer)
14114 Option '%s' will no longer be supported; converting it to '%s'

$ trace.c
14200 Trace change not allowed
14201 Invalid daemon name '%s'
14202 Must have 'on' or 'off'
14203 Default trace path must be absolute
14204 Invalid age
14205 Unknown trace option: %s
14206 Invalid size

$ signals.c
14250 -Si %s shutdown by signal %s

$  lib-rm-media (15000s)
$ ===================================================================
15000 Media Error : Unable to create EOF (%d:%s) %#x.
15001 Media Error : Unable to position to EOD (%d:%s).
15002 Media Error : No space left on media (%d:%s).
15003 Media Error : Failure creating tape mark at EOD(%d:%s).
15004 Media Error : Failure finding EOF (%d:%s) at %#x.
15005 Media Error : Failure finding EOD (%d:%s) at %#x.
15006 Media Error : Cannot read label for eq:slot:partition(%d:%d:%d)



$  lib-misc     (16000s)
$ ===================================================================

$  This group must match up exactly with the SAM error numbers (SAM_errno)
$  that are listed in /opt/SUNWsamfs/include/sam_errno.h.

16000 Not a removable media file
16001 File already archived
16002 File modified
16003 Data verification failed
16004 No master shared memory found
16005 No master shared memory found to attach
16006 No matching equipment ordinal
16007 No create response FIFO created
16008 FIFO path length too long
16009 FIFO command response received
16010 Cannot audit device specified
16011 Invalid slot number
16012 Slot is not occupied by media
16013 Operator is not privileged
16014 Invalid flag bit set in parameter
16015 Bad value for on_off parameter
16016 Slot or VSN required as parameter
16017 VSN specified was not found in robot
16018 Invalid characters in specified VSN
16019 Invalid length for specified VSN
16020 Robotic device required to be specified
16021 Device found is not correct type
16022 No device found
16023 Device is not ready
16024 Device is not type specified
16025 Device is not manually loaded
16026 Device is off or down
16027 Device is not in unavailable state
16028 Device is being used by another process
16029 VSN or barcode required to be specified
16030 Robot catalog is missing, audit needed
16031 Invalid block size specified
16032 Block size specified is too large
16033 Invalid media type specified
16034 Invalid user information length
16035 Device cannot be labeled
16036 Volume VSN not same as old VSN
16037 Old VSN not matching unknown
16038 Not a removable media device
16039 Media type is for Historian
16040 Historian requires media type
16041 Historian does not allow audit_eod
16042 Unable to status robot catalog
16043 Unable to map robot catalog
16044 Slot contains a cleaning cartridge
16045 Robot does not support moves
16046 ** NOT USED ** Not valid destination slot
16047 ** NOT USED ** Source slot not available for move
16048 ** NOT USED ** Destination slot not available for move
16049 Destination slot is occupied
16050 Invalid new state specified
16051 Down device can only be changed to off
16052 Passed structure too small for data
16053 No live device segment handles available
16054 No live catalog segment handles available
16055 No live mount request segment handles available
16056 No live device segment handle found
16057 No live catalog segment handle found
16058 No live mount request segment handle found
16059 No preview shared memory found
16060 No preview shared memory found to attach
16061 File is offline
16062 Duplicate VSN
16063 Invalid partition
16064 Unable to initialize catalog
16065 Slot required
16066 Partition required
16067 Volume not found in catalog
16068 Volume already reserved
16069 Volume not reserved
16070 File damaged
16071 File busy
16072 File locked
16073 This archive copy nonexistent
16074 File offline - would unarchive last good archive copy
16075 Requested device operation failed
16076 Robot does not support
16077 CAPID not defined. export -f fails.

16998 Unknown error number
16999 Unknown error number %d

$  samlogd       (17000s)
$ ===================================================================
17000 %s should only be started by sam-amld.
17001 %s shutdown by signal %d
17002 Fatal error(%d): %s(%s) called from: %s:%d
17003 Unknown token in %s: %s.  Ignored.
17004 Multiple %s lines in %s.  Using initial entry.
17005 Unexpected token: %s in %s.
17006 Cannot attach master shm
17007 Cannot attach shm
17008 Wrong segment name in shm
17009 Cannot open FIFO %s
17010 Rereading configuration.
17011 Logging: %s.
17012 No logging will occur.
17013 Cannot open %s.  No logging will occur.
17014 Cannot determine mount point for %d

$  sam-fsd       (17200s)
$ ===================================================================

17200 modload /kernel/fs/samfs failed
$ 17201 ** NOT USED ** Error in /etc/name_to_sysnum for samsys line %d
17202 System call change failed in %s
17203 License has expired or is corrupted; initialization failed.
17204 License initialization failed.
$ 17205 XX-UNUSED-XX No SAM-FS devices in mcf file
17206 Set mount values failed for %s
17207 File system samfs is not loaded.
17208 sysfs(samfs) failed.
17209 Read mcf %s failed.
17210 Problem in mcf file %s for filesystem %s
17211 Family set %s is duplicated in mcf %s
17212 Family set %s is missing from mcf %s
17213 File system %s has no devices.
17214 %s must be block special file.
17215 File system %s has invalid devices.
17216 Get mount values failed for %s
17217 Problem with file system devices.
17218 Problem with file system command file.
$ 17219 ** NOT USED ** No samsys system call found in \"/etc/name_to_sysnum\".
$ 17220 ** NOT USED ** Bad samsys system call %d - YOU MUST REBOOT or fix \"/etc/name_to_sysnum\"
17221 -wS Cannot reconfigure mounted filesystem %s
17222 Set mount values failed for %s
17223 -wS Cannot remove mounted filesystem %s
17224 Read defaults %s failed
17225 Cannot set group ID %d
17226 Cannot change log facility while running
17227 -wS Cannot reconfigure busy filesystem %s
17228 -wS Cannot remove mounted filesystem %s
17229 -wS Cannot remove busy filesystem %s
17230 Read diskvols %s failed
17231 Filesystem %s unchanged
17232 Would configure %s
17233 Would reconfigure %s
17234 Would remove %s
17235 %s is different for %s
17236 Solaris release mismatch - is %s, should be %s
17237 File system %s has no data devices.
$ 17238 ** NOT USED ** Family set %s is shared (unsupported for this Solaris release)
17239 File system %s has too many (%d) devices (limit = %d).

$ From fsd/readmcf.c
17240 No family set device for eq %d, set '%s'
17241 Device path name '%s' exceeds %d characters
17242 Equipment name '%s' already in use by eq %d
17243 Equipment ordinal missing
17244 Invalid equipment ordinal '%s'
17245 Equipment ordinal %d must be in the range 1 <= eq_ord <= %d
17246 Equipment ordinal %d already in use
17247 Device type missing
17248 Invalid device type '%s'
17249 Only one historian allowed '%s'
17250 Family set name '%s' exceeds %d characters
17251 Invalid device state '%s'
17252 Path name '%s' exceeds %d characters
17253 Extra fields on line
17254 Family set name missing
17255 Family set name '%s' in use by eq %d
17256 Device name '%s' must match family set name '%s'
17257 Invalid disk parameters '%s'
17258 Raw device name '%s' doesn't match device '%s'
17259 %s must be char special file.

$ From fsd/fsconfig.c
17260 GetFsMountDefs(%s) failed
17261 %s: 'moa' filesystem cannot be shared
17262 -wS Reconfigure mounted filesystem %s
17263 Cannot open object device %s

$ From fsd/defaults.c
17280 Invalid '%s' value '%s'
17281 Operator group %s not found.
17282 No preceding '%s' statement
17283 Trace file controls:

17290 SPM port registrar initialization failed (%d, '%s').
17291 Lock out of possible competing sam-fsd daemons failed (%s)
17292 Would start %s(%s)\n
17293 Would stop %s(%s)\n
17294 File system already configured.\n

$  mount       (17300s)
$ ===================================================================
17301 special_file name cannot exceed %d characters.
17302 syscall[SC_getmount] failed
17303 must specify family_set for block_device
17304 Client mount %s timed out
17305 must specify "shared" in mount options
17306 backgrounding:
17307 retrying:
17308 %s: mount point path name cannot exceed %d characters.\n
17309 Usage: %s [-F samfs] [generic options] [-o samfs_options] special_file mount_point.\n
17310 File System '%s' has non-native byte order.
17311 Package protocol version mismatch
17312 %s: bad mount option value for def_retention.\n

$  umount      (17400s)
$ ===================================================================
17400 Usage: umount [-f] [-o await_clients=#] fs\n
17401 Insufficient privilege\n
17402 %s: umount failed: %s
17403 %s: is busy\n
17404 %s: not mounted\n
17405 %s: shared file system has client(s) mounted\n
17420 Out of memory\n

$  catalog      (18000s)
$ ===================================================================
$ The following comments are also at the top of this file.
$ The messages in the range 18000 - 18020 are sent via SendCustMsg()
$ They may be preceded by an optional delivery code string that begins with
$ '-'.  The letters may be in any order and may be one of the following:
$     N - Send message to notify executable file.
$     S - Send message to syslog
$ The first letter of the syslog priority codes -
$     Emerg, alert, crit, err, warning, notice, info, debug
$ The default action is: -Si
$ Example: -SNw  sends a "warning" to both syslog and the notify executable.

18000 -nS Catalog file %s version %d renamed to %s
18001 -wS Failed to access catalog %s - %s
18002 -wS No catalog files available.
18003 -wS Created empty catalog %s
18004 -wS Failed to create empty catalog %s
18005 Catalog manager shutdown by signal %d
18006 -nS Bad catalog file %s renamed to %s

18020 -nS Volume %s exported to historian.
18021 -nS Historian volume %s moved to catalog %d.
18022 -nS Volume %s exported from all catalogs.
18023 -nS Volume %s exists in %d
18024 -wS Catalog entry %s VSN mismatch with %s from loaded media.
18025 -wS Catalog entry %s barcode mismatch with %s from loaded media.
18026 -wS Catalog entry %s capacity being reset to drive reported capacity %llu.
18027 -wS Catalog entry %s blocksize mismatch with %d from loaded media.
18028 -wS Catalog entry %s label time mismatch with %x from loaded media.
18029 -wS Catalog entry %s media type mismatch with %s from loaded media.
18030 -wS Catalog entry %s write_protect mismatch with loaded media %c vs %c.
18031 -iS Volume %s relabeled.
18032 -iS Volume %s relabeled with %s.
18033 -iS Volume %s %s.%s labeled.
18034 -iS Volume %s reserved to %s/%s/%s
18035 -iS Volume %s unreserved from %s/%s/%s

$ library
18200 Catalog file must be a regular file or symbolic link.
18201 Bad catalog file, larger than 1 gigabyte.
18202 Catalog version is %d and should be %d.
18203 Catalog audit time value is negative or exceeds current time.
18204 Catalog media type field is an invalid type of %x.
18205 Catalog entry count is negative.
18206 Catalog entry count of %d is inconsistent with catalog size.\n\tThis \
size of file should contain %d catalog entries.
18207 Volume specification error %s
18208 Volume specification error %s: %s
18209 Empty catalog file.
18210 Catalog file too small.
18211 Catalog initialization failed
18212 Volume specification missing.
18213 Cannot export volume
18214 Volume reserve specification error %s
18215 Reserve volume failed
18216 Unreserve volume failed

$ build_cat
18400 Line %d:
18401 Entry ignored.
18402 %d errors in input file, catalog file not written.
18403 %d bytes written to catalog file %s .
18404 Write to catalog file %s failed.
18405 too many arguments
18410 entry index is negative or greater than %d.
18411 attempting to overwrite previously entered line %d.
18412 %s missing.
18413 VSN too big.
18414 media type of catalog must be specified when z type media used.
18415 invalid media %s.
18416 media does not match default.
18417 capacity or space invalid.

$ dump_cat
18450 Incorrect catalog file version number %d, expected %d.
18451 # Index  VSN     Barcode  Type   PTOC Access Capacity  Space Status    Sector Label time  Slot Part Modification time Mount time Reserved Time Archive-Set/Owner/File System Volume Information
18452 #      ---status---  ---label time----  --last mod time--  ----mount time---



$  stager       (19000s)
$ ===================================================================
$ These message are sent via SendCustMsg function in samut library.
$ They may be preceded by an optional delivery code string that begins with
$ '-'.  The letters may be in any order and may be one of the following:
$     N - Send message to notify executable file.
$     S - Send message to syslog
$ The first letter of the syslog priority codes -
$     Emerg, alert, crit, err, warning, notice, info, debug
$ The default action is: -Si
$ Example: -SNw  sends a "warning" to both syslog and the notify executable.

19000
19001
19002 -Sw Cannot open %s.  No logging will occur.
19003 -Si Stager shutdown by signal %d.
19004 -Sw No configured file systems found.  No staging will be performed.
19005 -SeV[Stager:CannotReadDev] Could not read SAM-FS devices.
19006 -Sw %s(%s) system call failed, errno %d.
19007 -SwV[Stager:NoRemMedia] No removable media devices or disk archive volumes found.  No staging will be performed.
19008 -Sw Catalog initialization failed.
19009 -Se Unable to create new thread.
19010 -Si Maximum number of active stages was exceeded.
19011 -SwV[Stager:CannotAttachshm] Cannot attach SAM-FS shared memory segment.
19012 -Si %d: %s
19013 -Sw Error in stager command %s.
19014 -SwV[Stager:CmdErr] Stager command error: %d: %s - %s.
19015 ** NOT USED ** -Sw Unknown trace option %s.
19016 -Si Cannot find pathname.
19017 -Sw Cannot stage from volume %s.
19018 Cannot access %s file %s.
19019 Logfile name longer than %d characters.
19020 -aV[Stager:NoVolFound] Volume '%s.%s' not found.
19021 ** NOT USED ** -aV[Stager:MediaErr] Media error on volume '%s.%s'.
19022 -aV[Stager:NoLibAvail] Library containing volume '%s.%s' is not available.
19023 -SwV[Stager:CantCreatRMfile] Unable to create removable media file %s.\n\
		File system may be out of inodes.\n
19024 -Si Migration toolkit '%s' loaded.
19025 -Sw Migration toolkit api '%s' not found.
19026 -Sw Migration toolkit '%s' not loaded.
19027 -Sw Migration toolkit '%s' initialize function failed.
19028 Invalid logfile event '%s'.
19029 -Sw File '%s' (inode %d.%d), archive copy %d, is marked damaged.
19030 -Sw Positioning to %d failed on eq %d, errno %d.
19031 -Sw Read failed on eq %d, errno %d.
19032 -Sw Invalid tar header, file '%s' (inode %d.%d), archive copy %d.
19033 -Sw Cannot stage file: all copies are damaged. '%s'
19034 bufsize option must be 'lock'
$ File System Manager uses 19035 instead of 19023, custmsg has code to
$ prevent flooding
19035 Unable to create one or more removable media files. File system may be out of inodes. Please see the log files for additional information
19036 -Sw Unexpected STK 5800 metadata query result, more than 1 OID (%d) found

$ Operator messages
19200 Loading VSN %s
19201 Unloading VSN %s
19202 ** NOT USED ** Copying file %s
19203 Positioning for file %s
19204 Waiting for VSN %s to be loaded or imported
19205 Copied %s of %s for file %s
19206 Connection to host failed %s
19207 Disk archive file open failed %s
19208 Resources not available
19209 File system not mounted
19210 Resources busy
19211 Staging files
19212 Waiting for resources
19213 Resources not available VSN %s

$  misc         (20000s)
$ ===================================================================
20001 %s: No such device (%d).
20002 Invalid state change(%d).
20003 Unable to open device(%d):%s.
20004 Unable to stat drive(%d):%s.
20005 You must be root to run %s\n
20006 %s is already running.\n
20007 ** NOT USED **  %s: SAM-FS group name not found.\n
20008 Slot label only valid on active robots (eq %d)
20009 Device not ready for labeling (eq %d)
20010 unable to open %s
20011 Media type of catalog must be specified when z type media used, line %d.\n
20012 Input line %d attempting to overwrite previously entered slot %d. Entry ignored.\n
20013 shmget: Invalid shared memory segment size %d.\n

$  recycler      (20200-20399)
$ ===================================================================
$  Recycler also uses message numbers:
$     498 594 1063 1180 1421 1460 1616 1785
$     2053 2054 2055 2312 2523 2524 2593 2738 3066
$     3067 3068 4441 4499 4501 4498 4515 4800 4804

20200 Unable to attach master shm segment: %s
20201 Cannot malloc %d bytes for FIFO pathname: %s
20202 Cannot open(%s): %s
20203 Cannot fstat(%s): %s
20204 Cannot mmap(%s): %s
20205 \nRemovable Media Libraries: %d entries, Archive Sets: %d entries
20206 \nDisk Archive Sets: %d entries
20207 \n%-2d Family: %-20s Path: %s\n   Vendor: %-20s Product: %-20s
20208    SLOT                  ty    capacity         space vsn
20209     %3d   %s %s %s%13s%13s %-20s
20210       (no VSNs in this library)
20211    Total Capacity:  %s bytes, Total Space Available: %s bytes
20212    Volume utilization %lld%%, hwm %d%% mingain %d%%
20213    Volume utilization %lld%%, hwm %d%% mingain %d%%: *** Needs recycling ***
20214    Send mail to %s when this library needs recycling.
20215    Recycling is ignored on this library.
20216 VSN %s exists in multiple catalogs or is duplicated in the same catalog
20217 %s, line %d: %s
20218 %s: expired archive image
20219    %s: Archive copy %d made
20220    %s: vsn %s %s labeled
20221    Disk archive file '%s' for copy %d does not exist.
20222    VSN %s is not marked \"no_recycle\".  It will be a candidate again.
20223 \nNeed to select candidate for %s %s to free up %s bytes.\nQuantity of data to move limited to %s bytes and %s VSNs.
20224 Checking %s.  Need to free %s, quantity limit: %s, VSN count: %s.
20225    VSN has request files... skipping.
20226    VSN has no request files... good.
20227    VSN is in %s '%s'... good.
20228    VSN meets minimum gain requirement (%d%% > %d%%)... good.
20229    Library is accepting recycling - VSN marked for recycling.
20230    VSN exceeds %s limit - skipping.
20231    VSN doesn't meet minimum-gain requirement (%d%% <= %d%%)... skipping.
20232    VSN not in %s '%s'.
20233    Recycling is ignored on this library - VSN not marked for recycling.
20234 No candidate was found in this library.
20235    Archive copy %d of %s resides on VSN %s
20236    Archive set is accepting recycling - VSN marked for recycling.
20237 \nRemovable Media VSNs: %d entries
20238                       ---Archives---   -----Percent-----   %s
20239  ----Status-----    Count    Bytes   Use Obsolete Free   Library:Type:VSN
20240 %16s %10s%7s%6d%6d%7d%8s:%s:%s
20241                  Alpha: %f
20242 %16s %10s%7s                     <none>:%s:%s
20243 Failed to open %s: %s
20244 Failed to write %s: %s
20245 Cannot open %s for mailing to %s: %s
20246 Shell command %s failed: %s
20247 Unknown command %d.
20248 Free space on the %s %s '%s' is getting low.\n\nIt currently has a capacity of %s bytes, of which\n%s are free, for a utilization of %lld%%.\nThe configured high-water mark is %d%%.\n\n
20249 Suggestion: Recycle VSN %s.\n
20250 Previously selected VSN %s is being processed.\n
20252 I will recycle VSN %s.\n
20253 I suggest you recycle VSN %s.\n
20254 Cannot find any candidate VSN in this %s.\n
20255 %s %s recycle.
20256 Post-recycled VSN %s (Slot %s in %s.)
20257 VSN post-recycle
20259 \n
20260 Unable to find master shared memory segment: %s
20261 Master shared memory name incorrect.
20262 Cannot open .inodes for %s: %s
20263 No need to select additional candidates.  After accounting for already-recycling\nVSNs, recycler needs to free %s, quantity %s, count %s.
20264 %s already recycling - no need to select another candidate.
20265 Recycler exiting - another instance of a running recycler was detected.
20266    VSN was specified as \"no_recycle\" in recycler.cmd file... skipping.
20267    VSN was not specified as \"no_recycle\" in recycler.cmd file... good.
20268 VSN regular expression '%s' is malformed in recycler.cmd file.
20269 Cannot malloc %d bytes for no_recycle data: %s.
20270 VSN %s in this library is already marked \"recycling.\"
20271  VSN %s has request or noarchive files or is marked \"no_recycle\"\n    in recycler.cmd.  Need to select a new candidate.
20272 Cannot malloc %d bytes for regular expression: %s.
20273 Cannot open \"archset\" file %s: %s
20274 Cannot read header from archset file %s: %s
20275 Cannot read body of archset file %s: %s
20276 Cannot read VSN regular expression information from archset file %s: %s
20277    Recycling is ignored on this archive set.
20278    Send mail to %s when this archive set needs recycling.
20279 Warning: VSN %s of type %s is in multiple archive sets.
20280 Waiting for VSN %s:%s to drain, it still has %d active archive copies.
20282 Cannot malloc %d bytes for catalog for %s: %s.
20283 Cannot realloc %d bytes for catalog for %s: %s.
20284 Archive set name "%s", mentioned in %s, is not a currently defined archive set name - ignored.
20285    VSN does not exceed %s limit... good.
20286    VSN is already recycling... skipping.
20287    VSN is not already recycling... good.
20288    VSN has files marked 'archive -n'... skipping.
20289    VSN has no 'archive -n' files... good.
20290 This message was generated by the SAM-FS recycler at %s.\n\n
20291    VSN has 'archive -n' files - cannot recycle
20292    VSN has no 'archive -n' files...good.
20293   Empty VSNs suppressed.
20294 Cannot create post-mortem file %s: %s
20295 Cannot write post-mortem file %s: %s
20296 Cannot find any filesystems.
20297 %s: Not a valid SAM-FS filesystem.
20298 The command:\n\n %s\n\nfailed.
20299 unlimited
20300 <none>
20301 Unable to attach archiver's shared memory segment.
20302 %d files recycled from VSN %s (%s:%s)\n
20303 %d files recycled from VSN %s (%s)\n
20304 %d directories recycled from VSN %s (%s:%s)
20305 %d directories recycled from VSN %s (%s)
20306 Post-recycled VSN %s (%s:%s)
20307 Post-recycled VSN %s (%s)
20308 Recycling disk archive set %s (%s)
20309    Information only.  Recycling is ignored on this archive set.
20310    %s:%s
20311    %s
20312 Line too short.
20313  -hwm value is malformed or out of range.
20314  -mingain value is malformed or out of range.
20315 Option '%s' unrecognized or duplicate in recycler command file.
20316 No such library '%s' exists in the system.
20317 Recycle mail address missing
20318  -vsncount is malformed or out of range.
20319    Archiver says busy for %s.%s
20320 No candidate was found in this archive set.
20321 ** NOT USED ** sam_segment_vsn_stat() failed, %s(copy %d, segment %d)
20322 Cannot malloc %d bytes for file system information: %s.
20323 Archive sets file version mismatch.  Is: %d, should be: %d
20324 VSN %s: skipped disk archive tar file: %s because it has zero size. This tar file was supposed to contain the following file(s):
20325 Archive copy %d of %s
20326 getMultiVsnInfo() failed: %s, inode %d.%d, archive copy %d
20327 One or more archive images are expired. See the log for more information
20328 \nNeed to select recycling candidate for %s '%s'.
20329 Checking %s.
20330 Archive copy %d of %s resides on invalid VSN
20331 Section %d of archive copy %d of %s resides on invalid VSN
20332 Section %d of request file %s resides on invalid VSN
20333 Library %s has an invalid VSN in slot %d
20334 \n%-2d Name: %-20s
20335    INDEX                 ty        used        active vsn
20336     %3d   %s %s %s%13s%13s %-20s
20337    Total Used Space:  %s bytes, Total Active Space: %s bytes
20338    Recycling is ignored on this archive set - VSN not marked for recycling.
20339    Volume data inactive %d%%, active %d%% *** Needs recycling ***
20340 recycle candidate
20341 no recycle
20342 empty VSN
20343 no-data VSN
20344 in multiple sets
20345 archive -n files
20346 request files
20347 ignored candidate
20348 \nDisk Media VSNs: %d entries
20349 not a candidate
20350  ----Status-----        Count    Bytes   Use  Obsolete       VSN   Archive Set
20351 %-21s%10s%7s%6d%6d%16s %s
20352    Volume data inactive %d%%, active %d%%
20353 Recycler exiting - new recycler is enabled (nrecycler = yes) in defaults.conf.
20354 Metadata records for the following list of OIDs will be deleted.
20355 The following list of files and directories will be removed.

$  nrecycler      (20400-20699)
20400 \n========== Recycler begins at %s ===========
20401 Ignore option (-n) enabled.  No action will be taken.
20402 Recycler cannot complete due to above error(s).
20403 File system '%s' not mounted.
20404 Cannot read disk volume seqnum file for VSN 'dk.%s'.
20405 Failed to initialize SAM-QFS dump directory '%s'.
20406 Invalid SAM-QFS dump file '%s'.
20407 Cannot access %s file %s.
20408 No preceding '%s' statement
20409 '%s' missing
20410 Cannot attach SAM-FS shared memory segment.
20411 Recycler command error: %d: %s - %s.
20412 Error in recycler command %s.
20413 Recycler exiting - new recycler not enabled (nrecycler = yes) in defaults.conf.
20414 No valid SAM-QFS dump files were found.

$ d2format		(21000s)
$ ==========================================================
21213 %s: Cannot format media in slot %d: Waiting for audit.
21214 Cannot format media in slot %d - waiting for audit
21215 %s: Cannot format cleaning cartridge in slot %d.
21216 Cannot format cleaning cartridge.
21217 %s(%d): Unable to format media in slot %d, barcode %s: Readonly.
21218 Unable to format media in slot %d, barcode %s - readonly.
21219 %s(%d): Unable to format media in slot %d: Readonly.
21220 Unable to format media in slot %d - readonly

$  rft/remote (22000s)
$ ===================================================================
$ These message are sent via SendCustMsg function in rft daemon (was ftpd).
$ They may be preceded by an optional delivery code string that begins with
$ '-'.  The letters may be in any order and may be one of the following:
$     N - Send message to notify executable file.
$     S - Send message to syslog
$ The first letter of the syslog priority codes -
$     Emerg, alert, crit, err, warning, notice, info, debug
$ The default action is: -Si
$ Example: -SNw  sends a "warning" to both syslog and the notify executable.

22000 -Sw Sam rft daemon connection from '%s' is not authorized
22001 -Si Rft daemon shutdown by signal %d.
22002 -Sw Cannot open %s.  No logging will occur.
22003 -Sw No host address %s, errno %d.
22013 -Sw Error in rft command %s.
22014 -Sw Rft command error: %d: %s - %s.
22015
22016 ** NOT USED ** -Sw Unknown trace option %s.
22017 Invalid '%s' value
22018 Unknown internet address family %d
22019 Service name, '%s', failed to register (%d '%s').
22020 accept failed (%d, '%s').

22300 -Si Unable to open configuration file %s.
22301 -Si %s(%#x) Not authorized.
22302 -Si Too many clients-reject %s.
22303 -Si Syntax error in configuration file %s, line %d.
22304 -Si Invalid keyword %s in configuration file %s, line %d.
22305 -Si Too many clients for server %s, connection from '%s' not accepted.
22306 -Si Client '%s' authorized for %s.
22307 -Si Unable to find client '%s' for %s.
22308 -Si Invalid equipment ordinal %d in configuration file %s, line %d.
22309 -Si Equipment ordinal %d not found, configuration file %s, line %d.
22310 -Si Invalid media type %s in configuration file %s, line %d.
22311 -Si Syntax error '%s' in configuration file %s, line %d.
22312 -Si Connection failed to server %s (%d '%s').


$ misc commands (SAM-QFS SunCluster agent)

22600 Failed to retrieve the property %s: %s.
22601 Extension property %s is not set.
22602 Out of memory
22603 Unable to open /etc/vfstab: %s.
22604 Mount point %s is of fstype %s, should be samfs.
22605 Mount point %s should have mount-at-boot set to 'no'.
22606 Mount point %s does not have the '%s' option set.
22607 Mount point %s is not present in /etc/vfstab.
22608 Failed to get state of QFS filesystem %s: %s.
22609 Failed to get cluster handle: %s.
22610 Unable to open %s: %s
22612 Failed to start fault monitor.
22613 %s: Expected switchover not pending.
22614 %s: Waiting for switchover to complete.
22615 %s: Switchover complete.
22616 Failed to get state for node <%s>: %s.
22617 %s: Previous metadata server %s left cluster during a voluntary switchover.
22618 Started the fault monitor.
22619 Failed to stop fault monitor.
22620 Stopped the fault monitor.
22621 Cannot execute %s: %s.
22622 Command %s failed to run: %s.
22623 %s terminated with signal %d
22624 %s stopped with signal %d
22625 %s exited with status %d
22626 %s returned with status %d
22627 %s failed: %s.
22628 Failed to get nodeid for node <%s>: %s.
22629 %s: No master for QFS filesystem.
22630 The old master node %s came back online.
22631 Waiting for client leases to expire.
22632 Couldn't get FS %s param info: %s.
22633 %s: Voluntary failover failed.
22634 Inappropriate path in FS %s device component: %s.
22635 Failure returned by child process %d.  Error: %s
22636 %s: Switchover completed to server (NONE).
22637 Waiting for fencing to complete on %s
22638 Fencing is complete on %s
22639 %s: Waiting for mount change to complete.
22640 Invalid QFS property handle
22641 Fork failed while processing mount '%s'.  Error: %s
22642 Wait for child returned unexpected error: %s
22643 %s: Server %s is not a valid metadata server.
22644 Unable to get cluster handle: %s.
22645 Unable to get local node name: %s.
22646 Unable to get local private link name: %s.
22647 Unable to find the mount point '%s' in /etc/vfstab.
22648 %s: Filesystem is not mounted.
22649 %s: Metadata server not found.
22650 %s: Using involuntary failover.
22651 %s: This node is the metadata server.
22652 %s: The filesystem is online.
22653 %s: Metadata server %s found.
22654 %s: Attempting voluntary failover.
22655 %s: Error determining if %s is up.
22656 %s: Metadata server %s is down.
22657 %s: Switching to involuntary failover.
22658 %s: Clearing the metadata server.
22659 %s: Error clearing the metadata server, err=%s.
22660 %s: Checking cluster for fencing of node %s.
22661 %s: Error waiting for fencing.
22662 %s: Waiting %d seconds for expiration of leases.
22663 %s: Error waiting for client leases.
22664 %s: Error failing over to new server (%s): %s.
22665 %s: Error getting QFS state %d.
22666 %s: This node is not the metadata server.  Current metadata server: %s.
22667 %s: Cluster timed out probe command.
22668 %s: Cluster detected probe failure: %s.
22669 %s: Failure accessing filesystem. Message: %s Command: %s
22670 %s: This node is not the metadata server. Current metadata server is %s.
22671 %s: Unable to get the QFS hosts table.  Error: %s
22672 %s: Unable to find the host %s in the QFS hosts table.
22673 %s: Invalid priority (%s) for server %s
22674 %s: Unable to find private link %s for host %s in the QFS hosts table.
22675 %s: Unable to get partition information:  %s.
22676 %s: Client lease wait is over.
22677 %s: Metadata server is not responding.
22679 %s: Host failover already pending.
22680 %s: Involuntary failover from no server.
22681 %s: Using previous metadata server %s for fencing check.
22683 %s: Unable to get private link name for %s: %s.
22684 %s: Inconsistency found in the QFS hosts table for host '%s'.  Expected to find '%s', found '%s'.
22685 %s: Error while attempting to mount %s.
22686 Error while attempting to unmount %s.
22687 XX-UNUSED-XX %s: Error notifying the server of involuntary failover, err=%s.
22688 Unable to get resource group name.
22689 Unable to get nodelist for resource group %s.
22690 Can't find FS name for mount point %s.
22691 FS %s: Can't determine server.
22692 FS %s: Can't get cluster state for server.
22693 Mount point %s should not have the '%s' option set.
22694 FS %s: No previous master for QFS device.
22695 FS %s: No pending master for QFS device.
22696 FS %s: Couldn't get valid server info.
22697 Unable to open resource group '%s': %s.
22698 Retrying resource group get (%s): %s.


$ mount_samfs (23000s)
$ ==============================================================
23000 File System has been used for WORM files.        <opt mask 0x1>
23001 File System has used shared client sparse files. <opt mask 0x2>

$ WORM (24000s)
$ ==============================================================
24000 Retention period must be longer than current period.
24001 Parent does not exist.
24002 Parent or a hardlink's parent not WORM'd.
24003 Extension header error.
24004 Mount point not WORM'd.
24005 Unknown error when setting retention period.


$ libsammgmt (30000s)
$ ==============================================================
30004 fork exec command %s failed
30010 sammkfs failed: %s
30011 mount failed: %s
30012 umount failed: %s
30013 samgrowfs failed: %s
$ 30020 wrong read mode given, %s required
30021 wrong write mode given, %s required
$ 30021/30020 need to be removed
30030 IO operation %s failed: %s
30031 Invalid process type: %d
30032 Popen %s failed: %s
30033 Process %ld of type %d not found
30034 Process %ld cannot be killed: %s
$ changed 30035 %s contains %d error(s); first error : %s
$ ignoring the last argument %s for 4.1
30035 %s contains %d error(s). Manually edit the file, verify syntactic correctness and retry the operation
30036 Cannot create pipe: %s
30037 Unable to create file %s: %s
30038 Unexpected error reading mcf line %s. Execute /opt/SUNWsamfs/sbin/sam-fsd and fix any reported errors
30039 Unable to delete file %s. The path must be a fully qualified absolute path
30040 ioctl failed: %s 
30041 Invalid SCSI response
30100 Disk/volume discovery failed
30101 Invalid AU (Allocatable Unit) type: %s
30102 Invalid NULL parameter %s
30104 ** NOT USED ** The configuration file %s has been modified outside of this application. If you wish to continue, please retry the operation
30105 Check initialization of %s failed: %s
30106 Checking modification time of %s failed: %s
30107 libsammgmt not initialized
30108 Failed to backup %s: %s
30109 Unable to create the backup directory %s.
30110 %s configuration contains %d error(s). Manually edit the file to correct the error and retry the operation
30111 1 error in configuration file %s. Manually edit the file, verify syntactic correctness and retry the operation
30113 Invalid backup file name: %s
30114 Open failed for %s: %s
30115 Close failed for %s: %s
30116 Family set %s already exists
30117 Family set %s does not exist
30119 Device %d does not exist
30120 %s is not a valid file system type
30121 Device %s(%s) is invalid in file system %s
30122 File system %s contains %s devices and %s devices
30123 Invalid device state %d
30124 Invalid device type %s
30125 Invalid equipment ordinal %d
30126 Cannot dump the configuration to %s
30128 Required field %s is missing
30129 Maximum length of %d exceeded for %s
30130 Field %s contained whitespace characters
30131 %s already exists
30132 %s not found
30133 Setting %s failed: %s
30134 Invalid fsckpass value %s
30135 Unexpected error: %s
30136 Configuration change was unsuccessful: %s
30137 %s %d is out of range %d-%d
30138 %s %d is invalid
30140 %s is not a valid media type.
30141 Malformed regular expression %s
30142 There are not %d available equipment ordinals
30143 Filesystem %s has no free striped group ids
30144 no_recycle_vsn not found for media type %s
30145 no_recycle_vsn already exists for media type %s
30146 Robot %s not found
30149 Releasing directives not found for filesystem %s
30151 Filesystem %s not defined
30152 Drive directive not found for %s
30154 Buffer directive not found for %s
30157 %s cannot be converted to file size
30158 %s cannot be converted to seconds
30159 Logfile %s can not be created
30160 Script %s does not exist
30161 Unable to check robot against mcf
30162 Cannot check file system with mcf
30163 %s value must be greater than %lld
30164 %s value must be less than %lld
30165 %d is not a valid drive count for %s
30166 Invalid age priority type %d
30167 Value %f for %s must >= %f and <= %f
30168 list_size %d must be >= %d and < %d
30169 min_residence_age %d must be > 0
30170 %d is not a valid DAU size
30171 Set live mount options partially failed
30172 Set live mount options failed for all
30173 Mount option %s not defined
30174 Unable to create directory %s: %s
30175 Activating diskvols config failed
30176 Cannot remove the last filesystem while other family sets exist
30177 Removing file %s failed
30178 Retreiving package information failed
30179 Unable to view the file: %s
30180 Release files failed

30200 Unable to retrieve fault information from file %s
30201 Invalid request
30202 Unable to open faultlog: %s
30203 fstat failed for file %s
30204 Invalid severity
30205 Invalid state
30206 File not present: %s
30207 Notification address %s already exists in file %s
30209 Only one email present: cannot delete
30210 Email address %s not found in file %s
30211 No notification files found
30212 Malformed line in file %s. '<<' symbol should have space before it
30213 Log rotation failed for %s: %s
30214 Cannot write to %s: %s
30215 Cannot create directory %s to persist fault history: %s
30216 Cannot open file %s to persist fault information: %s
30217 stat failed for file %s
30218 Rename failed for file %s to %s
30219 Fault %ld not found
30220 Acknowledging faults failed for the fault ids: %s
30221 Deleting faults failed for the fault ids: %s
30222 Only %d faults can be acknowledged at a time. Some faults might not be acknowledged.
30223 Only %d faults can be deleted at a time. Some faults might not be deleted
30224 Email address %s not found for notification type %s
30225 Notification already exists for this address %s
30226 samexplorer failed
30227 A SAM Explorer report is being generated. This may take a few minutes to complete
30228 \t\t\t\tSYSTEM NOTIFICATION\n\n\nSystem\t\t:Sun Microsystem Inc, StorEdge SAM-FS/QFS software\nHost\t\t:%s\nSeverity\t:%s\nTime Stamp\t:%s\n\nProblem:\n
30229 Metadata Snapshot
30230 Acsls
30231 File system
30232 System Notification
30305 Sam remote configuration file %s has no media VSN defined
30309 Open (read mode) failed for file %s
30310 Close failed for file %s
30311 Open (create mode) failed for file %s
30312 Append failed for file %s
30313 Write failed for file %s
30314 Check failed for path %s
30315 popen (write mode) failed for %s
30316 popen (read mode) failed for %s
30317 Pipe write failed for %s
30318 Close pipe failed for %s
30319 Device %s busy
30322 Standalone drive family set %s does not equal '-'
30323 Standalone drive %s family set equipment ordinal does not equal 0
30324 No media library found
30325 Media library %s has mixed media types
30326 Cannot get available equipment ordinals 
30327 Media library %s has no drives defined
30328 No %s specified
30329 Drive not found
30330 No catalog entries found
30332 VSN %s is not found in any catalog
30333 VSN %s and media type %s do not match in any catalog
30334 equipment ordinal %d, slot %d and partition %d do not match in any catalog
30335 No catalog found for library with equipment ordinal %d
30336 End slot number %d is smaller than start slot number %d
30338 No media library with equipment ordinal %d found
30339 Media library configuration in mcf does not match the live system
30340 Media library configuration in live system does not match mcf
30341 Invalid regular expression: %s
30343 Unable to find master shared memory segment (SAM_FS is not running)
30344 Unable to attach master shared memory segment
30346 Invalid equipment ordinal : %d
30347 Equipment ordinal %d is not for a removable media device
30348 Unload: %d is not loaded
30349 Unable to open command fifo
30350 Media library %s already exists
30351 Drive %s already exists
30352 Equipment ordinal %d is not for a tape device
30353 The IBM3494 only supports automatic drive cleaning
30354 Unknown media type: %s
30355 Audit and foreign tape are mutually exclusive
30356 Catalog initialization failed. sam-catserverd might not be running.
30357 The volume specified by equipment ordinal %d, slot %d, and partition %d cannot be found
30358 Unable to get the catalog entry for equipment ordinal %d: %s
30359 Device %d is open by another process
30360 Invalid flag for chmed: %s
30361 Catalog slot initialization failed for %d: %s
30362 Set catalog field failed for %s: %s
30363 Failed to move cartridge from %s: %s
30364 Failed to export cartridge from %s: %s.
30365 Cannot change state to unknown state %d.
30366 Cannot change state to %s: %s
30367 Cannot reserve media located at %s: %s
30368 Cannot unreserve media located at %s: %s
30369 Network attached library type not supported
30370 Cannot import. Error mapping symbol %s: %s
30371 Scratch pools and VSN options are mutually exclusive
30372 The shared object library %s cannot be loaded: %s
30373 Device %d not ready
30374 Device with equipment ordinal %d is not a robotic device
30375 Barcode may only be specified for the historian
30376 Media type may only be specified for the historian
30377 audit_eod option is not allowed for the historian
30378 Media type and VSN or a barcode must be supplied to import into the historian
30379 For Network-attached StorageTek automated libraries, either the VSN or the scratch pool name and number of vsns to be imported from the pool are to be specified as import options.
30380 Import to GRAU, STK, SONY and IBM libraries must be by VSN or scratch pool(STK only)
30381 Scratch pool option only supported on STK libraries
30382 ACSLS query pool failed: %s
30383 ACSLS response failed: %s
30384 ACSLS response: wrong sequence
30385 ACSLS query pool: status error: %s
30386 Query response from StorageTek ACSLS software indicates that the pool is invalid or there are insufficent vsns in the pool to import
30387 ACSLS query pool: wrong pool: %d
30388 Only %d volumes left in pool
30389 ACSLS query scratch failed: %s
30390 ACSLS response(query scratch) failure: %s
30391 ACSLS response(query scratch): wrong sequence
30392 ACSLS query scratch: No scratch volumes
30393 Skipping %s: not home
30394 ACSLS set scratch failed: %s
30395 ACSLS response(set scratch) failure: %s
30396 ACSLS response(set scratch): wrong sequence
30397 Cannot label volume in historian
30398 Blocksize must be one of 16, 32, 64, 128, 256, 512, 1024 or 2048
30399 The VSN name %s must be one to six characters in length
30400 There are invalid characters in VSN %s. All characters in the VSN must be selected  from the  26 upper-case letters, the 10 digits, and the following special characters: !"%&'()*+,-./:;<=>?_. 
30401 Device cannot be labeled
30402 VSN name %s identified from the equipment ordinal and slot does not match the input old label %s
30403 Cannot create response FIFO
30404 Cannot retrieve tplabel command response
30405 wait response can not be completed
30406 VSN not specified
30407 Existing VSN not specified for relabel
30408 String trim call failed
30409 Cannot generate VSNs in the specified range
30410 Invalid robot type %s
30411 Invalid media type %s
30412 %s is not available
30413 Libraries cannot be added until a filesystem is configured
30414 This library is not supported by SAMFS
30415 Auditslot failed with error %s
30416 Check failed for catalog path %s
30417 Catalog file path %s is a directory
30418 Parameter file path %s is a directory
30419 Parameter file %s could not be accessed. Check that the file exists
30420 Equipment ordinal %d does not match in any catalog
30421 This library cannot be removed because of dependency: %s
30422 To change to state %s, current state must be off
30423 To change to state %s, current state must be idle
30424 Change state FIFO write failed
30425 Load failed for VSN at the slot %d in library with equipment ordinal %d: %s
30426 Failed to initialize %s. Probably all drives of the library are busy. Please try "samd stop; samd config; samd start" from /opt/SUNWsamfs/sbin directory in SAM-FS/QFS server
30427 Catalog file %s already exists
30428 samst driver configuration file %s does not exist
30429 To change to state %s, current state must be on
30430 media type %s is not found in any catalog
30431 Cannot open media database
30432 Communication with ACSLS server broken
30433 ACSLS command display error, the status is: %s
30434 ACSLS server response error, the status is: %s
30435 dlopen error: %s
30436 dlsym call error: %s
30437 acsls parameter incoming capacity list is NULL
30438 acsls parameter incoming device list is NULL
30439 System fork failed to start ssi_so daemon
30440 No ACSLS parameter file matches given lib equipment ordinal .
30441 exec cfgadm failed.
30442 exec grep failed.
30443 exec samst ls failed.
30444 exec rmt ls failed.
30445 exec cp failed.
30446 exec mv failed.
30447 exec ps failed.
30448 exec kill failed.
30449 Unable to map the media type received from acsls query to sam media type.
30450 Unable to get the STK ACSLS hostname, perhaps no file systems are mounted and SAM-FS and sam-catserverd are not running
30451 Unknown device type
30452 VSN names given in the start and end range should be of the same length and format

30501 Filesystem %s contains no devices
30502 Filesystem %s contained no data devices
30503 Filesystem %s containsno metadata devices
30504 At least one metadata device must be added in order to grow the filesystem
30506 Device %s already exists
30507 Invalid device path %s
30508 Stripe group contains no disks
30509 A vfstab entry already exists for file system %s
30510 The vfstab entry for %s contains too many fields
30511 The vfstab entry for %s contains too few fields
30512 Unrecognized vfstab error for filesystem %s: %d

30513 Internal error: GetFsStatus failed
30514 Internal error: GetFsInfo(%s) failed
30515 Internal error: GetFsParts(%s) failed
30516 Invalid fs device found (%x)
30518 fork failed
30519 Command ps failed: %s
30520 Cannot open disk %s: %s
30521 Cannot llseek disk %s: %s
30522 Cannot read disk %s: %s
30523 Read incomplete: %s
30524 % checking not implemented
30525 Filesystem %s is mounted
30526 Filesystem %s is not mounted
30527 There are no archiving filesystems mounted. Retry the operation after mounting an archiving file system
30528 %s failed: The connection to %s was refused. It may not be running. If no file systems are mounted this is normal
30529 Copy information must be specified for the archive set
30530 VSN assignments must be specified for the archive set
30531 The number of copies must match the number of VSN assignments
30532 The policy specified for the file system must be an explicit default policy

30601 A VSN copy map already exists for %s
30602 VSN pool %s already exists
30603 Pool %s is used by %s
30605 Copy %s cannot be removed: a VSN map exists for it
30606 Copy %s cannot be removed: params exist for it
30607 Archiver run failed: %s
30608 Archiver idle failed: %s
30609 Archiver restart failed: %s
30610 Archiver stop failed: %s
30611 Get archiverd state failed: %s
30612 Can only reserve by one of directory, user or group
30613 Cannot get ArchReq from %s
30614 ArchReq %s version mismatch. Is: %d, should be: %d
30615 Cannot get ArchReq %s: %s
30616 samd config failed: %s
30617 Archiver configuration contains error(s)
30618 Warning in archiving configuration from /etc/opt/SUNWsamfs/archiver.cmd configuration file.<br>Run /opt/SUNWsamfs/sbin/archiver -lv for details
30619 archiver command abnormally terminated/stopped
30620 Cannot activate archiver configuration
30621 samd start failed: %s
30622 samd stop failed: %s
30623 Checking group failed: %s
30624 It is not permitted to have duplicate file match criteria for a file system
30625 Deletion of the default archive set for a file system is not allowed. You must delete the file system to eliminate the archive set.
30626 Archiver rerun failed: %s
30627 An archiver configuration exists but there are no QFS file systems configured. No archiving will be performed
30628 The setting for scanlist_squash in the archiver.cmd must be either \"on\" or \"off\"
30629 Archive files failed


$ CSN Error messages
30700 Parameters required for registration were omitted
30701 An internal error occured while processing your registration. Note that registration is optional and you can continue to use the software without registering
30710 Registration failed. Unable to write to a file in /etc/opt/SUNWsamfs/csn 
30711 Registration failed. Unable to read a required file in /etc/opt/SUNWsamfs/csn 
30712 Either your login or your password was not valid. Please re-enter your information and try again
30713 The server processing the registration experienced a temporary failure. Please try again at another time. Note that registration is optional and you can continue to use the software without registering
30714 Unable to resolve the proxy server. Check your proxy settings and resubmit your request
30715 Unable to communicate with the registration service. Check your proxy settings and resubmit your request
30716 Registration failed. The certificate provided by the server was not valid. Note that registration is optional and you can continue to use the software without registering
30717 The registration call timed out. Please try again at another time. Note that registration is optional and you can continue to use the software without registering



$ rpc server side connection failures

30809 Unknown File System Manager client
30810 Access from host %s denied. Please run /opt/SUNWsamfs/sbin/fsmadm on the SAM-FS/QFS server to add the File System Manager host name to the list of supported clients
30811 Warning: Cannot access %s. Please ensure that host is accessible over the network. If host is in a different domain, specify hostname.domainname\n

$ sammgmt load API

30901 Unable to find preview shared memory segment. Check if the catalog daemon is running on the SAM_FS server.
30902 Unable to attach preview shared memory segment
30903 Index %d not in preview queue
30904 VSN and index inconsistent
30905 VSN not found
30906 Cannot open cmd FIFO

$ sammgmt stage API

30921 Open failed for file %s: %s
30922 fstat failed for file %s: %s
30923 Memory map failed for file %s: %s
30924 Invalid volume %s
30925 ** NOT USED ** Stager command %s failed: %s
30926 sam_lstat failed for %s: %s
30927 %s: Not a SAM-FS file
30928 sam_cancelstage failed for file %s
30929 Open directory %s failed: %s
30930 Change directory to %s failed: %s
30931 Close directory %s failed: %s
30932 Cannot realloc for directory names: %s
30933 Cannot determine current working directory: %s
30934 Invalid staging file name %s: absolute path expected
30935 Stager daemon is not running
30936 Cannot activate stager configuration
30937 Stage never and associative staging are mutually exclusive
30938 Stage failed. Check the log files for details
30939 Stage failed for one or more files. Check the log files for details
30940 The stager daemon is not running. Check the log files for details
30941 The stager daemon is waiting to be run. Issue a stager run command from the Archive Administrations's Archive Activity page and retry this operation
30942 Stager run failed: %s
30943 Stager idle failed: %s
30944 Clearing stage request failed: %s
30945 Stage failed. Unable to determine API version or decode arguments

$ sammgmt license API

30981 License file does not exist. Please check that the license exists in /etc/opt/SUNWsamfs/LICENSE.software_version e.g.LICENSE.4.3
30982 Failed to read license data (invalid mod time)
30983 Failed to read license data (cannot open)
30984 Failed to read license data (cannot read)
30985 License file is corrupted. Checksum on the license failed. Please validate the license.
30986 Unknown robot %s
30987 Unknown media %s
30988 Out of memory
30989 Unexpected duplicate entry for %s encountered.
30990 Improper iterator access. No more elements are present.
30992 License is not yet enabled.
30993 License is inconsistent. License is valid for a QFS standalone installation but SAM is installed
30994 License is inconsistent. License is valid for a SAM installation but QFS is installed
30995 Failed to get the clients that can securely access this SAM-FS/QFS server from %s : %s

$ SNMP (tapealert sysevent data mapping to user friendly textual data )

31000 Could not get tapealert text for tapelaert flag %llx with scsi type %d and flag length %d
31001 Failed to send SNMP trap %s%s : %s
31002 Incorrect number of arguments provided. Expected %d args, received %d args
31003 alerts are %s\n
31004 enabled
31005 disabled
31006 alerts [on/off]
31007 Failed to send email for %s%s : %s


$ shared filesystems errors

31101 The host configuration was NULL
31102 The host type was not set. It is not possible to create a shared file system with out a host type
31104 No host entry was found in the file system structure for the host %s
31105 The host must contain at least one address
31106 Unable to read the raw hosts file for %s error: %s
31107 Unable to get the hosts configuration for file system %s
31108 Unable to convert the hosts configuration for file system %s. Hosts data not available
31109 Shared file system clients do not have hosts.fs_name file
31110 The file system %s is not shared and therefore has no hosts configuration
31111 It was not possible to determine where the raw hosts file for %s is located. Check that the appropriate devices are configured for this file system on all hosts
31112 Clients can not be deleted while the file system is mounted on any host. Unmount the file system on all hosts and try again
31113 The shared file system cannot be added because no hosts configuration was included. A hosts configuration must be included when initially creating the file system and when adding the file system on a metadata server
31114 Update and write local copy cannot both be set on a call to samsharefs
31115 The file system %s already includes the host %s
31116 The equipment ordinal %d is already in use
31117 The device %s was included in the file system %s but has no super block. Run 'samfsconfig /dev/dsk/*' to check if all of the devices for this file system are available on this host.
31118 No devices were present for file system %s. Run 'samfsconfig /dev/dsk/*' to check if all of the devices for this file system are available on this host.
31119 The device %s was included in the file system %s, but its super block is from a different file system. Run 'samfsconfig /dev/dsk/*' to check if all of the devices for this file system are available
31120 The family set equipment ordinal for file system %s does not match the super block for the device %s. Run 'samfsconfig /dev/dsk/*' to check if all of the devices for this file system are available
31121 %s and %s were included in file system %s but they appear to be from two separate file systems. Run 'samfsconfig /dev/dsk/*' to check this file system
31122 %s and %s were included in file system %s but they appear to be from two separate file systems. Run 'samfsconfig /dev/dsk/*' to check this file system
31123 %s and %s were included in file system %s but they appear to be from two separate file systems. Run 'samfsconfig /dev/dsk/*' to check this file system
31124 %s and %s were included in file system %s but they appear to be from two separate file systems. Run 'samfsconfig /dev/dsk/*' to check this file system
31125 The device type for %s in file system %s does not match the super block. Run 'samfsconfig /dev/dsk/*' to check this file system
31126 The file system %s does not have the same number of metadata devices as the super block reports. Run 'samfsconfig /dev/dsk/*' to check this file system
31127 The file system %s does not have the same number of data devices as the super block reports. Run 'samfsconfig /dev/dsk/*' to check this file system
31128 The file system %s does not have the same number of devices as the super block reports. Run 'samfsconfig /dev/dsk/*' to check this file system
31129 Reordering the devices for %s failed. No device with equipment ordinal %d was found
31130 The equipment ordinal for device %s does not match the super block on that device
31131 Unable to determine what type of shared host %s is for the file system %s
31132 Unable to get the hostname
31133 Unable to read the super block from the devices. Run 'samfsconfig /dev/dsk/*' to check the devices for this file system
31134 The advanced network configuration could not be read. Check the /etc/opt/SUNWsamfs/hosts.local file for this file system
31135 The advanced network configuration could not be set. The input was not well-formed
31136 Unable to obtain the list of valid IP addresses and names for this host.

$ samrestore errors

31200 No value for keyword %s. Possible client/server version mismatch.
31201 Not a recognized keyword %s. Possible client/server version mismatch.
31202 Destination directory %s not usable. Please specify an existing directory.
31203 Script specification %s not usable. Please specify an existing script file.
31204 Bad compress keyword %s
31205 Missing keyword in string %s
31206 Unable to access file %s
31207 Reference to filestructure %s not found.
31208 Not a file %s
31209 Cannot read file %s
31210 Not a recognizable snapshot file %s
31211 Internal database confusion %s
31212 Not a directory %s
31213 A restore job is already in progress.  Please submit your request again later.
31214 File already exists %s
31215 Mismatched argument lengths
31216 Cannot restore segmented file %s
31217 ioctl (F_SAMRESTORE) failed %s
31218 Not a snapshot file %s
31219 User %s does not exist
31220 Group %s does not exist
31221 Could not take snapshot %s
31222 The prescript %s could not be invoked.\nPlease check that the prescript exists in the correct location and is executable
31223 Snapshot operation incomplete for %s, Compression failed.
31224 The uncompressed size of snapshot %s (%s) differs from previous %d snapshots for file system %s by %s.
31225 Restore ioctl (F_IDRESTORE) failed %s
31226 Specifying restore copy on directory %s is not legal.
31227 Illegal stage copy requested %s
31228 The prescript %s failed with error. Please check that the prescript is correct and does not have any errors.\nThe first line of the script must be in this format:\n#! pathname [arg]\nThe ! is essential.
31229 Could not take snapshot %s. Please check that the destination directory for the snapshot exists.
31230 Indexing %s failed.  The snapshot may not be usable.
31231 Could not take snapshot. File system %s is not mounted.\nPlease mount the file system before the next scheduled snapshot
31232 No results available %s
31233 You have been added to receive notifications from File System Manager
31234 You have been added to receive notifications from File System Manager.\n\nThis message was generated by File System Manager.\n\nDo not reply to this message.
31235 No such activity id %s.
31236 Restore failed, %s
31237 Snapshot %s started
31238 Snapshot %s completed
31239 Corrupt snapshot index %s. Delete the index file and try again.
31240 Starting restore %s
31241 Finished restore %s
31242 Index of %s already in progress.
31243 One or more files could not be restored. See the restore log for more information
31244 Snapshot failed, file %s has been removed.
31245 Non-existent or unavailable prescript file %s. Please specify an existing file.
31246 Non-existent or unavailable postscript file %s. Please specify an existing file.
31247 Could not take snapshot for file system %s. The destination for the snapshot, %s, could not be created or is not a directory.
31248 Snapshot file already exists
31249 Snapshot index already exists
31250 Compressed snapshot file already exists
31251 gzipped snapshot file already exists
31252 See %s for more information.
31253 mail message sent to root
31254 Snapshot for %s already in progress, will not continue.
31255 Could not exec %s: %s
31256 Could not fork %s
31257 Creation of metadata snapshot %s is completed but the post-snapshot script %s could not be invoked. \nPlease check that the post-snapshot script exists and is executable. You will have to manually run the script at this time.
31258 Creation of metadata snapshot %s is completed but the post-snapshot script %s failed with error. \nPlease check that the post-snapshot script is correct and run it manually at this time.
31259 Snapshot Notification
31260 File System Overflow Notification
31261 See restore log for details.
31262 Created metadata snapshot directory %s
31263 The time to perform snapshot %s (%.2f hours) differs from previous %d snapshots for file system %s by %.2f hours.
31264 Time to take snapshot %s (%.2f hours) is larger than the interval between scheduled snapshots (%d hours).
31265 The name pattern for scheduled snapshots must include at least the date specifications %%Y, %%m and %%d, and must not include any other date formatting other than %%H and %%M before the supported date specifications.  Snapshot files will not be purged.
31266 %s  removed as directed by retention policy.
31267 Could not set %s as un-deletable.
31268 Could not set %s as deletable.
31269 Snapshot %s is marked as un-deletable
31270 %s is not a valid SAM-FS filesystem.
31271 Could not decompress snapshot %s.
31272 The capacity of the file system %s has exceeded the configured high-water mark
31273 Could not remove index for snapshot %s:  errno = %d
31274 Snapshot failed, file %s has been removed. See restore log for details

$ disk volume errors

31300 Unable to get information from the disk volume dictionary. Execute sam-fsd to check the diskvols.conf file for errors
31301 Setting media flags for the disk volume failed.  Execute sam-fsd to check the diskvols.conf file for errors
31302 The data address for the STK5800 must be specified
31303 A disk volume name must be specified

$ Daemon Description Text

31400 File system daemon
31401 Archiver daemon
31402 File system archive monitor
31403 Stager daemon
31404 Stage-all daemon
31405 Master automated library daemon
31406 Manually mounted device monitor
31407 Media catalog daemon
31408 Management daemon
31409 Robot monitor
31410 Generic SCSI II library daemon
31411 StorageTek library daemon
31412 IBM 3494 library daemon
31413 Sony library daemon
31414 Shared file system daemon
31415 SAM-Remote server
31416 SAM-Remote client
31417 File transfer daemon
31418 Releaser
31419 Recycler

31420 File system check
31421 Tape label
31422 File system mount
31423 File system unmount
31424 File system creation command
31425 Archiver command
31426 archive
31427 release
31428 sam-explorer

31430 Alternate Recycler

31501 Device log
31502 Historian
31503 SAM system log
31504 Archive log
31505 Archive log (global)
31506 Releaser log
31507 Releaser log (global)
31508 Recycle log
31509 Stage log
31510 Log
31511 Trace
31512 On
31513 Off
31514 Restore log
31515 Failed to get system architecture
31516 %s contains invalid characters. Client name should be a valid IP address or a text string from the alphabet, digits, minus sign, underscore and period\n
31517 Solaris system log
31518 Emergency
31519 Alert
31520 Critical
31521 Error
31522 Warning
31523 Notice
31524 Information
31525 Debugging Info

31551 Errors were detected in the /etc/opt/SUNWsamfs/mcf file. Execute sam-fsd and correct any reported problems
31552 Errors were detected in the /etc/opt/SUNWsamfs/samfs.cmd file. Execute sam-fsd and correct any reported problems
31553 Errors were detected in the /etc/opt/SUNWsamfs/diskvols.conf file. Execute sam-fsd and correct any reported problems
31554 Errors were detected in the /etc/opt/SUNWsamfs/defaults.conf file. Execute sam-fsd and correct any reported problems
31555 Errors were detected in the releaser.cmd file. Execute sam-releaser and correct any reported problems
31556 Errors were detected in the recycler.cmd file. Execute 'sam-recycler -n' and correct any reported problems
31557 Errors were detected in the stager.cmd file. Check the file and correct any problems
31558 Errors were detected in the archiver.cmd file. Execute the archiver command and fix any reported problems
31559 There are archive policy copies that have no media assigned. No archiving will be performed. Go to the copy vsns page for each copy to assign vsns
31560 There are archive policy copies that have no available media. No archiving will be performed for these copies. Go to the copy vsns page for each copy to assign vsns
31561 The status of this file cannot be determined because of problems with another file
31562 UNKNOWN
31563 OK
31564 ERROR
31565 WARNING
31566 File not present


$ The following message must match the message for 4529 except for the
$ absence of the %s
31567 has no volumes defined
31568 The mcf file is not present but other files that depend on it are. This is likely to cause problems in your configuration.
31569 The status of this file cannot be determined due to a problem with

$ The following message must match the message for 14004  except for the
$ absence of the %d
31570 Error in line

$ Used in event corelation by the program that writes to faultlog
31600 Repeated %d time(s) from %s
$ Used as the format str for sscanf to read stime (April 4, 2005 5:01:00 PM)
31601 Repeated %d time(s) from %32[aA-zZ0-9:, ]

$ Used for NFS-related errors
31700 Could not determine NFS shares
31701 Could not set NFS shared options for %s
31702 Error parsing NFS shared options
31703 Could not NFS share %s: %s
31704 Could not NFS unshare %s: %s
31705 Could not remove all NFS shared paths from %s
31706 %s does not exist
31707 %s is not a path within %s
31708 Could not start the NFS server.  See the nfsd(1M) man page for how to start the NFS service on your system.
31709 <unknown>

$ used for file util related errors
31800 Unable to get vsn information for the file
31801 Unable to get segment information for the file

$ used for SunCluster-related errors
31900 Unable to retrieve SunCluster version from %s
31901 Unable to retrieve cluster name. Please verify that SunCluster is properly configured and SUNWscdev is installed.
31902 Unable to retrieve the names of the cluster nodes. Please verify that SunCluster is properly configured and SUNWscdev is installed.
31903 Unable to retrieve the status of the cluster nodes. Please verify that SunCluster is properly configured and SUNWscdev is installed.
31904 Unable to retrieve the node ids of the cluster nodes. Please verify that SunCluster is properly configured and SUNWscdev is installed.
31950 Unable to retrieve the state of the Java Web Console.
31951 Unable to run 'smreg' to verify if SunPlex Manager is registered with the Java Web Console

$ used for File System Manager Reports
32000 Send report file %s via email failed, reason: %s.
32001 Unable to open file system HWM exceeding event file %s for reading, reason: %s.
32002 Media Report 
32003 File System Report 
32004 ACSLS Report 
32005 ACSLS hardware error: resource type: %s and resource id: %s. 
32006 ACSLS server configration mismatch: resource type: %s and resource id: %s. 
32007 ACSLS maintance: resource type: %s and resource id: %s. 
32008 ACSLS LSM in degraded mode: resource type: %s and resource id: %s. 
32009 ACSLS server failure: resource type: %s and resource id: %s. 
32010 ACSLS server log failured: resource type: %s and resource id: %s. 
32011 ACSLS: offline: resource type: %s and resource id: %s. 
32012 ACSLS: serial number change: resource type: %s and resource id: %s and new serial number is %s. 
32013 ACSLS: LMU has a new master: resource type: %s and resource id: %s. 
32014 ACSLS: drive clean request: resource type: %s and resource id: %s. 
32015 ACSLS: a new drive added: resource type: %s and resource id: %s. 
32016 Unable to create a thread, reason: %s.
32017 Unable to contact the fsmdb database server.
32018 Unable to generate report: %s
32019 Invalid report type requested (%d)
32020 No metrics available

$ used for Intellistore errors
32100 Deletion of the Default policy is not allowed
32101 Invalid API call. The only valid interface to delete a data class if separate lifecycle is enabled is delete_class
32102 The default data class must have the lowest priority

$ Task Schedule Errors
32156 The schedule type %s is not supported
32157 Deletion of this type of schedule is not allowed
32158 Adding this type of schedule is not allowed
32159 %s is not a valid time period
32160 The schedule requires a starting time
32161 The schedule requires a frequency
32162 The schedule requires a duration
32163 %s is not a valid date specification
32200 Could not get the time at which high water mark was exceeded
