/* FXT - A library for creating Fuschia Tracing System (FXT) files
 *
 * FXT is the legal property of Adrian Astley
 * Copyright Adrian Astley 2023
 */

#pragma once

namespace fxt {

namespace internal {
template <typename... Ts>
using AllEventArguments = typename std::conjunction<std::is_base_of<EventArgument, std::remove_pointer_t<Ts>>...>::type;
template <typename... Ts>
using AllPointers = typename std::conjunction<std::is_pointer<Ts>...>::type;
} // End of namespace internal

template <typename... Arguments>
inline int Writer::AddInstantEvent(const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, Arguments... arguments) {
	static_assert(internal::AllEventArguments<Arguments...>(), "All arguments must derived from EventArgument");
	static_assert(internal::AllPointers<Arguments...>(), "All arguments must passed as pointers");

	const unsigned extraSizeInWords = 0;
	return WriteEventHeaderAndGenericData(internal::EventType::Instant, category, name, processID, threadID, timestamp, extraSizeInWords, arguments...);
}

template <typename... Arguments>
inline int Writer::AddCounterEvent(const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t counterID, Arguments... arguments) {
	static_assert(internal::AllEventArguments<Arguments...>(), "All arguments must derived from EventArgument");
	static_assert(internal::AllPointers<Arguments...>(), "All arguments must passed as pointers");

	const unsigned extraSizeInWords = 1;
	int ret = WriteEventHeaderAndGenericData(internal::EventType::Counter, category, name, processID, threadID, timestamp, extraSizeInWords, arguments...);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream(counterID);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

template <typename... Arguments>
inline int Writer::AddDurationBeginEvent(const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, Arguments... arguments) {
	static_assert(internal::AllEventArguments<Arguments...>(), "All arguments must derived from EventArgument");
	static_assert(internal::AllPointers<Arguments...>(), "All arguments must passed as pointers");

	const unsigned extraSizeInWords = 0;
	return WriteEventHeaderAndGenericData(internal::EventType::DurationBegin, category, name, processID, threadID, timestamp, extraSizeInWords, arguments...);
}

template <typename... Arguments>
inline int Writer::AddDurationEndEvent(const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, Arguments... arguments) {
	static_assert(internal::AllEventArguments<Arguments...>(), "All arguments must derived from EventArgument");
	static_assert(internal::AllPointers<Arguments...>(), "All arguments must passed as pointers");

	const unsigned extraSizeInWords = 0;
	return WriteEventHeaderAndGenericData(internal::EventType::DurationEnd, category, name, processID, threadID, timestamp, extraSizeInWords, arguments...);
}

template <typename... Arguments>
inline int Writer::AddDurationCompleteEvent(const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t beginTimestamp, uint64_t endTimestamp, Arguments... arguments) {
	static_assert(internal::AllEventArguments<Arguments...>(), "All arguments must derived from EventArgument");
	static_assert(internal::AllPointers<Arguments...>(), "All arguments must passed as pointers");

	const unsigned extraSizeInWords = 1;
	int ret = WriteEventHeaderAndGenericData(internal::EventType::DurationComplete, category, name, processID, threadID, beginTimestamp, extraSizeInWords, arguments...);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream(endTimestamp);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

template <typename... Arguments>
inline int Writer::AddAsyncBeginEvent(const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t asyncCorrelationID, Arguments... arguments) {
	static_assert(internal::AllEventArguments<Arguments...>(), "All arguments must derived from EventArgument");
	static_assert(internal::AllPointers<Arguments...>(), "All arguments must passed as pointers");

	const unsigned extraSizeInWords = 1;
	int ret = WriteEventHeaderAndGenericData(internal::EventType::AsyncBegin, category, name, processID, threadID, timestamp, extraSizeInWords, arguments...);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream(asyncCorrelationID);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

template <typename... Arguments>
inline int Writer::AddAsyncInstantEvent(const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t asyncCorrelationID, Arguments... arguments) {
	static_assert(internal::AllEventArguments<Arguments...>(), "All arguments must derived from EventArgument");
	static_assert(internal::AllPointers<Arguments...>(), "All arguments must passed as pointers");

	const unsigned extraSizeInWords = 1;
	int ret = WriteEventHeaderAndGenericData(internal::EventType::AsyncInstant, category, name, processID, threadID, timestamp, extraSizeInWords, arguments...);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream(asyncCorrelationID);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

template <typename... Arguments>
inline int Writer::AddAsyncEndEvent(const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t asyncCorrelationID, Arguments... arguments) {
	static_assert(internal::AllEventArguments<Arguments...>(), "All arguments must derived from EventArgument");
	static_assert(internal::AllPointers<Arguments...>(), "All arguments must passed as pointers");

	const unsigned extraSizeInWords = 1;
	int ret = WriteEventHeaderAndGenericData(internal::EventType::AsyncEnd, category, name, processID, threadID, timestamp, extraSizeInWords, arguments...);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream(asyncCorrelationID);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

template <typename... Arguments>
inline int Writer::AddFlowBeginEvent(const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t flowCorrelationID, Arguments... arguments) {
	static_assert(internal::AllEventArguments<Arguments...>(), "All arguments must derived from EventArgument");
	static_assert(internal::AllPointers<Arguments...>(), "All arguments must passed as pointers");

	const unsigned extraSizeInWords = 1;
	int ret = WriteEventHeaderAndGenericData(internal::EventType::FlowBegin, category, name, processID, threadID, timestamp, extraSizeInWords, arguments...);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream(flowCorrelationID);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

template <typename... Arguments>
inline int Writer::AddFlowStepEvent(const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t flowCorrelationID, Arguments... arguments) {
	static_assert(internal::AllEventArguments<Arguments...>(), "All arguments must derived from EventArgument");
	static_assert(internal::AllPointers<Arguments...>(), "All arguments must passed as pointers");

	const unsigned extraSizeInWords = 1;
	int ret = WriteEventHeaderAndGenericData(internal::EventType::FlowStep, category, name, processID, threadID, timestamp, extraSizeInWords, arguments...);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream(flowCorrelationID);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

template <typename... Arguments>
inline int Writer::AddFlowEndEvent(const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t flowCorrelationID, Arguments... arguments) {
	static_assert(internal::AllEventArguments<Arguments...>(), "All arguments must derived from EventArgument");
	static_assert(internal::AllPointers<Arguments...>(), "All arguments must passed as pointers");

	const unsigned extraSizeInWords = 1;
	int ret = WriteEventHeaderAndGenericData(internal::EventType::FlowEnd, category, name, processID, threadID, timestamp, extraSizeInWords, arguments...);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream(flowCorrelationID);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

inline int Writer::AddUserspaceObjectRecord(const char *name, KernelObjectID processID, KernelObjectID threadID, uintptr_t pointerValue) {
	uint16_t nameIndex;
	int ret = GetOrCreateStringIndex(name, &nameIndex);
	if (ret != 0) {
		return ret;
	}

	uint16_t threadIndex;
	ret = GetOrCreateThreadIndex(processID, threadID, &threadIndex);
	if (ret != 0) {
		return ret;
	}

	const uint64_t sizeInWords = /* Header */ 1 + /* pointer value */ 1 + /* argument data */ 0;
	constexpr size_t numArgs = 0;
	const uint64_t header = ((uint64_t)(numArgs) << 40) | (uint64_t(nameIndex) << 24) | (uint64_t(threadIndex) << 16) | (uint64_t(sizeInWords) << 4) | uint64_t(internal::RecordType::UserspaceObject);
	ret = WriteUInt64ToStream(header);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream((uint64_t)pointerValue);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

template <typename... Arguments>
inline int Writer::AddUserspaceObjectRecord(const char *name, KernelObjectID processID, KernelObjectID threadID, uintptr_t pointerValue, Arguments... arguments) {
	static_assert(internal::AllEventArguments<Arguments...>(), "All arguments must derived from EventArgument");
	static_assert(internal::AllEventArguments<Arguments...>(), "All arguments must passed as pointers");

	uint16_t nameIndex;
	int ret = GetOrCreateStringIndex(name, &nameIndex);
	if (ret != 0) {
		return ret;
	}

	uint16_t threadIndex;
	ret = GetOrCreateThreadIndex(processID, threadID, &threadIndex);
	if (ret != 0) {
		return ret;
	}

	// Add up the argument word size
	// And ensure the argument keys (and string values) are in the string table
	unsigned argumentSizeInWords = 0;
	for (EventArgument *arg : std::initializer_list<EventArgument *>{ arguments... }) {
		argumentSizeInWords += arg->ArgumentSizeInWords();

		ret = arg->InitStringEntries(this);
		if (ret != 0) {
			return ret;
		}
	}

	const uint64_t sizeInWords = /* Header */ 1 + /* pointer value */ 1 + /* argument data */ argumentSizeInWords;
	constexpr size_t numArgs = sizeof...(Arguments);
	const uint64_t header = ((uint64_t)(numArgs) << 40) | (uint64_t(nameIndex) << 24) | (uint64_t(threadIndex) << 16) | (uint64_t(sizeInWords) << 4) | uint64_t(internal::RecordType::UserspaceObject);
	ret = WriteUInt64ToStream(header);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream((uint64_t)pointerValue);
	if (ret != 0) {
		return ret;
	}

	unsigned wordsWritten = 0;
	for (EventArgument *arg : std::initializer_list<EventArgument *>{ arguments... }) {
		unsigned size;
		ret = arg->WriteArgumentDataToStream(this, &size);
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

inline int Writer::AddContextSwitchRecord(uint16_t cpuNumber, uint8_t outgoingThreadState, KernelObjectID outgoingThreadID, KernelObjectID incomingThreadID, uint64_t timestamp) {
	// Sanity check
	// Ideally we'd find out the actual ENUM of valid states
	if (outgoingThreadState > 0xF) {
		return FXT_ERR_INVALID_OUTGOING_THREAD_STATE;
	}

	const uint64_t sizeInWords = /* Header */ 1 + /* timestamp */ 1 + /* outgoing thread ID */ 1 + /* incoming thread ID */ 1 + /* argument data */ 0;
	constexpr size_t numArgs = 0;
	const uint64_t header = ((uint64_t)(internal::SchedulingRecordType::ContextSwitch) << 60) | ((uint64_t)(outgoingThreadState) << 36) | ((uint64_t)(cpuNumber) << 20) | ((uint64_t)(numArgs) << 16) | ((uint64_t)(sizeInWords) << 4) | (uint64_t)(internal::RecordType::Scheduling);
	int ret = WriteUInt64ToStream(header);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream(timestamp);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream((uint64_t)outgoingThreadID);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream((uint64_t)incomingThreadID);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

template <typename... Arguments>
inline int Writer::AddContextSwitchRecord(uint16_t cpuNumber, uint8_t outgoingThreadState, KernelObjectID outgoingThreadID, KernelObjectID incomingThreadID, uint64_t timestamp, Arguments... arguments) {
	static_assert(internal::AllEventArguments<Arguments...>(), "All arguments must derived from EventArgument");
	static_assert(internal::AllEventArguments<Arguments...>(), "All arguments must passed as pointers");

	// Sanity check
	// Ideally we'd find out the actual ENUM of valid states
	if (outgoingThreadState > 0xF) {
		return FXT_ERR_INVALID_OUTGOING_THREAD_STATE;
	}

	// Add up the argument word size
	// And ensure the argument keys (and string values) are in the string table
	unsigned argumentSizeInWords = 0;
	for (EventArgument *arg : std::initializer_list<EventArgument *>{ arguments... }) {
		argumentSizeInWords += arg->ArgumentSizeInWords();

		int ret = arg->InitStringEntries(this);
		if (ret != 0) {
			return ret;
		}
	}

	const uint64_t sizeInWords = /* Header */ 1 + /* timestamp */ 1 + /* outgoing thread ID */ 1 + /* incoming thread ID */ 1 + /* argument data */ argumentSizeInWords;
	constexpr size_t numArgs = sizeof...(Arguments);
	const uint64_t header = ((uint64_t)(internal::SchedulingRecordType::ContextSwitch) << 60) | ((uint64_t)(outgoingThreadState) << 36) | ((uint64_t)(cpuNumber) << 20) | ((uint64_t)(numArgs) << 16) | ((uint64_t)(sizeInWords) << 4) | (uint64_t)(internal::RecordType::Scheduling);
	int ret = WriteUInt64ToStream(header);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream(timestamp);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream((uint64_t)outgoingThreadID);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream((uint64_t)incomingThreadID);
	if (ret != 0) {
		return ret;
	}

	unsigned wordsWritten = 0;
	for (EventArgument *arg : std::initializer_list<EventArgument *>{ arguments... }) {
		unsigned size;
		ret = arg->WriteArgumentDataToStream(this, &size);
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

inline int Writer::AddThreadWakeupRecord(uint16_t cpuNumber, KernelObjectID wakingThreadID, uint64_t timestamp) {
	const uint64_t sizeInWords = /* Header */ 1 + /* timestamp */ 1 + /* waking thread ID */ 1 + /* argument data */ 0;
	constexpr size_t numArgs = 0;
	const uint64_t header = ((uint64_t)(internal::SchedulingRecordType::ThreadWakeup) << 60) | ((uint64_t)(cpuNumber) << 20) | ((uint64_t)(numArgs) << 16) | ((uint64_t)(sizeInWords) << 4) | (uint64_t)(internal::RecordType::Scheduling);
	int ret = WriteUInt64ToStream(header);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream(timestamp);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream(wakingThreadID);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

template <typename... Arguments>
inline int Writer::AddThreadWakeupRecord(uint16_t cpuNumber, KernelObjectID wakingThreadID, uint64_t timestamp, Arguments... arguments) {
	static_assert(internal::AllEventArguments<Arguments...>(), "All arguments must derived from EventArgument");
	static_assert(internal::AllEventArguments<Arguments...>(), "All arguments must passed as pointers");

	// Add up the argument word size
	// And ensure the argument keys (and string values) are in the string table
	unsigned argumentSizeInWords = 0;
	for (EventArgument *arg : std::initializer_list<EventArgument *>{ arguments... }) {
		argumentSizeInWords += arg->ArgumentSizeInWords();

		int ret = arg->InitStringEntries(this);
		if (ret != 0) {
			return ret;
		}
	}

	const uint64_t sizeInWords = /* Header */ 1 + /* timestamp */ 1 + /* waking thread ID */ 1 + /* argument data */ argumentSizeInWords;
	constexpr size_t numArgs = sizeof...(Arguments);
	const uint64_t header = ((uint64_t)(internal::SchedulingRecordType::ThreadWakeup) << 60) | ((uint64_t)(cpuNumber) << 20) | ((uint64_t)(numArgs) << 16) | ((uint64_t)(sizeInWords) << 4) | (uint64_t)(internal::RecordType::Scheduling);
	int ret = WriteUInt64ToStream(header);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream(timestamp);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream(wakingThreadID);
	if (ret != 0) {
		return ret;
	}

	unsigned wordsWritten = 0;
	for (EventArgument *arg : std::initializer_list<EventArgument *>{ arguments... }) {
		unsigned size;
		ret = arg->WriteArgumentDataToStream(this, &size);
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

inline int Writer::WriteEventHeaderAndGenericData(internal::EventType eventType, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, unsigned extraSizeInWords) {
	uint16_t categoryIndex;
	int ret = GetOrCreateStringIndex(category, &categoryIndex);
	if (ret != 0) {
		return ret;
	}

	uint16_t nameIndex;
	ret = GetOrCreateStringIndex(name, &nameIndex);
	if (ret != 0) {
		return ret;
	}

	uint16_t threadIndex;
	ret = GetOrCreateThreadIndex(processID, threadID, &threadIndex);
	if (ret != 0) {
		return ret;
	}

	const uint64_t sizeInWords = /* Header */ 1 + /* timestamp */ 1 + /* argument data */ 0 + /* extra stuff */ extraSizeInWords;
	constexpr size_t numArgs = 0;
	const uint64_t header = ((uint64_t)(nameIndex) << 48) | ((uint64_t)(categoryIndex) << 32) | ((uint64_t)(threadIndex) << 24) | ((uint64_t)(numArgs) << 20) | ((uint64_t)(eventType) << 16) | ((uint64_t)(sizeInWords) << 4) | (uint64_t)(internal::RecordType::Event);
	ret = WriteUInt64ToStream(header);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream(timestamp);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

template <typename... Arguments>
inline int Writer::WriteEventHeaderAndGenericData(internal::EventType eventType, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, unsigned extraSizeInWords, Arguments... arguments) {
	static_assert(internal::AllEventArguments<Arguments...>(), "All arguments must derived from EventArgument");
	static_assert(internal::AllEventArguments<Arguments...>(), "All arguments must passed as pointers");

	uint16_t categoryIndex;
	int ret = GetOrCreateStringIndex(category, &categoryIndex);
	if (ret != 0) {
		return ret;
	}

	uint16_t nameIndex;
	ret = GetOrCreateStringIndex(name, &nameIndex);
	if (ret != 0) {
		return ret;
	}

	uint16_t threadIndex;
	ret = GetOrCreateThreadIndex(processID, threadID, &threadIndex);
	if (ret != 0) {
		return ret;
	}

	// Add up the argument word size
	// And ensure the argument keys (and string values) are in the string table
	unsigned argumentSizeInWords = 0;
	for (EventArgument *arg : std::initializer_list<EventArgument *>{ arguments... }) {
		argumentSizeInWords += arg->ArgumentSizeInWords();

		ret = arg->InitStringEntries(this);
		if (ret != 0) {
			return ret;
		}
	}

	const uint64_t sizeInWords = /* Header */ 1 + /* timestamp */ 1 + /* argument data */ argumentSizeInWords + /* extra stuff */ extraSizeInWords;
	constexpr size_t numArgs = sizeof...(Arguments);
	const uint64_t header = ((uint64_t)(nameIndex) << 48) | ((uint64_t)(categoryIndex) << 32) | ((uint64_t)(threadIndex) << 24) | ((uint64_t)(numArgs) << 20) | ((uint64_t)(eventType) << 16) | ((uint64_t)(sizeInWords) << 4) | (uint64_t)(internal::RecordType::Event);
	ret = WriteUInt64ToStream(header);
	if (ret != 0) {
		return ret;
	}

	ret = WriteUInt64ToStream(timestamp);
	if (ret != 0) {
		return ret;
	}

	unsigned wordsWritten = 0;
	for (EventArgument *arg : std::initializer_list<EventArgument *>{ arguments... }) {
		unsigned size;
		ret = arg->WriteArgumentDataToStream(this, &size);
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

} // End of namespace fxt