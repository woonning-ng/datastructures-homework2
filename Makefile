CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -pedantic
SHELL := bash
TARGET := simplify
SRC := main.cpp Helper.cpp math.cpp Polygon.cpp Simplifier.cpp SpatialGrid.cpp
OBJ := $(SRC:.cpp=.o)

# Test settings
TEST_DIR := test_cases
GEN_DIR := generated_outputs
TIMEOUT_SECONDS ?= 30

# Colors for output
YELLOW := \033[1;33m
GREEN  := \033[0;32m
RED    := \033[0;31m
NC     := \033[0m

# Input:TargetPairs
TEST_CASES := \
    input_rectangle_with_two_holes.csv:11 \
    input_cushion_with_hexagonal_hole.csv:13 \
    input_blob_with_two_holes.csv:17 \
    input_wavy_with_three_holes.csv:21 \
    input_lake_with_two_islands.csv:17 \
    input_original_01.csv:99 \
    input_original_02.csv:99 \
    input_original_03.csv:99 \
    input_original_04.csv:99 \
    input_original_05.csv:99 \
    input_original_06.csv:99 \
    input_original_07.csv:99 \
    input_original_08.csv:99 \
    input_original_09.csv:99 \
    input_original_10.csv:99

define PYTHON_VALIDATOR
import re, sys
target = int(sys.argv[1])
with open(sys.argv[2], "r", encoding="utf-8") as f:
    lines = f.read().splitlines()
if not lines or lines[0].strip() != "ring_id,vertex_id,x,y": sys.exit(1)
v_lines, rings, area_in, area_out = [], {}, None, None
for l in lines[1:]:
    if "Total signed area in input:" in l:
        m = re.search(r"([-+0-9.eE]+)$$", l)
        if m: area_in = float(m.group(1))
    elif "Total signed area in output:" in l:
        m = re.search(r"([-+0-9.eE]+)$$", l)
        if m: area_out = float(m.group(1))
    elif "," in l:
        try:
            parts = l.split(',')
            rid = int(parts[0])
            rings[rid] = rings.get(rid, 0) + 1
            v_lines.append(l)
        except: continue
if not v_lines: sys.exit(1)
if len(v_lines) > max(target, 3 * len(rings)): sys.exit(1)
if any(c < 3 for c in rings.values()): sys.exit(1)
if area_in is None or area_out is None: sys.exit(1)
if abs(area_in - area_out) > 1e-6 * max(1.0, abs(area_in)): sys.exit(1)
endef

.PHONY: all clean run test

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: $(TARGET)
	@mkdir -p $(GEN_DIR)
	@# Create the python script with a literal $ (using $$$$)
	@echo 'import re, sys' > .val.py
	@echo 'target = int(sys.argv[1])' >> .val.py
	@echo 'with open(sys.argv[2], "r", encoding="utf-8") as f: lines = f.read().splitlines()' >> .val.py
	@echo 'if not lines or lines[0].strip() != "ring_id,vertex_id,x,y": sys.exit(1)' >> .val.py
	@echo 'v_lines, rings, area_in, area_out = [], {}, None, None' >> .val.py
	@echo 'for l in lines[1:]:' >> .val.py
	@echo '    if "Total signed area in input:" in l: m = re.search(r"([-+0-9.eE]+)$$$$", l); area_in = float(m.group(1)) if m else None' >> .val.py
	@echo '    elif "Total signed area in output:" in l: m = re.search(r"([-+0-9.eE]+)$$$$", l); area_out = float(m.group(1)) if m else None' >> .val.py
	@echo '    elif "," in l:' >> .val.py
	@echo '        try:' >> .val.py
	@echo '            rid = int(l.split(",")[0]); rings[rid] = rings.get(rid, 0) + 1; v_lines.append(l)' >> .val.py
	@echo '        except: continue' >> .val.py
	@echo 'if not v_lines or len(v_lines) > max(target, 3 * len(rings)) or any(c < 3 for c in rings.values()): sys.exit(1)' >> .val.py
	@echo 'if area_in is None or area_out is None or abs(area_in - area_out) > 1e-6 * max(1.0, abs(area_in)): sys.exit(1)' >> .val.py
	@echo -e "$(YELLOW)==> Running Geometric Validation Tests...$(NC)"
	@failed=0; total=0; \
	for test in $(TEST_CASES); do \
		input=$${test%%:*}; \
		vtx=$${test#*:}; \
		input_path=$(TEST_DIR)/$$input; \
		output_path=$(GEN_DIR)/$${input/input_/output_}; \
		output_path=$${output_path%.csv}.txt; \
		if [ ! -f $$input_path ]; then continue; fi; \
		total=$$((total+1)); \
		echo -ne "$(YELLOW)RUN$(NC)  $$input (target=$$vtx) ... "; \
		timeout $(TIMEOUT_SECONDS)s ./$(TARGET) $$input_path $$vtx > $$output_path 2>/dev/null; \
		if [ $$? -ne 0 ]; then \
			echo -e "$(RED)FAIL (Crash/Timeout)$(NC)"; failed=$$((failed+1)); \
		elif python3 .val.py $$vtx $$output_path; then \
			echo -e "$(GREEN)PASS$(NC)"; \
		else \
			echo -e "$(RED)FAIL (Logic Error)$(NC)"; failed=$$((failed+1)); \
		fi; \
	done; \
	rm -f .val.py; \
	echo "==============================="; \
	if [ $$failed -eq 0 ]; then \
		echo -e "$(GREEN)All $$total tests passed!$(NC)"; \
	else \
		echo -e "$(RED)$$failed / $$total tests failed.$(NC)"; exit 1; \
	fi
	
clean:
	rm -f $(TARGET) $(OBJ)
	rm -rf $(GEN_DIR)