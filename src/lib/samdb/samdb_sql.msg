$ *      SAM-DB samdb_sql.msg
$ *
$ *    SAM-QFS_notice_begin
$ *
$ * CDDL HEADER START
$ *
$ * The contents of this file are subject to the terms of the
$ * Common Development and Distribution License (the "License").
$ * You may not use this file except in compliance with the License.
$ *
$ * You can obtain a copy of the license at pkg/OPENSOLARIS.LICENSE
$ * or http://www.opensolaris.org/os/licensing.
$ * See the License for the specific language governing permissions
$ * and limitations under the License.
$ *
$ * When distributing Covered Code, include this CDDL HEADER in each
$ * file and include the License file at pkg/OPENSOLARIS.LICENSE.
$ * If applicable, add the following below this CDDL HEADER, with the
$ * fields enclosed by brackets "[]" replaced with your own identifying
$ * information: Portions Copyright [yyyy] [name of copyright owner]
$ *
$ * CDDL HEADER END
$ *
$ * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
$ * Use is subject to license terms.
$ *
$ *    SAM-QFS_notice_end
$ *
$ pragma ident "$Revision: 1.2 $"

$ *  set assignment:		set 1			All SAMDB Prepared Statements
$ *					set 2-256		Not-assigned
$set 1
$quote "

$  Catalog Index
$  ---------------------
$  sam_inode		(1000s)
$  sam_path		(2000s)
$  sam_file		(3000s)
$  sam_archive	(4000s)

$  sam_inode SQL (1000-1999)
$  ===================================================================
1000 INSERT INTO sam_inode (ino, gen, type, size, csum, create_time, modify_time, uid, gid, online) VALUES (?,?,?,?,?,?,?,?,?,?)
1100 SELECT ino, gen, type, size, csum, create_time, modify_time, uid, gid, online FROM sam_inode WHERE ino=? AND gen=?
1200 UPDATE sam_inode SET size=?, csum=?, modify_time=?, uid=?, gid=?, online=? WHERE ino=? AND gen=?
1201 UPDATE sam_inode SET online=? WHERE ino=? AND gen=?
1300 DELETE FROM sam_inode WHERE ino=? AND gen=?

$  sam_path SQL (2000-2999)
$  ===================================================================
2000 INSERT INTO sam_path (ino, gen, path) VALUES (?,?,?)
2100 SELECT ino, gen, path FROM sam_path WHERE ino=? AND gen=?
2200 UPDATE sam_path SET path=? WHERE ino=? AND gen=? 
2201 UPDATE sam_path SET path=concat(?, substring_index(path, ?, -1)) WHERE path LIKE ?
2300 DELETE FROM sam_path WHERE ino=? AND gen=?

$  sam_file SQL (3000-3999)
$  ===================================================================
3000 INSERT INTO sam_file (p_ino, p_gen, name_hash, name, ino, gen) VALUES (?,?,?,?,?,?)
3100 SELECT p_ino, p_gen, name_hash, name, ino, gen FROM sam_file WHERE p_ino=? AND p_gen=? \
AND name=?
3101 SELECT count(*) FROM sam_file WHERE p_ino=? AND p_gen=? AND name_hash=? AND ino=? AND gen=?
3200 UPDATE sam_file SET p_ino=?, p_gen=?, name=?, name_hash=? WHERE p_ino=? AND p_gen=? AND name=?
3201 UPDATE sam_file SET p_ino=?, p_gen=?, name=?, name_hash=? WHERE p_ino=? AND p_gen=? \
AND name_hash=? AND ino=? AND gen=?
3300 DELETE FROM sam_file WHERE p_ino=? AND p_gen=? AND name=?
3301 DELETE FROM sam_file WHERE p_ino=? AND p_gen=? AND name_hash=? AND ino=? AND gen=?
3302 DELETE FROM sam_file WHERE p_ino=? AND p_gen=?
3303 DELETE FROM sam_file WHERE ino=? AND gen=?
3304 DELETE FROM sam_file WHERE p_ino=? AND p_gen=? AND ino=? AND gen=?


$  sam_archive SQL (4000-4999)
$  ===================================================================
4000 REPLACE INTO sam_archive (ino, gen, copy, seq, media_type, vsn, position, offset, size, create_time, stale) \
VALUES (?,?,?,?,?,?,?,?,?,?,?)
4100 SELECT ino, gen, copy, seq, media_type, vsn, position, offset, size, create_time, stale \
FROM sam_archive WHERE ino=? AND gen=? AND copy=?
4200 UPDATE sam_archive SET stale=? WHERE ino=? AND gen=?
4300 DELETE FROM sam_archive WHERE ino=? AND gen=? AND copy=?
4301 DELETE FROM sam_archive WHERE ino=? AND gen=?
