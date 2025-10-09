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

#ifndef SHARE_INTERPRETER_BYTECODEHISTOGRAM_HPP
#define SHARE_INTERPRETER_BYTECODEHISTOGRAM_HPP

#include "interpreter/bytecodes.hpp"
#include "memory/allStatic.hpp"
// 用于统计和分析Java字节码执行情况的工具类，主要用于性能调优和热点代码分析

// BytecodeCounter counts the number of bytecodes executed
// 统计自JVM启动或上次重置以来执行的总字节数及执行时间
class BytecodeCounter: AllStatic {
 private:
  // 累计执行的总字节数（非生产环境有效）
  NOT_PRODUCT(static uintx _counter_value;)
  // 最近一次重置的时间戳（非生产环境有效）
  NOT_PRODUCT(static jlong _reset_time;)

  friend class TemplateInterpreterGenerator;
  friend class         BytecodeInterpreter;

 public:
  // Initialization
  // 重置计数器
  static void reset()                      PRODUCT_RETURN;

  // Counter info (all info since last reset)
  // 返回总字节数
  static uintx  counter_value()            PRODUCT_RETURN0 NOT_PRODUCT({ return _counter_value; });
  // 返回自上次重置后的时间（秒）
  static double elapsed_time()             PRODUCT_RETURN0; // in seconds
  // 计算执行速度（字节数/秒）
  static double frequency()                PRODUCT_RETURN0; // bytecodes/seconds

  // Counter printing
  // 输出统计结果
  static void   print()                    PRODUCT_RETURN;
};


// BytecodeHistogram collects number of executions of bytecodes
// 统计每个字节码指令的单独执行次数
class BytecodeHistogram: AllStatic {
 private:
  // 数组记录每个字节码的执行次数（非生产环境有效）
  NOT_PRODUCT(static int _counters[Bytecodes::number_of_codes];)   // a counter for each bytecode

  friend class TemplateInterpreterGenerator;
  friend class         BytecodeInterpreter;

 public:
  // Initialization
  // 置所有计数器
  static void reset()                       PRODUCT_RETURN; // reset counters

  // Profile printing
  // 按出现频率排序输出，可设置阈值（如仅显示占比>1%的字节码）
  static void print(float cutoff = 0.01F)   PRODUCT_RETURN; // cutoff in percent
};


// BytecodePairHistogram collects number of executions of bytecode pairs.
// A bytecode pair is any sequence of two consecutive bytecodes.
// 统计连续两个字节码组合的执行频率，用于分析代码模式
class BytecodePairHistogram: AllStatic {
 public: // for solstudio
  enum Constants {
    // 使用256（2^8）作为基数，覆盖所有可能的单字节码
    log2_number_of_codes = 8,                         // use a power of 2 for faster addressing
    // 存储所有可能的字节码对
    number_of_codes      = 1 << log2_number_of_codes, // must be no less than Bytecodes::number_of_codes
    // 二维数组记录每对字节码的出现次数
    number_of_pairs      = number_of_codes * number_of_codes
  };

 private:
  // 辅助追踪当前处理的字节码位置
  NOT_PRODUCT(static int  _index;)                      // new bytecode is shifted in - used to index into _counters
  NOT_PRODUCT(static int  _counters[number_of_pairs];)  // a counter for each pair

  friend class TemplateInterpreterGenerator;

 public:
  // Initialization
  // 重置所有计数器
  static void reset()                       PRODUCT_RETURN;   // reset counters

  // Profile printing
  // 按频率排序输出字节码对，支持阈值过滤
  static void print(float cutoff = 0.01F)   PRODUCT_RETURN;   // cutoff in percent
};

#endif // SHARE_INTERPRETER_BYTECODEHISTOGRAM_HPP
