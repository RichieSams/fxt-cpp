/* FXT - A library for creating Fuschia Tracing System (FXT) files
 *
 * FXT is the legal property of Adrian Astley
 * Copyright Adrian Astley 2023
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Gets a specific field from a given bitfield value
 *
 * @param begin    The bit the field starts at
 * @param end      The bit the field ends at (inclusive)
 * @param value    The bitfield to read from
 * @return         The field value
 */
inline uint64_t GetFieldFromValue(uint64_t begin, uint64_t end, uint64_t value) {
	const uint64_t mask = (uint64_t(1) << (end - begin + 1)) - uint64_t(1);
	return (value >> begin) & mask;
}

/**
 * @brief Reads a little-endian UInt64 value from the given byte array
 *
 * @param array    The start of the byte array
 * @return         The read value
 */
inline uint64_t ReadUInt64(uint8_t *array) {
	return (uint64_t)array[0] |
	       (uint64_t)array[1] << 8 |
	       (uint64_t)array[2] << 16 |
	       (uint64_t)array[3] << 24 |
	       (uint64_t)array[4] << 32 |
	       (uint64_t)array[5] << 40 |
	       (uint64_t)array[6] << 48 |
	       (uint64_t)array[7] << 56;
}
