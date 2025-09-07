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

#ifndef SHARE_MEMORY_ITERATOR_HPP
#define SHARE_MEMORY_ITERATOR_HPP

#include "memory/allocation.hpp"
#include "memory/memRegion.hpp"
#include "oops/oopsHierarchy.hpp"

class CodeBlob;
class nmethod;
class ReferenceDiscoverer;
class DataLayout;
class KlassClosure;
class ClassLoaderData;
class Symbol;
class Metadata;
class Thread;

// The following classes are C++ `closures` for iterating over objects, roots and spaces
// 所有闭包的基类，继承自StackObj（栈上分配对象）
class Closure : public StackObj { };

// 遍历线程的接口，定义do_thread(Thread*)方法
class ThreadClosure {
 public:
  virtual void do_thread(Thread* thread) = 0;
};

// OopClosure is used for iterating through references to Java objects.
// 处理Java对象引用的核心接口
class OopClosure : public Closure {
 public:
  // 处理普通对象指针
  virtual void do_oop(oop* o) = 0;
  // 处理压缩对象指针（32位模式下）
  virtual void do_oop(narrowOop* o) = 0;
};

// 空闭包
class DoNothingClosure : public OopClosure {
 public:
  virtual void do_oop(oop* p)       {}
  virtual void do_oop(narrowOop* p) {}
};
extern DoNothingClosure do_nothing_cl;

// OopIterateClosure adds extra code to be run during oop iterations.
// This is needed by the GC and is extracted to a separate type to not
// pollute the OopClosure interface.
// 继承自OopClosure，扩展元数据处理能力
class OopIterateClosure : public OopClosure {
 private:
  ReferenceDiscoverer* _ref_discoverer;

 protected:
  OopIterateClosure(ReferenceDiscoverer* rd) : _ref_discoverer(rd) { }
  OopIterateClosure() : _ref_discoverer(nullptr) { }
  ~OopIterateClosure() { }

  void set_ref_discoverer_internal(ReferenceDiscoverer* rd) { _ref_discoverer = rd; }

 public:
  ReferenceDiscoverer* ref_discoverer() const { return _ref_discoverer; }

  // Iteration of InstanceRefKlasses differ depending on the closure,
  // the below enum describes the different alternatives.
  // 迭代模式
  enum ReferenceIterationMode {
    DO_DISCOVERY,                // Apply closure and discover references
    DO_FIELDS,                   // Apply closure to all fields
    DO_FIELDS_EXCEPT_REFERENT    // Apply closure to all fields except the referent field
  };

  // The default iteration mode is to do discovery.
  // 定义引用处理策略（发现引用/遍历字段/忽略referent字段）
  virtual ReferenceIterationMode reference_iteration_mode() { return DO_DISCOVERY; }

  // If the do_metadata functions return "true",
  // we invoke the following when running oop_iterate():
  //
  // 1) do_klass on the header klass pointer.
  // 2) do_klass on the klass pointer in the mirrors.
  // 3) do_cld   on the class loader data in class loaders.
  //
  // Used to determine metadata liveness for class unloading GCs.

  // 是否处理类元数据
  virtual bool do_metadata() = 0;
  // 处理类数据
  virtual void do_klass(Klass* k) = 0;
  // 处理类加载器数据
  virtual void do_cld(ClassLoaderData* cld) = 0;

  // Class redefinition needs to get notified about methods from stackChunkOops
  // 处理方法
  virtual void do_method(Method* m) = 0;
  // The code cache unloading needs to get notified about methods from stackChunkOops
  // 处理JIT编译代码
  virtual void do_nmethod(nmethod* nm) = 0;
};

// An OopIterateClosure that can be used when there's no need to visit the Metadata.
// 简单实现OopIterateClosure，默认不处理元数据（抛出ShouldNotReachHere异常）
class BasicOopIterateClosure : public OopIterateClosure {
public:
  BasicOopIterateClosure(ReferenceDiscoverer* rd = nullptr) : OopIterateClosure(rd) {}

  virtual bool do_metadata() { return false; }
  virtual void do_klass(Klass* k) { ShouldNotReachHere(); }
  virtual void do_cld(ClassLoaderData* cld) { ShouldNotReachHere(); }
  virtual void do_method(Method* m) { ShouldNotReachHere(); }
  virtual void do_nmethod(nmethod* nm) { ShouldNotReachHere(); }
};

// Interface for applying an OopClosure to a set of oops.
// 定义oops_do(OopClosure*)方法，用于对对象集合执行闭包操作
class OopIterator {
public:
  virtual void oops_do(OopClosure* cl) = 0;
};

enum class derived_base : intptr_t;
enum class derived_pointer : intptr_t;
// 处理压缩指针（derived pointers）的特殊场景，需实现do_derived_oop(derived_base*, derived_pointer*)
class DerivedOopClosure : public Closure {
 public:
  enum { SkipNull = true };
  virtual void do_derived_oop(derived_base* base, derived_pointer* derived) = 0;
};

class KlassClosure : public Closure {
 public:
  virtual void do_klass(Klass* k) = 0;
};

// 处理类加载器数据（ClassLoaderData
class CLDClosure : public Closure {
 public:
  virtual void do_cld(ClassLoaderData* cld) = 0;
};

class MetadataClosure : public Closure {
 public:
  virtual void do_metadata(Metadata* md) = 0;
};


class CLDToOopClosure : public CLDClosure {
  OopClosure*       _oop_closure;
  int               _cld_claim;

