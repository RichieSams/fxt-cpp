/* FXT - A library for creating Fuschia Tracing System (FXT) files
 *
 * FXT is the legal property of Adrian Astley
 * Copyright Adrian Astley 2023
 */

/**
 * The code design below is borrowed from the Fuchsia trace-engine library
 * https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/zircon/system/ulib/trace-engine/include/lib/trace-engine/fields.h
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

#include <inttypes.h>
#include <stddef.h>

namespace fxt {

inline constexpr size_t Pad(size_t size) {
	return size + ((8 - (size & 7)) & 7);
}

inline constexpr size_t BytesToWords(size_t num_bytes) {
	return Pad(num_bytes) / sizeof(uint64_t);
}

inline constexpr size_t WordsToBytes(size_t num_words) {
	return num_words * sizeof(uint64_t);
}

// Describes the layout of a bit-field packed into a 64-bit word.
template <size_t begin, size_t end>
struct Field {
	static_assert(begin < sizeof(uint64_t) * 8, "begin is out of bounds");
	static_assert(end < sizeof(uint64_t) * 8, "end is out of bounds");
	static_assert(begin <= end, "begin must not be larger than end");
	static_assert(end - begin + 1 < 64, "must be a part of a word, not a whole word");

	static constexpr uint64_t kMask = (uint64_t(1) << (end - begin + 1)) - 1;

	static constexpr uint64_t Make(uint64_t value) {
		return (value & kMask) << begin;
	}

	template <typename U>
	static constexpr U Get(uint64_t word) {
		static_assert(sizeof(U) * 8 >= end - begin + 1, "type must fit all the bits");
		return static_cast<U>((word >> begin) & kMask);
	}

	static constexpr void Set(uint64_t &word, uint64_t value) {
		word = (word & ~(kMask << begin)) | Make(value);
	}
};

struct ArgumentFields {
	using Type = Field<0, 3>;
	using ArgumentSize = Field<4, 15>;
	using NameRef = Field<16, 31>;
};

struct Int32ArgumentFields : ArgumentFields {
	using Value = Field<32, 63>;
};

struct UInt32ArgumentFields : ArgumentFields {
	using Value = Field<32, 63>;
};

struct StringArgumentFields : ArgumentFields {
	using ValueRef = Field<32, 47>;
};

struct BoolArgumentFields : ArgumentFields {
	using Value = Field<32, 32>;
};

struct RecordFields {
	static constexpr uint64_t kMaxRecordSizeWords = 0xfff;
	static constexpr uint64_t kMaxRecordSizeBytes = WordsToBytes(kMaxRecordSizeWords);
	using Type = Field<0, 3>;
	using RecordSize = Field<4, 15>;
};

struct LargeRecordFields {
	static constexpr uint64_t kMaxRecordSizeWords = (1ull << 32) - 1;
	static constexpr uint64_t kMaxRecordSizeBytes = WordsToBytes(kMaxRecordSizeWords);
	using Type = Field<0, 3>;
	using RecordSize = Field<4, 35>;
	using LargeType = Field<36, 39>;
};

struct MetadataRecordFields : RecordFields {
	using MetadataType = Field<16, 19>;
};

struct ProviderInfoMetadataRecordFields : MetadataRecordFields {
	static constexpr size_t kMaxNameLength = 0xff;
	using ProviderID = Field<20, 51>;
	using NameLength = Field<52, 59>;
};

struct ProviderSectionMetadataRecordFields : MetadataRecordFields {
	using ProviderID = Field<20, 51>;
};

struct ProviderEventMetadataRecordFields : MetadataRecordFields {
	using ProviderID = Field<20, 51>;
	using Event = Field<52, 55>;
};

struct TraceInfoMetadataRecordFields : MetadataRecordFields {
	using TraceInfoType = Field<20, 23>;
};

struct MagicNumberRecordFields : TraceInfoMetadataRecordFields {
	using Magic = Field<24, 55>;
};

using InitializationRecordFields = RecordFields;

struct StringRefFields {
	static constexpr uint16_t MaxInlineStrLen = 0x7fff;
	static constexpr uint16_t Inline(int strLen) {
		return (uint16_t)0x8000 | (uint16_t)strLen;
	}
};

struct StringRecordFields : RecordFields {
	using StringIndex = Field<16, 30>;
	using StringLength = Field<32, 46>;
};

struct ThreadRecordFields : RecordFields {
	using ThreadIndex = Field<16, 23>;
};

struct EventRecordFields : RecordFields {
	using EventType = Field<16, 19>;
	using ArgumentCount = Field<20, 23>;
	using ThreadRef = Field<24, 31>;
	using CategoryStringRef = Field<32, 47>;
	using NameStringRef = Field<48, 63>;
};

struct BlobRecordFields : RecordFields {
	static constexpr size_t kMaxBlobLength = 0x7fffff;
	using NameStringRef = Field<16, 31>;
	using BlobSize = Field<32, 46>;
	using BlobType = Field<48, 55>;
};

struct UserspaceObjectRecordFields : RecordFields {
	using ThreadRef = Field<16, 23>;
	using NameStringRef = Field<24, 39>;
	using ArgumentCount = Field<40, 43>;
};

struct KernelObjectRecordFields : RecordFields {
	using ObjectType = Field<16, 23>;
	using NameStringRef = Field<24, 39>;
	using ArgumentCount = Field<40, 43>;
};

struct SchedulingRecordFields : RecordFields {
	using EventType = Field<60, 63>;
};

struct ContextSwitchRecordFields : SchedulingRecordFields {
	using ArgumentCount = Field<16, 19>;
	using CpuNumber = Field<20, 35>;
	using OutgoingThreadState = Field<36, 39>;
};

struct FiberSwitchRecordFields : SchedulingRecordFields {
	using ArgumentCount = Field<16, 19>;
};

struct ThreadWakeupRecordFields : SchedulingRecordFields {
	using ArgumentCount = Field<16, 19>;
	using CpuNumber = Field<20, 35>;
};

struct LogRecordFields : RecordFields {
	static constexpr size_t kMaxMessageLength = 0x7fff;
	using LogMessageLength = Field<16, 30>;
	using ThreadRef = Field<32, 39>;
};

struct LargeBlobFields : LargeRecordFields {
	using BlobFormat = Field<40, 43>;
};

struct BlobFormatAttachmentFields {
	using CategoryStringRef = Field<0, 15>;
	using NameStringRef = Field<16, 31>;
};

struct BlobFormatEventFields {
	using CategoryStringRef = Field<0, 15>;
	using NameStringRef = Field<16, 31>;
	using ArgumentCount = Field<32, 35>;
	using ThreadRef = Field<36, 43>;
};

} // namespace fxt
