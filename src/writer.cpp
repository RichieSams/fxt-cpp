/* FXT - A library for creating Fuschia Tracing System (FXT) files
 *
 * FXT is the legal property of Adrian Astley
 * Copyright Adrian Astley 2023
 */

#include "fxt/writer.h"

#define XXH_INLINE_ALL
#include "xxhash.h"

#include <type_traits>

namespace fxt {

// Helpers

template <class T, uint16_t N>
static constexpr uint16_t ArraySize(T (&)[N]) {
	return N;
}

// Casts an enum's value to its underlying type.
template <typename T>
inline constexpr typename std::underlying_type<T>::type ToUnderlyingType(T value) {
	return static_cast<typename std::underlying_type<T>::type>(value);
}

// Writer methods

// Stream write helpers
static int WriteUInt64ToStream(Writer *writer, uint64_t val);
static int WriteBytesToStream(Writer *writer, const void *val, size_t len);
static int WriteZeroPadding(Writer *writer, size_t count);

int WriteMagicNumberRecord(Writer *writer) {
	const char fxtMagic[] = { 0x10, 0x00, 0x04, 0x46, 0x78, 0x54, 0x16, 0x00 };
	return WriteBytesToStream(writer, fxtMagic, ArraySize(fxtMagic));
}

int AddProviderInfoRecord(Writer *writer, ProviderID providerID, const char *providerName) {
	const size_t strLen = strlen(providerName);
	const size_t paddedStrLen = (strLen + 8 - 1) & (-8);
	const size_t diff = paddedStrLen - strLen;

	if (paddedStrLen >= internal::ProviderInfoMetadataRecordFields::kMaxNameLength) {
		return FXT_ERR_STR_TOO_LONG;
	}

	// Write the header
	const uint64_t sizeInWords = 1 + (paddedStrLen / 8);
	const uint64_t header = internal::ProviderInfoMetadataRecordFields::Type::Make(ToUnderlyingType(internal::RecordType::Metadata)) |
	                        internal::ProviderInfoMetadataRecordFields::RecordSize::Make(sizeInWords) |
	                        internal::ProviderInfoMetadataRecordFields::MetadataType::Make(ToUnderlyingType(internal::MetadataType::ProviderInfo)) |
	                        internal::ProviderInfoMetadataRecordFields::ProviderID::Make(providerID) |
	                        internal::ProviderInfoMetadataRecordFields::NameLength::Make(strLen);
	int ret = WriteUInt64ToStream(writer, header);
	if (ret != 0) {
		return ret;
	}

	// Then the string data
	ret = WriteBytesToStream(writer, providerName, strLen);
	if (ret != 0) {
		return ret;
	}

	// And the zero padding
	if (diff > 0) {
		ret = WriteZeroPadding(writer, diff);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

int AddProviderSectionRecord(Writer *writer, ProviderID providerID) {
	const uint64_t sizeInWords = 1;
	const uint64_t header = internal::ProviderSectionMetadataRecordFields::Type::Make(ToUnderlyingType(internal::RecordType::Metadata)) |
	                        internal::ProviderSectionMetadataRecordFields::RecordSize::Make(sizeInWords) |
	                        internal::ProviderSectionMetadataRecordFields::MetadataType::Make(ToUnderlyingType(internal::MetadataType::ProviderSection)) |
	                        internal::ProviderSectionMetadataRecordFields::ProviderID::Make(providerID);
	int ret = WriteUInt64ToStream(writer, header);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

int AddProviderEventRecord(Writer *writer, ProviderID providerID, ProviderEventType eventType) {
	const uint64_t sizeInWords = 1;
	const uint64_t header = internal::ProviderEventMetadataRecordFields::Type::Make(ToUnderlyingType(internal::RecordType::Metadata)) |
	                        internal::ProviderEventMetadataRecordFields::RecordSize::Make(sizeInWords) |
	                        internal::ProviderEventMetadataRecordFields::MetadataType::Make(ToUnderlyingType(internal::MetadataType::ProviderSection)) |
	                        internal::ProviderEventMetadataRecordFields::ProviderID::Make(providerID) |
	                        internal::ProviderEventMetadataRecordFields::Event::Make(ToUnderlyingType(eventType));
	int ret = WriteUInt64ToStream(writer, header);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

int AddInitializationRecord(Writer *writer, uint64_t numTicksPerSecond) {
	const uint64_t sizeInWords = 2;
	const uint64_t header = internal::InitializationRecordFields::Type::Make(ToUnderlyingType(internal::RecordType::Initialization)) |
	                        internal::InitializationRecordFields::RecordSize::Make(sizeInWords);
	int ret = WriteUInt64ToStream(writer, header);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream(writer, numTicksPerSecond);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

static int AddStringRecord(Writer *writer, uint16_t stringIndex, const char *str, size_t strLen) {
	const size_t paddedStrLen = (strLen + 8 - 1) & (-8);
	const size_t diff = paddedStrLen - strLen;

	if (paddedStrLen >= 0x7fff) {
		return FXT_ERR_STR_TOO_LONG;
	}

	// Write the header
	const uint64_t sizeInWords = 1 + (paddedStrLen / 8);
	const uint64_t header = internal::StringRecordFields::Type::Make(ToUnderlyingType(internal::RecordType::String)) |
	                        internal::StringRecordFields::RecordSize::Make(sizeInWords) |
	                        internal::StringRecordFields::StringIndex::Make(stringIndex) |
	                        internal::StringRecordFields::StringLength::Make(strLen);
	int ret = WriteUInt64ToStream(writer, header);
	if (ret != 0) {
		return ret;
	}

	// Then the string data
	ret = WriteBytesToStream(writer, str, strLen);
	if (ret != 0) {
		return ret;
	}

	// And the zero padding
	if (diff > 0) {
		ret = WriteZeroPadding(writer, diff);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

static int GetOrCreateStringIndex(Writer *writer, const char *str, size_t strLen, uint16_t *strIndex) {
	// Hash the string
	const uint64_t hash = XXH3_64bits(str, strLen);

	// Linearly probe through the string table
	uint16_t max = writer->nextStringIndex;
	if (writer->nextStringIndex > ArraySize(writer->stringTable)) {
		max = ArraySize(writer->stringTable);
	}
	for (uint16_t i = 0; i < max; ++i) {
		if (writer->stringTable[i] == hash) {
			// 0 is a reserved index
			// So we increment all indices by 1
			*strIndex = i + 1;
			return 0;
		}
	}

	// We didn't find an entry
	// So we create one
	uint16_t index = writer->nextStringIndex % ArraySize(writer->stringTable);
	int ret = AddStringRecord(writer, index + 1, str, strLen);
	if (ret != 0) {
		return ret;
	}

	writer->stringTable[index] = hash;
	++writer->nextStringIndex;
	*strIndex = index + 1;

	return 0;
}

int GetOrCreateStringIndex(Writer *writer, const char *str, uint16_t *strIndex) {
	return GetOrCreateStringIndex(writer, str, strlen(str), strIndex);
}

static int AddThreadRecord(Writer *writer, uint16_t threadIndex, KernelObjectID processID, KernelObjectID threadID) {
	// Write the header
	const uint64_t sizeInWords = 3;
	const uint64_t header = internal::ThreadRecordFields::Type::Make(ToUnderlyingType(internal::RecordType::Thread)) |
	                        internal::ThreadRecordFields::RecordSize::Make(sizeInWords) |
	                        internal::ThreadRecordFields::ThreadIndex::Make(threadIndex);
	int ret = WriteUInt64ToStream(writer, header);
	if (ret != 0) {
		return ret;
	}

	// Then the process ID
	ret = WriteUInt64ToStream(writer, processID);
	if (ret != 0) {
		return ret;
	}

	// And finally the thread ID
	ret = WriteUInt64ToStream(writer, threadID);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

int GetOrCreateThreadIndex(Writer *writer, KernelObjectID processID, KernelObjectID threadID, uint16_t *threadIndex) {
	// Hash the processID and threadID
	XXH3_state_t state;
	XXH3_64bits_reset(&state);

	XXH3_64bits_update(&state, &processID, sizeof(processID));
	XXH3_64bits_update(&state, &threadID, sizeof(threadID));

	uint64_t hash = XXH3_64bits_digest(&state);

	// Linearly probe through the thread table
	uint16_t max = writer->nextThreadIndex;
	if (writer->nextThreadIndex > ArraySize(writer->threadTable)) {
		max = ArraySize(writer->threadTable);
	}
	for (uint16_t i = 0; i < max; ++i) {
		if (writer->threadTable[i] == hash) {
			// 0 is a reserved index
			// So we increment all indices by 1
			*threadIndex = i + 1;
			return 0;
		}
	}

	// We didn't find an entry
	// So we create one
	uint16_t index = writer->nextThreadIndex % ArraySize(writer->threadTable);
	int ret = AddThreadRecord(writer, index + 1, processID, threadID);
	if (ret != 0) {
		return ret;
	}

	writer->threadTable[index] = hash;
	++writer->nextThreadIndex;
	*threadIndex = index + 1;

	return 0;
}

static int ProcessArgs(Writer *writer, const RecordArgument *args, size_t numArgs, internal::ProcessedRecordArgument *processedArgs) {
	for (size_t i = 0; i < numArgs; ++i) {
		// First we process the name
		if (args[i].name.useStringTable) {
			int ret = GetOrCreateStringIndex(writer, args[i].name.name, args[i].name.nameLen, &processedArgs[i].nameStringRef);
			if (ret != 0) {
				return ret;
			}
			processedArgs[i].nameSizeInWords = 0;
		} else {
			if (args[i].name.nameLen > internal::StringRefFields::MaxInlineStrLen) {
				return FXT_ERR_ARG_NAME_TOO_LONG;
			}

			processedArgs[i].nameStringRef = internal::StringRefFields::Inline(args[i].name.nameLen);
			const size_t paddedNameStrLen = (args[i].name.nameLen + 8 - 1) & (-8);
			processedArgs[i].nameSizeInWords = paddedNameStrLen / 8;
		}

		// Then we process the value
		switch (args[i].value.type) {
		case internal::ArgumentType::Null:
			processedArgs[i].headerAndValueSizeInWords = 1;
			break;
		case internal::ArgumentType::Int32:
			processedArgs[i].headerAndValueSizeInWords = 1;
			break;
		case internal::ArgumentType::UInt32:
			processedArgs[i].headerAndValueSizeInWords = 1;
			break;
		case internal::ArgumentType::Int64:
			processedArgs[i].headerAndValueSizeInWords = 2;
			break;
		case internal::ArgumentType::UInt64:
			processedArgs[i].headerAndValueSizeInWords = 2;
			break;
		case internal::ArgumentType::Double:
			processedArgs[i].headerAndValueSizeInWords = 2;
			break;
		case internal::ArgumentType::String: {
			if (args[i].value.useStringTable) {
				int ret = GetOrCreateStringIndex(writer, args[i].value.stringValue, args[i].value.stringLen, &processedArgs[i].valueStringRef);
				if (ret != 0) {
					return ret;
				}
				processedArgs[i].headerAndValueSizeInWords = 0;
			} else {
				if (args[i].value.stringLen > internal::StringRefFields::MaxInlineStrLen) {
					return FXT_ERR_ARG_STR_VALUE_TOO_LONG;
				}

				processedArgs[i].valueStringRef = internal::StringRefFields::Inline(args[i].value.stringLen);
				const size_t paddedValueStrLen = (args[i].value.stringLen + 8 - 1) & (-8);
				processedArgs[i].headerAndValueSizeInWords = paddedValueStrLen / 8;
			}
			break;
		}
		case internal::ArgumentType::Pointer:
			processedArgs[i].headerAndValueSizeInWords = 2;
			break;
		case internal::ArgumentType::KOID:
			processedArgs[i].headerAndValueSizeInWords = 2;
			break;
		case internal::ArgumentType::Bool:
			processedArgs[i].headerAndValueSizeInWords = 1;
			break;
		}
	}

	return 0;
}

static int WriteArg(Writer *writer, const RecordArgument *arg, internal::ProcessedRecordArgument *processedArg, unsigned *wordsWritten) {
	switch (arg->value.type) {
	case internal::ArgumentType::Null: {
		const unsigned sizeInWords = processedArg->nameSizeInWords + processedArg->headerAndValueSizeInWords;
		const uint64_t header = internal::ArgumentFields::Type::Make(ToUnderlyingType(arg->value.type)) |
		                        internal::ArgumentFields::ArgumentSize::Make(sizeInWords) |
		                        internal::ArgumentFields::NameRef::Make(processedArg->nameStringRef);
		int ret = WriteUInt64ToStream(writer, header);
		if (ret != 0) {
			return ret;
		}

		// Write the name string if we're using inline strings
		if (!arg->name.useStringTable) {
			unsigned diff = processedArg->nameSizeInWords * 8 - arg->name.nameLen;
			ret = WriteBytesToStream(writer, arg->name.name, arg->name.nameLen);
			if (ret != 0) {
				return ret;
			}
			if (diff > 0) {
				ret = WriteZeroPadding(writer, diff);
				if (ret != 0) {
					return ret;
				}
			}
		}

		*wordsWritten = sizeInWords;
		return 0;
	}
	case internal::ArgumentType::Int32: {
		const unsigned sizeInWords = processedArg->nameSizeInWords + processedArg->headerAndValueSizeInWords;
		const uint64_t header = internal::Int32ArgumentFields::Type::Make(ToUnderlyingType(arg->value.type)) |
		                        internal::Int32ArgumentFields::ArgumentSize::Make(sizeInWords) |
		                        internal::Int32ArgumentFields::NameRef::Make(processedArg->nameStringRef) |
		                        internal::Int32ArgumentFields::Value::Make(arg->value.int32Value);
		int ret = WriteUInt64ToStream(writer, header);
		if (ret != 0) {
			return ret;
		}

		// Write the name string if we're using inline strings
		if (!arg->name.useStringTable) {
			unsigned diff = processedArg->nameSizeInWords * 8 - arg->name.nameLen;
			ret = WriteBytesToStream(writer, arg->name.name, arg->name.nameLen);
			if (ret != 0) {
				return ret;
			}
			if (diff > 0) {
				ret = WriteZeroPadding(writer, diff);
				if (ret != 0) {
					return ret;
				}
			}
		}

		*wordsWritten = sizeInWords;
		return 0;
	}
	case internal::ArgumentType::UInt32: {
		const unsigned sizeInWords = processedArg->nameSizeInWords + processedArg->headerAndValueSizeInWords;
		const uint64_t header = internal::UInt32ArgumentFields::Type::Make(ToUnderlyingType(arg->value.type)) |
		                        internal::UInt32ArgumentFields::ArgumentSize::Make(sizeInWords) |
		                        internal::UInt32ArgumentFields::NameRef::Make(processedArg->nameStringRef) |
		                        internal::UInt32ArgumentFields::Value::Make(arg->value.uint32Value);
		int ret = WriteUInt64ToStream(writer, header);
		if (ret != 0) {
			return ret;
		}

		// Write the name string if we're using inline strings
		if (!arg->name.useStringTable) {
			unsigned diff = processedArg->nameSizeInWords * 8 - arg->name.nameLen;
			ret = WriteBytesToStream(writer, arg->name.name, arg->name.nameLen);
			if (ret != 0) {
				return ret;
			}
			if (diff > 0) {
				ret = WriteZeroPadding(writer, diff);
				if (ret != 0) {
					return ret;
				}
			}
		}

		*wordsWritten = sizeInWords;
		return 0;
	}
	case internal::ArgumentType::Int64: {
		const unsigned sizeInWords = processedArg->nameSizeInWords + processedArg->headerAndValueSizeInWords;
		const uint64_t header = internal::ArgumentFields::Type::Make(ToUnderlyingType(arg->value.type)) |
		                        internal::ArgumentFields::ArgumentSize::Make(sizeInWords) |
		                        internal::ArgumentFields::NameRef::Make(processedArg->nameStringRef);
		int ret = WriteUInt64ToStream(writer, header);
		if (ret != 0) {
			return ret;
		}

		// Write the name string if we're using inline strings
		if (!arg->name.useStringTable) {
			unsigned diff = processedArg->nameSizeInWords * 8 - arg->name.nameLen;
			ret = WriteBytesToStream(writer, arg->name.name, arg->name.nameLen);
			if (ret != 0) {
				return ret;
			}
			if (diff > 0) {
				ret = WriteZeroPadding(writer, diff);
				if (ret != 0) {
					return ret;
				}
			}
		}

		ret = WriteUInt64ToStream(writer, (uint64_t)arg->value.int64Value);
		if (ret != 0) {
			return ret;
		}

		*wordsWritten = sizeInWords;
		return 0;
	}
	case internal::ArgumentType::UInt64: {
		const unsigned sizeInWords = processedArg->nameSizeInWords + processedArg->headerAndValueSizeInWords;
		const uint64_t header = internal::ArgumentFields::Type::Make(ToUnderlyingType(arg->value.type)) |
		                        internal::ArgumentFields::ArgumentSize::Make(sizeInWords) |
		                        internal::ArgumentFields::NameRef::Make(processedArg->nameStringRef);
		int ret = WriteUInt64ToStream(writer, header);
		if (ret != 0) {
			return ret;
		}

		// Write the name string if we're using inline strings
		if (!arg->name.useStringTable) {
			unsigned diff = processedArg->nameSizeInWords * 8 - arg->name.nameLen;
			ret = WriteBytesToStream(writer, arg->name.name, arg->name.nameLen);
			if (ret != 0) {
				return ret;
			}
			if (diff > 0) {
				ret = WriteZeroPadding(writer, diff);
				if (ret != 0) {
					return ret;
				}
			}
		}

		ret = WriteUInt64ToStream(writer, arg->value.uint64Value);
		if (ret != 0) {
			return ret;
		}

		*wordsWritten = sizeInWords;
		return 0;
	}
	case internal::ArgumentType::Double: {
		const unsigned sizeInWords = processedArg->nameSizeInWords + processedArg->headerAndValueSizeInWords;
		const uint64_t header = internal::ArgumentFields::Type::Make(ToUnderlyingType(arg->value.type)) |
		                        internal::ArgumentFields::ArgumentSize::Make(sizeInWords) |
		                        internal::ArgumentFields::NameRef::Make(processedArg->nameStringRef);
		int ret = WriteUInt64ToStream(writer, header);
		if (ret != 0) {
			return ret;
		}

		// Write the name string if we're using inline strings
		if (!arg->name.useStringTable) {
			unsigned diff = processedArg->nameSizeInWords * 8 - arg->name.nameLen;
			ret = WriteBytesToStream(writer, arg->name.name, arg->name.nameLen);
			if (ret != 0) {
				return ret;
			}
			if (diff > 0) {
				ret = WriteZeroPadding(writer, diff);
				if (ret != 0) {
					return ret;
				}
			}
		}

		ret = WriteUInt64ToStream(writer, *(uint64_t *)(&arg->value.doubleValue));
		if (ret != 0) {
			return ret;
		}

		*wordsWritten = sizeInWords;
		return 0;
	}
	case internal::ArgumentType::String: {
		const unsigned sizeInWords = processedArg->nameSizeInWords + processedArg->headerAndValueSizeInWords;
		const uint64_t header = internal::StringArgumentFields::Type::Make(ToUnderlyingType(arg->value.type)) |
		                        internal::StringArgumentFields::ArgumentSize::Make(sizeInWords) |
		                        internal::StringArgumentFields::NameRef::Make(processedArg->nameStringRef) |
		                        internal::StringArgumentFields::ValueRef::Make(processedArg->valueStringRef);
		int ret = WriteUInt64ToStream(writer, header);
		if (ret != 0) {
			return ret;
		}

		// Write the name string if we're using inline strings
		if (!arg->name.useStringTable) {
			unsigned diff = processedArg->nameSizeInWords * 8 - arg->name.nameLen;
			ret = WriteBytesToStream(writer, arg->name.name, arg->name.nameLen);
			if (ret != 0) {
				return ret;
			}
			if (diff > 0) {
				ret = WriteZeroPadding(writer, diff);
				if (ret != 0) {
					return ret;
				}
			}
		}

		// Write the value string if we're using inline strings
		if (!arg->value.useStringTable) {
			unsigned diff = processedArg->headerAndValueSizeInWords * 8 - arg->value.stringLen;
			ret = WriteBytesToStream(writer, arg->value.stringValue, arg->value.stringLen);
			if (ret != 0) {
				return ret;
			}
			if (diff > 0) {
				ret = WriteZeroPadding(writer, diff);
				if (ret != 0) {
					return ret;
				}
			}
		}

		*wordsWritten = sizeInWords;
		return 0;
	}
	case internal::ArgumentType::Pointer: {
		const unsigned sizeInWords = processedArg->nameSizeInWords + processedArg->headerAndValueSizeInWords;
		const uint64_t header = internal::ArgumentFields::Type::Make(ToUnderlyingType(arg->value.type)) |
		                        internal::ArgumentFields::ArgumentSize::Make(sizeInWords) |
		                        internal::ArgumentFields::NameRef::Make(processedArg->nameStringRef);
		int ret = WriteUInt64ToStream(writer, header);
		if (ret != 0) {
			return ret;
		}

		// Write the name string if we're using inline strings
		if (!arg->name.useStringTable) {
			unsigned diff = processedArg->nameSizeInWords * 8 - arg->name.nameLen;
			ret = WriteBytesToStream(writer, arg->name.name, arg->name.nameLen);
			if (ret != 0) {
				return ret;
			}
			if (diff > 0) {
				ret = WriteZeroPadding(writer, diff);
				if (ret != 0) {
					return ret;
				}
			}
		}

		ret = WriteUInt64ToStream(writer, (uint64_t)arg->value.pointerValue);
		if (ret != 0) {
			return ret;
		}

		*wordsWritten = sizeInWords;
		return 0;
	}
	case internal::ArgumentType::KOID: {
		const unsigned sizeInWords = processedArg->nameSizeInWords + processedArg->headerAndValueSizeInWords;
		const uint64_t header = internal::ArgumentFields::Type::Make(ToUnderlyingType(arg->value.type)) |
		                        internal::ArgumentFields::ArgumentSize::Make(sizeInWords) |
		                        internal::ArgumentFields::NameRef::Make(processedArg->nameStringRef);
		int ret = WriteUInt64ToStream(writer, header);
		if (ret != 0) {
			return ret;
		}

		// Write the name string if we're using inline strings
		if (!arg->name.useStringTable) {
			unsigned diff = processedArg->nameSizeInWords * 8 - arg->name.nameLen;
			ret = WriteBytesToStream(writer, arg->name.name, arg->name.nameLen);
			if (ret != 0) {
				return ret;
			}
			if (diff > 0) {
				ret = WriteZeroPadding(writer, diff);
				if (ret != 0) {
					return ret;
				}
			}
		}

		ret = WriteUInt64ToStream(writer, (uint64_t)arg->value.koidValue);
		if (ret != 0) {
			return ret;
		}

		*wordsWritten = sizeInWords;
		return 0;
	}
	case internal::ArgumentType::Bool: {
		const unsigned sizeInWords = processedArg->nameSizeInWords + processedArg->headerAndValueSizeInWords;
		const uint64_t header = internal::BoolArgumentFields::Type::Make(ToUnderlyingType(arg->value.type)) |
		                        internal::BoolArgumentFields::ArgumentSize::Make(sizeInWords) |
		                        internal::BoolArgumentFields::NameRef::Make(processedArg->nameStringRef) |
		                        internal::BoolArgumentFields::Value::Make(arg->value.boolValue ? 1 : 0);
		int ret = WriteUInt64ToStream(writer, header);
		if (ret != 0) {
			return ret;
		}

		// Write the name string if we're using inline strings
		if (!arg->name.useStringTable) {
			unsigned diff = processedArg->nameSizeInWords * 8 - arg->name.nameLen;
			ret = WriteBytesToStream(writer, arg->name.name, arg->name.nameLen);
			if (ret != 0) {
				return ret;
			}
			if (diff > 0) {
				ret = WriteZeroPadding(writer, diff);
				if (ret != 0) {
					return ret;
				}
			}
		}

		*wordsWritten = sizeInWords;
		return 0;
	}
	default:
		return FXT_ERR_INVALID_ARG_TYPE;
	}
}

int SetProcessName(Writer *writer, KernelObjectID processID, const char *name) {
	// TODO: Just use an inline string
	//       It's unlikely we'll ever get use of this string again
	uint16_t nameIndex;
	int ret = GetOrCreateStringIndex(writer, name, &nameIndex);
	if (ret != 0) {
		return ret;
	}

	// Write the header
	const uint64_t sizeInWords = /* header */ 1 + /* processID */ 1;
	const uint64_t numArgs = 0;
	const uint64_t header = internal::KernelObjectRecordFields::Type::Make(ToUnderlyingType(internal::RecordType::KernelObject)) |
	                        internal::KernelObjectRecordFields::RecordSize::Make(sizeInWords) |
	                        internal::KernelObjectRecordFields::ObjectType::Make(ToUnderlyingType(internal::KOIDType::Process)) |
	                        internal::KernelObjectRecordFields::NameStringRef::Make(nameIndex) |
	                        internal::KernelObjectRecordFields::ArgumentCount::Make(numArgs);
	ret = WriteUInt64ToStream(writer, header);
	if (ret != 0) {
		return ret;
	}

	// Then the process ID
	ret = WriteUInt64ToStream(writer, processID);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

int SetThreadName(Writer *writer, KernelObjectID processID, KernelObjectID threadID, const char *name) {
	// TODO: Just use an inline string
	//       It's unlikely we'll ever get use of this string again
	uint16_t nameIndex;
	int ret = GetOrCreateStringIndex(writer, name, &nameIndex);
	if (ret != 0) {
		return ret;
	}

	RecordArgument processArg("process", RecordArgumentValue::KOID(processID));

	internal::ProcessedRecordArgument processedProcessArg;
	ret = ProcessArgs(writer, &processArg, 1, &processedProcessArg);
	if (ret != 0) {
		return ret;
	}

	const unsigned argSizeInWords = processedProcessArg.nameSizeInWords + processedProcessArg.headerAndValueSizeInWords;
	const uint64_t sizeInWords = /* header */ 1 + /* threadID */ 1 + /* argument data */ argSizeInWords;
	if (sizeInWords > internal::KernelObjectRecordFields::kMaxRecordSizeWords) {
		return FXT_ERR_RECORD_SIZE_TOO_LARGE;
	}
	const uint64_t numArgs = 1;
	const uint64_t header = internal::KernelObjectRecordFields::Type::Make(ToUnderlyingType(internal::RecordType::KernelObject)) |
	                        internal::KernelObjectRecordFields::RecordSize::Make(sizeInWords) |
	                        internal::KernelObjectRecordFields::ObjectType::Make(ToUnderlyingType(internal::KOIDType::Thread)) |
	                        internal::KernelObjectRecordFields::NameStringRef::Make(nameIndex) |
	                        internal::KernelObjectRecordFields::ArgumentCount::Make(numArgs);
	ret = WriteUInt64ToStream(writer, header);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream(writer, threadID);
	if (ret != 0) {
		return ret;
	}

	// Write KIOD Argument to reference the process ID

	unsigned wordsWritten;
	ret = WriteArg(writer, &processArg, &processedProcessArg, &wordsWritten);
	if (ret != 0) {
		return ret;
	}
	if (wordsWritten != argSizeInWords) {
		return FXT_ERR_WRITE_LENGTH_MISMATCH;
	}

	return 0;
}

static int WriteEventHeaderAndGenericData(Writer *writer, internal::EventType eventType, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, unsigned extraSizeInWords, const RecordArgument *args, size_t numArgs) {
	if (numArgs > FXT_MAX_NUM_ARGS) {
		return FXT_ERR_TOO_MANY_ARGS;
	}

	uint16_t categoryIndex;
	int ret = GetOrCreateStringIndex(writer, category, &categoryIndex);
	if (ret != 0) {
		return ret;
	}

	uint16_t nameIndex;
	ret = GetOrCreateStringIndex(writer, name, &nameIndex);
	if (ret != 0) {
		return ret;
	}

	uint16_t threadIndex;
	ret = GetOrCreateThreadIndex(writer, processID, threadID, &threadIndex);
	if (ret != 0) {
		return ret;
	}

	internal::ProcessedRecordArgument processedArgs[FXT_MAX_NUM_ARGS];
	ret = ProcessArgs(writer, args, numArgs, processedArgs);
	if (ret != 0) {
		return ret;
	}

	// Add up the argument word size
	unsigned argumentSizeInWords = 0;
	for (size_t i = 0; i < numArgs; ++i) {
		argumentSizeInWords += processedArgs[i].nameSizeInWords + processedArgs[i].headerAndValueSizeInWords;
	}

	const uint64_t sizeInWords = /* Header */ 1 + /* timestamp */ 1 + /* argument data */ argumentSizeInWords + /* extra stuff */ extraSizeInWords;
	const uint64_t header = internal::EventRecordFields::Type::Make(ToUnderlyingType(internal::RecordType::Event)) |
	                        internal::EventRecordFields::RecordSize::Make(sizeInWords) |
	                        internal::EventRecordFields::EventType::Make(ToUnderlyingType(eventType)) |
	                        internal::EventRecordFields::ArgumentCount::Make(numArgs) |
	                        internal::EventRecordFields::ThreadRef::Make(threadIndex) |
	                        internal::EventRecordFields::CategoryStringRef::Make(categoryIndex) |
	                        internal::EventRecordFields::NameStringRef::Make(nameIndex);
	ret = WriteUInt64ToStream(writer, header);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream(writer, timestamp);
	if (ret != 0) {
		return ret;
	}

	unsigned wordsWritten = 0;
	for (size_t i = 0; i < numArgs; ++i) {
		unsigned size;
		ret = WriteArg(writer, &args[i], &processedArgs[i], &size);
		if (ret != 0) {
			return ret;
		}
		wordsWritten += size;
	}

	if (wordsWritten != argumentSizeInWords) {
		return FXT_ERR_WRITE_LENGTH_MISMATCH;
	}

	return 0;
}

int AddInstantEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp) {
	return AddInstantEvent(writer, category, name, processID, threadID, timestamp, nullptr, 0);
}

int AddInstantEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, std::initializer_list<RecordArgument> args) {
	return AddInstantEvent(writer, category, name, processID, threadID, timestamp, args.begin(), args.size());
}

int AddInstantEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, const RecordArgument *args, size_t numArgs) {
	constexpr unsigned extraSizeInWords = 0;
	return WriteEventHeaderAndGenericData(writer, internal::EventType::Instant, category, name, processID, threadID, timestamp, extraSizeInWords, args, numArgs);
}

int AddCounterEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t counterID, std::initializer_list<RecordArgument> args) {
	return AddCounterEvent(writer, category, name, processID, threadID, timestamp, counterID, args.begin(), args.size());
}

int AddCounterEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t counterID, const RecordArgument *args, size_t numArgs) {
	constexpr unsigned extraSizeInWords = 1;
	int ret = WriteEventHeaderAndGenericData(writer, internal::EventType::Counter, category, name, processID, threadID, timestamp, extraSizeInWords, args, numArgs);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream(writer, counterID);
	if (ret != 0) {
		return ret;
	}
	return 0;
}

int AddDurationBeginEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp) {
	return AddDurationBeginEvent(writer, category, name, processID, threadID, timestamp, nullptr, 0);
}

int AddDurationBeginEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, std::initializer_list<RecordArgument> args) {
	return AddDurationBeginEvent(writer, category, name, processID, threadID, timestamp, args.begin(), args.size());
}

int AddDurationBeginEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, const RecordArgument *args, size_t numArgs) {
	constexpr unsigned extraSizeInWords = 0;
	return WriteEventHeaderAndGenericData(writer, internal::EventType::DurationBegin, category, name, processID, threadID, timestamp, extraSizeInWords, args, numArgs);
}

int AddDurationEndEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp) {
	return AddDurationEndEvent(writer, category, name, processID, threadID, timestamp, nullptr, 0);
}

int AddDurationEndEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, std::initializer_list<RecordArgument> args) {
	return AddDurationEndEvent(writer, category, name, processID, threadID, timestamp, args.begin(), args.size());
}

int AddDurationEndEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, const RecordArgument *args, size_t numArgs) {
	constexpr unsigned extraSizeInWords = 0;
	return WriteEventHeaderAndGenericData(writer, internal::EventType::DurationEnd, category, name, processID, threadID, timestamp, extraSizeInWords, args, numArgs);
}

int AddDurationCompleteEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t beginTimestamp, uint64_t endTimestamp) {
	return AddDurationCompleteEvent(writer, category, name, processID, threadID, beginTimestamp, endTimestamp, nullptr, 0);
}

int AddDurationCompleteEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t beginTimestamp, uint64_t endTimestamp, std::initializer_list<RecordArgument> args) {
	return AddDurationCompleteEvent(writer, category, name, processID, threadID, beginTimestamp, endTimestamp, args.begin(), args.size());
}

int AddDurationCompleteEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t beginTimestamp, uint64_t endTimestamp, const RecordArgument *args, size_t numArgs) {
	constexpr unsigned extraSizeInWords = 1;
	int ret = WriteEventHeaderAndGenericData(writer, internal::EventType::DurationComplete, category, name, processID, threadID, beginTimestamp, extraSizeInWords, args, numArgs);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream(writer, endTimestamp);
	if (ret != 0) {
		return ret;
	}
	return 0;
}

int AddAsyncBeginEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t asyncCorrelationID) {
	return AddAsyncBeginEvent(writer, category, name, processID, threadID, timestamp, asyncCorrelationID, nullptr, 0);
}

int AddAsyncBeginEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t asyncCorrelationID, std::initializer_list<RecordArgument> args) {
	return AddAsyncBeginEvent(writer, category, name, processID, threadID, timestamp, asyncCorrelationID, args.begin(), args.size());
}

int AddAsyncBeginEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t asyncCorrelationID, const RecordArgument *args, size_t numArgs) {
	const unsigned extraSizeInWords = 1;
	int ret = WriteEventHeaderAndGenericData(writer, internal::EventType::AsyncBegin, category, name, processID, threadID, timestamp, extraSizeInWords, args, numArgs);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream(writer, asyncCorrelationID);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

int AddAsyncInstantEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t asyncCorrelationID) {
	return AddAsyncInstantEvent(writer, category, name, processID, threadID, timestamp, asyncCorrelationID, nullptr, 0);
}

int AddAsyncInstantEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t asyncCorrelationID, std::initializer_list<RecordArgument> args) {
	return AddAsyncInstantEvent(writer, category, name, processID, threadID, timestamp, asyncCorrelationID, args.begin(), args.size());
}

int AddAsyncInstantEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t asyncCorrelationID, const RecordArgument *args, size_t numArgs) {
	const unsigned extraSizeInWords = 1;
	int ret = WriteEventHeaderAndGenericData(writer, internal::EventType::AsyncInstant, category, name, processID, threadID, timestamp, extraSizeInWords, args, numArgs);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream(writer, asyncCorrelationID);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

int AddAsyncEndEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t asyncCorrelationID) {
	return AddAsyncEndEvent(writer, category, name, processID, threadID, timestamp, asyncCorrelationID, nullptr, 0);
}

int AddAsyncEndEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t asyncCorrelationID, std::initializer_list<RecordArgument> args) {
	return AddAsyncEndEvent(writer, category, name, processID, threadID, timestamp, asyncCorrelationID, args.begin(), args.size());
}

int AddAsyncEndEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t asyncCorrelationID, const RecordArgument *args, size_t numArgs) {
	const unsigned extraSizeInWords = 1;
	int ret = WriteEventHeaderAndGenericData(writer, internal::EventType::AsyncEnd, category, name, processID, threadID, timestamp, extraSizeInWords, args, numArgs);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream(writer, asyncCorrelationID);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

int AddFlowBeginEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t flowCorrelationID) {
	return AddFlowBeginEvent(writer, category, name, processID, threadID, timestamp, flowCorrelationID, nullptr, 0);
}

int AddFlowBeginEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t flowCorrelationID, std::initializer_list<RecordArgument> args) {
	return AddFlowBeginEvent(writer, category, name, processID, threadID, timestamp, flowCorrelationID, args.begin(), args.size());
}

int AddFlowBeginEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t flowCorrelationID, const RecordArgument *args, size_t numArgs) {
	const unsigned extraSizeInWords = 1;
	int ret = WriteEventHeaderAndGenericData(writer, internal::EventType::FlowBegin, category, name, processID, threadID, timestamp, extraSizeInWords, args, numArgs);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream(writer, flowCorrelationID);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

int AddFlowStepEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t flowCorrelationID) {
	return AddFlowStepEvent(writer, category, name, processID, threadID, timestamp, flowCorrelationID, nullptr, 0);
}

int AddFlowStepEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t flowCorrelationID, std::initializer_list<RecordArgument> args) {
	return AddFlowStepEvent(writer, category, name, processID, threadID, timestamp, flowCorrelationID, args.begin(), args.size());
}

int AddFlowStepEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t flowCorrelationID, const RecordArgument *args, size_t numArgs) {
	const unsigned extraSizeInWords = 1;
	int ret = WriteEventHeaderAndGenericData(writer, internal::EventType::FlowStep, category, name, processID, threadID, timestamp, extraSizeInWords, args, numArgs);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream(writer, flowCorrelationID);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

int AddFlowEndEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t flowCorrelationID) {
	return AddFlowEndEvent(writer, category, name, processID, threadID, timestamp, flowCorrelationID, nullptr, 0);
}

int AddFlowEndEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t flowCorrelationID, std::initializer_list<RecordArgument> args) {
	return AddFlowEndEvent(writer, category, name, processID, threadID, timestamp, flowCorrelationID, args.begin(), args.size());
}

int AddFlowEndEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t flowCorrelationID, const RecordArgument *args, size_t numArgs) {
	const unsigned extraSizeInWords = 1;
	int ret = WriteEventHeaderAndGenericData(writer, internal::EventType::FlowEnd, category, name, processID, threadID, timestamp, extraSizeInWords, args, numArgs);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream(writer, flowCorrelationID);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

int AddBlobRecord(Writer *writer, const char *name, void *data, size_t dataLen, BlobType blobType) {
	if (dataLen > internal::BlobRecordFields::kMaxBlobLength) {
		// Blob length is stored in 23 bits
		return FXT_ERR_DATA_TOO_LONG;
	}

	uint16_t nameIndex;
	int ret = GetOrCreateStringIndex(writer, name, &nameIndex);
	if (ret != 0) {
		return ret;
	}

	const size_t paddedSize = (dataLen + 8 - 1) & (-8);
	const size_t diff = paddedSize - dataLen;

	// Write the header
	const uint64_t sizeInWords = 1 + (paddedSize / 8);
	const uint64_t header = internal::BlobRecordFields::Type::Make(ToUnderlyingType(internal::RecordType::Blob)) |
	                        internal::BlobRecordFields::RecordSize::Make(sizeInWords) |
	                        internal::BlobRecordFields::NameStringRef::Make(nameIndex) |
	                        internal::BlobRecordFields::BlobSize::Make(dataLen) |
	                        internal::BlobRecordFields::BlobType::Make(ToUnderlyingType(blobType));
	ret = WriteUInt64ToStream(writer, header);
	if (ret != 0) {
		return ret;
	}

	// Then the data
	ret = WriteBytesToStream(writer, data, dataLen);
	if (ret != 0) {
		return ret;
	}

	// And the zero padding
	if (diff > 0) {
		ret = WriteZeroPadding(writer, diff);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

int AddUserspaceObjectRecord(Writer *writer, const char *name, KernelObjectID processID, KernelObjectID threadID, uintptr_t pointerValue) {
	return AddUserspaceObjectRecord(writer, name, processID, threadID, pointerValue, nullptr, 0);
}

int AddUserspaceObjectRecord(Writer *writer, const char *name, KernelObjectID processID, KernelObjectID threadID, uintptr_t pointerValue, std::initializer_list<RecordArgument> args) {
	return AddUserspaceObjectRecord(writer, name, processID, threadID, pointerValue, args.begin(), args.size());
}

int AddUserspaceObjectRecord(Writer *writer, const char *name, KernelObjectID processID, KernelObjectID threadID, uintptr_t pointerValue, const RecordArgument *args, size_t numArgs) {
	if (numArgs > FXT_MAX_NUM_ARGS) {
		return FXT_ERR_TOO_MANY_ARGS;
	}

	uint16_t nameIndex;
	int ret = GetOrCreateStringIndex(writer, name, &nameIndex);
	if (ret != 0) {
		return ret;
	}

	uint16_t threadIndex;
	ret = GetOrCreateThreadIndex(writer, processID, threadID, &threadIndex);
	if (ret != 0) {
		return ret;
	}

	internal::ProcessedRecordArgument processedArgs[FXT_MAX_NUM_ARGS];
	ret = ProcessArgs(writer, args, numArgs, processedArgs);
	if (ret != 0) {
		return ret;
	}

	// Add up the argument word size
	unsigned argumentSizeInWords = 0;
	for (size_t i = 0; i < numArgs; ++i) {
		argumentSizeInWords += processedArgs[i].nameSizeInWords + processedArgs[i].headerAndValueSizeInWords;
	}

	const uint64_t sizeInWords = /* Header */ 1 + /* pointer value */ 1 + /* argument data */ argumentSizeInWords;
	const uint64_t header = internal::UserspaceObjectRecordFields::Type::Make(ToUnderlyingType(internal::RecordType::UserspaceObject)) |
	                        internal::UserspaceObjectRecordFields::RecordSize::Make(sizeInWords) |
	                        internal::UserspaceObjectRecordFields::ThreadRef::Make(threadIndex) |
	                        internal::UserspaceObjectRecordFields::NameStringRef::Make(nameIndex) |
	                        internal::UserspaceObjectRecordFields::ArgumentCount::Make(numArgs);
	ret = WriteUInt64ToStream(writer, header);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream(writer, (uint64_t)pointerValue);
	if (ret != 0) {
		return ret;
	}

	unsigned wordsWritten = 0;
	for (size_t i = 0; i < numArgs; ++i) {
		unsigned size;
		ret = WriteArg(writer, &args[i], &processedArgs[i], &size);
		if (ret != 0) {
			return ret;
		}
		wordsWritten += size;
	}

	if (wordsWritten != argumentSizeInWords) {
		return FXT_ERR_WRITE_LENGTH_MISMATCH;
	}

	return 0;
}

int AddContextSwitchRecord(Writer *writer, uint16_t cpuNumber, uint8_t outgoingThreadState, KernelObjectID outgoingThreadID, KernelObjectID incomingThreadID, uint64_t timestamp) {
	return AddContextSwitchRecord(writer, cpuNumber, outgoingThreadState, outgoingThreadID, incomingThreadID, timestamp, nullptr, 0);
}

int AddContextSwitchRecord(Writer *writer, uint16_t cpuNumber, uint8_t outgoingThreadState, KernelObjectID outgoingThreadID, KernelObjectID incomingThreadID, uint64_t timestamp, std::initializer_list<RecordArgument> args) {
	return AddContextSwitchRecord(writer, cpuNumber, outgoingThreadState, outgoingThreadID, incomingThreadID, timestamp, args.begin(), args.size());
}

int AddContextSwitchRecord(Writer *writer, uint16_t cpuNumber, uint8_t outgoingThreadState, KernelObjectID outgoingThreadID, KernelObjectID incomingThreadID, uint64_t timestamp, const RecordArgument *args, size_t numArgs) {
	// Sanity check
	// Ideally we'd find out the actual ENUM of valid states
	if (outgoingThreadState > 0xF) {
		return FXT_ERR_INVALID_OUTGOING_THREAD_STATE;
	}

	if (numArgs > FXT_MAX_NUM_ARGS) {
		return FXT_ERR_TOO_MANY_ARGS;
	}

	internal::ProcessedRecordArgument processedArgs[FXT_MAX_NUM_ARGS];
	int ret = ProcessArgs(writer, args, numArgs, processedArgs);
	if (ret != 0) {
		return ret;
	}

	// Add up the argument word size
	unsigned argumentSizeInWords = 0;
	for (size_t i = 0; i < numArgs; ++i) {
		argumentSizeInWords += processedArgs[i].nameSizeInWords + processedArgs[i].headerAndValueSizeInWords;
	}

	const uint64_t sizeInWords = /* Header */ 1 + /* timestamp */ 1 + /* outgoing thread ID */ 1 + /* incoming thread ID */ 1 + /* argument data */ argumentSizeInWords;
	const uint64_t header = internal::ContextSwitchRecordFields::Type::Make(ToUnderlyingType(internal::RecordType::Scheduling)) |
	                        internal::ContextSwitchRecordFields::RecordSize::Make(sizeInWords) |
	                        internal::ContextSwitchRecordFields::ArgumentCount::Make(numArgs) |
	                        internal::ContextSwitchRecordFields::CpuNumber::Make(cpuNumber) |
	                        internal::ContextSwitchRecordFields::OutgoingThreadState::Make(outgoingThreadState) |
	                        internal::ContextSwitchRecordFields::EventType::Make(ToUnderlyingType(internal::SchedulingRecordType::ContextSwitch));
	ret = WriteUInt64ToStream(writer, header);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream(writer, timestamp);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream(writer, (uint64_t)outgoingThreadID);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream(writer, (uint64_t)incomingThreadID);
	if (ret != 0) {
		return ret;
	}

	unsigned wordsWritten = 0;
	for (size_t i = 0; i < numArgs; ++i) {
		unsigned size;
		ret = WriteArg(writer, &args[i], &processedArgs[i], &size);
		if (ret != 0) {
			return ret;
		}
		wordsWritten += size;
	}

	if (wordsWritten != argumentSizeInWords) {
		return FXT_ERR_WRITE_LENGTH_MISMATCH;
	}

	return 0;
}

int AddFiberSwitchRecord(Writer *writer, KernelObjectID processID, KernelObjectID threadID, KernelObjectID outgoingFiberID, KernelObjectID incomingFiberID, uint64_t timestamp) {
	return AddFiberSwitchRecord(writer, processID, threadID, outgoingFiberID, incomingFiberID, timestamp, nullptr, 0);
}

int AddFiberSwitchRecord(Writer *writer, KernelObjectID processID, KernelObjectID threadID, KernelObjectID outgoingFiberID, KernelObjectID incomingFiberID, uint64_t timestamp, std::initializer_list<RecordArgument> args) {
	return AddFiberSwitchRecord(writer, processID, threadID, outgoingFiberID, incomingFiberID, timestamp, args.begin(), args.size());
}

int AddFiberSwitchRecord(Writer *writer, KernelObjectID processID, KernelObjectID threadID, KernelObjectID outgoingFiberID, KernelObjectID incomingFiberID, uint64_t timestamp, const RecordArgument *args, size_t numArgs) {
	if (numArgs > FXT_MAX_NUM_ARGS) {
		return FXT_ERR_TOO_MANY_ARGS;
	}

	internal::ProcessedRecordArgument processedArgs[FXT_MAX_NUM_ARGS];
	int ret = ProcessArgs(writer, args, numArgs, processedArgs);
	if (ret != 0) {
		return ret;
	}

	// Add up the argument word size
	unsigned argumentSizeInWords = 0;
	for (size_t i = 0; i < numArgs; ++i) {
		argumentSizeInWords += processedArgs[i].nameSizeInWords + processedArgs[i].headerAndValueSizeInWords;
	}

	const uint64_t sizeInWords = /* Header */ 1 + /* timestamp */ 1 + /* outgoing fiber ID */ 1 + /* incoming fiber ID */ 1 + /* argument data */ argumentSizeInWords;
	const uint64_t header = internal::FiberSwitchRecordFields::Type::Make(ToUnderlyingType(internal::RecordType::Scheduling)) |
	                        internal::FiberSwitchRecordFields::RecordSize::Make(sizeInWords) |
	                        internal::FiberSwitchRecordFields::ArgumentCount::Make(numArgs) |
	                        internal::FiberSwitchRecordFields::EventType::Make(ToUnderlyingType(internal::SchedulingRecordType::FiberSwitch));
	ret = WriteUInt64ToStream(writer, header);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream(writer, timestamp);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream(writer, (uint64_t)outgoingFiberID);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream(writer, (uint64_t)incomingFiberID);
	if (ret != 0) {
		return ret;
	}

	unsigned wordsWritten = 0;
	for (size_t i = 0; i < numArgs; ++i) {
		unsigned size;
		ret = WriteArg(writer, &args[i], &processedArgs[i], &size);
		if (ret != 0) {
			return ret;
		}
		wordsWritten += size;
	}

	if (wordsWritten != argumentSizeInWords) {
		return FXT_ERR_WRITE_LENGTH_MISMATCH;
	}

	return 0;
}

int AddThreadWakeupRecord(Writer *writer, uint16_t cpuNumber, KernelObjectID wakingThreadID, uint64_t timestamp) {
	return AddThreadWakeupRecord(writer, cpuNumber, wakingThreadID, timestamp, nullptr, 0);
}

int AddThreadWakeupRecord(Writer *writer, uint16_t cpuNumber, KernelObjectID wakingThreadID, uint64_t timestamp, std::initializer_list<RecordArgument> args) {
	return AddThreadWakeupRecord(writer, cpuNumber, wakingThreadID, timestamp, args.begin(), args.size());
}

int AddThreadWakeupRecord(Writer *writer, uint16_t cpuNumber, KernelObjectID wakingThreadID, uint64_t timestamp, const RecordArgument *args, size_t numArgs) {
	if (numArgs > FXT_MAX_NUM_ARGS) {
		return FXT_ERR_TOO_MANY_ARGS;
	}

	internal::ProcessedRecordArgument processedArgs[FXT_MAX_NUM_ARGS];
	int ret = ProcessArgs(writer, args, numArgs, processedArgs);
	if (ret != 0) {
		return ret;
	}

	// Add up the argument word size
	unsigned argumentSizeInWords = 0;
	for (size_t i = 0; i < numArgs; ++i) {
		argumentSizeInWords += processedArgs[i].nameSizeInWords + processedArgs[i].headerAndValueSizeInWords;
	}

	const uint64_t sizeInWords = /* Header */ 1 + /* timestamp */ 1 + /* waking thread ID */ 1 + /* argument data */ argumentSizeInWords;
	const uint64_t header = internal::ThreadWakeupRecordFields::Type::Make(ToUnderlyingType(internal::RecordType::Scheduling)) |
	                        internal::ThreadWakeupRecordFields::RecordSize::Make(sizeInWords) |
	                        internal::ThreadWakeupRecordFields::ArgumentCount::Make(numArgs) |
	                        internal::ThreadWakeupRecordFields::CpuNumber::Make(cpuNumber) |
	                        internal::ThreadWakeupRecordFields::EventType::Make(ToUnderlyingType(internal::SchedulingRecordType::ThreadWakeup));
	ret = WriteUInt64ToStream(writer, header);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream(writer, timestamp);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream(writer, wakingThreadID);
	if (ret != 0) {
		return ret;
	}

	unsigned wordsWritten = 0;
	for (size_t i = 0; i < numArgs; ++i) {
		unsigned size;
		ret = WriteArg(writer, &args[i], &processedArgs[i], &size);
		if (ret != 0) {
			return ret;
		}
		wordsWritten += size;
	}

	if (wordsWritten != argumentSizeInWords) {
		return FXT_ERR_WRITE_LENGTH_MISMATCH;
	}

	return 0;
}

static int WriteUInt64ToStream(Writer *writer, uint64_t val) {
	uint8_t buffer[8];

	buffer[0] = uint8_t(val);
	buffer[1] = uint8_t(val >> 8);
	buffer[2] = uint8_t(val >> 16);
	buffer[3] = uint8_t(val >> 24);
	buffer[4] = uint8_t(val >> 32);
	buffer[5] = uint8_t(val >> 40);
	buffer[6] = uint8_t(val >> 48);
	buffer[7] = uint8_t(val >> 56);

	return writer->writeFunc(writer->userContext, buffer, sizeof(buffer));
}

static int WriteBytesToStream(Writer *writer, const void *val, size_t len) {
	return writer->writeFunc(writer->userContext, (uint8_t *)val, len);
}

static int WriteZeroPadding(Writer *writer, size_t count) {
	uint8_t zero = 0;

	for (size_t i = 0; i < count; ++i) {
		int ret = writer->writeFunc(writer->userContext, &zero, 1);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

} // End of namespace fxt
