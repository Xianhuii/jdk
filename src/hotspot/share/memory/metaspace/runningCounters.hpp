/*
 * Copyright (c) 2020, 2024, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2020, 2022 SAP SE. All rights reserved.
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

#ifndef SHARE_MEMORY_METASPACE_RUNNINGCOUNTERS_HPP
#define SHARE_MEMORY_METASPACE_RUNNINGCOUNTERS_HPP

#include "memory/allStatic.hpp"

namespace metaspace {
// 提供了元空间内存使用的实时统计功能
// This class is a convenience interface for accessing global metaspace counters.
struct RunningCounters : public AllStatic {

  // ---- virtual memory -----

  // Return reserved size, in words, for Metaspace
  // 元空间已向OS申请的总内存容量（单位：单词数，通常为8字节）
  static size_t reserved_words();
  static size_t reserved_words_class();
  static size_t reserved_words_nonclass();

  // Return total committed size, in words, for Metaspace
  // 实际分配给元空间的物理内存/交换空间容量
  static size_t committed_words();
  static size_t committed_words_class();
  static size_t committed_words_nonclass();

  // ---- used chunks -----

  // Returns size, in words, used for metadata.
  // 当前被类元数据占用的内存量
  static size_t used_words();
  static size_t used_words_class();
  static size_t used_words_nonclass();

  // ---- free chunks -----

  // Returns size, in words, of all chunks in all freelists.
  // 可用内存块总容量（尚未分配给具体对象的内存）
  static size_t free_chunks_words();
  static size_t free_chunks_words_class();
  static size_t free_chunks_words_nonclass();

};

} // namespace metaspace

#endif // SHARE_MEMORY_METASPACE_RUNNINGCOUNTERS_HPP
