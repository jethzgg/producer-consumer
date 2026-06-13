CC := gcc

BASE_CFLAGS := -Wall -Wextra -Wpedantic -Werror -std=c11 -pthread \
	-D_POSIX_C_SOURCE=200809L -Iincludes
CFLAGS ?= $(BASE_CFLAGS)
LDFLAGS ?=

TARGET := producer_consumer
BUILD_DIR := build
TEST_TARGET := $(BUILD_DIR)/test_suite

APP_SOURCES := \
	src/main.c \
	src/cli.c \
	src/scenario.c \
	src/simulation.c \
	src/bounded_buffer.c \
	src/worker.c \
	src/statistics.c \
	src/logger.c

CORE_SOURCES := $(filter-out src/main.c src/cli.c,$(APP_SOURCES))
APP_OBJECTS := $(APP_SOURCES:src/%.c=$(BUILD_DIR)/%.o)
CORE_OBJECTS := $(CORE_SOURCES:src/%.c=$(BUILD_DIR)/%.o)

TEST_SOURCES := \
	tests/test_main.c \
	tests/test_scenario.c \
	tests/test_bounded_buffer.c \
	tests/test_statistics.c \
	tests/test_logger.c \
	tests/test_simulation.c
TEST_OBJECTS := $(TEST_SOURCES:tests/%.c=$(BUILD_DIR)/tests/%.o)

COVERAGE_FLAGS := -fprofile-instr-generate -fcoverage-mapping
COVERAGE_CC ?= clang
LLVM_PROFDATA := $(shell command -v llvm-profdata 2>/dev/null)
LLVM_COV := $(shell command -v llvm-cov 2>/dev/null)
ifeq ($(LLVM_PROFDATA),)
LLVM_PROFDATA := xcrun llvm-profdata
endif
ifeq ($(LLVM_COV),)
LLVM_COV := xcrun llvm-cov
endif

.PHONY: all run test coverage clean

all: $(TARGET)

$(TARGET): $(APP_OBJECTS)
	$(CC) $(APP_OBJECTS) -o $@ $(LDFLAGS) -pthread

$(TEST_TARGET): $(CORE_OBJECTS) $(TEST_OBJECTS)
	$(CC) $(CORE_OBJECTS) $(TEST_OBJECTS) -o $@ $(LDFLAGS) -pthread

$(BUILD_DIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR)/tests/%.o: tests/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

run: all
	./$(TARGET)

test: all $(TEST_TARGET)
	$(TEST_ENV) ./$(TEST_TARGET)
	$(TEST_ENV) sh tests/test_cli.sh ./$(TARGET)

coverage:
	$(MAKE) clean
	@mkdir -p $(BUILD_DIR)/coverage
	$(MAKE) test \
		CC="$(COVERAGE_CC)" \
		CFLAGS="$(BASE_CFLAGS) $(COVERAGE_FLAGS)" \
		LDFLAGS="$(COVERAGE_FLAGS)" \
		TEST_ENV="LLVM_PROFILE_FILE=$(BUILD_DIR)/coverage/%p.profraw"
	$(LLVM_PROFDATA) merge -sparse $(BUILD_DIR)/coverage/*.profraw \
		-o $(BUILD_DIR)/coverage/coverage.profdata
	$(LLVM_COV) report ./$(TEST_TARGET) \
		-instr-profile=$(BUILD_DIR)/coverage/coverage.profdata \
		-object=./$(TARGET) -ignore-filename-regex='tests/' \
		> $(BUILD_DIR)/coverage/report.txt
	@cat $(BUILD_DIR)/coverage/report.txt
	@awk '/TOTAL/ { value=$$10; gsub("%", "", value); \
		if (value + 0 < 80) { \
			printf "Line coverage %.2f%% is below 80%%\n", value; exit 1; \
		} \
	}' $(BUILD_DIR)/coverage/report.txt

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

-include $(APP_OBJECTS:.o=.d) $(TEST_OBJECTS:.o=.d)
