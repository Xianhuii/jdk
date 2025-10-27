/*
 * Copyright (c) 2020, 2024, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SHARE_RUNTIME_STACKCHUNKFRAMESTREAM_HPP
#define SHARE_RUNTIME_STACKCHUNKFRAMESTREAM_HPP

#include "memory/allocation.hpp"
#include "oops/oopsHierarchy.hpp"
#include "utilities/globalDefinitions.hpp"
#include "utilities/macros.hpp"

// 实现了对混合模式堆栈帧的高效遍历和管理，为垃圾回收等关键功能提供了底层支持
class CodeBlob;
class frame;
class ImmutableOopMap;
class RegisterMap;
class VMRegImpl;
typedef VMRegImpl* VMReg;

// 堆栈帧类型
enum ChunkFrames {
  CompiledOnly, // 仅编译后的帧
  Mixed // 混合模式帧（包含解释执行部分）
};

template <ChunkFrames frame_kind> // 指定帧类型
class StackChunkFrameStream : public StackObj {
private:
  // 栈结束位置
  intptr_t* _end;
  // 当前栈指针
  intptr_t* _sp;
  // （仅在Mixed模式下使用）：未扩展栈指针
  intptr_t* _unextended_sp; // used only when mixed
  // 指向CodeBlob对象（表示JIT编译后的代码块）
  CodeBlob* _cb;
  // 不可变OopMap指针（用于GC时识别对象引用）
  mutable const ImmutableOopMap* _oopmap;

#ifndef PRODUCT
  stackChunkOop _chunk;
  int _index;
#endif

#ifdef ASSERT
  int _has_stub;
#endif

public:
  StackChunkFrameStream() { NOT_PRODUCT(_chunk = nullptr; _index = -1;) DEBUG_ONLY(_has_stub = false;) }
  inline StackChunkFrameStream(stackChunkOop chunk);
  inline StackChunkFrameStream(stackChunkOop chunk, const frame& f);

  // 判断是否遍历完成
  bool is_done() const { return _sp >= _end; }

  // Query
  intptr_t*        sp() const  { return _sp; }
  // 获取当前程序计数器
  inline address   pc() const  { return get_pc(); }
  // 获取框架指针
  inline intptr_t* fp() const;
  inline intptr_t* unextended_sp() const { return frame_kind == ChunkFrames::Mixed ? _unextended_sp : _sp; }
  inline address orig_pc() const;

  // 是否解释型帧
  inline bool is_interpreted() const;
  // 是否存根帧
  inline bool is_stub() const;
  // 是否编译后帧
  inline bool is_compiled() const;
  CodeBlob* cb() const { return _cb; }
  // 获取CodeBlob对象
  inline void get_cb();
  const ImmutableOopMap* oopmap() const { if (_oopmap == nullptr) get_oopmap(); return _oopmap; }
  // 帧大小
  inline int frame_size() const;
  inline int stack_argsize() const;
  // Oop引用数量
  inline int num_oops() const;

  // 初始化寄存器映射
  inline void initialize_register_map(RegisterMap* map);
  template <typename RegisterMapT> inline void next(RegisterMapT* map, bool stop = false);

  // 更新寄存器映射
  template <typename RegisterMapT> inline void update_reg_map(RegisterMapT* map);

  // 处理去优化情况
  void handle_deopted() const;

  // 转换为frame对象
  inline frame to_frame() const;

#ifdef ASSERT
  bool is_in_frame(void* p) const;
  template <typename RegisterMapT> bool is_in_oops(void* p, const RegisterMapT* map) const;
#endif

  void print_on(outputStream* st) const PRODUCT_RETURN;

 private:
  inline address get_pc() const;

  inline int interpreter_frame_size() const;
  inline int interpreter_frame_num_oops() const;
  inline int interpreter_frame_stack_argsize() const;
  inline void next_for_interpreter_frame();
  inline intptr_t* unextended_sp_for_interpreter_frame() const;
  inline intptr_t* derelativize(int offset) const;
  inline void get_oopmap() const;
  inline void get_oopmap(address pc, int oopmap_slot) const;

  template <typename RegisterMapT> inline void update_reg_map_pd(RegisterMapT* map);

  template <typename RegisterMapT>
  inline void* reg_to_loc(VMReg reg, const RegisterMapT* map) const;

  void assert_is_interpreted_and_frame_type_mixed() const NOT_DEBUG_RETURN;

public:
  template <class OopClosureType, class RegisterMapT>
  // 遍历Oop引用
  inline void iterate_oops(OopClosureType* closure, const RegisterMapT* map) const;
  template <class DerivedOopClosureType, class RegisterMapT>
  // 遍历派生指针
  inline void iterate_derived_pointers(DerivedOopClosureType* closure, const RegisterMapT* map) const;
};

#endif // SHARE_RUNTIME_STACKCHUNKFRAMESTREAM_HPP
