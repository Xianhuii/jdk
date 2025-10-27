/*
 * Copyright (c) 2018, 2025, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2018, Google and/or its affiliates. All rights reserved.
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

#ifndef SHARE_RUNTIME_THREADHEAPSAMPLER_HPP
#define SHARE_RUNTIME_THREADHEAPSAMPLER_HPP

#include "memory/allocation.hpp"
#include "oops/oopsHierarchy.hpp"
// 通过周期性采样线程的堆内存分配活动，收集内存使用数据（如分配速率、TLAB 使用率等），为 JVM 的内存管理优化（如 GC 策略调整、内存泄漏检测）提供依据。
class ThreadHeapSampler {
 private:
  // Amount of bytes to allocate before taking the next sample
  // 下次触发采样的字节数阈值。当线程分配内存超过此值时，触发采样逻辑。
  size_t _sample_threshold;

  // The TLAB top address when the last sampling happened, or
  // TLAB start if a new TLAB is allocated
  // 上次采样时 TLAB（Thread-Local Allocation Buffer）的顶部地址。若为新分配的 TLAB，则指向其起始地址。
  HeapWord* _tlab_top_at_sample_start;

  // The accumulated amount of allocated bytes in a TLAB since the last sampling
  // excluding the amount between _tlab_sample_start and top
  // 自上次采样以来，TLAB 内部累计分配的字节数（不含 _tlab_top_at_sample_start到当前 TLAB 顶部的增量）。
  size_t _accumulated_tlab_bytes_since_sample;

  // The accumulated amount of allocated bytes outside TLABs since last sample point
  // 自上次采样以来，TLAB 外部（如直接分配到老年代的大对象）累计分配的字节数。
  size_t _accumulated_outside_tlab_bytes_since_sample;

  // Cheap random number generator
  // 静态随机数种子，用于生成几何分布的采样间隔，降低采样频率波动。
  static uint64_t _rnd;

  // 静态采样间隔参数，控制采样触发的平均时间间隔。
  static volatile int _sampling_interval;

  // 基于几何分布计算下一个采样间隔，实现随机但平均可控的采样频率。
  void pick_next_geometric_sample();
  // 结合 _sampling_interval和随机数生成器 _rnd，确定具体的采样阈值。
  void pick_next_sample();

  static double fast_log2(const double& d);
  uint64_t next_random(uint64_t rnd);

  size_t current_tlab_bytes_since_last_sample(HeapWord* tlab_top)  const {
    // Both can be nullptr if there's not active TLAB, but otherwise
    // they both should be non-null.
    assert((tlab_top != nullptr) == (_tlab_top_at_sample_start != nullptr),
           "Both should either be uninitialized or initialized "
           "tlab_top: " PTR_FORMAT " _tlab_top_at_sample_start: " PTR_FORMAT,
           p2i(tlab_top), p2i(_tlab_top_at_sample_start));

    return pointer_delta(tlab_top, _tlab_top_at_sample_start, 1);
  }

  size_t tlab_bytes_since_last_sample(HeapWord* tlab_top) const {
    return _accumulated_tlab_bytes_since_sample + current_tlab_bytes_since_last_sample(tlab_top);
  }

  size_t outside_tlab_bytes_since_last_sample() const {
    return _accumulated_outside_tlab_bytes_since_sample;
  }

 public:
  ThreadHeapSampler() :
      _sample_threshold(0),
      _tlab_top_at_sample_start(nullptr),
      _accumulated_tlab_bytes_since_sample(0),
      _accumulated_outside_tlab_bytes_since_sample(0) {
    _rnd = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(this));
    if (_rnd == 0) {
      _rnd = 1;
    }

    // Call this after _rnd is initialized to initialize _sample_threshold.
    pick_next_sample();
  }

  size_t bytes_since_last_sample(HeapWord* tlab_top) const {
    return tlab_bytes_since_last_sample(tlab_top) +
           outside_tlab_bytes_since_last_sample();
  }

  size_t bytes_until_sample(HeapWord* tlab_top) const {
    const size_t since_last_sample = bytes_since_last_sample(tlab_top);
    return _sample_threshold - MIN2(since_last_sample, _sample_threshold);
  }

  bool should_sample(HeapWord* tlab_top) const {
    return bytes_until_sample(tlab_top) == 0;
  }

  void set_tlab_top_at_sample_start(HeapWord* tlab_top) {
    _tlab_top_at_sample_start = tlab_top;
  }

  void reset_after_sample(HeapWord* tlab_top) {
    _tlab_top_at_sample_start = tlab_top;
    _accumulated_tlab_bytes_since_sample = 0;
    _accumulated_outside_tlab_bytes_since_sample = 0;
  }

  void retire_tlab(HeapWord* tlab_top) {
    _accumulated_tlab_bytes_since_sample += current_tlab_bytes_since_last_sample(tlab_top);
    _tlab_top_at_sample_start = nullptr;
  }

  void inc_outside_tlab_bytes(size_t size) {
    _accumulated_outside_tlab_bytes_since_sample += size;
  }

  void log_sample_decision(HeapWord* tlab_top) PRODUCT_RETURN;

  // 核心采样方法，接收当前分配对象 obj和 TLAB 顶部地址 tlab_top，执行实际采样逻辑（如记录分配事件、更新统计信息）
  void sample(oop obj, HeapWord* tlab_top);

  static void set_sampling_interval(int sampling_interval);
  static int get_sampling_interval();
};

#endif // SHARE_RUNTIME_THREADHEAPSAMPLER_HPP
