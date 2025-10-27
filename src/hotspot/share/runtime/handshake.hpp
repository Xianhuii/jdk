/*
 * Copyright (c) 2017, 2025, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SHARE_RUNTIME_HANDSHAKE_HPP
#define SHARE_RUNTIME_HANDSHAKE_HPP

#include "memory/allStatic.hpp"
#include "memory/iterator.hpp"
#include "runtime/flags/flagSetting.hpp"
#include "runtime/mutex.hpp"
#include "runtime/orderAccess.hpp"
#include "utilities/filterQueue.hpp"

class HandshakeOperation;
class AsyncHandshakeOperation;
class JavaThread;
class UnsafeAccessErrorHandshakeClosure;
class ThreadsListHandle;

// 处理线程间协作和安全点操作的核心组件，主要用于在安全点（Safepoint）或线程处于可协作状态时执行回调操作
// 线程协作流程：
// 1. 发起握手：调用Handshake::execute()，将操作加入目标线程的HandshakeState队列，并确保目标线程处于安全状态。
// 2. 目标线程处理：
// 2.1 当目标线程进入安全点时，检查并执行队列中的操作。
// 2.2 同步操作（HandshakeClosure）由发起线程或VMThread完成；异步操作（AsyncHandshakeClosure）立即返回。
// 3. 状态同步：使用_lock和_active_handshaker确保操作的原子性和互斥性，避免竞态条件。

// A handshake closure is a callback that is executed for a JavaThread
// while it is in a safepoint/handshake-safe state. Depending on the
// nature of the closure, the callback may be executed by the initiating
// thread, the target thread, or the VMThread. If the callback is not executed
// by the target thread it will remain in a blocked state until the callback completes.
// 作为回调接口，封装需要在目标线程安全状态下执行的操作
// 同步执行，由发起线程或VMThread完成操作后唤醒目标线程
class HandshakeClosure : public ThreadClosure, public CHeapObj<mtThread> {
  const char* const _name;
 public:
  HandshakeClosure(const char* name) : _name(name) {}
  virtual ~HandshakeClosure()                      {}
  const char* name() const                         { return _name; }
  virtual bool is_async()                          { return false; }
  virtual bool is_suspend()                        { return false; }
  virtual bool is_async_exception()                { return false; }
  virtual void do_thread(Thread* thread) = 0;
};
// 异步执行，不等待目标线程完成，直接返回
class AsyncHandshakeClosure : public HandshakeClosure {
 public:
   AsyncHandshakeClosure(const char* name) : HandshakeClosure(name) {}
   virtual ~AsyncHandshakeClosure() {}
   virtual bool is_async()          { return true; }
};

// 握手管理器，简化握手操作的调用，隐藏线程同步细节
class Handshake : public AllStatic {
 public:
  // Execution of handshake operation
  // 执行同步握手操作，隐式使用当前线程列表保护目标
  static void execute(HandshakeClosure*       hs_cl);
  // This version of execute() relies on a ThreadListHandle somewhere in
  // the caller's context to protect target (and we sanity check for that).
  // 指定目标线程，需外部保证线程列表保护
  static void execute(HandshakeClosure*       hs_cl, JavaThread* target);
  // This version of execute() is used when you have a ThreadListHandle in
  // hand and are using it to protect target. If tlh == nullptr, then we
  // sanity check for a ThreadListHandle somewhere in the caller's context
  // to verify that target is protected.
  static void execute(HandshakeClosure*       hs_cl, ThreadsListHandle* tlh, JavaThread* target);
  // This version of execute() relies on a ThreadListHandle somewhere in
  // the caller's context to protect target (and we sanity check for that).
  // 执行异步握手，立即返回
  static void execute(AsyncHandshakeClosure*  hs_cl, JavaThread* target);
};

class JvmtiRawMonitor;

// The HandshakeState keeps track of an ongoing handshake for this JavaThread.
// VMThread/Handshaker and JavaThread are serialized with _lock making sure the
// operation is only done by either VMThread/Handshaker on behalf of the
// JavaThread or by the target JavaThread itself.
// 握手状态，每个JavaThread关联一个HandshakeState实例
class HandshakeState {
  friend UnsafeAccessErrorHandshakeClosure;
  friend JavaThread;
  // This a back reference to the JavaThread,
  // the target for all operation in the queue.
  JavaThread* _handshakee;
  // The queue containing handshake operations to be performed on _handshakee.
  // 存储待处理的HandshakeOperation队列
  FilterQueue<HandshakeOperation*> _queue;
  // Provides mutual exclusion to this state and queue. Also used for
  // JavaThread suspend/resume operations performed by SuspendResumeManager.
  // 保护队列和状态的互斥锁
  Monitor _lock;
  // Set to the thread executing the handshake operation.
  // 记录当前执行操作的线程
  Thread* volatile _active_handshaker;

  bool claim_handshake();
  bool possibly_can_process_handshake();
  bool can_process_handshake();

  bool have_non_self_executable_operation();
  HandshakeOperation* get_op_for_self(bool allow_suspend, bool check_async_exception);
  HandshakeOperation* get_op();
  void remove_op(HandshakeOperation* op);

  void set_active_handshaker(Thread* thread) { Atomic::store(&_active_handshaker, thread); }

  class MatchOp {
    HandshakeOperation* _op;
   public:
    MatchOp(HandshakeOperation* op) : _op(op) {}
    bool operator()(HandshakeOperation* op) {
      return op == _op;
    }
  };

 public:
  HandshakeState(JavaThread* thread);
  ~HandshakeState();

  void add_operation(HandshakeOperation* op);

  bool has_operation() { return !_queue.is_empty(); }
  bool has_operation(bool allow_suspend, bool check_async_exception);
  bool has_async_exception_operation();
  void clean_async_exception_operation();

  bool operation_pending(HandshakeOperation* op);

  // If the method returns true we need to check for a possible safepoint.
  // This is due to a suspension handshake which put the JavaThread in blocked
  // state so a safepoint may be in-progress.
  bool process_by_self(bool allow_suspend, bool check_async_exception);

  enum ProcessResult {
    _no_operation = 0,
    _not_safe,
    _claim_failed,
    _processed,
    _succeeded,
    _number_states
  };
  ProcessResult try_process(HandshakeOperation* match_op);

  Thread* active_handshaker() const { return Atomic::load(&_active_handshaker); }

  // Support for asynchronous exceptions
 private:
  bool _async_exceptions_blocked;

  bool async_exceptions_blocked() { return _async_exceptions_blocked; }
  void set_async_exceptions_blocked(bool b) { _async_exceptions_blocked = b; }
  void handle_unsafe_access_error();
};
#endif // SHARE_RUNTIME_HANDSHAKE_HPP
