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

#ifndef SHARE_RUNTIME_JAVAFRAMEANCHOR_HPP
#define SHARE_RUNTIME_JAVAFRAMEANCHOR_HPP

#include "runtime/orderAccess.hpp"
#include "utilities/globalDefinitions.hpp"
#include "utilities/macros.hpp"

//
// An object for encapsulating the machine/os dependent part of a JavaThread frame state
// 封装 Java 线程帧状态的机器/操作系统相关部分，用于管理 Java 到本地代码（如 JNI）调用时的栈帧信息
//
class JavaThread;
class MacroAssembler;
class UpcallLinker;
class ZeroFrame;

class JavaFrameAnchor {
// Too many friends...
friend class OptoRuntime;
friend class Runtime1;
friend class StubAssembler;
friend class CallRuntimeDirectNode;
friend class MacroAssembler;
friend class LIR_Assembler;
friend class GraphKit;
friend class StubGenerator;
friend class JavaThread;
friend class frame;
friend class VMStructs;
friend class JVMCIVMStructs;
friend class BytecodeInterpreter;
friend class JavaCallWrapper;
friend class UpcallLinker;

 private:
  //
  // Whenever _last_Java_sp != nullptr other anchor fields MUST be valid!
  // The stack may not be walkable [check with walkable() ] but the values must be valid.
  // The profiler apparently depends on this.
  //
  // 指向当前 Java 线程栈顶的指针
  intptr_t* volatile _last_Java_sp;

  // Whenever we call from Java to native we can not be assured that the return
  // address that composes the last_Java_frame will be in an accessible location
  // so calls from Java to native store that pc (or one good enough to locate
  // the oopmap) in the frame anchor. Since the frames that call from Java to
  // native are never deoptimized we never need to patch the pc and so this
  // is acceptable.
  // 记录从 Java 调用本地代码时的返回地址（PC 寄存器值
  volatile  address _last_Java_pc;

  // tells whether the last Java frame is set
  // It is important that when last_Java_sp != nullptr that the rest of the frame
  // anchor (including platform specific) all be valid.
  // 返回 _last_Java_sp是否非空，判断栈帧是否有效
  bool has_last_Java_frame() const                   { return _last_Java_sp != nullptr; }
  // This is very dangerous unless sp == nullptr
  // Invalidate the anchor so that has_last_frame is false
  // and no one should look at the other fields.
  // 将 _last_Java_sp设为 nullptr，使锚点失效，避免无效栈帧被误用
  void zap(void)                                     { _last_Java_sp = nullptr; }

#include CPU_HEADER(javaFrameAnchor)

public:
  JavaFrameAnchor()                              { clear(); }
  JavaFrameAnchor(JavaFrameAnchor *src)          { copy(src); }

  // 设置 _last_Java_pc的值为给定地址
  void set_last_Java_pc(address pc)              { _last_Java_pc = pc; }

  // Assembly stub generation helpers
  // 返回成员变量在类中的内存偏移量，用于底层内存操作（如汇编代码直接访问）
  static ByteSize last_Java_sp_offset()          { return byte_offset_of(JavaFrameAnchor, _last_Java_sp); }
  static ByteSize last_Java_pc_offset()          { return byte_offset_of(JavaFrameAnchor, _last_Java_pc); }

};

#endif // SHARE_RUNTIME_JAVAFRAMEANCHOR_HPP
