/* FXT - A library for creating Fuschia Tracing System (FXT) files
 *
 * FXT is the legal property of Adrian Astley
 * Copyright Adrian Astley 2023
 */

#include "fxt/writer.h"

#include "constants.h"
#include "fields.h"
#include "record_args.h"

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
	// This record does follow "real" record / metadata record header patterns.
	// However, given the value is static, we just directly hardcode the byte values
	const char fxtMagic[] = { 0x10, 0x00, 0x04, 0x46, 0x78, 0x54, 0x16, 0x00 };
	return WriteBytesToStream(writer, fxtMagic, ArraySize(fxtMagic));
}

int AddProviderInfoRecord(Writer *writer, ProviderID providerID, const char *providerName) {
	const size_t strLen = strlen(providerName);
	const size_t paddedStrLen = (strLen + 8 - 1) & (-8);
	const size_t diff = paddedStrLen - strLen;

	if (paddedStrLen >= ProviderInfoMetadataRecordFields::kMaxNameLength) {
		return FXT_ERR_STR_TOO_LONG;
	}

	// Write the header
	const uint64_t sizeInWords = 1 + (paddedStrLen / 8);
	const uint64_t header = ProviderInfoMetadataRecordFields::Type::Make(ToUnderlyingType(RecordType::Metadata)) |
	                        ProviderInfoMetadataRecordFields::RecordSize::Make(sizeInWords) |
	                        ProviderInfoMetadataRecordFields::MetadataType::Make(ToUnderlyingType(MetadataType::ProviderInfo)) |
	                        ProviderInfoMetadataRecordFields::ProviderID::Make(providerID) |
	                        ProviderInfoMetadataRecordFields::NameLength::Make(strLen);
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
	const uint64_t header = ProviderSectionMetadataRecordFields::Type::Make(ToUnderlyingType(RecordType::Metadata)) |
	                        ProviderSectionMetadataRecordFields::RecordSize::Make(sizeInWords) |
	                        ProviderSectionMetadataRecordFields::MetadataType::Make(ToUnderlyingType(MetadataType::ProviderSection)) |
	                        ProviderSectionMetadataRecordFields::ProviderID::Make(providerID);
	int ret = WriteUInt64ToStream(writer, header);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

int AddProviderEventRecord(Writer *writer, ProviderID providerID, ProviderEventType eventType) {
	const uint64_t sizeInWords = 1;
	const uint64_t header = ProviderEventMetadataRecordFields::Type::Make(ToUnderlyingType(RecordType::Metadata)) |
	                        ProviderEventMetadataRecordFields::RecordSize::Make(sizeInWords) |
	                        ProviderEventMetadataRecordFields::MetadataType::Make(ToUnderlyingType(MetadataType::ProviderEvent)) |
	                        ProviderEventMetadataRecordFields::ProviderID::Make(providerID) |
	                        ProviderEventMetadataRecordFields::Event::Make(ToUnderlyingType(eventType));
	int ret = WriteUInt64ToStream(writer, header);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

int AddInitializationRecord(Writer *writer, uint64_t numTicksPerSecond) {
	const uint64_t sizeInWords = 2;
	const uint64_t header = InitializationRecordFields::Type::Make(ToUnderlyingType(RecordType::Initialization)) |
	                        InitializationRecordFields::RecordSize::Make(sizeInWords);
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
	const uint64_t header = StringRecordFields::Type::Make(ToUnderlyingType(RecordType::String)) |
	                        StringRecordFields::RecordSize::Make(sizeInWords) |
	                        StringRecordFields::StringIndex::Make(stringIndex) |
	                        StringRecordFields::StringLength::Make(strLen);
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
	const uint64_t header = ThreadRecordFields::Type::Make(ToUnderlyingType(RecordType::Thread)) |
	                        ThreadRecordFields::RecordSize::Make(sizeInWords) |
	                        ThreadRecordFields::ThreadIndex::Make(threadIndex);
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

int ProcessArgs(Writer *writer, const RecordArgument *args, size_t numArgs, ProcessedRecordArgument *processedArgs) {
	for (size_t i = 0; i < numArgs; ++i) {
		// First we process the name
		if (args[i].name.useStringTable) {
			int ret = GetOrCreateStringIndex(writer, args[i].name.name, args[i].name.nameLen, &processedArgs[i].nameStringRef);
			if (ret != 0) {
				return ret;
			}
			processedArgs[i].nameSizeInWords = 0;
		} else {
			if (args[i].name.nameLen > StringRefFields::MaxInlineStrLen) {
				return FXT_ERR_ARG_NAME_TOO_LONG;
			}

			processedArgs[i].nameStringRef = StringRefFields::Inline(args[i].name.nameLen);
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
			if (args[i].value.hexEncode) {
				// Hex encoding takes up two chars per byte
				size_t encodedLen = args[i].value.stringLen * 2;

				if (encodedLen > StringRefFields::MaxInlineStrLen) {
					return FXT_ERR_ARG_STR_VALUE_TOO_LONG;
				}

				processedArgs[i].valueStringRef = StringRefFields::Inline(encodedLen);
				const size_t paddedValueStrLen = (encodedLen + 8 - 1) & (-8);
				processedArgs[i].headerAndValueSizeInWords = 1 + paddedValueStrLen / 8;
			} else {
				if (args[i].value.useStringTable) {
					int ret = GetOrCreateStringIndex(writer, args[i].value.stringValue, args[i].value.stringLen, &processedArgs[i].valueStringRef);
					if (ret != 0) {
						return ret;
					}
					processedArgs[i].headerAndValueSizeInWords = 1;
				} else {
					if (args[i].value.stringLen > StringRefFields::MaxInlineStrLen) {
						return FXT_ERR_ARG_STR_VALUE_TOO_LONG;
					}

					processedArgs[i].valueStringRef = StringRefFields::Inline(args[i].value.stringLen);
					const size_t paddedValueStrLen = (args[i].value.stringLen + 8 - 1) & (-8);
					processedArgs[i].headerAndValueSizeInWords = 1 + paddedValueStrLen / 8;
				}
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

int WriteArg(Writer *writer, const RecordArgument *arg, ProcessedRecordArgument *processedArg, unsigned *wordsWritten) {
	switch (arg->value.type) {
	case internal::ArgumentType::Null: {
		const unsigned sizeInWords = processedArg->nameSizeInWords + processedArg->headerAndValueSizeInWords;
		const uint64_t header = ArgumentFields::Type::Make(ToUnderlyingType(arg->value.type)) |
		                        ArgumentFields::ArgumentSize::Make(sizeInWords) |
		                        ArgumentFields::NameRef::Make(processedArg->nameStringRef);
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
		const uint64_t header = Int32ArgumentFields::Type::Make(ToUnderlyingType(arg->value.type)) |
		                        Int32ArgumentFields::ArgumentSize::Make(sizeInWords) |
		                        Int32ArgumentFields::NameRef::Make(processedArg->nameStringRef) |
		                        Int32ArgumentFields::Value::Make(arg->value.int32Value);
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
		const uint64_t header = UInt32ArgumentFields::Type::Make(ToUnderlyingType(arg->value.type)) |
		                        UInt32ArgumentFields::ArgumentSize::Make(sizeInWords) |
		                        UInt32ArgumentFields::NameRef::Make(processedArg->nameStringRef) |
		                        UInt32ArgumentFields::Value::Make(arg->value.uint32Value);
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
		const uint64_t header = ArgumentFields::Type::Make(ToUnderlyingType(arg->value.type)) |
		                        ArgumentFields::ArgumentSize::Make(sizeInWords) |
		                        ArgumentFields::NameRef::Make(processedArg->nameStringRef);
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
		const uint64_t header = ArgumentFields::Type::Make(ToUnderlyingType(arg->value.type)) |
		                        ArgumentFields::ArgumentSize::Make(sizeInWords) |
		                        ArgumentFields::NameRef::Make(processedArg->nameStringRef);
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
		const uint64_t header = ArgumentFields::Type::Make(ToUnderlyingType(arg->value.type)) |
		                        ArgumentFields::ArgumentSize::Make(sizeInWords) |
		                        ArgumentFields::NameRef::Make(processedArg->nameStringRef);
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
		const uint64_t header = StringArgumentFields::Type::Make(ToUnderlyingType(arg->value.type)) |
		                        StringArgumentFields::ArgumentSize::Make(sizeInWords) |
		                        StringArgumentFields::NameRef::Make(processedArg->nameStringRef) |
		                        StringArgumentFields::ValueRef::Make(processedArg->valueStringRef);
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
		if (arg->value.hexEncode) {
			static const char hexChars[] = "0123456789abcdef";

			size_t encodedLen = arg->value.stringLen * 2;
			size_t diff = (processedArg->headerAndValueSizeInWords - 1) * 8 - encodedLen;

			char buffer[256];
			static_assert(sizeof(buffer) % 2 == 0, "buffer must be an even size, so we can guarantee we have space for both chars per byte");
			size_t offset = 0;
			size_t writtenBytes = 0;

			// Start encoding in blocks
			for (size_t i = 0; i < arg->value.stringLen; ++i) {
				if (offset > sizeof(buffer)) {
					ret = WriteBytesToStream(writer, buffer, sizeof(buffer));
					if (ret != 0) {
						return ret;
					}

					offset = 0;
				}

				buffer[offset] = hexChars[(arg->value.stringValue[i] >> 4) & 0x0F];
				buffer[offset + 1] = hexChars[(arg->value.stringValue[i]) & 0x0F];

				offset += 2;
			}

			// Flush any remaining bytes
			if (offset > 0) {
				ret = WriteBytesToStream(writer, buffer, offset);
				if (ret != 0) {
					return ret;
				}
			}

			if (diff > 0) {
				ret = WriteZeroPadding(writer, diff);
				if (ret != 0) {
					return ret;
				}
			}
		} else if (!arg->value.useStringTable) {
			size_t diff = (processedArg->headerAndValueSizeInWords - 1) * 8 - arg->value.stringLen;
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
		const uint64_t header = ArgumentFields::Type::Make(ToUnderlyingType(arg->value.type)) |
		                        ArgumentFields::ArgumentSize::Make(sizeInWords) |
		                        ArgumentFields::NameRef::Make(processedArg->nameStringRef);
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
		const uint64_t header = ArgumentFields::Type::Make(ToUnderlyingType(arg->value.type)) |
		                        ArgumentFields::ArgumentSize::Make(sizeInWords) |
		                        ArgumentFields::NameRef::Make(processedArg->nameStringRef);
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
		const uint64_t header = BoolArgumentFields::Type::Make(ToUnderlyingType(arg->value.type)) |
		                        BoolArgumentFields::ArgumentSize::Make(sizeInWords) |
		                        BoolArgumentFields::NameRef::Make(processedArg->nameStringRef) |
		                        BoolArgumentFields::Value::Make(arg->value.boolValue ? 1 : 0);
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
	const uint64_t header = KernelObjectRecordFields::Type::Make(ToUnderlyingType(RecordType::KernelObject)) |
	                        KernelObjectRecordFields::RecordSize::Make(sizeInWords) |
	                        KernelObjectRecordFields::ObjectType::Make(ToUnderlyingType(KOIDType::Process)) |
	                        KernelObjectRecordFields::NameStringRef::Make(nameIndex) |
	                        KernelObjectRecordFields::ArgumentCount::Make(numArgs);
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

	ProcessedRecordArgument processedProcessArg;
	ret = ProcessArgs(writer, &processArg, 1, &processedProcessArg);
	if (ret != 0) {
		return ret;
	}

	const unsigned argSizeInWords = processedProcessArg.nameSizeInWords + processedProcessArg.headerAndValueSizeInWords;
	const uint64_t sizeInWords = /* header */ 1 + /* threadID */ 1 + /* argument data */ argSizeInWords;
	if (sizeInWords > KernelObjectRecordFields::kMaxRecordSizeWords) {
		return FXT_ERR_RECORD_SIZE_TOO_LARGE;
	}
	const uint64_t numArgs = 1;
	const uint64_t header = KernelObjectRecordFields::Type::Make(ToUnderlyingType(RecordType::KernelObject)) |
	                        KernelObjectRecordFields::RecordSize::Make(sizeInWords) |
	                        KernelObjectRecordFields::ObjectType::Make(ToUnderlyingType(KOIDType::Thread)) |
	                        KernelObjectRecordFields::NameStringRef::Make(nameIndex) |
	                        KernelObjectRecordFields::ArgumentCount::Make(numArgs);
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

static int WriteEventHeaderAndGenericData(Writer *writer, EventType eventType, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, unsigned extraSizeInWords, const RecordArgument *args, size_t numArgs) {
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

	ProcessedRecordArgument processedArgs[FXT_MAX_NUM_ARGS];
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
	const uint64_t header = EventRecordFields::Type::Make(ToUnderlyingType(RecordType::Event)) |
	                        EventRecordFields::RecordSize::Make(sizeInWords) |
	                        EventRecordFields::EventType::Make(ToUnderlyingType(eventType)) |
	                        EventRecordFields::ArgumentCount::Make(numArgs) |
	                        EventRecordFields::ThreadRef::Make(threadIndex) |
	                        EventRecordFields::CategoryStringRef::Make(categoryIndex) |
	                        EventRecordFields::NameStringRef::Make(nameIndex);
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
	return WriteEventHeaderAndGenericData(writer, EventType::Instant, category, name, processID, threadID, timestamp, extraSizeInWords, args, numArgs);
}

int AddCounterEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t counterID, std::initializer_list<RecordArgument> args) {
	return AddCounterEvent(writer, category, name, processID, threadID, timestamp, counterID, args.begin(), args.size());
}

int AddCounterEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t counterID, const RecordArgument *args, size_t numArgs) {
	constexpr unsigned extraSizeInWords = 1;
	int ret = WriteEventHeaderAndGenericData(writer, EventType::Counter, category, name, processID, threadID, timestamp, extraSizeInWords, args, numArgs);
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
	return WriteEventHeaderAndGenericData(writer, EventType::DurationBegin, category, name, processID, threadID, timestamp, extraSizeInWords, args, numArgs);
}

int AddDurationEndEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp) {
	return AddDurationEndEvent(writer, category, name, processID, threadID, timestamp, nullptr, 0);
}

int AddDurationEndEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, std::initializer_list<RecordArgument> args) {
	return AddDurationEndEvent(writer, category, name, processID, threadID, timestamp, args.begin(), args.size());
}

int AddDurationEndEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, const RecordArgument *args, size_t numArgs) {
	constexpr unsigned extraSizeInWords = 0;
	return WriteEventHeaderAndGenericData(writer, EventType::DurationEnd, category, name, processID, threadID, timestamp, extraSizeInWords, args, numArgs);
}

int AddDurationCompleteEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t beginTimestamp, uint64_t endTimestamp) {
	return AddDurationCompleteEvent(writer, category, name, processID, threadID, beginTimestamp, endTimestamp, nullptr, 0);
}

int AddDurationCompleteEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t beginTimestamp, uint64_t endTimestamp, std::initializer_list<RecordArgument> args) {
	return AddDurationCompleteEvent(writer, category, name, processID, threadID, beginTimestamp, endTimestamp, args.begin(), args.size());
}

int AddDurationCompleteEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t beginTimestamp, uint64_t endTimestamp, const RecordArgument *args, size_t numArgs) {
	constexpr unsigned extraSizeInWords = 1;
	int ret = WriteEventHeaderAndGenericData(writer, EventType::DurationComplete, category, name, processID, threadID, beginTimestamp, extraSizeInWords, args, numArgs);
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
	int ret = WriteEventHeaderAndGenericData(writer, EventType::AsyncBegin, category, name, processID, threadID, timestamp, extraSizeInWords, args, numArgs);
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
	int ret = WriteEventHeaderAndGenericData(writer, EventType::AsyncInstant, category, name, processID, threadID, timestamp, extraSizeInWords, args, numArgs);
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
	int ret = WriteEventHeaderAndGenericData(writer, EventType::AsyncEnd, category, name, processID, threadID, timestamp, extraSizeInWords, args, numArgs);
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
	int ret = WriteEventHeaderAndGenericData(writer, EventType::FlowBegin, category, name, processID, threadID, timestamp, extraSizeInWords, args, numArgs);
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
	int ret = WriteEventHeaderAndGenericData(writer, EventType::FlowStep, category, name, processID, threadID, timestamp, extraSizeInWords, args, numArgs);
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
	int ret = WriteEventHeaderAndGenericData(writer, EventType::FlowEnd, category, name, processID, threadID, timestamp, extraSizeInWords, args, numArgs);
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
	if (dataLen > BlobRecordFields::kMaxBlobLength) {
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
	const uint64_t header = BlobRecordFields::Type::Make(ToUnderlyingType(RecordType::Blob)) |
	                        BlobRecordFields::RecordSize::Make(sizeInWords) |
	                        BlobRecordFields::NameStringRef::Make(nameIndex) |
	                        BlobRecordFields::BlobSize::Make(dataLen) |
	                        BlobRecordFields::BlobType::Make(ToUnderlyingType(blobType));
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

	ProcessedRecordArgument processedArgs[FXT_MAX_NUM_ARGS];
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
	const uint64_t header = UserspaceObjectRecordFields::Type::Make(ToUnderlyingType(RecordType::UserspaceObject)) |
	                        UserspaceObjectRecordFields::RecordSize::Make(sizeInWords) |
	                        UserspaceObjectRecordFields::ThreadRef::Make(threadIndex) |
	                        UserspaceObjectRecordFields::NameStringRef::Make(nameIndex) |
	                        UserspaceObjectRecordFields::ArgumentCount::Make(numArgs);
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

	ProcessedRecordArgument processedArgs[FXT_MAX_NUM_ARGS];
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
	const uint64_t header = ContextSwitchRecordFields::Type::Make(ToUnderlyingType(RecordType::Scheduling)) |
	                        ContextSwitchRecordFields::RecordSize::Make(sizeInWords) |
	                        ContextSwitchRecordFields::ArgumentCount::Make(numArgs) |
	                        ContextSwitchRecordFields::CpuNumber::Make(cpuNumber) |
	                        ContextSwitchRecordFields::OutgoingThreadState::Make(outgoingThreadState) |
	                        ContextSwitchRecordFields::EventType::Make(ToUnderlyingType(SchedulingRecordType::ContextSwitch));
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

	ProcessedRecordArgument processedArgs[FXT_MAX_NUM_ARGS];
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
	const uint64_t header = FiberSwitchRecordFields::Type::Make(ToUnderlyingType(RecordType::Scheduling)) |
	                        FiberSwitchRecordFields::RecordSize::Make(sizeInWords) |
	                        FiberSwitchRecordFields::ArgumentCount::Make(numArgs) |
	                        FiberSwitchRecordFields::EventType::Make(ToUnderlyingType(SchedulingRecordType::FiberSwitch));
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

	ProcessedRecordArgument processedArgs[FXT_MAX_NUM_ARGS];
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
	const uint64_t header = ThreadWakeupRecordFields::Type::Make(ToUnderlyingType(RecordType::Scheduling)) |
	                        ThreadWakeupRecordFields::RecordSize::Make(sizeInWords) |
	                        ThreadWakeupRecordFields::ArgumentCount::Make(numArgs) |
	                        ThreadWakeupRecordFields::CpuNumber::Make(cpuNumber) |
	                        ThreadWakeupRecordFields::EventType::Make(ToUnderlyingType(SchedulingRecordType::ThreadWakeup));
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
