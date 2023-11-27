/* FXT - A library for creating Fuschia Tracing System (FXT) files
 *
 * FXT is the legal property of Adrian Astley
 * Copyright Adrian Astley 2023
 */

#include "fxt/writer.h"

#include "writer_test.h"

#include "catch2/catch_test_macros.hpp"

#include <stdio.h>
#include <vector>

TEST_CASE("TestGeneralWrite", "[write]") {
	std::vector<uint8_t> buffer;

	fxt::Writer writer((void *)&buffer, [](void *userContext, const void *data, size_t len) -> int {
		std::vector<uint8_t> *buffer = (std::vector<uint8_t> *)userContext;

		buffer->insert(buffer->end(), (const uint8_t *)data, (const uint8_t *)data + len);
		return 0;
	});

	// // Add the magic number record
	REQUIRE(WriteMagicNumberRecord(&writer) == 0);

	// Set up the provider info
	REQUIRE(AddProviderInfoRecord(&writer, 1234, "Test Provider") == 0);
	REQUIRE(AddProviderSectionRecord(&writer, 1234) == 0);
	REQUIRE(AddInitializationRecord(&writer, 1000) == 0);

	// Name the processes / threads
	REQUIRE(SetProcessName(&writer, 3, "Test.exe") == 0);
	REQUIRE(SetThreadName(&writer, 3, 45, "Main") == 0);
	REQUIRE(SetThreadName(&writer, 3, 87, "Worker0") == 0);
	REQUIRE(SetThreadName(&writer, 3, 26, "Worker1") == 0);
	REQUIRE(SetProcessName(&writer, 4, "Server.exe") == 0);
	REQUIRE(SetThreadName(&writer, 4, 50, "ServerThread") == 0);

	// Do a basic set of spans
	// And throw in some async
	REQUIRE(AddDurationBeginEvent(&writer, "Foo", "Root", 3, 45, 200) == 0);
	REQUIRE(AddInstantEvent(&writer, "OtherThing", "EventHappened", 3, 45, 300) == 0);
	REQUIRE(AddDurationBeginEvent(&writer, "Foo", "Inner", 3, 45, 400) == 0);
	REQUIRE(AddAsyncBeginEvent(&writer, "Asdf", "AsyncThing", 3, 45, 450, 111) == 0);
	REQUIRE(AddDurationCompleteEvent(&writer, "OtherService", "DoStuff", 3, 45, 500, 800) == 0);
	REQUIRE(AddAsyncInstantEvent(&writer, "Asdf", "AsyncInstant", 3, 87, 825, 111) == 0);
	REQUIRE(AddAsyncEndEvent(&writer, "Asdf", "AsyncThing", 3, 87, 850, 111) == 0);
	REQUIRE(AddDurationEndEvent(&writer, "Foo", "Inner", 3, 45, 900) == 0);
	REQUIRE(AddDurationEndEvent(&writer, "Foo", "Root", 3, 45, 900) == 0);

	// Test out flows
	REQUIRE(AddDurationBeginEvent(&writer, "CategoryA", "REST Request to server", 3, 45, 950) == 0);
	REQUIRE(AddFlowBeginEvent(&writer, "CategoryA", "AwesomeFlow", 3, 45, 955, 123) == 0);
	REQUIRE(AddDurationEndEvent(&writer, "CategoryA", "REST Request to server", 3, 45, 1000) == 0);
	REQUIRE(AddDurationBeginEvent(&writer, "CategoryA", "Server process request", 4, 50, 1000) == 0);
	REQUIRE(AddFlowStepEvent(&writer, "CategoryA", "Server request handler", 4, 50, 1005, 123) == 0);
	REQUIRE(AddDurationEndEvent(&writer, "CategoryA", "Server process request", 4, 50, 1100) == 0);
	REQUIRE(AddDurationBeginEvent(&writer, "CategoryA", "Process server response", 3, 45, 1150) == 0);
	REQUIRE(AddFlowEndEvent(&writer, "CategoryA", "AwesomeFlow", 3, 45, 1155, 123) == 0);
	REQUIRE(AddDurationEndEvent(&writer, "CategoryA", "Process server response", 3, 45, 1200) == 0);

	// Add some counter events
	REQUIRE(FXT_ADD_COUNTER_EVENT(&writer, "Bar", "CounterA", 3, 45, 250, 555, "int_arg", 111, "uint_arg", (uint32_t)984, "double_arg", 1.0, "int64_arg", (int64_t)851, "uint64_arg", (uint64_t)35) == 0);
	REQUIRE(FXT_ADD_COUNTER_EVENT(&writer, "Bar", "CounterA", 3, 45, 500, 555, "int_arg", 784, "uint_arg", (uint32_t)561, "double_arg", 4.0, "int64_arg", (int64_t)445, "uint64_arg", (uint64_t)95) == 0);
	REQUIRE(FXT_ADD_COUNTER_EVENT(&writer, "Bar", "CounterA", 3, 45, 700, 555, "int_arg", 333, "uint_arg", (uint32_t)845, "double_arg", 9.0, "int64_arg", (int64_t)521, "uint64_arg", (uint64_t)24) == 0);

	// Add a blob record
	REQUIRE(AddBlobRecord(&writer, "TestBlob", (void *)"testing123", 10, fxt::BlobType::Data) == 0);

	// Add events with argument data
	REQUIRE(FXT_ADD_DURATION_BEGIN_EVENT(&writer, "Foo", "Root", 3, 87, 1200, "null_arg", nullptr) == 0);
	REQUIRE(FXT_ADD_INSTANT_EVENT(&writer, "OtherThing", "EventHappened", 3, 87, 1300, "int_arg", 4567) == 0);
	REQUIRE(FXT_ADD_DURATION_BEGIN_EVENT(&writer, "Foo", "Inner", 3, 87, 1400, "uint_arg", (uint32_t)333) == 0);
	REQUIRE(FXT_ADD_ASYNC_BEGIN_EVENT(&writer, "Asdf", "AsyncThing2", 3, 87, 1450, 222, "int64_arg", (int64_t)784) == 0);
	REQUIRE(FXT_ADD_DURATION_COMPLETE_EVENT(&writer, "OtherService", "DoStuff", 3, 87, 1500, 800, "uint64_arg", (uint64_t)454) == 0);
	REQUIRE(FXT_ADD_ASYNC_INSTANT_EVENT(&writer, "Asdf", "AsyncInstant2", 3, 26, 1825, 222, "double_arg", 333.3424) == 0);
	REQUIRE(FXT_ADD_ASYNC_END_EVENT(&writer, "Asdf", "AsyncThing2", 3, 26, 1850, 222, "string_arg", "str_value") == 0);
	REQUIRE(FXT_ADD_USERSPACE_OBJECT_RECORD(&writer, "MyAwesomeObject", 3, 26, (uintptr_t)(67890), "bool_arg", true) == 0);
	REQUIRE(FXT_ADD_DURATION_END_EVENT(&writer, "Foo", "Inner", 3, 87, 1900, "pointer_arg", &writer) == 0);
	REQUIRE(FXT_ADD_DURATION_END_EVENT(&writer, "Foo", "Root", 3, 87, 1900, "koid_arg", fxt::RecordArgumentValue::KOID(3)) == 0);

	// Add some scheduling events
	FXT_ADD_CONTEXT_SWITCH_RECORD(&writer, 3, 1, 45, 87, 250, "incoming_weight", 2, "outgoing_weight", 4);
	FXT_ADD_CONTEXT_SWITCH_RECORD(&writer, 3, 1, 87, 45, 255, "incoming_weight", 2, "outgoing_weight", 4);
	FXT_ADD_THREAD_WAKEUP_RECORD(&writer, 3, 45, 925, "weight", 5);
}

