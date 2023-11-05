/* FXT - A library for creating Fuschia Tracing System (FXT) files
 *
 * FXT is the legal property of Adrian Astley
 * Copyright Adrian Astley 2023
 */

#pragma once

#include "fxt/err.h"
#include "fxt/event_args.h"
#include "fxt/internal/constants.h"

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

/**
 * @brief Writer is a class for creating FXT streams
 *
 * See asdf
 */
class Writer {
public:
	/**
	 * @brief Construct a new Writer.
	 *
	 * @note Write does *not* do any buffering of data. It will directly call writeFunc() for each
	 *       write. It's up to the user to do buffering if they require it
	 *
	 * @warning Writer is *not* thread-safe. It's up to the caller to protect it with a mutex if they
	 *          wish to call it from multiple threads
	 *
	 * @param userContext    A pointer that will be passed to writeFunc when called
	 * @param writeFunc      This function will be called whenever data needs to be written to the stream
	 */
	Writer(void *userContext, WriteFunc writeFunc);
	~Writer();

private:
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
	uint64_t m_stringTable[512];
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
	uint64_t m_threadTable[128];
	uint16_t m_nextStringIndex = 0;
	uint16_t m_nextThreadIndex = 0;

	void *m_userContext;
	WriteFunc m_writeFunc;

public:
	/**
	 * @brief Adds a Magic Number record to the stream
	 *
	 * @return    0 on success. Non-zero for failure
	 *
	 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#magic-number-record
	 */
	int WriteMagicNumberRecord();
	/**
	 * @brief Adds a provider info metadata record to the stream.
	 *
	 * @param providerID      The provider ID
	 * @param providerName    The human-readable name for the provider
	 * @return                0 on success. Non-zero for failure
	 *
	 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#provider-info-metadata
	 */
	int AddProviderInfoRecord(uint32_t providerID, const char *providerName);
	/**
	 * @brief Adds a provider section metadata record to the stream.
	 *
	 * @param providerID    The provider ID
	 * @return              0 on success. Non-zero for failure
	 *
	 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#provider-section-metadata
	 */
	int AddProviderSectionRecord(uint32_t providerID);
	/**
	 * @brief Adds a provider event metadata record to the stream.
	 *
	 * @param providerID    The provider ID
	 * @param eventType     The event type
	 * @return              0 on success. Non-zero for failure
	 *
	 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#provider-event-metadata
	 */
	int AddProviderEventRecord(uint32_t providerID, ProviderEventType eventType);
	/**
	 * @brief Adds an initialization record to the stream.
	 *
	 * This specifies the number of ticks per second for all event records after this one.
	 * If you need to change the tick rate, you can add another initialization record to
	 * the stream, and then send more event records.
	 *
	 * @param numTicksPerSecond    The number of ticks per second for all events after this record
	 * @return                     0 on success. Non-zero for failure
	 *
	 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#initialization-record
	 */
	int AddInitializationRecord(uint64_t numTicksPerSecond);
	/**
	 * @brief Adds a kernel object record to give a human-readable name to a process ID.
	 *
	 * @param processID    The process ID
	 * @param name         The human-readable name to give the process
	 * @return             0 on success. Non-zero for failure
	 *
	 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#kernel-object-record
	 */
	int SetProcessName(KernelObjectID processID, const char *name);
	/**
	 * @brief Adds a kernel object record to give a human-readable name to a thread ID.
	 *
	 * @param processID    The process ID
	 * @param threadID     The thread ID
	 * @param name         The human-readable name to give the thread
	 * @return             0 on success. Non-zero for failure
	 *
	 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#kernel-object-record
	 */
	int SetThreadName(KernelObjectID processID, KernelObjectID threadID, const char *name);

