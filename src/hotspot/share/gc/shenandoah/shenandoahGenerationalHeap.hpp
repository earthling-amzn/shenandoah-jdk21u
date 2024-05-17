/*
 * Copyright Amazon.com Inc. or its affiliates. All Rights Reserved.
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

#ifndef SHARE_GC_SHENANDOAH_SHENANDOAHGENERATIONALHEAP
#define SHARE_GC_SHENANDOAH_SHENANDOAHGENERATIONALHEAP

#include "gc/shenandoah/shenandoahHeap.hpp"

class ShenandoahRegulatorThread;
class ShenandoahGenerationalControlThread;

class ShenandoahGenerationalHeap : public ShenandoahHeap {
public:
  explicit ShenandoahGenerationalHeap(ShenandoahCollectorPolicy* policy);

  static ShenandoahGenerationalHeap* heap();

  void print_init_logger() const override;
  size_t unsafe_max_tlab_alloc(Thread *thread) const override;

  // ---------- Evacuations and Promotions
  //
  ShenandoahSharedFlag  _is_aging_cycle;
  void set_aging_cycle(bool cond) {
    _is_aging_cycle.set_cond(cond);
  }

  inline bool is_aging_cycle() const {
    return _is_aging_cycle.is_set();
  }

  oop evacuate_object(oop p, Thread* thread) override;
  oop try_evacuate_object(oop p, Thread* thread, ShenandoahHeapRegion* from_region, ShenandoahAffiliation target_gen);

  size_t plab_min_size() const { return _min_plab_size; }
  size_t plab_max_size() const { return _max_plab_size; }

  void retire_plab(PLAB* plab);
  void retire_plab(PLAB* plab, Thread* thread);

private:
  HeapWord* allocate_from_plab(Thread* thread, size_t size, bool is_promotion);
  HeapWord* allocate_from_plab_slow(Thread* thread, size_t size, bool is_promotion);
  HeapWord* allocate_new_plab(size_t min_size, size_t word_size, size_t* actual_size);

  const size_t _min_plab_size;
  const size_t _max_plab_size;

  static size_t calculate_min_plab();
  static size_t calculate_max_plab();

public:
  // ---------- Serviceability
  //
  void initialize_serviceability() override;
  GrowableArray<MemoryPool*> memory_pools() override;

  ShenandoahRegulatorThread* regulator_thread() const { return _regulator_thread;  }

  void gc_threads_do(ThreadClosure* tcl) const override;

  void stop() override;

  // Used for logging the result of a region transfer outside the heap lock
  struct TransferResult {
    bool success;
    size_t region_count;
    const char* region_destination;

    void print_on(const char* when, outputStream* ss) const;
  };

  // Zeros out the evacuation and promotion reserves
  void reset_generation_reserves();

  // Computes the optimal size for the old generation, represented as a surplus or deficit of old regions
  void compute_old_generation_balance(size_t old_xfer_limit, size_t old_cset_regions);

  // Transfers surplus old regions to young, or takes regions from young to satisfy old region deficit
  TransferResult balance_generations();

  // Makes old regions parsable
  void coalesce_and_fill_old_regions(bool concurrent);

private:
  void initialize_controller() override;

  ShenandoahRegulatorThread* _regulator_thread;

  MemoryPool* _young_gen_memory_pool;
  MemoryPool* _old_gen_memory_pool;
};

#endif //SHARE_GC_SHENANDOAH_SHENANDOAHGENERATIONALHEAP
