/* FXT - A library for creating Fuschia Tracing System (FXT) files
 *
 * FXT is the legal property of Adrian Astley
 * Copyright Adrian Astley 2023
 */

#pragma once

#include <stdint.h>

#define FXT_MAX_NUM_ARGS 15

namespace fxt {

struct ProcessedRecordArgument {
	uint16_t nameStringRef;
	uint16_t nameSizeInWords;

	uint16_t valueStringRef;
	uint16_t headerAndValueSizeInWords;
};

} // End of namespace fxt
