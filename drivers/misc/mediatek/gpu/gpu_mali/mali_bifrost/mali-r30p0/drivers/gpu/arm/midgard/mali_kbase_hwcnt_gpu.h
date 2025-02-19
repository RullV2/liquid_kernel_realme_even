/* SPDX-License-Identifier: GPL-2.0 */
/*
 *
 * (C) COPYRIGHT 2018, 2020-2021 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 *
 */

#ifndef _KBASE_HWCNT_GPU_H_
#define _KBASE_HWCNT_GPU_H_

#include <linux/types.h>

struct kbase_device;
struct kbase_hwcnt_metadata;
struct kbase_hwcnt_enable_map;
struct kbase_hwcnt_dump_buffer;

#define KBASE_HWCNT_V5_BLOCK_TYPE_COUNT 4
#define KBASE_HWCNT_V5_HEADERS_PER_BLOCK 4
#define KBASE_HWCNT_V5_COUNTERS_PER_BLOCK 60
#define KBASE_HWCNT_V5_VALUES_PER_BLOCK                                        \
	(KBASE_HWCNT_V5_HEADERS_PER_BLOCK + KBASE_HWCNT_V5_COUNTERS_PER_BLOCK)
/** Index of the PRFCNT_EN header into a V5 counter block */
#define KBASE_HWCNT_V5_PRFCNT_EN_HEADER 2

/**
 * enum kbase_hwcnt_gpu_group_type - GPU hardware counter group types, used to
 *                                   identify metadata groups.
 * @KBASE_HWCNT_GPU_GROUP_TYPE_V5: GPU V5 group type.
 */
enum kbase_hwcnt_gpu_group_type {
	KBASE_HWCNT_GPU_GROUP_TYPE_V5,
};

/**
 * enum kbase_hwcnt_gpu_v5_block_type - GPU V5 hardware counter block types,
 *                                      used to identify metadata blocks.
 * @KBASE_HWCNT_GPU_V5_BLOCK_TYPE_PERF_UNDEFINED: Undefined block (e.g. if a
 *                                                counter set that a block
 *                                                doesn't support is used).
 * @KBASE_HWCNT_GPU_V5_BLOCK_TYPE_PERF_FE:        Front End block (Job manager
 *                                                or CSF HW).
 * @KBASE_HWCNT_GPU_V5_BLOCK_TYPE_PERF_FE2:       Secondary Front End block (Job
 *                                                manager or CSF HW).
 * @KBASE_HWCNT_GPU_V5_BLOCK_TYPE_PERF_FE3:       Tertiary Front End block (Job
 *                                                manager or CSF HW).
 * @KBASE_HWCNT_GPU_V5_BLOCK_TYPE_PERF_TILER:     Tiler block.
 * @KBASE_HWCNT_GPU_V5_BLOCK_TYPE_PERF_SC:        Shader Core block.
 * @KBASE_HWCNT_GPU_V5_BLOCK_TYPE_PERF_SC2:       Secondary Shader Core block.
 * @KBASE_HWCNT_GPU_V5_BLOCK_TYPE_PERF_SC3:       Tertiary Shader Core block.
 * @KBASE_HWCNT_GPU_V5_BLOCK_TYPE_PERF_MEMSYS:    Memsys block.
 * @KBASE_HWCNT_GPU_V5_BLOCK_TYPE_PERF_MEMSYS2:   Secondary Memsys block.
 */
enum kbase_hwcnt_gpu_v5_block_type {
	KBASE_HWCNT_GPU_V5_BLOCK_TYPE_PERF_UNDEFINED,
	KBASE_HWCNT_GPU_V5_BLOCK_TYPE_PERF_FE,
	KBASE_HWCNT_GPU_V5_BLOCK_TYPE_PERF_FE2,
	KBASE_HWCNT_GPU_V5_BLOCK_TYPE_PERF_FE3,
	KBASE_HWCNT_GPU_V5_BLOCK_TYPE_PERF_TILER,
	KBASE_HWCNT_GPU_V5_BLOCK_TYPE_PERF_SC,
	KBASE_HWCNT_GPU_V5_BLOCK_TYPE_PERF_SC2,
	KBASE_HWCNT_GPU_V5_BLOCK_TYPE_PERF_SC3,
	KBASE_HWCNT_GPU_V5_BLOCK_TYPE_PERF_MEMSYS,
	KBASE_HWCNT_GPU_V5_BLOCK_TYPE_PERF_MEMSYS2,
};

