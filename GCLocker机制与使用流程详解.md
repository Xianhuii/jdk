# GCLocker机制与使用流程详解

## 1. 功能

**GCLocker** 是 JVM 内部用于**协调垃圾回收（GC）与 JNI 临界区（JNI Critical Region）**的同步机制。其主要功能包括：

- 当有线程进入 JNI 临界区（如通过 `GetPrimitiveArrayCritical` 等 JNI 方法）时，**阻止 GC 的发生**，保证临界区内的原生代码访问 Java 堆内存的安全性。
- 当需要进行 GC 时，**阻止新的线程进入 JNI 临界区**，并等待所有已进入临界区的线程退出后再执行 GC。
- 提供 enter/exit、block/unblock 等接口，分别用于线程进入/退出临界区和 GC 请求/结束的同步。

---

## 2. 使用场景

- **JNI 临界区**：Java 线程通过 JNI 调用 `GetPrimitiveArrayCritical`、`ReleasePrimitiveArrayCritical` 等方法时，进入/退出 JNI 临界区，期间不能发生 GC。
- **GC 触发**：当 JVM 需要进行 GC（如分配失败、显式 System.gc()、内存压力等）时，必须等待所有线程退出 JNI 临界区后才能安全执行。
- **高性能原生访问**：JNI 临界区允许原生代码直接访问 Java 堆内存，避免了复制和锁定，但需要 GCLocker 保证期间不会发生 GC。

---

## 3. 源码结构与关键方法

位于 `src/hotspot/share/gc/shared/gcLocker.hpp/cpp/inline.hpp`，核心结构如下：

- **成员变量**
  - `_lock`：全局 Monitor（互斥锁+条件变量），用于同步 GC 和临界区线程。
  - `_is_gc_request_pending`：是否有 GC 请求正在等待。
  - `_verify_in_cr_count`（DEBUG）：调试用，记录当前处于临界区的线程数。

- **主要方法**
  - `enter(JavaThread* current_thread)` / `exit(JavaThread* current_thread)`：线程进入/退出 JNI 临界区时调用，保证同步。
  - `block()` / `unblock()`：GC 线程请求/结束 GC 时调用，阻止/允许新线程进入临界区，并等待所有线程退出。
  - `is_active()`：判断当前是否有线程处于临界区。
  - `enter_slow(JavaThread* current_thread)`：慢路径，处理 GC 请求期间线程进入临界区的竞争。

- **同步机制**
  - 通过全局锁和原子变量，保证 GC 和 JNI 临界区线程的互斥和有序。

---

## 4. 使用流程图

```mermaid
flowchart TD
    A[Java线程调用GetPrimitiveArrayCritical] --> B[GCLocker::enter()]
    B --> C[线程进入JNI临界区，禁止GC]
    C --> D[线程执行原生代码]
    D --> E[Java线程调用ReleasePrimitiveArrayCritical]
    E --> F[GCLocker::exit()]
    F --> G[线程退出JNI临界区，允许GC]

    H[GC线程需要执行GC] --> I[GCLocker::block()]
    I --> J[阻止新线程进入临界区]
    J --> K[等待所有线程退出临界区]
    K --> L[执行GC]
    L --> M[GCLocker::unblock()]
    M --> N[允许新线程进入临界区]
```

---

## 5. 典型源码调用链

1. **线程进入 JNI 临界区**
   ```cpp
   GCLocker::enter(JavaThread::current());
   // ... 原生代码访问 Java 堆 ...
   GCLocker::exit(JavaThread::current());
   ```

2. **GC 请求时阻塞**
   ```cpp
   GCLocker::block();
   // ... 等待所有线程退出临界区 ...
   // ... 执行GC ...
   GCLocker::unblock();
   ```

3. **慢路径处理**
   - 如果线程在 GC 请求期间尝试进入临界区，会被 enter_slow() 挂起，直到 GC 完成。

---

## 6. 总结

- **GCLocker** 是 JVM 保证 JNI 临界区与 GC 安全协作的关键机制，防止 GC 期间原生代码直接访问堆内存导致的不一致和崩溃。
- 通过全局锁和原子变量，GCLocker 实现了高效的同步和互斥，兼顾了性能和安全性。
- 其机制广泛用于高性能原生访问、JNI库开发、GC安全保障等场景。

---

如需进一步分析 JNI 临界区与 GC 的具体交互细节或源码实现，请随时告知！