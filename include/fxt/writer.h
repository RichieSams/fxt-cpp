/* FXT - A library for creating Fuschia Tracing System (FXT) files
 *
 * FXT is the legal property of Adrian Astley
 * Copyright Adrian Astley 2023
 */

#pragma once

#include "fxt/err.h"
#include "fxt/internal/constants.h"
#include "fxt/internal/defines.h"
#include "fxt/internal/fields.h"
#include "fxt/record_args.h"

#include <stddef.h>
#include <stdint.h>

#include <initializer_list>
#include <type_traits>

namespace fxt {

/**
 * @brief A user-defined function for how FXT stream data should be written
 *
 * @param userContext    The userContext value passed to the Writer constructor
 * @param data           The data to write
 * @param len            The length of the data array
 */
typedef int (*WriteFunc)(void *userContext, const void *data, size_t len);

struct Writer {
	Writer(void *userContext, WriteFunc writeFunc)
	        : userContext(userContext),
	          writeFunc(writeFunc) {
	}
	~Writer() = default;

	/**
	 * @brief A hash lookup array for strings.
	 *
	 * FXT allows you to create String records. In subsequent records, users can then just reference the String record
	 * index (a uint16) instead of having to include the raw string each and every time. This is faster and requires
	 * less data.
	 *
	 * FXT allows (0xFFFF - 1) string indices. However, tracking that many here in the writer would use a large amount
	 * of memory. The FXT stream is *stateful*. A user can re-use the same string index in a new String record. That new
	 * string will be applied to any records subsequently. (Until a new String record with the same index replaces it).
	 *
	 * We exploit this fact to limit memory usage. We have a fixed buffer of 512 entries, which is a compromise between
	 * getting good String re-use and memory usage.
	 *
	 * We don't actually store the strings. Instead we just store their hash
	 *
	 * @see Writer::AddStringRecord
	 * @see Writer::GetOrCreateStringIndex
	 */
	uint64_t stringTable[512];
	/**
	 * @brief A hash lookup array for threads.
	 *
	 * FXT allows you to create Thread records. In subsequent records, users can then just reference the Thread record
	 * index (a uint16) instead of having to include the raw process ID and thread ID each and every time. This is faster
	 * and requires less data.
	 *
	 * FXT allows (0xFFFF - 1) thread indices. However, tracking that many here in the writer would use a large amount
	 * of memory. The FXT stream is *stateful*. A user can re-use the same thread index in a new Thread record. That new
	 * process ID / thread ID will be applied to any records subsequently. (Until a new Thread record with the same index
	 * replaces it).
	 *
	 * We exploit this fact to limit memory usage. We have a fixed buffer of 128 entries, which is a compromise between
	 * getting good Thread re-use and memory usage.
	 *
	 * We don't actually store the strings. Instead we just store their hash
	 *
	 * @see Writer::AddThreadRecord
	 * @see Writer::GetOrCreateThreadIndex
	 */
	uint64_t threadTable[128];
	uint16_t nextStringIndex = 0;
	uint16_t nextThreadIndex = 0;

