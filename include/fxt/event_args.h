/* FXT - A library for creating Fuschia Tracing System (FXT) files
 *
 * FXT is the legal property of Adrian Astley
 * Copyright Adrian Astley 2023
 */

#pragma once

#include "fxt/internal/defines.h"

#include <inttypes.h>

namespace fxt {

class Writer;

class EventArgument {
public:
	virtual ~EventArgument() = default;

public:
	virtual unsigned ArgumentSizeInWords() = 0;
	virtual int InitStringEntries(Writer *writer) = 0;
	virtual int WriteArgumentDataToStream(Writer *writer, unsigned *wordsWritten) = 0;
};

class NullEventArgument : public EventArgument {
public:
	NullEventArgument(const char *key)
	        : m_key(key),
	          m_keyStrIndex(0) {
	}
	~NullEventArgument() = default;

private:
	const char *m_key;
	uint16_t m_keyStrIndex;
	int32_t m_value;

public:
	unsigned ArgumentSizeInWords() {
		return 1;
	}
	int InitStringEntries(Writer *writer);
	int WriteArgumentDataToStream(Writer *writer, unsigned *wordsWritten);
};

class Int32EventArgument : public EventArgument {
public:
	Int32EventArgument(const char *key, int32_t value)
	        : m_key(key),
	          m_keyStrIndex(0),
	          m_value(value) {
	}
	~Int32EventArgument() = default;

private:
	const char *m_key;
	uint16_t m_keyStrIndex;
	int32_t m_value;

public:
	unsigned ArgumentSizeInWords() {
		return 1;
	}
	int InitStringEntries(Writer *writer);
	int WriteArgumentDataToStream(Writer *writer, unsigned *wordsWritten);
};

class UInt32EventArgument : public EventArgument {
public:
	UInt32EventArgument(const char *key, uint32_t value)
	        : m_key(key),
	          m_keyStrIndex(0),
	          m_value(value) {
	}
	~UInt32EventArgument() = default;

private:
	const char *m_key;
	uint16_t m_keyStrIndex;
	uint32_t m_value;

public:
	unsigned ArgumentSizeInWords() {
		return 1;
	}
	int InitStringEntries(Writer *writer);
	int WriteArgumentDataToStream(Writer *writer, unsigned *wordsWritten);
};

class Int64EventArgument : public EventArgument {
public:
	Int64EventArgument(const char *key, int64_t value)
	        : m_key(key),
	          m_keyStrIndex(0),
	          m_value(value) {
	}
	~Int64EventArgument() = default;

private:
	const char *m_key;
	uint16_t m_keyStrIndex;
	int64_t m_value;

public:
	unsigned ArgumentSizeInWords() {
		return 2;
	}
	int InitStringEntries(Writer *writer);
	int WriteArgumentDataToStream(Writer *writer, unsigned *wordsWritten);
};

class UInt64EventArgument : public EventArgument {
public:
	UInt64EventArgument(const char *key, uint64_t value)
	        : m_key(key),
	          m_keyStrIndex(0),
	          m_value(value) {
	}
	~UInt64EventArgument() = default;

private:
	const char *m_key;
	uint16_t m_keyStrIndex;
	uint64_t m_value;

public:
	unsigned ArgumentSizeInWords() {
		return 2;
	}
	int InitStringEntries(Writer *writer);
	int WriteArgumentDataToStream(Writer *writer, unsigned *wordsWritten);
};

class DoubleEventArgument : public EventArgument {
public:
	DoubleEventArgument(const char *key, double value)
	        : m_key(key),
	          m_keyStrIndex(0),
	          m_value(value) {
	}
	~DoubleEventArgument() = default;

private:
	const char *m_key;
	uint16_t m_keyStrIndex;
	double m_value;

public:
	unsigned ArgumentSizeInWords() {
		return 2;
	}
	int InitStringEntries(Writer *writer);
	int WriteArgumentDataToStream(Writer *writer, unsigned *wordsWritten);
};

class StringEventArgument : public EventArgument {
public:
	StringEventArgument(const char *key, const char *value)
	        : m_key(key),
	          m_keyStrIndex(0),
	          m_value(value),
	          m_valueStrIndex(0)

	{
	}
	~StringEventArgument() = default;

private:
	const char *m_key;
	uint16_t m_keyStrIndex;
	const char *m_value;
	uint16_t m_valueStrIndex;

public:
	unsigned ArgumentSizeInWords() {
		return 1;
	}
	int InitStringEntries(Writer *writer);
	int WriteArgumentDataToStream(Writer *writer, unsigned *wordsWritten);
};

class PointerEventArgument : public EventArgument {
public:
	PointerEventArgument(const char *key, uintptr_t value)
	        : m_key(key),
	          m_keyStrIndex(0),
	          m_value(value) {
	}
	~PointerEventArgument() = default;

private:
	const char *m_key;
	uint16_t m_keyStrIndex;
	uintptr_t m_value;

public:
	unsigned ArgumentSizeInWords() {
		return 2;
	}
	int InitStringEntries(Writer *writer);
	int WriteArgumentDataToStream(Writer *writer, unsigned *wordsWritten);
};

class KOIDEventArgument : public EventArgument {
public:
	KOIDEventArgument(const char *key, KernelObjectID value)
	        : m_key(key),
	          m_keyStrIndex(0),
	          m_value(value) {
	}
	~KOIDEventArgument() = default;

private:
	const char *m_key;
	uint16_t m_keyStrIndex;
	KernelObjectID m_value;

public:
	unsigned ArgumentSizeInWords() {
		return 2;
	}
	int InitStringEntries(Writer *writer);
	int WriteArgumentDataToStream(Writer *writer, unsigned *wordsWritten);
};

class BoolEventArgument : public EventArgument {
public:
	BoolEventArgument(const char *key, bool value)
	        : m_key(key),
	          m_keyStrIndex(0),
	          m_value(value) {
	}
	~BoolEventArgument() = default;

private:
	const char *m_key;
	uint16_t m_keyStrIndex;
	bool m_value;

public:
	unsigned ArgumentSizeInWords() {
		return 1;
	}
	int InitStringEntries(Writer *writer);
	int WriteArgumentDataToStream(Writer *writer, unsigned *wordsWritten);
};

} // End of namespace fxt
