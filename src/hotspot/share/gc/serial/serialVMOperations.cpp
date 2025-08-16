/*
 * Copyright (c) 2024, 2025, Oracle and/or its affiliates. All rights reserved.
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

#include "gc/serial/serialVMOperations.hpp"
#include "gc/shared/gcLocker.hpp"

// serialGC在分配内存时的垃圾回收操作
void VM_SerialCollectForAllocation::doit() {
  SerialHeap* gch = SerialHeap::heap(); // 串行堆
  GCCauseSetter gccs(gch, _gc_cause); // 垃圾回收原因设置器
  _result = gch->satisfy_failed_allocation(_word_size, _tlab); // 尝试满足失败的分配
  assert(_result == nullptr || gch->is_in_reserved(_result), "result not in heap");
}

void VM_SerialGCCollect::doit() {
  SerialHeap* gch = SerialHeap::heap(); // 串行堆
  GCCauseSetter gccs(gch, _gc_cause); // 垃圾回收原因设置器
  gch->collect_at_safepoint(_full); // 收集堆
}
