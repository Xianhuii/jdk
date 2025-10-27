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

#ifndef SHARE_RUNTIME_SAFEPOINT_HPP
#define SHARE_RUNTIME_SAFEPOINT_HPP

#include "memory/allStatic.hpp"
#include "runtime/javaThread.hpp"
#include "runtime/os.hpp"
#include "runtime/vmOperation.hpp"
#include "utilities/ostream.hpp"
#include "utilities/waitBarrier.hpp"

// JVM中安全点（SafePoint）同步机制的核心实现，主要用于协调JVM线程在全局操作（如垃圾回收）时的暂停与恢复。
// 确保所有Java线程在特定时刻（如GC）暂停执行，进入安全状态，避免CPU执行敏感操作（如对象引用修改），以保证全局操作的原子性和一致性。
//
// Safepoint synchronization
////
// The VMThread uses the SafepointSynchronize::begin/end
// methods to enter/exit a safepoint region. The begin method will roll
// all JavaThreads forward to a safepoint.
//
// JavaThreads must use the ThreadSafepointState abstraction (defined in
// thread.hpp) to indicate that that they are at a safepoint.
//
// The Mutex/Condition variable and ObjectLocker classes calls the enter/
// exit safepoint methods, when a thread is blocked/restarted. Hence, all mutex exter/
// exit points *must* be at a safepoint.

class ThreadSafepointState;

// 通过safepoint_id和at_safepoint标志，辅助判断线程是否需要响应安全点请求
class SafepointStateTracker {
  uint64_t _safepoint_id;
  bool     _at_safepoint;
public:
  SafepointStateTracker(uint64_t safepoint_id, bool at_safepoint);
  bool safepoint_state_changed();
};

//
// Implements roll-forward to safepoint (safepoint synchronization)
//
class SafepointSynchronize : AllStatic {
 public:
  enum SynchronizeState {
      _not_synchronized = 0,                   // Threads not synchronized at a safepoint. Keep this value 0.
      _synchronizing    = 1,                   // Synchronizing in progress
      _synchronized     = 2                    // All Java threads are running in native, blocked in OS or stopped at safepoint.
                                               // VM thread and any NonJavaThread may be running.
  };

 private:
  friend class SafepointMechanism;
  friend class ThreadSafepointState;
  friend class HandshakeState;
  friend class SafepointStateTracker;

  // Threads might read this flag directly, without acquiring the Threads_lock:
  // 跟踪同步状态（未同步、同步中、已同步）
  static volatile SynchronizeState _state;
  // Number of threads we are waiting for to block:
  // 等待阻塞的线程数
  static int              _waiting_to_block;
  // Counts the number of active critical natives during the safepoint:
  // 记录安全点期间活动的JNI原生方法数量
  static int              _current_jni_active_count;

  // This counter is used for fast versions of jni_Get<Primitive>Field.
  // An even value means there are no ongoing safepoint operations.
  // The counter is incremented ONLY at the beginning and end of each
  // safepoint.
  static volatile uint64_t _safepoint_counter;

  // A change in this counter or a change in the result of
  // is_at_safepoint() are used by SafepointStateTracker::
  // safepoint_state_changed() to determine its answer.
  static uint64_t _safepoint_id;

  // JavaThreads that need to block for the safepoint will stop on the
  // _wait_barrier, where they can quickly be started again.
  static WaitBarrier* _wait_barrier;
  static julong       _coalesced_vmop_count;     // coalesced vmop count

  // For debug long safepoint
  static void print_safepoint_timeout();

  // Helper methods for safepoint procedure:
  static void arm_safepoint();
  // 实际执行线程同步逻辑，通过条件变量和轮询机制阻塞线程
  static int synchronize_threads(jlong safepoint_limit_time, int nof_threads, int* initial_running);
  static void disarm_safepoint();
  static void increment_jni_active_count();
  static void decrement_waiting_to_block();
  static bool thread_not_running(ThreadSafepointState *cur_state);

  // Used in safepoint_safe to do a stable load of the thread state.
  static bool try_stable_load_state(JavaThreadState *state,
                                    JavaThread *thread,
                                    uint64_t safepoint_count);

  // Called when a thread voluntarily blocks
  static void block(JavaThread *thread);

  // Called from VMThread during handshakes.
  // If true the VMThread may safely process the handshake operation for the JavaThread.
  static bool handshake_safe(JavaThread *thread);

  static uint64_t safepoint_counter()             { return _safepoint_counter; }

public:

  static void init(Thread* vmthread);

  // Roll all threads forward to safepoint. Must be called by the VMThread.
  // 触发安全点同步，使所有Java线程进入安全点
  static void begin();
  // 恢复所有被阻塞的线程
  static void end();                    // Start all suspended threads again...

  // The value for a not set safepoint id.
  static const uint64_t InactiveSafepointCounter;

