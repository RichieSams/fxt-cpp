/* FXT - A library for creating Fuschia Tracing System (FXT) files
 *
 * FXT is the legal property of Adrian Astley
 * Copyright Adrian Astley 2023
 */

#pragma once

#include "fxt/writer.h"

namespace fxt {

int GetOrCreateStringIndex(Writer *writer, const char *str, uint16_t *strIndex);
int GetOrCreateThreadIndex(Writer *writer, KernelObjectID processID, KernelObjectID threadID, uint16_t *threadIndex);

unsigned GetArgSizeInWords(const RecordArgument *args, size_t numArgs);
int WriteArg(Writer *writer, const RecordArgument *arg, unsigned *wordsWritten);

} // End of namespace fxt
