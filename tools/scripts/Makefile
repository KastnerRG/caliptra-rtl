# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

SHELL = /usr/bin/env bash
PLAYBOOK_RANDOM_SEED ?= $(shell date +%s)
BUILD_CFLAGS ?= 
# Cflags to give fw visibility to expected hw configuration
# Current release version is 2.1 - smoke test uses this value to check HW_REV_ID register
# Update the value here after 2.1
CALIPTRA_HW_REV_ID=0x0012
# After 1.1, LMS accelerator is always available
HWCFG_CFLAGS ?= -DCALIPTRA_HWCONFIG_LMS_EN -DCALIPTRA_HW_REV_ID=$(CALIPTRA_HW_REV_ID)
TEST_CFLAGS = -g -O3 -DMY_RANDOM_SEED=$(PLAYBOOK_RANDOM_SEED) $(BUILD_CFLAGS)
CALIPTRA_INTERNAL_TRNG ?= 1
CALIPTRA_MODE_SUBSYSTEM ?= 0
VERILATOR = verilator
GCC_PREFIX = riscv64-unknown-elf
BUILD_DIR = $(CURDIR)
# CALIPTRA_PRIM_ROOT  defaults to src/caliptra_prim_generic
CALIPTRA_PRIM_ROOT ?= $(CALIPTRA_ROOT)/src/caliptra_prim_generic
CALIPTRA_PRIM_MODULE_PREFIX ?= caliptra_prim_generic