	void *userContext;
	WriteFunc writeFunc;
};

/**
 * @brief Adds a Magic Number record to the stream
 *
 * @param writer    The writer to use
 * @return          0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#magic-number-record
 */
int WriteMagicNumberRecord(Writer *writer);

/**
 * @brief Adds a provider info metadata record to the stream.
 *
 * @param writer          The writer to use
 * @param providerID      The provider ID
 * @param providerName    The human-readable name for the provider
 * @return                0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#provider-info-metadata
 */
int AddProviderInfoRecord(Writer *writer, ProviderID providerID, const char *providerName);

/**
 * @brief Adds a provider section metadata record to the stream.
 *
 * @param writer        The writer to use
 * @param providerID    The provider ID
 * @return              0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#provider-section-metadata
 */
int AddProviderSectionRecord(Writer *writer, ProviderID providerID);

/**
 * @brief Adds a provider event metadata record to the stream.
 *
 * @param writer        The writer to use
 * @param providerID    The provider ID
 * @param eventType     The event type
 * @return              0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#provider-event-metadata
 */
int AddProviderEventRecord(Writer *writer, ProviderID providerID, ProviderEventType eventType);

/**
 * @brief Adds an initialization record to the stream.
 *
 * This specifies the number of ticks per second for all event records after this one.
 * If you need to change the tick rate, you can add another initialization record to
 * the stream, and then send more event records.
 *
 * @param writer               The writer to use
 * @param numTicksPerSecond    The number of ticks per second for all events after this record
 * @return                     0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#initialization-record
 */
int AddInitializationRecord(Writer *writer, uint64_t numTicksPerSecond);

/**
 * @brief Adds a kernel object record to give a human-readable name to a process ID.
 *
 * @param writer       The writer to use
 * @param processID    The process ID
 * @param name         The human-readable name to give the process
 * @return             0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#kernel-object-record
 */
int SetProcessName(Writer *writer, KernelObjectID processID, const char *name);

/**
 * @brief Adds a kernel object record to give a human-readable name to a thread ID.
 *
 * @param writer       The writer to use
 * @param processID    The process ID
 * @param threadID     The thread ID
 * @param name         The human-readable name to give the thread
 * @return             0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#kernel-object-record
 */
int SetThreadName(Writer *writer, KernelObjectID processID, KernelObjectID threadID, const char *name);

/**
 * @brief Adds an instant event record to the stream.
 *
 * @param writer       The writer to use
 * @param category     The category of the event
 * @param name         The name of the event
 * @param processID    The process ID
 * @param threadID     The thread ID
 * @param timestamp    The timestamp of the event
 * @return             0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#instant-event
 */
int AddInstantEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp);
/**
 * @brief Adds an instant event record to the stream.
 *
 * @param writer       The writer to use
 * @param category     The category of the event
 * @param name         The name of the event
 * @param processID    The process ID
 * @param threadID     The thread ID
 * @param timestamp    The timestamp of the event
 * @param args         Arguments to add to the event
 * @param numArgs      The number of arguments
 * @return             0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#instant-event
 */
int AddInstantEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, std::initializer_list<RecordArgument> args);
/**
 * @brief Adds an instant event record to the stream.
 *
 * @param writer       The writer to use
 * @param category     The category of the event
 * @param name         The name of the event
 * @param processID    The process ID
 * @param threadID     The thread ID
 * @param timestamp    The timestamp of the event
 * @param args         Arguments to add to the event
 * @param numArgs      The number of arguments
 * @return             0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#instant-event
 */
int AddInstantEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, const RecordArgument *args, size_t numArgs);

/**
 * @brief Adds a counter event record to the stream.
 *
 * @param writer       The writer to use
 * @param category     The category of the event
 * @param name         The name of the event
 * @param processID    The process ID
 * @param threadID     The thread ID
 * @param timestamp    The timestamp of the event
 * @param counterID    The counter ID
 * @return             0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#counter-event
 */
int AddCounterEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t counterID);
/**
 * @brief Adds a counter event record to the stream.
 *
 * @param writer       The writer to use
 * @param category     The category of the event
 * @param name         The name of the event
 * @param processID    The process ID
 * @param threadID     The thread ID
 * @param timestamp    The timestamp of the event
 * @param counterID    The counter ID
 * @param args         Arguments to add to the event
 * @return             0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#counter-event
 */
int AddCounterEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t counterID, std::initializer_list<RecordArgument> args);
/**
 * @brief Adds a counter event record to the stream.
 *
 * @param writer       The writer to use
 * @param category     The category of the event
 * @param name         The name of the event
 * @param processID    The process ID
 * @param threadID     The thread ID
 * @param timestamp    The timestamp of the event
 * @param counterID    The counter ID
 * @param args         Arguments to add to the event
 * @param numArgs      The number of arguments
 * @return             0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#counter-event
 */
int AddCounterEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t counterID, const RecordArgument *args, size_t numArgs);

/**
 * @brief Adds a duration begin event record to the stream.
 *
 * @param writer       The writer to use
 * @param category     The category of the event
 * @param name         The name of the event
 * @param processID    The process ID
 * @param threadID     The thread ID
 * @param timestamp    The timestamp of the event
 * @return             0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#duration-begin-event
 */
int AddDurationBeginEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp);
/**
 * @brief Adds a duration begin event record to the stream.
 *
 * @param writer       The writer to use
 * @param category     The category of the event
 * @param name         The name of the event
 * @param processID    The process ID
 * @param threadID     The thread ID
 * @param timestamp    The timestamp of the event
 * @param args         Arguments to add to the event
 * @return             0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#duration-begin-event
 */
int AddDurationBeginEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, std::initializer_list<RecordArgument> args);
/**
 * @brief Adds a duration begin event record to the stream.
 *
 * @param writer       The writer to use
 * @param category     The category of the event
 * @param name         The name of the event
 * @param processID    The process ID
 * @param threadID     The thread ID
 * @param timestamp    The timestamp of the event
 * @param args         Arguments to add to the event
 * @param numArgs      The number of arguments
 * @return             0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#duration-begin-event
 */
int AddDurationBeginEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, const RecordArgument *args, size_t numArgs);

/**
 * @brief Adds a duration end event record to the stream.
 *
 * @param writer       The writer to use
 * @param category     The category of the event
 * @param name         The name of the event
 * @param processID    The process ID
 * @param threadID     The thread ID
 * @param timestamp    The timestamp of the event
 * @return             0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#duration-end-event
 */
int AddDurationEndEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp);
/**
 * @brief Adds a duration end event record to the stream.
 *
 * @param writer       The writer to use
 * @param category     The category of the event
 * @param name         The name of the event
 * @param processID    The process ID
 * @param threadID     The thread ID
 * @param timestamp    The timestamp of the event
 * @param args         Arguments to add to the event
 * @return             0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#duration-end-event
 */
int AddDurationEndEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, std::initializer_list<RecordArgument> args);
/**
 * @brief Adds a duration end event record to the stream.
 *
 * @param writer       The writer to use
 * @param category     The category of the event
 * @param name         The name of the event
 * @param processID    The process ID
 * @param threadID     The thread ID
 * @param timestamp    The timestamp of the event
 * @param args         Arguments to add to the event
 * @param numArgs      The number of arguments
 * @return             0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#duration-end-event
 */
int AddDurationEndEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, const RecordArgument *args, size_t numArgs);

/**
 * @brief Adds a duration complete event record to the stream.
 *
 * @param writer       The writer to use
 * @param category     The category of the event
 * @param name         The name of the event
 * @param processID    The process ID
 * @param threadID     The thread ID
 * @param timestamp    The timestamp of the event
 * @return             0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#duration-complete-event
 */
int AddDurationCompleteEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t beginTimestamp, uint64_t endTimestamp);
/**
 * @brief Adds a duration complete event record to the stream.
 *
 * @param writer       The writer to use
 * @param category     The category of the event
 * @param name         The name of the event
 * @param processID    The process ID
 * @param threadID     The thread ID
 * @param timestamp    The timestamp of the event
 * @param args         Arguments to add to the event
 * @return             0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#duration-complete-event
 */
int AddDurationCompleteEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t beginTimestamp, uint64_t endTimestamp, std::initializer_list<RecordArgument> args);
/**
 * @brief Adds a duration complete event record to the stream.
 *
 * @param writer       The writer to use
 * @param category     The category of the event
 * @param name         The name of the event
 * @param processID    The process ID
 * @param threadID     The thread ID
 * @param timestamp    The timestamp of the event
 * @param args         Arguments to add to the event
 * @param numArgs      The number of arguments
 * @return             0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#duration-complete-event
 */
int AddDurationCompleteEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t beginTimestamp, uint64_t endTimestamp, const RecordArgument *args, size_t numArgs);

/**
 * @brief Adds an async begin event record to the stream.
 *
 * @param writer                The writer to use
 * @param category              The category of the event
 * @param name                  The name of the event
 * @param processID             The process ID
 * @param threadID              The thread ID
 * @param timestamp             The timestamp of the event
 * @param asyncCorrelationID    The async correlation ID
 * @return                      0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#async-begin-event
 */
int AddAsyncBeginEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t asyncCorrelationID);
/**
 * @brief Adds an async begin event record to the stream.
 *
 * @param writer                The writer to use
 * @param category              The category of the event
 * @param name                  The name of the event
 * @param processID             The process ID
 * @param threadID              The thread ID
 * @param timestamp             The timestamp of the event
 * @param asyncCorrelationID    The async correlation ID
 * @param args                  Arguments to add to the event
 * @return                      0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#async-begin-event
 */
int AddAsyncBeginEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t asyncCorrelationID, std::initializer_list<RecordArgument> args);
/**
 * @brief Adds an async begin event record to the stream.
 *
 * @param writer                The writer to use
 * @param category              The category of the event
 * @param name                  The name of the event
 * @param processID             The process ID
 * @param threadID              The thread ID
 * @param timestamp             The timestamp of the event
 * @param asyncCorrelationID    The async correlation ID
 * @param args                  Arguments to add to the event
 * @param numArgs               The number of arguments
 * @return                      0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#async-begin-event
 */
int AddAsyncBeginEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t asyncCorrelationID, const RecordArgument *args, size_t numArgs);

/**
 * @brief Adds an async instant event record to the stream.
 *
 * @param writer                The writer to use
 * @param category              The category of the event
 * @param name                  The name of the event
 * @param processID             The process ID
 * @param threadID              The thread ID
 * @param timestamp             The timestamp of the event
 * @param asyncCorrelationID    The async correlation ID
 * @return                      0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#async-instant-event
 */
int AddAsyncInstantEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t asyncCorrelationID);
/**
 * @brief Adds an async instant event record to the stream.
 *
 * @param writer                The writer to use
 * @param category              The category of the event
 * @param name                  The name of the event
 * @param processID             The process ID
 * @param threadID              The thread ID
 * @param timestamp             The timestamp of the event
 * @param asyncCorrelationID    The async correlation ID
 * @param args                  Arguments to add to the event
 * @param numArgs               The number of arguments
 * @return                      0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#async-instant-event
 */
int AddAsyncInstantEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t asyncCorrelationID, std::initializer_list<RecordArgument> args);
/**
 * @brief Adds an async instant event record to the stream.
 *
 * @param writer                The writer to use
 * @param category              The category of the event
 * @param name                  The name of the event
 * @param processID             The process ID
 * @param threadID              The thread ID
 * @param timestamp             The timestamp of the event
 * @param asyncCorrelationID    The async correlation ID
 * @param args                  Arguments to add to the event
 * @param numArgs               The number of arguments
 * @return                      0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#async-instant-event
 */
int AddAsyncInstantEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t asyncCorrelationID, const RecordArgument *args, size_t numArgs);

/**
 * @brief Adds an async end event record to the stream.
 *
 * @param writer                The writer to use
 * @param category              The category of the event
 * @param name                  The name of the event
 * @param processID             The process ID
 * @param threadID              The thread ID
 * @param timestamp             The timestamp of the event
 * @param asyncCorrelationID    The async correlation ID
 * @return                      0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#async-end-event
 */
int AddAsyncEndEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t asyncCorrelationID);
/**
 * @brief Adds an async end event record to the stream.
 *
 * @param writer                The writer to use
 * @param category              The category of the event
 * @param name                  The name of the event
 * @param processID             The process ID
 * @param threadID              The thread ID
 * @param timestamp             The timestamp of the event
 * @param asyncCorrelationID    The async correlation ID
 * @param args                  Arguments to add to the event
 * @return                      0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#async-end-event
 */
int AddAsyncEndEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t asyncCorrelationID, std::initializer_list<RecordArgument> args);
/**
 * @brief Adds an async end event record to the stream.
 *
 * @param writer                The writer to use
 * @param category              The category of the event
 * @param name                  The name of the event
 * @param processID             The process ID
 * @param threadID              The thread ID
 * @param timestamp             The timestamp of the event
 * @param asyncCorrelationID    The async correlation ID
 * @param args                  Arguments to add to the event
 * @param numArgs               The number of arguments
 * @return                      0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#async-end-event
 */
int AddAsyncEndEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t asyncCorrelationID, const RecordArgument *args, size_t numArgs);

/**
 * @brief Adds an flow begin event record to the stream.
 *
 * @param writer               The writer to use
 * @param category             The category of the event
 * @param name                 The name of the event
 * @param processID            The process ID
 * @param threadID             The thread ID
 * @param timestamp            The timestamp of the event
 * @param flowCorrelationID    The flow correlation ID
 * @return                     0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#flow-begin-event
 */
int AddFlowBeginEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t flowCorrelationID);
/**
 * @brief Adds an flow begin event record to the stream.
 *
 * @param writer               The writer to use
 * @param category             The category of the event
 * @param name                 The name of the event
 * @param processID            The process ID
 * @param threadID             The thread ID
 * @param timestamp            The timestamp of the event
 * @param flowCorrelationID    The flow correlation ID
 * @param args                 Arguments to add to the event
 * @return                     0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#flow-begin-event
 */
int AddFlowBeginEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t flowCorrelationID, std::initializer_list<RecordArgument> args);
/**
 * @brief Adds an flow begin event record to the stream.
 *
 * @param writer               The writer to use
 * @param category             The category of the event
 * @param name                 The name of the event
 * @param processID            The process ID
 * @param threadID             The thread ID
 * @param timestamp            The timestamp of the event
 * @param flowCorrelationID    The flow correlation ID
 * @param args                 Arguments to add to the event
 * @param numArgs              The number of arguments
 * @return                     0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#flow-begin-event
 */
int AddFlowBeginEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t flowCorrelationID, const RecordArgument *args, size_t numArgs);

/**
 * @brief Adds an flow step event record to the stream.
 *
 * @param writer               The writer to use * @param numArgs              The number of arguments

 * @param category             The category of the event
 * @param name                 The name of the event
 * @param processID            The process ID
 * @param threadID             The thread ID
 * @param timestamp            The timestamp of the event
 * @param flowCorrelationID    The flow correlation ID
 * @return                     0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#flow-instant-event
 */
int AddFlowStepEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t flowCorrelationID);
/**
 * @brief Adds an flow step event record to the stream.
 *
 * @param writer               The writer to use
 * @param category             The category of the event
 * @param name                 The name of the event
 * @param processID            The process ID
 * @param threadID             The thread ID
 * @param timestamp            The timestamp of the event
 * @param flowCorrelationID    The flow correlation ID
 * @param args                 Arguments to add to the event
 * @return                     0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#flow-instant-event
 */
int AddFlowStepEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t flowCorrelationID, std::initializer_list<RecordArgument> args);
/**
 * @brief Adds an flow step event record to the stream.
 *
 * @param writer               The writer to use
 * @param category             The category of the event
 * @param name                 The name of the event
 * @param processID            The process ID
 * @param threadID             The thread ID
 * @param timestamp            The timestamp of the event
 * @param flowCorrelationID    The flow correlation ID
 * @param args                 Arguments to add to the event
 * @param numArgs              The number of arguments
 * @return                     0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#flow-instant-event
 */
int AddFlowStepEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t flowCorrelationID, const RecordArgument *args, size_t numArgs);

/**
 * @brief Adds an flow end event record to the stream.
 *
 * @param writer               The writer to use
 * @param category             The category of the event
 * @param name                 The name of the event
 * @param processID            The process ID
 * @param threadID             The thread ID
 * @param timestamp            The timestamp of the event
 * @param flowCorrelationID    The flow correlation ID
 * @return                     0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#flow-end-event
 */
int AddFlowEndEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t flowCorrelationID);
/**
 * @brief Adds an flow end event record to the stream.
 *
 * @param writer               The writer to use
 * @param category             The category of the event
 * @param name                 The name of the event
 * @param processID            The process ID
 * @param threadID             The thread ID
 * @param timestamp            The timestamp of the event
 * @param flowCorrelationID    The flow correlation ID
 * @param args                 Arguments to add to the event
 * @return                     0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#flow-end-event
 */
int AddFlowEndEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t flowCorrelationID, std::initializer_list<RecordArgument> args);
/**
 * @brief Adds an flow end event record to the stream.
 *
 * @param writer               The writer to use
 * @param category             The category of the event
 * @param name                 The name of the event
 * @param processID            The process ID
 * @param threadID             The thread ID
 * @param timestamp            The timestamp of the event
 * @param flowCorrelationID    The flow correlation ID
 * @param args                 Arguments to add to the event
 * @param numArgs              The number of arguments
 * @return                     0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#flow-end-event
 */
int AddFlowEndEvent(Writer *writer, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t flowCorrelationID, const RecordArgument *args, size_t numArgs);

/**
 * @brief Adds a blob record to the stream.
 *
 * @param writer      The writer to use
 * @param name        The name of the blob
 * @param data        A pointer to the blob data
 * @param dataLen     The length of the data
 * @param blobType    The type of the blob
 * @return            0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#blob-record
 */
