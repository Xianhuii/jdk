/*
 * Copyright (c) 2001, 2024, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SHARE_GC_SERIAL_DEFNEWGENERATION_HPP
#define SHARE_GC_SERIAL_DEFNEWGENERATION_HPP

#include "gc/serial/cSpaceCounters.hpp"
#include "gc/serial/generation.hpp"
#include "gc/serial/tenuredGeneration.hpp"
#include "gc/shared/ageTable.hpp"
#include "gc/shared/copyFailedInfo.hpp"
#include "gc/shared/gc_globals.hpp"
#include "gc/shared/generationCounters.hpp"
#include "gc/shared/stringdedup/stringDedup.hpp"
#include "gc/shared/tlab_globals.hpp"
#include "utilities/align.hpp"
#include "utilities/stack.hpp"

class ContiguousSpace;
class CSpaceCounters;
class OldGenScanClosure;
class YoungGenScanClosure;
class DefNewTracer;
class SerialHeap;
class STWGCTimer;

// DefNewGeneration is a young generation containing eden, from- and
// to-space.
// HotSpot VM 中 Serial GC 的年轻代实现。它负责管理年轻代内存，包括 Eden 区和两个 Survivor 区（From 和 To）。其主要作用是：
// 1. 分配新对象：当新对象需要分配时，会先尝试在 Eden 区分配。如果 Eden 区空间不足，会触发一次 Minor GC。
// 2. 垃圾回收 (Minor GC)：在 Minor GC 中，存活的对象会从 Eden 区复制到 Survivor 区（From 或 To）。
// 3. 对象晋升：每个对象在 Survivor 区中存在的时间称为年龄。当对象在 Survivor 区中存在的时间超过阈值（默认是 15 次），
//    它会被提升到老年代。
// 4. 内存管理：动态调整 Eden 区和 Survivor 区的大小以适应应用行为。
class DefNewGeneration: public Generation {
  friend class VMStructs;

  // 老年代的指针，用于对象晋升
  TenuredGeneration* _old_gen;

  // 年龄阈值，用于对象晋升
  uint        _tenuring_threshold;   // Tenuring threshold for next collection.
  // 年龄表，用于记录对象的年龄
  AgeTable    _age_table;
  // Size of object to pretenure in words; command line provides bytes
  // 对象直接晋升到老年代的大小阈值（以字为单位）。
  size_t      _pretenure_size_threshold_words;

  // ("Weak") Reference processing support
  SpanSubjectToDiscoveryClosure _span_based_discoverer;
  ReferenceProcessor* _ref_processor; // 引用处理器，用于处理弱引用、软引用等

  AgeTable*   age_table() { return &_age_table; }

  // Initialize state to optimistically assume no promotion failure will
  // happen.
  void   init_assuming_no_promotion_failure();
  // True iff a promotion has failed in the current collection.
  // 标记当前GC中是否发生了晋升失败
  bool   _promotion_failed;
  bool   promotion_failed() { return _promotion_failed; }
  // 记录晋升失败的信息
  PromotionFailedInfo _promotion_failed_info;

  // Handling promotion failure.  A young generation collection
  // can fail if a live object cannot be copied out of its
  // location in eden or from-space during the collection.  If
  // a collection fails, the young generation is left in a
  // consistent state such that it can be collected by a
  // full collection.
  //   Before the collection
  //     Objects are in eden or from-space
  //     All roots into the young generation point into eden or from-space.
  //
  //   After a failed collection
  //     Objects may be in eden, from-space, or to-space
  //     An object A in eden or from-space may have a copy B
  //       in to-space.  If B exists, all roots that once pointed
  //       to A must now point to B.
  //     All objects in the young generation are unmarked.
  //     Eden, from-space, and to-space will all be collected by
  //       the full collection.
  void handle_promotion_failure(oop);

  // In the absence of promotion failure, we wouldn't look at "from-space"
  // objects after a young-gen collection.  When promotion fails, however,
  // the subsequent full collection will look at from-space objects:
  // therefore we must remove their forwarding pointers.
  void remove_forwarding_pointers();

  Stack<oop, mtGC> _promo_failure_scan_stack;
  void drain_promo_failure_scan_stack(void);
  bool _promo_failure_drain_in_progress;

  // Performance Counters 性能计数器
  GenerationCounters*  _gen_counters; // 年轻代性能计数器
  CSpaceCounters*      _eden_counters; // Eden 区性能计数器
  CSpaceCounters*      _from_counters; // From 区性能计数器
  CSpaceCounters*      _to_counters; // To 区性能计数器

  // sizing information
  size_t               _max_eden_size;
  size_t               _max_survivor_size;

  // Tenuring
  void adjust_desired_tenuring_threshold();

  // Spaces 内存空间
  ContiguousSpace* _eden_space; // Eden 区
  ContiguousSpace* _from_space; // From 区
  ContiguousSpace* _to_space; // To 区

  STWGCTimer* _gc_timer;

  DefNewTracer* _gc_tracer;

  StringDedup::Requests _string_dedup_requests;

  // Return the size of a survivor space if this generation were of size
  // gen_size.
  size_t compute_survivor_size(size_t gen_size, size_t alignment) const {
    size_t n = gen_size / (SurvivorRatio + 2);
    return n > alignment ? align_down(n, alignment) : alignment;
  }

 public:
  /*
   * 构造函数
   * @param rs 保留的内存空间
   * @param initial_byte_size 初始大小
   * @param min_byte_size 最小大小
   * @param max_byte_size 最大大小
   * @param policy 收集策略
   */
  DefNewGeneration(ReservedSpace rs,
                   size_t initial_byte_size,
                   size_t min_byte_size,
                   size_t max_byte_size,
                   const char* policy="Serial young collection pauses");

  // allocate and initialize ("weak") refs processing support
  // 初始化引用处理器
  void ref_processor_init();
  ReferenceProcessor* ref_processor() { return _ref_processor; }

  // Accessing spaces
  ContiguousSpace* eden() const           { return _eden_space; }
  ContiguousSpace* from() const           { return _from_space; }
  ContiguousSpace* to()   const           { return _to_space;   }

  // Space enquiries
  size_t capacity() const;
  size_t used() const;
  size_t free() const;
  size_t max_capacity() const;
  size_t capacity_before_gc() const;

  // Returns "TRUE" iff "p" points into the used areas in each space of young-gen.
  // 检查指针是否指向年轻代的已使用区域
  bool is_in(const void* p) const;

  // Return an estimate of the maximum allocation that could be performed
  // in the generation without triggering any collection or expansion
  // activity.  It is "unsafe" because no locks are taken; the result
  // should be treated as an approximation, not a guarantee, for use in
  // heuristic resizing decisions.
  size_t unsafe_max_alloc_nogc() const;

  size_t max_eden_size() const              { return _max_eden_size; }
  size_t max_survivor_size() const          { return _max_survivor_size; }

  // Thread-local allocation buffers
  size_t tlab_capacity() const;
  size_t tlab_used() const;
  size_t unsafe_max_tlab_alloc() const;

  // Grow the generation by the specified number of bytes.
  // The size of bytes is assumed to be properly aligned.
  // Return true if the expansion was successful.
  // 扩容
  bool expand(size_t bytes);


  // Iteration
  /*
   * 遍历对象
   * @param blk 对象闭包
   */
  void object_iterate(ObjectClosure* blk);

  HeapWord* block_start(const void* p) const;

  // Allocation support
  /*
   * 检查是否应该分配对象
   * @param word_size 对象大小
   * @param is_tlab 是否使用 TLAB
   * @return 如果应该分配对象，则返回 true；否则返回 false
   */
  bool should_allocate(size_t word_size, bool is_tlab) {
    assert(UseTLAB || !is_tlab, "Should not allocate tlab");
    assert(word_size != 0, "precondition");

    size_t overflow_limit    = (size_t)1 << (BitsPerSize_t - LogHeapWordSize);

    const bool overflows     = word_size >= overflow_limit;
    const bool check_too_big = _pretenure_size_threshold_words > 0;
    const bool not_too_big   = word_size < _pretenure_size_threshold_words;
    const bool size_ok       = is_tlab || !check_too_big || not_too_big;

    bool result = !overflows &&
                  size_ok;

    return result;
  }

  // Allocate requested size or return null; single-threaded and lock-free versions.
  /*
   * 分配对象
   * @param word_size 对象大小
   * @return 分配的对象指针
   */
  HeapWord* allocate(size_t word_size);
  /*
   * 并行分配对象
   * @param word_size 对象大小
   * @return 分配的对象指针
   */
  HeapWord* par_allocate(size_t word_size);

  /*
   * 垃圾收集结束处理
   * @param full 是否是完整的垃圾收集
   */
  void gc_epilogue(bool full);

  // For Old collection (part of running Full GC), the DefNewGeneration can
  // contribute the free part of "to-space" as the scratch space.
  /*
   * 贡献 To 区的空闲空间
   * @param scratch 空闲空间指针
   * @param num_words 空闲空间大小
   */
  void contribute_scratch(void*& scratch, size_t& num_words);

  // Reset for contribution of "to-space".
  /*
   * 重置 To 区的空闲空间
   */
  void reset_scratch();

  // GC support
  /*
   * 计算新的大小
   */
  void compute_new_size();

  /*
   * 收集对象
   * @param clear_all_soft_refs 是否清除所有软引用
   * @return 如果收集成功，则返回 true；否则返回 false
   */
  bool collect(bool clear_all_soft_refs);

  /*
   * 复制对象到 Survivor 区
   * @param old 对象指针
   * @return 复制后的对象指针
   */
  oop copy_to_survivor_space(oop old);
  uint tenuring_threshold() { return _tenuring_threshold; }

  // Performance Counter support
  void update_counters();

  // Printing
  const char* name() const { return "DefNew"; }

  void print_on(outputStream* st) const;

  void verify();

  bool promo_failure_scan_is_complete() const {
    return _promo_failure_scan_stack.is_empty();
  }

  DefNewTracer* gc_tracer() const { return _gc_tracer; }

 protected:
  // If clear_space is true, clear the survivor spaces.  Eden is
  // cleared if the minimum size of eden is 0.  If mangle_space
  // is true, also mangle the space in debug mode.
  /*
   * 计算空间边界
   * @param minimum_eden_size 最小 Eden 大小
   * @param clear_space 是否清除空间
   * @param mangle_space 是否损坏空间
   */
  void compute_space_boundaries(uintx minimum_eden_size,
                                bool clear_space,
                                bool mangle_space);

  // Return adjusted new size for NewSizeThreadIncrease.
  // If any overflow happens, revert to previous new size.
  /*
   * 调整新大小以适应线程增加
   * @param new_size_candidate 新大小候选值
   * @param new_size_before 之前的新大小
   * @param alignment 对齐大小
   * @param thread_increase_size 线程增加大小
   * @return 调整后的新大小
   */
  size_t adjust_for_thread_increase(size_t new_size_candidate,
                                    size_t new_size_before,
                                    size_t alignment,
                                    size_t thread_increase_size) const;

  size_t calculate_thread_increase_size(int threads_count) const;


  // Scavenge support
  /*
   * 交换 From 和 To 区的角色。在每次 Minor GC 成功后都会执行。
   */
  void swap_spaces();
};

#endif // SHARE_GC_SERIAL_DEFNEWGENERATION_HPP
