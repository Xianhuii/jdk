/*
 * Copyright (c) 2001, 2025, Oracle and/or its affiliates. All rights reserved.
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

#include "gc/serial/cardTableRS.hpp"
#include "gc/serial/serialGcRefProcProxyTask.hpp"
#include "gc/serial/serialHeap.inline.hpp"
#include "gc/serial/serialStringDedup.inline.hpp"
#include "gc/serial/tenuredGeneration.hpp"
#include "gc/shared/adaptiveSizePolicy.hpp"
#include "gc/shared/ageTable.inline.hpp"
#include "gc/shared/collectorCounters.hpp"
#include "gc/shared/continuationGCSupport.inline.hpp"
#include "gc/shared/gcArguments.hpp"
#include "gc/shared/gcHeapSummary.hpp"
#include "gc/shared/gcLocker.hpp"
#include "gc/shared/gcPolicyCounters.hpp"
#include "gc/shared/gcTimer.hpp"
#include "gc/shared/gcTrace.hpp"
#include "gc/shared/gcTraceTime.inline.hpp"
#include "gc/shared/referencePolicy.hpp"
#include "gc/shared/referenceProcessorPhaseTimes.hpp"
#include "gc/shared/space.hpp"
#include "gc/shared/spaceDecorator.hpp"
#include "gc/shared/strongRootsScope.hpp"
#include "gc/shared/weakProcessor.hpp"
#include "logging/log.hpp"
#include "memory/iterator.inline.hpp"
#include "memory/reservedSpace.hpp"
#include "memory/resourceArea.hpp"
#include "oops/instanceRefKlass.hpp"
#include "oops/oop.inline.hpp"
#include "runtime/java.hpp"
#include "runtime/javaThread.hpp"
#include "runtime/prefetch.inline.hpp"
#include "runtime/threads.hpp"
#include "utilities/align.hpp"
#include "utilities/copy.hpp"
#include "utilities/globalDefinitions.hpp"
#include "utilities/stack.inline.hpp"

/*
 * 晋升失败闭包
 */
class PromoteFailureClosure : public InHeapScanClosure {
  /*
   * 处理对象
   * @param p 对象指针
   */
  template <typename T>
  void do_oop_work(T* p) {
    assert(is_in_young_gen(p), "promote-fail objs must be in young-gen");
    assert(!SerialHeap::heap()->young_gen()->to()->is_in_reserved(p), "must not be in to-space");

    // 尝试复制对象到survivor空间
    try_scavenge(p, [] (auto) {});
  }
public:
  PromoteFailureClosure(DefNewGeneration* g) : InHeapScanClosure(g) {}

  void do_oop(oop* p)       { do_oop_work(p); }
  void do_oop(narrowOop* p) { do_oop_work(p); }
};

/*
 * 根扫描闭包
 */
class RootScanClosure : public OffHeapScanClosure {
  template <typename T>
  void do_oop_work(T* p) {
    assert(!SerialHeap::heap()->is_in_reserved(p), "outside the heap");
    // 尝试复制对象到survivor空间
    try_scavenge(p,  [] (auto) {});
  }
public:
  RootScanClosure(DefNewGeneration* g) : OffHeapScanClosure(g) {}

  void do_oop(oop* p)       { do_oop_work(p); }
  void do_oop(narrowOop* p) { do_oop_work(p); }
};

/*
 * CLD 扫描闭包
 */
class CLDScanClosure: public CLDClosure {

  class CLDOopClosure : public OffHeapScanClosure {
    ClassLoaderData* _scanned_cld;

    template <typename T>
    void do_oop_work(T* p) {
      assert(!SerialHeap::heap()->is_in_reserved(p), "outside the heap");

      try_scavenge(p, [&] (oop new_obj) {
        assert(_scanned_cld != nullptr, "inv");
        if (is_in_young_gen(new_obj) && !_scanned_cld->has_modified_oops()) { // 新对象在新生代，且CLD未被修改
          _scanned_cld->record_modified_oops(); // 记录CLD已被修改
        }
      });
    }

  public:
    CLDOopClosure(DefNewGeneration* g) : OffHeapScanClosure(g),
      _scanned_cld(nullptr) {}

    void set_scanned_cld(ClassLoaderData* cld) {
      assert(cld == nullptr || _scanned_cld == nullptr, "Must be");
      _scanned_cld = cld;
    }

    void do_oop(oop* p)       { do_oop_work(p); }
    void do_oop(narrowOop* p) { ShouldNotReachHere(); }
  };

  CLDOopClosure _oop_closure;
 public:
  CLDScanClosure(DefNewGeneration* g) : _oop_closure(g) {}

