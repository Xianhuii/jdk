# HotSpot垃圾收集器统一接口分析

## 1. 概述

HotSpot的垃圾收集器采用统一的接口设计，所有垃圾收集器都继承自`CollectedHeap`抽象基类。这种设计提供了：

- **统一的接口规范**：所有GC实现必须遵循相同的接口
- **可插拔的GC策略**：运行时可以选择不同的GC实现
- **代码复用**：共享的GC基础设施和工具类
- **扩展性**：便于添加新的GC算法

## 2. 核心接口：CollectedHeap

### 2.1 类层次结构

```
CollectedHeap (抽象基类)
├── SerialHeap (串行GC)
├── ParallelScavengeHeap (并行GC)
├── G1CollectedHeap (G1 GC)
├── ZCollectedHeap (ZGC)
├── ShenandoahHeap (Shenandoah GC)
└── EpsilonHeap (Epsilon GC)
```

### 2.2 关键抽象方法

#### **内存分配接口**
```cpp
// TLAB分配
virtual HeapWord* allocate_new_tlab(size_t min_size,
                                   size_t requested_size,
                                   size_t* actual_size) = 0;

// 直接内存分配
virtual HeapWord* mem_allocate(size_t size,
                              bool* gc_overhead_limit_was_exceeded) = 0;
```

#### **垃圾收集接口**
```cpp
// 执行垃圾收集
virtual void collect(GCCause::Cause cause) = 0;

// 执行完整GC
virtual void do_full_collection(bool clear_all_soft_refs) = 0;
```

#### **内存管理接口**
```cpp
// 堆容量
virtual size_t capacity() const = 0;
virtual size_t max_capacity() const = 0;

// 已使用内存
virtual size_t used() const = 0;

// 内存检查
virtual bool is_in(const void* p) const = 0;
```

#### **TLAB管理接口**
```cpp
// TLAB容量
virtual size_t tlab_capacity(Thread *thr) const = 0;
virtual size_t tlab_used(Thread *thr) const = 0;
virtual size_t unsafe_max_tlab_alloc(Thread *thr) const = 0;
```

#### **对象遍历接口**
```cpp
// 对象遍历
virtual void object_iterate(ObjectClosure* cl) = 0;

// 并行对象遍历
virtual ParallelObjectIteratorImpl* parallel_object_iterator(uint thread_num);
```

#### **验证和调试接口**
```cpp
// 堆验证
virtual void verify(VerifyOption option) = 0;

// 准备验证
virtual void prepare_for_verify() = 0;

// 打印堆信息
virtual void print_heap_on(outputStream* st) const = 0;
virtual void print_gc_on(outputStream* st) const = 0;
```

#### **GC线程管理**
```cpp
// GC线程遍历
virtual void gc_threads_do(ThreadClosure* tc) const = 0;
```

#### **代码缓存管理**
```cpp
// 注册/注销nmethod
virtual void register_nmethod(nmethod* nm) = 0;
virtual void unregister_nmethod(nmethod* nm) = 0;
virtual void verify_nmethod(nmethod* nm) = 0;
```

### 2.3 共享基础设施

#### **GC统计信息**
```cpp
// GC计数器
unsigned int _total_collections;
unsigned int _total_full_collections;

// 历史信息
size_t _capacity_at_last_gc;
size_t _used_at_last_gc;
```

#### **GC原因管理**
```cpp
// GC原因
GCCause::Cause _gc_cause;
GCCause::Cause _gc_lastcause;
```

#### **软引用策略**
```cpp
// 软引用策略
SoftRefPolicy _soft_ref_policy;
```

## 3. 各种GC实现

### 3.1 SerialHeap (串行GC)

**特点**：
- 单线程GC
- 适合客户端应用
- 内存占用小

**关键实现**：
```cpp
class SerialHeap : public CollectedHeap {
public:
  Name kind() const override { return Serial; }

  // 串行分配
  HeapWord* allocate_new_tlab(size_t min_size, size_t requested_size, size_t* actual_size) override;
  HeapWord* mem_allocate(size_t size, bool* gc_overhead_limit_was_exceeded) override;

  // 串行收集
  void collect(GCCause::Cause cause) override;
  void do_full_collection(bool clear_all_soft_refs) override;
};
```

### 3.2 ParallelScavengeHeap (并行GC)

**特点**：
- 多线程并行GC
- 吞吐量优先
- 适合服务器应用

**关键实现**：
```cpp
class ParallelScavengeHeap : public CollectedHeap {
public:
  Name kind() const override { return Parallel; }

  // 并行分配
  HeapWord* allocate_new_tlab(size_t min_size, size_t requested_size, size_t* actual_size) override;
  HeapWord* mem_allocate(size_t size, bool* gc_overhead_limit_was_exceeded) override;

  // 并行收集
  void collect(GCCause::Cause cause) override;
  void do_full_collection(bool clear_all_soft_refs) override;
};
```

### 3.3 G1CollectedHeap (G1 GC)

**特点**：
- 低延迟GC
- 区域化设计
- 可预测的暂停时间

**关键实现**：
```cpp
class G1CollectedHeap : public CollectedHeap {
public:
  Name kind() const override { return G1; }

  // G1分配策略
  HeapWord* allocate_new_tlab(size_t min_size, size_t requested_size, size_t* actual_size) override;
  HeapWord* mem_allocate(size_t word_size, bool* gc_overhead_limit_was_exceeded) override;

  // G1收集策略
  void collect(GCCause::Cause cause) override;
  void do_full_collection(bool clear_all_soft_refs) override;

  // G1特有方法
  void start_concurrent_cycle(bool concurrent_operation_is_full_mark);
  void do_collection_pause_at_safepoint();
};
```

