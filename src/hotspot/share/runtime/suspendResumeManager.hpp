/*
 * Copyright (c) 2025, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SHARE_RUNTIME_SUSPENDRESUMEMANAGER_HPP
#define SHARE_RUNTIME_SUSPENDRESUMEMANAGER_HPP

class SuspendThreadHandshakeClosure;
class ThreadSelfSuspensionHandshakeClosure;

// 用于管理 Java 线程挂起与恢复 的核心组件，支持同步/异步挂起操作，并与虚拟线程（vthread）注册机制协同工作。
class SuspendResumeManager {
  friend SuspendThreadHandshakeClosure;
  friend ThreadSelfSuspensionHandshakeClosure;
  friend JavaThread;

  // 目标线程对象，即被管理的 Java 线程实例
  JavaThread* _target;
  // 同步锁对象，用于保护状态变更时的线程安全
  Monitor* _state_lock;

  // 初始化目标线程和同步锁
  SuspendResumeManager(JavaThread* thread, Monitor* state_lock);

  // This flag is true when the thread owning this
  // SuspendResumeManager (the _target) is suspended.
  // 标记目标线程是否处于挂起状态（true表示挂起）
  volatile bool _suspended;
  // This flag is true while there is async handshake (trap)
  // on queue. Since we do only need one, we can reuse it if
  // thread gets suspended again (after a resume)
  // and we have not yet processed it.
  // 标记是否存在待处理的 异步挂起请求（如陷阱指令触发的挂起）
  bool _async_suspend_handshake;

  // 尝试挂起目标线程
  bool suspend(bool register_vthread_SR);
  // 恢复目标线程的执行，同样支持虚拟线程状态注册
  bool resume(bool register_vthread_SR);

  // Called from the async handshake (the trap)
  // to stop a thread from continuing execution when suspended.
  // 在异步挂起握手时被调用，直接终止目标线程的执行
  void do_owner_suspend();

  // Called from the suspend handshake.
  // 实际执行挂起的逻辑，结合 同步握手协议
  bool suspend_with_handshake(bool register_vthread_SR);

  // 原子性设置 _suspended标志，并处理虚拟线程注册逻辑
  void set_suspended(bool to, bool register_vthread_SR);

  bool is_suspended() {
    return Atomic::load(&_suspended);
  }

  // 查询/设置异步挂起请求标志
  bool has_async_suspend_handshake() { return _async_suspend_handshake; }
  void set_async_suspend_handshake(bool to) { _async_suspend_handshake = to; }
};

#endif // SHARE_RUNTIME_SUSPENDRESUMEMANAGER_HPP
