/*
 * Copyright (c) 2001, 2024, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SHARE_GC_SERIAL_CARDTABLERS_HPP
#define SHARE_GC_SERIAL_CARDTABLERS_HPP

#include "gc/shared/cardTable.hpp"
#include "memory/memRegion.hpp"
#include "oops/oop.hpp"

class OldGenScanClosure;
class TenuredGeneration;

// This RemSet uses a card table both as shared data structure
// for a mod ref barrier set and for the rem set information.

class CardTableRS : public CardTable {
  friend class VMStructs;

  // 检查卡片是否为脏
  static bool is_dirty(const CardValue* const v) {
    return !is_clean(v);
  }

  // 检查卡片是否为干净
  static bool is_clean(const CardValue* const v) {
    return *v == clean_card_val();
  }

  // 清除一段卡片，将其标记为干净
  static void clear_cards(CardValue* start, CardValue* end);

  // 查找第一个脏卡片
  static CardValue* find_first_dirty_card(CardValue* start_card,
                                          CardValue* end_card);

  // 查找第一个干净卡片
  template<typename Func>
  CardValue* find_first_clean_card(CardValue* start_card,
                                   CardValue* end_card,
                                   Func& object_start);

public:
  CardTableRS(MemRegion whole_heap);

  // 扫描老年代对年轻代的引用
  void scan_old_to_young_refs(TenuredGeneration* tg, HeapWord* saved_top);

  // 写屏障实现：当老年代对象引用年轻代对象时调用，将对应卡片标记为脏
  void inline_write_ref_field_gc(void* field) {
    // 计算字段所在的卡片地址
    CardValue* byte = byte_for(field);
    // 将卡片标记为脏
    *byte = dirty_card_val();
  }

  // 检查指定地址对应的卡片是否为脏
  bool is_dirty_for_addr(const void* p) const {
    CardValue* card = byte_for(p);
    return is_dirty(card);
  }

  // 验证卡表的正确性，检查是否有未标记的老年代到年轻代引用
  void verify();

  // Update old gen cards to maintain old-to-young-pointer invariant: Clear
  // the old generation card table completely if the young generation had been
  // completely evacuated, otherwise dirties the whole old generation to
  // conservatively not loose any old-to-young pointer.
  // 维护老年代到新生代的引用不变性：如果新生代已被完全 evacuated，则完全清除老年代的卡片表，
  // 否则将整个老年代标记为脏，以保守地不丢失任何老年代到新生代的指针。
  void maintain_old_to_young_invariant(TenuredGeneration* old_gen, bool is_young_gen_empty);

  // Iterate over the portion of the card-table which covers the given
  // region mr in the given space and apply cl to any dirty sub-regions
  // of mr. Clears the dirty cards as they are processed.
  // 遍历指定区域的卡片表，将其中的脏卡片应用到闭包中
  void non_clean_card_iterate(TenuredGeneration* tg,
                              MemRegion mr,
                              OldGenScanClosure* cl);

  bool is_in_young(const void* p) const override;
};

#endif // SHARE_GC_SERIAL_CARDTABLERS_HPP
