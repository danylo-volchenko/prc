#!/usr/bin/env zsh

# A simple C/C++ project manager script

# --- Configuration ---
readonly COLOR_RED="\e[31m"
readonly COLOR_GREEN="\e[32m"
readonly COLOR_YELLOW="\e[33m"
readonly COLOR_BLUE="\e[34m"
readonly COLOR_CYAN="\e[1;36m"
readonly COLOR_RESET="\e[0m"
readonly TEMPLATE_DIR="${XDG_CONFIG_HOME:-$HOME/.config}/prc/template"
readonly MAKEFILE="Makefile"
readonly CONFIG_MK="config.mk"
readonly RULE_MK="rule.mk"

msg_info() { echo -e "${COLOR_BLUE}$*${COLOR_RESET}"; }
msg_success() { echo -e "${COLOR_GREEN}$*${COLOR_RESET}"; }
msg_error() { echo -e "${COLOR_RED}Error: $*${COLOR_RESET}" >&2; }
msg_warning() { echo -e "${COLOR_YELLOW}$*${COLOR_RESET}"; }

usage() {
    cat <<EOF
Usage: prc <command> [options]

Commands:
  new <project_name>   - Create a new project from the template.
  build [make_args...] - Compile the project using make.
  run [program_args...]  - Run the compiled executable (builds if needed).
  set-std <std>        - Change the C/C++ standard (e.g., c++23, c17).
  help                 - Show this help message.
EOF
}

