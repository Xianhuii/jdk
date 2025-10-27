/*
 * Copyright (c) 2015, 2022, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SHARE_RUNTIME_SEMAPHORE_HPP
#define SHARE_RUNTIME_SEMAPHORE_HPP

#include "memory/allocation.hpp"
#include "utilities/globalDefinitions.hpp"

#if defined(LINUX) || defined(AIX)
# include "semaphore_posix.hpp"
#else
# include OS_HEADER(semaphore)
#endif

class JavaThread;

// Implements the limited, platform independent Semaphore API.
// 用于实现跨平台信号量（Semaphore），主要功能是为JVM提供线程同步机制。
class Semaphore : public CHeapObj<mtSynchronizer> {
  SemaphoreImpl _impl;

  NONCOPYABLE(Semaphore);

 public:
  // 构造函数，初始化信号量计数值，默认为0。
  Semaphore(uint value = 0) : _impl(value) {}
  // 析构函数，释放资源。
  ~Semaphore() {}

  // 增加信号量计数，允许最多count个线程继续执行。
  void signal(uint count = 1) { _impl.signal(count); }

  // 阻塞当前线程，直到信号量计数大于0，然后减1。
  void wait()                 { _impl.wait(); }

  // 尝试立即获取信号量（非阻塞），成功返回true，失败返回false。
  bool trywait()              { return _impl.trywait(); }

  // 安全点检查下的等待，需传入JavaThread指针，用于JVM内部线程协调。
  void wait_with_safepoint_check(JavaThread* thread);
};

#endif // SHARE_RUNTIME_SEMAPHORE_HPP