/**
 * enum kbase_hwcnt_set - GPU hardware counter sets
 * @KBASE_HWCNT_SET_PRIMARY:   The Primary set of counters
 * @KBASE_HWCNT_SET_SECONDARY: The Secondary set of counters
 * @KBASE_HWCNT_SET_TERTIARY:  The Tertiary set of counters
 */
enum kbase_hwcnt_set {
	KBASE_HWCNT_SET_PRIMARY,
	KBASE_HWCNT_SET_SECONDARY,
	KBASE_HWCNT_SET_TERTIARY,
};

/**
 * struct kbase_hwcnt_physical_enable_map - Representation of enable map
 *                                          directly used by GPU.
 * @fe_bm:     Front end (JM/CSHW) counters selection bitmask.
 * @shader_bm: Shader counters selection bitmask.
 * @tiler_bm:  Tiler counters selection bitmask.
 * @mmu_l2_bm: MMU_L2 counters selection bitmask.
 */
struct kbase_hwcnt_physical_enable_map {
	u32 fe_bm;
	u32 shader_bm;
	u32 tiler_bm;
	u32 mmu_l2_bm;
};

/*
 * Values for Hardware Counter SET_SELECT value.
 * Directly passed to HW.
 */
enum kbase_hwcnt_physical_set {
	KBASE_HWCNT_PHYSICAL_SET_PRIMARY = 0,
	KBASE_HWCNT_PHYSICAL_SET_SECONDARY = 1,
	KBASE_HWCNT_PHYSICAL_SET_TERTIARY = 2,
};

/**
 * struct kbase_hwcnt_gpu_info - Information about hwcnt blocks on the GPUs.
 * @l2_count:   L2 cache count.
 * @core_mask:  Shader core mask. May be sparse.
 * @clk_cnt:    Number of clock domains available.
 */
struct kbase_hwcnt_gpu_info {
	size_t l2_count;
	u64 core_mask;
	u8 clk_cnt;
};

/**
 * kbase_hwcnt_jm_metadata_create() - Create hardware counter metadata for the
 *                                    JM GPUs.
 * @info:           Non-NULL pointer to info struct.
 * @counter_set:    The performance counter set used.
 * @out_metadata:   Non-NULL pointer to where created metadata is stored on
 *                  success.
 * @out_dump_bytes: Non-NULL pointer to where the size of the GPU counter dump
 *                  buffer is stored on success.
 *
 * Return: 0 on success, else error code.
 */
int kbase_hwcnt_jm_metadata_create(
	const struct kbase_hwcnt_gpu_info *info,
	enum kbase_hwcnt_set counter_set,
	const struct kbase_hwcnt_metadata **out_metadata,
	size_t *out_dump_bytes);

/**
 * kbase_hwcnt_jm_metadata_destroy() - Destroy JM GPU hardware counter metadata.
 *
 * @metadata: Pointer to metadata to destroy.
 */
void kbase_hwcnt_jm_metadata_destroy(
	const struct kbase_hwcnt_metadata *metadata);

/**
 * kbase_hwcnt_csf_metadata_create() - Create hardware counter metadata for the
 *                                     CSF GPUs.
 * @info:           Non-NULL pointer to info struct.
 * @counter_set:    The performance counter set used.
 * @out_metadata:   Non-NULL pointer to where created metadata is stored on
 *                  success.
 *
 * Return: 0 on success, else error code.
 */
int kbase_hwcnt_csf_metadata_create(
	const struct kbase_hwcnt_gpu_info *info,
	enum kbase_hwcnt_set counter_set,
	const struct kbase_hwcnt_metadata **out_metadata);

/**
 * kbase_hwcnt_csf_metadata_destroy() - Destroy CSF GPU hardware counter
 *                                      metadata.
 * @metadata: Pointer to metadata to destroy.
 */
void kbase_hwcnt_csf_metadata_destroy(
	const struct kbase_hwcnt_metadata *metadata);

/**
 * kbase_hwcnt_jm_dump_get() - Copy or accumulate enabled counters from the raw
 *                             dump buffer in src into the dump buffer
 *                             abstraction in dst.
 * @dst:            Non-NULL pointer to dst dump buffer.
 * @src:            Non-NULL pointer to src raw dump buffer, of same length
 *                  as returned in out_dump_bytes parameter of
 *                  kbase_hwcnt_jm_metadata_create.
 * @dst_enable_map: Non-NULL pointer to enable map specifying enabled values.
 * @pm_core_mask:   PM state synchronized shaders core mask with the dump.
 * @accumulate:     True if counters in src should be accumulated into dst,
 *                  rather than copied.
 *
 * The dst and dst_enable_map MUST have been created from the same metadata as
 * returned from the call to kbase_hwcnt_jm_metadata_create as was used to get
 * the length of src.
 *
 * Return: 0 on success, else error code.
 */