  /*
   * 处理CLD
   * @param cld CLD指针
   */
  void do_cld(ClassLoaderData* cld) {
    // If the cld has not been dirtied we know that there's
    // no references into  the young gen and we can skip it.
    if (cld->has_modified_oops()) { // CLD已被修改

      // Tell the closure which CLD is being scanned so that it can be dirtied
      // if oops are left pointing into the young gen.
      _oop_closure.set_scanned_cld(cld); // 记录CLD已被修改

      // Clean the cld since we're going to scavenge all the metadata.
      cld->oops_do(&_oop_closure, ClassLoaderData::_claim_none, /*clear_modified_oops*/true); // 遍历CLD中的对象，尝试复制到survivor空间

      _oop_closure.set_scanned_cld(nullptr); // 重置CLD指针
    }
  }
};

/*
 * 存活检查闭包
 */
class IsAliveClosure: public BoolObjectClosure {
  HeapWord*         _young_gen_end;
public:
  IsAliveClosure(DefNewGeneration* g): _young_gen_end(g->reserved().end()) {}

  /*
   * 检查对象是否存活
   * @param p 对象指针
   * @return true 如果对象存活，false 否则
   */
  bool do_object_b(oop p) {
    return cast_from_oop<HeapWord*>(p) >= _young_gen_end || p->is_forwarded();
  }
};

/*
 * 弱根调整闭包
 */
class AdjustWeakRootClosure: public OffHeapScanClosure {
  template <class T>
  void do_oop_work(T* p) {
    DEBUG_ONLY(SerialHeap* heap = SerialHeap::heap();)
    assert(!heap->is_in_reserved(p), "outside the heap");

    oop obj = RawAccess<IS_NOT_NULL>::oop_load(p); // 加载对象指针
    if (is_in_young_gen(obj)) { // 如果对象在新生代
      assert(!heap->young_gen()->to()->is_in_reserved(obj), "inv");
      assert(obj->is_forwarded(), "forwarded before weak-root-processing");
      oop new_obj = obj->forwardee(); // 获取转发后的对象
      RawAccess<IS_NOT_NULL>::oop_store(p, new_obj); // 存储转发后的对象指针
    }
  }
 public:
  AdjustWeakRootClosure(DefNewGeneration* g): OffHeapScanClosure(g) {}

  void do_oop(oop* p)       { do_oop_work(p); }
  void do_oop(narrowOop* p) { ShouldNotReachHere(); }
};

/*
 * 存活对象闭包
 */
class KeepAliveClosure: public OopClosure {
  DefNewGeneration* _young_gen;
  HeapWord*         _young_gen_end;
  CardTableRS* _rs;

  bool is_in_young_gen(void* p) const {
    return p < _young_gen_end;
  }

  template <class T>
  void do_oop_work(T* p) {
    oop obj = RawAccess<IS_NOT_NULL>::oop_load(p);

    if (is_in_young_gen(obj)) { // 如果对象在新生代
      oop new_obj = obj->is_forwarded() ? obj->forwardee() // 如果对象已转发，获取转发后的对象
                                        : _young_gen->copy_to_survivor_space(obj); // 否则，复制到幸存者空间
      RawAccess<IS_NOT_NULL>::oop_store(p, new_obj);

      if (is_in_young_gen(new_obj) && !is_in_young_gen(p)) {
        _rs->inline_write_ref_field_gc(p); // 如果新对象在新生代且旧对象不在新生代，内联写入引用字段  
      }
    }
  }
public:
  KeepAliveClosure(DefNewGeneration* g) :
    _young_gen(g),
    _young_gen_end(g->reserved().end()),
    _rs(SerialHeap::heap()->rem_set()) {}

  void do_oop(oop* p)       { do_oop_work(p); }
  void do_oop(narrowOop* p) { do_oop_work(p); }
};

/*
 * 快速疏散 follower 闭包
 */
class FastEvacuateFollowersClosure: public VoidClosure {
  SerialHeap* _heap;
  YoungGenScanClosure* _young_cl;
  OldGenScanClosure* _old_cl;
public:
  FastEvacuateFollowersClosure(SerialHeap* heap,
                               YoungGenScanClosure* young_cl,
                               OldGenScanClosure* old_cl) :
    _heap(heap), _young_cl(young_cl), _old_cl(old_cl)
  {}

  void do_void() {
    _heap->scan_evacuated_objs(_young_cl, _old_cl); // 扫描已疏散的对象
  }
};

/*
 * 新代构造函数
 * @param rs 保留空间
 * @param initial_size 初始大小
 * @param min_size 最小大小
 * @param max_size 最大大小
 * @param policy 策略
 */
