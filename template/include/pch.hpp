#pragma once /* pch */

#if defined(__cplusplus)
/* threading */
// #include <condition_variable>
// #include <future>
// #include <mutex>

/* containers */
// #include <deque>
// #include <string>
// #include <unordered_map>
// #include <vector>
// #include <string_view>

/* utility */
// #include <chrono>
// #include <expected>
// #include <format>
// #include <optional>
// #include <ranges>
// #include <source_location>
// #include <charconv>

/* IO */
// #include <filesystem>
// #include <fstream>
// #include <iostream>
// #include <sstream>
#elif !defined(__cplusplus)

/* network */
// #include <netdb.h>
// #include <unistd.h>
// #include <sys/fcntl.h>
// #include <sys/socket.h>
#endif

/* Project specific */
#include "util/logger.hpp"