int kbase_hwcnt_jm_dump_get(struct kbase_hwcnt_dump_buffer *dst, void *src,
			    const struct kbase_hwcnt_enable_map *dst_enable_map,
			    const u64 pm_core_mask, bool accumulate);

/**
 * kbase_hwcnt_csf_dump_get() - Copy or accumulate enabled counters from the raw
 *                              dump buffer in src into the dump buffer
 *                              abstraction in dst.
 * @dst:            Non-NULL pointer to dst dump buffer.
 * @src:            Non-NULL pointer to src raw dump buffer, of same length
 *                  as returned in out_dump_bytes parameter of
 *                  kbase_hwcnt_csf_metadata_create.
 * @dst_enable_map: Non-NULL pointer to enable map specifying enabled values.
 * @accumulate:     True if counters in src should be accumulated into dst,
 *                  rather than copied.
 *
 * The dst and dst_enable_map MUST have been created from the same metadata as
 * returned from the call to kbase_hwcnt_csf_metadata_create as was used to get
 * the length of src.
 *
 * Return: 0 on success, else error code.
 */
int kbase_hwcnt_csf_dump_get(
	struct kbase_hwcnt_dump_buffer *dst, void *src,
	const struct kbase_hwcnt_enable_map *dst_enable_map,
	bool accumulate);

/**
 * kbase_hwcnt_gpu_enable_map_to_physical() - Convert an enable map abstraction
 *                                            into a physical enable map.
 * @dst: Non-NULL pointer to dst physical enable map.
 * @src: Non-NULL pointer to src enable map abstraction.
 *
 * The src must have been created from a metadata returned from a call to
 * kbase_hwcnt_jm_metadata_create or kbase_hwcnt_csf_metadata_create.
 *
 * This is a lossy conversion, as the enable map abstraction has one bit per
 * individual counter block value, but the physical enable map uses 1 bit for
 * every 4 counters, shared over all instances of a block.
 */
void kbase_hwcnt_gpu_enable_map_to_physical(
	struct kbase_hwcnt_physical_enable_map *dst,
	const struct kbase_hwcnt_enable_map *src);

/**
 * kbase_hwcnt_gpu_set_to_physical() - Map counter set selection to physical
 *                                     SET_SELECT value.
 *
 * @dst: Non-NULL pointer to dst physical SET_SELECT value.
 * @src: Non-NULL pointer to src counter set selection.
 */
void kbase_hwcnt_gpu_set_to_physical(enum kbase_hwcnt_physical_set *dst,
				     enum kbase_hwcnt_set src);

/**
 * kbase_hwcnt_gpu_enable_map_from_physical() - Convert a physical enable map to
 *                                              an enable map abstraction.
 * @dst: Non-NULL pointer to dst enable map abstraction.
 * @src: Non-NULL pointer to src physical enable map.
 *
 * The dst must have been created from a metadata returned from a call to
 * kbase_hwcnt_jm_metadata_create or kbase_hwcnt_csf_metadata_create.
 *
 * This is a lossy conversion, as the physical enable map can technically
 * support counter blocks with 128 counters each, but no hardware actually uses
 * more than 64, so the enable map abstraction has nowhere to store the enable
 * information for the 64 non-existent counters.
 */
void kbase_hwcnt_gpu_enable_map_from_physical(
	struct kbase_hwcnt_enable_map *dst,
	const struct kbase_hwcnt_physical_enable_map *src);

/**
 * kbase_hwcnt_gpu_patch_dump_headers() - Patch all the performance counter
 *                                        enable headers in a dump buffer to
 *                                        reflect the specified enable map.
 * @buf:        Non-NULL pointer to dump buffer to patch.
 * @enable_map: Non-NULL pointer to enable map.
 *
 * The buf and enable_map must have been created from a metadata returned from
 * a call to kbase_hwcnt_jm_metadata_create or kbase_hwcnt_csf_metadata_create.
 *
 * This function should be used before handing off a dump buffer over the
 * kernel-user boundary, to ensure the header is accurate for the enable map
 * used by the user.
 */
void kbase_hwcnt_gpu_patch_dump_headers(
	struct kbase_hwcnt_dump_buffer *buf,
	const struct kbase_hwcnt_enable_map *enable_map);

#endif /* _KBASE_HWCNT_GPU_H_ */