DefNewGeneration::DefNewGeneration(ReservedSpace rs,
                                   size_t initial_size,
                                   size_t min_size,
                                   size_t max_size,
                                   const char* policy)
  : Generation(rs, initial_size),
    _promotion_failed(false),
    _promo_failure_drain_in_progress(false),
    _string_dedup_requests()
{
  MemRegion cmr((HeapWord*)_virtual_space.low(),
                (HeapWord*)_virtual_space.high());
  SerialHeap* gch = SerialHeap::heap();

  gch->rem_set()->resize_covered_region(cmr);

  _eden_space = new ContiguousSpace(); // 创建eden区
  _from_space = new ContiguousSpace(); // 创建from_space
  _to_space   = new ContiguousSpace(); // 创建to_space

  // Compute the maximum eden and survivor space sizes. These sizes
  // are computed assuming the entire reserved space is committed.
  // These values are exported as performance counters.
  // 计算最大的eden区和survivor区大小
  uintx size = _virtual_space.reserved_size();
  _max_survivor_size = compute_survivor_size(size, SpaceAlignment);
  _max_eden_size = size - (2*_max_survivor_size);

  // allocate the performance counters

  // Generation counters -- generation 0, 3 subspaces
  // 新代性能计数器
  _gen_counters = new GenerationCounters("new", 0, 3,
      min_size, max_size, _virtual_space.committed_size());
  _gc_counters = new CollectorCounters(policy, 0);
  // 新代eden区性能计数器
  _eden_counters = new CSpaceCounters("eden", 0, _max_eden_size, _eden_space,
                                      _gen_counters);
  // 新代from区性能计数器
  _from_counters = new CSpaceCounters("s0", 1, _max_survivor_size, _from_space,
                                      _gen_counters);
  // 新代to区性能计数器
  _to_counters = new CSpaceCounters("s1", 2, _max_survivor_size, _to_space,
                                    _gen_counters);

  // 初始化新代空间边界
  compute_space_boundaries(0, SpaceDecorator::Clear, SpaceDecorator::Mangle);
  update_counters();
  _old_gen = nullptr;
  _tenuring_threshold = MaxTenuringThreshold;
  _pretenure_size_threshold_words = PretenureSizeThreshold >> LogHeapWordSize;

  _ref_processor = nullptr;

  _gc_timer = new STWGCTimer();

  _gc_tracer = new DefNewTracer();
}

/*
 * 初始化新代空间边界
 * @param minimum_eden_size 最小eden区大小
 * @param clear_space 是否清空空间
 * @param mangle_space 是否混淆空间
 */
void DefNewGeneration::compute_space_boundaries(uintx minimum_eden_size,
                                                bool clear_space,
                                                bool mangle_space) {
  // If the spaces are being cleared (only done at heap initialization
  // currently), the survivor spaces need not be empty.
  // Otherwise, no care is taken for used areas in the survivor spaces
  // so check.
  assert(clear_space || (to()->is_empty() && from()->is_empty()),
    "Initialization of the survivor spaces assumes these are empty");

  // Compute sizes
  uintx size = _virtual_space.committed_size();
  uintx survivor_size = compute_survivor_size(size, SpaceAlignment); // 计算最大的survivor区大小
  uintx eden_size = size - (2*survivor_size); // 计算最大的eden区大小=总大小-2*survivor大小
  if (eden_size > max_eden_size()) {
    // Need to reduce eden_size to satisfy the max constraint. The delta needs
    // to be 2*SpaceAlignment aligned so that both survivors are properly
    // aligned.
    uintx eden_delta = align_up(eden_size - max_eden_size(), 2*SpaceAlignment);
    eden_size     -= eden_delta;
    survivor_size += eden_delta/2;
  }
  assert(eden_size > 0 && survivor_size <= eden_size, "just checking");

  if (eden_size < minimum_eden_size) {
    // May happen due to 64Kb rounding, if so adjust eden size back up
    minimum_eden_size = align_up(minimum_eden_size, SpaceAlignment);
    uintx maximum_survivor_size = (size - minimum_eden_size) / 2;
    uintx unaligned_survivor_size =
      align_down(maximum_survivor_size, SpaceAlignment);
    survivor_size = MAX2(unaligned_survivor_size, SpaceAlignment);
    eden_size = size - (2*survivor_size);
    assert(eden_size > 0 && survivor_size <= eden_size, "just checking");
    assert(eden_size >= minimum_eden_size, "just checking");
  }

  char *eden_start = _virtual_space.low();
  char *from_start = eden_start + eden_size;
  char *to_start   = from_start + survivor_size;
  char *to_end     = to_start   + survivor_size;

  assert(to_end == _virtual_space.high(), "just checking");
  assert(is_aligned(eden_start, SpaceAlignment), "checking alignment");
  assert(is_aligned(from_start, SpaceAlignment), "checking alignment");
  assert(is_aligned(to_start, SpaceAlignment),   "checking alignment");

  MemRegion edenMR((HeapWord*)eden_start, (HeapWord*)from_start);
  MemRegion fromMR((HeapWord*)from_start, (HeapWord*)to_start);
  MemRegion toMR  ((HeapWord*)to_start, (HeapWord*)to_end);

  // A minimum eden size implies that there is a part of eden that
  // is being used and that affects the initialization of any
  // newly formed eden.
  bool live_in_eden = minimum_eden_size > 0;

  // Reset the spaces for their new regions.
  // 初始化新代eden区
  eden()->initialize(edenMR,
                     clear_space && !live_in_eden,
                     SpaceDecorator::Mangle);
  // If clear_space and live_in_eden, we will not have cleared any
  // portion of eden above its top. This can cause newly
  // expanded space not to be mangled if using ZapUnusedHeapArea.
  // We explicitly do such mangling here.
  if (ZapUnusedHeapArea && clear_space && live_in_eden && mangle_space) {
    eden()->mangle_unused_area();
  }
  // 初始化新代from区
  from()->initialize(fromMR, clear_space, mangle_space);
  // 初始化新代to区
  to()->initialize(toMR, clear_space, mangle_space);
}

