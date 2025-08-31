/*
 * Copyright (c) 2020, 2024, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SHARE_CLASSFILE_CLASSLOADINFO_HPP
#define SHARE_CLASSFILE_CLASSLOADINFO_HPP

#include "runtime/handles.hpp"

class InstanceKlass;

// 在 JVM 中，ClassInstanceInfo和 ClassLoadInfo是两个与类加载和实例化密切相关的核心数据结构，分别用于管理类的实例元数据和类加载过程的上下文信息。

// 存储类的实例元数据：记录类的父类、接口、字段、方法等结构化信息。
// 支持反射与动态调用：提供类的方法签名、访问权限等元数据，供反射机制使用。
// 内存布局管理：与对象头（Object Header）配合，确定实例字段的偏移量和内存对齐方式。
class ClassInstanceInfo : public StackObj {
 private:
  // 指向动态嵌套类的宿主类（如内部类或匿名类），用于支持嵌套类的访问权限和生命周期管理。
  // 例如，Java 中内部类需要持有外部类的引用，_dynamic_nest_host即存储该外部类的 InstanceKlass。
  InstanceKlass* _dynamic_nest_host;

  // 通过 Handle引用类的元数据（如常量池、方法表、字段信息等），这些数据在类加载时由 ClassFileParser解析并填充到 InstanceKlass中。
  Handle _class_data;

 public:
  ClassInstanceInfo() {
    _dynamic_nest_host = nullptr;
    _class_data = Handle();
  }
  ClassInstanceInfo(InstanceKlass* dynamic_nest_host, Handle class_data) {
    _dynamic_nest_host = dynamic_nest_host;
    _class_data = class_data;
  }

  InstanceKlass* dynamic_nest_host() const { return _dynamic_nest_host; }
  Handle class_data() const { return _class_data; }
  friend class ClassLoadInfo;
};

// ClassLoadInfo是类加载过程中用于管理类加载上下文的核心数据结构
class ClassLoadInfo : public StackObj {
 private:
  // 存储类的来源信息（如 JAR 文件、网络地址），用于安全管理器（SecurityManager）的权限校验。例如，验证类是否来自可信路径或签名是否有效。
  Handle                 _protection_domain;

  // 持有类的实例信息（如动态嵌套宿主类、常量池、方法表），支持嵌套类（如内部类）的访问权限控制。
  ClassInstanceInfo      _class_hidden_info;

  // 标记类是否被隐藏（如模块化系统中非导出包内的类）
  bool                   _is_hidden;
  bool                   _is_strong_hidden;

  // 控制是否允许访问 JVM 级别注解（如 @InvisibleForSerialization）。
  bool                   _can_access_vm_annotations;

 public:
  ClassLoadInfo(Handle protection_domain) {
    _protection_domain = protection_domain;
    _class_hidden_info._dynamic_nest_host = nullptr;
    _class_hidden_info._class_data = Handle();
    _is_hidden = false;
    _is_strong_hidden = false;
    _can_access_vm_annotations = false;
  }

  ClassLoadInfo(Handle protection_domain, InstanceKlass* dynamic_nest_host,
                Handle class_data, bool is_hidden, bool is_strong_hidden,
                bool can_access_vm_annotations) {
    _protection_domain = protection_domain;
    _class_hidden_info._dynamic_nest_host = dynamic_nest_host;
    _class_hidden_info._class_data = class_data;
    _is_hidden = is_hidden;
    _is_strong_hidden = is_strong_hidden;
    _can_access_vm_annotations = can_access_vm_annotations;
  }

  Handle protection_domain()             const { return _protection_domain; }
  const ClassInstanceInfo* class_hidden_info_ptr() const { return &_class_hidden_info; }
  bool is_hidden()                       const { return _is_hidden; }
  bool is_strong_hidden()                const { return _is_strong_hidden; }
  bool can_access_vm_annotations()       const { return _can_access_vm_annotations; }
};

#endif // SHARE_CLASSFILE_CLASSLOADINFO_HPP
