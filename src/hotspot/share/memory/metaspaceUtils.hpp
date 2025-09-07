/*
 * Copyright (c) 2021, 2023, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2021 SAP SE. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 *
 */
#ifndef SHARE_MEMORY_METASPACEUTILS_HPP
#define SHARE_MEMORY_METASPACEUTILS_HPP

#include "memory/metaspace.hpp"
#include "memory/metaspaceChunkFreeListSummary.hpp"
#include "memory/metaspaceStats.hpp"

class outputStream;

// Metaspace are deallocated when their class loader are GC'ed.
// This class implements a policy for inducing GC's to recover
// Metaspaces.
// 管理触发 GC 的策略，决定何时通过 GC 回收类加载器关联的元空间
class MetaspaceGCThresholdUpdater : public AllStatic {
 public:
  enum Type {
    ComputeNewSize, // 重新计算 GC 高水位标记
    ExpandAndAllocate, // 扩展元空间并分配内存
    Last
  };

  static const char* to_string(MetaspaceGCThresholdUpdater::Type updater) {
    switch (updater) {
      case ComputeNewSize:
        return "compute_new_size";
      case ExpandAndAllocate:
        return "expand_and_allocate";
      default:
        assert(false, "Got bad updater: %d", (int) updater);
        return nullptr;
    };
  }
};

// 维护元空间的 GC 阈值（_capacity_until_GC），控制内存扩展与回收逻辑
class MetaspaceGC : public AllStatic {

  // The current high-water-mark for inducing a GC.
  // When committed memory of all metaspaces reaches this value,
  // a GC is induced and the value is increased. Size is in bytes.
  static volatile size_t _capacity_until_GC; // 触发 GC 的高水位标记（字节）
  static uint _shrink_factor; // 内存收缩比例因子

  static size_t shrink_factor() { return _shrink_factor; }
  void set_shrink_factor(uint v) { _shrink_factor = v; }

 public:
  // 初始化 GC 阈值管理
  static void initialize();
  static void post_initialize();

  // 获取当前高水位标记
  static size_t capacity_until_GC();

  // 动态调整高水位标记
  static bool inc_capacity_until_GC(size_t v,
                                    size_t* new_cap_until_GC = nullptr,
                                    size_t* old_cap_until_GC = nullptr,
                                    bool* can_retry = nullptr);
  static size_t dec_capacity_until_GC(size_t v);

  // The amount to increase the high-water-mark (_capacity_until_GC)
  // 计算高水位标记的增量
  static size_t delta_capacity_until_GC(size_t bytes);

  // Tells if we have can expand metaspace without hitting set limits.
  // 判断是否可扩展元空间而无需触发 GC
  static bool can_expand(size_t words, bool is_class);

  // Returns amount that we can expand without hitting a GC,
  // measured in words.
  // 返回无 GC 情况下的最大扩展空间（以字为单位
  static size_t allowed_expansion();

  // Calculate the new high-water mark at which to induce
  // a GC.
  // 重新计算高水位标记
  static void compute_new_size();
};

// 提供元空间的内存统计与调试工具
class MetaspaceUtils : AllStatic {
public:

  // Committed space actually in use by Metadata
  // 已使用的元空间字数/字节数（支持按类型过滤）
  static size_t used_words();
  static size_t used_words(Metaspace::MetadataType mdtype);

  // Space committed for Metaspace
  // 已提交的内存字数/字节数
  static size_t committed_words();
  static size_t committed_words(Metaspace::MetadataType mdtype);

  // Space reserved for Metaspace
  // 保留的总内存字数/字节数
  static size_t reserved_words();
  static size_t reserved_words(Metaspace::MetadataType mdtype);

  // _bytes() variants for convenience...
  static size_t used_bytes()                                    { return used_words() * BytesPerWord; }
  static size_t used_bytes(Metaspace::MetadataType mdtype)      { return used_words(mdtype) * BytesPerWord; }
  static size_t committed_bytes()                               { return committed_words() * BytesPerWord; }
  static size_t committed_bytes(Metaspace::MetadataType mdtype) { return committed_words(mdtype) * BytesPerWord; }
  static size_t reserved_bytes()                                { return reserved_words() * BytesPerWord; }
  static size_t reserved_bytes(Metaspace::MetadataType mdtype)  { return reserved_words(mdtype) * BytesPerWord; }

  // Retrieve all statistics in one go; make sure the values are consistent.
  // 获取详细的统计信息
  static MetaspaceStats get_statistics(Metaspace::MetadataType mdtype);
  static MetaspaceCombinedStats get_combined_statistics();

  // (See JDK-8251342). Implement or Consolidate.
  // 返回元空间空闲列表的统计信息（占位实现）
  static MetaspaceChunkFreeListSummary chunk_free_list_summary(Metaspace::MetadataType mdtype) {
    return MetaspaceChunkFreeListSummary(0,0,0,0,0,0,0,0);
  }

  // Log change in used metadata.
  // 打印元空间使用量变化
  static void print_metaspace_change(const MetaspaceCombinedStats& pre_meta_values);

  // This will print out a basic metaspace usage report but
  // unlike print_report() is guaranteed not to lock or to walk the CLDG.
  // 输出简略的元空间状态报告
  static void print_basic_report(outputStream* st, size_t scale = 0);

  // Prints a report about the current metaspace state.
  // Function will walk the CLDG and will lock the expand lock; if that is not
  // convenient, use print_basic_report() instead.
  // 输出详细的元空间状态报告
  static void print_report(outputStream* out, size_t scale = 0);

  // 向输出流打印元空间信息
  static void print_on(outputStream * out);

  DEBUG_ONLY(static void verify();)

};

#endif // SHARE_MEMORY_METASPACEUTILS_HPP