/*
 * 交换新代from区和to区
 */
void DefNewGeneration::swap_spaces() {
  ContiguousSpace* s = from();
  _from_space        = to();
  _to_space          = s;

  if (UsePerfData) {
    CSpaceCounters* c = _from_counters;
    _from_counters = _to_counters;
    _to_counters = c;
  }
}

/*
 * 扩展新代空间
 * @param bytes 扩展大小
 * @return 是否扩展成功
 */
bool DefNewGeneration::expand(size_t bytes) {
  HeapWord* prev_high = (HeapWord*) _virtual_space.high();
  bool success = _virtual_space.expand_by(bytes);
  if (success && ZapUnusedHeapArea) {
    // Mangle newly committed space immediately because it
    // can be done here more simply that after the new
    // spaces have been computed.
    HeapWord* new_high = (HeapWord*) _virtual_space.high();
    MemRegion mangle_region(prev_high, new_high);
    SpaceMangler::mangle_region(mangle_region);
  }

  return success;
}

/*
 * 计算新代空间增加大小
 * @param threads_count 线程数量
 * @return 新代空间增加大小
 */
size_t DefNewGeneration::calculate_thread_increase_size(int threads_count) const {
    size_t thread_increase_size = 0;
    // Check an overflow at 'threads_count * NewSizeThreadIncrease'.
    if (threads_count > 0 && NewSizeThreadIncrease <= max_uintx / threads_count) {
      thread_increase_size = threads_count * NewSizeThreadIncrease;
    }
    return thread_increase_size;
}

/*
 * 调整新代空间大小
 * @param new_size_candidate 新代空间大小候选值
 * @param new_size_before 新代空间大小之前值
 * @param alignment 对齐大小
 * @param thread_increase_size 线程增加大小
 * @return 新代空间大小
 */
size_t DefNewGeneration::adjust_for_thread_increase(size_t new_size_candidate,
                                                    size_t new_size_before,
                                                    size_t alignment,
                                                    size_t thread_increase_size) const {
  size_t desired_new_size = new_size_before;

  if (NewSizeThreadIncrease > 0 && thread_increase_size > 0) {

    // 1. Check an overflow at 'new_size_candidate + thread_increase_size'.
    if (new_size_candidate <= max_uintx - thread_increase_size) {
      new_size_candidate += thread_increase_size;

      // 2. Check an overflow at 'align_up'.
      size_t aligned_max = ((max_uintx - alignment) & ~(alignment-1));
      if (new_size_candidate <= aligned_max) {
        desired_new_size = align_up(new_size_candidate, alignment);
      }
    }
  }

  return desired_new_size;
}

/*
 * 计算新代空间大小
 */
