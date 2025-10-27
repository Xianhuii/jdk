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

#ifndef SHARE_RUNTIME_SUSPENDEDTHREADTASK_HPP
#define SHARE_RUNTIME_SUSPENDEDTHREADTASK_HPP

class Thread;

// 封装线程挂起时的上下文信息
class SuspendedThreadTaskContext {
 private:
  // 指向被挂起线程的 Thread对象指针
  Thread* _thread;
  // 保存线程上下文的通用指针（如寄存器状态、栈信息等）
  void* _ucontext;
 public:
  SuspendedThreadTaskContext(Thread* thread, void* ucontext) : _thread(thread), _ucontext(ucontext) {}
  Thread* thread() const { return _thread; }
  void* ucontext() const { return _ucontext; }
};

// 抽象基类，定义在线程挂起时执行任务的框架
class SuspendedThreadTask {
 private:
  // 指向目标线程的指针
  Thread* _thread;
  // 纯虚函数的辅助方法，实际逻辑在派生类中实现
  void internal_do_task();
 protected:
  ~SuspendedThreadTask() {}
 public:
  SuspendedThreadTask(Thread* thread) : _thread(thread) {}
  // 模板方法，调用 internal_do_task()执行任务
  void run() { internal_do_task(); }
  // 纯虚函数，需派生类实现具体任务逻辑
  virtual void do_task(const SuspendedThreadTaskContext& context) = 0;
};

#endif // SHARE_RUNTIME_SUSPENDEDTHREADTASK_HPP
