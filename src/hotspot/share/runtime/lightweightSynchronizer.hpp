/*
 * Copyright (c) 2024, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SHARE_RUNTIME_LIGHTWEIGHTSYNCHRONIZER_HPP
#define SHARE_RUNTIME_LIGHTWEIGHTSYNCHRONIZER_HPP

#include "memory/allStatic.hpp"
#include "runtime/javaThread.hpp"
#include "runtime/objectMonitor.hpp"
#include "runtime/synchronizer.hpp"

class ObjectMonitorTable;

// 定义了 JVM 中轻量级同步机制的核心逻辑，负责管理 Java 对象锁的快速进入、膨胀、收缩及并发控制。
// 高效处理锁竞争，平衡轻量级锁（偏向锁/自旋锁）与重量级锁（ObjectMonitor）的使用，以优化多线程场景下的性能。

// 提供锁的快速路径（fast_lock_*）、膨胀路径（inflate_*）、退出路径（exit）等核心逻辑
class LightweightSynchronizer : AllStatic {
 private:
  static ObjectMonitor* get_or_insert_monitor_from_table(oop object, JavaThread* current, bool* inserted);
  static ObjectMonitor* get_or_insert_monitor(oop object, JavaThread* current, ObjectSynchronizer::InflateCause cause);

  static ObjectMonitor* add_monitor(JavaThread* current, ObjectMonitor* monitor, oop obj);
  static bool remove_monitor(Thread* current, ObjectMonitor* monitor, oop obj);

  static void deflate_mark_word(oop object);

  // 确保线程的锁栈有足够空间记录锁状态，防止栈溢出
  static void ensure_lock_stack_space(JavaThread* current);

  // 可能用于缓存热点对象的锁状态，加速后续锁操作
  class CacheSetter;
  // 处理锁竞争时的膨胀逻辑，将锁升级为重量级锁
  class LockStackInflateContendedLocks;
  // 验证线程状态，确保锁操作的合法性（如未阻塞时才能释放锁）
  class VerifyThreadState;

 public:
  static void initialize();

  static bool needs_resize();
  // 动态调整 ObjectMonitorTable大小，应对高并发场景下的扩容需求
  static bool resize_table(JavaThread* current);

 private:
  // 尝试通过自旋或 CAS 操作快速获取锁，避免阻塞
  static inline bool fast_lock_try_enter(oop obj, LockStack& lock_stack, JavaThread* current);
  static bool fast_lock_spin_enter(oop obj, LockStack& lock_stack, JavaThread* current, bool observed_deflation);

 public:
  // 尝试快速进入同步块
  // 若成功（无竞争），直接通过 LockStack记录锁状态
  // 若失败（存在竞争），调用 inflate_into_object_header膨胀为重量级锁（ObjectMonitor）
  static void enter_for(Handle obj, BasicLock* lock, JavaThread* locking_thread);
  static void enter(Handle obj, BasicLock* lock, JavaThread* current);
  // 释放锁并清理线程的 LockStack
  static void exit(oop object, BasicLock* lock, JavaThread* current);

  // 将对象头的轻量级锁升级为重量级锁（ObjectMonitor），处理锁竞争
  static ObjectMonitor* inflate_into_object_header(oop object, ObjectSynchronizer::InflateCause cause, JavaThread* locking_thread, Thread* current);
  static ObjectMonitor* inflate_locked_or_imse(oop object, ObjectSynchronizer::InflateCause cause, TRAPS);
  static ObjectMonitor* inflate_fast_locked_object(oop object, ObjectSynchronizer::InflateCause cause, JavaThread* locking_thread, JavaThread* current);
  static ObjectMonitor* inflate_and_enter(oop object, BasicLock* lock, ObjectSynchronizer::InflateCause cause, JavaThread* locking_thread, JavaThread* current);

  // 收缩膨胀的锁（如无竞争时降级回轻量级锁）
  static void deflate_monitor(Thread* current, oop obj, ObjectMonitor* monitor);

  // 从全局 ObjectMonitorTable获取或创建与对象关联的监视器
  static ObjectMonitor* get_monitor_from_table(Thread* current, oop obj);

  static bool contains_monitor(Thread* current, ObjectMonitor* monitor);

  static bool quick_enter(oop obj, BasicLock* Lock, JavaThread* current);
};

#endif // SHARE_RUNTIME_LIGHTWEIGHTSYNCHRONIZER_HPP