void DefNewGeneration::compute_new_size() {
  // This is called after a GC that includes the old generation, so from-space
  // will normally be empty.
  // Note that we check both spaces, since if scavenge failed they revert roles.
  // If not we bail out (otherwise we would have to relocate the objects).
  if (!from()->is_empty() || !to()->is_empty()) {
    return;
  }

  SerialHeap* gch = SerialHeap::heap();

  size_t old_size = gch->old_gen()->capacity();
  size_t new_size_before = _virtual_space.committed_size();
  size_t min_new_size = NewSize;
  size_t max_new_size = reserved().byte_size();
  assert(min_new_size <= new_size_before &&
         new_size_before <= max_new_size,
         "just checking");
  // All space sizes must be multiples of Generation::GenGrain.
  size_t alignment = Generation::GenGrain;

  int threads_count = Threads::number_of_non_daemon_threads();
  size_t thread_increase_size = calculate_thread_increase_size(threads_count);

  size_t new_size_candidate = old_size / NewRatio;
  // Compute desired new generation size based on NewRatio and NewSizeThreadIncrease
  // and reverts to previous value if any overflow happens
  size_t desired_new_size = adjust_for_thread_increase(new_size_candidate, new_size_before,
                                                       alignment, thread_increase_size);

  // Adjust new generation size
  desired_new_size = clamp(desired_new_size, min_new_size, max_new_size);
  assert(desired_new_size <= max_new_size, "just checking");

  bool changed = false;
  if (desired_new_size > new_size_before) {
    size_t change = desired_new_size - new_size_before;
    assert(change % alignment == 0, "just checking");
    if (expand(change)) {
       changed = true;
    }
    // If the heap failed to expand to the desired size,
    // "changed" will be false.  If the expansion failed
    // (and at this point it was expected to succeed),
    // ignore the failure (leaving "changed" as false).
  }
  if (desired_new_size < new_size_before && eden()->is_empty()) {
    // bail out of shrinking if objects in eden
    size_t change = new_size_before - desired_new_size;
    assert(change % alignment == 0, "just checking");
    _virtual_space.shrink_by(change);
    changed = true;
  }
  if (changed) {
    // The spaces have already been mangled at this point but
    // may not have been cleared (set top = bottom) and should be.
    // Mangling was done when the heap was being expanded.
    compute_space_boundaries(eden()->used(),
                             SpaceDecorator::Clear,
                             SpaceDecorator::DontMangle);
    MemRegion cmr((HeapWord*)_virtual_space.low(),
                  (HeapWord*)_virtual_space.high());
    gch->rem_set()->resize_covered_region(cmr);

    log_debug(gc, ergo, heap)(
        "New generation size %zuK->%zuK [eden=%zuK,survivor=%zuK]",
        new_size_before/K, _virtual_space.committed_size()/K,
        eden()->capacity()/K, from()->capacity()/K);
    log_trace(gc, ergo, heap)(
        "  [allowed %zuK extra for %d threads]",
          thread_increase_size/K, threads_count);
      }
}

/*
 * 初始化新代引用处理器
 */
void DefNewGeneration::ref_processor_init() {
  assert(_ref_processor == nullptr, "a reference processor already exists");
  assert(!_reserved.is_empty(), "empty generation?");
  _span_based_discoverer.set_span(_reserved);
  _ref_processor = new ReferenceProcessor(&_span_based_discoverer);    // a vanilla reference processor
}

size_t DefNewGeneration::capacity() const {
  return eden()->capacity()
       + from()->capacity();  // to() is only used during scavenge
}


size_t DefNewGeneration::used() const {
  return eden()->used()
       + from()->used();      // to() is only used during scavenge
}


size_t DefNewGeneration::free() const {
  return eden()->free()
       + from()->free();      // to() is only used during scavenge
}

size_t DefNewGeneration::max_capacity() const {
  const size_t reserved_bytes = reserved().byte_size();
  return reserved_bytes - compute_survivor_size(reserved_bytes, SpaceAlignment);
}

bool DefNewGeneration::is_in(const void* p) const {
  return eden()->is_in(p)
      || from()->is_in(p)
      || to()  ->is_in(p);
}

size_t DefNewGeneration::unsafe_max_alloc_nogc() const {
  return eden()->free();
}

size_t DefNewGeneration::capacity_before_gc() const {
  return eden()->capacity();
}

/*
 * 遍历新代对象
 */
void DefNewGeneration::object_iterate(ObjectClosure* blk) {
  eden()->object_iterate(blk);
  from()->object_iterate(blk);
}

// If "p" is in the space, returns the address of the start of the
// "block" that contains "p".  We say "block" instead of "object" since
// some heaps may not pack objects densely; a chunk may either be an
// object or a non-object.  If "p" is not in the space, return null.
// Very general, slow implementation.
/*
 * 计算新代对象块起始地址
 * @param cs 连续空间
 * @param p 对象地址
 * @return 对象块起始地址
 */
static HeapWord* block_start_const(const ContiguousSpace* cs, const void* p) {
  assert(MemRegion(cs->bottom(), cs->end()).contains(p),
         "p (" PTR_FORMAT ") not in space [" PTR_FORMAT ", " PTR_FORMAT ")",
         p2i(p), p2i(cs->bottom()), p2i(cs->end()));
  if (p >= cs->top()) {
    return cs->top();
  } else {
    HeapWord* last = cs->bottom();
    HeapWord* cur = last;
    while (cur <= p) {
      last = cur;
      cur += cast_to_oop(cur)->size();
    }
    assert(oopDesc::is_oop(cast_to_oop(last)), PTR_FORMAT " should be an object start", p2i(last));
    return last;
  }
}

/*
 * 计算新代对象块起始地址
 * @param p 对象地址
 * @return 对象块起始地址
 */
HeapWord* DefNewGeneration::block_start(const void* p) const {
  if (eden()->is_in_reserved(p)) {
    return block_start_const(eden(), p);
  }
  if (from()->is_in_reserved(p)) {
    return block_start_const(from(), p);
  }
  assert(to()->is_in_reserved(p), "inv");
  return block_start_const(to(), p);
}

/*
 * 调整新代期望晋升阈值：根据to空间容量计算期望晋升年龄
 */
