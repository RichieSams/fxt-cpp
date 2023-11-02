/* FXT - A library for creating Fuschia Tracing System (FXT) files
 *
 * FXT is the legal property of Adrian Astley
 * Copyright Adrian Astley 2023
 */

#pragma once

#include "fxt/writer.h"

namespace fxt {

class WriterTest {
public:
	WriterTest(Writer *writer)
	        : m_writer(writer) {
	}

private:
	Writer *m_writer;

public:
	int GetOrCreateStringIndex(const char *str, uint16_t *strIndex) {
		return m_writer->GetOrCreateStringIndex(str, strIndex);
	}
	int GetOrCreateThreadIndex(KernelObjectID processID, KernelObjectID threadID, uint16_t *threadIndex) {
		return m_writer->GetOrCreateThreadIndex(processID, threadID, threadIndex);
	}
};

} // End of namespace fxt