# Ensure that RISC-V toolchain is installed
ifeq ($(shell which $(GCC_PREFIX)-gcc 2> /dev/null),)
$(error RISC-V toolchain not found, please refer to https://github.com/chipsalliance/caliptra-rtl?tab=readme-ov-file#riscv-toolchain-installation for more details)
endif

rv_gcc_version = $(strip $(shell $(GCC_PREFIX)-gcc --version | head -1 | sed "s/$(GCC_PREFIX)-gcc (.*) //"))
$(info rv_gcc_version is $(rv_gcc_version))
ifeq "12.2.0" "$(word 1, $(sort 12.2.0 $(rv_gcc_version)))"
ABI = -mabi=ilp32 -march=rv32imc_zicsr_zifencei
else
ABI = -mabi=ilp32 -march=rv32imc
endif

# Define test name
TESTNAME ?= iccm_lock
TEST_DIR = $(CALIPTRA_ROOT)/src/integration/test_suites/$(TESTNAME)
HEX_DIR = $(CALIPTRA_ROOT)/src/integration/test_suites/hex
INCLUDES_DIR = $(CALIPTRA_ROOT)/src/integration/test_suites/includes
TBDIR = $(CALIPTRA_ROOT)/src/integration/tb
RISCV_HW_IF_DIR = $(CALIPTRA_ROOT)/src/integration/test_suites/libs/riscv_hw_if
ISR_DIR    = $(CALIPTRA_ROOT)/src/integration/test_suites/libs/caliptra_isr
PRINTF_DIR = $(CALIPTRA_ROOT)/src/integration/test_suites/libs/printf
ifeq ($(CALIPTRA_MODE_SUBSYSTEM), 1)
CLP_REG_DIR = $(CALIPTRA_ROOT)/src/integration/rtl/caliptra_reg_ss
else
CLP_REG_DIR = $(CALIPTRA_ROOT)/src/integration/rtl/caliptra_reg
endif
# Determine test directory
#ifneq (,$(wildcard $(TBDIR)/tests/$(TEST)))
#  TEST_DIR = $(TBDIR)/tests/$(TEST)
#endif

# Libraries providing functionality for each peripheral component in Caliptra
COMP_LIB_NAMES := aes \
		  doe \
		  ecc \
		  mldsa \
		  mlkem \
		  hmac \
		  keyvault \
		  datavault \
		  sha256 \
		  sha512 \
		  sha3 \
		  soc_ifc \
		  caliptra_rtl_lib \
		  clk_gate \
		  wdt
COMP_LIBS := $(foreach name, $(COMP_LIB_NAMES), $(CALIPTRA_ROOT)/src/integration/test_suites/libs/$(name))

# Optional per-test makefile fragment for extra objects/libs/headers
-include $(TEST_DIR)/$(TESTNAME).mk

AUX_HEADER_FILES ?=
HEADER_FILES := $(INCLUDES_DIR)/caliptra_defines.h \
		$(INCLUDES_DIR)/defines.h \
		$(CLP_REG_DIR)/caliptra_reg.h  \
		$(RISCV_HW_IF_DIR)/riscv_hw_if.h \
		$(wildcard $(TEST_DIR)/*.h) \
		$(PRINTF_DIR)/printf.h \
		$(ISR_DIR)/riscv-csr.h \
		$(ISR_DIR)/riscv-interrupts.h \
		$(ISR_DIR)/veer-csr.h \
		$(AUX_HEADER_FILES) \
		$(foreach comp_lib, $(COMP_LIBS), $(wildcard $(comp_lib)/*.h))

TEST_GEN_FILES := $(CALIPTRA_ROOT)/src/ecc/tb/ecc_secp384r1.exe \
	          $(CALIPTRA_ROOT)/src/doe/tb/doe_test_gen.py \
	          $(CALIPTRA_ROOT)/src/sha256/tb/sha256_wntz_test_gen.py \
	          $(CALIPTRA_ROOT)/submodules/adams-bridge/src/abr_top/uvmf/Dilithium_ref/dilithium/ref/test/test_dilithium5 \
	          $(CALIPTRA_ROOT)/submodules/adams-bridge/src/abr_top/uvmf/Dilithium_ref/dilithium/ref/test/test_dilithium5_debug \
	          $(CALIPTRA_ROOT)/src/mldsa/tb/smoke_test_mldsa_vector.hex \
	          $(CALIPTRA_ROOT)/submodules/adams-bridge/src/abr_top/uvmf/ML_KEM_ref/ml-kem

# Separate OFILE variable since this is not used to build remote images
# (i.e. FMC or RunTime)
ifeq (0,$(shell test -e $(TEST_DIR)/$(TESTNAME).c && echo $$?))
	OFILE_CRT := crt0.o
endif
AUX_OFILES ?=
OFILES += $(AUX_OFILES)
OFILES += $(TESTNAME).o
ifeq (0,$(shell test -e $(TEST_DIR)/caliptra_isr.h && echo $$?))
	OFILES += caliptra_isr.o
endif
# Assume all tests will import our printf lib
OFILES += printf.o
# Always compile the lib files - for every target
OFILES += $(foreach comp_lib_name, $(COMP_LIB_NAMES), $(comp_lib_name).o)

# Verilator trace format
VERILATOR_TRACE=vcd

ifdef debug
 DEBUG_PLUS = +dumpon
 IRUN_DEBUG = -access +rc
 IRUN_DEBUG_RUN = -input $(RV_ROOT)/testbench/input.tcl
 VCS_DEBUG = -debug_access
 ifeq ($(VERILATOR_TRACE), vcd)
  VERILATOR_DEBUG = --trace --trace-structs
 else ifeq ($(VERILATOR_TRACE), fst)
  VERILATOR_DEBUG = --trace-fst --trace-structs --trace-threads 2
 else
  $(error Unsupported Verilator trace format '$(VERILATOR_TRACE)')
 endif
 RIVIERA_DEBUG = +access +r
endif

ifdef coverage
COVERAGE_ARGS = -cm line+cond+fsm+tgl+branch+assert -cm_hier $(CALIPTRA_ROOT)/src/integration/coverage/config/caliptra_top_tb_cm_hier.cfg
endif

# RK
# provide specific link file
ifeq (,$(wildcard $(TEST_DIR)/$(TESTNAME).ld))
	LINK = $(RISCV_HW_IF_DIR)/link.ld
else
	LINK = $(TEST_DIR)/$(TESTNAME).ld
endif

# AUX_LIB_DIR can be used to specify additional source build directories to supplement the default search
# areas for C source code
# It should be used in conjunction with overrides for the AUX_OFILES variable on the command line invocation
AUX_LIB_DIR ?=
VPATH = $(TEST_DIR) $(BUILD_DIR) $(TBDIR) $(RISCV_HW_IF_DIR) $(ISR_DIR) $(PRINTF_DIR) $(COMP_LIBS) $(AUX_LIB_DIR)

# Use eval to expand env variables in the .vf file
$(eval TBFILES = $(shell cat $(TBDIR)/../config/caliptra_top_tb.vf | grep -v '+incdir+'))

# Default VFs; overridden to a VeeR-stripped version for FB builds (see ifeq FB_HAL below)
VERILATOR_VF = $(TBDIR)/../config/caliptra_top_tb.vf
# VCS always needs Axi4PC.sv stripped (it has #!/bin/sh -h stub under `ifndef VERILATOR)
VCS_BASE_VF  = $(BUILD_DIR)/caliptra_top_tb_vcs_base.vf
VCS_VF       = $(VCS_BASE_VF)

#defines  = $(BUILD_DIR)/common_defines.vh
#defines += $(RV_ROOT)/design/include/el2_def.sv
#defines += $(BUILD_DIR)/el2_pdef.vh
includes = -I$(BUILD_DIR)
#
#         .------------------------------------------------------------------------------------------------------------.
#         | Necessary to avoid a warning that is thrown in reference to a change in beh_lib.sv. beh_lib clkhdr         |
#         | definition was upgraded to remove an `ifdef VERILATOR, with a special negedge-triggered always block,      |
#         | which produces a latch element that Verilator warns about.                                                 |
#         '---------------------------------------------------------------------------------------------------------+--'
#                                                                                                                   |
#         .---------------------------------------------------------------------------------------------.           |
#         | Addresses a warning thrown in newer versions of Verilator regarding 'force' statements      |           |
#         | used in caliptra_top_tb_services.                                                           |           |
#         '------------------------------------------------------------------------------------------+--'           |
#                                                                                                    |              |
#         .--------------------------------------------------------------------------------.         |              |
#         | Verilator does not support unpacked structs/unions - waive this warning so     |         |              |
#         | that unpacked structs are treated as packed. This should only impact files     |         |              |
#         | generated from RDL register files, which use unpacked structs liberally.       |         |              |
#         '-----------------------------------------------------------------------------+--'         |              |
#                                                                                       |            |              |
#         .---------------------------------------------------------------------.       |            |              |
#         | This is flagged for every generated SystemVerilog file defining a   |       |            |              |
#         | register block. The field_storage signal is the target of the       |       |            |              |
#         | warning, indicating multiple drivers/different clocking.            |       |            |              |
#         '----------------------------------------------------------------+----'       |            |              |
#                                                                          |            |            |              |
#         .------------------------------------------------------.         |            |            |              |
#         | PeakRDL implements the default incrsaturate behavior |         |            |            |              |
#         | for counter registers by performing a > comparison   |         |            |            |              |
#         | with an all-ones value (i.e. cnt > 32'hFFFFFFFF)     |         |            |            |              |
#         | which, by definition, can never be true. Verilator   |         |            |            |              |
#         | warns that this will compile to a constant 0.        |         |            |            |              |
#         '------------------------------------------------+-----'         |            |            |              |
#                                                          |               |            |            |              |
#         .--------------------------------------------.   |               |            |            |              |
#         | Vectors that are little bit-endian are not |   |               |            |            |              |
#         | supported by Verilator, according to a     |   |               |            |            |              |
#         | warning that is thrown. This is supposedly |   |               |            |            |              |
#         | resolved as of version 3.720, according    |   |               |            |            |              |
#         | to the release notes.                      |   |               |            |            |              |
#         '-----------------------------------+--------'   |               |            |            |              |
#                                             |            |               |            |            |              |
#         .------------------------------.    |            |               |            |            |              |
#         | Inherited from ChipsAlliance |    |            |               |            |            |              |
#         | VeeR repository provided     |    |            |               |            |            |              |
#         | Makefile                     |    |            |               |            |            |              |
#         '-----+------------+-----------'    |            |               |            |            |              |
#               |            |                |            |               |            |            |              |
#               v            v                v            v               v            v            v              v
#                                                                                                -Wno-SIDEEFFECT
suppress = -Wno-WIDTH -Wno-WIDTHTRUNC -Wno-UNOPTFLAT -Wno-LITENDIAN -Wno-CMPCONST -Wno-MULTIDRIVEN -Wno-UNPACKED -Wno-SIDEEFFECT -Wno-MISINDENT -Wno-LATCH

# CFLAGS for verilator generated Makefiles. Without -std=c++17 it
# complains for `auto` variables
CFLAGS += -std=c++17

# Optimization for better performance; alternative is nothing for
# slower runtime (faster compiles) -O2 for faster runtime (slower
# compiles), or -O for balance.
VERILATOR_MAKE_FLAGS = OPT_FAST="-Os"

# Testbench DPI sources
TB_DPI_SRCS = jtagdpi/jtagdpi.c \
              tcp_server/tcp_server.c

TB_DPI_INCS := $(addprefix -I$(CALIPTRA_ROOT)/src/integration/test_suites/libs/,$(dir $(TB_DPI_SRCS)))
TB_DPI_SRCS := $(addprefix $(CALIPTRA_ROOT)/src/integration/test_suites/libs/,$(TB_DPI_SRCS))

# Testbench sources
TB_VERILATOR_SRCS = $(TBDIR)/test_caliptra_top_tb.cpp $(TB_DPI_SRCS)
TB_VERILATOR_EXE_SRCS = test_caliptra_top_tb.cpp
TB_VERILATOR_EXE_DEPS =
FB_ROOT ?= $(abspath $(CALIPTRA_ROOT)/../firebridge)

# FireBridge HAL (Mode B, REAL firmware, AHB-only): VeeR held in reset and
# fb_ahb_vip drives the internal AHB bus in its place; the tb's normal soc_bfm
# boots Caliptra over the external AXI exactly as in non-FB runs. It links the
# UNMODIFIED test firmware, compiled natively as C, into the verilated
# executable. The firmware's lsu_*/printf/SEND_STDOUT_CTRL are redirected through
# the FB HAL headers (firebridge/fb_hal/*) into ahb.cpp, which drives the
# internal AHB bus.
$(VCS_BASE_VF): $(TBDIR)/../config/caliptra_top_tb.vf
	awk '/Axi4PC\.sv/ {next} {print}' $< > $@

FB_HAL ?= 0
# FB=1 is a convenience alias for FB_HAL=1
ifeq ($(FB),1)
FB_HAL := 1
endif
ifeq ($(FB_HAL),1)
CFLAGS += -DCALIPTRA_FB_AHB -DSIM -I$(FB_ROOT)/h -I$(CLP_REG_DIR)
TB_VERILATOR_EXE_SRCS += $(FB_ROOT)/c/ahb.cpp
TB_VERILATOR_EXE_DEPS += $(FB_ROOT)/c/ahb.cpp
FB_HAL_RTL_SRCS = $(FB_ROOT)/sv/fb_ahb_vip.sv \
                  $(FB_ROOT)/sv/el2_veer_wrapper_stub.sv
# Strip the heavy VeeR RTL implementation files from the VF; keep only the
# small definitions that non-VeeR code still needs (el2_pkg, el2_mem_if,
# common_defines) plus the +incdir+ entries.  VeeR is ifdef'd out under
# CALIPTRA_FB_AHB so Verilator never elaborates it — this just avoids parsing
# ~50 large RTL files, cutting compile time by ~40-50%.
FB_VF     = $(BUILD_DIR)/caliptra_top_tb_noveer.vf
VCS_FB_VF = $(BUILD_DIR)/caliptra_top_tb_vcs_fb.vf
VERILATOR_VF = $(FB_VF)
VCS_VF       = $(VCS_FB_VF)

$(FB_VF): $(TBDIR)/../config/caliptra_top_tb.vf
	awk '/veer_el2\/rtl\// && \
	     !/el2_pdef\.vh/ && \
	     !/el2_def\.sv/ && \
	     !/common_defines\.sv/ && \
	     !/el2_mem_if\.sv/ && \
	     !/mem_lib\.sv/ && \
	     !/beh_lib\.sv/ && \
	     !/\+incdir\+/ \
	     {next} {print}' $< > $@

# VCS FB VF: strips VeeR RTL AND Axi4PC.sv (which has an intentional
# #!/bin/sh syntax-error stub guarded by `ifndef VERILATOR — harmless to
# exclude since Axi4PC is only instantiated under `ifdef AXI4PC).
$(VCS_FB_VF): $(TBDIR)/../config/caliptra_top_tb.vf
	awk '/veer_el2\/rtl\// && \
	     !/el2_pdef\.vh/ && \
	     !/el2_def\.sv/ && \
	     !/common_defines\.sv/ && \
	     !/el2_mem_if\.sv/ && \
	     !/mem_lib\.sv/ && \
	     !/beh_lib\.sv/ && \
	     !/\+incdir\+/ \
	     {next} /Axi4PC\.sv/ {next} {print}' $< > $@
# Firmware sources compiled natively (as C) and linked into the executable.
# FB_FW_LIB_SRCS / FB_FW_LIB_DIRS let each test pull in the crypto lib(s) it uses.
FB_FW_CC ?= gcc
FB_FW_LIB_SRCS ?= $(CALIPTRA_ROOT)/src/integration/test_suites/libs/sha256/sha256.c
FB_FW_LIB_DIRS ?= $(CALIPTRA_ROOT)/src/integration/test_suites/libs/sha256
# Upstream firmware (test + crypto libs): compiled with the FB HAL headers first
# and entry points renamed so they neither break C++ rules (void main) nor
# clobber libc symbols Verilator's runtime relies on (printf/putchar/puts/stdout).
FB_FW_UPSTREAM_SRCS = $(TEST_DIR)/$(TESTNAME).c $(FB_FW_LIB_SRCS)
# FireBridge-side sources: provide the fw_* sinks + init_interrupts stub, using
# the host C library normally, so they are compiled WITHOUT the renames.
FB_FW_FB_SRCS = $(FB_ROOT)/c/printf.c $(FB_ROOT)/c/isr.c
FB_FW_SRCS = $(FB_FW_UPSTREAM_SRCS) $(FB_FW_FB_SRCS)
FB_FW_OBJS = $(foreach s,$(FB_FW_SRCS),$(BUILD_DIR)/fbfw_$(notdir $(s:.c=.o)))
FB_FW_RENAMES = -Dmain=fw_main -Dstdout=fw_stdout \
                -Dprintf=fw_printf -Dputchar=fw_putchar -Dputs=fw_puts
FB_ALL_LIB_DIRS := $(wildcard $(CALIPTRA_ROOT)/src/integration/test_suites/libs/*)
FB_FW_INCS = -I$(FB_ROOT)/c -I$(FB_ROOT)/h -I$(INCLUDES_DIR) -I$(CLP_REG_DIR) \
             -I$(TEST_DIR) $(foreach d,$(FB_FW_LIB_DIRS),-I$(d)) \
             -include $(FB_ROOT)/h/wfi_stub.h \
             $(foreach d,$(FB_ALL_LIB_DIRS),-I$(d))
FB_FW_CFLAGS ?=          # extra C flags for firmware compilation (e.g. -DCALIPTRA_INTERNAL_TRNG)
FB_FW_STAMP = $(BUILD_DIR)/.fbfw_built
FB_HAL_LDFLAGS = -LDFLAGS "$(FB_FW_OBJS)"

$(FB_FW_STAMP): $(FB_FW_SRCS) $(FB_ROOT)/c/riscv_hw_if.h \
                $(FB_ROOT)/h/printf.h $(HEADER_FILES)
	@set -e; for s in $(FB_FW_UPSTREAM_SRCS); do \
	  o=$(BUILD_DIR)/fbfw_$$(basename $${s%.c}).o; \
	  echo "[FB_HAL] cc (upstream) $$s -> $$o"; \
	  $(FB_FW_CC) -std=gnu11 -g -O2 -c -DFIREBRIDGE $(FB_FW_RENAMES) $(FB_FW_INCS) $(FB_FW_CFLAGS) $$s -o $$o; \
	done; \
	SHA3_DEFINE=$$(echo "$(FB_FW_LIB_SRCS)" | grep -q sha3 && echo "-DFIREBRIDGE_HAS_SHA3" || true); \
	for s in $(FB_FW_FB_SRCS); do \
	  o=$(BUILD_DIR)/fbfw_$$(basename $${s%.c}).o; \
	  echo "[FB_HAL] cc (fb) $$s -> $$o"; \
	  $(FB_FW_CC) -std=gnu11 -g -O2 -c -DFIREBRIDGE $$SHA3_DEFINE $(FB_FW_INCS) $$s -o $$o; \
	done
	@touch $@
endif

# Testbench defs
TB_DEFS =
TB_DEFS += +define+CALIPTRA_TOP_TB
TB_DEFS += +define+CALIPTRA_TOP=caliptra_top_tb
TB_DEFS += +define+RV_TOP=caliptra_top_tb.caliptra_top_dut.rvtop
ifeq ($(FB_HAL),1)
TB_DEFS += +define+CALIPTRA_FB_AHB
endif
ifeq ($(CALIPTRA_INTERNAL_TRNG), 1)
TB_DEFS += +define+CALIPTRA_INTERNAL_TRNG
HWCFG_CFLAGS += -DCALIPTRA_HWCONFIG_TRNG_EN
endif
ifeq ($(CALIPTRA_MODE_SUBSYSTEM), 1)
TB_DEFS += +define+CALIPTRA_MODE_SUBSYSTEM
HWCFG_CFLAGS += -DCALIPTRA_HWCONFIG_SUBSYSTEM_MODE
endif

# By default debugging (JTAG) is locked in Caliptra. Add "DEBUG_UNLOCKED=1" to
# enable it.
ifdef DEBUG_UNLOCKED
    TB_DEFS += +define+CALIPTRA_DEBUG_UNLOCKED
endif

# To enforce holding the RISC-V core in reset add "FORCE_CPU_RESET=1".
ifdef FORCE_CPU_RESET
    TB_DEFS += +define+CALIPTRA_FORCE_CPU_RESET
endif

# Run time arguments from command line
VERILATOR_RUN_ARGS ?= ""

# Add testbench lib include paths
CFLAGS += $(TB_DPI_INCS)

# Targets
all: clean verilator

clean:
	rm -rf *.log *.s *.hex *.dis *.size *.tbl irun* vcs* simv* .map *.map snapshots \
	verilator* *.exe obj* *.o ucli.key vc_hdrs.h csrc *.csv work \
	dataset.asdb  library.cfg vsimsa.cfg  riviera-build wave.asdb sim.vcd \
	*.h

clean_fw:
	rm -rf *.o *.h

############ Model Builds ###############################

verilator-build: $(TBFILES) $(INCLUDES_DIR)/defines.h $(TB_VERILATOR_SRCS) $(TB_VERILATOR_EXE_DEPS) $(FB_HAL_RTL_SRCS) $(FB_FW_STAMP) $(VERILATOR_VF)
	set -eo pipefail; $(VERILATOR) $(TB_VERILATOR_SRCS) $(FB_HAL_RTL_SRCS) --cc -CFLAGS "$(CFLAGS)" \
	  +libext+.v+.sv +define+RV_OPENSOURCE \
	  --timescale 1ns/100ps \
	  --timing \
	  $(includes) \
	  $(suppress) \
	  -f $(VERILATOR_VF) --top-module caliptra_top_tb \
	  -f $(TBDIR)/../config/caliptra_top_tb.vlt \
	  -exe $(TB_VERILATOR_EXE_SRCS) --autoflush $(VERILATOR_DEBUG) \
	  $(FB_HAL_LDFLAGS) \
	  $(TB_DEFS) 2>&1 | tee verilator_build.log
	set -eo pipefail; $(MAKE) -j`nproc` -e -C obj_dir/ -f Vcaliptra_top_tb.mk $(VERILATOR_MAKE_FLAGS) VM_PARALLEL_BUILDS=1 2>&1 | tee verilator_make.log
	touch verilator-build

vcs-build: $(TBFILES) $(INCLUDES_DIR)/defines.h $(TB_DPI_SRCS) $(FB_HAL_RTL_SRCS) $(FB_FW_STAMP) $(VCS_BASE_VF) $(VCS_VF)
	vlogan -full64 -sverilog -kdb -incr_vlogan +lint=IA_CHECKFAIL -assert svaext \
	  +define+CLP_ASSERT_ON +define+ABR_ASSERT_ON +define+SIMULATION +define+VCS $(TB_DEFS) -noinherit_timescale=1ns/1ps \
	  -f $(VCS_VF) \
	  $(FB_HAL_RTL_SRCS)
	vcs -full64 -kdb -lca -debug_access+all -j8 +vcs+lic+wait -partcomp -fastpartcomp=j8 $(COVERAGE_ARGS) \
	  -assert enable_hier caliptra_top_tb -o simv.caliptra_top_tb +dpi \
	  -cflags "$(TB_DPI_INCS) $(if $(filter 1,$(FB_HAL)),-DCALIPTRA_FB_AHB -DSIM -I$(FB_ROOT)/h -I$(CLP_REG_DIR))" \
	  $(TB_DPI_SRCS) \
	  $(if $(filter 1,$(FB_HAL)),$(FB_ROOT)/c/ahb.cpp $(FB_FW_OBJS))

############ TEST Simulation ###############################

verilator: program.hex verilator-build
	set -eo pipefail; ./obj_dir/Vcaliptra_top_tb $(VERILATOR_RUN_ARGS) 2>&1 | tee verilator_sim.log

vcs: program.hex vcs-build
	cp -r $(TEST_GEN_FILES) $(BUILD_DIR)
	./simv.caliptra_top_tb $(COVERAGE_ARGS)

############ TEST build ###############################

ifeq ($(shell which $(GCC_PREFIX)-gcc 2> /dev/null),)
program.hex: $(BUILD_DIR)/defines.h
	@echo " !!! No $(GCC_PREFIX)-gcc in path, using canned hex files !!"
	cp $(HEX_DIR)/$(TESTNAME).hex program.hex
else ifneq (,$(wildcard $(TEST_DIR)/$(TESTNAME).makefile))
program.hex:
	@echo Building $(TESTNAME) via $(TEST_DIR)/$(TESTNAME).makefile
	$(MAKE) -f $(TEST_DIR)/$(TESTNAME).makefile
else ifneq (,$(wildcard $(TEST_DIR)/$(TESTNAME)))
# Build program.hex from RUST executable
program.hex:
	@echo "Building program.hex from $(TESTNAME) using Crypto Test rules for pre-compiled RUST executables"
	-$(GCC_PREFIX)-objcopy -O verilog -R .data -R .rodata -R .bss -R .sbss -R .iccm -R .dccm -R .eh_frame --pad-to 0x18000 --gap-fill 0xFF --no-change-warnings $(TEST_DIR)/$(TESTNAME) program.hex
	-$(GCC_PREFIX)-objcopy -O verilog -j .data -j .rodata -j .bss -j .sbss -j .dccm \
			      --change-section-lma       .data-0x50000000 \
			      --change-section-lma     .rodata-0x50000000 \
			      --change-section-lma        .bss-0x50000000 \
			      --change-section-lma       .sbss-0x50000000 \
			      --change-section-lma       .dccm-0x50000000 \
			      --pad-to 0x40000 \
			      --no-change-warnings \
			      $(TEST_DIR)/$(TESTNAME) dccm.hex
	-$(GCC_PREFIX)-objcopy -O verilog -j .iccm                       --change-section-address .iccm=0 --pad-to 0x40000 --no-change-warnings $(TEST_DIR)/$(TESTNAME) iccm.hex
	-$(GCC_PREFIX)-objcopy -O verilog -j .mailbox --gap-fill 0x0     --change-section-address .mailbox=0 --pad-to 0x40000 --no-change-warnings $(TEST_DIR)/$(TESTNAME) mailbox.hex
	$(GCC_PREFIX)-objdump -S  $(TEST_DIR)/$(TESTNAME) > $(TESTNAME).dis
	$(GCC_PREFIX)-size        $(TEST_DIR)/$(TESTNAME) | tee $(TESTNAME).size

else
# Build program.hex from source (test_suites)
program.hex: $(OFILE_CRT) $(OFILES) $(LINK)
	@echo Building $(TESTNAME)
	$(GCC_PREFIX)-gcc $(ABI) -Wl,-Map=$(TESTNAME).map -lgcc -T$(LINK) -o $(TESTNAME).exe $(OFILE_CRT) $(OFILES) -nostartfiles  $(TEST_LIBS)
	-$(GCC_PREFIX)-objcopy -O verilog -R .data_iccm0 -R .data_iccm1 -R .data_iccm2 -R .iccm -R .dccm -R .eh_frame --pad-to 0x18000 --no-change-warnings $(TESTNAME).exe program.hex
	-$(GCC_PREFIX)-objcopy -O verilog -j .data_iccm0 -j .data_iccm1 -j .data_iccm2 -j .dccm \
			      --change-section-lma .data_iccm0-0x50000000 \
			      --change-section-lma .data_iccm1-0x50000000 \
			      --change-section-lma .data_iccm2-0x50000000 \
			      --change-section-lma       .dccm-0x50000000 \
			      --pad-to 0x40000 \
			      --no-change-warnings \
			      $(TESTNAME).exe dccm.hex
	-$(GCC_PREFIX)-objcopy -O verilog -j .iccm                       --change-section-address .iccm=0 --pad-to 0x40000 --no-change-warnings $(TESTNAME).exe iccm.hex
	-$(GCC_PREFIX)-objcopy -O verilog -j .mailbox --gap-fill 0x0     --change-section-address .mailbox=0 --pad-to 0x40000 --no-change-warnings $(TESTNAME).exe mailbox.hex
	$(GCC_PREFIX)-objdump -S  $(TESTNAME).exe > $(TESTNAME).dis
	$(GCC_PREFIX)-size        $(TESTNAME).exe | tee $(TESTNAME).size
	@echo Completed building $(TESTNAME)

# Cannot build caliptra_{fmc,rt}.hex and program.hex in the same make invocation (TESTNAME will be invalid, and consequently other dependencies like LINK)
# Entire image will be loaded into the mailbox, so it must fit the 128KiB memory
%.hex: $(OFILES) $(LINK)
	@echo Building $(TESTNAME)
	$(GCC_PREFIX)-gcc $(ABI) -Wl,-Map=$(TESTNAME).map -lgcc -T$(LINK) -o $(TESTNAME).exe $(OFILES) -nostartfiles  $(TEST_LIBS)
	-$(GCC_PREFIX)-objcopy -O verilog -R .eh_frame --pad-to 0x40000 --no-change-warnings $(TESTNAME).exe $@
	$(GCC_PREFIX)-objdump -S  $(TESTNAME).exe > $(TESTNAME).dis
	$(GCC_PREFIX)-size        $(TESTNAME).exe | tee $(TESTNAME).size
	@echo Completed building $(TESTNAME)

%.o : %.s $(HEADER_FILES)
	cp $(HEADER_FILES) $(BUILD_DIR)
	$(GCC_PREFIX)-cpp $(includes) -I$(TEST_DIR)  $<  > $*.cpp.s
	$(GCC_PREFIX)-as $(ABI) $*.cpp.s -o $@

%.o : %.c $(HEADER_FILES)
	cp $(HEADER_FILES) $(BUILD_DIR)
	$(GCC_PREFIX)-gcc $(includes) $(TEST_CFLAGS) $(HWCFG_CFLAGS) -DCOMPILER_FLAGS="\"$(TEST_CFLAGS)\"" $(ABI) -nostdlib -c $< -o $@

endif

help:
	@echo Make sure the environment variable RV_ROOT is set.
	@echo Possible targets: verilator vcs irun vlog riviera help clean all verilator-build irun-build vcs-build riviera-build program.hex

.PHONY: help clean clean_fw verilator vcs irun vlog riviera
