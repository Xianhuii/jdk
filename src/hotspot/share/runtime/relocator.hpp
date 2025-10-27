/*
 * Copyright (c) 1997, 2023, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SHARE_RUNTIME_RELOCATOR_HPP
#define SHARE_RUNTIME_RELOCATOR_HPP

#include "interpreter/bytecodes.hpp"
#include "oops/method.hpp"
#include "utilities/bytes.hpp"

// 实现Java方法字节码的动态重定位机制，用于在运行时调整代码结构（如插入指令、扩展操作码、修复跳转偏移等），
// 并维护相关元数据（异常表、行号表、局部变量表等）的一致性。通过监听器模式通知外部模块代码变更事件。
// This code has been converted from the 1.1E java virtual machine
// Thanks to the JavaTopics group for using the code

class ChangeItem;

// Callback object for code relocations
// 回调接口
// 在代码重定位过程中接收通知，允许外部模块响应代码变更。
class RelocatorListener : public StackObj {
 public:
  // bci：字节码索引（Bytecode Index）
  // delta：代码块的位移量（正值表示向后移动，负值向前）
  // new_method_size：调整后方法的总长度
  virtual void relocated(int bci, int delta, int new_method_size) = 0;
};

// 核心重定位器
// 管理方法字节码的插入、扩展、收缩及关联元数据的调整。
class Relocator : public ResourceObj {
 public:
  // 初始化重定位上下文，绑定目标方法和监听器。
  Relocator(const methodHandle& method, RelocatorListener* listener);
  // 在指定bci位置插入space长度的字节码，使用inst_buffer提供新指令，并触发重定位流程。
  methodHandle insert_space_at(int bci, int space, u_char inst_buffer[], TRAPS);

  // Callbacks from ChangeItem's
  // 主入口，协调所有变更处理逻辑。
  bool handle_code_changes();
  // 处理指令扩展（如将短跳转改为长跳转）
  bool handle_widen       (int bci, int new_ilen, u_char inst_buffer[]);  // handles general instructions
  void push_jump_widen  (int bci, int delta, int new_delta);    // pushes jumps
  // 专门处理跳转指令的扩展。
  bool handle_jump_widen  (int bci, int delta);     // handles jumps
  // 调整tableswitch/lookupswitch的填充字节。
  bool handle_switch_pad  (int bci, int old_pad, bool is_lookup_switch); // handles table and lookup switches

 private:
  // 原始字节码数组
  unsigned char* _code_array;
  int            _code_array_length;
  // 有效字节码长度
  int            _code_length;
  unsigned char* _compressed_line_number_table;
  int            _compressed_line_number_table_size;
  // 关联的Method对象句柄
  methodHandle   _method;
  // 临时存储被覆盖的字节（用于指令缩短时的回滚）
  u_char         _overwrite[3];             // stores overwritten bytes for shrunken instructions

  // 记录代码变更事件的列表
  GrowableArray<ChangeItem*>* _changes;

  unsigned char* code_array() const         { return _code_array; }
  void set_code_array(unsigned char* array) { _code_array = array; }

  int code_length() const                   { return _code_length; }
  void set_code_length(int length)          { _code_length = length; }

  int code_array_length() const             { return _code_array_length; }
  void set_code_array_length(int length)    { _code_array_length = length; }

  unsigned char* compressed_line_number_table() const         { return _compressed_line_number_table; }
  void set_compressed_line_number_table(unsigned char* table) { _compressed_line_number_table = table; }

  int compressed_line_number_table_size() const               { return _compressed_line_number_table_size; }
  void set_compressed_line_number_table_size(int size)        { _compressed_line_number_table_size = size; }

  methodHandle method() const               { return _method; }
  void set_method(const methodHandle& method)      { _method = method; }

  // This will return a raw bytecode, which is possibly rewritten.
  Bytecodes::Code code_at(int bci) const          { return (Bytecodes::Code) code_array()[bci]; }
  void code_at_put(int bci, Bytecodes::Code code) { code_array()[bci] = (char) code; }

  // get and set signed integers in the code_array
  inline int   int_at(int bci) const               { return Bytes::get_Java_u4(&code_array()[bci]); }
  inline void  int_at_put(int bci, int value)      { Bytes::put_Java_u4(&code_array()[bci], value); }

  // get and set signed shorts in the code_array
  inline short short_at(int bci) const            { return (short)Bytes::get_Java_u2(&code_array()[bci]); }
  inline void  short_at_put(int bci, short value) { Bytes::put_Java_u2((address) &code_array()[bci], value); }

  // get the address of in the code_array
  inline char* addr_at(int bci) const             { return (char*) &code_array()[bci]; }

  int  instruction_length_at(int bci)             { return Bytecodes::length_at(nullptr, code_array() + bci); }

  // Helper methods
  int  align(int n) const                          { return (n+3) & ~3; }
  int  code_slop_pct() const                       { return 25; }
  bool is_opcode_lookupswitch(Bytecodes::Code bc);

  // basic relocation methods
  bool relocate_code         (int bci, int ilen, int delta);
  void change_jumps          (int break_bci, int delta);
  void change_jump           (int bci, int offset, bool is_short, int break_bci, int delta);
  void adjust_exception_table(int bci, int delta);
  void adjust_line_no_table  (int bci, int delta);
  void adjust_local_var_table(int bci, int delta);
  void adjust_stack_map_table(int bci, int delta);
  int  get_orig_switch_pad   (int bci, bool is_lookup_switch);
  int  rc_instr_len          (int bci);
  bool expand_code_array     (int delta);

  // Callback support
  RelocatorListener *_listener;
  void notify(int bci, int delta, int new_code_length) {
    if (_listener != nullptr)
      _listener->relocated(bci, delta, new_code_length);
  }
};

#endif // SHARE_RUNTIME_RELOCATOR_HPP
