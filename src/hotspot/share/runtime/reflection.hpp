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

#ifndef SHARE_RUNTIME_REFLECTION_HPP
#define SHARE_RUNTIME_REFLECTION_HPP

#include "oops/oop.hpp"
#include "runtime/fieldDescriptor.hpp"
#include "utilities/accessFlags.hpp"
#include "utilities/growableArray.hpp"

// Class Reflection contains utility methods needed for implementing the
// reflection api.
//
// Used by functions in the JVM interface.
//
// NOTE that in JDK 1.4 most of reflection is now implemented in Java
// using dynamic bytecode generation. The Array class has not yet been
// rewritten using bytecodes; if it were, most of the rest of this
// class could go away, as well as a few more entry points in jvm.cpp.

// 功能定位：为Java反射API提供底层实现支持，主要用于JVM内部调用。
// 历史背景：JDK 1.4后大部分反射功能已迁移到Java层（通过动态字节码生成），但此文件仍保留关键底层逻辑（如Array类相关操作）。
class Reflection: public AllStatic {
 public:
  // Constants defined by java reflection api classes
  enum SomeConstants {
    PUBLIC            = 0,
    DECLARED          = 1,
    MEMBER_PUBLIC     = 0,
    MEMBER_DECLARED   = 1,
    MAX_DIM           = 255
  };

  // Results returned by verify_class_access()
  enum VerifyClassAccessResults {
    ACCESS_OK = 0,
    MODULE_NOT_READABLE = 1,
    TYPE_NOT_EXPORTED = 2,
    OTHER_PROBLEM = 3
  };

  // Boxing. Returns boxed value of appropriate type. Throws IllegalArgumentException.
  // 将基本类型值（jvalue）封装为对应的Java对象（oop），如int→Integer。若类型不匹配，抛出IllegalArgumentException。
  static oop box(jvalue* v, BasicType type, TRAPS);
  // Unboxing. Returns type code and sets value.
  // 从对象中提取基本类型值，前者针对原始类型包装类（如Integer），后者针对普通对象（可能返回类型码）。
  static BasicType unbox_for_primitive(oop boxed_value, jvalue* value, TRAPS);
  static BasicType unbox_for_regular_object(oop boxed_value, jvalue* value);

  // Widening of basic types. Throws IllegalArgumentException.
  // 执行基本类型的拓宽转换（如int→long），若转换非法（如String→int），抛出异常。
  static void widen(jvalue* value, BasicType current_type, BasicType wide_type, TRAPS);

  // Reflective array access. Returns type code. Throws ArrayIndexOutOfBoundsException.
  // 获取/设置数组元素值，需处理越界异常（ArrayIndexOutOfBoundsException）。
  static BasicType array_get(jvalue* value, arrayOop a, int index, TRAPS);
  static void      array_set(jvalue* value, arrayOop a, int index, BasicType value_type, TRAPS);

  // Object creation
  // 动态创建一维或多维数组，参数包括元素类型镜像（element_mirror）和维度信息（dimensions）。
  static arrayOop reflect_new_array(oop element_mirror, jint length, TRAPS);
  static arrayOop reflect_new_multi_array(oop element_mirror, typeArrayOop dimensions, TRAPS);

  // Verification
  // 检查类访问权限，返回结果枚举（ACCESS_OK或错误类型，如模块不可读MODULE_NOT_READABLE）。
  static VerifyClassAccessResults verify_class_access(const Klass* current_class,
                                                      const InstanceKlass* new_class,
                                                      bool classloader_only);
  // Return an error message specific to the specified Klass*'s and result.
  // This function must be called from within a block containing a ResourceMark.
  // 生成具体的错误信息字符串（需配合ResourceMark使用）。
  static char*    verify_class_access_msg(const Klass* current_class,
                                          const InstanceKlass* new_class,
                                          const VerifyClassAccessResults result);

  // 验证成员（方法/字段）的访问权限，考虑类加载器、保护限制（protected_restriction）等因素。
  static bool     verify_member_access(const Klass* current_class,
                                       const Klass* resolved_class,
                                       const Klass* member_class,
                                       AccessFlags access,
                                       bool classloader_only,
                                       bool protected_restriction,
                                       TRAPS);

  // 判断两个类是否属于同一包。
  static bool     is_same_class_package(const Klass* class1, const Klass* class2);

  // inner class reflection
  // raise an ICCE unless the required relationship can be proven to hold
  // If inner_is_member, require the inner to be a member of the outer.
  // If !inner_is_member, require the inner to be anonymous (a non-member).
  // Caller is responsible for figuring out in advance which case must be true.
  // 确保内部类与外部类的关系合法性（如成员内部类或匿名类），否则抛出IllegalClassFormatException（ICCE）。
  static void check_for_inner_class(const InstanceKlass* outer,
                                    const InstanceKlass* inner,
                                    bool inner_is_member,
                                    TRAPS);

  //
  // Support for reflection based on dynamic bytecode generation (JDK 1.4)
  //

  // Create a java.lang.reflect.Method object based on a method
  // 根据methodHandle创建java.lang.reflect.Method对象。
  static oop new_method(const methodHandle& method, bool for_constant_pool_access, TRAPS);
  // Create a java.lang.reflect.Constructor object based on a method
  // 基于方法句柄创建构造器对象。
  static oop new_constructor(const methodHandle& method, TRAPS);
  // Create a java.lang.reflect.Field object based on a field descriptor
  // 根据字段描述符（fieldDescriptor）生成Field对象。
  static oop new_field(fieldDescriptor* fd, TRAPS);
  // Create a java.lang.reflect.Parameter object based on a
  // MethodParameterElement
  // 创建方法参数元数据对象（Parameter）。
  static oop new_parameter(Handle method, int index, Symbol* sym,
                           int flags, TRAPS);
  // Method invocation through java.lang.reflect.Method
  // 通过反射调用目标方法（Method对象）。
  static oop      invoke_method(oop method_mirror,
                               Handle receiver,
                               objArrayHandle args,
                               TRAPS);
  // Method invocation through java.lang.reflect.Constructor
  // 调用构造器创建实例。
  static oop      invoke_constructor(oop method_mirror, objArrayHandle args, TRAPS);

};

#endif // SHARE_RUNTIME_REFLECTION_HPP