check_build_files_exist() {
    local missing_files=()
    [[ ! -f "$MAKEFILE" ]] && missing_files+=("$MAKEFILE")
    [[ ! -f "$CONFIG_MK" ]] && missing_files+=("$CONFIG_MK")
    [[ ! -f "$RULE_MK" ]] && missing_files+=("$RULE_MK")

    if (( ${#missing_files[@]} > 0 )); then
        msg_error "Required build file(s) not found: ${missing_files[*]}. Are you in a project directory?"
        return 1
    fi
    return 0
}

get_target_name() {
    check_build_files_exist || return 1
    grep '^TARGET *:=' "$CONFIG_MK" | cut -d '=' -f2 | xargs
}

_configure_for_c() {
    msg_info "Configuring build system for C..."

    # Ensure files exist before modifying
    check_build_files_exist || return 1

    # --- Changes in config.mk ---
    sed -i 's/CXX ?= g++/CC = gcc/' "./$CONFIG_MK"
    sed -i 's/CXX_MSAN ?= clang++/CC_MSAN = clang/' "./$CONFIG_MK"
    sed -i 's/CXXFLAGS_BASE/CFLAGS_BASE/' "./$CONFIG_MK"

    # --- Changes in rule.mk ---
    sed -i 's/\$(CXX)/\$(CC)/g' "./$RULE_MK"
    sed -i 's/\$(CXXFLAGS)/\$(CFLAGS)/g' "./$RULE_MK"
    sed -i 's/PCH_CXXFLAGS/PCH_CFLAGS/g' "./$RULE_MK"
    sed -i 's/-x c++-header/-x c-header/' "./$RULE_MK"
    sed -i '/\$(OBJ_DIR)\/%.o :/ s/\.cpp/\.c/' "./$RULE_MK"
    sed -i 's/\$(INCLUDE_DIR)\/pch.hpp/\$(INCLUDE_DIR)\/pch.h/' "./$RULE_MK"

    # --- Changes in the main Makefile ---
    sed -i 's/\*.cpp/\*.c/' "./$MAKEFILE"
    sed -i '/^OBJS =/ s/\.cpp/\.c/' "./$MAKEFILE"
    sed -i '/^DEPS =/ s/\.cpp/\.c/' "./$MAKEFILE"
    sed -i 's/override CXXFLAGS/override CFLAGS/g' "./$MAKEFILE"
    sed -i 's/CXXFLAGS_BASE/CFLAGS_BASE/g' "./$MAKEFILE"
    sed -i 's/CXX := \$(CXX_MSAN)/CC := \$(CC_MSAN)/' "./$MAKEFILE"

    # --- Rename template files ---
    msg_info "Renaming template files to .c/.h..."
    mv src/main.cpp src/main.c 2>/dev/null || true
    mv include/pch.hpp include/pch.h 2>/dev/null || true
    mv include/util/typedefs.hpp include/util/typedefs.h 2>/dev/null || true
    mv include/util/logger.hpp include/util/logger.h 2>/dev/null || true

    # --- Update includes in source files ---
    msg_info "Updating #include directives in source files..."
    local src_files=("src/main.c" "include/pch.h")
    for f in "${src_files[@]}"; do
        if [[ -f "$f" ]]; then
            sed -i 's/\(pch\|logger\|typedefs\)\.hpp/\1.h/g' "$f"
        fi
    done
    msg_success "C configuration complete."
    return 0
}

_configure_for_cpp() {
    msg_info "Configuring build system for C++..."
    check_build_files_exist || return 1

    # Config
    sed -i 's/CC = gcc/CXX ?= g++/' "./$CONFIG_MK"
    sed -i 's/CC_MSAN = clang/CXX_MSAN ?= clang++/' "./$CONFIG_MK"
    sed -i 's/CFLAGS_BASE/CXXFLAGS_BASE/' "./$CONFIG_MK"

    # Rules
    sed -i 's/\$(CC)/\$(CXX)/g' "./$RULE_MK"
    sed -i 's/\$(CFLAGS)/\$(CXXFLAGS)/g' "./$RULE_MK"
    sed -i 's/PCH_CFLAGS/PCH_CXXFLAGS/g' "./$RULE_MK"
    sed -i 's/-x c-header/-x c++-header/' "./$RULE_MK"
    sed -i '/\$(OBJ_DIR)\/%.o :/ s/\.c/\.cpp/' "./$RULE_MK"
    sed -i 's/\$(INCLUDE_DIR)\/pch.h/\$(INCLUDE_DIR)\/pch.hpp/' "./$RULE_MK"

    # Main Makefile
    sed -i 's/\*.c/\*.cpp/' "./$MAKEFILE"
    sed -i '/^OBJS =/ s/\.c/\.cpp/' "./$MAKEFILE"
    sed -i '/^DEPS =/ s/\.c/\.cpp/' "./$MAKEFILE"
    sed -i 's/override CFLAGS/override CXXFLAGS/g' "./$MAKEFILE"
    sed -i 's/CFLAGS_BASE/CXXFLAGS_BASE/g' "./$MAKEFILE"
    sed -i 's/CC := \$(CC_MSAN)/CXX := \$(CXX_MSAN)/' "./$MAKEFILE"

    # Rename files
    msg_info "Renaming files to .cpp/.hpp..."
    mv src/main.c src/main.cpp 2>/dev/null || true
    mv include/pch.h include/pch.hpp 2>/dev/null || true

    # Update includes
    msg_info "Updating #include directives..."
    local src_files=("src/main.cpp" "include/pch.hpp")
    for f in "${src_files[@]}"; do
        if [[ -f "$f" ]]; then
            sed -i 's/\(pch\)\.h/\1.hpp/g' "$f"
        fi
    done
    msg_success "C++ configuration complete."
}

# --- Command Functions ---
cmd_new() {
    local dirname="$1"
    if [[ -z "$dirname" ]]; then
        msg_error "Project name is required."
        echo "Usage: prc new <project_name>" >&2
        return 1
    fi

    if [[ -e "$dirname" ]]; then
        msg_error "Directory '$dirname' already exists."
        return 1
    fi

    if [[ ! -d "$TEMPLATE_DIR" ]]; then
        msg_error "Template directory not found at '$TEMPLATE_DIR'"
        return 1
    fi

    msg_info "Creating project directory: '$dirname'..."
    mkdir -p "$dirname" || return 1

    msg_info "Copying template files..."
    cp -r "$TEMPLATE_DIR"/* "$dirname" || return 1
    cp -r "$TEMPLATE_DIR"/.* "$dirname" 2>/dev/null || true # Ignore errors for dotfiles

    msg_info "Initializing Git repository..."
    ( cd "$dirname" && git init --initial-branch=main >/dev/null ) || return 1

    # Use subshell for modifications within the new project dir
    (
        cd "$dirname" || return 1
        msg_info "Configuring Makefile target..."
        sed -i "s/^TARGET := .*/TARGET := $dirname/" "./$CONFIG_MK"

        echo -n "Set C/C++ standard (e.g. c++20, c17, gnu17) [optional]: "
        read -r std
        if [[ -n "$std" ]]; then
            cmd_set_std "$std" || return 1
        fi
        msg_success "Project '$dirname' created successfully."
        msg_warning "Build system is GNU MAKE"
        echo "To get started: cd $dirname"
    )
    local subshell_status=$?
    [[ $subshell_status -ne 0 ]] && return $subshell_status

    # --- Direnv Integration ---
    local envrc_file="${dirname}/.envrc"
    if [[ -f "$envrc_file" ]]; then
        echo
        read -q "ans?Found an .envrc file. Inspect and grant permissions? [y/N] "
        echo
        if [[ "$ans" =~ ^[Yy]$ ]]; then
            echo "\n${COLOR_BLUE}--- .envrc Content ---${COLOR_RESET}"
            cat -n "$envrc_file"
            echo "${COLOR_BLUE}------------------------${COLOR_RESET}\n"

            read -q "ans?Confirm and allow 'direnv' to load this file? [y/N] "
            echo
            if [[ "$ans" != "n" && "$ans" != "N" ]]; then
                direnv allow "$envrc_file"
                msg_success "NOTICE: Granted ${envrc_file} loading."
            else
                msg_warning "NOTICE: Permission denied."
            fi
        fi
    fi
    return 0
}

# Builds the project in the current directory
cmd_build() {
    set -o pipefail
    check_build_files_exist || return 1

    msg_info "--- Starting Build ---"
    local start_time=$(date +%s.%N)

    script -q -c "/usr/bin/env make $*" /dev/null
    local ext_status=$?

    local end_time=$(date +%s.%N)
    local duration=$((end_time - start_time)) # Zsh handles float arithmetic
    local formatted_duration
    if (( duration < 60 )); then
        formatted_duration=$(printf "%.2fs" "$duration")
    else
        local minutes=$(( integer(duration / 60) ))
        local seconds=$(( duration - (minutes * 60) ))
        formatted_duration=$(printf "%dm %.2fs" "$minutes" "$seconds")
    fi

    if [[ $ext_status -ne 0 ]]; then
        msg_warning "Build ${COLOR_RED}failed${COLOR_RESET} ${COLOR_CYAN}in ${formatted_duration}${COLOR_RESET} on: $(date +'%Y/%m/%d %H:%M:%S')"
    else
        echo -e "Build ${COLOR_GREEN}completed successfully${COLOR_RESET} ${COLOR_CYAN}in ${formatted_duration}${COLOR_RESET} on: $(date +'%Y/%m/%d %H:%M:%S')"
    fi
    return $ext_status
}

# Runs the compiled executable
cmd_run() {
    local target=$(get_target_name)
    local target_status=$?
    [[ $target_status -ne 0 ]] && return $target_status

    local executable_path="./$target" # Assume target is in current dir after build symlink

    # Check if first argument is 'rebuild' or if executable doesn't exist/run
    if [[ "$1" == "rebuild" ]] || [[ ! -x "$executable_path" ]]; then
        # If 'rebuild' was the argument, remove it before passing to make
        local make_args=("$@")
        [[ "$1" == "rebuild" ]] && make_args=("$@[2,-1]")

        msg_info "Executable '$target' not found or rebuild requested. Building first..."
        cmd_build "${make_args[@]}" # Pass remaining args to make
        local build_status=$?
        [[ $build_status -ne 0 ]] && return $build_status
    fi

    # Remove 'rebuild' if it was the first argument before passing to executable
    local run_args=("$@")
    [[ "$1" == "rebuild" ]] && run_args=("$@[2,-1]")

    if [[ ! -x "$executable_path" ]]; then
        msg_error "Executable '$target' still not found or not executable after build."
        return 1
    fi

    msg_info "--- Running '$target' ---"
    "$executable_path" "${run_args[@]}"
    local run_status=$?
    echo "${COLOR_RESET}--- Process exited with code: ${COLOR_YELLOW}$run_status${COLOR_RESET} ---"
    return $run_status
}

# Changes the C/C++ standard in the Makefile system
cmd_set_std() {
    local std="$1"
    if [[ -z "$std" ]]; then
        msg_error "Standard is required."
        echo "Usage: prj set-std <new_standard>" >&2
        return 1
    fi

    check_build_files_exist || return 1

    sed -i "s/-std=[a-zA-Z0-9+]\+/-std=$std/" "./$CONFIG_MK"
    local sed_status=$?
    [[ $sed_status -ne 0 ]] && msg_error "Failed to update standard in $CONFIG_MK" && return $sed_status

    # If the standard is C, ensure the build system is configured for C
    if [[ "$std" =~ ^(c|gnu)[0-9]+ ]]; then
        # Check current compiler setting
        local current_compiler=$(grep '^CC *=' "$CONFIG_MK" | cut -d '=' -f2 | xargs)
        if [[ "$current_compiler" != "gcc" ]]; then
             _configure_for_c || return 1
        else
            msg_info "Build system already configured for C."
        fi
    else # Standard is C++
        # Potentially add logic here to revert back to C++ settings if needed
        # For now, just setting the std flag is enough
        msg_info "C++ standard detected. Ensuring standard flag is set..."
        _configure_for_cpp || return 1
    fi

    msg_success "Build system updated to use standard: $std"
    return 0
}

# --- Main Dispatcher ---
if [[ $# -eq 0 ]]; then
    usage
    exit 0
fi

cmd="$1"
shift

case "$cmd" in
    new)      cmd_new "$@" ;;
    build)    cmd_build "$@" ;;
    run)      cmd_run "$@" ;;
    set-std)  cmd_set_std "$@" ;;
    help|--help|-h) usage ;;
    *)
        msg_error "Unknown command '$cmd'"
        usage
        exit 1
        ;;
esac

exit $?
