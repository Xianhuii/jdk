/*
 * Copyright (c) 2014, 2021, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SHARE_MEMORY_METASPACETRACER_HPP
#define SHARE_MEMORY_METASPACETRACER_HPP

#include "memory/allocation.hpp"
#include "memory/metaspace.hpp"
#include "memory/metaspaceUtils.hpp"

class ClassLoaderData;

// 继承自CHeapObj<mtTracing>
// 表示对象在C堆（非Java堆）上分配，mtTracing是内存类型标识符，用于区分不同用途的内存分配
class MetaspaceTracer : public CHeapObj<mtTracing> {
  // 模板函数，用于发送具体的分配失败事件。
  // 可能根据不同的事件类型（如OOM、普通失败）生成不同格式的事件数据，供外部系统（如监控工具）消费。
  template <typename E>
  void send_allocation_failure_event(ClassLoaderData *cld,
                                     size_t word_size,
                                     MetaspaceObj::Type objtype,
                                     Metaspace::MetadataType mdtype) const;
 public:
  // 当元空间GC阈值变化时触发
  void report_gc_threshold(size_t old_val, // 旧阈值
                           size_t new_val, // 新阈值
                           MetaspaceGCThresholdUpdater::Type updater // 触发更新的类型
                           ) const;

  // 元空间分配失败时报告
  void report_metaspace_allocation_failure(ClassLoaderData *cld, // 关联的类加载器数据
                                           size_t word_size, // 分配失败的内存大小（以字为单位）
                                           MetaspaceObj::Type objtype, // 分配对象的类型（如类、方法等）
                                           Metaspace::MetadataType mdtype // 元数据类型（如类元数据、常量池等）
                                           ) const;

  // 元数据内存耗尽（OOM）时触发
  void report_metadata_oom(ClassLoaderData *cld,
                           size_t word_size,
                           MetaspaceObj::Type objtype,
                           Metaspace::MetadataType mdtype) const;

};

#endif // SHARE_MEMORY_METASPACETRACER_HPP
