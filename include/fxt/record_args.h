/* FXT - A library for creating Fuschia Tracing System (FXT) files
 *
 * FXT is the legal property of Adrian Astley
 * Copyright Adrian Astley 2023
 */

#pragma once

#include "fxt/internal/constants.h"
#include "fxt/internal/defines.h"
#include "fxt/internal/pairs.h"

#include <inttypes.h>
#include <string.h>

namespace fxt {

struct RecordArgumentValue {
public:
	explicit RecordArgumentValue(decltype(nullptr) value)
	        : type(internal::ArgumentType::Null) {
	}
	explicit RecordArgumentValue(int32_t value)
	        : type(internal::ArgumentType::Int32),
	          int32Value(value) {
	}
	explicit RecordArgumentValue(uint32_t value)
	        : type(internal::ArgumentType::UInt32),
	          uint32Value(value) {
	}
	explicit RecordArgumentValue(int64_t value)
	        : type(internal::ArgumentType::Int64),
	          int64Value(value) {
	}
	explicit RecordArgumentValue(uint64_t value)
	        : type(internal::ArgumentType::UInt64),
	          uint64Value(value) {
	}
	explicit RecordArgumentValue(double value)
	        : type(internal::ArgumentType::Double),
	          doubleValue(value) {
	}
	explicit RecordArgumentValue(const char *value, bool useStringTable = false)
	        : type(internal::ArgumentType::String),
	          stringValue(value),
	          stringLen(strlen(value)),
	          useStringTable(useStringTable),
	          hexEncode(false) {
	}
	template <typename T>
	explicit RecordArgumentValue(T *value)
	        : type(internal::ArgumentType::Pointer),
	          pointerValue(reinterpret_cast<uintptr_t>(value)) {
	}
	explicit RecordArgumentValue(bool value)
	        : type(internal::ArgumentType::Bool),
	          boolValue(value) {
	}

private:
	explicit RecordArgumentValue(internal::ArgumentType valueType)
	        : type(valueType) {
	}

public:
	static RecordArgumentValue KOID(KernelObjectID value) {
		RecordArgumentValue arg(internal::ArgumentType::KOID);
		arg.koidValue = value;
		return arg;
	}
	static RecordArgumentValue CharArray(char *value, unsigned valueLen, bool useStringTable = false) {
		RecordArgumentValue arg(internal::ArgumentType::String);
		arg.stringValue = value;
		arg.stringLen = valueLen;
		arg.useStringTable = useStringTable;
		arg.hexEncode = false;
		return arg;
	}

	/**
	 * @brief Encodes raw bytes as a hex string
	 *
	 * @param value             The byte array to encode
	 * @param valueLen          The length of value
	 * @return                  The constructed value
	 */
	static RecordArgumentValue HexArray(uint8_t *value, unsigned valueLen) {
		RecordArgumentValue arg(internal::ArgumentType::String);
		arg.stringValue = (char *)value;
		arg.stringLen = valueLen;
		// For ease of code maintenance, since hex arrays are dynamically
		// generated, and we don't want to do memory allocation, we don't allow them to be
		// in the string table
		arg.useStringTable = false;
		arg.hexEncode = true;
		return arg;
	}

public:
	internal::ArgumentType type;
	union {
		int32_t int32Value;
		uint32_t uint32Value;
		int64_t int64Value;
		uint64_t uint64Value;
		double doubleValue;
		const char *stringValue;
		uintptr_t pointerValue;
		KernelObjectID koidValue;
		bool boolValue;
	};
	size_t stringLen;
	bool useStringTable;
	bool hexEncode;
};

struct RecordArgumentName {
	RecordArgumentName(const char *name, size_t nameLen, bool useStringTable)
	        : name(name),
	          nameLen(nameLen),
	          useStringTable(useStringTable) {
	}
	RecordArgumentName(const char *name, bool useStringTable)
	        : name(name),
	          nameLen(strlen(name)),
	          useStringTable(useStringTable) {
	}

	const char *name;
	size_t nameLen;
	bool useStringTable;
};

struct RecordArgument {
	explicit RecordArgument(const char *name, RecordArgumentValue value)
	        : name(RecordArgumentName(name, strlen(name), false)),
	          value(value) {
	}
	explicit RecordArgument(RecordArgumentName name, RecordArgumentValue value)
	        : name(name),
	          value(value) {
	}
	~RecordArgument() = default;

	RecordArgumentName name;
	RecordArgumentValue value;
};

namespace internal {

#define FXT_MAX_NUM_ARGS 15

struct ProcessedRecordArgument {
	uint16_t nameStringRef;
	uint16_t nameSizeInWords;

	uint16_t valueStringRef;
	uint16_t headerAndValueSizeInWords;
};

} // End of namespace internal

} // End of namespace fxt
