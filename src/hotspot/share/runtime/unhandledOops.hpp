/*
 * Copyright (c) 2005, 2023, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SHARE_RUNTIME_UNHANDLEDOOPS_HPP
#define SHARE_RUNTIME_UNHANDLEDOOPS_HPP

#ifdef CHECK_UNHANDLED_OOPS

// Detect unhanded oops in VM code

// The design is that when an oop is declared on the stack as a local
// variable, the oop is actually a C++ struct with constructor and
// destructor.  The constructor adds the oop address on a list
// off each thread and the destructor removes the oop.  At a potential
// safepoint, the stack addresses of the local variable oops are trashed
// with a recognizable value.  If the local variable is used again, it
// will segfault, indicating an unsafe use of that oop.
// eg:
//    oop o;    //register &o on list
//    funct();  // if potential safepoint - causes clear_naked_oops()
//              // which trashes o above.
//    o->do_something();  // Crashes because o is unsafe.
//
// This code implements the details of the unhandled oop list on the thread.
//
// 检测未处理oop：在JVM运行过程中，若栈上的oop引用在安全点后被错误使用（如GC后对象已移动但栈上仍有残留引用），会导致未定义行为。
// 该机制通过覆盖无效oop地址，在后续非法访问时触发崩溃，从而暴露问题。
// 实现机制
// 栈上局部变量封装：
// 开发者在栈上声明oop变量时，实际创建UnhandledOopEntry实例，其构造函数将oop地址注册到当前线程的UnhandledOops列表。
// 变量离开作用域时，析构函数自动注销oop。
// 安全点处理：
// JVM在安全点（如GC前）调用clear_unhandled_oops()，遍历所有线程的_oop_list，将其中_oop_ptr指向的地址覆盖为BAD_OOP_ADDR（如0xfffffff1）。
// 若后续代码错误使用该oop（如通过悬空指针访问），会立即触发内存访问异常（如Segfault），便于调试定位问题。
class oop;
class Thread;

// 表示单个未处理oop的记录项
class UnhandledOopEntry : public CHeapObj<mtThread> {
 friend class UnhandledOops;
 private:
  // 指向oop的指针
  oop* _oop_ptr;
  // 标记是否允许GC处理该oop（默认false）
  bool _ok_for_gc;

  // 匹配指定oop指针是否属于当前记录
  bool match_oop_entry(oop* op) const {
    return _oop_ptr == op;
  }

 public:
  UnhandledOopEntry() : _oop_ptr(nullptr), _ok_for_gc(false) {}
  UnhandledOopEntry(oop* op) :
                        _oop_ptr(op),   _ok_for_gc(false) {}
};

// 管理线程内所有未处理oop的列表
class UnhandledOops : public CHeapObj<mtThread> {
 friend class Thread;
 private:
  // 所属线程
  Thread* _thread;
  // 记录嵌套层级（可能与异常处理或同步块相关）
  int _level;
  // 动态数组，存储UnhandledOopEntry实例
  GrowableArray<UnhandledOopEntry> *_oop_list;
  void allow_unhandled_oop(oop* op);
  // 安全点时触发，覆盖所有oop地址为BAD_OOP_ADDR
  void clear_unhandled_oops();
  UnhandledOops(Thread* thread);
  ~UnhandledOops();

 public:
  // 静态方法，输出所有未处理oop信息
  static void dump_oops(UnhandledOops* list);
  // 将oop加入列表
  void register_unhandled_oop(oop* op);
  // 从列表移除oop
  void unregister_unhandled_oop(oop* op);
};

#ifdef _LP64
const intptr_t BAD_OOP_ADDR =  0xfffffffffffffff1;
#else
const intptr_t BAD_OOP_ADDR =  0xfffffff1;
#endif // _LP64
#endif // CHECK_UNHANDLED_OOPS

#endif // SHARE_RUNTIME_UNHANDLEDOOPS_HPP
