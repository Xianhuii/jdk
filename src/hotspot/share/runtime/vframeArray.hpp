/*
 * Copyright (c) 1997, 2024, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SHARE_RUNTIME_VFRAMEARRAY_HPP
#define SHARE_RUNTIME_VFRAMEARRAY_HPP

#include "memory/allocation.hpp"
#include "oops/arrayOop.hpp"
#include "runtime/deoptimization.hpp"
#include "runtime/frame.hpp"
#include "runtime/monitorChunk.hpp"
#include "runtime/registerMap.hpp"
#include "utilities/growableArray.hpp"

// 去优化（Deoptimization）机制相关的核心实现，主要用于管理在去优化过程中临时存储的Java方法帧（vframe）
// 去优化支持：当JVM需要从编译后的机器码（如C1/C2生成的代码）回退到解释执行时（例如因逆优化或异常处理），vframeArray负责保存和恢复方法调用的状态。
// 栈外存储：所有帧数据存储在栈外（off-stack），避免跨安全点（Safepoint）时的GC问题。
// A vframeArray is an array used for momentarily storing off stack Java method activations
// during deoptimization. Essentially it is an array of vframes where each vframe
// data is stored off stack. This structure will never exist across a safepoint so
// there is no need to gc any oops that are stored in the structure.


class LocalsClosure;
class ExpressionStackClosure;
class MonitorStackClosure;
class MonitorArrayElement;
class StackValueCollection;

// A vframeArrayElement is an element of a vframeArray. Each element
// represent an interpreter frame which will eventually be created.
// 表示单个解释器帧的快照，包含恢复该帧所需的所有信息
class vframeArrayElement {
  friend class VMStructs;

  private:

    // 解释器帧的元数据（如PC寄存器、栈顶指针）
    frame _frame;                                                // the interpreter frame we will unpack into
    // 字节码索引（Bytecode Index），标识当前执行的指令位置
    int  _bci;                                                   // raw bci for this vframe
    bool _reexecute;                                             // whether we should reexecute this bytecode
    // 对应的方法对象
    Method*    _method;                                          // the method for this vframe
    // 活跃的监视器（锁）信息（
    MonitorChunk* _monitors;                                     // active monitors for this vframe
    // 局部变量和表达式栈的值集合
    StackValueCollection* _locals;
    StackValueCollection* _expressions;
#ifdef ASSERT
    bool _removed_monitors;
#endif

  public:

  frame* iframe(void)                { return &_frame; }

  int bci(void) const;

  int raw_bci(void) const            { return _bci; }
  bool should_reexecute(void) const  { return _reexecute; }

  Method* method(void) const       { return _method; }

  MonitorChunk* monitors(void) const { return _monitors; }

  void free_monitors();

  StackValueCollection* locals(void) const             { return _locals; }

  StackValueCollection* expressions(void) const        { return _expressions; }

  // 从编译后的虚拟帧（compiledVFrame）填充数据
  void fill_in(compiledVFrame* vf, bool realloc_failures);

  // Formerly part of deoptimizedVFrame


  // Returns the on stack word size for this frame
  // callee_parameters is the number of callee locals residing inside this frame
  int on_stack_size(int callee_parameters,
                    int callee_locals,
                    bool is_top_frame,
                    int popframe_extra_stack_expression_els) const;

  // Unpacks the element to skeletal interpreter frame
  // 将帧数据恢复到实际栈上，重建解释器帧
  void unpack_on_stack(int caller_actual_parameters,
                       int callee_parameters,
                       int callee_locals,
                       frame* caller,
                       bool is_top_frame,
                       bool is_bottom_frame,
                       int exec_mode);

#ifdef ASSERT
  void set_removed_monitors() {
    _removed_monitors = true;
  }
#endif

#ifndef PRODUCT
  void print(outputStream* st);
#endif /* PRODUCT */
};

// this can be a ResourceObj if we don't save the last one...
// but it does make debugging easier even if we can't look
// at the data in each vframeElement
// 管理一组vframeArrayElement，作为去优化期间的帧缓存
class vframeArray: public CHeapObj<mtCompiler> {
  friend class VMStructs;

 private:


  // Here is what a vframeArray looks like in memory

  /*
      fixed part
        description of the original frame
        _frames - number of vframes in this array
        adapter info
        callee register save area
      variable part
        vframeArrayElement   [ 0 ]
        ...
        vframeArrayElement   [_frames - 1]

  */

  // 所属的Java线程
  JavaThread*                  _owner_thread;
  // 原始帧及其调用者/发送者帧信息
  frame                        _original;          // the original frame of the deoptee
  frame                        _caller;            // caller of root frame in vframeArray
  frame                        _sender;

  // 去优化控制块（UnrollBlock），包含恢复上下文
  Deoptimization::UnrollBlock* _unroll_block;
  int                          _frame_size;

  int                          _frames; // number of javavframes in the array (does not count any adapter)

  intptr_t                     _callee_registers[RegisterMap::reg_count];

  // 动态数组，存储所有vframeArrayElement实例
  vframeArrayElement           _elements[1];   // First variable section.

 public:


  // Tells whether index is within bounds.
  bool is_within_bounds(int index) const        { return 0 <= index && index < frames(); }

  // Accessories for instance variable
  int frames() const                            { return _frames;   }

  // 分配并初始化vframeArray实例
  static vframeArray* allocate(JavaThread* thread, int frame_size, GrowableArray<compiledVFrame*>* chunk,
                               RegisterMap* reg_map, frame sender, frame caller, frame self,
                               bool realloc_failures);


  vframeArrayElement* element(int index)        { assert(is_within_bounds(index), "Bad index"); return &_elements[index]; }

  // Allocates a new vframe in the array and fills the array with vframe information in chunk
  // 从编译帧列表批量填充vframe数据
  void fill_in(JavaThread* thread, int frame_size, GrowableArray<compiledVFrame*>* chunk, const RegisterMap *reg_map, bool realloc_failures);

  // Returns the owner of this vframeArray
  JavaThread* owner_thread() const           { return _owner_thread; }

  // Accessors for sp
  intptr_t* sp() const                       { return _original.sp(); }

  intptr_t* unextended_sp() const;

  frame original() const                     { return _original; }

  frame sender() const                       { return _sender; }

  // Accessors for unroll block
  Deoptimization::UnrollBlock* unroll_block() const         { return _unroll_block; }
  void set_unroll_block(Deoptimization::UnrollBlock* block) { _unroll_block = block; }

  // Returns the size of the frame that got deoptimized
  int frame_size() const { return _frame_size; }

  // Unpack the array on the stack passed in stack interval
  // 将缓存的帧展开到物理栈，恢复执行
  void unpack_to_stack(frame &unpack_frame, int exec_mode, int caller_actual_parameters);

  // Deallocates monitor chunks allocated during deoptimization.
  // This should be called when the array is not used anymore.
  // 释放所有监视器资源
  void deallocate_monitor_chunks();



  // Accessor for register map
  address register_location(int i) const;

  void print_on_2(outputStream* st) PRODUCT_RETURN;
  void print_value_on(outputStream* st) const PRODUCT_RETURN;

#ifndef PRODUCT
  // Comparing
  bool structural_compare(JavaThread* thread, GrowableArray<compiledVFrame*>* chunk);
#endif

};

#endif // SHARE_RUNTIME_VFRAMEARRAY_HPP
