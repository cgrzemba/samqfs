/*
 *  proto.h - Function prototypes
 */

/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at pkg/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at pkg/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#ifndef _AML_PROTO_H
#define	_AML_PROTO_H

#pragma ident "$Revision: 1.18 $"

#include "sam/osversion.h"
#include "aml/robots.h"

int add_preview_fs(struct sam_handle *, sam_resource_t *,
	float, enum callback);
int check_preview_status(struct dev_ent *);
int check_for_vsn(char *, int);
int create_bof(int, struct dev_ent *, sam_resource_t *);
int create_optic_eof(int, struct dev_ent *, sam_resource_t *);
int create_tape_eof(int *, struct dev_ent *, sam_resource_t *);
char *dt_to_nm(int);
int find_file(int, struct dev_ent *, sam_resource_t *);
int find_tape_file(int, struct dev_ent *, sam_resource_t *);
int get_capacity(int, struct dev_ent *);
int open_tape(struct dev_ent *, pid_t, int);
int position_tape(struct dev_ent *, int, uint_t);
int position_tape_offset(struct dev_ent *, int, uint_t, uint_t);
int post_dev_mes(struct dev_ent *, char *, int);
int post_sys_mes(char *, int);
int read_position(struct dev_ent *, int, uint_t *);
int open_unit(struct dev_ent *, char *, int);
#ifndef ROB_CMD
enum sam_scsi_action process_scsi_error(struct dev_ent *,
	struct sam_scsi_err *, int);
#endif
int do_tur(dev_ent_t *un, int open_fd, int retry);
uint64_t read_tape_space(dev_ent_t *un, int open_fd);
void set_bad_media(dev_ent_t *);
int dodirio(sam_io_reader_t *control, sam_actmnt_t *actmnt_req);
int scsi_cmd(const int, struct dev_ent *, const int, const int, ...);
int scsi_reset(const int, struct dev_ent *);
int send_robot_todo(enum todo_sub, equ_t, equ_t, enum callback,
	struct sam_handle *, sam_resource_t *, void *);
int send_tp_todo(enum todo_sub, dev_ent_t *, struct sam_handle *,
	sam_resource_t *, enum callback callback);
void send_sp_todo(enum todo_sub, dev_ent_t *, struct sam_handle *,
	sam_resource_t *, enum callback callback);
int send_scanner_todo(enum todo_sub, struct dev_ent *, enum callback,
	struct sam_handle *, sam_resource_t *, void *);
int spin_unit(dev_ent_t *, char *, int *, int, int);
int standalone(void);
int tape_append(int, struct dev_ent *, sam_resource_t *);
int vfyansi_label(void *, int);

struct dev_ent *find_historian(void);
struct preview *scan_preview(int, struct CatalogEntry *, const int,
	void *, int (*)(struct preview *,
	struct CatalogEntry *ce, void *, uint_t, int));

int handles_match(sam_handle_t *, sam_handle_t *);
void send_notify_email(dev_ent_t *device, char *script);
uint64_t read_tape_capacity(struct dev_ent *, int);

char *samst_devname(struct dev_ent *);
char *error_handler(int);
time_t thread_mktime(struct tm *);

void ChangeMode(const char *, const int);
void DownDevice(dev_ent_t *un, int source_flag);
void OffDevice(dev_ent_t *un, int source_flag);
void HandleMediaInOffDevice(dev_ent_t *un);
void OnDevice(dev_ent_t *un);
void *add_preview_cmd(void *);
void *delete_preview_cmd(void *);
void *malloc_wait(const size_t, const uint_t, const int);
void *valloc_wait(const size_t, const uint_t, const int);
void *mount_thread(void *);
void *remove_stale_preview(void *);
void *wt_labels(void *);
void assert_compression(int, dev_ent_t *);
void check_preview(struct dev_ent *);
void clear_un_fields(struct dev_ent *);
void close_unit(struct dev_ent *, int *);
void dtb(uchar_t *string, const int llen);
void format_cdb(int, void *);
void format_sense(int, void *);
void ident_dev(struct dev_ent *, int);
void ld_devices(equ_t);
void mem_map_fs();
void notify_fs_invalid_cache(struct dev_ent *);
void notify_fs_mount(struct sam_handle *, sam_resource_t *,
	struct dev_ent *, const int);
void notify_tp_mount(struct sam_handle *, struct dev_ent *, const int);
void notify_fs_unload(struct sam_handle *, u_longlong_t, const int);
void process_optic_labels(int, struct dev_ent *);
void process_tape_labels(int, struct dev_ent *);
void remove_preview_ent(struct preview *, struct dev_ent *, int, int);
void remove_preview_handle(struct sam_fs_fifo *, enum callback);
void scan_a_device(struct dev_ent *, int);
void send_fs_error(struct sam_handle *, const int);
void signal_fsumount(equ_t);
void signal_preview(void);
#if defined(_AML_CATALOG_H)
#define	UpdateCatalog(a, b, c) _UpdateCatalog(__FILE__, __LINE__, \
	(a), (b), _##c)
void _UpdateCatalog(const char *SrcFile, const int SrcLine, struct dev_ent *un,
	uint32_t status, int (*func)(const char *SrcFile, const int SrcLine,
	struct CatalogEntry *ce));
#define	CatalogMediaUnloaded(a, b, c, d, e) _CatalogMediaUnloaded(_SrcFile, \
	__LINE__, (a), (b), (c), (d), (e))
int _CatalogMediaUnloaded(const char *SrcFile, const int SrcLine,
	char *media_type, vsn_t vsn, int eq, int slot, char *string);
#endif /* defined(_AML_CATALOG_H) */

void zfn(char *, char *, const int);

#if defined(_AML_MESSAGE_H)
void unload_media(struct dev_ent *, struct todo_request *);
#endif

#if defined(_SAM_DEFAULTS_H)
void vsn_from_barcode(char *, char *, sam_defaults_t *, int);
#endif

#if defined(_AML_ODLABELS_H)
int blank_label_optic(int, dev_ent_t *, dkpri_label_t *, dkpart_label_t *,
	int, int);
int write_labels(int, dev_ent_t *, label_req_t *);
#endif

#if defined(_AML_TPLABELS_H)
int write_tape_labels(int *, dev_ent_t *, label_req_t *);
int format_tape(int *, dev_ent_t *, format_req_t *);
#endif

#if defined(_AML_EXIT_FIFO_H)
int create_exit_FIFO(exit_FIFO_id *);
int unlink_exit_FIFO(exit_FIFO_id *);
void write_client_exit_string(exit_FIFO_id *, int, char *);
int read_server_exit_string(exit_FIFO_id *, int *, char *, int, int);
int timeout_factor(equ_t);
#endif

/*
 * these two prototypes are found in /usr/include/time.h, but for reasons
 * we cannot determine the lint program doesn't pick them up.
 */

#ifndef __PRAGMA_REDEFINE_EXTNAME
extern char *ctime_r(const time_t *, char *, int);
#endif
extern struct tm *localtime_r(const time_t *, struct tm *);

#endif /* _AML_PROTO_H */
