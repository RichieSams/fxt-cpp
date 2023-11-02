/* FXT - A library for creating Fuschia Tracing System (FXT) files
 *
 * FXT is the legal property of Adrian Astley
 * Copyright Adrian Astley 2023
 */

#include "fxt/event_args.h"

#include "fxt/writer.h"

namespace fxt {

int fxt::NullEventArgument::InitStringEntries(Writer *writer) {
	return writer->GetOrCreateStringIndex(m_key, &m_keyStrIndex);
}

int NullEventArgument::WriteArgumentDataToStream(Writer *writer, unsigned *wordsWritten) {
	const unsigned sizeInWords = 1;
	const uint64_t header = ((uint64_t)(m_keyStrIndex) << 16) | ((uint64_t)(sizeInWords) << 4) | (uint64_t)(internal::ArgumentType::Null);
	int ret = writer->WriteUInt64ToStream(header);
	if (ret != 0) {
		return ret;
	}

	*wordsWritten = sizeInWords;
	return 0;
}

int Int32EventArgument::InitStringEntries(Writer *writer) {
	return writer->GetOrCreateStringIndex(m_key, &m_keyStrIndex);
}

int Int32EventArgument::WriteArgumentDataToStream(Writer *writer, unsigned *wordsWritten) {
	const unsigned sizeInWords = 1;
	const uint64_t header = ((uint64_t)(m_value) << 32) | ((uint64_t)(m_keyStrIndex) << 16) | ((uint64_t)(sizeInWords) << 4) | (uint64_t)(internal::ArgumentType::Int32);
	int ret = writer->WriteUInt64ToStream(header);
	if (ret != 0) {
		return ret;
	}

	*wordsWritten = sizeInWords;
	return 0;
}

int UInt32EventArgument::InitStringEntries(Writer *writer) {
	return writer->GetOrCreateStringIndex(m_key, &m_keyStrIndex);
}

int UInt32EventArgument::WriteArgumentDataToStream(Writer *writer, unsigned *wordsWritten) {
	const unsigned sizeInWords = 1;
	const uint64_t header = ((uint64_t)(m_value) << 32) | ((uint64_t)(m_keyStrIndex) << 16) | ((uint64_t)(sizeInWords) << 4) | (uint64_t)(internal::ArgumentType::UInt32);
	int ret = writer->WriteUInt64ToStream(header);
	if (ret != 0) {
		return ret;
	}

	*wordsWritten = sizeInWords;
	return 0;
}

int Int64EventArgument::InitStringEntries(Writer *writer) {
	return writer->GetOrCreateStringIndex(m_key, &m_keyStrIndex);
}

int Int64EventArgument::WriteArgumentDataToStream(Writer *writer, unsigned *wordsWritten) {
	const unsigned sizeInWords = 2;
	const uint64_t header = ((uint64_t)(m_keyStrIndex) << 16) | ((uint64_t)(sizeInWords) << 4) | (uint64_t)(internal::ArgumentType::Int64);
	int ret = writer->WriteUInt64ToStream(header);
	if (ret != 0) {
		return ret;
	}

	ret = writer->WriteUInt64ToStream((uint64_t)m_value);
	if (ret != 0) {
		return ret;
	}

	*wordsWritten = sizeInWords;
	return 0;
}

int UInt64EventArgument::InitStringEntries(Writer *writer) {
	return writer->GetOrCreateStringIndex(m_key, &m_keyStrIndex);
}

int UInt64EventArgument::WriteArgumentDataToStream(Writer *writer, unsigned *wordsWritten) {
	const unsigned sizeInWords = 2;
	const uint64_t header = ((uint64_t)(m_keyStrIndex) << 16) | ((uint64_t)(sizeInWords) << 4) | (uint64_t)(internal::ArgumentType::UInt64);
	int ret = writer->WriteUInt64ToStream(header);
	if (ret != 0) {
		return ret;
	}

	ret = writer->WriteUInt64ToStream((uint64_t)m_value);
	if (ret != 0) {
		return ret;
	}

	*wordsWritten = sizeInWords;
	return 0;
}

int DoubleEventArgument::InitStringEntries(Writer *writer) {
	return writer->GetOrCreateStringIndex(m_key, &m_keyStrIndex);
}

int DoubleEventArgument::WriteArgumentDataToStream(Writer *writer, unsigned *wordsWritten) {
	const unsigned sizeInWords = 2;
	const uint64_t header = ((uint64_t)(m_keyStrIndex) << 16) | ((uint64_t)(sizeInWords) << 4) | (uint64_t)(internal::ArgumentType::Double);
	int ret = writer->WriteUInt64ToStream(header);
	if (ret != 0) {
		return ret;
	}

	ret = writer->WriteUInt64ToStream(*(uint64_t *)(&m_value));
	if (ret != 0) {
		return ret;
	}

	*wordsWritten = sizeInWords;
	return 0;
}

int StringEventArgument::InitStringEntries(Writer *writer) {
	int ret = writer->GetOrCreateStringIndex(m_key, &m_keyStrIndex);
	if (ret != 0) {
		return ret;
	}

	ret = writer->GetOrCreateStringIndex(m_value, &m_valueStrIndex);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

int StringEventArgument::WriteArgumentDataToStream(Writer *writer, unsigned *wordsWritten) {
	const unsigned sizeInWords = 1;
	const uint64_t header = ((uint64_t)(m_valueStrIndex) << 32) | ((uint64_t)(m_keyStrIndex) << 16) | ((uint64_t)(sizeInWords) << 4) | (uint64_t)(internal::ArgumentType::String);
	int ret = writer->WriteUInt64ToStream(header);
	if (ret != 0) {
		return ret;
	}

	*wordsWritten = sizeInWords;
	return 0;
}

int PointerEventArgument::InitStringEntries(Writer *writer) {
	return writer->GetOrCreateStringIndex(m_key, &m_keyStrIndex);
}

int PointerEventArgument::WriteArgumentDataToStream(Writer *writer, unsigned *wordsWritten) {
	const unsigned sizeInWords = 2;
	const uint64_t header = ((uint64_t)(m_keyStrIndex) << 16) | ((uint64_t)(sizeInWords) << 4) | (uint64_t)(internal::ArgumentType::Pointer);
	int ret = writer->WriteUInt64ToStream(header);
	if (ret != 0) {
		return ret;
	}

	ret = writer->WriteUInt64ToStream((uint64_t)m_value);
	if (ret != 0) {
		return ret;
	}

	*wordsWritten = sizeInWords;
	return 0;
}

int KOIDEventArgument::InitStringEntries(Writer *writer) {
	return writer->GetOrCreateStringIndex(m_key, &m_keyStrIndex);
}

int KOIDEventArgument::WriteArgumentDataToStream(Writer *writer, unsigned *wordsWritten) {
	const unsigned sizeInWords = 2;
	const uint64_t header = ((uint64_t)(m_keyStrIndex) << 16) | ((uint64_t)(sizeInWords) << 4) | (uint64_t)(internal::ArgumentType::KOID);
	int ret = writer->WriteUInt64ToStream(header);
	if (ret != 0) {
		return ret;
	}

	ret = writer->WriteUInt64ToStream((uint64_t)m_value);
	if (ret != 0) {
		return ret;
	}

	*wordsWritten = sizeInWords;
	return 0;
}

int BoolEventArgument::InitStringEntries(Writer *writer) {
	return writer->GetOrCreateStringIndex(m_key, &m_keyStrIndex);
}

int BoolEventArgument::WriteArgumentDataToStream(Writer *writer, unsigned *wordsWritten) {
	const uint64_t valueBit = m_value ? 1 : 0;

	const unsigned sizeInWords = 1;
	const uint64_t header = (valueBit << 32) | ((uint64_t)(m_keyStrIndex) << 16) | ((uint64_t)(sizeInWords) << 4) | (uint64_t)(internal::ArgumentType::Bool);
	int ret = writer->WriteUInt64ToStream(header);
	if (ret != 0) {
		return ret;
	}

	*wordsWritten = sizeInWords;
	return 0;
}

} // End of namespace fxt