TEST_CASE("TestUsingSameStringReturnsSameIndex", "[write]") {
	fxt::Writer writer(nullptr, [](void *userContext, const void *data, size_t len) -> int {
		// Just drop the data
		// We don't need it for this test
		return 0;
	});

	// Get the string index
	uint16_t strIndex;
	REQUIRE(fxt::GetOrCreateStringIndex(&writer, "foo", &strIndex) == 0);

	// Do it again with the same string
	// It should return the same index
	uint16_t newIndex;
	REQUIRE(fxt::GetOrCreateStringIndex(&writer, "foo", &newIndex) == 0);

	REQUIRE(newIndex == strIndex);

	// If we do it a third time with a new string, it should return a different index
	uint16_t thirdIndex;
	REQUIRE(fxt::GetOrCreateStringIndex(&writer, "bar", &thirdIndex) == 0);
	REQUIRE(thirdIndex != strIndex);
}

TEST_CASE("TestUsingSameThreadReturnsSameIndex", "[write]") {
	fxt::Writer writer(nullptr, [](void *userContext, const void *data, size_t len) -> int {
		// Just drop the data
		// We don't need it for this test
		return 0;
	});

	// Get the thread index
	uint16_t threadIndex;
	REQUIRE(GetOrCreateThreadIndex(&writer, 1, 2, &threadIndex) == 0);

	// Do it again with the same thread
	// It should return the same index
	uint16_t newIndex;
	REQUIRE(GetOrCreateThreadIndex(&writer, 1, 2, &newIndex) == 0);

	REQUIRE(newIndex == threadIndex);

	// If we do it a third time with a new threadID, it should return a different index
	uint16_t thirdIndex;
	REQUIRE(GetOrCreateThreadIndex(&writer, 1, 3, &thirdIndex) == 0);
	REQUIRE(thirdIndex != threadIndex);

	// Using the same thread ID, but a different process ID is still a different thread
	uint16_t fourthIndex;
	REQUIRE(GetOrCreateThreadIndex(&writer, 2, 2, &fourthIndex) == 0);
	REQUIRE(fourthIndex != threadIndex);
	REQUIRE(fourthIndex != thirdIndex);
}

