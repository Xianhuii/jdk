/*
 * Copyright (c) 2014, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SHARE_MEMORY_METASPACECHUNKFREELISTSUMMARY_HPP
#define SHARE_MEMORY_METASPACECHUNKFREELISTSUMMARY_HPP

#include "utilities/globalDefinitions.hpp"

// Todo: will need to rework this, see JDK-8251342
// MetaspaceChunkFreeListSummary 用于统计元空间（Metaspace）中空闲内存块的分类汇总信息，帮助管理元空间内存分配与回收策略。
// 元空间是JVM存储类元数据（如类定义、常量池等）的区域，替代了早期版本的永久代（PermGen）
class MetaspaceChunkFreeListSummary {
  size_t _num_specialized_chunks; // 特殊尺寸空闲块的数量
  size_t _num_small_chunks; // 小尺寸空闲块的数量
  size_t _num_medium_chunks; // 中等尺寸空闲块的数量
  size_t _num_humongous_chunks; // 巨型尺寸空闲块的数量

  size_t _specialized_chunks_size_in_bytes;
  size_t _small_chunks_size_in_bytes;
  size_t _medium_chunks_size_in_bytes;
  size_t _humongous_chunks_size_in_bytes;

 public:
  MetaspaceChunkFreeListSummary() :
    _num_specialized_chunks(0),
    _num_small_chunks(0),
    _num_medium_chunks(0),
    _num_humongous_chunks(0),
    _specialized_chunks_size_in_bytes(0),
    _small_chunks_size_in_bytes(0),
    _medium_chunks_size_in_bytes(0),
    _humongous_chunks_size_in_bytes(0)
  {}

  MetaspaceChunkFreeListSummary(size_t num_specialized_chunks,
                                size_t num_small_chunks,
                                size_t num_medium_chunks,
                                size_t num_humongous_chunks,
                                size_t specialized_chunks_size_in_bytes,
                                size_t small_chunks_size_in_bytes,
                                size_t medium_chunks_size_in_bytes,
                                size_t humongous_chunks_size_in_bytes) :
    _num_specialized_chunks(num_specialized_chunks),
    _num_small_chunks(num_small_chunks),
    _num_medium_chunks(num_medium_chunks),
    _num_humongous_chunks(num_humongous_chunks),
    _specialized_chunks_size_in_bytes(specialized_chunks_size_in_bytes),
    _small_chunks_size_in_bytes(small_chunks_size_in_bytes),
    _medium_chunks_size_in_bytes(medium_chunks_size_in_bytes),
    _humongous_chunks_size_in_bytes(humongous_chunks_size_in_bytes)
  {}

  size_t num_specialized_chunks() const {
    return _num_specialized_chunks;
  }

  size_t num_small_chunks() const {
    return _num_small_chunks;
  }

  size_t num_medium_chunks() const {
    return _num_medium_chunks;
  }

  size_t num_humongous_chunks() const {
    return _num_humongous_chunks;
  }

  size_t specialized_chunks_size_in_bytes() const {
    return _specialized_chunks_size_in_bytes;
  }

  size_t small_chunks_size_in_bytes() const {
    return _small_chunks_size_in_bytes;
  }

  size_t medium_chunks_size_in_bytes() const {
    return _medium_chunks_size_in_bytes;
  }

  size_t humongous_chunks_size_in_bytes() const {
    return _humongous_chunks_size_in_bytes;
  }
};

#endif // SHARE_MEMORY_METASPACECHUNKFREELISTSUMMARY_HPP
