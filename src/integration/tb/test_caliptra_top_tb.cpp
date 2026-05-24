// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Western Digital Corporation or its affiliates.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include <stdlib.h>
#include <iostream>
#include <utility>
#include <string>
#include "Vcaliptra_top_tb.h"
#include "verilated.h"
#if VM_TRACE_VCD
#include "verilated_vcd_c.h"
#elif VM_TRACE_FST
#include "verilated_fst_c.h"
#endif

vluint64_t main_time = 0;
Vcaliptra_top_tb* tb = nullptr;
#if VM_TRACE
  #if VM_TRACE_VCD
    VerilatedVcdC* tfp = nullptr;
  #elif VM_TRACE_FST
    VerilatedFstC* tfp = nullptr;
  #endif
#endif

double sc_time_stamp () {
 return main_time;
}

#ifdef CALIPTRA_FB_AXI
extern "C" unsigned char get_clk();

extern "C" void step_time_veri() {
#if VM_TRACE
  if (tfp != nullptr) {
    tfp->dump(main_time);
  }
#endif
  main_time += 1;
  if (main_time % 50 == 0) {
    tb->core_clk = !tb->core_clk;
  }
  tb->eval();
}

extern "C" void at_posedge_clk() {
  vluint8_t prev_clk = get_clk();
  while (true) {
    step_time_veri();
    if (prev_clk == 0 && get_clk() == 1) break;
    prev_clk = get_clk();
  }
}
#endif


int main(int argc, char** argv) {
  std::cout << "\nVerilatorTB: Start of sim\n" << std::endl;

  Verilated::commandArgs(argc, argv);

  tb = new Vcaliptra_top_tb;

  // init trace dump
#if VM_TRACE
  Verilated::traceEverOn(true);
  #if VM_TRACE_VCD
    tfp = new VerilatedVcdC;
    tb->trace (tfp, 24);
    tfp->open ("sim.vcd");
  #elif VM_TRACE_FST
    tfp = new VerilatedFstC;
    tb->trace (tfp, 24);
    tfp->open ("sim.fst");
  #endif
#endif
  // Simulate
  while(!Verilated::gotFinish()){
#if VM_TRACE
      tfp->dump (main_time);
#endif
      main_time += 1;
      // Toggle every 5ns (timescale precision is 100ps)
      if (main_time % 50 == 0) tb->core_clk = !tb->core_clk;
      tb->eval();
  }

#if VM_TRACE
  tfp->close();
#endif

  std::cout << "\nVerilatorTB: End of sim" << std::endl;
  exit(EXIT_SUCCESS);

}
