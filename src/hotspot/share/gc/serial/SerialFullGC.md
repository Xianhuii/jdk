# SerialFullGC 模块详细分析

SerialFullGC 模块是 JVM 中 Serial GC 的核心组件，负责执行全局标记-压缩（Mark-Compact）垃圾收集。这是一个单线程的垃圾收集器，在执行过程中会暂停所有应用线程（Stop-The-World）。下面对其进行详细分析：

## 1. 基本概念与设计

SerialFullGC 类被设计为一个全静态（AllStatic）类，实现了经典的四阶段标记-压缩算法：

1. **标记阶段**：递归遍历所有活跃对象并标记
2. **计算新地址阶段**：为存活对象计算压缩后的新地址
3. **调整指针阶段**：更新所有指针以反映对象的新位置
4. **对象移动阶段**：将对象移动到新位置

## 2. 核心数据结构

- **标记栈**：`_marking_stack` 和 `_objarray_stack`，用于标记阶段的对象遍历
- **标记保存**：`_preserved_marks`、`_preserved_count` 和 `_preserved_overflow_stack_set`，用于保存和恢复对象的标记字
- **引用处理器**：`_ref_processor`，处理弱引用、软引用等特殊引用
- **闭包**：多种闭包类型（Closure）用于遍历和处理对象图

## 3. 关键组件分析

### 3.1 Compacter 类

Compacter 类实现了压缩算法的核心逻辑，负责第二、三、四阶段的工作：

- **CompactionSpace 结构**：记录每个空间的压缩信息，包括压缩后的顶部位置和第一个死亡对象位置
- **phase2_calculate_new_addr**：计算存活对象的新地址，并处理死亡空间
- **phase3_adjust_pointers**：调整所有指针以指向对象的新位置
- **phase4_compact**：实际移动对象到新位置

### 3.2 DeadSpacer 类

用于优化压缩过程，允许在空间底部保留一定量的垃圾对象，避免在收益不大的情况下进行压缩：

- 根据 `MarkSweepDeadRatio` 参数控制允许的死亡空间比例
- 通过 `MarkSweepAlwaysCompactCount` 参数定期强制完全压缩

## 4. 关键方法分析

### 4.1 标记阶段（phase1_mark）

- 从根集合开始递归标记所有可达对象
- 处理引用对象（软引用、弱引用、虚引用等）
- 执行类卸载和代码缓存清理

### 4.2 标记过程的核心方法

- **mark_object**：标记对象并保存原始标记字
- **follow_object**：遍历对象的引用字段
- **follow_stack**：处理标记栈中的对象
- **follow_root**：处理根对象

### 4.3 指针调整与对象移动

- **adjust_pointer**：更新指针以指向对象的新位置
- **adjust_marks**：调整保存的标记字中的指针
- **restore_marks**：恢复对象的原始标记字

## 5. 执行流程

### 5.1 invoke_at_safepoint 方法

这是 SerialFullGC 的入口点，在安全点调用，执行完整的垃圾收集过程：

1. 准备阶段：分配栈空间，保存旧生代使用区域
2. 第一阶段：标记所有活跃对象
3. 第二阶段：计算新对象地址
4. 第三阶段：调整指针
5. 第四阶段：移动对象
6. 清理阶段：恢复标记，释放栈空间，处理字符串去重请求

### 5.2 优化技术

- **预取（Prefetch）**：通过 `prefetch_read_scan`、`prefetch_write_scan` 和 `prefetch_write_copy` 方法优化内存访问
- **分块处理**：对象数组分块处理，避免标记栈溢出
- **死亡空间处理**：允许保留一定量的死亡对象，避免不必要的压缩

## 6. 与其他组件的交互

- **SerialHeap**：获取堆和代的信息
- **DefNewGeneration**：年轻代，提供临时空间用于保存标记字
- **TenuredGeneration**：老年代，需要更新块偏移表（BOT）
- **FullGCForwarding**：处理对象转发
- **ReferenceProcessor**：处理特殊引用

## 7. 总结

SerialFullGC 模块实现了经典的四阶段标记-压缩算法，是 Serial GC 的核心组件。它通过单线程方式执行全堆垃圾收集，包括标记、计算新地址、调整指针和移动对象四个主要阶段。虽然执行过程中会暂停所有应用线程，但其实现简单、内存开销小，适用于内存受限的环境和对暂停时间不敏感的应用场景。

该模块的设计充分考虑了性能优化，包括预取技术、分块处理大数组、死亡空间处理等，同时还支持类卸载、代码缓存清理和字符串去重等高级功能。
        