int AddBlobRecord(Writer *writer, const char *name, void *data, size_t dataLen, BlobType blobType);

/**
 * @brief Adds a userspace object record to the stream with accompanying argument data.
 *
 * @param writer          The writer to use
 * @param name            The name of the userspace object
 * @param processID       The process ID
 * @param pointerValue    The pointer value
 * @return                0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#userspace-object-record
 */
int AddUserspaceObjectRecord(Writer *writer, const char *name, KernelObjectID processID, KernelObjectID threadID, uintptr_t pointerValue);
/**
 * @brief Adds a userspace object record to the stream with accompanying argument data.
 *
 * @param writer          The writer to use
 * @param name            The name of the userspace object
 * @param processID       The process ID
 * @param pointerValue    The pointer value
 * @param args            Arguments to add to the event
 * @return                0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#userspace-object-record
 */
int AddUserspaceObjectRecord(Writer *writer, const char *name, KernelObjectID processID, KernelObjectID threadID, uintptr_t pointerValue, std::initializer_list<RecordArgument> args);
/**
 * @brief Adds a userspace object record to the stream with accompanying argument data.
 *
 * @param writer          The writer to use
 * @param name            The name of the userspace object
 * @param processID       The process ID
 * @param pointerValue    The pointer value
 * @param args            Arguments to add to the event
 * @param numArgs         The number of arguments
 * @return                0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#userspace-object-record
 */
int AddUserspaceObjectRecord(Writer *writer, const char *name, KernelObjectID processID, KernelObjectID threadID, uintptr_t pointerValue, const RecordArgument *args, size_t numArgs);

/**
 * @brief Adds a context switch scheduling record to the stream with accompanying argument data.
 *
 * @note By convention, the caller may optionally include the following named arguments to provide additiona information to trace consumers:
 *     - "incoming_weight": Int32 describing the relative weight of the incoming thread
 *     - "outgoing_weight": Int32 describing the relative weight of the outgoing thread
 *
 * @param writer                 The writer to use
 * @param cpuNumber              The CPU core doing the switching
 * @param outgoingThreadState    The outgoing thread state
 * @param outgoingThreadID       The outgoing thread ID
 * @param incomingThreadID       The incoming thread ID
 * @param timestamp              The timestamp of the event
 * @return                       0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#context-switch-record-scheduling-event-record-type-1
 */
int AddContextSwitchRecord(Writer *writer, uint16_t cpuNumber, uint8_t outgoingThreadState, KernelObjectID outgoingThreadID, KernelObjectID incomingThreadID, uint64_t timestamp);
/**
 * @brief Adds a context switch scheduling record to the stream with accompanying argument data.
 *
 * @note By convention, the caller may optionally include the following named arguments to provide additiona information to trace consumers:
 *     - "incoming_weight": Int32 describing the relative weight of the incoming thread
 *     - "outgoing_weight": Int32 describing the relative weight of the outgoing thread
 *
 * @param writer                 The writer to use
 * @param cpuNumber              The CPU core doing the switching
 * @param outgoingThreadState    The outgoing thread state
 * @param outgoingThreadID       The outgoing thread ID
 * @param incomingThreadID       The incoming thread ID
 * @param timestamp              The timestamp of the event
 * @param args                   Arguments to add to the event
 * @return                       0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#context-switch-record-scheduling-event-record-type-1
 */
int AddContextSwitchRecord(Writer *writer, uint16_t cpuNumber, uint8_t outgoingThreadState, KernelObjectID outgoingThreadID, KernelObjectID incomingThreadID, uint64_t timestamp, std::initializer_list<RecordArgument> args);
/**
 * @brief Adds a context switch scheduling record to the stream with accompanying argument data.
 *
 * @note By convention, the caller may optionally include the following named arguments to provide additiona information to trace consumers:
 *     - "incoming_weight": Int32 describing the relative weight of the incoming thread
 *     - "outgoing_weight": Int32 describing the relative weight of the outgoing thread
 *
 * @param writer                 The writer to use
 * @param cpuNumber              The CPU core doing the switching
 * @param outgoingThreadState    The outgoing thread state
 * @param outgoingThreadID       The outgoing thread ID
 * @param incomingThreadID       The incoming thread ID
 * @param timestamp              The timestamp of the event
 * @param args                   Arguments to add to the event
 * @param numArgs                The number of arguments
 * @return                       0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#context-switch-record-scheduling-event-record-type-1
 */
