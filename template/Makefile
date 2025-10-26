# This Makefile is designed to build a C/C++ project with multiple build profiles.
# It supports:
# - Release builds.
# - Debug builds with optional sanitizers (generic, address, thread, memory).
#
# Usage:
#   make [target] [PROFILE=...] [SANITIZER=...]
#   or set the variables inside config.mk
#
# Key Variables:
#   PROFILE   : Build profile (debug or release). Default: debug
#   SANITIZER : For debug profile only (none, address, thread, memory). Default: none
#
# Examples:
#   make                 # Builds debug (default)
#   make PROFILE=release # Builds release
#   make PROFILE=debug SANITIZER=address # Builds debug with address sanitizer
#
# Other targets: all, clean, linter, compdb, help
#
# Notes:
# - Parallel builds enabled by default using all CPU cores.
# - Precompiled headers (PCH) used in release for faster compilation.
# - Separate build directories for each profile/flavor.
# ------------------------------------------------------------------------------
# Configuration
# ------------------------------------------------------------------------------
include config.mk
# ------------------------------------------------------------------------------
# Project-Specific File Definitions
# ------------------------------------------------------------------------------
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
# ------------------------------------------------------------------------------
# Profile & Flavor Logic (The "Glue")
# ------------------------------------------------------------------------------
# Compute flavor based on profile and sanitizer
ifeq ($(PROFILE),release)
	FLAVOR := release
else
	FLAVOR := debug-$(SANITIZER)
endif

# Derived paths based on flavor
OBJ_DIR := $(BUILD_DIR)/$(OBJ_FILES_DIR)/$(FLAVOR)
TARGET_DIR := $(BUILD_DIR)/$(TARGETS_DIR)/$(FLAVOR)
DEP_DIR := $(BUILD_DIR)/deps
OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))
DEPS = $(patsubst $(SRC_DIR)/%.cpp,$(DEP_DIR)/%.d,$(SRCS))
CLEAN_FILES := $(BUILD_DIR) $(TARGET) release debug-*

# Base CXXFLAGS (common to all)
override CXXFLAGS := $(CXXFLAGS_BASE)
override CXXFLAGS += -I$(INCLUDE_DIR) $(WFLAGS)

# Base LDFLAGS (common to all)
override LDFLAGS := $(addprefix -l,$(LIBS))

# Apply profile-specific flags
ifeq ($(PROFILE),release)
	override CXXFLAGS += -march=native -O3 -flto -DUSE_PCH
	override LDFLAGS += -march=native -O3 -flto
	USE_PCH := yes
else
	override CXXFLAGS += $(DFLAGS)
ifeq ($(SANITIZER),address)
	override LDFLAGS += $(SANITIZE_ADDRESS)
endif
ifeq ($(SANITIZER),thread)
	override LDFLAGS += $(SANITIZE_THREAD)
endif
ifeq ($(SANITIZER),memory)
	CXX := $(CXX_MSAN)
	override LDFLAGS += $(SANITIZE_MEMORY)
endif
	USE_PCH := no
endif

# ------------------------------------------------------------------------------
# Include Generic Build Rules
# ------------------------------------------------------------------------------
include rule.mk
# ------------------------------------------------------------------------------
# Project-Specific Targets
# ------------------------------------------------------------------------------
.DEFAULT_GOAL := build

.PHONY: build
build::
	@echo -e "$(PURPLE)--- Building Project ($(FLAVOR)) ---$(RESET)"
build:: pch .WAIT $(OBJS) | $(TARGET_DIR)
	$(LINK.o) $(OBJS) -o $(TARGET_DIR)/$(TARGET) $(LDFLAGS)
ifdef STRIP
		@echo -e "$(YELLOW)[Stripping]$(RESET) $(TARGET_DIR)/$(TARGET)"
		strip --strip-unneeded --preserve-dates $(TARGET_DIR)/$(TARGET)
endif
	@echo -e "$(BLUE)[Post-Build]$(RESET) Symlinking"
	@ln -sfn $(TARGET_DIR) $(FLAVOR)
	@ln -sfn $(TARGET_DIR)/$(TARGET) $(TARGET)
	@echo -e "$(GREEN)[Build Complete]$(RESET) Profile: $(PROFILE), Flavor: $(FLAVOR)"
	@if [[ -f "$(LOGFILE)" || -f "$(DEBUG_LOGFILE)" ]]; then \
		if [ "$(PROFILE)" = "release" ]; then \
			echo -e "\n$(YELLOW)Remove previous log? $(LOGFILE) [Y/n]$(RESET)"; \
			read -t 5 -p "" ans; \
			if ! [[ "$$ans" =~ ^[nN]$$ ]]; then rm -f "$(LOGFILE)"; fi; \
			else \
				echo -e "\n$(YELLOW)Remove previous log? $(DEBUG_LOGFILE) [Y/n]$(RESET)"; \
				read -t 5 -p "" ans; \
			if ! [[ "$$ans" =~ ^[nN]$$ ]]; then rm -f "$(DEBUG_LOGFILE)"; fi; \
			fi; \
		fi

.PHONY: all
all:
	@echo -e "$(PURPLE)--- Building All Variants ---$(RESET)"
	@$(MAKE) PROFILE=release build
	@$(MAKE) PROFILE=debug SANITIZER=none build
	@$(MAKE) PROFILE=debug SANITIZER=address build
	@$(MAKE) PROFILE=debug SANITIZER=thread build
	@$(MAKE) PROFILE=debug SANITIZER=memory build
	@$(MAKE) compdb

# Generate compilation database
.PHONY: compdb
compdb:
	@echo -e "$(PURPLE)--- Generating Compile Database ---$(RESET)"
	@command -v bear >/dev/null || { echo -e "$(RED)Error: bear not found!$(RESET)"; exit 1; }
	@mkdir -p $(BUILD_DIR)
	@bear --output $(BUILD_DIR)/compile_commands.json -- make PROFILE=release build -B
	@sed -i '/-DUSE_PCH/d' $(BUILD_DIR)/compile_commands.json
	@echo -e "$(GREEN)[CompDB Complete]$(RESET)"

# Run static analysis tool
.PHONY: linter
linter:
	@echo -e "$(PURPLE)--- Running Linter ---$(RESET)"
	cppcheck --check-level=exhaustive --enable=all --inconclusive --verbose \
		--suppress=missingIncludeSystem --suppress=unusedFunction \
		-I $(INCLUDE_DIR) .

# Help: Display available targets
.PHONY: help
help:
	@echo -e "$(PURPLE)--- Makefile Help ---$(RESET)"
	@echo -e "$(BLUE)Available Targets:$(RESET)"
	@echo "  build         : Build the project (default, uses PROFILE and SANITIZER)"
	@echo "  all           : Build release and all debug variants + compdb"
	@echo "  compdb        : Generate compile_commands.json (requires bear)"
	@echo "  linter        : Run cppcheck linter"
	@echo "  clean         : Remove all build artifacts"
	@echo "  help          : Show this help message"
	@echo -e "\n$(BLUE)Variables:$(RESET)"
	@echo "  PROFILE       : debug (default) or release"
	@echo "  SANITIZER     : For debug - none (default), address, thread, memory"

# Sort config.mk to delete repetitions
.PHONY: sort_config
sort_config: $(CONFIG_FILE)
	$(info SortConfig...)
	$(SORTER_TOOL) -u $(CONFIG_FILE) -o $(CONFIG_FILE)
