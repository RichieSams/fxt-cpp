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
	explicit RecordArgumentValue(const char *value)
	        : type(internal::ArgumentType::String),
	          stringValue(value),
	          stringLen(strlen(value)) {
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
	static RecordArgumentValue CharArray(char *value, unsigned valueLen) {
		RecordArgumentValue arg(internal::ArgumentType::String);
		arg.stringValue = value;
		arg.stringLen = valueLen;
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
};

struct RecordArgument {
	explicit RecordArgument(const char *name, RecordArgumentValue value)
	        : name(name),
	          nameLen(strlen(name)),
	          value(value) {
	}
	~RecordArgument() = default;

	const char *name;
	size_t nameLen;
	RecordArgumentValue value;
};

} // End of namespace fxt
