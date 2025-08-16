/*
 * Copyright (c) 2018, 2024, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SHARE_GC_SERIAL_SERIAL_GLOBALS_HPP
#define SHARE_GC_SERIAL_SERIAL_GLOBALS_HPP

// 定义Serial GC的全局参数：ShrinkHeapInSteps
// 该参数用于控制在Full GC时是否启用逐步缩小堆大小的功能
// 该参数的默认值为true，即启用逐步缩小堆大小的功能
// 该参数的作用是：
//  1. 当启用时，GC会在多个全GC中逐步缩小堆大小，以避免在一次全GC中缩小堆大小导致的性能问题
//  2. 当禁用时，GC会直接将堆大小缩小到目标大小，而不是逐步缩小
// 该参数的注意事项是：
//  1. 该参数仅在Full GC时生效
//  2. 该参数的默认值为true，即启用逐步缩小堆大小的功能
//  3. 该参数的作用是优化Full GC的性能，避免在一次Full GC中缩小堆大小导致的性能问题
#define GC_SERIAL_FLAGS(develop,                                            \
                        develop_pd,                                         \
                        product,                                            \
                        product_pd,                                         \
                        range,                                              \
                        constraint)                                         \
  product(bool, ShrinkHeapInSteps, true,                                    \
          "When disabled, informs the GC to shrink the java heap directly"  \
          " to the target size at the next full GC rather than requiring"   \
          " smaller steps during multiple full GCs.")                       \

// end of GC_SERIAL_FLAGS

#endif // SHARE_GC_SERIAL_SERIAL_GLOBALS_HPP
