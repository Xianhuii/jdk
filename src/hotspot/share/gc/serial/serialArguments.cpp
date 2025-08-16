/*
 * Copyright (c) 2017, Red Hat, Inc. and/or its affiliates.
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

#include "gc/serial/serialArguments.hpp"
#include "gc/serial/serialHeap.hpp"
#include "gc/shared/fullGCForwarding.hpp"
#include "gc/shared/gcArguments.hpp"

/*
 * 初始化串行收集器参数
 * 该方法用于初始化串行收集器的参数，包括初始化父类的参数和设置Full GC转发的参数
 */
void SerialArguments::initialize() {
  GCArguments::initialize(); // 初始化父类的参数
  FullGCForwarding::initialize_flags(MaxHeapSize); // 设置Full GC转发的参数
}

/*
 * 创建串行收集器
 * 该方法用于创建串行收集器，返回一个SerialHeap对象
 */
CollectedHeap* SerialArguments::create_heap() {
  return new SerialHeap();
}
