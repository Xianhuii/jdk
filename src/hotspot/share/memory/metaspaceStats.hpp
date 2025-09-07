/*
 * Copyright (c) 2021, 2022, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2021 SAP SE. All rights reserved.
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
#ifndef SHARE_MEMORY_METASPACESTATS_HPP
#define SHARE_MEMORY_METASPACESTATS_HPP

#include "utilities/globalDefinitions.hpp"

// Data holder classes for metaspace statistics.
//
// - MetaspaceStats: keeps reserved, committed and used byte counters;
//                   retrieve with MetaspaceUtils::get_statistics(MetadataType) for either class space
//                   or non-class space
//
// - MetaspaceCombinedStats: keeps reserved, committed and used byte counters, separately for both class- and non-class-space;
//                      retrieve with MetaspaceUtils::get_combined_statistics()

// (Note: just for NMT these objects need to be mutable)
// 表示单个元空间（非类空间或类空间）的内存统计信息
class MetaspaceStats {
  size_t _reserved; // 预留的虚拟内存大小（字节）
  size_t _committed; // 已提交的内存大小（已分配给操作系统的物理内存或交换空间）
  size_t _used; // 实际已使用的内存大小（由JVM分配的对象占用）
public:
  MetaspaceStats() : _reserved(0), _committed(0), _used(0) {}
  MetaspaceStats(size_t r, size_t c, size_t u) : _reserved(r), _committed(c), _used(u) {}
  size_t used() const       { return _used; }
  size_t committed() const  { return _committed; }
  size_t reserved() const   { return _reserved; }
};

// Class holds combined statistics for both non-class and class space.
// 组合统计类空间（Class Space）和非类空间（Non-Class Space）的内存使用
class MetaspaceCombinedStats : public MetaspaceStats {
  MetaspaceStats _cstats;  // class space stats 类空间的MetaspaceStats对象
  MetaspaceStats _ncstats; // non-class space stats 非类空间的MetaspaceStats对象
public:
  MetaspaceCombinedStats() {}
  MetaspaceCombinedStats(const MetaspaceStats& cstats, const MetaspaceStats& ncstats) :
    MetaspaceStats(cstats.reserved() + ncstats.reserved(),
                   cstats.committed() + ncstats.committed(),
                   cstats.used() + ncstats.used()),
    _cstats(cstats), _ncstats(ncstats)
  {}

  const MetaspaceStats& class_space_stats() const { return _cstats; }
  const MetaspaceStats& non_class_space_stats() const { return _ncstats; }
  size_t class_used() const       { return _cstats.used(); }
  size_t class_committed() const  { return _cstats.committed(); }
  size_t class_reserved() const   { return _cstats.reserved(); }
  size_t non_class_used() const       { return _ncstats.used(); }
  size_t non_class_committed() const  { return _ncstats.committed(); }
  size_t non_class_reserved() const   { return _ncstats.reserved(); }
};

#endif // SHARE_MEMORY_METASPACESTATS_HPP
