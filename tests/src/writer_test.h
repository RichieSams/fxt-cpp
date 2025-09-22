/* FXT - A library for creating Fuschia Tracing System (FXT) files
 *
 * FXT is the legal property of Adrian Astley
 * Copyright Adrian Astley 2023
 */

#pragma once

#include "fxt/writer.h"
#include "record_args.h"

namespace fxt {

// Define functions that are implemented in writer.cpp so we can test them directly

int GetOrCreateStringIndex(Writer *writer, const char *str, uint16_t *strIndex);
int GetOrCreateThreadIndex(Writer *writer, KernelObjectID processID, KernelObjectID threadID, uint16_t *threadIndex);

int ProcessArgs(Writer *writer, const RecordArgument *args, size_t numArgs, ProcessedRecordArgument *processedArgs);
int WriteArg(Writer *writer, const RecordArgument *arg, ProcessedRecordArgument *processedArg, unsigned *wordsWritten);

} // End of namespace fxt
