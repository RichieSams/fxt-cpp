/* FXT - A library for creating Fuschia Tracing System (FXT) files
 *
 * FXT is the legal property of Adrian Astley
 * Copyright Adrian Astley 2023
 */

#pragma once

#include <inttypes.h>

namespace fxt {

enum class BlobType {
	Data = 1,
	LastBranch = 2,
	Perfetto = 3,
};

enum class ProviderEventType {
	BufferFilledUp = 0,
};

namespace internal {

enum class RecordType {
	Metadata = 0,
	Initialization = 1,
	String = 2,
	Thread = 3,
	Event = 4,
	Blob = 5,
	UserspaceObject = 6,
	KernelObject = 7,
	Scheduling = 8,
	Log = 9,
	LargeBlob = 15,
};

enum class MetadataType {
	ProviderInfo = 1,
	ProviderSection = 2,
	ProviderEvent = 3,
};

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

enum class EventType {
	Instant = 0,
	Counter = 1,
	DurationBegin = 2,
	DurationEnd = 3,
	DurationComplete = 4,
	AsyncBegin = 5,
	AsyncInstant = 6,
	AsyncEnd = 7,
	FlowBegin = 8,
	FlowStep = 9,
	FlowEnd = 10,
};

enum class KOIDType {
	Process = 1,
	Thread = 2,
};

enum class SchedulingRecordType {
	ContextSwitch = 1,
	ThreadWakeup = 2,
};

} // End of namespace internal

} // End of namespace fxt
