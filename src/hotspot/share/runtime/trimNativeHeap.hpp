/*
 * Copyright (c) 2023 SAP SE. All rights reserved.
 * Copyright (c) 2023 Red Hat Inc. All rights reserved.
 * Copyright (c) 2023, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SHARE_RUNTIME_TRIMNATIVEHEAP_HPP
#define SHARE_RUNTIME_TRIMNATIVEHEAP_HPP

#include "memory/allStatic.hpp"
#include "runtime/globals.hpp"

class outputStream;

// 管理本地堆（Native Heap）的周期性内存修剪，优化内存使用效率
class NativeHeapTrimmer : public AllStatic {

  // Pause periodic trim (if enabled).
  // 暂停周期性修剪，需提供操作原因（如调试标记或事件日志）
  static void suspend_periodic_trim(const char* reason);

  // Unpause periodic trim (if enabled).
  // 恢复周期性修剪，需提供与暂停时一致的原因
  static void resume_periodic_trim(const char* reason);

public:

  // 初始化本地堆修剪器（如启动定时任务）
  static void initialize();
  // 释放资源，停止定时任务等清理工作
  static void cleanup();

  // 静态内联方法，检查修剪功能是否启用。条件为全局变量TrimNativeHeapInterval > 0
  static inline bool enabled() { return TrimNativeHeapInterval > 0; }

  static void print_state(outputStream* st);

  // Pause periodic trimming while in scope; when leaving scope,
  // resume periodic trimming.
  // 实现作用域内的自动暂停/恢复机制
  // 确保在代码块执行期间临时禁用修剪，避免干扰关键操作（如大规模内存分配）
  struct SuspendMark {
    const char* const _reason;
    // 进入作用域时暂停修剪，并记录原因
    SuspendMark(const char* reason = "unknown") : _reason(reason) {
      if (NativeHeapTrimmer::enabled()) {
        suspend_periodic_trim(_reason);
      }
    }
    // 离开作用域时自动恢复修剪
    ~SuspendMark()  {
      if (NativeHeapTrimmer::enabled()) {
        resume_periodic_trim(_reason);
      }
    }
  };
};

#endif // SHARE_RUNTIME_TRIMNATIVEHEAP_HPP
