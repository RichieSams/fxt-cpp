/* FXT - A library for creating Fuschia Tracing System (FXT) files
 *
 * FXT is the legal property of Adrian Astley
 * Copyright Adrian Astley 2023
 */

#pragma once

#include <inttypes.h>
#include <string.h>

namespace fxt {

namespace internal {

enum class ArgumentType {
	Null = 0,
	Int32 = 1,
	UInt32 = 2,
	Int64 = 3,
	UInt64 = 4,
	Double = 5,
	String = 6,
	Pointer = 7,
	KOID = 8,
	Bool = 9,
};

}

using KernelObjectID = uint64_t;

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

} // End of namespace fxt

/**
 * The macros below are inspired by the trace library
 * https://fuchsia.googlesource.com/fuchsia/+/9cf01c788257/zircon/system/ulib/trace/include/lib/trace/internal/pairs_internal.h
 *
 * Fuchsia is is governed by a BSD-style license
 *
 * Copyright 2019 The Fuchsia Authors.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *    * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

// Multiple expansion layers to ensure proper macro processing
#define FXT_INTERNAL_EXPAND(...) __VA_ARGS__
#define FXT_INTERNAL_EXPAND2(...) FXT_INTERNAL_EXPAND(__VA_ARGS__)
#define FXT_INTERNAL_EXPAND3(...) FXT_INTERNAL_EXPAND2(__VA_ARGS__)

/**
 * @brief Count the number of pairs in the argument list
 *
 * When the number of arguments is uneven, rounds down.
 * Works with 0 to 15 pairs.
 */
#define FXT_INTERNAL_COUNT_PAIRS(...) \
	FXT_INTERNAL_EXPAND(FXT_INTERNAL_COUNT_PAIRS_(__VA_ARGS__, 15, 15, 14, 14, 13, 13, 12, 12, 11, 11, 10, 10, 9, 9, 8, 8, 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1, 0, 0))

#define FXT_INTERNAL_COUNT_PAIRS_(_15, _15X, _14, _14X, _13, _13X, _12, _12X, _11, _11X, _10, _10X, _9, _9X, _8, _8X, _7, _7X, _6, _6X, _5, _5X, _4, _4X, _3, _3X, _2, _2X, _1, _1X, N, ...) N

// Implementation macros for different pair counts
#define FXT_INTERNAL_SELECT_0(fn, ...) /* empty */
#define FXT_INTERNAL_SELECT_1(fn, ...) /* empty */
#define FXT_INTERNAL_SELECT_2(fn, a, b, ...) fn(a, b)
#define FXT_INTERNAL_SELECT_3(fn, a, b, ...) fn(a, b)
#define FXT_INTERNAL_SELECT_4(fn, a, b, c, d, ...) fn(a, b), fn(c, d)
#define FXT_INTERNAL_SELECT_5(fn, a, b, c, d, ...) fn(a, b), fn(c, d)
#define FXT_INTERNAL_SELECT_6(fn, a, b, c, d, e, f, ...) fn(a, b), fn(c, d), fn(e, f)
#define FXT_INTERNAL_SELECT_7(fn, a, b, c, d, e, f, ...) fn(a, b), fn(c, d), fn(e, f)
#define FXT_INTERNAL_SELECT_8(fn, a, b, c, d, e, f, g, h, ...) fn(a, b), fn(c, d), fn(e, f), fn(g, h)
#define FXT_INTERNAL_SELECT_9(fn, a, b, c, d, e, f, g, h, ...) fn(a, b), fn(c, d), fn(e, f), fn(g, h)
#define FXT_INTERNAL_SELECT_10(fn, a, b, c, d, e, f, g, h, i, j, ...) fn(a, b), fn(c, d), fn(e, f), fn(g, h), fn(i, j)
#define FXT_INTERNAL_SELECT_11(fn, a, b, c, d, e, f, g, h, i, j, ...) fn(a, b), fn(c, d), fn(e, f), fn(g, h), fn(i, j)
#define FXT_INTERNAL_SELECT_12(fn, a, b, c, d, e, f, g, h, i, j, k, l, ...) fn(a, b), fn(c, d), fn(e, f), fn(g, h), fn(i, j), fn(k, l)
#define FXT_INTERNAL_SELECT_13(fn, a, b, c, d, e, f, g, h, i, j, k, l, ...) fn(a, b), fn(c, d), fn(e, f), fn(g, h), fn(i, j), fn(k, l)
#define FXT_INTERNAL_SELECT_14(fn, a, b, c, d, e, f, g, h, i, j, k, l, m, n, ...) fn(a, b), fn(c, d), fn(e, f), fn(g, h), fn(i, j), fn(k, l), fn(m, n)
#define FXT_INTERNAL_SELECT_15(fn, a, b, c, d, e, f, g, h, i, j, k, l, m, n, ...) fn(a, b), fn(c, d), fn(e, f), fn(g, h), fn(i, j), fn(k, l), fn(m, n)

// Helper macros for indirection and selection
#define FXT_INTERNAL_SELECT_HELPER(N) FXT_INTERNAL_SELECT_##N
#define FXT_INTERNAL_SELECT(N) FXT_INTERNAL_SELECT_HELPER(N)

/**
 * Applies the given fn to each pair of arguments
 */
#define FXT_INTERNAL_APPLY_PAIRWISE_CSV(fn, ...) \
	FXT_INTERNAL_EXPAND3(FXT_INTERNAL_SELECT(FXT_INTERNAL_EXPAND(FXT_INTERNAL_COUNT_PAIRS(__VA_ARGS__)))(fn, __VA_ARGS__))

#define FXT_INTERNAL_DECLARE_ARG(name, value) \
	fxt::RecordArgument(name, fxt::RecordArgumentValue(value))

/**
 * @brief Creates a std::initializer_list<fxt::RecordArgument> taking in a list of argument key value pairs
 *
 * The keys and values *can* be just plain values. The compiler will attempt to find a corresponding constructor
 * for RecordArgumentName / RecordArgumentValue. Alternatively, you can construct the name / value yourself to
 * have more control.
 *
 * NOTE: By default, all string keys and values will use "inline" string records. If you anticipate the string
 *       will be used often, you can instead use the explicit RecordArgument*() construtor or RecordArgument*::CharArray()
 *       static function so you can set useStringTable = true. This will create a string table record if necessary.
 *       And then all future references to that string will just be an index into the string table instead of needing
 *       to include the raw characters over and over.
 */
#define FXT_ARG_LIST(...) { FXT_INTERNAL_APPLY_PAIRWISE_CSV(FXT_INTERNAL_DECLARE_ARG, __VA_ARGS__) }
