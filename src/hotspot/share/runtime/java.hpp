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

#ifndef SHARE_RUNTIME_JAVA_HPP
#define SHARE_RUNTIME_JAVA_HPP

#include "runtime/os.hpp"
#include "utilities/globalDefinitions.hpp"

class Handle;
class JavaThread;
class Symbol;

// 提供多种退出策略，确保资源释放和状态处理的健壮性

// Execute code before all handles are released and thread is killed; prologue to vm_exit
// 在所有资源释放前执行预处理逻辑（如清理Handles），作为vm_exit的前置钩子
extern void before_exit(JavaThread * thread, bool halt = false);

// Forced VM exit (i.e, internal error or JVM_Exit)
// 强制终止JVM，根据code返回退出状态。若halt=true，会立即停止进程
extern void vm_exit(int code);

// Wrapper for ::exit()
extern void vm_direct_exit(int code);
extern void vm_direct_exit(int code, const char* message);

// Shutdown the VM but do not exit the process
// 关闭JVM但不退出进程（如保留服务进程）
extern void vm_shutdown();
// Shutdown the VM and abort the process
// 终止进程并可选生成核心转储文件（用于调试）
extern void vm_abort(bool dump_core=true);

// Trigger any necessary notification of the VM being shutdown
extern void notify_vm_shutdown();

// VM exit if error occurs during initialization of VM
// vm_exit_during_initialization()系列函数在JVM启动阶段检测到致命错误时触发退出，支持传递异常信息或错误描述
extern void vm_exit_during_initialization();
extern void vm_exit_during_initialization(Handle exception);
extern void vm_exit_during_initialization(Symbol* exception_name, const char* message);
extern void vm_exit_during_initialization(const char* error, const char* message = nullptr);
extern void vm_shutdown_during_initialization(const char* error, const char* message = nullptr);

extern void vm_exit_during_cds_dumping(const char* error, const char* message = nullptr);

// This is defined in linkType.cpp due to linking restraints
extern bool is_vm_statically_linked();

/**
 * With the integration of the changes to handle the version string
 * as defined by JEP-223, most of the code related to handle the version
 * string prior to JDK 1.6 was removed (partial initialization)
 */
 // 封装JDK版本信息，支持精细化版本比较和格式化输出
class JDK_Version {
  friend class VMStructs;
  friend class Universe;
  friend void JDK_Version_init();
 private:

  static JDK_Version _current;
  static const char* _java_version;
  static const char* _runtime_name;
  static const char* _runtime_version;
  static const char* _runtime_vendor_version;
  static const char* _runtime_vendor_vm_bug_url;

  int _major;
  int _minor;
  int _security;
  int _patch;
  int _build;

  bool is_valid() const {
    return (_major != 0);
  }

  // initializes or partially initializes the _current static field
  static void initialize();

 public:

  JDK_Version() :
      _major(0), _minor(0), _security(0), _patch(0), _build(0)
      {}

  JDK_Version(int major, int minor = 0, int security = 0,
              int patch = 0, int build = 0) :
      _major(major), _minor(minor), _security(security), _patch(patch), _build(build)
      {}

  // Returns the current running JDK version
  static JDK_Version current() { return _current; }

  // Factory methods for convenience
  static JDK_Version jdk(int m) {
    return JDK_Version(m);
  }

  static JDK_Version undefined() {
    return JDK_Version(0);
  }

  bool is_undefined() const {
    return _major == 0;
  }

  int major_version() const          { return _major; }
  int minor_version() const          { return _minor; }
  int security_version() const       { return _security; }
  int patch_version() const          { return _patch; }
  int build_number() const           { return _build; }

  // Performs a full ordering comparison using all fields (patch, build, etc.)
  int compare(const JDK_Version& other) const;

  void to_string(char* buffer, size_t buflen) const;

  static const char* java_version() {
    return _java_version;
  }
  static void set_java_version(const char* version) {
    _java_version = os::strdup(version);
  }

  static const char* runtime_name() {
    return _runtime_name;
  }
  static void set_runtime_name(const char* name) {
    _runtime_name = os::strdup(name);
  }

  static const char* runtime_version() {
    return _runtime_version;
  }
  static void set_runtime_version(const char* version) {
    _runtime_version = os::strdup(version);
  }

  static const char* runtime_vendor_version() {
    return _runtime_vendor_version;
  }
  static void set_runtime_vendor_version(const char* vendor_version) {
    _runtime_vendor_version = os::strdup(vendor_version);
  }

  static const char* runtime_vendor_vm_bug_url() {
    return _runtime_vendor_vm_bug_url;
  }
  static void set_runtime_vendor_vm_bug_url(const char* vendor_vm_bug_url) {
    _runtime_vendor_vm_bug_url = os::strdup(vendor_vm_bug_url);
  }

};

#endif // SHARE_RUNTIME_JAVA_HPP
