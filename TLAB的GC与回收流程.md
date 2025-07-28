# TLAB的GC与回收流程

## 1. TLAB与GC的关系

- **TLAB本身不是独立的内存区域**，而是Eden区（新生代堆空间）的一部分。
- **TLAB只影响对象分配，不影响对象的生命周期**。对象一旦分配在TLAB中并被初始化，就和普通Eden对象一样，受GC管理。

---

## 2. TLAB的回收时机

### 2.1 正常回收（主动回收）

- **TLAB空间用尽或分配大对象时**，线程会主动“retire”当前TLAB（即回收未用空间），并尝试分配新的TLAB。
- 回收时，TLAB中未分配的空间（`top`到`end`）会被填充为“filler object”，以保证堆结构的完整性。
- 已分配的对象（`start`到`top`）不会被清理，仍然作为堆对象存在。

### 2.2 GC触发时的回收（被动回收）

- **当发生Minor GC（新生代GC）时**，所有线程的TLAB都必须“退休”：
  - JVM会遍历所有线程，将它们的TLAB中未分配空间用filler object填充。
  - 这样，Eden区的所有空间都变成了标准的堆对象或filler object，便于GC遍历和回收。
- GC后，Eden区被清空，TLAB也会被重置或重新分配。

---

## 3. 详细流程

### 3.1 TLAB主动回收流程

```mermaid
flowchart TD
    A[TLAB空间不足或需分配大对象] --> B[Thread::retire_tlab()]
    B --> C[统计已分配对象]
    C --> D[用filler object填充未分配空间]
    D --> E[TLAB重置或重新分配]
```

### 3.2 GC时TLAB回收流程

```mermaid
flowchart TD
    A[Minor GC触发] --> B[所有线程 retire_tlab()]
    B --> C[TLAB未分配空间填充filler object]
    C --> D[GC遍历Eden区所有对象]
    D --> E[GC后TLAB重置或重新分配]
```

---

## 4. 相关源码入口

- `Thread::retire_tlab()`：线程主动回收TLAB
- `ThreadLocalAllocBuffer::retire()`：TLAB回收实现
- `CollectedHeap::ensure_parsability()`：GC前确保所有TLAB可遍历（即回收未用空间）
- `G1CollectedHeap::ensure_parsability()`、`PSScavenge::ensure_parsability()`等：不同GC实现的TLAB回收入口

---

## 5. 关键点总结

- **TLAB的回收不会影响已分配对象**，只处理未分配空间。
- **GC前必须确保所有TLAB可遍历**，即未分配空间被filler object填充。
- **GC后TLAB会被重置或重新分配**，以适应新的Eden区空间。

---

## 6. 伪代码示例

```cpp
// GC前，确保所有TLAB可遍历
for (Thread* t : all_threads) {
    t->retire_tlab();
}

// TLAB回收实现
void ThreadLocalAllocBuffer::retire() {
    // 1. 统计已分配对象
    // 2. 用filler object填充未分配空间
    // 3. 重置TLAB元数据
}
```

---

如需详细源码追踪或更细致的流程图，请随时告知！