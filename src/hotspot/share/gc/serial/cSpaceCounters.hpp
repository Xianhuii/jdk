/*
 * Copyright (c) 2002, 2023, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SHARE_GC_SERIAL_CSPACECOUNTERS_HPP
#define SHARE_GC_SERIAL_CSPACECOUNTERS_HPP

#include "gc/shared/generationCounters.hpp"
#include "gc/shared/space.hpp"
#include "runtime/perfData.hpp"

// A CSpaceCounters is a holder class for performance counters
// that track a space;

// 连续空间性能计数器
class CSpaceCounters: public CHeapObj<mtGC> {
  friend class VMStructs;

 private:
  PerfVariable*      _capacity; // 容量
  PerfVariable*      _used; // 使用量
  PerfVariable*      _max_capacity; // 最大容量

  // Constant PerfData types don't need to retain a reference.
  // However, it's a good idea to document them here.
  // PerfConstant*     _size;

  ContiguousSpace*     _space; // 被监控的连续空间
  char*                _name_space; // 性能计数器的命名空间

 public:

  // 构造函数
  CSpaceCounters(const char* name, int ordinal, size_t max_size,
                 ContiguousSpace* s, GenerationCounters* gc);

  // 析构函数
  ~CSpaceCounters();

  // 更新容量
  void update_capacity();
  // 更新使用量
  void update_used();
  // 更新所有性能计数器
  void update_all();
  // 获取性能计数器的命名空间
  const char* name_space() const        { return _name_space; }
};

#endif // SHARE_GC_SERIAL_CSPACECOUNTERS_HPP
