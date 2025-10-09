/*
 * Copyright (c) 1997, 2023, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SHARE_INTERPRETER_BYTECODES_HPP
#define SHARE_INTERPRETER_BYTECODES_HPP

#include "memory/allStatic.hpp"
#include "utilities/globalDefinitions.hpp"

// Bytecodes specifies all bytecodes used in the VM and
// provides utility functions to get bytecode attributes.

class Method;

// NOTE: replicated in SA in vm/agent/sun/jvm/hotspot/interpreter/Bytecodes.java
class Bytecodes: AllStatic {
 public:
  enum Code {
    _illegal              =  -1,

    // Java bytecodes
    _nop                  =   0, // 0x00
    _aconst_null          =   1, // 0x01 将引用类型常量null推到当前栈帧的操作数栈顶
    _iconst_m1            =   2, // 0x02 将int类型常量-1推到当前栈帧的操作数栈顶
    _iconst_0             =   3, // 0x03 将int类型常量0推到当前栈帧的操作数栈顶
    _iconst_1             =   4, // 0x04 将int类型常量1推到当前栈帧的操作数栈顶
    _iconst_2             =   5, // 0x05 将int类型常量2推到当前栈帧的操作数栈顶
    _iconst_3             =   6, // 0x06 将int类型常量3推到当前栈帧的操作数栈顶
    _iconst_4             =   7, // 0x07 将int类型常量4推到当前栈帧的操作数栈顶
    _iconst_5             =   8, // 0x08 将int类型常量5推到当前栈帧的操作数栈顶
    _lconst_0             =   9, // 0x09 将long类型常量0推到当前栈帧的操作数栈顶
    _lconst_1             =  10, // 0x0a 将long类型常量1推到当前栈帧的操作数栈顶
    _fconst_0             =  11, // 0x0b 将float类型常量0推到当前栈帧的操作数栈顶
    _fconst_1             =  12, // 0x0c 将float类型常量1推到当前栈帧的操作数栈顶
    _fconst_2             =  13, // 0x0d 将float类型常量2推到当前栈帧的操作数栈顶
    _dconst_0             =  14, // 0x0e 将double类型常量0推到当前栈帧的操作数栈顶
    _dconst_1             =  15, // 0x0f 将double类型常量1推到当前栈帧的操作数栈顶
    _bipush               =  16, // 0x10 将int类型常量（-128~127）推送到当前栈帧的操作数栈顶，例如：bipush 100
    _sipush               =  17, // 0x11 将int类型常量（-32768~32767）推送到当前栈帧的操作数栈顶，例如：sipush 32767
    _ldc                  =  18, // 0x12 从当前类的运行时常量池中加载指定索引（0~255）的常量，推送到当前栈帧的操作数栈顶
    _ldc_w                =  19, // 0x13 从当前类的运行时常量池中加载指定索引（0~65535）的常量，推送到当前栈帧的操作数栈顶
    _ldc2_w               =  20, // 0x14 从当前类的运行时常量池中加载指定索引（0~65535）的常量（long/double），推送到当前栈帧的操作数栈顶
    _iload                =  21, // 0x15 从当前栈帧的局部变量表加载指定索引（0~255）的int类型数据到操作数栈顶
    _lload                =  22, // 0x16 从当前栈帧的局部变量表加载指定索引（0~255）的long类型数据到操作数栈顶
    _fload                =  23, // 0x17 从当前栈帧的局部变量表加载指定索引（0~255）的float类型数据到操作数栈顶
    _dload                =  24, // 0x18 从当前栈帧的局部变量表加载指定索引（0~255）的double类型数据到操作数栈顶
    _aload                =  25, // 0x19 从当前栈帧的局部变量表加载指定索引（0~255）的reference类型数据到操作数栈顶
    _iload_0              =  26, // 0x1a 从当前栈帧的局部变量表加载指定索引（0）的int类型数据到操作数栈顶
    _iload_1              =  27, // 0x1b 从当前栈帧的局部变量表加载指定索引（1）的int类型数据到操作数栈顶
    _iload_2              =  28, // 0x1c 从当前栈帧的局部变量表加载指定索引（2）的int类型数据到操作数栈顶
    _iload_3              =  29, // 0x1d 从当前栈帧的局部变量表加载指定索引（3）的int类型数据到操作数栈顶
    _lload_0              =  30, // 0x1e 从当前栈帧的局部变量表加载指定索引（0）的long类型数据到操作数栈顶
    _lload_1              =  31, // 0x1f 从当前栈帧的局部变量表加载指定索引（1）的long类型数据到操作数栈顶
    _lload_2              =  32, // 0x20 从当前栈帧的局部变量表加载指定索引（2）的long类型数据到操作数栈顶
    _lload_3              =  33, // 0x21 从当前栈帧的局部变量表加载指定索引（3）的long类型数据到操作数栈顶
    _fload_0              =  34, // 0x22 从当前栈帧的局部变量表加载指定索引（0）的float类型数据到操作数栈顶
    _fload_1              =  35, // 0x23 从当前栈帧的局部变量表加载指定索引（1）的float类型数据到操作数栈顶
    _fload_2              =  36, // 0x24 从当前栈帧的局部变量表加载指定索引（2）的float类型数据到操作数栈顶
    _fload_3              =  37, // 0x25 从当前栈帧的局部变量表加载指定索引（3）的float类型数据到操作数栈顶
    _dload_0              =  38, // 0x26 从当前栈帧的局部变量表加载指定索引（0）的double类型数据到操作数栈顶
    _dload_1              =  39, // 0x27 从当前栈帧的局部变量表加载指定索引（1）的double类型数据到操作数栈顶
    _dload_2              =  40, // 0x28 从当前栈帧的局部变量表加载指定索引（2）的double类型数据到操作数栈顶
    _dload_3              =  41, // 0x29 从当前栈帧的局部变量表加载指定索引（3）的double类型数据到操作数栈顶
    _aload_0              =  42, // 0x2a 从当前栈帧的局部变量表加载指定索引（0）的reference类型数据到操作数栈顶
    _aload_1              =  43, // 0x2b 从当前栈帧的局部变量表加载指定索引（1）的reference类型数据到操作数栈顶
    _aload_2              =  44, // 0x2c 从当前栈帧的局部变量表加载指定索引（2）的reference类型数据到操作数栈顶
    _aload_3              =  45, // 0x2d 从当前栈帧的局部变量表加载指定索引（3）的reference类型数据到操作数栈顶
    _iaload               =  46, // 0x2e 从int数组中加载指定索引的int类型数据到操作数栈顶
    _laload               =  47, // 0x2f 从long数组中加载指定索引的long类型数据到操作数栈顶
    _faload               =  48, // 0x30 从float数组中加载指定索引的float类型数据到操作数栈顶
    _daload               =  49, // 0x31 从double数组中加载指定索引的double类型数据到操作数栈顶
    _aaload               =  50, // 0x32 从reference数组中加载指定索引的reference类型数据到操作数栈顶
    _baload               =  51, // 0x33 从byte/boolean数组中加载指定索引的byte/boolean类型数据到操作数栈顶
    _caload               =  52, // 0x34 从char数组中加载指定索引的char类型数据到操作数栈顶
    _saload               =  53, // 0x35 从short数组中加载指定索引的short类型数据到操作数栈顶
    _istore               =  54, // 0x36 将int类型数据从操作数栈顶弹出，存储至局部变量表的指定索引位置
    _lstore               =  55, // 0x37 将long类型数据从操作数栈顶弹出，存储至局部变量表的指定索引位置
    _fstore               =  56, // 0x38 将float类型数据从操作数栈顶弹出，存储至局部变量表的指定索引位置
    _dstore               =  57, // 0x39 将double类型数据从操作数栈顶弹出，存储至局部变量表的指定索引位置
    _astore               =  58, // 0x3a 将reference类型数据从操作数栈顶弹出，存储至局部变量表的指定索引位置
    _istore_0             =  59, // 0x3b 将int类型数据从操作数栈顶弹出，存储至局部变量表的指定索引位置（0）
    _istore_1             =  60, // 0x3c 将int类型数据从操作数栈顶弹出，存储至局部变量表的指定索引位置（1）
    _istore_2             =  61, // 0x3d 将int类型数据从操作数栈顶弹出，存储至局部变量表的指定索引位置（2）
    _istore_3             =  62, // 0x3e 将int类型数据从操作数栈顶弹出，存储至局部变量表的指定索引位置（3）
    _lstore_0             =  63, // 0x3f 将long类型数据从操作数栈顶弹出，存储至局部变量表的指定索引位置（0）
    _lstore_1             =  64, // 0x40 将long类型数据从操作数栈顶弹出，存储至局部变量表的指定索引位置（1）
    _lstore_2             =  65, // 0x41 将long类型数据从操作数栈顶弹出，存储至局部变量表的指定索引位置（2）
    _lstore_3             =  66, // 0x42 将long类型数据从操作数栈顶弹出，存储至局部变量表的指定索引位置（3）
    _fstore_0             =  67, // 0x43 将float类型数据从操作数栈顶弹出，存储至局部变量表的指定索引位置（0）
    _fstore_1             =  68, // 0x44 将float类型数据从操作数栈顶弹出，存储至局部变量表的指定索引位置（1）
    _fstore_2             =  69, // 0x45 将float类型数据从操作数栈顶弹出，存储至局部变量表的指定索引位置（2）
    _fstore_3             =  70, // 0x46 将float类型数据从操作数栈顶弹出，存储至局部变量表的指定索引位置（3）
    _dstore_0             =  71, // 0x47 将double类型数据从操作数栈顶弹出，存储至局部变量表的指定索引位置（0）
    _dstore_1             =  72, // 0x48 将double类型数据从操作数栈顶弹出，存储至局部变量表的指定索引位置（1）
    _dstore_2             =  73, // 0x49 将double类型数据从操作数栈顶弹出，存储至局部变量表的指定索引位置（2）
    _dstore_3             =  74, // 0x4a 将double类型数据从操作数栈顶弹出，存储至局部变量表的指定索引位置（3）
    _astore_0             =  75, // 0x4b 将reference类型数据从操作数栈顶弹出，存储至局部变量表的指定索引位置（0）
    _astore_1             =  76, // 0x4c 将reference类型数据从操作数栈顶弹出，存储至局部变量表的指定索引位置（1）
    _astore_2             =  77, // 0x4d 将reference类型数据从操作数栈顶弹出，存储至局部变量表的指定索引位置（2）
    _astore_3             =  78, // 0x4e 将reference类型数据从操作数栈顶弹出，存储至局部变量表的指定索引位置（3）
    _iastore              =  79, // 0x4f 从操作数栈顶弹出三个值（数组引用、索引、int 值），将 int 值存储到数组的指定索引位置
    _lastore              =  80, // 0x50 从操作数栈顶弹出三个值（数组引用、索引、long 值），将 long 值存储到数组的指定索引位置
    _fastore              =  81, // 0x51 从操作数栈顶弹出三个值（数组引用、索引、float 值），将 float 值存储到数组的指定索引位置
    _dastore              =  82, // 0x52 从操作数栈顶弹出三个值（数组引用、索引、double 值），将 double 值存储到数组的指定索引位置
    _aastore              =  83, // 0x53 从操作数栈顶弹出三个值（数组引用、索引、reference 值），将 reference 值存储到数组的指定索引位置
    _bastore              =  84, // 0x54 从操作数栈顶弹出三个值（数组引用、索引、byte/boolean 值），将 byte/boolean 值存储到数组的指定索引位置
    _castore              =  85, // 0x55 从操作数栈顶弹出三个值（数组引用、索引、char 值），将 char 值存储到数组的指定索引位置
    _sastore              =  86, // 0x56 从操作数栈顶弹出三个值（数组引用、索引、short 值），将 short 值存储到数组的指定索引位置
    _pop                  =  87, // 0x57 从操作数栈顶弹出一个元素（类型不限），不保留该值
    _pop2                 =  88, // 0x58 弹出1个元素（当元素为 int/float/引用等单 Slot 类型时）；弹出2个元素（当元素为 long/double等双 Slot 类型时）
    _dup                  =  89, // 0x59 复制栈顶的1个Slot元素（如 int/float/引用）并压入栈顶，保持栈深度增加 1
    _dup_x1               =  90, // 0x5a 复制栈顶元素（1 个 Slot），将复制值插入到栈顶下方第 2 个 Slot（即插入位置为 1+1=2）
    _dup_x2               =  91, // 0x5b 复制栈顶元素（1 个 Slot），将复制值插入到栈顶下方第 3 个 Slot（即插入位置为 1+2=3）
    _dup2                 =  92, // 0x5c 复制栈顶的2个Slot元素（如 long/double或两个 int）并压入栈顶，保持栈深度增加 1
    _dup2_x1              =  93, // 0x5d 复制栈顶的双 Slot 元素（如 long/double或两个 int），将复制值插入到 栈顶下方第 3 个 Slot（即插入位置为 2+1=3）
    _dup2_x2              =  94, // 0x5e 复制栈顶的双 Slot 元素（如 long/double或两个 int，将复制值插入到栈顶下方第 4 个 Slot（即插入位置为 2+2=4）
    _swap                 =  95, // 0x5f 交换操作数栈顶的两个元素（无论类型是否相同），不改变栈深度，仅调整元素顺序
    _iadd                 =  96, // 0x60 从操作数栈顶弹出两个 int值，执行加法运算，并将结果压入操作数栈顶
    _ladd                 =  97, // 0x61 从操作数栈顶弹出两个 long值，执行加法运算，并将结果压入操作数栈顶
    _fadd                 =  98, // 0x62 从操作数栈顶弹出两个 float值，执行加法运算，并将结果压入操作数栈顶
    _dadd                 =  99, // 0x63 从操作数栈顶弹出两个 double值，执行加法运算，并将结果压入操作数栈顶
    _isub                 = 100, // 0x64 从操作数栈顶弹出两个 int值，执行减法运算（第二个弹出的值减去第一个弹出的值），并将结果压入操作数栈
    _lsub                 = 101, // 0x65 从操作数栈顶弹出两个 long值，执行减法运算（第二个弹出的值减去第一个弹出的值），并将结果压入操作数栈顶
    _fsub                 = 102, // 0x66 从操作数栈顶弹出两个 float值，执行减法运算（第二个弹出的值减去第一个弹出的值），并将结果压入操作数栈顶
    _dsub                 = 103, // 0x67 从操作数栈顶弹出两个 double值，执行减法运算（第二个弹出的值减去第一个弹出的值），并将结果压入操作数栈顶
    _imul                 = 104, // 0x68 从操作数栈顶弹出两个 int值，执行乘法运算，并将结果压入操作数栈顶
    _lmul                 = 105, // 0x69 从操作数栈顶弹出两个 long值，执行乘法运算，并将结果压入操作数栈顶
    _fmul                 = 106, // 0x6a 从操作数栈顶弹出两个 floatt值，执行乘法运算，并将结果压入操作数栈顶
    _dmul                 = 107, // 0x6b 从操作数栈顶弹出两个 double值，执行乘法运算，并将结果压入操作数栈顶
    _idiv                 = 108, // 0x6c 从操作数栈顶弹出两个 int值（被除数和除数），执行整数除法运算（被除数 ÷ 除数），并将结果压入操作数栈顶。若除数为零或发生溢出（如 Integer.MIN_VALUE / -1），则抛出异常
    _ldiv                 = 109, // 0x6d 从操作数栈顶弹出两个 long值（被除数和除数），执行长整数除法运算（被除数 ÷ 除数），并将结果压入操作数栈顶。若除数为零，则抛出 ArithmeticException
    _fdiv                 = 110, // 0x6e 从操作数栈顶弹出两个 float值（被除数和除数），执行单精度浮点数除法运算（被除数 ÷ 除数），并将结果压入操作数栈顶。若除数为零，不会抛出异常，而是返回正无穷大（Infinity）或非数值（NaN）
    _ddiv                 = 111, // 0x6f 从操作数栈顶弹出两个 double值（被除数和除数），执行双精度浮点数除法运算（被除数 ÷ 除数），并将结果压入操作数栈顶。若除数为零，不会抛出异常，而是返回正无穷大（Infinity）或非数值（NaN）
    _irem                 = 112, // 0x70 从操作数栈顶弹出两个 int类型的数值（记为 a和 b），计算 a % b的余数，并将结果压入栈顶
    _lrem                 = 113, // 0x71 从操作数栈顶弹出两个 long类型的数值（记为 a和 b），计算 a % b的余数，并将结果压入栈顶
    _frem                 = 114, // 0x72 从操作数栈顶弹出两个 float类型的数值（记为 a和 b），计算 a % b的余数，并将结果压入栈顶
    _drem                 = 115, // 0x73 从操作数栈顶弹出两个 double类型的数值（记为 a和 b），计算 a % b的余数，并将结果压入栈顶
    _ineg                 = 116, // 0x74 从操作数栈顶弹出一个 int类型的数值（记为 a），计算其按位取反后的结果 -a-1（即 ~a），并将结果压入栈顶
    _lneg                 = 117, // 0x75 从操作数栈顶弹出一个 long类型的数值（记为 a），计算其按位取反后的结果 -a-1（即 ~a），并将结果压入栈顶
    _fneg                 = 118, // 0x76 从操作数栈顶弹出一个 float类型的数值（记为 a），计算其按位取反后的结果 -a，并将结果压入栈顶
    _dneg                 = 119, // 0x77 从操作数栈顶弹出一个 double类型的数值（记为 a），计算其按位取反后的结果 -a，并将结果压入栈顶
    _ishl                 = 120, // 0x78 从操作数栈顶弹出两个 int类型的数值（记为 value和 shift），将 value的二进制位向左移动 shift位，高位补零，结果压入栈顶
    _lshl                 = 121, // 0x79 从操作数栈顶弹出两个 long类型的数值（记为 value和 shift），将 value的二进制位向左移动 shift位，高位补零，结果压入栈顶
    _ishr                 = 122, // 0x7a 从操作数栈顶弹出两个 int类型的数值（记为 value和 shift），将 value的二进制位向右移动 shift位，高位补符号位（即正数补0，负数补1），结果压入栈顶
    _lshr                 = 123, // 0x7b 从操作数栈顶弹出两个 long类型的数值（记为 value和 shift），将 value的二进制位向右移动 shift位，高位补符号位（即正数补0，负数补1），结果压入栈顶
    _iushr                = 124, // 0x7c 从操作数栈顶弹出两个 int类型的数值（记为 value和 shift），将 value的二进制位向右移动 shift位，高位补0（无论原符号位如何），结果压入栈顶
    _lushr                = 125, // 0x7d 从操作数栈顶弹出两个 long类型的数值（记为 value和 shift），将 value的二进制位向右移动 shift位，高位补0（无论原符号位如何），结果压入栈顶
    _iand                 = 126, // 0x7e 从操作数栈顶弹出两个 int类型的数值（记为 value1和 value2），对它们的二进制位执行按位与操作（即对应位均为1时结果位为1），并将结果压入栈顶
    _land                 = 127, // 0x7f 从操作数栈顶弹出两个 long类型的数值（记为 value1和 value2），对它们的二进制位执行按位与操作（即对应位均为1时结果位为1），并将结果压入栈顶
    _ior                  = 128, // 0x80 从操作数栈顶弹出两个 int类型的数值（记为 value1和 value2），对它们的二进制位执行按位或操作（即对应位中任意一个为1时结果位为1），并将结果压入栈顶
    _lor                  = 129, // 0x81 从操作数栈顶弹出两个 long类型的数值（记为 value1和 value2），对它们的二进制位执行按位或操作（即对应位中任意一个为1时结果位为1），并将结果压入栈顶
    _ixor                 = 130, // 0x82 从操作数栈顶弹出两个 int类型的数值（记为 value1和 value2），对它们的二进制位执行按位异或操作（即对应位不同时结果为1，相同时为0），并将结果压入栈顶
    _lxor                 = 131, // 0x83 从操作数栈顶弹出两个 long类型的数值（记为 value1和 value2），对它们的二进制位执行按位异或操作（即对应位不同时结果为1，相同时为0），并将结果压入栈顶
    _iinc                 = 132, // 0x84 整数局部变量自增/自减指令，专门用于对局部变量表中的 int类型变量进行直接增减操作
    _i2l                  = 133, // 0x85 从操作数栈顶弹出一个 int值，将其符号扩展为 64 位 long类型，并将结果压入栈顶
    _i2f                  = 134, // 0x86
    _i2d                  = 135, // 0x87
    _l2i                  = 136, // 0x88
    _l2f                  = 137, // 0x89
    _l2d                  = 138, // 0x8a
    _f2i                  = 139, // 0x8b
    _f2l                  = 140, // 0x8c
    _f2d                  = 141, // 0x8d
    _d2i                  = 142, // 0x8e
    _d2l                  = 143, // 0x8f
    _d2f                  = 144, // 0x90
    _i2b                  = 145, // 0x91
    _i2c                  = 146, // 0x92
    _i2s                  = 147, // 0x93
    _lcmp                 = 148, // 0x94 长整型（long）比较指令，专门用于比较栈顶的两个 long类型数值，并将比较结果（1、0 或 -1）压入操作数栈
    _fcmpl                = 149, // 0x95
    _fcmpg                = 150, // 0x96
    _dcmpl                = 151, // 0x97
    _dcmpg                = 152, // 0x98
    _ifeq                 = 153, // 0x99 从操作数栈顶弹出一个 int值，若该值为 0，则跳转到指定偏移量；否则继续执行下一条指令
    _ifne                 = 154, // 0x9a 从操作数栈顶弹出一个 int值，若该值 不等于 0，则跳转到指定偏移量；否则继续执行下一条指令
    _iflt                 = 155, // 0x9b 从操作数栈顶弹出一个 int值，若该值 小于 0，则跳转到指定偏移量；否则继续执行下一条指令
    _ifge                 = 156, // 0x9c
    _ifgt                 = 157, // 0x9d
    _ifle                 = 158, // 0x9e
    _if_icmpeq            = 159, // 0x9f 从操作数栈顶弹出两个 int值（记为 value1和 value2），若 value1 == value2，则跳转到指定偏移量；否则继续执行下一条指令
    _if_icmpne            = 160, // 0xa0
    _if_icmplt            = 161, // 0xa1
    _if_icmpge            = 162, // 0xa2
    _if_icmpgt            = 163, // 0xa3
    _if_icmple            = 164, // 0xa4
    _if_acmpeq            = 165, // 0xa5
    _if_acmpne            = 166, // 0xa6
    _goto                 = 167, // 0xa7 无条件跳转指令，用于直接跳转到当前方法内的指定代码位置（偏移量）
    _jsr                  = 168, // 0xa8 【废弃】(Jump to Subroutine）从当前指令位置无条件跳转到目标偏移量，并将下一条指令地址（返回地址）压入操作数栈顶
    _ret                  = 169, // 0xa9 【废弃】ret从操作数栈顶弹出返回地址（通常由 jsr指令压入），并将程序计数器（PC）设置为该地址，从而跳转回子程序调用点
    _tableswitch          = 170, // 0xaa 密集型跳转指令，用于高效处理switch语句中连续或密集分布的case值。其核心设计目标是实现O(1)时间复杂度的跳转，适用于case值覆盖范围小且密集的场景。
    _lookupswitch         = 171, // 0xab 通过键值对列表存储case值与跳转偏移量的映射关系，通过二分查找快速定位目标分支
    _ireturn              = 172, // 0xac 将当前方法操作数栈顶的 int类型值弹出，并压入调用者方法的操作数栈中，同时丢弃当前方法栈帧中的其他数据
    _lreturn              = 173, // 0xad
    _freturn              = 174, // 0xae
    _dreturn              = 175, // 0xaf
    _areturn              = 176, // 0xb0 将当前方法操作数栈顶的引用类型值弹出，并压入调用者方法的操作数栈中，同时丢弃当前方法栈帧中的其他数据
    _return               = 177, // 0xb1 无返回值方法（void）或构造器的终止指令，其核心功能是结束当前方法执行并释放栈帧
    _getstatic            = 178, // 0xb2 通过常量池索引定位类的静态字段符号引用，解析后获取字段值并压入操作数栈
    _putstatic            = 179, // 0xb3 通过常量池索引定位类的静态字段符号引用，解析后获取字段内存地址，将栈顶值写入该地址
    _getfield             = 180, // 0xb4 通过常量池索引 定位实例字段的符号引用，解析后获取字段在对象内存中的偏移量，从对象实例中读取字段值并压入操作数栈
    _putfield             = 181, // 0xb5 通过常量池索引定位实例字段的符号引用，解析后获取字段在对象内存中的偏移量，将栈顶值写入该地址
    _invokevirtual        = 182, // 0xb6 实例方法动态分派指令，用于支持 Java 的多态特性。通过虚方法表（vtable）在运行时动态解析并调用对象的实际方法版本，支持重写（Override）和多态
    _invokespecial        = 183, // 0xb7 特殊方法调用指令，用于调用实例初始化方法（构造函数）、私有方法或父类方法。通过常量池索引定位方法的符号引用，解析后直接调用目标方法。其调用目标在编译期确定，属于静态绑定，不支持动态分派。
    _invokestatic         = 184, // 0xb8 静态方法调用指令，用于直接调用类的静态方法。通过常量池索引定位静态方法的符号引用，解析后直接调用目标方法。其调用目标在编译期确定，属于静态绑定，不支持动态分派。
    _invokeinterface      = 185, // 0xb9 接口方法动态分派指令，用于调用接口中定义的方法。通过接口方法表（itable）在运行时动态解析并调用接口方法。由于类可以实现多个接口且方法签名可能重复，需通过接口类型和接口方法表定位具体实现。
    _invokedynamic        = 186, // 0xba 动态方法调用指令，旨在支持动态语言特性和现代 Java 语言创新（如 Lambda 表达式）。通过引导方法（Bootstrap Method）和 调用点（CallSite）机制，在运行时动态解析并调用方法。其目标方法在编译期未确定，需通过动态绑定逻辑（如 Lambda 表达式或动态语言逻辑）生成。
    _new                  = 187, // 0xbb 对象实例化的核心入口，其底层实现涉及类加载、内存分配、初始化等多阶段流程。
    _newarray             = 188, // 0xbc 用于在堆内存中分配一个基本类型数组（如 int[]、float[]等），并将数组引用压入操作数栈。数组长度由操作数栈顶的 int值指定。
    _anewarray            = 189, // 0xbd 用于在堆内存中分配一个引用类型数组，并将数组引用压入操作数栈。数组长度由操作数栈顶的 int值指定，数组元素类型由常量池中的符号引用确定。
    _arraylength          = 190, // 0xbe 从操作数栈顶弹出一个数组引用，获取该数组的长度（元素个数），并将长度值压入栈顶
    _athrow               = 191, // 0xbf 用于在字节码中显式抛出异常对象（无论是手动 throw语句触发的异常，还是运行时自动触发的异常，如 ArithmeticException）
    _checkcast            = 192, // 0xc0 用于在运行时检查对象引用是否可以安全转换为目标类型。若检查通过，操作数栈顶的引用保持不变；若失败，则抛出 ClassCastException。
    _instanceof           = 193, // 0xc1 用于在运行时判断对象是否是特定类或其子类的实例，或是否实现了指定接口，返回 boolean类型结果
    _monitorenter         = 194, // 0xc2 用于在同步代码块或同步方法中获取对象的监视器锁，确保同一时刻只有一个线程能执行临界区代码。
    _monitorexit          = 195, // 0xc3 用于释放当前线程持有的对象监视器锁，使其他线程可以竞争获取锁。
    _wide                 = 196, // 0xc4 用于将局部变量索引从8 位（0~255）扩展为 16 位（0~65535），支持访问更多局部变量。
    _multianewarray       = 197, // 0xc5 用于在运行时动态创建多维数组（或指定维度的数组），并分配内存空间
    _ifnull               = 198, // 0xc6 用于判断对象引用是否为 null，若为 null则跳转到指定分支，否则继续执行后续指令
    _ifnonnull            = 199, // 0xc7 用于判断对象引用是否不为 null，若满足条件则跳转到指定分支，否则继续执行后续指令
    _goto_w               = 200, // 0xc8 用于实现长距离无条件跳转，支持跳转到方法内任意位置的指令地址（通过 4 字节偏移量指定）
    _jsr_w                = 201, // 0xc9 用于无条件跳转至子例程（通常是 finally块），并将跳转后需返回的指令地址（即 jsr_w的下一条指令地址）压入操作数栈
    _breakpoint           = 202, // 0xca JVM 调试体系中的特殊指令，用于在代码执行到指定位置时触发断点事件，通知调试器暂停程序运行

    number_of_java_codes,

    // JVM bytecodes
    _fast_agetfield       = number_of_java_codes, // 用于直接读取对象实例的非静态字段，跳过 JVM 解释器的常规字段解析流程（如动态类型检查、缓存更新等），直接通过偏移量访问内存中的字段值
    _fast_bgetfield       , // 用于直接读取对象实例的 byte类型字段，跳过 JVM 解释器的常规字段解析流程（如动态类型检查、缓存更新等），直接通过偏移量访问内存中的字段值
    _fast_cgetfield       ,
    _fast_dgetfield       ,
    _fast_fgetfield       ,
    _fast_igetfield       ,
    _fast_lgetfield       ,
    _fast_sgetfield       ,

    _fast_aputfield       , // 用于直接写入对象数组的指定索引元素，跳过 JVM 解释器的常规数组访问流程（如数组类型检查、边界检查、同步控制等），直接通过偏移量计算内存地址并存储值
    _fast_bputfield       ,
    _fast_zputfield       ,
    _fast_cputfield       ,
    _fast_dputfield       ,
    _fast_fputfield       ,
    _fast_iputfield       ,
    _fast_lputfield       ,
    _fast_sputfield       ,

    _fast_aload_0         , // 用于直接加载局部变量表中索引为 0 的引用类型值（通常是 this引用），跳过 JVM 解释器的常规加载流程（如类型检查、边界验证等），直接通过偏移量访问内存中的值
    _fast_iaccess_0       , // 用于直接加载局部变量表中索引为 0 的 int类型值，跳过 JVM 解释器的常规加载流程（如类型检查、边界验证等），直接通过偏移量访问内存中的值
    _fast_aaccess_0       ,
    _fast_faccess_0       ,

    _fast_iload           , // 用于直接加载局部变量表中指定索引的 int类型值，跳过 JVM 解释器的常规加载流程（如类型检查、边界验证等），直接通过偏移量访问内存中的值
    _fast_iload2          ,
    _fast_icaload         ,

    _fast_invokevfinal    , // 用于直接调用 final方法，跳过 JVM 解释器的常规虚方法分派流程（如动态类型检查、方法表查找等），直接通过静态绑定执行方法体
    _fast_linearswitch    , // 用于高效处理 switch语句的分支跳转，尤其针对case 值分布稀疏或中等规模的场景。它结合了 tableswitch（直接跳转表）和 lookupswitch（二分查找）的优点，在特定条件下生成更优化的跳转逻辑
    _fast_binaryswitch    , // 通过二分查找快速定位匹配的 case分支，减少线性探测的开销

    // special handling of oop constants:
    _fast_aldc            ,
    _fast_aldc_w          ,

    _return_register_finalizer    ,

    // special handling of signature-polymorphic methods:
    _invokehandle         , // 直接调用 MethodHandle关联的目标方法，绕过 JVM 的常规动态分派流程（如反射的 Method.invoke），实现高效的方法调用

    // These bytecodes are rewritten at CDS dump time, so that we can prevent them from being
    // rewritten at run time. This way, the ConstMethods can be placed in the CDS ReadOnly
    // section, and RewriteByteCodes/RewriteFrequentPairs can rewrite non-CDS bytecodes
    // at run time.
    //
    // Rewritten at CDS dump time to | Original bytecode
    // _invoke_virtual rewritten on sparc, will be disabled if UseSharedSpaces turned on.
    // ------------------------------+------------------
    _nofast_getfield      ,          //  <- _getfield
    _nofast_putfield      ,          //  <- _putfield
    _nofast_aload_0       ,          //  <- _aload_0
    _nofast_iload         ,          //  <- _iload

    _shouldnotreachhere   ,          // For debugging


    number_of_codes
  };

  static_assert(number_of_codes <= 256, "too many bytecodes");

  // Flag bits derived from format strings, can_trap, can_rewrite, etc.:
  enum Flags : jchar {
    // semantic flags:
    _bc_can_trap      = 1<<0,     // bytecode execution can trap or block
    _bc_can_rewrite   = 1<<1,     // bytecode execution has an alternate form

    // format bits (determined only by the format string):
    _fmt_has_c        = 1<<2,     // constant, such as sipush "bcc"
    _fmt_has_j        = 1<<3,     // constant pool cache index, such as getfield "bjj"
    _fmt_has_k        = 1<<4,     // constant pool index, such as ldc "bk"
    _fmt_has_i        = 1<<5,     // local index, such as iload
    _fmt_has_o        = 1<<6,     // offset, such as ifeq
    _fmt_has_nbo      = 1<<7,     // contains native-order field(s)
    _fmt_has_u2       = 1<<8,     // contains double-byte field(s)
    _fmt_has_u4       = 1<<9,     // contains quad-byte field
    _fmt_not_variable = 1<<10,    // not of variable length (simple or wide)
    _fmt_not_simple   = 1<<11,    // either wide or variable length
    _all_fmt_bits     = (_fmt_not_simple*2 - _fmt_has_c),

    // Example derived format syndromes:
    _fmt_b      = _fmt_not_variable,
    _fmt_bc     = _fmt_b | _fmt_has_c,
    _fmt_bi     = _fmt_b | _fmt_has_i,
    _fmt_bkk    = _fmt_b | _fmt_has_k | _fmt_has_u2,
    _fmt_bJJ    = _fmt_b | _fmt_has_j | _fmt_has_u2 | _fmt_has_nbo,
    _fmt_bo2    = _fmt_b | _fmt_has_o | _fmt_has_u2,
    _fmt_bo4    = _fmt_b | _fmt_has_o | _fmt_has_u4
  };

 private:
  static       bool        _is_initialized;
  static const char* const _name       [number_of_codes];
  static const BasicType   _result_type[number_of_codes];
  static const s_char      _depth      [number_of_codes];
  static const u_char      _lengths    [number_of_codes];
  static const Code        _java_code  [number_of_codes];
  static       jchar       _flags      [(1<<BitsPerByte)*2]; // all second page for wide formats

  static void def_flags(Code code, const char* format, const char* wide_format, bool can_trap, Code java_code);

  // Verify that bcp points into method
#ifdef ASSERT
  static bool        check_method(const Method* method, address bcp);
#endif
  static bool check_must_rewrite(Bytecodes::Code bc);
  static jchar compute_flags  (const char* format, jchar more_flags);  // compute the flags

 public:
  // Conversion
  static void        check          (Code code)    { assert(is_defined(code),      "illegal code: %d", (int)code); }
  static void        wide_check     (Code code)    { assert(wide_is_defined(code), "illegal code: %d", (int)code); }
  static Code        cast           (int  code)    { return (Code)code; }


  // Fetch a bytecode, hiding breakpoints as necessary.  The method
  // argument is used for conversion of breakpoints into the original
  // bytecode.  The CI uses these methods but guarantees that
  // breakpoints are hidden so the method argument should be passed as
  // null since in that case the bcp and Method* are unrelated
  // memory.
  static Code       code_at(const Method* method, address bcp) {
    assert(method == nullptr || check_method(method, bcp), "bcp must point into method");
    Code code = cast(*bcp);
    assert(code != _breakpoint || method != nullptr, "need Method* to decode breakpoint");
    return (code != _breakpoint) ? code : non_breakpoint_code_at(method, bcp);
  }
  static Code       java_code_at(const Method* method, address bcp) {
    return java_code(code_at(method, bcp));
  }

  // Fetch a bytecode or a breakpoint:
  static Code       code_or_bp_at(address bcp)    { return (Code)cast(*bcp); }

  static Code       code_at(Method* method, int bci);

  // find a bytecode, behind a breakpoint if necessary:
  static Code       non_breakpoint_code_at(const Method* method, address bcp);

  // Bytecode attributes
  static bool        is_valid       (int  code)    { return 0 <= code && code < number_of_codes; }
  static bool        is_defined     (int  code)    { return is_valid(code) && flags(code, false) != 0; }
  static bool        wide_is_defined(int  code)    { return is_defined(code) && flags(code, true) != 0; }
  static const char* name           (Code code)    { check(code);      return _name          [code]; }
  static BasicType   result_type    (Code code)    { check(code);      return _result_type   [code]; }
  static int         depth          (Code code)    { check(code);      return _depth         [code]; }
  // Note: Length functions must return <=0 for invalid bytecodes.
  // Calling check(code) in length functions would throw an unwanted assert.
  static int         length_for     (Code code)    { return is_valid(code) ? _lengths[code] & 0xF : -1; }
  static int         wide_length_for(Code code)    { return is_valid(code) ? _lengths[code]  >> 4 : -1; }
  static bool        can_trap       (Code code)    { check(code);      return has_all_flags(code, _bc_can_trap, false); }
  static Code        java_code      (Code code)    { check(code);      return _java_code     [code]; }
  static bool        can_rewrite    (Code code)    { check(code);      return has_all_flags(code, _bc_can_rewrite, false); }
  static bool        must_rewrite(Bytecodes::Code code) { return can_rewrite(code) && check_must_rewrite(code); }
  static bool        native_byte_order(Code code)  { check(code);      return has_all_flags(code, _fmt_has_nbo, false); }
  static bool        uses_cp_cache  (Code code)    { check(code);      return has_all_flags(code, _fmt_has_j, false); }
  // if 'end' is provided, it indicates the end of the code buffer which
  // should not be read past when parsing.
  static int         special_length_at(Bytecodes::Code code, address bcp, address end = nullptr);
  static int         raw_special_length_at(address bcp, address end = nullptr);
  static int         length_for_code_at(Bytecodes::Code code, address bcp)  { int l = length_for(code); return l > 0 ? l : special_length_at(code, bcp); }
  static int         length_at      (Method* method, address bcp)  { return length_for_code_at(code_at(method, bcp), bcp); }
  static int         java_length_at (Method* method, address bcp)  { return length_for_code_at(java_code_at(method, bcp), bcp); }
  static bool        is_java_code   (Code code)    { return 0 <= code && code < number_of_java_codes; }

  static bool        is_store_into_local(Code code){ return (_istore <= code && code <= _astore_3); }
  static bool        is_const       (Code code)    { return (_aconst_null <= code && code <= _ldc2_w); }
  static bool        is_zero_const  (Code code)    { return (code == _aconst_null || code == _iconst_0
                                                           || code == _fconst_0 || code == _dconst_0); }
  static bool        is_return      (Code code)    { return (_ireturn <= code && code <= _return); }
  static bool        is_invoke      (Code code)    { return (_invokevirtual <= code && code <= _invokedynamic); }
  static bool        is_field_code  (Code code)    { return (_getstatic <= java_code(code) && java_code(code) <= _putfield); }
  static bool        has_receiver   (Code code)    { assert(is_invoke(code), "");  return code == _invokevirtual ||
                                                                                          code == _invokespecial ||
                                                                                          code == _invokeinterface; }
  static bool        has_optional_appendix(Code code) { return code == _invokedynamic || code == _invokehandle; }

  static int         flags          (int code, bool is_wide) {
    assert(code == (u_char)code, "must be a byte");
    return _flags[code + (is_wide ? (1<<BitsPerByte) : 0)];
  }
  static bool        has_all_flags  (Code code, int test_flags, bool is_wide) {
    return (flags(code, is_wide) & test_flags) == test_flags;
  }

  // Initialization
  static void        initialize     ();
};

#endif // SHARE_INTERPRETER_BYTECODES_HPP
