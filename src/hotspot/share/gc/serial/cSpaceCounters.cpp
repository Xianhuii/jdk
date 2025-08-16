/*
 * Copyright (c) 2002, 2025, Oracle and/or its affiliates. All rights reserved.
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

#include "gc/serial/cSpaceCounters.hpp"
#include "memory/allocation.inline.hpp"
#include "memory/resourceArea.hpp"

// 构造函数
CSpaceCounters::CSpaceCounters(const char* name, int ordinal, size_t max_size,
                               ContiguousSpace* s, GenerationCounters* gc)
    : _space(s) { // 设置被监控的连续空间
  if (UsePerfData) { // 如果使用性能数据 -XX:+UsePerfData
    EXCEPTION_MARK;
    ResourceMark rm;

    // 构建性能计数器的命名空间
    const char* cns = PerfDataManager::name_space(gc->name_space(), "space",
                                                  ordinal);

    _name_space = NEW_C_HEAP_ARRAY(char, strlen(cns)+1, mtGC);
    strcpy(_name_space, cns);

    // 创建性能计数器
    const char* cname = PerfDataManager::counter_name(_name_space, "name");
    PerfDataManager::create_string_constant(SUN_GC, cname, name, CHECK);

    // 创建最大容量性能计数器
    cname = PerfDataManager::counter_name(_name_space, "maxCapacity");
    _max_capacity = PerfDataManager::create_variable(SUN_GC, cname,
                                                     PerfData::U_Bytes,
                                                     (jlong)max_size,
                                                     CHECK);

    // 创建容量性能计数器
    cname = PerfDataManager::counter_name(_name_space, "capacity");
    _capacity = PerfDataManager::create_variable(SUN_GC, cname,
                                                 PerfData::U_Bytes,
                                                 _space->capacity(),
                                                 CHECK);

    // 创建使用量性能计数器
    cname = PerfDataManager::counter_name(_name_space, "used");
    _used = PerfDataManager::create_variable(SUN_GC, cname, PerfData::U_Bytes,
                                             _space->used(),
                                             CHECK);

    // 创建初始容量性能计数器
    cname = PerfDataManager::counter_name(_name_space, "initCapacity");
    PerfDataManager::create_constant(SUN_GC, cname, PerfData::U_Bytes,
                                     _space->capacity(), CHECK);
  }
}

// 析构函数
CSpaceCounters::~CSpaceCounters() {
  FREE_C_HEAP_ARRAY(char, _name_space);
}

// 更新容量
void CSpaceCounters::update_capacity() {
  _capacity->set_value(_space->capacity());
}

// 更新使用量
void CSpaceCounters::update_used() {
  _used->set_value(_space->used());
}

// 更新所有性能计数器
void CSpaceCounters::update_all() {
  update_used();
  update_capacity();
}
