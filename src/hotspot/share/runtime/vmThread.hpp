/*
 * Copyright (c) 1998, 2024, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SHARE_RUNTIME_VMTHREAD_HPP
#define SHARE_RUNTIME_VMTHREAD_HPP

#include "runtime/atomic.hpp"
#include "runtime/javaThread.hpp"
#include "runtime/perfDataTypes.hpp"
#include "runtime/nonJavaThread.hpp"
#include "runtime/task.hpp"
#include "runtime/vmOperation.hpp"

// JVM（Java虚拟机）中VMThread的核心实现，负责执行虚拟机级别的重量级操作
// VM operation timeout handling: warn or abort the VM when VM operation takes
// too long. Periodic tasks do not participate in safepoint protocol, and therefore
// can fire when application threads are stopped.

// 超时监控：周期性检查VM操作是否超时
class VMOperationTimeoutTask : public PeriodicTask {
private:
  // 标记是否激活超时检测
  volatile int _armed;
  // 超时检测启动时间
  jlong _arm_time;
  // 当前操作的名称
  const char* _vm_op_name;
public:
  VMOperationTimeoutTask(size_t interval_time) :
          PeriodicTask(interval_time), _armed(0), _arm_time(0), _vm_op_name(nullptr) {}

  // 周期性执行的任务逻辑
  virtual void task();

  // 检查是否处于监控状态
  bool is_armed();
  // 启动/停止超时监控
  void arm(const char* vm_op_name);
  void disarm();
};

//
// A single VMThread is used by other threads to offload heavy vm operations
// like scavenge, garbage_collect etc.
//

class VMThread: public NamedThread {
 private:
  volatile bool _is_running;

  static ThreadPriority _current_priority;

  static bool _should_terminate;
  static bool _terminated;
  static Monitor * _terminate_lock;
  static PerfCounter* _perf_accumulated_vm_operation_time;

  static VMOperationTimeoutTask* _timeout_task;

  static bool handshake_or_safepoint_alot();

  // 评估并执行具体操作（如GC、线程栈处理等）
  void evaluate_operation(VM_Operation* op);
  void inner_execute(VM_Operation* op);
  void wait_for_operation();

  // Constructor
  VMThread();

  // No destruction allowed
  ~VMThread() {
    guarantee(false, "VMThread deletion must fix the race with VM termination");
  }

  // The ever running loop for the VMThread
  // 主线程循环，不断从队列中取出并执行操作
  void loop();

 public:
  bool is_running() const { return Atomic::load(&_is_running); }

  // Tester
  bool is_VM_thread() const                      { return true; }

  // Called to stop the VM thread
  // 等待VMThread安全退出
  static void wait_for_vm_thread_exit();
  // 检查终止状态
  static bool should_terminate()                  { return _should_terminate; }
  static bool is_terminated()                     { return _terminated == true; }

  // Execution of vm operation
  // 提交新操作到队列
  static void execute(VM_Operation* op);

  // Returns the current vm operation if any.
  static VM_Operation* vm_operation()             {
    assert(Thread::current()->is_VM_thread(), "Must be");
    return _cur_vm_operation;
  }

  static VM_Operation::VMOp_Type vm_op_type()     {
    VM_Operation* op = vm_operation();
    assert(op != nullptr, "sanity");
    return op->type();
  }

  // Returns the single instance of VMThread.
  static VMThread* vm_thread()                    { return _vm_thread; }

  void verify();

  // Performance measurement
  static PerfCounter* perf_accumulated_vm_operation_time() {
    return _perf_accumulated_vm_operation_time;
  }

  // Entry for starting vm thread
  // 线程入口，启动主循环loop()
  virtual void run();

  // Creations/Destructions
  // 创建和销毁VMThread
  static void create();
  static void destroy();

  static void wait_until_executed(VM_Operation* op);

  // Printing
  const char* type_name() const { return "VMThread"; }

 private:
  // VM_Operation support
  // 当前正在执行的VM操作
  static VM_Operation*     _cur_vm_operation;   // Current VM operation
  // 待处理的下一操作
  static VM_Operation*     _next_vm_operation;  // Next VM operation

  bool set_next_operation(VM_Operation *op);    // Set the _next_vm_operation if possible.

  // Pointer to single-instance of VM thread
  // 通过静态成员_vm_thread维护唯一的VMThread实例
  static VMThread*     _vm_thread;
};

#endif // SHARE_RUNTIME_VMTHREAD_HPP
