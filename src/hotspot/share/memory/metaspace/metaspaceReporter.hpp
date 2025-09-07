/*
 * Copyright (c) 2018, 2022, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2018, 2022 SAP SE. All rights reserved.
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

#ifndef SHARE_MEMORY_METASPACE_METASPACEREPORTER_HPP
#define SHARE_MEMORY_METASPACE_METASPACEREPORTER_HPP

#include "memory/allStatic.hpp"

namespace metaspace {
// 提供元空间（Metaspace）内存使用情况的报告生成功能，用于监控和分析JVM内存分配状态。
class MetaspaceReporter : public AllStatic {
public:

  // Flags for print_report().
  // 通过位掩码（bitmask）组合标志位，控制报告内容的详细程度
  enum class Option {
    // Show usage by class loader.
    // 按类加载器（ClassLoader）分组展示内存占用
    ShowLoaders                 = (1 << 0),
    // Breaks report down by chunk type (small, medium, ...).
    // 按内存块类型（如 small/medium/large）细分
    BreakDownByChunkType        = (1 << 1),
    // Breaks report down by space type (anonymous, reflection, ...).
    // 按空间类型（如匿名空间、反射空间）分类
    BreakDownBySpaceType        = (1 << 2),
    // Print details about the underlying virtual spaces.
    // 列出底层虚拟空间（Virtual Space）的详细信息
    ShowVSList                  = (1 << 3),
    // If show_loaders: show loaded classes for each loader.
    // 若启用 ShowLoaders，则显示每个加载器加载的类信息
    ShowClasses                 = (1 << 4),
    // Print details about the underlying virtual spaces.
    // 展示内存块的空闲列表（Free List）状态
    ShowChunkFreeList           = (1 << 5)
  };

  // This will print out a basic metaspace usage report but
  // unlike print_report() is guaranteed not to lock or to walk the CLDG.
  // 快速生成基础报告，不涉及同步或遍历类加载器图（CLDG）
  static void print_basic_report(outputStream* st, size_t scale);

  // Prints a report about the current metaspace state.
  // Optional parts can be enabled via flags.
  // Function will walk the CLDG and will lock the expand lock; if that is not
  // convenient, use print_basic_report() instead.
  // 生成完整报告，支持自定义选项和格式
  static void print_report(outputStream* out, size_t scale = 0, int flags = 0);

};

} // namespace metaspace

#endif // SHARE_MEMORY_METASPACE_METASPACEREPORTER_HPP
