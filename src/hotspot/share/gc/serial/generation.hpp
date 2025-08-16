/*
 * Copyright (c) 1997, 2024, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SHARE_GC_SERIAL_GENERATION_HPP
#define SHARE_GC_SERIAL_GENERATION_HPP

#include "gc/shared/collectorCounters.hpp"
#include "gc/shared/referenceProcessor.hpp"
#include "gc/shared/space.hpp"
#include "logging/log.hpp"
#include "memory/allocation.hpp"
#include "memory/memRegion.hpp"
#include "memory/virtualspace.hpp"
#include "runtime/mutex.hpp"
#include "runtime/perfData.hpp"
#include "runtime/prefetch.inline.hpp"

// A Generation models a heap area for similarly-aged objects.
// It will contain one ore more spaces holding the actual objects.
//
// The Generation class hierarchy:
//
// Generation                      - abstract base class
// - DefNewGeneration              - allocation area (copy collected)
// - TenuredGeneration             - tenured (old object) space (mark-compact)
//
// The system configuration currently allowed is:
//
//   DefNewGeneration + TenuredGeneration
//

class DefNewGeneration;
class GCMemoryManager;
class ContiguousSpace;
class OopClosure;
class ReservedSpace;

/*
 * Generation（代）是Serial GC中的核心抽象概念，代表Java堆中一块特定的内存区域，用于存放具有相似生命周期的对象。
 * 在Serial GC中，Generation是一个抽象基类，它有两个主要的子类实现：
 *  1. DefNewGeneration ：年轻代实现，负责新对象的分配和Minor GC，使用复制算法进行垃圾回收
 *  2. TenuredGeneration ：老年代实现，存储长期存活的对象，使用标记-整理算法进行垃圾回收
 * Generation模块的主要作用是：
 *  1. 不同年龄的对象提供独立的内存管理策略
 *  2. 定义代的基本属性和行为（如容量、已使用空间、空闲空间等）
 *  3. 为垃圾回收提供统一的抽象接口
 *  4. 管理内存的预留和提交
 */
class Generation: public CHeapObj<mtGC> {
  friend class VMStructs;
 private:
  GCMemoryManager* _gc_manager;

 protected:
  // Minimum and maximum addresses for memory reserved (not necessarily
  // committed) for generation.
  // Used by card marking code. Must not overlap with address ranges of
  // other generations.
  MemRegion _reserved; // 代表为该代预留的内存区域，定义了代的边界

  // Memory area reserved for generation
  VirtualSpace _virtual_space; // 代表为该代实际分配的内存区域，可能小于_reserved

  // Performance Counters
  CollectorCounters* _gc_counters; // 代的性能计数器，用于统计垃圾回收的相关指标

  // Initialize the generation.
  /*
   * 初始化代，为其预留内存空间和初始化性能计数器
   * @param rs 代的预留内存空间
   * @param initial_byte_size 代的初始容量
   */
  Generation(ReservedSpace rs, size_t initial_byte_size);

 public:
  /*
   * 代的公共常量定义
   */
  enum SomePublicConstants {
    // Generations are GenGrain-aligned and have size that are multiples of
    // GenGrain.
    // Note: on ARM we add 1 bit for card_table_base to be properly aligned
    // (we expect its low byte to be zero - see implementation of post_barrier)
    LogOfGenGrain = 16 ARM32_ONLY(+1), // 代的对齐单位，通常为16字节
    GenGrain = 1 << LogOfGenGrain, // 代的对齐单位，通常为16字节
  };

  virtual size_t capacity() const = 0;  // The maximum number of object bytes the
                                        // generation can currently hold.
  virtual size_t used() const = 0;      // The number of used bytes in the gen.
  virtual size_t free() const = 0;      // The number of free bytes in the gen.

  // Support for java.lang.Runtime.maxMemory(); see CollectedHeap.
  // Returns the total number of bytes  available in a generation
  // for the allocation of objects.
  virtual size_t max_capacity() const;

  MemRegion reserved() const { return _reserved; }

  /* Returns "TRUE" iff "p" points into the reserved area of the generation. */
  bool is_in_reserved(const void* p) const {
    return _reserved.contains(p);
  }

  virtual void verify() = 0;

public:
  // Performance Counter support
  CollectorCounters* counters() { return _gc_counters; }

  GCMemoryManager* gc_manager() const {
    assert(_gc_manager != nullptr, "not initialized yet");
    return _gc_manager;
  }

  void set_gc_manager(GCMemoryManager* gc_manager) {
    _gc_manager = gc_manager;
  }
};

#endif // SHARE_GC_SERIAL_GENERATION_HPP
