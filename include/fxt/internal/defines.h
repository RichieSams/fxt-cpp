/* FXT - A library for creating Fuschia Tracing System (FXT) files
 *
 * FXT is the legal property of Adrian Astley
 * Copyright Adrian Astley 2023
 */

#pragma once

#include <inttypes.h>

namespace fxt {

using ProviderID = uint32_t;
using KernelObjectID = uint64_t;

namespace internal {

using StringRef = uint16_t;
using ThreadRef = uint16_t;

}; // End of namespace internal

} // End of namespace fxt