int AddContextSwitchRecord(Writer *writer, uint16_t cpuNumber, uint8_t outgoingThreadState, KernelObjectID outgoingThreadID, KernelObjectID incomingThreadID, uint64_t timestamp, const RecordArgument *args, size_t numArgs);

/**
 * @brief Adds a thread wakeup scheduling record to the stream with accompanying argument data.
 *
 * @note By convention, the caller may optionally include the following named arguments to provide additional information to trace consumers:
 *     - "weight": Int32 describing the relative weight of the waking thread
 *
 * @param writer            The writer to use
 * @param cpuNumber         The CPU core doing the waking
 * @param wakingThreadID    The thread ID of the woken thread
 * @param timestamp         The timestamp of the event
 * @return                  0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#context-switch-record-scheduling-event-record-type-2
 */
int AddThreadWakeupRecord(Writer *writer, uint16_t cpuNumber, KernelObjectID wakingThreadID, uint64_t timestamp);
/**
 * @brief Adds a thread wakeup scheduling record to the stream with accompanying argument data.
 *
 * @note By convention, the caller may optionally include the following named arguments to provide additional information to trace consumers:
 *     - "weight": Int32 describing the relative weight of the waking thread
 *
 * @param writer            The writer to use
 * @param cpuNumber         The CPU core doing the waking
 * @param wakingThreadID    The thread ID of the woken thread
 * @param timestamp         The timestamp of the event
 * @param args              Arguments to add to the event
 * @return                  0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#context-switch-record-scheduling-event-record-type-2
 */
int AddThreadWakeupRecord(Writer *writer, uint16_t cpuNumber, KernelObjectID wakingThreadID, uint64_t timestamp, std::initializer_list<RecordArgument> args);
/**
 * @brief Adds a thread wakeup scheduling record to the stream with accompanying argument data.
 *
 * @note By convention, the caller may optionally include the following named arguments to provide additional information to trace consumers:
 *     - "weight": Int32 describing the relative weight of the waking thread
 *
 * @param writer            The writer to use
 * @param cpuNumber         The CPU core doing the waking
 * @param wakingThreadID    The thread ID of the woken thread
 * @param timestamp         The timestamp of the event
 * @param args              Arguments to add to the event
 * @param numArgs           The number of arguments
 * @return                  0 on success. Non-zero for failure
 *
 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#context-switch-record-scheduling-event-record-type-2
 */
int AddThreadWakeupRecord(Writer *writer, uint16_t cpuNumber, KernelObjectID wakingThreadID, uint64_t timestamp, const RecordArgument *args, size_t numArgs);

} // End of namespace fxt

#define FXT_INTERNAL_DECLARE_ARG(name, value) \
	fxt::RecordArgument(name, fxt::RecordArgumentValue(value))

#define FXT_ADD_INSTANT_EVENT(writer, category, name, processID, threadID, timestamp, ...) \
	AddInstantEvent(writer, category, name, processID, threadID, timestamp, { FXT_INTERNAL_APPLY_PAIRWISE_CSV(FXT_INTERNAL_DECLARE_ARG, __VA_ARGS__) })

#define FXT_ADD_COUNTER_EVENT(writer, category, name, processID, threadID, timestamp, counterID, ...) \
	AddCounterEvent(writer, category, name, processID, threadID, timestamp, counterID, { FXT_INTERNAL_APPLY_PAIRWISE_CSV(FXT_INTERNAL_DECLARE_ARG, __VA_ARGS__) })

#define FXT_ADD_DURATION_BEGIN_EVENT(writer, category, name, processID, threadID, timestamp, ...) \
	AddDurationBeginEvent(writer, category, name, processID, threadID, timestamp, { FXT_INTERNAL_APPLY_PAIRWISE_CSV(FXT_INTERNAL_DECLARE_ARG, __VA_ARGS__) })

#define FXT_ADD_DURATION_END_EVENT(writer, category, name, processID, threadID, timestamp, ...) \
	AddDurationEndEvent(writer, category, name, processID, threadID, timestamp, { FXT_INTERNAL_APPLY_PAIRWISE_CSV(FXT_INTERNAL_DECLARE_ARG, __VA_ARGS__) })

