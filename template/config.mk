# config.mk: Project Configuration
# ------------------------------------------------------------------------------
# Project & Build Profile
# ------------------------------------------------------------------------------
TARGET := draft
PROFILE ?= debug
SANITIZER ?= none

# ------------------------------------------------------------------------------
# Tools & Compilers
# ------------------------------------------------------------------------------
CXX ?= g++
CXX_MSAN ?= clang++
SORTER_TOOL ?= sort

# Enable parallel builds using number of CPU cores
NPROC := $(shell nproc)
MAKEFLAGS += -j$(NPROC)

# ------------------------------------------------------------------------------
# Directories & files
# ------------------------------------------------------------------------------
SRC_DIR := ./src
INCLUDE_DIR := ./include
BUILD_DIR := ./build
OBJ_FILES_DIR := objs
TARGETS_DIR := target

LOGFILE ?= log.txt
DEBUG_LOGFILE ?= gdb.txt
# ------------------------------------------------------------------------------
# Base Compiler & Linker Flags
# ------------------------------------------------------------------------------
CXXFLAGS_BASE := -std=c++23
WFLAGS := -Wall -Wextra -Wpedantic -Werror
DFLAGS := -O0 -ggdb3 -fno-omit-frame-pointer

# Libraries
LIBS :=

# Sanitizer definitions
SANITIZE_UB := undefined shift alignment bounds enum return unreachable object-size null vptr
SANITIZE_ADDRESS := -fsanitize=address $(addprefix -fsanitize=,$(SANITIZE_UB))
SANITIZE_THREAD := -fsanitize=thread
SANITIZE_MEMORY := -fsanitize=memory -fsanitize-memory-track-origins=2

# ------------------------------------------------------------------------------
# Cosmetics (ANSI Color Codes)
# ------------------------------------------------------------------------------
RESET :=
GREEN :=
YELLOW :=
BLUE :=
PURPLE :=
RED :=
ifeq ($(NO_COLOR),)
  ifneq ($(shell tput colors 2>/dev/null),)
    RESET := \033[0m
    GREEN := \033[1;32m
    YELLOW := \033[1;33m
    BLUE := \033[1;34m
    PURPLE := \033[1;35m
    RED := \033[1;31m
  endif
endif
