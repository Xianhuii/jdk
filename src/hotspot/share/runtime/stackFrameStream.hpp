/*
 * Copyright (c) 2021, 2022, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SHARE_RUNTIME_STACKFRAMESTREAM_HPP
#define SHARE_RUNTIME_STACKFRAMESTREAM_HPP

#include "memory/allocation.hpp"
#include "runtime/frame.hpp"
#include "runtime/registerMap.hpp"

class JavaThread;

// 遍历线程堆栈帧，从顶层帧开始，逐层向下访问。
// 自动管理寄存器状态（若启用更新）和延迟 GC 处理（若启用）。
// 支持安全点（Safepoint）场景下的全寄存器保存。
//
// StackFrameStream iterates through the frames of a thread starting from
// top most frame. It automatically takes care of updating the location of
// all (callee-saved) registers iff the update flag is set. It also
// automatically takes care of lazily applying deferred GC processing
// onto exposed frames, such that all oops are valid iff the process_frames
// flag is set.
//
// Notice: If a thread is stopped at a safepoint, all registers are saved,
// not only the callee-saved ones.
//
// Use:
//
//   for(StackFrameStream fst(thread, true /* update */, true /* process_frames */);
//       !fst.is_done();
//       fst.next()) {
//     ...
//   }
//
class StackFrameStream : public StackObj {
 private:
  // 当前正在访问的堆栈帧对象
  frame       _fr;
  // 记录寄存器与堆栈槽的映射关系，用于 GC 时追踪对象引用
  RegisterMap _reg_map;
  // 标识迭代是否完成
  bool        _is_done;
 public:
  StackFrameStream(JavaThread *thread, bool update, bool process_frames, bool allow_missing_reg = false);

  // Iteration
  inline bool is_done();
  void next()                     { if (!_is_done) _fr = _fr.sender(&_reg_map); }

  // Query
  frame *current()                { return &_fr; }
  RegisterMap* register_map()     { return &_reg_map; }
};

#endif // SHARE_RUNTIME_STACKFRAMESTREAM_HPP


