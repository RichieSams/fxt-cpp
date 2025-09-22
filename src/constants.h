/* FXT - A library for creating Fuschia Tracing System (FXT) files
 *
 * FXT is the legal property of Adrian Astley
 * Copyright Adrian Astley 2023
 */

#pragma once

#include <inttypes.h>

namespace fxt {

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
	TraceInfo = 4,
};

enum class TraceInfoType {
	MagicNumberRecord = 0,
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
	FiberSwitch = 3,
};

} // End of namespace fxt