void DefNewGeneration::adjust_desired_tenuring_threshold() {
  // Set the desired survivor size to half the real survivor space
  size_t const survivor_capacity = to()->capacity() / HeapWordSize;
  size_t const desired_survivor_size = (size_t)((((double)survivor_capacity) * TargetSurvivorRatio) / 100);

  _tenuring_threshold = age_table()->compute_tenuring_threshold(desired_survivor_size);

  if (UsePerfData) {
    GCPolicyCounters* gc_counters = SerialHeap::heap()->counters();
    gc_counters->tenuring_threshold()->set_value(_tenuring_threshold);
    gc_counters->desired_survivor_size()->set_value(desired_survivor_size * oopSize);
  }

  age_table()->print_age_table();
}

/*
 * 执行新代垃圾回收
 * @param clear_all_soft_refs 是否清除所有软引用
 * @return 是否成功
 */
bool DefNewGeneration::collect(bool clear_all_soft_refs) {
  SerialHeap* heap = SerialHeap::heap(); // 获取堆实例

  assert(to()->is_empty(), "Else not collection_attempt_is_safe");
  _gc_timer->register_gc_start(); // 注册垃圾回收开始时间
  _gc_tracer->report_gc_start(heap->gc_cause(), _gc_timer->gc_start()); // 报告垃圾回收开始时间
  _ref_processor->start_discovery(clear_all_soft_refs); // 开始引用发现

  _old_gen = heap->old_gen(); // 获取老年代实例

  init_assuming_no_promotion_failure(); // 初始化假设没有晋升失败

  GCTraceTime(Trace, gc, phases) tm("DefNew", nullptr, heap->gc_cause()); // 跟踪新代垃圾回收

  heap->trace_heap_before_gc(_gc_tracer); // 报告垃圾回收前堆状态

  // These can be shared for all code paths
  IsAliveClosure is_alive(this); // 存活对象闭包

  age_table()->clear(); // 清除年龄表
  to()->clear(SpaceDecorator::Mangle); // 清除to空间

  YoungGenScanClosure young_gen_cl(this); // 新代扫描闭包
  OldGenScanClosure   old_gen_cl(this); // 老代扫描闭包

  FastEvacuateFollowersClosure evacuate_followers(heap,
                                                  &young_gen_cl,
                                                  &old_gen_cl); // 快速晋升闭包

  // 扫描根节点
  {
    StrongRootsScope srs(0); // 强根节点范围
    RootScanClosure root_cl{this}; // 根节点扫描闭包
    CLDScanClosure cld_cl{this}; // 类加载器数据扫描闭包

    MarkingNMethodClosure code_cl(&root_cl,
                                  NMethodToOopClosure::FixRelocations,
                                  false /* keepalive_nmethods */); // 代码缓存扫描闭包

    HeapWord* saved_top_in_old_gen = _old_gen->space()->top(); // 保存老年代顶部地址
    // 扫描root节点
    heap->process_roots(SerialHeap::SO_ScavengeCodeCache,
                        &root_cl,
                        &cld_cl,
                        &cld_cl,
                        &code_cl);
    // 扫描卡表中老年代对年轻代的引用
    _old_gen->scan_old_to_young_refs(saved_top_in_old_gen);
  }

  // "evacuate followers".
  evacuate_followers.do_void(); // 快速晋升

  // 处理引用
  {
    // Reference processing
    KeepAliveClosure keep_alive(this); // 保持存活闭包
    ReferenceProcessor* rp = ref_processor(); // 引用处理器
    ReferenceProcessorPhaseTimes pt(_gc_timer, rp->max_num_queues()); // 引用处理器阶段时间
    SerialGCRefProcProxyTask task(is_alive, keep_alive, evacuate_followers); // 引用处理器代理任务
  
    const ReferenceProcessorStats& stats = rp->process_discovered_references(task, nullptr, pt); // 处理已发现引用
    _gc_tracer->report_gc_reference_stats(stats); // 报告引用处理器统计信息
    _gc_tracer->report_tenuring_threshold(tenuring_threshold()); // 报告期望晋升年龄
    pt.print_all_references(); // 打印所有引用
  }

  // 处理弱引用
  {
    AdjustWeakRootClosure cl{this}; // 调整弱根节点闭包
    WeakProcessor::weak_oops_do(&is_alive, &cl); // 处理弱引用
  }

  _string_dedup_requests.flush(); // 刷新字符串去重请求

  if (!_promotion_failed) { // 如果没有晋升失败
    // Swap the survivor spaces.
    eden()->clear(SpaceDecorator::Mangle); // 清除eden空间
    from()->clear(SpaceDecorator::Mangle); // 清除from空间
    swap_spaces(); // 交换eden和from空间

    assert(to()->is_empty(), "to space should be empty now");

    adjust_desired_tenuring_threshold(); // 调整新代期望晋升阈值
  } else { // 如果有晋升失败
    assert(_promo_failure_scan_stack.is_empty(), "post condition");
    _promo_failure_scan_stack.clear(true); // Clear cached segments. 清除晋升失败扫描栈缓存

    remove_forwarding_pointers(); // 清除转发指针
    log_info(gc, promotion)("Promotion failed");

    _gc_tracer->report_promotion_failed(_promotion_failed_info); // 报告晋升失败信息

    // Reset the PromotionFailureALot counters.
    NOT_PRODUCT(heap->reset_promotion_should_fail();)
  }

  heap->trace_heap_after_gc(_gc_tracer); // 报告垃圾回收后堆状态

  _gc_timer->register_gc_end(); // 注册垃圾回收结束时间

  _gc_tracer->report_gc_end(_gc_timer->gc_end(), _gc_timer->time_partitions()); // 报告垃圾回收结束时间

  return !_promotion_failed; // 返回是否成功
}

