# TLAB空间初始化与堆分配机制

## 1. TLAB空间的初始化与分配流程

### 1.1 初始化时机
- TLAB的分配通常发生在：
  - 线程第一次需要分配对象时（且开启了UseTLAB）
  - 线程原有TLAB空间耗尽，需要“refill”时

### 1.2 分配流程
1. **计算TLAB大小**
   JVM根据历史分配速率、全局参数（如`TLABSize`、`MinTLABSize`、`MaxTLABSize`等）和当前Eden区剩余空间，动态计算本次TLAB的大小。
   - 相关代码：`ThreadLocalAllocBuffer::compute_size`、`ThreadLocalAllocBuffer::compute_min_size`

2. **向堆（Eden区）申请空间**
   通过 `Universe::heap()->allocate_new_tlab(...)` 向堆管理器（通常是Eden区）申请一块连续内存。
   - 相关代码：`MemAllocator::mem_allocate_inside_tlab_slow`、`CollectedHeap::allocate_new_tlab`

3. **初始化TLAB元数据**
   设置TLAB的`_start`、`_top`、`_end`等指针，准备好分配。
   - 相关代码：`Thread::fill_tlab`、`ThreadLocalAllocBuffer::fill`

4. **分配对象时直接在TLAB内递增指针**
   不再需要全局锁，效率极高。

---

## 2. 相关源码片段

### TLAB分配空间的核心调用链
```cpp
// MemAllocator::mem_allocate_inside_tlab_slow
size_t min_tlab_size = ThreadLocalAllocBuffer::compute_min_size(_word_size);
mem = Universe::heap()->allocate_new_tlab(min_tlab_size, new_tlab_size, &allocation._allocated_tlab_size);
// 这里的 Universe::heap() 实际就是堆（Eden区）
```

### TLAB初始化
```cpp
// Thread::fill_tlab
tlab().fill(start, start + pre_reserved, new_size);
```

---

## 3. 总结

- **TLAB的存储空间就是从Java堆（Eden区）中分配出来的**，每个线程独享一小块。
- TLAB的分配和回收完全受JVM堆管理器控制，保证了堆空间的一致性和可回收性。
- TLAB只是分配加速手段，不改变对象的生命周期和GC行为。

---

如需详细源码追踪或分配流程图，可进一步说明！