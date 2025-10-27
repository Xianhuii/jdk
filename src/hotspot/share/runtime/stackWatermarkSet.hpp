/*
 * Copyright (c) 2020, 2024, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SHARE_RUNTIME_STACKWATERMARKSET_HPP
#define SHARE_RUNTIME_STACKWATERMARKSET_HPP

#include "memory/allStatic.hpp"
#include "runtime/stackWatermarkKind.hpp"

class frame;
class JavaThread;
class StackWatermark;

// A thread may have multiple StackWatermarks installed, for different unrelated client
// applications of lazy stack processing. The StackWatermarks class is the thread-local
// data structure used to store said watermarks. The StackWatermarkSet is the corresponding
// AllStatic class you use to interact with watermarks from shared runtime code. It allows
// hooks for all watermarks, or requesting specific action for specific client StackWatermark
// instances (if they have been installed).
// 存储单个线程的所有StackWatermark实例
// 每个JavaThread维护独立的StackWatermarks实例，通过head()和set_head()管理水印链表。
// 支持为不同客户端注册多种水印（通过StackWatermarkKind枚举区分），实现灵活扩展。
// 在关键栈操作（展开、遍历、安全点）中插入回调，确保水印逻辑及时触发。
// 延迟处理直到必要时（如安全点唤醒），减少不必要的性能开销。
class StackWatermarks {
  friend class StackWatermarkSet;
private:
  // 指向链表头部的指针，用于管理多个水印
  StackWatermark* _head;

public:
  StackWatermarks();
  ~StackWatermarks();
};

// 提供全局静态接口，用于操作线程的StackWatermark集合
class StackWatermarkSet : public AllStatic {
private:
  static StackWatermark* head(JavaThread* jt);
  static void set_head(JavaThread* jt, StackWatermark* watermark);

public:
  // 向线程添加水印
  static void add_watermark(JavaThread* jt, StackWatermark* watermark);

  // 按类型获取水印（含非模板和模板版本）
  static StackWatermark* get(JavaThread* jt, StackWatermarkKind kind);

  template <typename T>
  static T* get(JavaThread* jt, StackWatermarkKind kind);

  // 检查是否存在特定类型的水印
  static bool has_watermark(JavaThread* jt, StackWatermarkKind kind);

  // Called when a thread is about to unwind a frame
  // 栈展开前触发
  static void before_unwind(JavaThread* jt);

  // Called when a thread just unwound a frame
  // 栈展开后触发
  static void after_unwind(JavaThread* jt);

  // Called by stack walkers when walking into a frame
  // 栈遍历时进入新帧时触发
  static void on_iteration(JavaThread* jt, const frame& fr);

  // Called to ensure that processing of the thread is started when waking up from safepoint
  // 安全点唤醒时确保处理开始
  static void on_safepoint(JavaThread* jt);

  // Called to ensure that processing of the thread is started
  // 启动特定类型水印的处理
  static void start_processing(JavaThread* jt, StackWatermarkKind kind);

  // Returns true if all StackWatermarks have been started.
  // 检查所有水印是否已启动
  static bool processing_started(JavaThread* jt);

  // Called to finish the processing of a thread
  // 完成处理并传递上下文
  static void finish_processing(JavaThread* jt, void* context, StackWatermarkKind kind);

  // The lowest watermark among the watermarks in the set (the first encountered
  // watermark in the set as you unwind frames)
  // 返回当前栈中优先级最低的水印地址
  static uintptr_t lowest_watermark(JavaThread* jt);

  // We are synchronizing a safepoint, so we might want to ensure processing has at least
  // started, as safepoint operations sometimes assume that is the case
  // 安全点同步开始时触发
  static void safepoint_synchronize_begin();
};

#endif // SHARE_RUNTIME_STACKWATERMARKSET_HPP
