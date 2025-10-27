/*
 * Copyright (c) 1997, 2020, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SHARE_RUNTIME_TIMER_HPP
#define SHARE_RUNTIME_TIMER_HPP

#include "utilities/globalDefinitions.hpp"

// Timers for simple measurement.
// 测量时间间隔类
class elapsedTimer {
  friend class VMStructs;
 private:
  // 累计的时间计数器（单位：底层时钟tick）
  jlong _counter;
  // 计时起点tick值
  jlong _start_counter;
  // 标记计时器是否处于活动状态
  bool  _active;
 public:
  elapsedTimer()             { _active = false; reset(); }
  void add(elapsedTimer t);
  void add_nanoseconds(jlong ns);
  // 开始计时（记录当前tick）
  void start();
  // 停止计时（将间隔tick累加到_counter）
  void stop();
  // 将累计时间清零
  void reset()               { _counter = 0; }
  // 返回累计时间的秒数（通过TimeHelper转换）
  double seconds() const;
  // 返回累计时间的毫秒数
  jlong milliseconds() const;
  // 返回原始累计tick值
  jlong ticks() const        { return _counter; }
  // 返回活动状态下的tick增量
  jlong active_ticks() const;
  bool  is_active() const { return _active; }
};

// TimeStamp is used for recording when an event took place.
// 记录事件发生时间点类
class TimeStamp {
 private:
  // 记录时间点的tick值（初始为0）
  jlong _counter;
 public:
  TimeStamp()  { _counter = 0; }
  // has the timestamp been updated since being created or cleared?
  bool is_updated() const { return _counter != 0; }
  // update to current elapsed time
  void update();
  // update to given elapsed time
  void update_to(jlong ticks);
  // returns seconds since updated
  // (must not be in a cleared state:  must have been previously updated)
  double seconds() const;
  jlong milliseconds() const;
  // ticks elapsed between VM start and last update
  jlong ticks() const { return _counter; }
  // ticks elapsed since last update
  jlong ticks_since_update() const;
};

// 时间单位转换工具类
class TimeHelper {
 public:
  static double counter_to_seconds(jlong counter);
  static double counter_to_millis(jlong counter);
  static jlong millis_to_counter(jlong millis);
  static jlong micros_to_counter(jlong micros);
};

#endif // SHARE_RUNTIME_TIMER_HPP
