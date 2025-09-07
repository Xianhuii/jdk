/*
 * Copyright (c) 2018, 2020, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2018, 2020 SAP SE. All rights reserved.
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

#ifndef SHARE_MEMORY_METASPACE_PRINTCLDMETASPACEINFOCLOSURE_HPP
#define SHARE_MEMORY_METASPACE_PRINTCLDMETASPACEINFOCLOSURE_HPP

#include "memory/iterator.hpp"
#include "memory/metaspace.hpp"
#include "memory/metaspace/metaspaceStatistics.hpp"
#include "utilities/globalDefinitions.hpp"

class outputStream;

namespace metaspace {

// 遍历类加载器数据（ClassLoaderData），收集并输出元空间统计信息
class PrintCLDMetaspaceInfoClosure : public CLDClosure {
private:
  outputStream* const _out; // 统计结果的输出目标
  const size_t        _scale; // 数值缩放比例（用于可读性，如将字节转换为KB/MB）
  const bool          _do_print; // 是否启用打印功能
  const bool          _do_print_classes; // 是否打印类级别的统计信息
  const bool          _break_down_by_chunktype; // 是否按内存块类型细分统计

public:

  uintx                           _num_loaders; // 总类加载器数量
  uintx                           _num_loaders_without_metaspace; // 未使用元空间的类加载器数
  uintx                           _num_loaders_unloading; // 正在卸载的类加载器数
  ClmsStats                       _stats_total; // 全局元空间统计（如总大小、已用/空闲空间）

  uintx                           _num_loaders_by_spacetype [Metaspace::MetaspaceTypeCount]; // 各类型元空间的加载器数量
  ClmsStats                       _stats_by_spacetype [Metaspace::MetaspaceTypeCount]; // 各类型元空间的详细统计

  uintx                           _num_classes_by_spacetype [Metaspace::MetaspaceTypeCount]; // 各类型元空间中的类数量
  uintx                           _num_classes_shared_by_spacetype [Metaspace::MetaspaceTypeCount]; // 各类型元空间中的共享类数量
  uintx                           _num_classes; // 总类数量
  uintx                           _num_classes_shared; // 总共享类数量

  PrintCLDMetaspaceInfoClosure(outputStream* out, size_t scale, bool do_print,
                               bool do_print_classes, bool break_down_by_chunktype);
  // 实现CLDClosure接口，处理单个ClassLoaderData对象
  void do_cld(ClassLoaderData* cld);

};

} // namespace metaspace

#endif // SHARE_MEMORY_METASPACE_PRINTCLDMETASPACEINFOCLOSURE_HPP
