#pragma once
/**
 * @file typedefs.hpp
 * @brief Type aliases for fixed-width integer types.
 */
#if defined(__cplusplus)
#include <cstdint>

namespace typedefs {

/* Signed integer aliases */
using s8  = std::int8_t;
using s16 = std::int16_t;
using s32 = std::int32_t;
using s64 = std::int64_t;

/* Unsigned integer aliases */
using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using usize = std::size_t;
using ssize = std::ptrdiff_t;

} /* namespace typedefs */
#elif !defined(__cplusplus)
#include <stdint.h>
#include <stddef.h>
/* Signed integer aliases */
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

/* Unsigned integer aliases */
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef size_t usize;
typedef ptrdiff_t ssize;
#endif
