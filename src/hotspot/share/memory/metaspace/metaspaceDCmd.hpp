/*
 * Copyright (c) 2018, 2024, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2018, 2020 SAP SE. All rights reserved.
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

#ifndef SHARE_MEMORY_METASPACE_METASPACEDCMD_HPP
#define SHARE_MEMORY_METASPACE_METASPACEDCMD_HPP

#include "services/diagnosticCommand.hpp"

class outputStream;

namespace metaspace {

// 继承自 DCmdWithParser，表明其是一个可解析参数的诊断命令
class MetaspaceDCmd : public DCmdWithParser {
  DCmdArgument<bool> _basic; // 是否输出基础统计信息（如总大小、已用空间）
  DCmdArgument<bool> _show_loaders; // 是否显示类加载器树状结构
  DCmdArgument<bool> _by_spacetype; // 是否按空间类型（如Class Space、Non-Class Space）分类统计
  DCmdArgument<bool> _by_chunktype; // 是否按内存块类型（如Free、Used）分类
  DCmdArgument<bool> _show_vslist; // 是否列出所有虚拟空间（Virtual Space）
  DCmdArgument<bool> _show_chunkfreelist; // 是否显示空闲内存块列表
  DCmdArgument<char*> _scale; // 数值缩放参数（如输入 "MB" 则以MB为单位输出
  DCmdArgument<bool> _show_classes; // 是否显示类加载的详细信息
public:
  MetaspaceDCmd(outputStream* output, bool heap);
  static const char* name() {
    return "VM.metaspace";
  }
  static const char* description() {
    return "Prints the statistics for the metaspace";
  }
  static const char* impact() {
      return "Medium: Depends on number of classes loaded.";
  }
  static int num_arguments() { return 8; }
  virtual void execute(DCmdSource source, TRAPS);
};

} // namespace metaspace

#endif // SHARE_MEMORY_METASPACE_METASPACEDCMD_HPP
