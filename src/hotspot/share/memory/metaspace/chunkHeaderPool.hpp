/*
 * Copyright (c) 2020, 2023, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2020 SAP SE. All rights reserved.
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

#ifndef SHARE_MEMORY_METASPACE_CHUNKHEADERPOOL_HPP
#define SHARE_MEMORY_METASPACE_CHUNKHEADERPOOL_HPP

#include "memory/allocation.hpp"
#include "memory/metaspace/counters.hpp"
#include "memory/metaspace/metachunk.hpp"
#include "memory/metaspace/metachunkList.hpp"
#include "utilities/debug.hpp"
#include "utilities/globalDefinitions.hpp"

namespace metaspace {
// JVM（Java虚拟机）中元空间（Metaspace）内存管理的关键组件，实现了Metachunk头部对象的高效内存池化分配机制。
// 核心目标是优化频繁创建/销毁Metachunk头部对象时的性能，减少动态内存分配的开销。
// Chunk headers (Metachunk objects) are separate entities from their payload.
//  Since they are allocated and released frequently in the course of buddy allocation
//  (splitting, merging chunks happens often) we want allocation of them fast. Therefore
//  we keep them in a simple pool (somewhat like a primitive slab allocator).

// 管理Metachunk头部对象的内存池
// 继承自CHeapObj<mtMetaspace>，表示对象在元空间堆（Metaspace Heap）上分配
class ChunkHeaderPool : public CHeapObj<mtMetaspace> {

  static const int SlabCapacity = 128; // 每个Slab可容纳的Metachunk数量

  // 内存池的基本分配单元，批量预分配Metachunk对象
  struct Slab : public CHeapObj<mtMetaspace> {
    Slab* _next; // 指向下一个Slab的指针
    int _top; // 当前Slab中已分配的Metachunk索引
    Metachunk _elems [SlabCapacity]; // 存储Metachunk对象的数组
    Slab() : _next(nullptr), _top(0) {
      for (int i = 0; i < SlabCapacity; i++) {
        _elems[i].clear();
      }
    }
  };

  IntCounter _num_slabs; // 已分配的Slab数量
  Slab* _first_slab; // 指向第一个Slab的指针
  Slab* _current_slab; // 当前正在使用的Slab

  IntCounter _num_handed_out; // 已分配但未归还的Metachunk数量

  MetachunkList _freelist; // 空闲Metachunk链表

  void allocate_new_slab();

  static ChunkHeaderPool* _chunkHeaderPool; // 全局唯一的ChunkHeaderPool实例

public:

  ChunkHeaderPool();

  ~ChunkHeaderPool();

  // Allocates a Metachunk structure. The structure is uninitialized.
  // 内存分配
  Metachunk* allocate_chunk_header() {
    DEBUG_ONLY(verify());

    Metachunk* c = nullptr;
    c = _freelist.remove_first(); // 优先从空闲链表获取
    assert(c == nullptr || c->is_dead(), "Not a freelist chunk header?");
    // 按需分配新Slab
    if (c == nullptr) {
      if (_current_slab == nullptr ||
          _current_slab->_top == SlabCapacity) {
        allocate_new_slab();
        assert(_current_slab->_top < SlabCapacity, "Sanity");
      }
      c = _current_slab->_elems + _current_slab->_top;
      _current_slab->_top++;
    }
    _num_handed_out.increment();
    // By contract, the returned structure is uninitialized.
    // Zap to make this clear.
    DEBUG_ONLY(c->zap_header(0xBB);)

    return c;
  }

  // 内存回收
  void return_chunk_header(Metachunk* c) {
    // We only ever should return free chunks, since returning chunks
    // happens only on merging and merging only works with free chunks.
    assert(c != nullptr && c->is_free(), "Sanity");
#ifdef ASSERT
    // In debug, fill dead header with pattern.
    c->zap_header(0xCC);
    c->set_next(nullptr);
    c->set_prev(nullptr);
#endif
    c->set_dead();
    _freelist.add(c);
    _num_handed_out.decrement();
  }

  // Returns number of allocated elements.
  int used() const                   { return _num_handed_out.get(); }

  // Returns number of elements in free list.
  int freelist_size() const          { return _freelist.count(); }

  // Returns size of memory used.
  size_t memory_footprint_words() const;

  DEBUG_ONLY(void verify() const;)

  static void initialize();

  // Returns reference to the one global chunk header pool.
  static ChunkHeaderPool* pool() { return _chunkHeaderPool; }

};

} // namespace metaspace

#endif // SHARE_MEMORY_METASPACE_CHUNKHEADERPOOL_HPP