	/**
	 * @brief Adds an instant event record to the stream.
	 *
	 * @param category     The category of the event
	 * @param name         The name of the event
	 * @param processID    The process ID
	 * @param threadID     The thread ID
	 * @param timestamp    The timestamp of the event
	 * @param arguments    Optional arguments to add to the event
	 * @return             0 on success. Non-zero for failure
	 *
	 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#instant-event
	 */
	template <typename... Arguments>
	int AddInstantEvent(const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, Arguments... arguments);
	/**
	 * @brief Adds a counter event record to the stream.
	 *
	 * @param category     The category of the event
	 * @param name         The name of the event
	 * @param processID    The process ID
	 * @param threadID     The thread ID
	 * @param timestamp    The timestamp of the event
	 * @param counterID    The counter ID
	 * @param arguments    Optional arguments to add to the event
	 * @return             0 on success. Non-zero for failure
	 *
	 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#counter-event
	 */
	template <typename... Arguments>
	int AddCounterEvent(const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t counterID, Arguments... arguments);
	/**
	 * @brief Adds a duration begin event record to the stream.
	 *
	 * @param category     The category of the event
	 * @param name         The name of the event
	 * @param processID    The process ID
	 * @param threadID     The thread ID
	 * @param timestamp    The timestamp of the event
	 * @param arguments    Optional arguments to add to the event
	 * @return             0 on success. Non-zero for failure
	 *
	 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#duration-begin-event
	 */
	template <typename... Arguments>
	int AddDurationBeginEvent(const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, Arguments... arguments);
	/**
	 * @brief Adds a duration end event record to the stream.
	 *
	 * @param category     The category of the event
	 * @param name         The name of the event
	 * @param processID    The process ID
	 * @param threadID     The thread ID
	 * @param timestamp    The timestamp of the event
	 * @param arguments    Optional arguments to add to the event
	 * @return             0 on success. Non-zero for failure
	 *
	 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#duration-end-event
	 */
	template <typename... Arguments>
	int AddDurationEndEvent(const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, Arguments... arguments);
	/**
	 * @brief Adds a duration complete event record to the stream.
	 *
	 * @param category     The category of the event
	 * @param name         The name of the event
	 * @param processID    The process ID
	 * @param threadID     The thread ID
	 * @param timestamp    The timestamp of the event
	 * @param arguments    Optional arguments to add to the event
	 * @return             0 on success. Non-zero for failure
	 *
	 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#duration-complete-event
	 */
	template <typename... Arguments>
	int AddDurationCompleteEvent(const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t beginTimestamp, uint64_t endTimestamp, Arguments... arguments);
	/**
	 * @brief Adds an async begin event record to the stream.
	 *
	 * @param category              The category of the event
	 * @param name                  The name of the event
	 * @param processID             The process ID
	 * @param threadID              The thread ID
	 * @param timestamp             The timestamp of the event
	 * @param asyncCorrelationID    The async correlation ID
	 * @param arguments             Optional arguments to add to the event
	 * @return                      0 on success. Non-zero for failure
	 *
	 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#async-begin-event
	 */
	template <typename... Arguments>
	int AddAsyncBeginEvent(const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t asyncCorrelationID, Arguments... arguments);
	/**
	 * @brief Adds an async instant event record to the stream.
	 *
	 * @param category              The category of the event
	 * @param name                  The name of the event
	 * @param processID             The process ID
	 * @param threadID              The thread ID
	 * @param timestamp             The timestamp of the event
	 * @param asyncCorrelationID    The async correlation ID
	 * @param arguments             Optional arguments to add to the event
	 * @return                      0 on success. Non-zero for failure
	 *
	 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#async-instant-event
	 */
	template <typename... Arguments>
	int AddAsyncInstantEvent(const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t asyncCorrelationID, Arguments... arguments);
	/**
	 * @brief Adds an async end event record to the stream.
	 *
	 * @param category              The category of the event
	 * @param name                  The name of the event
	 * @param processID             The process ID
	 * @param threadID              The thread ID
	 * @param timestamp             The timestamp of the event
	 * @param asyncCorrelationID    The async correlation ID
	 * @param arguments             Optional arguments to add to the event
	 * @return                      0 on success. Non-zero for failure
	 *
	 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#async-end-event
	 */
	template <typename... Arguments>
	int AddAsyncEndEvent(const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t asyncCorrelationID, Arguments... arguments);
	/**
	 * @brief Adds an flow begin event record to the stream.
	 *
	 * @param category             The category of the event
	 * @param name                 The name of the event
	 * @param processID            The process ID
	 * @param threadID             The thread ID
	 * @param timestamp            The timestamp of the event
	 * @param flowCorrelationID    The flow correlation ID
	 * @param arguments            Optional arguments to add to the event
	 * @return                     0 on success. Non-zero for failure
	 *
	 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#flow-begin-event
	 */
	template <typename... Arguments>
	int AddFlowBeginEvent(const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t flowCorrelationID, Arguments... arguments);
	/**
	 * @brief Adds an flow step event record to the stream.
	 *
	 * @param category             The category of the event
	 * @param name                 The name of the event
	 * @param processID            The process ID
	 * @param threadID             The thread ID
	 * @param timestamp            The timestamp of the event
	 * @param flowCorrelationID    The flow correlation ID
	 * @param arguments            Optional arguments to add to the event
	 * @return                     0 on success. Non-zero for failure
	 *
	 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#flow-instant-event
	 */
	template <typename... Arguments>
	int AddFlowStepEvent(const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t flowCorrelationID, Arguments... arguments);
	/**
	 * @brief Adds an flow end event record to the stream.
	 *
	 * @param category             The category of the event
	 * @param name                 The name of the event
	 * @param processID            The process ID
	 * @param threadID             The thread ID
	 * @param timestamp            The timestamp of the event
	 * @param flowCorrelationID    The flow correlation ID
	 * @param arguments            Optional arguments to add to the event
	 * @return                     0 on success. Non-zero for failure
	 *
	 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#flow-end-event
	 */
	template <typename... Arguments>
	int AddFlowEndEvent(const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, uint64_t flowCorrelationID, Arguments... arguments);

