/*
 * Copyright (c) 1998, 2023, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SHARE_RUNTIME_JNIHANDLES_HPP
#define SHARE_RUNTIME_JNIHANDLES_HPP

#include "memory/allStatic.hpp"
#include "runtime/handles.hpp"

class JavaThread;
class OopStorage;
class Thread;

// Interface for creating and resolving local/global JNI handles
// 单例类，提供JNI句柄的创建、销毁、解析及类型检查功能
class JNIHandles : AllStatic {
  friend class VMStructs;
 private:
  // These are used by the serviceability agent.
  // 存储全局JNI句柄的OopStorage
  static OopStorage* _global_handles;
  // 存储弱全局JNI句柄的OopStorage
  static OopStorage* _weak_global_handles;
  friend void jni_handles_init();

  static OopStorage* global_handles();
  static OopStorage* weak_global_handles();

  inline static bool is_local_tagged(jobject handle);
  inline static bool is_weak_global_tagged(jobject handle);
  inline static bool is_global_tagged(jobject handle);
  inline static oop* local_ptr(jobject handle);
  inline static oop* global_ptr(jobject handle);
  inline static oop* weak_global_ptr(jweak handle);

  template <DecoratorSet decorators, bool external_guard> inline static oop resolve_impl(jobject handle);

  // Resolve handle into oop, without keeping the object alive
  inline static oop resolve_no_keepalive(jobject handle);

  // This method is not inlined in order to avoid circular includes between
  // this header file and thread.hpp.
  static bool current_thread_in_native();

 public:
  // Low tag bits in jobject used to distinguish its type. Checking
  // the underlying storage type is unsuitable for performance reasons.
  enum TypeTag {
    local = 0b00,
    weak_global = 0b01,
    global = 0b10,
  };

private:
  inline static bool is_tagged_with(jobject handle, TypeTag tag);

public:
  static const uintptr_t tag_size = 2;
  static const uintptr_t tag_mask = ((1u << tag_size) - 1u);

  STATIC_ASSERT((TypeTag::local & tag_mask) == TypeTag::local);
  STATIC_ASSERT((TypeTag::weak_global & tag_mask) == TypeTag::weak_global);
  STATIC_ASSERT((TypeTag::global & tag_mask) == TypeTag::global);

  // Resolve handle into oop
  // 将句柄解析为oop对象（可能触发垃圾回收）
  inline static oop resolve(jobject handle);
  // Resolve handle into oop, result guaranteed not to be null
  // 确保解析结果非空
  inline static oop resolve_non_null(jobject handle);
  // Resolve externally provided handle into oop with some guards
  // 外部调用时的安全解析
  static oop resolve_external_guard(jobject handle);

  // Check for equality without keeping objects alive
  static bool is_same_object(jobject handle1, jobject handle2);

  // Local handles
  // 为当前线程或指定线程创建本地句柄
  static jobject make_local(oop obj);
  static jobject make_local(JavaThread* thread, oop obj,  // Faster version when current thread is known
                            AllocFailType alloc_failmode = AllocFailStrategy::EXIT_OOM);
  // 销毁本地句柄
  inline static void destroy_local(jobject handle);

  // Global handles
  // 创建全局句柄
  static jobject make_global(Handle  obj,
                             AllocFailType alloc_failmode = AllocFailStrategy::EXIT_OOM);
  // 销毁全局句柄
  static void destroy_global(jobject handle);

  // Weak global handles
  // 创建弱全局句柄
  static jweak make_weak_global(Handle obj,
                                AllocFailType alloc_failmode = AllocFailStrategy::EXIT_OOM);
  // 销毁弱全局句柄
  static void destroy_weak_global(jweak handle);
  static bool is_weak_global_cleared(jweak handle); // Test jweak without resolution

  // Debugging
  static void print_on(outputStream* st);
  static void print();
  static void verify();
  // The category predicates all require handle != nullptr.
  static bool is_local_handle(JavaThread* thread, jobject handle);
  static bool is_frame_handle(JavaThread* thread, jobject handle);
  static bool is_global_handle(jobject handle);
  static bool is_weak_global_handle(jobject handle);

  // precondition: handle != nullptr.
  // 返回句柄类型（jobjectRefType）
  static jobjectRefType handle_type(JavaThread* thread, jobject handle);

  // Garbage collection support(global handles only, local handles are traversed from thread)
  // Traversal of regular global handles
  static void oops_do(OopClosure* f);
  // Traversal of weak global handles.
  static void weak_oops_do(OopClosure* f);

  static bool is_global_storage(const OopStorage* storage);
};



// JNI handle blocks holding local/global JNI handles
// 表示存储JNI句柄的内存块，支持动态分配和垃圾回收
class JNIHandleBlock : public CHeapObj<mtInternal> {
  friend class VMStructs;
  friend class ZeroInterpreter;

 private:
  enum SomeConstants {
    block_size_in_oops  = 32                    // Number of handles per handle block
  };

  // 固定大小（默认32个）的句柄数组
  uintptr_t       _handles[block_size_in_oops]; // The handles
  // 记录已分配的句柄数量
  int             _top;                         // Index of next unused handle
  int             _allocate_before_rebuild;     // Number of blocks to allocate before rebuilding free list
  // 指向下一个JNIHandleBlock，形成链表
  JNIHandleBlock* _next;                        // Link to next block

  // The following instance variables are only used by the first block in a chain.
  // Having two types of blocks complicates the code and the space overhead in negligible.
  // 首块记录最后一个使用的块（优化遍历）
  JNIHandleBlock* _last;                        // Last block in use
  JNIHandleBlock* _pop_frame_link;              // Block to restore on PopLocalFrame call
  // 空闲句柄链表（仅首块使用
  uintptr_t*      _free_list;                   // Handle free list

  static int      _blocks_allocated;            // For debugging/printing

  // Fill block with bad_handle values
  void zap() NOT_DEBUG_RETURN;

  // Free list computation
  void rebuild_free_list();

  // No more handles in the both the current and following blocks
  void clear() { _top = 0; }

 public:
  // Handle allocation
  // 从当前块分配句柄，溢出时自动扩展
  jobject allocate_handle(JavaThread* caller, oop obj, AllocFailType alloc_failmode = AllocFailStrategy::EXIT_OOM);

  // Block allocation and block free list management
  // 分配新块并加入链表
  static JNIHandleBlock* allocate_block(JavaThread* thread = nullptr, AllocFailType alloc_failmode = AllocFailStrategy::EXIT_OOM);
  // 释放不再使用的块
  static void release_block(JNIHandleBlock* block, JavaThread* thread = nullptr);

  // JNI PushLocalFrame/PopLocalFrame support
  JNIHandleBlock* pop_frame_link() const          { return _pop_frame_link; }
  void set_pop_frame_link(JNIHandleBlock* block)  { _pop_frame_link = block; }

  // Stub generator support
  static ByteSize top_offset()           { return byte_offset_of(JNIHandleBlock, _top); }

  // Garbage collection support
  // Traversal of handles
  void oops_do(OopClosure* f);

  // Debugging
  bool chain_contains(jobject handle) const;    // Does this block or following blocks contain handle
  bool contains(jobject handle) const;          // Does this block contain handle
};

#endif // SHARE_RUNTIME_JNIHANDLES_HPP
