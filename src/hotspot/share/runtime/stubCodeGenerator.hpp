/*
 * Copyright (c) 1997, 2025, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SHARE_RUNTIME_STUBCODEGENERATOR_HPP
#define SHARE_RUNTIME_STUBCODEGENERATOR_HPP

#include "asm/assembler.hpp"
#include "memory/allocation.hpp"
#include "runtime/stubInfo.hpp"

// 提供存根代码（Stub Code）生成、管理和调试的基础设施，用于JVM内部优化和底层操作（如异常处理、同步原语等）。
// 核心流程
// 代码生成阶段：
// 使用StubCodeGenerator创建存根代码，通过MacroAssembler生成汇编指令。
// StubCodeMark在代码块前后自动记录元数据（通过stub_prolog和stub_epilog）。
// 调试与追踪：
// StubCodeDesc链表存储所有生成的代码段信息。
// 可通过desc_for(pc)快速定位某段代码的归属和范围。
// 内存管理：
// 所有StubCodeDesc实例通过CHeapObj在堆上分配，需手动管理生命周期。
// 冻结机制防止运行时意外修改已注册的代码段元数据。
// All the basic framework for stub code generation/debugging/printing.


// A StubCodeDesc describes a piece of generated code (usually stubs).
// This information is mainly useful for debugging and printing.
// Currently, code descriptors are simply chained in a linked list,
// this may have to change if searching becomes too slow.
// 描述一段生成的存根代码的元数据，用于调试和追踪
class StubCodeDesc: public CHeapObj<mtCode> {
 private:
  // 全局链表头，管理所有StubCodeDesc实例
  static StubCodeDesc* _list;     // the list of all descriptors
  // 标记是否允许修改链表
  static bool          _frozen;   // determines whether _list modifications are allowed

  StubCodeDesc*        _next;     // the next element in the linked list
  // 代码所属的逻辑分组（如"Interpreter"、"Compiler"）
  const char*          _group;    // the group to which the stub code belongs
  // 存根的唯一名称（如"call_stub"）
  const char*          _name;     // the name assigned to the stub code
  // 代码的内存范围（字节地址）
  address              _begin;    // points to the first byte of the stub code    (included)
  address              _end;      // points to the first byte after the stub code (excluded)
  // 相对于基地址的偏移量
  uint                 _disp;     // Displacement relative base address in buffer.

  friend class StubCodeMark;
  friend class StubCodeGenerator;

  void set_begin(address begin) {
    assert(begin >= _begin, "begin may not decrease");
    assert(_end == nullptr || begin <= _end, "begin & end not properly ordered");
    _begin = begin;
  }

  void set_end(address end) {
    assert(_begin <= end, "begin & end not properly ordered");
    _end = end;
  }

  void set_disp(uint disp) { _disp = disp; }

 public:
  static StubCodeDesc* first() { return _list; }
  static StubCodeDesc* next(StubCodeDesc* desc)  { return desc->_next; }

  // 根据PC地址查找对应的StubCodeDesc
  static StubCodeDesc* desc_for(address pc);     // returns the code descriptor for the code containing pc or null

  StubCodeDesc(const char* group, const char* name, address begin, address end = nullptr) {
    assert(!_frozen, "no modifications allowed");
    assert(name != nullptr, "no name specified");
    _next           = _list;
    _group          = group;
    _name           = name;
    _begin          = begin;
    _end            = end;
    _disp           = 0;
    _list           = this;
  };

  // 控制链表的修改权限
  static void freeze();
  static void unfreeze();

  const char* group() const                      { return _group; }
  const char* name() const                       { return _name; }
  address     begin() const                      { return _begin; }
  address     end() const                        { return _end; }
  uint        disp() const                       { return _disp; }
  int         size_in_bytes() const              { return pointer_delta_as_int(_end, _begin); }
  bool        contains(address pc) const         { return _begin <= pc && pc < _end; }
  void        print_on(outputStream* st) const;
  void        print() const;
};

// forward declare blob and stub id enums

// The base class for all stub-generating code generators.
// Provides utility functions.
// 生成存根代码的核心基类，提供汇编生成工具和生命周期管理
class StubCodeGenerator: public StackObj {
 private:
  // 是否打印生成的汇编代码
  bool _print_code;
  // 标识代码块的类型（如BlobType::InterpreterStub）
  BlobId _blob_id;
 protected:
  // 指向MacroAssembler实例，负责实际汇编指令生成
  MacroAssembler*  _masm;

 public:
  StubCodeGenerator(CodeBuffer* code, bool print_code = false);
  StubCodeGenerator(CodeBuffer* code, BlobId blob_id, bool print_code = false);
  ~StubCodeGenerator();

  MacroAssembler* assembler() const              { return _masm; }
  BlobId blob_id()                               { return _blob_id; }

  // 在存根代码的开始和结束时被调用，用于注册元数据
  virtual void stub_prolog(StubCodeDesc* cdesc); // called by StubCodeMark constructor
  virtual void stub_epilog(StubCodeDesc* cdesc); // called by StubCodeMark destructor

#ifdef ASSERT
  // 调试时验证存根ID的有效性
  void verify_stub(StubId stub_id);
#endif
};

// Stack-allocated helper class used to associate a stub code with a name.
// All stub code generating functions that use a StubCodeMark will be registered
// in the global StubCodeDesc list and the generated stub code can be identified
// later via an address pointing into it.
// 通过RAII机制关联存根代码与名称，简化StubCodeDesc的注册
class StubCodeMark: public StackObj {
 private:
  StubCodeGenerator* _cgen;
  StubCodeDesc*      _cdesc;

 public:
  // 构造时指定StubCodeGenerator、组名和存根名，自动生成StubCodeDesc并插入链表
  StubCodeMark(StubCodeGenerator* cgen, const char* group, const char* name);
  StubCodeMark(StubCodeGenerator* cgen, StubId stub_id);
  // 析构时自动清理资源
  ~StubCodeMark();

};

#endif // SHARE_RUNTIME_STUBCODEGENERATOR_HPP
