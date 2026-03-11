/*
 *  sblk_utility.h - common definitions for sammkfs and samfsck.
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
 * or https://illumos.org/license/CDDL.
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
 * Use is subject to license terms.
 *
 */

#ifndef SAM_SBLK_UTILITY_H
#define	SAM_SBLK_UTILITY_H

static inline time_t get_sblk_init_time(struct sam_sbinfo sb)
{
	if (sb.magic == SAMFS_SBLKV3)
		return sb.init64;
	else
		return sb.init;
}

static inline time_t get_sblk_time_time(struct sam_sbinfo sb)
{
	if (sb.magic == SAMFS_SBLKV3)
		return sb.time64;
	else
		return sb.time;
}

static inline time_t get_sblk_repaired_time(struct sam_sbinfo sb)
{
	if (sb.magic == SAMFS_SBLKV3)
		return sb.repaired64;
	else
		return sb.repaired;
}

static inline void set_sblk_init_time(struct sam_sbinfo sb, time_t time)
{
	if (sb.magic == SAMFS_SBLKV3)
		sb.init64 = time;
	else
		sb.init = (time32_t) time & 0xffffffff;
}

static inline void set_sblk_time_time(struct sam_sbinfo sb, time_t time)
{
	if (sb.magic == SAMFS_SBLKV3)
		sb.time64 = time;
	else
		sb.time = (time32_t) time & 0xffffffff;
}

static inline void set_sblk_repaired_time(struct sam_sbinfo sb, time_t time)
{
	if (sb.magic == SAMFS_SBLKV3)
		sb.repaired64 = time;
	else
		sb.repaired = (time32_t) time & 0xffffffff;
}

#endif /* SAM_SBLK_UTILITY_H */
