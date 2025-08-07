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

#ifndef SHARE_GC_SHARED_MEMALLOCATOR_HPP
#define SHARE_GC_SHARED_MEMALLOCATOR_HPP

#include "memory/memRegion.hpp"
#include "oops/oopsHierarchy.hpp"
#include "runtime/javaThread.hpp"
#include "utilities/exceptions.hpp"
#include "utilities/globalDefinitions.hpp"
#include "utilities/macros.hpp"

// These fascilities are used for allocating, and initializing newly allocated objects.
// 分配内存&初始化工具，相当于是一个代理
class MemAllocator: StackObj { // 继承StackObj
protected: // 受保护成员（派生类可访问）
  class Allocation;
  // 成员变量
  Thread* const        _thread; // 业务线程（mutator）
  Klass* const         _klass; // 待实例化对象的类型

  // 字长，JVM 运行时的基本内存操作单元的大小，通常与底层硬件架构的位数直接相关。
  // 它决定了 JVM 中基本数据类型、对象头、指针等关键内存结构的存储方式，是 JVM 内存管理和性能优化的重要基础。
  // 32位JVM：word_size=4字节（32位）；64位JVM：word_size=8字节（64位）
  // 决定数据类型的存储大小；影响对象内存布局（对象头/数组长度）；控制内存对齐与填充；锁机制与内存占用；
  const size_t         _word_size;

  // Allocate from the current thread's TLAB, without taking a new TLAB (no safepoint).
  // 从TLAB中快速分配内存（如果没有TLAB不会重新创建）
 HeapWord* mem_allocate_inside_tlab_fast() const; // 带const：常量成员函数

private: // 私有成员变量和函数（没有显示声明访问修饰符时，默认为private）
  // Allocate in a TLAB. Could allocate a new TLAB, and therefore potentially safepoint.
  // 从TLAB中分配内存，如果没有TLAB会重新创建
  HeapWord* mem_allocate_inside_tlab_slow(Allocation& allocation) const;

  // Allocate outside a TLAB. Could safepoint.
  // 不使用TLAB，直接从堆中分配内存
  HeapWord* mem_allocate_outside_tlab(Allocation& allocation) const;

protected:
  MemAllocator(Klass* klass, size_t word_size, Thread* thread) // 构造函数
    : _thread(thread), // : 表示参数构造化
      _klass(klass),
      _word_size(word_size)
  {
    assert(_thread == Thread::current(), "must be");
  }

  // Initialization provided by subclasses.
  // 初始化
  virtual oop initialize(HeapWord* mem) const = 0; // virtual：纯虚函数（抽象函数）

  // This function clears the memory of the object.
  // 清除内存
  void mem_clear(HeapWord* mem) const;

  // This finish constructing an oop by installing the mark word and the Klass* pointer
  // last. At the point when the Klass pointer is initialized, this is a constructed object
  // that must be parseable as an oop by concurrent collectors.
  oop finish(HeapWord* mem) const;

  // Raw memory allocation. This will try to do a TLAB allocation, and otherwise fall
  // back to calling CollectedHeap::mem_allocate().
  // 内存分配函数，先尝试从TLAB中分配，尝试失败后从堆中分配
  HeapWord* mem_allocate(Allocation& allocation) const;

public: // 公共成员变量和函数
  // Allocate and fully construct the object, and perform various instrumentation. Could safepoint.
  // 内存分配&实例化对象
  oop allocate() const;
};

class ObjAllocator: public MemAllocator { // 对象的分配器
public:
  ObjAllocator(Klass* klass, size_t word_size, Thread* thread = Thread::current())
    : MemAllocator(klass, word_size, thread) {}

  virtual oop initialize(HeapWord* mem) const;
};

class ObjArrayAllocator: public MemAllocator { // 对象数组的分配器（需要数组长度等额外信息）
protected:
  const int  _length;
  const bool _do_zero;

  void mem_zap_start_padding(HeapWord* mem) const PRODUCT_RETURN;
  void mem_zap_end_padding(HeapWord* mem) const PRODUCT_RETURN;

public:
  ObjArrayAllocator(Klass* klass, size_t word_size, int length, bool do_zero,
                    Thread* thread = Thread::current())
    : MemAllocator(klass, word_size, thread),
      _length(length),
      _do_zero(do_zero) {}

  virtual oop initialize(HeapWord* mem) const;
};

class ClassAllocator: public MemAllocator { // 类对象分配器
public:
  ClassAllocator(Klass* klass, size_t word_size, Thread* thread = Thread::current())
    : MemAllocator(klass, word_size, thread) {}

  virtual oop initialize(HeapWord* mem) const;
};

// Manages a scope where a failed heap allocation results in
// suppression of JVMTI "resource exhausted" events and
// throwing a shared, backtrace-less OOME instance.
// Used for OOMEs that will not be propagated to user code.
class InternalOOMEMark: public StackObj { // JVM内部OOM消息封装
 private:
  bool _outer;
  JavaThread* _thread;

 public:
  explicit InternalOOMEMark(JavaThread* thread) {
    assert(thread != nullptr, "nullptr is not supported");
    _outer = thread->is_in_internal_oome_mark();
    thread->set_is_in_internal_oome_mark(true);
    _thread = thread;
  }

  ~InternalOOMEMark() { // ~开头：析构函数，用于释放动态分配的资源（如文件句柄）
    // Check that only InternalOOMEMark sets
    // JavaThread::_is_in_internal_oome_mark
    assert(_thread->is_in_internal_oome_mark(), "must be");
    _thread->set_is_in_internal_oome_mark(_outer);
  }

  JavaThread* thread() const  { return _thread; }
};

#endif // SHARE_GC_SHARED_MEMALLOCATOR_HPP