#define FXT_ADD_DURATION_COMPLETE_EVENT(writer, category, name, processID, threadID, beginTimestamp, endTimestamp, ...) \
	AddDurationCompleteEvent(writer, category, name, processID, threadID, beginTimestamp, endTimestamp, { FXT_INTERNAL_APPLY_PAIRWISE_CSV(FXT_INTERNAL_DECLARE_ARG, __VA_ARGS__) })

#define FXT_ADD_ASYNC_BEGIN_EVENT(writer, category, name, processID, threadID, timestamp, asyncCorrelationID, ...) \
	AddAsyncBeginEvent(writer, category, name, processID, threadID, timestamp, asyncCorrelationID, { FXT_INTERNAL_APPLY_PAIRWISE_CSV(FXT_INTERNAL_DECLARE_ARG, __VA_ARGS__) })

#define FXT_ADD_ASYNC_INSTANT_EVENT(writer, category, name, processID, threadID, timestamp, asyncCorrelationID, ...) \
	AddAsyncInstantEvent(writer, category, name, processID, threadID, timestamp, asyncCorrelationID, { FXT_INTERNAL_APPLY_PAIRWISE_CSV(FXT_INTERNAL_DECLARE_ARG, __VA_ARGS__) })

#define FXT_ADD_ASYNC_END_EVENT(writer, category, name, processID, threadID, timestamp, asyncCorrelationID, ...) \
	AddAsyncEndEvent(writer, category, name, processID, threadID, timestamp, asyncCorrelationID, { FXT_INTERNAL_APPLY_PAIRWISE_CSV(FXT_INTERNAL_DECLARE_ARG, __VA_ARGS__) })

#define FXT_ADD_FLOW_BEGIN_EVENT(writer, category, name, processID, threadID, timestamp, flowCorrelationID, ...) \
	AddFlowBeginEvent(writer, category, name, processID, threadID, timestamp, flowCorrelationID, { FXT_INTERNAL_APPLY_PAIRWISE_CSV(FXT_INTERNAL_DECLARE_ARG, __VA_ARGS__) })

#define FXT_ADD_FLOW_STEP_EVENT(writer, category, name, processID, threadID, timestamp, flowCorrelationID, ...) \
	AddFlowStepEvent(writer, category, name, processID, threadID, timestamp, flowCorrelationID, { FXT_INTERNAL_APPLY_PAIRWISE_CSV(FXT_INTERNAL_DECLARE_ARG, __VA_ARGS__) })

#define FXT_ADD_FLOW_END_EVENT(writer, category, name, processID, threadID, timestamp, flowCorrelationID, ...) \
	AddFlowEndEvent(writer, category, name, processID, threadID, timestamp, flowCorrelationID, { FXT_INTERNAL_APPLY_PAIRWISE_CSV(FXT_INTERNAL_DECLARE_ARG, __VA_ARGS__) })

#define FXT_ADD_USERSPACE_OBJECT_RECORD(writer, name, processID, threadID, pointerValue, ...) \
	AddUserspaceObjectRecord(writer, name, processID, threadID, pointerValue, { FXT_INTERNAL_APPLY_PAIRWISE_CSV(FXT_INTERNAL_DECLARE_ARG, __VA_ARGS__) })

#define FXT_ADD_CONTEXT_SWITCH_RECORD(writer, cpuNumber, outgoingThreadState, outgoingThreadID, incomingThreadID, timestamp, ...) \
	AddContextSwitchRecord(writer, cpuNumber, outgoingThreadState, outgoingThreadID, incomingThreadID, timestamp, { FXT_INTERNAL_APPLY_PAIRWISE_CSV(FXT_INTERNAL_DECLARE_ARG, __VA_ARGS__) })

#define FXT_ADD_THREAD_WAKEUP_RECORD(writer, cpuNumber, wakingThreadID, timestamp, ...) \
	AddThreadWakeupRecord(writer, cpuNumber, wakingThreadID, timestamp, { FXT_INTERNAL_APPLY_PAIRWISE_CSV(FXT_INTERNAL_DECLARE_ARG, __VA_ARGS__) })