	/**
	 * @brief Adds a blob record to the stream.
	 *
	 * @param name        The name of the blob
	 * @param data        A pointer to the blob data
	 * @param dataLen     The length of the data
	 * @param blobType    The type of the blob
	 * @return            0 on success. Non-zero for failure
	 *
	 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#blob-record
	 */
	int AddBlobRecord(const char *name, void *data, size_t dataLen, BlobType blobType);

	/**
	 * @brief Adds a userspace object record to the stream.
	 *
	 * @param name            The name of the userspace object
	 * @param processID       The process ID
	 * @param pointerValue    The pointer value
	 * @return                0 on success. Non-zero for failure
	 *
	 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#userspace-object-record
	 */
	int AddUserspaceObjectRecord(const char *name, KernelObjectID processID, KernelObjectID threadID, uintptr_t pointerValue);
	/**
	 * @brief Adds a userspace object record to the stream with accompanying argument data.
	 *
	 * @param name            The name of the userspace object
	 * @param processID       The process ID
	 * @param pointerValue    The pointer value
	 * @param arguments       Optional arguments to add to the record
	 * @return                0 on success. Non-zero for failure
	 *
	 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#userspace-object-record
	 */
	template <typename... Arguments>
	int AddUserspaceObjectRecord(const char *name, KernelObjectID processID, KernelObjectID threadID, uintptr_t pointerValue, Arguments... arguments);

	/**
	 * @brief Adds a context switch scheduling record to the stream.
	 *
	 * @note By convention, the caller may optionally include the following named arguments to provide additiona information to trace consumers:
	 *     - "incoming_weight": Int32 describing the relative weight of the incoming thread
	 *     - "outgoing_weight": Int32 describing the relative weight of the outgoing thread
	 *
	 * @param cpuNumber              The CPU core doing the switching
	 * @param outgoingThreadState    The outgoing thread state
	 * @param outgoingThreadID       The outgoing thread ID
	 * @param incomingThreadID       The incoming thread ID
	 * @param timestamp              The timestamp of the event
	 * @return                       0 on success. Non-zero for failure
	 *
	 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#context-switch-record-scheduling-event-record-type-1
	 */
	int AddContextSwitchRecord(uint16_t cpuNumber, uint8_t outgoingThreadState, KernelObjectID outgoingThreadID, KernelObjectID incomingThreadID, uint64_t timestamp);
	/**
	 * @brief Adds a context switch scheduling record to the stream with accompanying argument data.
	 *
	 * @note By convention, the caller may optionally include the following named arguments to provide additiona information to trace consumers:
	 *     - "incoming_weight": Int32 describing the relative weight of the incoming thread
	 *     - "outgoing_weight": Int32 describing the relative weight of the outgoing thread
	 *
	 * @param cpuNumber              The CPU core doing the switching
	 * @param outgoingThreadState    The outgoing thread state
	 * @param outgoingThreadID       The outgoing thread ID
	 * @param incomingThreadID       The incoming thread ID
	 * @param timestamp              The timestamp of the event
	 * @param arguments              Optional arguments to add to the record
	 * @return                       0 on success. Non-zero for failure
	 *
	 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#context-switch-record-scheduling-event-record-type-1
	 */
	template <typename... Arguments>
	int AddContextSwitchRecord(uint16_t cpuNumber, uint8_t outgoingThreadState, KernelObjectID outgoingThreadID, KernelObjectID incomingThreadID, uint64_t timestamp, Arguments... arguments);