### 3.4 ZCollectedHeap (ZGC)

**特点**：
- 可扩展低延迟GC
- 并发收集
- 支持TB级堆

**关键实现**：
```cpp
class ZCollectedHeap : public CollectedHeap {
public:
  Name kind() const override { return Z; }

  // ZGC分配策略
  HeapWord* allocate_new_tlab(size_t min_size, size_t requested_size, size_t* actual_size) override;
  HeapWord* mem_allocate(size_t size, bool* gc_overhead_limit_was_exceeded) override;

  // ZGC收集策略
  void collect(GCCause::Cause cause) override;
  void do_full_collection(bool clear_all_soft_refs) override;

  // ZGC特有方法
  void start_concurrent_gc();
  void concurrent_gc_do_concurrent_phase();
};
```

### 3.5 ShenandoahHeap (Shenandoah GC)

**特点**：
- 低暂停时间GC
- 并发整理
- 适合大堆应用

**关键实现**：
```cpp
class ShenandoahHeap : public CollectedHeap {
public:
  Name kind() const override { return Shenandoah; }

  // Shenandoah分配策略
  HeapWord* allocate_new_tlab(size_t min_size, size_t requested_size, size_t* actual_size) override;
  HeapWord* mem_allocate(size_t size, bool* gc_overhead_limit_was_exceeded) override;

  // Shenandoah收集策略
  void collect(GCCause::Cause cause) override;
  void do_full_collection(bool clear_all_soft_refs) override;

  // Shenandoah特有方法
  void start_concurrent_mark();
  void concurrent_mark_do_mark_phase();
};
```

### 3.6 EpsilonHeap (Epsilon GC)

**特点**：
- 无GC收集器
- 仅分配，不回收
- 用于性能测试

**关键实现**：
```cpp
class EpsilonHeap : public CollectedHeap {
public:
  Name kind() const override { return Epsilon; }

  // Epsilon分配策略（简单分配）
  HeapWord* allocate_new_tlab(size_t min_size, size_t requested_size, size_t* actual_size) override;
  HeapWord* mem_allocate(size_t size, bool* gc_overhead_limit_was_exceeded) override;

  // Epsilon收集策略（空实现）
  void collect(GCCause::Cause cause) override;
  void do_full_collection(bool clear_all_soft_refs) override;
};
```

## 4. GC选择机制

### 4.1 运行时GC选择

```cpp
// 在CollectedHeap中
template<typename T>
static T* named_heap(Name kind) {
  switch (kind) {
    case Serial:      return static_cast<T*>(Universe::heap());
    case Parallel:    return static_cast<T*>(Universe::heap());
    case G1:          return static_cast<T*>(Universe::heap());
    case Epsilon:     return static_cast<T*>(Universe::heap());
    case Z:           return static_cast<T*>(Universe::heap());
    case Shenandoah:  return static_cast<T*>(Universe::heap());
    default:          return nullptr;
  }
}
```

### 4.2 GC参数配置

```cpp
// GC参数解析
class GCArguments {
public:
  static bool parse();
  static CollectedHeap* create_heap();
};
```

## 5. 共享组件

### 5.1 GC工具类

#### **GCCause**
```cpp
enum class GCCause : uint {
  _java_lang_system_gc,
  _full_gc_alot,
  _scavenge_alot,
  _allocation_pro failure,
  _g1_humongous_allocation,
  _g1_periodic_collection,
  // ... 更多原因
};
```

#### **GCWhen**
```cpp
enum class GCWhen : uint {
  BeforeGC,
  AfterGC,
  GCWhenEndSentinel
};
```

#### **VerifyOption**
```cpp
enum VerifyOption {
  VerifyOption_G1UsePrevMarking,
  VerifyOption_G1UseNextMarking,
  VerifyOption_G1UseFullMarking,
  VerifyOption_Count
};
```

### 5.2 内存分配器

#### **MemAllocator**
```cpp
class MemAllocator {
public:
  static oop obj_allocate(Klass* klass, size_t size, TRAPS);
  static oop array_allocate(Klass* klass, size_t size, int length, bool do_zero, TRAPS);
};
```

### 5.3 GC监控

#### **GCTracer**
```cpp
class GCTracer {
public:
  virtual void report_gc_start(GCCause::Cause cause, const Ticks& timestamp) = 0;
  virtual void report_gc_end(const Ticks& timestamp, TimePartitions* time_partitions) = 0;
};
```

## 6. 接口优势

### 6.1 统一性
- 所有GC实现遵循相同的接口规范
- 便于代码维护和测试
- 统一的监控和调试接口

### 6.2 可扩展性
- 新增GC算法只需实现CollectedHeap接口
- 共享基础设施减少重复代码
- 插件化的GC选择机制

### 6.3 性能优化
- 接口设计考虑了性能需求
- 支持并行和并发操作
- 内存分配和GC分离设计

### 6.4 可观测性
- 丰富的统计信息收集
- 统一的日志和监控接口
- 支持JFR和JMC等工具

## 7. 总结

HotSpot的垃圾收集器统一接口设计体现了优秀的软件架构思想：

- **抽象与具体分离**：CollectedHeap定义接口，具体GC实现细节
- **开闭原则**：对扩展开放，对修改封闭
- **依赖倒置**：高层模块不依赖低层模块，都依赖抽象
- **单一职责**：每个GC专注于自己的算法实现

这种设计使得HotSpot能够：
- 支持多种GC算法
- 根据应用场景选择合适的GC
- 持续优化和添加新的GC实现
- 保持代码的可维护性和可扩展性