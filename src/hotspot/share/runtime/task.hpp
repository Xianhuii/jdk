/*
 * Copyright (c) 1997, 2022, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SHARE_RUNTIME_TASK_HPP
#define SHARE_RUNTIME_TASK_HPP

#include "memory/allocation.hpp"
#include "runtime/timer.hpp"

// A PeriodicTask has the sole purpose of executing its task
// function with regular intervals.
// Usage:
//   PeriodicTask pf(10);
//   pf.enroll();
//   ...
//   pf.disenroll();
// 这是一个用于实现周期性任务调度的框架，允许用户定义定时任务并通过WatcherThread（监控线程）统一管理执行。
// 任务按固定时间间隔重复执行，适用于需要周期性操作的场景（如监控、清理等）。
class PeriodicTask: public CHeapObj<mtInternal> {
 public:
  // Useful constants.
  // The interval constants are used to ensure the declared interval
  // is appropriate;  it must be between min_interval and max_interval,
  // and have a granularity of interval_gran (all in millis).
  enum { max_tasks     = 10,       // Max number of periodic tasks in system
         interval_gran = 10,
         min_interval  = 10,
         max_interval  = 10000 };

  static int num_tasks()   { return _num_tasks; }

 private:
  // 记录已流逝的时间（相对于任务间隔的偏移量）
  int _counter;
  // 任务执行间隔（毫秒）
  const int _interval;

  // 当前注册的任务数量
  static int _num_tasks;
  // 存储最多max_tasks(10)个任务指针的数组
  static PeriodicTask* _tasks[PeriodicTask::max_tasks];
  // Can only be called by the WatcherThread
  static void real_time_tick(int delay_time);

  // Only the WatcherThread can cause us to execute PeriodicTasks
  // JVM内部线程，负责驱动任务调度
  friend class WatcherThread;
 public:
  PeriodicTask(size_t interval_time); // interval is in milliseconds of elapsed time
  virtual ~PeriodicTask();

  // Make the task active
  // For dynamic enrollment at the time T, the task will execute somewhere
  // between T and T + interval_time.
  // 将任务注册到调度队列，开始周期性执行
  void enroll();

  // Make the task inactivate
  // 从调度队列移除任务，停止执行
  void disenroll();

  // 检查是否到达执行时间，若是则调用task()
  void execute_if_pending(int delay_time) {
    // make sure we don't overflow
    jlong tmp = (jlong) _counter + (jlong) delay_time;

    if (tmp >= (jlong) _interval) {
      _counter = 0;
      task();
    } else {
      _counter += delay_time;
    }
  }

  // Returns how long (time in milliseconds) before the next time we should
  // execute this task.
  // 返回距离下一次执行剩余的时间（毫秒）
  int time_to_next_interval() const {
    assert(_interval > _counter,  "task counter greater than interval?");
    return _interval - _counter;
  }

  // Calculate when the next periodic task will fire.
  // Called by the WatcherThread's run method.
  // Requires the PeriodicTask_lock.
  // 静态方法，计算所有任务中最小的等待时间（供WatcherThread休眠）
  static int time_to_wait();

  // The task to perform at each period
  // 用户需实现的周期性任务逻辑
  virtual void task() = 0;
};

#endif // SHARE_RUNTIME_TASK_HPP
