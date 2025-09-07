/*
 * Copyright (c) 2020, 2024, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SHARE_MEMORY_METASPACE_METASPACEARENA_HPP
#define SHARE_MEMORY_METASPACE_METASPACEARENA_HPP

#include "memory/allocation.hpp" // 内存分配基础
#include "memory/metaspace.hpp" // 元空间核心定义
#include "memory/metaspace/counters.hpp" // 统计计数器
#include "memory/metaspace/metablock.hpp" // 内存块管理
#include "memory/metaspace/metachunkList.hpp" // 块列表管理

class outputStream;
class Mutex;

namespace metaspace {

class ArenaGrowthPolicy; // 定义内存池的增长策略（如按需扩展、固定步长）
struct ArenaStats; // 统计内存使用情况（已用、提交、容量等）
class ChunkManager; // 管理元空间的物理内存块（Chunk）分配与回收
class FreeBlocks; // 维护已释放但未合并的空闲内存块，减少碎片
class Metachunk; // 表示元空间的物理内存块，包含已分配和可用空间
class MetaspaceContext; // 关联类加载器的上下文信息


// The MetaspaceArena is a growable metaspace memory pool belonging to a CLD;
//  internally it consists of a list of metaspace chunks, of which the head chunk
//  is the current chunk from which we allocate via pointer bump.
//
//  +---------------+
//  |     Arena     |
//  +---------------+
//            |
//            | _chunks                                               commit top
//            |                                                       v
//        +----------+      +----------+      +----------+      +----------+
//        | retired  | ---> | retired  | ---> | retired  | ---> | current  |
//        | chunk    |      | chunk    |      | chunk    |      | chunk    |
//        +----------+      +----------+      +----------+      +----------+
//                                                                  ^
//                                                                  used top
//
//        +------------+
//        | FreeBlocks | --> O -> O -> O -> O
//        +------------+
//
//

// When the current chunk is used up, MetaspaceArena requests a new chunk from
//  the associated ChunkManager.
//
// MetaspaceArena also keeps a FreeBlocks structure to manage memory blocks which
//  had been deallocated prematurely.
//
// MetaspaceArena是 JVM 元空间（Metaspace）的内存池实现，负责为类加载器（ClassLoader）动态分配和管理类元数据（如类定义、方法等）。
// 其设计目标是高效、可扩展，并支持多线程环境下的安全操作。
class MetaspaceArena : public CHeapObj<mtClass> {
  friend class MetaspaceArenaTestFriend;

  // Please note that access to a metaspace arena may be shared
  // between threads and needs to be synchronized in CLMS.

  // Allocation alignment specific to this arena
  // 内存分配的对齐粒度（以字为单位）
  const size_t _allocation_alignment_words;

  // Reference to the chunk manager to allocate chunks from.
  // 负责分配新内存块的 ChunkManager引用
  ChunkManager* const _chunk_manager;

  // Reference to the growth policy to use.
  // 控制内存池扩展策略的对象
  const ArenaGrowthPolicy* const _growth_policy;

  // List of chunks. Head of the list is the current chunk.
  // 双向链表，存储当前 Arena 的所有内存块（头部为当前活动块）
  MetachunkList _chunks;

  // Structure to take care of leftover/deallocated space in used chunks.
  // Owned by the Arena. Gets allocated on demand only.
  // FreeBlocks结构，管理提前释放的内存块
  FreeBlocks* _fbl;

  // 获取当前活动内存块
  Metachunk* current_chunk()              { return _chunks.first(); }
  const Metachunk* current_chunk() const  { return _chunks.first(); }

  // Reference to an outside counter to keep track of used space.
  // 原子计数器，跟踪 Arena 已使用的总字数
  SizeAtomicCounter* const _total_used_words_counter;

  // A name for purely debugging/logging purposes.
  // 调试标识符
  const char* const _name;

  ChunkManager* chunk_manager() const           { return _chunk_manager; }

  // free block list
  FreeBlocks* fbl() const                       { return _fbl; }
  void add_allocation_to_fbl(MetaBlock bl);

  // Given a chunk, return the committed remainder of this chunk.
  // 回收废弃块中的剩余空间
  MetaBlock salvage_chunk(Metachunk* c);

  // Allocate a new chunk from the underlying chunk manager able to hold at least
  // requested word size.
  Metachunk* allocate_new_chunk(size_t requested_word_size);

  // Returns the level of the next chunk to be added, acc to growth policy.
  // 根据策略确定下一个内存块的级别（大小）
  chunklevel_t next_chunk_level() const;

  // Attempt to enlarge the current chunk to make it large enough to hold at least
  //  requested_word_size additional words.
  //
  // On success, true is returned, false otherwise.
  bool attempt_enlarge_current_chunk(size_t requested_word_size);

  // Allocate from the arena proper, once dictionary allocations and fencing are sorted out.
  // 实际执行内存分配逻辑，处理对齐和分片
  MetaBlock allocate_inner(size_t word_size, MetaBlock& wastage);

public:
  // 初始化 Arena，绑定到指定的 ChunkManager和增长策略
  MetaspaceArena(MetaspaceContext* context,
                 const ArenaGrowthPolicy* growth_policy,
                 size_t allocation_alignment_words,
                 const char* name);
  // 释放资源，清理内存块
  ~MetaspaceArena();

  size_t allocation_alignment_words() const { return _allocation_alignment_words; }
  size_t allocation_alignment_bytes() const { return allocation_alignment_words() * BytesPerWord; }

  // Allocate memory from Metaspace.
  // On success, returns non-empty block of the specified word size, and
  // possibly a wastage block that is the result of alignment operations.
  // On failure, returns an empty block. Failure may happen if we hit a
  // commit limit.
  // 尝试从当前块分配内存，失败时请求新块或扩展当前块
  MetaBlock allocate(size_t word_size, MetaBlock& wastage);

  // Prematurely returns a metaspace allocation to the _block_freelists because it is not
  // needed anymore.
  // 将内存块归还给 FreeBlocks，供后续复用
  void deallocate(MetaBlock bl);

  // Update statistics. This walks all in-use chunks.
  // 将 Arena 的统计信息汇总到 ArenaStats
  void add_to_statistics(ArenaStats* out) const;

  // Convenience method to get the most important usage statistics.
  // For deeper analysis use add_to_statistics().
  // 快速获取已用、提交、容量字数
  void usage_numbers(size_t* p_used_words, size_t* p_committed_words, size_t* p_capacity_words) const;

  // 验证内存结构的完整性（调试用途）
  DEBUG_ONLY(void verify() const;)
  DEBUG_ONLY(void verify_allocation_guards() const;)

  void print_on(outputStream* st) const;

  // Returns true if the given block is contained in this arena
  DEBUG_ONLY(bool contains(MetaBlock bl) const;)
};

} // namespace metaspace

#endif // SHARE_MEMORY_METASPACE_METASPACEARENA_HPP