	/**
	 * @brief Adds a thread wakeup scheduling record to the stream.
	 *
	 * @note By convention, the caller may optionally include the following named arguments to provide additiona information to trace consumers:
	 *     - "weight": Int32 describing the relative weight of the waking thread
	 *
	 * @param cpuNumber         The CPU core doing the waking
	 * @param wakingThreadID    The thread ID of the woken thread
	 * @param timestamp         The timestamp of the event
	 * @return                  0 on success. Non-zero for failure
	 *
	 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#context-switch-record-scheduling-event-record-type-2
	 */
	int AddThreadWakeupRecord(uint16_t cpuNumber, KernelObjectID wakingThreadID, uint64_t timestamp);
	/**
	 * @brief Adds a thread wakeup scheduling record to the stream with accompanying argument data.
	 *
	 * @note By convention, the caller may optionally include the following named arguments to provide additiona information to trace consumers:
	 *     - "weight": Int32 describing the relative weight of the waking thread
	 *
	 * @param cpuNumber         The CPU core doing the waking
	 * @param wakingThreadID    The thread ID of the woken thread
	 * @param timestamp         The timestamp of the event
	 * @param arguments         Optional arguments to add to the record
	 * @return                  0 on success. Non-zero for failure
	 *
	 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#context-switch-record-scheduling-event-record-type-2
	 */
	template <typename... Arguments>
	int AddThreadWakeupRecord(uint16_t cpuNumber, KernelObjectID wakingThreadID, uint64_t timestamp, Arguments... arguments);

private:
	/**
	 * @brief Writes the given value to the stream.
	 *
	 * @param val    The value to write to the stream
	 * @return       0 on success. Non-zero for failure
	 */
	int WriteUInt64ToStream(uint64_t val);
	/**
	 * @brief Writes the given byte array to the stream.
	 *
	 * @param val     A pointer to the byte array
	 * @param len     The length of the byte array
	 * @return        0 on success. Non-zero for failure
	 */
	int WriteBytesToStream(const void *val, size_t len);
	/**
	 * @brief Writes a null to the stream the given number of times.
	 *
	 * @param count     The number of nulls to write
	 * @return          0 on success. Non-zero for failure
	 */
	int WriteZeroPadding(size_t count);

	/**
	 * @brief Adds a string record to the stream.
	 *
	 * @param stringIndex    The index of the string (for future use in other records)
	 * @param str            A pointer to the string characters
	 * @param strLen         The length of the string
	 * @return               0 on success. Non-zero for failure
	 *
	 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#string-record
	 */
	int AddStringRecord(uint16_t stringIndex, const char *str, size_t strLen);
	/**
	 * @brief Adds a thread record to the stream.
	 *
	 * @param threadIndex    The index of the thread (for future use in other records)
	 * @param processID      The process ID
	 * @param threadID       The thread ID
	 * @return               0 on success. Non-zero for failure
	 *
	 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#thread-record
	 */
	int AddThreadRecord(uint16_t threadIndex, KernelObjectID processID, KernelObjectID threadID);

	/**
	 * @brief Finds the matching string index if it exists. Or creates a new string record and returns that index.
	 *
	 * @param      str         The string to get an index for
	 * @param[out] strIndex    On success, will be filled with the computed string index
	 * @return                 0 on success. Non-zero for failure
	 *
	 * @see Writer::m_stringTable
	 */
	int GetOrCreateStringIndex(const char *str, uint16_t *strIndex);
	/**
	 * @brief Finds the matching thread index if it exists. Or creates a new thread record and returns that index.
	 *
	 * @param processID      The process ID
	 * @param threadID       The thread ID
	 * @param threadIndex    On success, will be filled with the computed thread index
	 * @return               0 on success. Non-zero for failure
	 *
	 * @see Writer::m_threadTable
	 */
	int GetOrCreateThreadIndex(KernelObjectID processID, KernelObjectID threadID, uint16_t *threadIndex);

	/**
	 * @brief Helper function to write out an event record.
	 *
	 * @param eventType           The event type
	 * @param category            The event category
	 * @param name                The event name
	 * @param processID           The process ID
	 * @param threadID            The thread ID
	 * @param timestamp           The timestamp of the event
	 * @param extraSizeInWords    Some events add extra data to the record. This is the size of that data *in words*
	 * @return                    0 on success. Non-zero for failure
	 *
	 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#event-record
	 */
	int WriteEventHeaderAndGenericData(internal::EventType eventType, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, unsigned extraSizeInWords);
	/**
	 * @brief Helper function to write out an event record with accompanying argument data.
	 *
	 * @param eventType           The event type
	 * @param category            The event category
	 * @param name                The event name
	 * @param processID           The process ID
	 * @param threadID            The thread ID
	 * @param timestamp           The timestamp of the event
	 * @param extraSizeInWords    Some events add extra data to the record. This is the size of that data *in words*
	 * @param arguments           The argument data to include. These *must* be pointers to EventArgument derived classes
	 * @return                    0 on success. Non-zero for failure
	 *
	 * @see https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md#event-record
	 */
	template <typename... Arguments>
	int WriteEventHeaderAndGenericData(internal::EventType eventType, const char *category, const char *name, KernelObjectID processID, KernelObjectID threadID, uint64_t timestamp, unsigned extraSizeInWords, Arguments... arguments);

private:
	// Friends

	// We declare these as friends so we don't expose all the internal helper functions to the public
	friend class NullEventArgument;
	friend class Int32EventArgument;
	friend class UInt32EventArgument;
	friend class Int64EventArgument;
	friend class UInt64EventArgument;
	friend class DoubleEventArgument;
	friend class StringEventArgument;
	friend class PointerEventArgument;
	friend class KOIDEventArgument;
	friend class BoolEventArgument;

	// So we can test private methods with unit testing
	friend class WriterTest;
};

} // End of namespace fxt

// Implementation
#include "fxt/internal/writer-impl.h"