/*
 * 初始化假设没有晋升失败
 */
void DefNewGeneration::init_assuming_no_promotion_failure() {
  _promotion_failed = false;
  _promotion_failed_info.reset();
}

/*
 * 清除转发指针
 */
void DefNewGeneration::remove_forwarding_pointers() {
  assert(_promotion_failed, "precondition");

  // Will enter Full GC soon due to failed promotion. Must reset the mark word
  // of objs in young-gen so that no objs are marked (forwarded) when Full GC
  // starts. (The mark word is overloaded: `is_marked()` == `is_forwarded()`.)
  struct ResetForwardedMarkWord : ObjectClosure {
    void do_object(oop obj) override {
      if (obj->is_self_forwarded()) { // 如果是自己转发
        obj->unset_self_forwarded(); // 清除自己转发标记
      } else if (obj->is_forwarded()) { // 如果是转发
        // To restore the klass-bits in the header.
        // Needed for object iteration to work properly.
        obj->set_mark(obj->forwardee()->prototype_mark()); // 设置标记单词为转发对象的原型标记
      }
    }
  } cl; // 重置转发标记单词闭包
  eden()->object_iterate(&cl); // 遍历eden空间
  from()->object_iterate(&cl); // 遍历from空间
}

/*
 * 处理晋升失败
 * @param old 失败的对象
 */
void DefNewGeneration::handle_promotion_failure(oop old) {
  log_debug(gc, promotion)("Promotion failure size = %zu) ", old->size());

  _promotion_failed = true; // 标记为晋升失败
  _promotion_failed_info.register_copy_failure(old->size()); // 注册复制失败

  ContinuationGCSupport::transform_stack_chunk(old); // 转换栈块

  // forward to self
  old->forward_to_self(); // 转发到自己

  _promo_failure_scan_stack.push(old); // 压入晋升失败扫描栈

  if (!_promo_failure_drain_in_progress) { // 如果没有进行晋升失败扫描
    // prevent recursion in copy_to_survivor_space()
    _promo_failure_drain_in_progress = true; // 标记为正在进行晋升失败扫描
    drain_promo_failure_scan_stack(); // 扫描晋升失败栈
    _promo_failure_drain_in_progress = false; // 标记为已完成晋升失败扫描
  }
}

/*
 * 复制到幸存者空间
 *  1. 尝试在to空间分配对象
 *  2. 如果to空间分配失败，尝试在老年代分配对象
 *  3. 如果老年代分配失败，处理晋升失败，直接返回原对象
 *  4. 如果对象没有晋升到老年代，增加对象年龄，将对象添加到年龄表格
 *  6. 设置old的forward指针指向新对象
 *  7. 字符串去重请求列表处理
 *  8. 返回新对象
 * @param old 要复制的对象
 * @return 复制后的对象
 */
oop DefNewGeneration::copy_to_survivor_space(oop old) {
  assert(is_in_reserved(old) && !old->is_forwarded(),
         "shouldn't be scavenging this oop");
  size_t s = old->size(); // 获取对象大小
  oop obj = nullptr;

  // Try allocating obj in to-space (unless too old)
  if (old->age() < tenuring_threshold()) { // 如果对象年龄小于期望晋升年龄
    obj = cast_to_oop(to()->allocate(s)); // 在to空间分配对象
  }

  bool new_obj_is_tenured = false;
  // Otherwise try allocating obj tenured
  if (obj == nullptr) { // 如果在to空间分配失败
    obj = _old_gen->allocate_for_promotion(old, s); // 在老年代分配对象
    if (obj == nullptr) { // 如果在老年代分配失败
      handle_promotion_failure(old); // 处理晋升失败
      return old;
    }

    new_obj_is_tenured = true; // 记录对象是否晋升到老年代
  }

  // Prefetch beyond obj
  const intx interval = PrefetchCopyIntervalInBytes; // 预取间隔
  Prefetch::write(obj, interval); // 预取对象

  // Copy obj 拷贝对象数据
  Copy::aligned_disjoint_words(cast_from_oop<HeapWord*>(old), cast_from_oop<HeapWord*>(obj), s);

  ContinuationGCSupport::transform_stack_chunk(obj); // 转换栈块

  if (!new_obj_is_tenured) { // 如果对象没有晋升到老年代
    // Increment age if obj still in new generation
    obj->incr_age(); // 增加对象年龄
    age_table()->add(obj, s); // 增加年龄表格
  }

  // Done, insert forward pointer to obj in this header
  old->forward_to(obj); // 转发到新对象

  if (SerialStringDedup::is_candidate_from_evacuation(obj, new_obj_is_tenured)) { // 如果是从疏散复制的字符串去重候选
    // Record old; request adds a new weak reference, which reference
    // processing expects to refer to a from-space object.
    _string_dedup_requests.add(old); // 添加到字符串去重请求列表
  }
  return obj; // 返回新对象
}

