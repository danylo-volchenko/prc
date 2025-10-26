# rule.mk: Generic Build Rules
# ------------------------------------------------------------------------------
# Implicit Rules & Variables
# ------------------------------------------------------------------------------
# Define generic linking command
LINK.o = $(CXX)

# Dependency generation flags
DEPFLAGS := -MMD -MP -MF

# ------------------------------------------------------------------------------
# Generic Build Rules
# ------------------------------------------------------------------------------

# Rule to create directories needed by the build
$(TARGET_DIR) $(DEP_DIR) $(OBJ_DIR):
	@mkdir -p $@

# Generic rule for compiling a C++ source file to an object file.
$(OBJ_DIR)/%.o : $(SRC_DIR)/%.cpp | $(OBJ_DIR) $(DEP_DIR)
	@echo -e "$(GREEN)[Compiling]$(RESET) $<"
	$(CXX) -c $(CXXFLAGS) $(DEPFLAGS) \
		$(DEP_DIR)/$*.d $< -o $@

# Precompiled header rule
ifeq ($(USE_PCH),yes)
PCH_FILE := $(INCLUDE_DIR)/pch.hpp.gch
PCH_CXXFLAGS := -x c++-header -Winvalid-pch
$(PCH_FILE): $(INCLUDE_DIR)/pch.hpp
	@echo -e "$(GREEN)[Generating PCH]$(RESET) $@"
	$(CXX) $(PCH_CXXFLAGS) $(filter-out -Werror,$(CXXFLAGS)) \
		$< -o $@
.PHONY pch: $(PCH_FILE)
else
.PHONY pch: ; # An empty rule for 'pch'
endif

# ------------------------------------------------------------------------------
# Generic Targets
# ------------------------------------------------------------------------------

# Generic clean rule that uses the CLEAN_FILES variable from the main Makefile.
.PHONY: clean
clean:
	@echo -e "$(PURPLE)--- Cleaning Project ---$(RESET)"
	@rm -rf $(CLEAN_FILES)
	@echo -e "$(GREEN)[Clean Complete]$(RESET)"

# ------------------------------------------------------------------------------
# Dependency Inclusion
# ------------------------------------------------------------------------------
ifneq ($(MAKECMDGOALS),clean)
	-include $(DEPS)
endif
