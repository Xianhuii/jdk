# TLAB实现原理与分配流程分析

## 一、TLAB实现原理

### 1. 设计目的
- 避免多线程并发分配对象时的锁竞争。
- 每个Java线程拥有自己的TLAB，直接在本地缓冲区分配小对象，只有缓冲区不足时才访问全局堆。

### 2. 关键数据结构

#### 2.1 ThreadLocalAllocBuffer
- 位于 `src/hotspot/share/gc/shared/threadLocalAllocBuffer.hpp`
- 主要成员：
  - `_start`：TLAB起始地址
  - `_top`：当前分配指针
  - `_end`：TLAB分配终止地址
  - `_allocation_end`：实际TLAB末尾
  - `_desired_size`：期望TLAB大小
  - `_refill_waste_limit`：剩余空间大于此值则不丢弃TLAB
- 主要方法：
  - `allocate(size_t size)`：尝试在TLAB分配对象
  - `compute_size(size_t obj_size)`：计算新TLAB大小
  - `retire()`：回收/重置TLAB
  - `fill()`：初始化TLAB

#### 2.2 Thread
- 每个线程持有一个 `ThreadLocalAllocBuffer _tlab;`
- 通过 `Thread::tlab()` 访问

#### 2.3 MemAllocator
- 对象分配的统一入口，负责协调TLAB分配与堆分配。

---

## 二、TLAB分配流程

### 1. 分配对象的主流程（以 MemAllocator::allocate 为例）

1. **快速路径：TLAB分配**
   - 调用 `ThreadLocalAllocBuffer::allocate(size)`，尝试在TLAB中分配对象。
   - 如果TLAB剩余空间足够，直接分配，更新`_top`指针，返回对象地址。

2. **慢路径：TLAB不足时处理**
   - 如果TLAB空间不足，进入慢路径：
     - 检查TLAB剩余空间是否大于`_refill_waste_limit`，若是则不丢弃TLAB，直接走堆分配。
     - 否则，回收当前TLAB（`retire_tlab`），并尝试分配新的TLAB（`compute_size`、`allocate_new_tlab`）。
     - 新TLAB分配成功后，初始化并分配对象。

3. **TLAB外分配**
   - 如果无法分配TLAB或对象过大，直接在堆上分配。

4. **采样与监控**
   - 分配过程中可能触发采样点、事件上报等。

---

## 三、TLAB分配流程图

```mermaid
flowchart TD
    A[开始对象分配] --> B{UseTLAB?}
    B -- 否 --> H[直接在堆分配]
    B -- 是 --> C[TLAB.allocate(size)]
    C -- 成功 --> D[更新TLAB top，返回对象]
    C -- 失败 --> E{TLAB剩余空间 > refill_waste_limit?}
    E -- 是 --> H
    E -- 否 --> F[retire_tlab 回收TLAB]
    F --> G[compute_size 计算新TLAB大小]
    G --> I{能分配新TLAB?}
    I -- 否 --> H
    I -- 是 --> J[fill_tlab 初始化TLAB]
    J --> K[TLAB.allocate(size)]
    K -- 成功 --> D
    K -- 失败 --> H
    H[在堆分配对象]
```

---

## 四、源码关键点

- **TLAB分配**：`ThreadLocalAllocBuffer::allocate(size_t size)`
- **慢路径**：`MemAllocator::mem_allocate_inside_tlab_slow`
- **TLAB回收**：`Thread::retire_tlab`
- **新TLAB分配**：`Universe::heap()->allocate_new_tlab`

---

## 五、总结

TLAB通过为每个线程分配独立的小块内存，极大减少了对象分配时的锁竞争。分配流程分为快速路径（TLAB内分配）和慢路径（TLAB不足时回收/新建/堆分配），并配合采样、监控等机制，兼顾了性能与可观测性。

如需更详细的源码注释或流程细节，可进一步指定关注点。