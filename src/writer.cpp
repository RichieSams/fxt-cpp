/* FXT - A library for creating Fuschia Tracing System (FXT) files
 *
 * FXT is the legal property of Adrian Astley
 * Copyright Adrian Astley 2023
 */

#include "fxt/writer.h"

#define XXH_INLINE_ALL
#include "xxhash.h"

#include <algorithm>
#include <limits>
#include <string.h>

namespace fxt {

// Helpers

template <class T, uint16_t N>
static constexpr uint16_t ArraySize(T (&)[N]) {
	return N;
}

// Writer methods

Writer::Writer(void *userContext, WriteFunc writeFunc)
        : m_userContext(userContext),
          m_writeFunc(writeFunc) {
}

Writer::~Writer() {
}

int Writer::WriteMagicNumberRecord() {
	const char fxtMagic[] = {0x10, 0x00, 0x04, 0x46, 0x78, 0x54, 0x16, 0x00};
	return WriteBytesToStream(fxtMagic, ArraySize(fxtMagic));
}

int Writer::AddProviderInfoRecord(uint32_t providerID, const char *providerName) {
	const size_t strLen = strlen(providerName);
	const size_t paddedStrLen = (strLen + 8 - 1) & (-8);
	const size_t diff = paddedStrLen - strLen;

	if (paddedStrLen >= std::numeric_limits<uint8_t>::max()) {
		return FXT_ERR_STR_TOO_LONG;
	}

	// Write the header
	const uint64_t sizeInWords = 1 + (paddedStrLen / 8);
	const uint64_t header = ((uint64_t)(strLen) << 52) | ((uint64_t)(providerID) << 20) | ((uint64_t)(internal::MetadataType::ProviderInfo) << 16) | ((uint64_t)(sizeInWords) << 4) | (uint64_t)(internal::RecordType::Metadata);
	int ret = WriteUInt64ToStream(header);
	if (ret != 0) {
		return ret;
	}

	// Then the string data
	ret = WriteBytesToStream(providerName, strLen);
	if (ret != 0) {
		return ret;
	}

	// And the zero padding
	if (diff > 0) {
		ret = WriteZeroPadding(diff);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

int Writer::AddProviderSectionRecord(uint32_t providerID) {
	const uint64_t sizeInWords = 1;
	const uint64_t header = ((uint64_t)(providerID) << 20) | ((uint64_t)(internal::MetadataType::ProviderSection) << 16) | ((uint64_t)(sizeInWords) << 4) | (uint64_t)(internal::RecordType::Metadata);
	int ret = WriteUInt64ToStream(header);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

int Writer::AddProviderEventRecord(uint32_t providerID, ProviderEventType eventType) {
	const uint64_t sizeInWords = 1;
	const uint64_t header = ((uint64_t)(eventType) << 52) | ((uint64_t)(providerID) << 20) | ((uint64_t)(internal::MetadataType::ProviderEvent) << 16) | ((uint64_t)(sizeInWords) << 4) | (uint64_t)(internal::RecordType::Metadata);
	int ret = WriteUInt64ToStream(header);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

int Writer::AddInitializationRecord(uint64_t numTicksPerSecond) {
	const uint64_t sizeInWords = 2;
	const uint64_t header = ((uint64_t)(sizeInWords) << 4) | (uint64_t)(internal::RecordType::Initialization);
	int ret = WriteUInt64ToStream(header);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream(numTicksPerSecond);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

int Writer::SetProcessName(KernelObjectID processID, const char *name) {
	uint16_t nameIndex;
	int ret = GetOrCreateStringIndex(name, &nameIndex);
	if (ret != 0) {
		return ret;
	}

	// Write the header
	const uint64_t sizeInWords = /* header */ 1 + /* processID */ 1;
	const uint64_t numArgs = 0;
	const uint64_t header = ((uint64_t)(numArgs) << 40) | ((uint64_t)(nameIndex) << 24) | ((uint64_t)(internal::KOIDType::Process) << 16) | ((uint64_t)(sizeInWords) << 4) | (uint64_t)(internal::RecordType::KernelObject);
	ret = WriteUInt64ToStream(header);
	if (ret != 0) {
		return ret;
	}

	// Then the process ID
	ret = WriteUInt64ToStream(processID);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

int Writer::SetThreadName(KernelObjectID processID, KernelObjectID threadID, const char *name) {
	uint16_t nameIndex;
	int ret = GetOrCreateStringIndex(name, &nameIndex);
	if (ret != 0) {
		return ret;
	}

	KOIDEventArgument processArg("process", processID);
	ret = processArg.InitStringEntries(this);
	if (ret != 0) {
		return ret;
	}

	const unsigned argSizeInWords = processArg.ArgumentSizeInWords();
	const uint64_t sizeInWords = /* header */ 1 + /* threadID */ 1 + /* argument data */ argSizeInWords;
	const uint64_t numArgs = 1;
	const uint64_t header = ((uint64_t)(numArgs) << 40) | ((uint64_t)(nameIndex) << 24) | ((uint64_t)(internal::KOIDType::Thread) << 16) | ((uint64_t)(sizeInWords) << 4) | (uint64_t)(internal::RecordType::KernelObject);
	ret = WriteUInt64ToStream(header);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream(threadID);
	if (ret != 0) {
		return ret;
	}

	// Write KIOD Argument to reference the process ID
	unsigned wordsWritten;
	ret = processArg.WriteArgumentDataToStream(this, &wordsWritten);
	if (ret != 0) {
		return ret;
	}
	if (wordsWritten != argSizeInWords) {
		return FXT_ERR_WRITE_LENGTH_MISMATCH;
	}

	return 0;
}

int Writer::AddBlobRecord(const char *name, void *data, size_t dataLen, BlobType blobType) {
	if (dataLen > 0x7FFFFF) {
		// Blob length is stored in 23 bits
		return FXT_ERR_DATA_TOO_LONG;
	}

	uint16_t nameIndex;
	int ret = GetOrCreateStringIndex(name, &nameIndex);
	if (ret != 0) {
		return ret;
	}

	const size_t paddedSize = (dataLen + 8 - 1) & (-8);
	const size_t diff = paddedSize - dataLen;

	// Write the header
	const uint64_t sizeInWords = 1 + (paddedSize / 8);
	const uint64_t header = ((uint64_t)(blobType) << 48) | ((uint64_t)(dataLen) << 24) | ((uint64_t)(nameIndex) << 16) | ((uint64_t)(sizeInWords) << 4) | (uint64_t)(internal::RecordType::Blob);
	ret = WriteUInt64ToStream(header);
	if (ret != 0) {
		return ret;
	}

	// Then the data
	ret = WriteBytesToStream(data, dataLen);
	if (ret != 0) {
		return ret;
	}

	// And the zero padding
	if (diff > 0) {
		ret = WriteZeroPadding(diff);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

int Writer::WriteUInt64ToStream(uint64_t val) {
	uint8_t buffer[8];

	buffer[0] = uint8_t(val);
	buffer[1] = uint8_t(val >> 8);
	buffer[2] = uint8_t(val >> 16);
	buffer[3] = uint8_t(val >> 24);
	buffer[4] = uint8_t(val >> 32);
	buffer[5] = uint8_t(val >> 40);
	buffer[6] = uint8_t(val >> 48);
	buffer[7] = uint8_t(val >> 56);

	return m_writeFunc(m_userContext, buffer, sizeof(buffer));
}

int Writer::WriteBytesToStream(const void *val, size_t len) {
	return m_writeFunc(m_userContext, (uint8_t *)val, len);
}

int Writer::WriteZeroPadding(size_t count) {
	uint8_t zero = 0;

	for (size_t i = 0; i < count; ++i) {
		int ret = m_writeFunc(m_userContext, &zero, 1);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

int Writer::AddStringRecord(uint16_t stringIndex, const char *str, size_t strLen) {
	const size_t paddedStrLen = (strLen + 8 - 1) & (-8);
	const size_t diff = paddedStrLen - strLen;

	if (paddedStrLen >= 0x7fff) {
		return FXT_ERR_STR_TOO_LONG;
	}

	// Write the header
	const uint64_t sizeInWords = 1 + (paddedStrLen / 8);
	const uint64_t header = ((uint64_t)(strLen) << 32) | ((uint64_t)(stringIndex) << 16) | ((uint64_t)(sizeInWords) << 4) | (uint64_t)(internal::RecordType::String);
	int ret = WriteUInt64ToStream(header);
	if (ret != 0) {
		return ret;
	}

	// Then the string data
	ret = WriteBytesToStream(str, strLen);
	if (ret != 0) {
		return ret;
	}

	// And the zero padding
	if (diff > 0) {
		ret = WriteZeroPadding(diff);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

int Writer::AddThreadRecord(uint16_t threadIndex, KernelObjectID processID, KernelObjectID threadID) {
	const uint64_t sizeInWords = 3;
	const uint64_t header = ((uint64_t)(threadIndex) << 16) | ((uint64_t)(sizeInWords) << 4) | (uint64_t)(internal::RecordType::Thread);

	// Write the header
	int ret = WriteUInt64ToStream(header);
	if (ret != 0) {
		return ret;
	}

	// Then the process ID
	ret = WriteUInt64ToStream(processID);
	if (ret != 0) {
		return ret;
	}

	// And finally the thread ID
	ret = WriteUInt64ToStream(threadID);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

int Writer::GetOrCreateStringIndex(const char *str, uint16_t *strIndex) {
	// Hash the string
	size_t strLen = strlen(str);

	const uint64_t hash = XXH3_64bits(str, strLen);

	// Linearly probe through the string table
	const uint16_t max = std::min(m_nextStringIndex, ArraySize(m_stringTable));
	for (uint16_t i = 0; i < max; ++i) {
		if (m_stringTable[i] == hash) {
			// 0 is a reserved index
			// So we increment all indices by 1
			*strIndex = i + 1;
			return 0;
		}
	}

	// We didn't find an entry
	// So we create one
	uint16_t index = m_nextStringIndex % ArraySize(m_stringTable);
	int ret = AddStringRecord(index + 1, str, strLen);
	if (ret != 0) {
		return ret;
	}

	m_stringTable[index] = hash;
	++m_nextStringIndex;
	*strIndex = index + 1;

	return 0;
}

int Writer::GetOrCreateThreadIndex(KernelObjectID processID, KernelObjectID threadID, uint16_t *threadIndex) {
	// Hash the processID and threadID
	XXH3_state_t state;
	XXH3_64bits_reset(&state);

	XXH3_64bits_update(&state, &processID, sizeof(processID));
	XXH3_64bits_update(&state, &threadID, sizeof(threadID));

	uint64_t hash = XXH3_64bits_digest(&state);

	// Linearly probe through the thread table
	const uint16_t max = std::min(m_nextThreadIndex, ArraySize(m_threadTable));
	for (uint16_t i = 0; i < max; ++i) {
		if (m_threadTable[i] == hash) {
			// 0 is a reserved index
			// So we increment all indices by 1
			*threadIndex = i + 1;
			return 0;
		}
	}

	// We didn't find an entry
	// So we create one
	uint16_t index = m_nextThreadIndex % ArraySize(m_threadTable);
	int ret = AddThreadRecord(index + 1, processID, threadID);
	if (ret != 0) {
		return ret;
	}

	m_threadTable[index] = hash;
	++m_nextThreadIndex;
	*threadIndex = index + 1;

	return 0;
}

} // End of namespace fxt
