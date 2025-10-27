/*
 * Copyright (c) 1997, 2024, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2021, Azul Systems, Inc. All rights reserved.
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

#ifndef SHARE_RUNTIME_THREADS_HPP
#define SHARE_RUNTIME_THREADS_HPP

#include "jni.h"
#include "utilities/exceptions.hpp"
#include "utilities/globalDefinitions.hpp"
#include "utilities/macros.hpp"

class JavaThread;
class Metadata;
class MetadataClosure;
class OopClosure;
class Thread;
class ThreadClosure;
class ThreadsList;
class outputStream;

// The active thread queue. It also keeps track of the current used
// thread priorities.
// 所有成员为静态的单例类，负责 JVM 中线程的全局管理
class Threads: AllStatic {
  friend class VMStructs;
 private:
  // 当前活动线程总数
  static int         _number_of_threads;
  // 非守护线程数量
  static int         _number_of_non_daemon_threads;
  // JVM 初始化/销毁操作的返回码
  static int         _return_code;
  // 线程认领令牌，用于并行任务中避免重复处理同一线程
  static uintx       _thread_claim_token;
#ifdef ASSERT
  // 标记 JVM 是否已完成初始化（通过 ASSERT宏控制）
  static bool        _vm_complete;
#endif

  static void initialize_java_lang_classes(JavaThread* main_thread, TRAPS);
  static void initialize_jsr292_core_classes(TRAPS);

 public:
  // Thread management
  // force_daemon is a concession to JNI, where we may need to add a
  // thread to the thread list before allocating its thread object
  // 将 JavaThread对象加入活动线程列表。force_daemon允许在 JNI 场景下提前注册未完全初始化的线程
  static void add(JavaThread* p, bool force_daemon = false);
  // 从活动线程列表移除线程，并更新守护线程计数
  static void remove(JavaThread* p, bool is_daemon);
  // 对非 Java 线程（如 VM 内部线程）执行闭包操作
  static void non_java_threads_do(ThreadClosure* tc);
  // 对 Java 线程（如用户创建的线程）执行闭包操作
  static void java_threads_do(ThreadClosure* tc);
  // 同时遍历 Java 和非 Java 线程
  static void threads_do(ThreadClosure* tc);
  // 根据 is_par参数决定是否并行执行线程遍历
  static void possibly_parallel_threads_do(bool is_par, ThreadClosure* tc);

  // Initializes the vm and creates the vm thread
  // 初始化 JVM，解析参数并创建 VM 线程，返回状态码
  static jint create_vm(JavaVMInitArgs* args, bool* canTryAgain);
  // 将初始化阶段的库转换为代理（Agent）模式
  static void convert_vm_init_libraries_to_agents();
  static void create_vm_init_libraries();
  static void create_vm_init_agents();
  // 销毁 VM 代理
  static void shutdown_vm_agents();
  // 销毁整个 JVM 实例
  static void destroy_vm();
  // Supported VM versions via JNI
  // Includes JNI_VERSION_1_1
  // 检查 JNI 版本是否被支持（包含 JNI 1.1）
  static jboolean is_supported_jni_version_including_1_1(jint version);
  // Does not include JNI_VERSION_1_1
  // 检查 JNI 版本是否被支持（不包含 JNI 1.1）
  static jboolean is_supported_jni_version(jint version);

private:
  // The "thread claim token" provides a way for threads to be claimed
  // by parallel worker tasks.
  //
  // Each thread contains a "token" field. A task will claim the
  // thread only if its token is different from the global token,
  // which is updated by calling change_thread_claim_token().  When
  // a thread is claimed, it's token is set to the global token value
  // so other threads in the same iteration pass won't claim it.
  //
  // For this to work change_thread_claim_token() needs to be called
  // exactly once in sequential code before starting parallel tasks
  // that should claim threads.
  //
  // New threads get their token set to 0 and change_thread_claim_token()
  // never sets the global token to 0.
  // 获取当前全局认领令牌
  static uintx thread_claim_token() { return _thread_claim_token; }

public:
  // 更新令牌以触发新一轮认领
  static void change_thread_claim_token();
  // 断言所有线程已被认领（仅调试模式）
  static void assert_all_threads_claimed() NOT_DEBUG_RETURN;

  // Apply "f->do_oop" to all root oops in all threads.
  // This version may only be called by sequential code.
  // 遍历所有线程的根对象（普通对象指针），用于垃圾回收
  static void oops_do(OopClosure* f, NMethodClosure* cf);
  // This version may be called by sequential or parallel code.
  // 并行版本的根对象扫描
  static void possibly_parallel_oops_do(bool is_par, OopClosure* f, NMethodClosure* cf);

  // RedefineClasses support
  // 对所有线程的元数据（如类、方法等）执行闭包操作
  static void metadata_do(MetadataClosure* f);
  // 处理元数据句柄的闭包操作
  static void metadata_handles_do(void f(Metadata*));

#ifdef ASSERT
  static bool is_vm_complete() { return _vm_complete; }
#endif // ASSERT

  // Verification
  static void verify();
  static void print_on(outputStream* st, bool print_stacks, bool internal_format, bool print_concurrent_locks, bool print_extended_info);
  static void print(bool print_stacks, bool internal_format) {
    // this function is only used by debug.cpp
    print_on(tty, print_stacks, internal_format, false /* no concurrent lock printed */, false /* simple format */);
  }
  static void print_on_error(outputStream* st, Thread* current, char* buf, int buflen);
  static void print_on_error(Thread* this_thread, outputStream* st, Thread* current, char* buf,
                             int buflen, bool* found_current);
  // Print threads busy compiling, and returns the number of printed threads.
  static unsigned print_threads_compiling(outputStream* st, char* buf, int buflen, bool short_form = false);

  // Get Java threads that are waiting to enter or re-enter the specified monitor.
  // Java threads that are executing mounted virtual threads are not included.
  // 获取等待进入监视器的 Java 线程列表
  static GrowableArray<JavaThread*>* get_pending_threads(ThreadsList * t_list,
                                                         int count, address monitor);

  // Get owning Java thread from the basicLock address.
  // 从锁地址或对象反向查找持有锁的 Java 线程
  static JavaThread *owning_thread_from_stacklock(ThreadsList * t_list, address basicLock);

  static JavaThread* owning_thread_from_object(ThreadsList* t_list, oop obj);
  static JavaThread* owning_thread_from_monitor(ThreadsList* t_list, ObjectMonitor* owner);

  // Number of threads on the active threads list
  static int number_of_threads()                 { return _number_of_threads; }
  // Number of non-daemon threads on the active threads list
  static int number_of_non_daemon_threads()      { return _number_of_non_daemon_threads; }

  struct Test;                  // For private gtest access.
};

#endif // SHARE_RUNTIME_THREADS_HPP
