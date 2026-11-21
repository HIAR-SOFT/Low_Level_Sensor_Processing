# Recipes start with '>' instead of a TAB (GNU Make >= 3.82) so copy/paste is safe.
.RECIPEPREFIX := >

CC      := gcc
NASM    := nasm
BUILD   := build
CFLAGS  := -std=c11 -Wall -Wextra -Wpedantic -g -Iinclude -ffp-contract=off
LDLIBS  := -lm
DEPFLAGS := -MMD -MP

KERNEL_SRC := $(addprefix c_implementation/,moving_average.c rms.c peak_detection.c statistics.c)
UTIL_SRC   := c_implementation/monitor.c c_implementation/data_io.c
ASM_SRC    := $(addprefix assembly/,moving_average.asm rms.asm peak_detection.asm statistics.asm)

KOBJ     = $(patsubst c_implementation/%.c,$(BUILD)/$(1)/%.o,$(KERNEL_SRC))
UTIL_OBJ := $(patsubst c_implementation/%.c,$(BUILD)/util/%.o,$(UTIL_SRC))
ASM_OBJ  := $(patsubst assembly/%.asm,$(BUILD)/asm/%.o,$(ASM_SRC))
HEADERS  := $(wildcard include/*.h)

BIN := $(BUILD)/bench_o0 $(BUILD)/bench_o2 $(BUILD)/bench_o3 \
       $(BUILD)/sp_cli $(BUILD)/test_o0 $(BUILD)/test_o2

.PHONY: all test test-c test-py data bench clean
all: $(BIN)

# ---- kernels compiled at three optimisation levels ----
$(BUILD)/o0/%.o: c_implementation/%.c
> @mkdir -p $(@D)
> $(CC) $(CFLAGS) $(DEPFLAGS) -O0 -c $< -o $@
$(BUILD)/o2/%.o: c_implementation/%.c
> @mkdir -p $(@D)
> $(CC) $(CFLAGS) $(DEPFLAGS) -O2 -c $< -o $@
$(BUILD)/o3/%.o: c_implementation/%.c
> @mkdir -p $(@D)
> $(CC) $(CFLAGS) $(DEPFLAGS) -O3 -c $< -o $@

# ---- non-benchmarked helpers (always -O2) ----
$(BUILD)/util/%.o: c_implementation/%.c
> @mkdir -p $(@D)
> $(CC) $(CFLAGS) $(DEPFLAGS) -O2 -c $< -o $@

# ---- NASM ----
$(BUILD)/asm/%.o: assembly/%.asm
> @mkdir -p $(@D)
> $(NASM) -f elf64 -g -F dwarf $< -o $@

# ---- executables (harness/test/CLI code is always -O2) ----
$(BUILD)/bench_o0: benchmarks/benchmark.c $(HEADERS) $(call KOBJ,o0) $(ASM_OBJ) $(UTIL_OBJ)
> $(CC) $(CFLAGS) -O2 $< $(filter %.o,$^) -o $@ $(LDLIBS)
$(BUILD)/bench_o2: benchmarks/benchmark.c $(HEADERS) $(call KOBJ,o2) $(ASM_OBJ) $(UTIL_OBJ)
> $(CC) $(CFLAGS) -O2 $< $(filter %.o,$^) -o $@ $(LDLIBS)
$(BUILD)/bench_o3: benchmarks/benchmark.c $(HEADERS) $(call KOBJ,o3) $(ASM_OBJ) $(UTIL_OBJ)
> $(CC) $(CFLAGS) -O2 $< $(filter %.o,$^) -o $@ $(LDLIBS)

$(BUILD)/sp_cli: tools/sp_cli.c $(HEADERS) $(call KOBJ,o2) $(ASM_OBJ) $(UTIL_OBJ)
> $(CC) $(CFLAGS) -O2 $< $(filter %.o,$^) -o $@ $(LDLIBS)

$(BUILD)/test_o0: tests/test_algorithms.c $(HEADERS) $(call KOBJ,o0) $(ASM_OBJ) $(UTIL_OBJ)
> $(CC) $(CFLAGS) -O2 $< $(filter %.o,$^) -o $@ $(LDLIBS)
$(BUILD)/test_o2: tests/test_algorithms.c $(HEADERS) $(call KOBJ,o2) $(ASM_OBJ) $(UTIL_OBJ)
> $(CC) $(CFLAGS) -O2 $< $(filter %.o,$^) -o $@ $(LDLIBS)

# ---- convenience targets ----
test-c: $(BUILD)/test_o0 $(BUILD)/test_o2
> ./$(BUILD)/test_o0
> ./$(BUILD)/test_o2
test-py: $(BUILD)/sp_cli
> python3 -m unittest discover -s tests -p 'test_correctness.py' -v
test: test-c test-py

data:
> python3 python/generate_sensor_data.py --samples 1000000 --scenario mixed

bench: all
> bash benchmarks/run_benchmarks.sh

clean:
> rm -rf $(BUILD)

-include $(wildcard $(BUILD)/*/*.d)