 public:
  CLDToOopClosure(OopClosure* oop_closure,
                  int cld_claim) :
      _oop_closure(oop_closure),
      _cld_claim(cld_claim) {}

  void do_cld(ClassLoaderData* cld);
};

template <int claim>
class ClaimingCLDToOopClosure : public CLDToOopClosure {
public:
  ClaimingCLDToOopClosure(OopClosure* cl) : CLDToOopClosure(cl, claim) {}
};

class ClaimMetadataVisitingOopIterateClosure : public OopIterateClosure {
 protected:
  const int _claim;

 public:
  ClaimMetadataVisitingOopIterateClosure(int claim, ReferenceDiscoverer* rd = nullptr) :
      OopIterateClosure(rd),
      _claim(claim) { }

  virtual bool do_metadata() { return true; }
  virtual void do_klass(Klass* k);
  virtual void do_cld(ClassLoaderData* cld);
  virtual void do_method(Method* m);
  virtual void do_nmethod(nmethod* nm);
};

// The base class for all concurrent marking closures,
// that participates in class unloading.
// It's used to proxy through the metadata to the oops defined in them.
class MetadataVisitingOopIterateClosure: public ClaimMetadataVisitingOopIterateClosure {
 public:
  MetadataVisitingOopIterateClosure(ReferenceDiscoverer* rd = nullptr);
};

// ObjectClosure is used for iterating through an object space
// 遍历对象空间的接口，定义do_object(oop)方法
class ObjectClosure : public Closure {
 public:
  // Called for each object.
  virtual void do_object(oop obj) = 0;
};

class BoolObjectClosure : public Closure {
 public:
  virtual bool do_object_b(oop obj) = 0;
};

class OopFieldClosure {
public:
  virtual void do_field(oop base, oop* p) = 0;
};

class AlwaysTrueClosure: public BoolObjectClosure {
 public:
  bool do_object_b(oop p) { return true; }
};

class AlwaysFalseClosure : public BoolObjectClosure {
 public:
  bool do_object_b(oop p) { return false; }
};

// Applies an oop closure to all ref fields in objects iterated over in an
// object iteration.
// 将对象转换为oop并应用OopIterateClosure
class ObjectToOopClosure: public ObjectClosure {
  OopIterateClosure* _cl;
public:
  void do_object(oop obj);
  ObjectToOopClosure(OopIterateClosure* cl) : _cl(cl) {}
};

// NMethodClosure is used for iterating through nmethods
// in the code cache or on thread stacks
// 处理JIT编译后的代码（nmethod）
class NMethodClosure : public Closure {
 public:
  virtual void do_nmethod(nmethod* n) = 0;
};

// Applies an oop closure to all ref fields in nmethods
// iterated over in an object iteration.
// 处理JIT编译后的代码（nmethod）
class NMethodToOopClosure : public NMethodClosure {
 protected:
  OopClosure* _cl;
  bool _fix_relocations;
 public:
  // If fix_relocations(), then cl must copy objects to their new location immediately to avoid
  // patching nmethods with the old locations.
  NMethodToOopClosure(OopClosure* cl, bool fix_relocations) : _cl(cl), _fix_relocations(fix_relocations) {}
  void do_nmethod(nmethod* nm) override;

  bool fix_relocations() const { return _fix_relocations; }
  const static bool FixRelocations = true;
};

class MarkingNMethodClosure : public NMethodToOopClosure {
  bool _keepalive_nmethods;

 public:
  MarkingNMethodClosure(OopClosure* cl, bool fix_relocations, bool keepalive_nmethods) :
      NMethodToOopClosure(cl, fix_relocations),
      _keepalive_nmethods(keepalive_nmethods) {}

  // Called for each nmethod.
  virtual void do_nmethod(nmethod* nm);
};

// MonitorClosure is used for iterating over monitors in the monitors cache

class ObjectMonitor;
// 遍历监视器（ObjectMonitor）
class MonitorClosure : public StackObj {
 public:
  // called for each monitor in cache
  virtual void do_monitor(ObjectMonitor* m) = 0;
};

// A closure that is applied without any arguments.
class VoidClosure : public StackObj {
 public:
  virtual void do_void() = 0;
};


// YieldClosure is intended for use by iteration loops
// to incrementalize their work, allowing interleaving
// of an interruptible task so as to allow other
// threads to run (which may not otherwise be able to access
// exclusive resources, for instance). Additionally, the
// closure also allows for aborting an ongoing iteration
// by means of checking the return value from the polling
// call.
// 支持迭代过程中的中断和恢复，通过should_return()控制流程
class YieldClosure : public StackObj {
public:
 virtual bool should_return() = 0;

 // Yield on a fine-grain level. The check in case of not yielding should be very fast.
 virtual bool should_return_fine_grain() { return false; }
};

// 处理符号表（Symbol）
class SymbolClosure : public StackObj {
 public:
  virtual void do_symbol(Symbol**) = 0;
};

// 泛型比较接口，用于自定义对象比较逻辑
template <typename E>
class CompareClosure : public Closure {
public:
    virtual int do_compare(const E&, const E&) = 0;
};

class OopIteratorClosureDispatch {
 public:
  template <typename OopClosureType> static void oop_oop_iterate(OopClosureType* cl, oop obj, Klass* klass);
  template <typename OopClosureType> static void oop_oop_iterate(OopClosureType* cl, oop obj, Klass* klass, MemRegion mr);
  template <typename OopClosureType> static void oop_oop_iterate_backwards(OopClosureType* cl, oop obj, Klass* klass);
};

#endif // SHARE_MEMORY_ITERATOR_HPP