/*
 * 处理晋升失败栈
 */
void DefNewGeneration::drain_promo_failure_scan_stack() {
  PromoteFailureClosure cl{this}; // 处理晋升失败闭包
  while (!_promo_failure_scan_stack.is_empty()) { // 遍历晋升失败栈
     oop obj = _promo_failure_scan_stack.pop(); // 弹出对象
     obj->oop_iterate(&cl); // 遍历对象的引用
  }
}

/*
 * 贡献划痕空间
 * @param scratch 划痕空间指针
 * @param num_words 划痕空间大小
 */
void DefNewGeneration::contribute_scratch(void*& scratch, size_t& num_words) {
  if (_promotion_failed) { // 如果晋升失败，直接返回
    return;
  }

  const size_t MinFreeScratchWords = 100;

  ContiguousSpace* to_space = to(); // 获取to空间
  const size_t free_words = pointer_delta(to_space->end(), to_space->top()); // 计算to空间空闲大小
  if (free_words >= MinFreeScratchWords) { // 如果空闲大小足够
    scratch = to_space->top(); // 分配划痕空间
    num_words = free_words; // 分配大小
  }
}

/*
 * 重置划痕空间
 */
void DefNewGeneration::reset_scratch() {
  // If contributing scratch in to_space, mangle all of
  // to_space if ZapUnusedHeapArea.  This is needed because
  // top is not maintained while using to-space as scratch.
  if (ZapUnusedHeapArea) {
    to()->mangle_unused_area();
  }
}

/*
 * 垃圾收集结束
 * @param full 是否是全垃圾收集
 */
void DefNewGeneration::gc_epilogue(bool full) {
  assert(!GCLocker::is_active(), "We should not be executing here");
  // update the generation and space performance counters
  update_counters(); // 更新性能计数器
}

/*
 * 更新性能计数器
 */
void DefNewGeneration::update_counters() {
  if (UsePerfData) {
    _eden_counters->update_all();
    _from_counters->update_all();
    _to_counters->update_all();
    _gen_counters->update_capacity(_virtual_space.committed_size());
  }
}

/*
 * 验证空间
 */
void DefNewGeneration::verify() {
  eden()->verify();
  from()->verify();
    to()->verify();
}

/*
 * 打印空间信息
 */
void DefNewGeneration::print_on(outputStream* st) const {
  st->print("%-10s", name());

  st->print(" total %zuK, used %zuK ", capacity() / K, used() / K);
  _virtual_space.print_space_boundaries_on(st);

  StreamIndentor si(st, 1);
  eden()->print_on(st, "eden ");
  from()->print_on(st, "from ");
  to()->print_on(st, "to   ");
}

/*
 * 分配内存
 * @param word_size 分配大小
 * @return 分配内存指针
 */
HeapWord* DefNewGeneration::allocate(size_t word_size) {
  // This is the slow-path allocation for the DefNewGeneration.
  // Most allocations are fast-path in compiled code.
  // We try to allocate from the eden.  If that works, we are happy.
  // Note that since DefNewGeneration supports lock-free allocation, we
  // have to use it here, as well.
  HeapWord* result = eden()->par_allocate(word_size); // 尝试在eden区分配内存
  return result;
}

/*
 * 分配内存（Lock-free）
 * @param word_size 分配大小
 * @return 分配内存指针
 */
HeapWord* DefNewGeneration::par_allocate(size_t word_size) {
  return eden()->par_allocate(word_size); // 尝试在eden区分配内存
}

/*
 * 获取TLAB容量
 * @return TLAB容量
 */
size_t DefNewGeneration::tlab_capacity() const {
  return eden()->capacity();
}

/*
 * 获取TLAB使用量
 * @return TLAB使用量
 */
size_t DefNewGeneration::tlab_used() const {
  return eden()->used();
}

/*
 * 获取最大TLAB分配量
 * @return 最大TLAB分配量
 */
size_t DefNewGeneration::unsafe_max_tlab_alloc() const {
  return unsafe_max_alloc_nogc();
}
