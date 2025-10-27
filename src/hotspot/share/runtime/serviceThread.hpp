/*
 * Copyright (c) 2011, 2024, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SHARE_RUNTIME_SERVICETHREAD_HPP
#define SHARE_RUNTIME_SERVICETHREAD_HPP

#include "prims/jvmtiImpl.hpp"
#include "runtime/javaThread.hpp"

// A hidden from external view JavaThread for JVMTI compiled-method-load
// events, oop storage cleanup, and the maintenance of string, symbol,
// protection domain, and resolved method tables.
class JvmtiDeferredEvent;
// JVM内部的一个特殊线程类，继承自JavaThread，主要用于处理JVM底层服务任务。
// 它对外部不可见（通过is_hidden_from_external_view()标识），直接服务于JVM内部机制，
// 如JVMTI事件处理、垃圾回收（GC）支持及内部数据结构的维护。
class ServiceThread : public JavaThread {
  friend class VMStructs;
 private:
  DEBUG_ONLY(static JavaThread* _instance;)
  // 当前处理的JVMTI事件指针
  static JvmtiDeferredEvent* _jvmti_event;
  // 存储待处理的JVMTI事件
  static JvmtiDeferredEventQueue _jvmti_service_queue;

  // 启动服务线程并设置其执行入口
  static void service_thread_entry(JavaThread* thread, TRAPS);
  ServiceThread(ThreadFunction entry_point) : JavaThread(entry_point) {};

 public:
  static void initialize();

  // Hide this thread from external view.
  bool is_hidden_from_external_view() const      { return true; }
  bool is_service_thread() const                 { return true; }

  // Add event to the service thread event queue.
  // 将事件（如编译方法加载事件）加入队列，供服务线程异步处理
  static void enqueue_deferred_event(JvmtiDeferredEvent* event);

  // GC support
  // 在GC期间遍历线程栈中的对象（不包含帧信息）
  void oops_do_no_frames(OopClosure* f, NMethodClosure* cf);
  // 处理已编译的本地方法（nmethods），如清理或更新引用
  void nmethods_do(NMethodClosure* cf);
};

#endif // SHARE_RUNTIME_SERVICETHREAD_HPP
