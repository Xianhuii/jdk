/*
 * Copyright (c) 2017, 2025, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SHARE_RUNTIME_SAFEPOINTMECHANISM_HPP
#define SHARE_RUNTIME_SAFEPOINTMECHANISM_HPP

#include "runtime/globals.hpp"
#include "runtime/osInfo.hpp"
#include "utilities/globalDefinitions.hpp"
#include "utilities/macros.hpp"
#include "utilities/sizes.hpp"

class JavaThread;
class Thread;

// 定义了 JVM 安全点（Safepoint）机制的抽象接口，用于协调线程暂停（SafePoint Polling），以支持全局操作（如垃圾回收、线程栈转储等）
// 提供跨平台的统一接口（通过 pd_initialize()允许平台相关实现）
// 最小化安全点操作对程序性能的影响（通过轮询优化和内存屏障）
// This is the abstracted interface for the safepoint implementation
class SafepointMechanism : public AllStatic {
  friend class StackWatermark;
  // 标记安全点页面的“武装”状态（是否触发安全点检查）
  static uintptr_t _poll_page_armed_value;
  static uintptr_t _poll_page_disarmed_value;

  // 线程本地轮询词的武装状态
  static uintptr_t _poll_word_armed_value;
  static uintptr_t _poll_word_disarmed_value;

  // 安全点页面的地址（通过内存页保护机制触发陷阱）
  static address _polling_page;

  static inline void disarm_local_poll(JavaThread* thread);

  // 检查是否存在全局安全点请求
  static inline bool global_poll();

  // 判断线程是否有待处理的安全点请求
  static inline bool has_pending_safepoint(JavaThread* thread);

  // 处理安全点请求，可挂起线程并处理异步异常
  static void process(JavaThread *thread, bool allow_suspend, bool check_async_exception);

  static void default_initialize();

  static void pd_initialize() NOT_AIX({ default_initialize(); });

  static uintptr_t compute_poll_word(bool armed, uintptr_t stack_watermark);

  // 轮询位的掩码（1，用于标记状态）
  const static intptr_t _poll_bit = 1;
 public:
  // 检查线程的本地轮询点是否已启用
  static inline bool local_poll_armed(JavaThread* thread);
  static intptr_t poll_bit() { return _poll_bit; }

  static address get_polling_page()             { return _polling_page; }
  static bool    is_poll_address(address addr)  { return addr >= _polling_page && addr < (_polling_page + OSInfo::vm_page_size()); }

  struct ThreadData {
    volatile uintptr_t _polling_word;
    volatile uintptr_t _polling_page;

    inline void set_polling_word(uintptr_t poll_value);
    inline uintptr_t get_polling_word();

    inline void set_polling_page(uintptr_t poll_value);
  };

  // Call this method to see if this thread should block for a safepoint or process handshake.
  static inline bool should_process(JavaThread* thread, bool allow_suspend = true);

  // Processes a pending requested operation.
  static inline void process_if_requested(JavaThread* thread, bool allow_suspend, bool check_async_exception);
  static inline void process_if_requested_with_exit_check(JavaThread* thread, bool check_async_exception);
  // Compute what the poll values should be and install them.
  // 根据线程状态更新轮询词的值
  static void update_poll_values(JavaThread* thread);

  // 启用/禁用本地轮询点（使用内存屏障保证可见性）
  // Caller is responsible for using a memory barrier if needed.
  static inline void arm_local_poll(JavaThread* thread);
  // Release semantics
  static inline void arm_local_poll_release(JavaThread* thread);

  // Setup the selected safepoint mechanism
  // 全局初始化安全点机制
  static void initialize();
  // 为线程初始化安全点相关数据结构
  static void initialize_header(JavaThread* thread);
};

#endif // SHARE_RUNTIME_SAFEPOINTMECHANISM_HPP
