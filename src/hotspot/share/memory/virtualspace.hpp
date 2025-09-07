/*
 * Copyright (c) 1997, 2025, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SHARE_MEMORY_VIRTUALSPACE_HPP
#define SHARE_MEMORY_VIRTUALSPACE_HPP

#include "utilities/globalDefinitions.hpp"
#include "utilities/macros.hpp"

class outputStream;
class ReservedSpace;

// VirtualSpace is data structure for committing a previously reserved address range in smaller chunks.
// 该类用于高效管理一块预先保留（Reserved）的地址范围，支持按需分块提交（Commit）物理内存。适用于需要动态调整内存使用场景（如 JVM 堆内存管理）
class VirtualSpace {
  friend class VMStructs;
 private:
  // Reserved area 预留内存的起始和结束地址
  char* _low_boundary;
  char* _high_boundary;

  // Committed area 当前已提交内存的起始和结束地址
  char* _low;
  char* _high;

  // The entire space has been committed and pinned in memory, no
  // os::commit_memory() or os::uncommit_memory().
  // 若为 true，表示内存已全部提交且不可修改（无需后续 OS 操作）
  bool _special;

  // Need to know if commit should be executable. 标识提交的内存是否可执行（安全性相关）
  bool   _executable;

  // MPSS Support 多页大小支持
  // Each virtualspace region has a lower, middle, and upper region.
  // Each region has an end boundary and a high pointer which is the
  // high water mark for the last allocated byte.
  // The lower and upper unaligned to LargePageSizeInBytes uses default page.
  // size.  The middle region uses large page size.
  char* _lower_high;
  char* _middle_high;
  char* _upper_high;

  char* _lower_high_boundary;
  char* _middle_high_boundary;
  char* _upper_high_boundary;

  size_t _lower_alignment;
  size_t _middle_alignment;
  size_t _upper_alignment;

  // MPSS Accessors
  char* lower_high() const { return _lower_high; }
  char* middle_high() const { return _middle_high; }
  char* upper_high() const { return _upper_high; }

  char* lower_high_boundary() const { return _lower_high_boundary; }
  char* middle_high_boundary() const { return _middle_high_boundary; }
  char* upper_high_boundary() const { return _upper_high_boundary; }

  size_t lower_alignment() const { return _lower_alignment; }
  size_t middle_alignment() const { return _middle_alignment; }
  size_t upper_alignment() const { return _upper_alignment; }

 public:
  // Committed area
  char* low()  const { return _low; }
  char* high() const { return _high; }

  // Reserved area
  char* low_boundary()  const { return _low_boundary; }
  char* high_boundary() const { return _high_boundary; }

  bool special() const { return _special; }

 public:
  // Initialization
  VirtualSpace();
  // 按指定粒度和最大承诺大小初始化空间
  bool initialize_with_granularity(ReservedSpace rs, size_t committed_byte_size, size_t max_commit_ganularity);
  // 简化版初始化（无粒度参数）
  bool initialize(ReservedSpace rs, size_t committed_byte_size);

  // Destruction
  ~VirtualSpace();

  // Reserved memory 预留内存总大小
  size_t reserved_size() const;
  // Actually committed OS memory 实际已提交物理内存大小
  size_t actual_committed_size() const;
  // Memory used/expanded in this virtual space 当前已使用内存大小
  size_t committed_size() const;
  // Memory left to use/expand in this virtual space 剩余可提交内存大小
  size_t uncommitted_size() const;

  bool   contains(const void* p) const; // 检查指针是否在管理范围内

  // Operations
  // returns true on success, false otherwise
  // 动态扩展已提交内存（可选预触达 pre_touch）
  bool expand_by(size_t bytes, bool pre_touch = false);
  // 缩减已提交内存
  void shrink_by(size_t bytes);
  // 释放所有资源
  void release();

  void check_for_contiguity() PRODUCT_RETURN;

  // Debugging
  void print_on(outputStream* out) const PRODUCT_RETURN;
  void print() const;

  void print_space_boundaries_on(outputStream* out) const;
};

#endif // SHARE_MEMORY_VIRTUALSPACE_HPP