TEST_CASE("TestOverflowingStringTableWraps", "[write]") {
	fxt::Writer writer(nullptr, [](void *userContext, const void *data, size_t len) -> int {
		// Just drop the data
		// We don't need it for this test
		return 0;
	});

	char buffer[128];
	uint16_t strIndex;
	for (int i = 0; i < 512; ++i) {
		// Generate a unique string for each round. So we get a new index
		REQUIRE(snprintf(buffer, sizeof(buffer), "str-%d", i) < sizeof(buffer));
		REQUIRE(fxt::GetOrCreateStringIndex(&writer, buffer, &strIndex) == 0);
		// Zero is a reserved index
		// We should never get it
		REQUIRE(strIndex != 0);
	}

	REQUIRE(strIndex == 512);

	// If we add one more string, we should wrap
	REQUIRE(fxt::GetOrCreateStringIndex(&writer, "foo", &strIndex) == 0);
	REQUIRE(strIndex == 1);
}

TEST_CASE("TestOverflowingThreadTableWraps", "[write]") {
	fxt::Writer writer(nullptr, [](void *userContext, const void *data, size_t len) -> int {
		// Just drop the data
		// We don't need it for this test
		return 0;
	});

	uint16_t threadIndex;
	for (int i = 0; i < 128; ++i) {
		REQUIRE(GetOrCreateThreadIndex(&writer, 1, i, &threadIndex) == 0);
		// Zero is a reserved index
		// We should never get it
		REQUIRE(threadIndex != 0);
	}

	REQUIRE(threadIndex == 128);

	// If we add one more thread, we should wrap
	REQUIRE(GetOrCreateThreadIndex(&writer, 2, 1, &threadIndex) == 0);
	REQUIRE(threadIndex == 1);
}
