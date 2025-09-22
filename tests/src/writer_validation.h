/* FXT - A library for creating Fuschia Tracing System (FXT) files
 *
 * FXT is the legal property of Adrian Astley
 * Copyright Adrian Astley 2023
 */

#pragma once

#include <stdint.h>

inline uint64_t GetFieldFromValue(uint64_t begin, uint64_t end, uint64_t value) {
	const uint64_t mask = (uint64_t(1) << (end - begin + 1)) - uint64_t(1);
	return (value >> begin) & mask;
}