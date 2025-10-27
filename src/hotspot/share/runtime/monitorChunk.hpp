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

#ifndef SHARE_RUNTIME_MONITORCHUNK_HPP
#define SHARE_RUNTIME_MONITORCHUNK_HPP

#include "memory/allocation.hpp"

class BasicObjectLock;
class OopClosure;

// Data structure for holding monitors for one activation during
// deoptimization.
// MonitorChunk是 JVM 运行时用于存储去优化（deoptimization）期间激活方法（activation）的监视器（monitors）信息的数据结构。
// 主要用于恢复方法从编译执行回退到解释执行时的锁状态。
class MonitorChunk: public CHeapObj<mtSynchronizer> {
 private:
  // 记录当前存储的监视器数量
  int              _number_of_monitors;
  // 动态分配的 BasicObjectLock指针数组，每个元素对应一个对象锁的元数据（如锁状态、持有线程等）
  BasicObjectLock* _monitors;
  BasicObjectLock* monitors() const { return _monitors; }
 public:
  // Constructor
  MonitorChunk(int number_on_monitors);
  ~MonitorChunk();

  // Returns the number of monitors
  int number_of_monitors() const { return _number_of_monitors; }

  // Returns the index'th monitor
  BasicObjectLock* at(int index)            { assert(index >= 0 && index < number_of_monitors(), "out of bounds check"); return &monitors()[index]; }

  // Memory management
  void oops_do(OopClosure* f);

  // Tells whether the addr point into the monitors.
  bool contains(void* addr) const           { return (addr >= (void*) monitors()) && (addr <  (void*) (monitors() + number_of_monitors())); }
};

#endif // SHARE_RUNTIME_MONITORCHUNK_HPP