  // Query
  // 判断当前是否处于安全点
  static bool is_at_safepoint()                   { return _state == _synchronized; }
  static bool is_synchronizing()                  { return _state == _synchronizing; }

  static uint64_t safepoint_id() {
    return _safepoint_id;
  }

  static SafepointStateTracker safepoint_state_tracker() {
    return SafepointStateTracker(safepoint_id(), is_at_safepoint());
  }

  // Exception handling for page polling
  static void handle_polling_page_exception(JavaThread *thread);

  static void set_is_at_safepoint()             { _state = _synchronized; }
  static void set_is_not_at_safepoint()         { _state = _not_synchronized; }

  // Only used for making sure that no safepoint has happened in
  // JNI_FastGetField. Therefore only the low 32-bits are needed
  // even if this is a 64-bit counter.
  static address safepoint_counter_addr() {
#ifdef VM_LITTLE_ENDIAN
    return (address)&_safepoint_counter;
#else /* BIG */
    // Return pointer to the 32 LSB:
    return (address) (((uint32_t*)(&_safepoint_counter)) + 1);
#endif
  }
};

// Some helper assert macros for safepoint checks.

#define assert_at_safepoint()                                           \
  assert(SafepointSynchronize::is_at_safepoint(), "should be at a safepoint")

#define assert_at_safepoint_msg(...)                                    \
  assert(SafepointSynchronize::is_at_safepoint(), __VA_ARGS__)

#define assert_not_at_safepoint()                                       \
  assert(!SafepointSynchronize::is_at_safepoint(), "should not be at a safepoint")

#define assert_not_at_safepoint_msg(...)                                \
  assert(!SafepointSynchronize::is_at_safepoint(), __VA_ARGS__)

// State class for a thread suspended at a safepoint
// 线程状态跟踪
class ThreadSafepointState: public CHeapObj<mtThread> {
 private:
  // At polling page safepoint (NOT a poll return safepoint):
  // 标识线程是否因安全点轮询而暂停
  volatile bool                   _at_poll_safepoint;
  JavaThread*                     _thread;
  // 表示线程已安全抵达安全点
  bool                            _safepoint_safe;
  // 关联到当前安全点实例的ID，用于状态验证
  volatile uint64_t               _safepoint_id;

  ThreadSafepointState*           _next;

  void account_safe_thread();

 public:
  ThreadSafepointState(JavaThread *thread);

  // Linked list support:
  ThreadSafepointState* get_next() const { return _next; }
  void set_next(ThreadSafepointState* value) { _next = value; }
  ThreadSafepointState** next_ptr() { return &_next; }

  // examine/restart
  // 检查线程是否满足安全点条件
  void examine_state_of_thread(uint64_t safepoint_count);
  // 恢复线程执行
  void restart();

  // Query
  JavaThread*  thread() const         { return _thread; }
  bool         is_running() const     { return !_safepoint_safe; }

  uint64_t get_safepoint_id() const;
  void     reset_safepoint_id();
  void     set_safepoint_id(uint64_t sid);

  // Support for safepoint timeout (debugging)
  bool is_at_poll_safepoint()           { return _at_poll_safepoint; }
  void set_at_poll_safepoint(bool val)  { _at_poll_safepoint = val; }

  // 处理安全点轮询页面触发的异常
  void handle_polling_page_exception();

  // debugging
  void print_on(outputStream* st) const;

  // Initialize
  static void create(JavaThread *thread);
  static void destroy(JavaThread *thread);
};

// 统计与日志
class SafepointTracing : public AllStatic {
private:
  // Absolute
  static jlong _last_safepoint_begin_time_ns;
  static jlong _last_safepoint_sync_time_ns;
  static jlong _last_safepoint_leave_time_ns;
  static jlong _last_safepoint_end_time_ns;

  // Relative
  static jlong _last_app_time_ns;

  static int _nof_threads;
  static int _nof_running;
  static int _page_trap;

  static VM_Operation::VMOp_Type _current_type;
  static jlong     _max_sync_time;
  static jlong     _max_vmop_time;
  static uint64_t  _op_count[VM_Operation::VMOp_Terminating];

  static void statistics_log();

public:
  static void init();

  static void begin(VM_Operation::VMOp_Type type);
  static void synchronized(int nof_threads, int nof_running, int traps);
  static void leave();
  static void end();

  static void statistics_exit_log();

  static jlong time_since_last_safepoint_ms() {
    return nanos_to_millis(os::javaTimeNanos() - _last_safepoint_end_time_ns);
  }

  static jlong end_of_last_safepoint_ms() {
    return nanos_to_millis(_last_safepoint_end_time_ns);
  }

  static jlong start_of_safepoint() {
    return _last_safepoint_begin_time_ns;
  }
};

#endif // SHARE_RUNTIME_SAFEPOINT_HPP
