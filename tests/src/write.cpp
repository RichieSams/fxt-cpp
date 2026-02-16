/* FXT - A library for creating Fuschia Tracing System (FXT) files
 *
 * FXT is the legal property of Adrian Astley
 * Copyright Adrian Astley 2023
 */

#include "fxt/writer.h"

#include "constants.h"

#include "writer_test.h"
#include "writer_validation.h"

#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_string.hpp"
#include "catch2/matchers/catch_matchers_vector.hpp"

#include <stdio.h>
#include <vector>

static uint64_t ValidateStringTableRecord(uint8_t *buffer, const char *expectedValue, uint16_t expectedValueStrRef);

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
	REQUIRE(AddCounterEvent(&writer, "Bar", "CounterA", 3, 45, 250, 555, FXT_ARG_LIST("int_arg", 111, "uint_arg", (uint32_t)984, "double_arg", 1.0, "int64_arg", (int64_t)851, "uint64_arg", (uint64_t)35)) == 0);
	REQUIRE(AddCounterEvent(&writer, "Bar", "CounterA", 3, 45, 500, 555, FXT_ARG_LIST("int_arg", 784, "uint_arg", (uint32_t)561, "double_arg", 4.0, "int64_arg", (int64_t)445, "uint64_arg", (uint64_t)95)) == 0);
	REQUIRE(AddCounterEvent(&writer, "Bar", "CounterA", 3, 45, 700, 555, FXT_ARG_LIST("int_arg", 333, "uint_arg", (uint32_t)845, "double_arg", 9.0, "int64_arg", (int64_t)521, "uint64_arg", (uint64_t)24)) == 0);

	// Add a blob record
	REQUIRE(AddBlobRecord(&writer, "TestBlob", (void *)"testing123", 10, fxt::BlobType::Data) == 0);

	// Add events with argument data
	REQUIRE(AddDurationBeginEvent(&writer, "Foo", "Root", 3, 87, 1200, FXT_ARG_LIST("null_arg", nullptr)) == 0);
	REQUIRE(AddInstantEvent(&writer, "OtherThing", "EventHappened", 3, 87, 1300, FXT_ARG_LIST("int_arg", 4567)) == 0);
	REQUIRE(AddDurationBeginEvent(&writer, "Foo", "Inner", 3, 87, 1400, FXT_ARG_LIST("uint_arg", (uint32_t)333)) == 0);
	REQUIRE(AddAsyncBeginEvent(&writer, "Asdf", "AsyncThing2", 3, 87, 1450, 222, FXT_ARG_LIST("int64_arg", (int64_t)784)) == 0);
	REQUIRE(AddDurationCompleteEvent(&writer, "OtherService", "DoStuff", 3, 87, 1500, 800, FXT_ARG_LIST("uint64_arg", (uint64_t)454)) == 0);
	REQUIRE(AddAsyncInstantEvent(&writer, "Asdf", "AsyncInstant2", 3, 26, 1825, 222, FXT_ARG_LIST("double_arg", 333.3424)) == 0);
	REQUIRE(AddAsyncEndEvent(&writer, "Asdf", "AsyncThing2", 3, 26, 1850, 222, FXT_ARG_LIST("string_arg", "str_value")) == 0);
	REQUIRE(AddUserspaceObjectRecord(&writer, "MyAwesomeObject", 3, 26, (uintptr_t)(67890), FXT_ARG_LIST("bool_arg", true)) == 0);
	REQUIRE(AddDurationEndEvent(&writer, "Foo", "Inner", 3, 87, 1900, FXT_ARG_LIST("pointer_arg", &writer)) == 0);
	REQUIRE(AddDurationEndEvent(&writer, "Foo", "Root", 3, 87, 1900, FXT_ARG_LIST("koid_arg", fxt::RecordArgumentValue::KOID(3))) == 0);

	// Add some scheduling events
	AddContextSwitchRecord(&writer, 3, 1, 45, 87, 250, FXT_ARG_LIST("incoming_weight", 2, "outgoing_weight", 4));
	AddContextSwitchRecord(&writer, 3, 1, 87, 45, 255, FXT_ARG_LIST("incoming_weight", 2, "outgoing_weight", 4));
	AddThreadWakeupRecord(&writer, 3, 45, 925, FXT_ARG_LIST("weight", 5));
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
		REQUIRE((size_t)snprintf(buffer, sizeof(buffer), "str-%d", i) < sizeof(buffer));
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

TEST_CASE("TestProviderInfoMetadataRecord", "[write]") {
	std::vector<uint8_t> buffer;

	fxt::Writer writer((void *)&buffer, [](void *userContext, const void *data, size_t len) -> int {
		std::vector<uint8_t> *buffer = (std::vector<uint8_t> *)userContext;

		buffer->insert(buffer->end(), (const uint8_t *)data, (const uint8_t *)data + len);
		return 0;
	});

	const uint64_t providerID = 128563;
	const char *providerName = "abcdefghi";

	REQUIRE(fxt::AddProviderInfoRecord(&writer, providerID, providerName) == 0);

	// The record should be a header (8 bytes) plus the length of the name, padded to the
	// next multiple of 8 bytes. (16 bytes)
	uint64_t expectedRecordSizeInWords = 3;
	REQUIRE(buffer.size() == expectedRecordSizeInWords * 8);

	// Validate the header fields
	uint64_t header = ReadUInt64(&buffer[0]);
	INFO("Header: " << std::showbase << std::hex << header);

	// Record type
	REQUIRE(GetFieldFromValue(0, 3, header) == (uint64_t)fxt::RecordType::Metadata);
	// Record size in multiples of uint64_t
	REQUIRE(GetFieldFromValue(4, 15, header) == expectedRecordSizeInWords);
	// Metadata type
	REQUIRE(GetFieldFromValue(16, 19, header) == (uint64_t)fxt::MetadataType::ProviderInfo);
	// Provider ID
	REQUIRE(GetFieldFromValue(20, 51, header) == providerID);
	// Name length
	const uint64_t nameLen = GetFieldFromValue(52, 59, header);
	REQUIRE(nameLen == strlen(providerName));
	// The rest should be zero
	REQUIRE(GetFieldFromValue(60, 63, header) == 0);

	// Check the name string
	std::string actualName(buffer.begin() + 8, buffer.begin() + (8 + nameLen));
	REQUIRE_THAT(actualName, Catch::Matchers::Equals(providerName));

	// Check that the remaining bytes are all zero
	std::vector<uint8_t> expectedZeros(5, 0);
	std::vector<uint8_t> actualBytes(buffer.begin() + 19, buffer.end());
	REQUIRE_THAT(actualBytes, Catch::Matchers::Equals(expectedZeros));
}

TEST_CASE("TestProviderSectionMetadataRecord", "[write]") {
	std::vector<uint8_t> buffer;

	fxt::Writer writer((void *)&buffer, [](void *userContext, const void *data, size_t len) -> int {
		std::vector<uint8_t> *buffer = (std::vector<uint8_t> *)userContext;

		buffer->insert(buffer->end(), (const uint8_t *)data, (const uint8_t *)data + len);
		return 0;
	});

	const uint64_t providerID = 128563;

	REQUIRE(fxt::AddProviderSectionRecord(&writer, providerID) == 0);

	// The record should just be a header (8 bytes)
	uint64_t expectedRecordSizeInWords = 1;
	REQUIRE(buffer.size() == expectedRecordSizeInWords * 8);

	// Validate the header fields
	uint64_t header = ReadUInt64(&buffer[0]);
	INFO("Header: " << std::showbase << std::hex << header);

	// Record type
	REQUIRE(GetFieldFromValue(0, 3, header) == (uint64_t)fxt::RecordType::Metadata);
	// Record size in multiples of uint64_t
	REQUIRE(GetFieldFromValue(4, 15, header) == expectedRecordSizeInWords);
	// Metadata type
	REQUIRE(GetFieldFromValue(16, 19, header) == (uint64_t)fxt::MetadataType::ProviderSection);
	// Provider ID
	REQUIRE(GetFieldFromValue(20, 51, header) == providerID);
	// The rest should be zero
	REQUIRE(GetFieldFromValue(52, 63, header) == 0);
}

TEST_CASE("TestProviderEventMetadataRecord", "[write]") {
	std::vector<uint8_t> buffer;

	fxt::Writer writer((void *)&buffer, [](void *userContext, const void *data, size_t len) -> int {
		std::vector<uint8_t> *buffer = (std::vector<uint8_t> *)userContext;

		buffer->insert(buffer->end(), (const uint8_t *)data, (const uint8_t *)data + len);
		return 0;
	});

	const uint64_t providerID = 128563;
	const fxt::ProviderEventType providerEvent = fxt::ProviderEventType::BufferFilledUp;

	REQUIRE(fxt::AddProviderEventRecord(&writer, providerID, providerEvent) == 0);

	// The record should just be a header (8 bytes)
	uint64_t expectedRecordSizeInWords = 1;
	REQUIRE(buffer.size() == expectedRecordSizeInWords * 8);

	// Validate the header fields
	uint64_t header = ReadUInt64(&buffer[0]);
	INFO("Header: " << std::showbase << std::hex << header);

	// Record type
	REQUIRE(GetFieldFromValue(0, 3, header) == (uint64_t)fxt::RecordType::Metadata);
	// Record size in multiples of uint64_t
	REQUIRE(GetFieldFromValue(4, 15, header) == expectedRecordSizeInWords);
	// Metadata type
	REQUIRE(GetFieldFromValue(16, 19, header) == (uint64_t)fxt::MetadataType::ProviderEvent);
	// Provider ID
	REQUIRE(GetFieldFromValue(20, 51, header) == providerID);
	// Provider event type
	REQUIRE(GetFieldFromValue(52, 55, header) == (uint64_t)providerEvent);
	// The rest should be zero
	REQUIRE(GetFieldFromValue(52, 63, header) == 0);
}

TEST_CASE("TestMagicNumberRecord", "[write]") {
	std::vector<uint8_t> buffer;

	fxt::Writer writer((void *)&buffer, [](void *userContext, const void *data, size_t len) -> int {
		std::vector<uint8_t> *buffer = (std::vector<uint8_t> *)userContext;

		buffer->insert(buffer->end(), (const uint8_t *)data, (const uint8_t *)data + len);
		return 0;
	});

	REQUIRE(fxt::WriteMagicNumberRecord(&writer) == 0);

	// The record should only be the header (8 bytes)
	uint64_t expectedRecordSizeInWords = 1;
	REQUIRE(buffer.size() == expectedRecordSizeInWords * 8);

	// Validate the header fields
	uint64_t header = ReadUInt64(&buffer[0]);
	INFO("Header: " << std::showbase << std::hex << header);

	// Record type
	REQUIRE(GetFieldFromValue(0, 3, header) == (uint64_t)fxt::RecordType::Metadata);
	// Record size in multiples of uint64_t
	REQUIRE(GetFieldFromValue(4, 15, header) == expectedRecordSizeInWords);
	// Metadata type
	REQUIRE(GetFieldFromValue(16, 19, header) == (uint64_t)fxt::MetadataType::TraceInfo);
	// Trace info type
	REQUIRE(GetFieldFromValue(20, 23, header) == (uint64_t)fxt::TraceInfoType::MagicNumberRecord);
	// The actual magic number
	REQUIRE(GetFieldFromValue(24, 55, header) == 0x16547846);
	// The rest should be zero
	REQUIRE(GetFieldFromValue(56, 63, header) == 0);
}

TEST_CASE("TestInitializationRecord", "[write]") {
	std::vector<uint8_t> buffer;

	fxt::Writer writer((void *)&buffer, [](void *userContext, const void *data, size_t len) -> int {
		std::vector<uint8_t> *buffer = (std::vector<uint8_t> *)userContext;

		buffer->insert(buffer->end(), (const uint8_t *)data, (const uint8_t *)data + len);
		return 0;
	});

	const uint64_t numTicksPerSecond = 8723538789381374;

	REQUIRE(fxt::AddInitializationRecord(&writer, numTicksPerSecond) == 0);

	// The record should be the header (8 bytes) plus a uint64 (8 bytes)
	uint64_t expectedRecordSizeInWords = 2;
	REQUIRE(buffer.size() == expectedRecordSizeInWords * 8);

	// Validate the header fields
	uint64_t header = ReadUInt64(&buffer[0]);
	INFO("Header: " << std::showbase << std::hex << header);

	// Record type
	REQUIRE(GetFieldFromValue(0, 3, header) == (uint64_t)fxt::RecordType::Initialization);
	// Record size in multiples of uint64_t
	REQUIRE(GetFieldFromValue(4, 15, header) == expectedRecordSizeInWords);
	// The rest should be zero
	REQUIRE(GetFieldFromValue(16, 63, header) == 0);

	uint64_t actualTicksPerSecond = ReadUInt64(&buffer[8]);
	REQUIRE(actualTicksPerSecond == numTicksPerSecond);
}

TEST_CASE("TestStringRecord", "[write]") {
	std::vector<uint8_t> buffer;

	fxt::Writer writer((void *)&buffer, [](void *userContext, const void *data, size_t len) -> int {
		std::vector<uint8_t> *buffer = (std::vector<uint8_t> *)userContext;

		buffer->insert(buffer->end(), (const uint8_t *)data, (const uint8_t *)data + len);
		return 0;
	});

	const char *expectedStringValue;
	uint64_t stringValueLen;
	uint64_t paddedStrLen;

	SECTION("Uneven string length") {
		// String is not a multiple of 8 bytes
		// Meaning it needs to be padded
		expectedStringValue = "abcdefghi";
		stringValueLen = 9;
		paddedStrLen = 16;
	}
	SECTION("Even multiple string length") {
		// String *is* a multiple of 8 bytes
		// So it does not need to be padded
		expectedStringValue = "abcdefghijklmnopqrstuvwx";
		stringValueLen = 24;
		paddedStrLen = 24;
	}

	uint16_t strIndex;
	REQUIRE(fxt::GetOrCreateStringIndex(&writer, expectedStringValue, &strIndex) == 0);

	// The record should be the header (8 bytes) plus the padded string
	uint64_t expectedRecordSizeInWords = 1 + (paddedStrLen / 8);
	REQUIRE(buffer.size() == expectedRecordSizeInWords * 8);

	// Validate the header fields
	uint64_t header = ReadUInt64(&buffer[0]);
	INFO("Header: " << std::showbase << std::hex << header);

	// Record type
	REQUIRE(GetFieldFromValue(0, 3, header) == (uint64_t)fxt::RecordType::String);
	// Record size in multiples of uint64_t
	REQUIRE(GetFieldFromValue(4, 15, header) == expectedRecordSizeInWords);
	// String index
	REQUIRE(GetFieldFromValue(16, 30, header) == strIndex);
	// Check padding bit
	REQUIRE(GetFieldFromValue(31, 31, header) == 0);
	// String length
	uint64_t actualStringLen = GetFieldFromValue(32, 46, header);
	REQUIRE(actualStringLen == stringValueLen);
	// The rest should be zero
	REQUIRE(GetFieldFromValue(47, 63, header) == 0);

	// Check the string
	std::string actualStringValue(buffer.begin() + 8, buffer.begin() + (8 + stringValueLen));
	REQUIRE_THAT(actualStringValue, Catch::Matchers::Equals(expectedStringValue));

	// Check that the remaining bytes are all zero
	std::vector<uint8_t> expectedZeros(paddedStrLen - stringValueLen, 0);
	std::vector<uint8_t> actualBytes(buffer.begin() + (8 + stringValueLen), buffer.end());
	REQUIRE_THAT(actualBytes, Catch::Matchers::Equals(expectedZeros));
}

TEST_CASE("TestThreadRecord", "[write]") {
	std::vector<uint8_t> buffer;

	fxt::Writer writer((void *)&buffer, [](void *userContext, const void *data, size_t len) -> int {
		std::vector<uint8_t> *buffer = (std::vector<uint8_t> *)userContext;

		buffer->insert(buffer->end(), (const uint8_t *)data, (const uint8_t *)data + len);
		return 0;
	});

	const fxt::KernelObjectID processID = 254895426;
	const fxt::KernelObjectID threadID = 5743738;

	uint16_t threadIndex;
	REQUIRE(fxt::GetOrCreateThreadIndex(&writer, processID, threadID, &threadIndex) == 0);

	// The record should be the header (8 bytes) plus two uint64 (16 bytes)
	uint64_t expectedRecordSizeInWords = 3;
	REQUIRE(buffer.size() == expectedRecordSizeInWords * 8);

	// Validate the header fields
	uint64_t header = ReadUInt64(&buffer[0]);
	INFO("Header: " << std::showbase << std::hex << header);

	// Record type
	REQUIRE(GetFieldFromValue(0, 3, header) == (uint64_t)fxt::RecordType::Thread);
	// Record size in multiples of uint64_t
	REQUIRE(GetFieldFromValue(4, 15, header) == expectedRecordSizeInWords);
	// Thread index
	REQUIRE(GetFieldFromValue(16, 23, header) == threadIndex);
	// The rest should be zero
	REQUIRE(GetFieldFromValue(24, 63, header) == 0);

	uint64_t actualProcessID = ReadUInt64(&buffer[8]);
	REQUIRE(actualProcessID == processID);

	uint64_t actualThreadID = ReadUInt64(&buffer[16]);
	REQUIRE(actualThreadID == threadID);
}

TEST_CASE("TestArguments", "[write]") {
	std::vector<uint8_t> buffer;

	fxt::Writer writer((void *)&buffer, [](void *userContext, const void *data, size_t len) -> int {
		std::vector<uint8_t> *buffer = (std::vector<uint8_t> *)userContext;

		buffer->insert(buffer->end(), (const uint8_t *)data, (const uint8_t *)data + len);
		return 0;
	});

	SECTION("Inline name") {
		const char *argumentName = "myArgumentName";
		const uint64_t argumentNameLen = 14;
		const uint64_t argumentNamePaddedLen = 16;
		// The string ref has the high bit set to signal it's an inline string
		const uint16_t expectedNameStrRef = 0x8000 | argumentNameLen;

		SECTION("Null") {
			// The argument should just be the header (8 bytes) and the padded name (16 bytes)
			uint64_t expectedArgSizeInWords = 3;

			fxt::RecordArgument arg = fxt::RecordArgument(argumentName, fxt::RecordArgumentValue(nullptr));

			fxt::ProcessedRecordArgument processedArg;
			REQUIRE(fxt::ProcessArgs(&writer, &arg, 1, &processedArg) == 0);

			// Validate the processing

			// We're using an inline name string
			REQUIRE(processedArg.nameSizeInWords == argumentNamePaddedLen / 8);
			// Name str ref
			REQUIRE(processedArg.nameStringRef == expectedNameStrRef);
			// Null arg doesn't take up any extra space
			REQUIRE(processedArg.valueSizeInWords == 0);

			// Write the argument
			unsigned int wordsWritten;
			REQUIRE(fxt::WriteArg(&writer, &arg, &processedArg, &wordsWritten) == 0);
			REQUIRE(wordsWritten == expectedArgSizeInWords);

			// Validate the written bytes
			REQUIRE(buffer.size() == expectedArgSizeInWords * 8);

			// Validate the header fields
			uint64_t header = ReadUInt64(&buffer[0]);
			INFO("Header: " << std::showbase << std::hex << header);

			// Arg type
			REQUIRE(GetFieldFromValue(0, 3, header) == (uint64_t)fxt::internal::ArgumentType::Null);
			// Arg size in multiples of uint64_t
			REQUIRE(GetFieldFromValue(4, 15, header) == expectedArgSizeInWords);
			// Arg name string ref
			REQUIRE((uint16_t)GetFieldFromValue(16, 31, header) == expectedNameStrRef);
			// The rest should be zero
			REQUIRE(GetFieldFromValue(32, 63, header) == 0);

			// Check the name string
			std::string actualStringValue(buffer.begin() + 8, buffer.begin() + (8 + argumentNameLen));
			REQUIRE_THAT(actualStringValue, Catch::Matchers::Equals(argumentName));

			// Check that the remaining bytes are all zero
			std::vector<uint8_t> expectedZeros(argumentNamePaddedLen - argumentNameLen, 0);
			std::vector<uint8_t> actualBytes(buffer.begin() + (8 + argumentNameLen), buffer.end());
			REQUIRE_THAT(actualBytes, Catch::Matchers::Equals(expectedZeros));
		}
		SECTION("Int32") {
			// The argument should just be the header (8 bytes) and the padded name (16 bytes)
			uint64_t expectedArgSizeInWords = 3;
			int32_t expectedValue = 3456871;

			fxt::RecordArgument arg = fxt::RecordArgument(argumentName, fxt::RecordArgumentValue(expectedValue));

			fxt::ProcessedRecordArgument processedArg;
			REQUIRE(fxt::ProcessArgs(&writer, &arg, 1, &processedArg) == 0);

			// Validate the processing

			// We're using an inline name string
			REQUIRE(processedArg.nameSizeInWords == argumentNamePaddedLen / 8);
			// Name str ref
			REQUIRE(processedArg.nameStringRef == expectedNameStrRef);
			// Int32 arg doesn't take up any extra space
			REQUIRE(processedArg.valueSizeInWords == 0);

			// Write the argument
			unsigned int wordsWritten;
			REQUIRE(fxt::WriteArg(&writer, &arg, &processedArg, &wordsWritten) == 0);
			REQUIRE(wordsWritten == expectedArgSizeInWords);

			// Validate the written bytes
			REQUIRE(buffer.size() == expectedArgSizeInWords * 8);

			// Validate the header fields
			uint64_t header = ReadUInt64(&buffer[0]);
			INFO("Header: " << std::showbase << std::hex << header);

			// Arg type
			REQUIRE(GetFieldFromValue(0, 3, header) == (uint64_t)fxt::internal::ArgumentType::Int32);
			// Arg size in multiples of uint64_t
			REQUIRE(GetFieldFromValue(4, 15, header) == expectedArgSizeInWords);
			// Arg name string ref
			REQUIRE((uint16_t)GetFieldFromValue(16, 31, header) == expectedNameStrRef);
			// Value
			REQUIRE((int32_t)GetFieldFromValue(32, 63, header) == expectedValue);

			// Check the name string
			std::string actualStringValue(buffer.begin() + 8, buffer.begin() + (8 + argumentNameLen));
			REQUIRE_THAT(actualStringValue, Catch::Matchers::Equals(argumentName));

			// Check that the remaining bytes are all zero
			std::vector<uint8_t> expectedZeros(argumentNamePaddedLen - argumentNameLen, 0);
			std::vector<uint8_t> actualBytes(buffer.begin() + (8 + argumentNameLen), buffer.end());
			REQUIRE_THAT(actualBytes, Catch::Matchers::Equals(expectedZeros));
		}
		SECTION("UInt32") {
			// The argument should just be the header (8 bytes) and the padded name (16 bytes)
			uint64_t expectedArgSizeInWords = 3;
			uint32_t expectedValue = 845914856;

			fxt::RecordArgument arg = fxt::RecordArgument(argumentName, fxt::RecordArgumentValue(expectedValue));

			fxt::ProcessedRecordArgument processedArg;
			REQUIRE(fxt::ProcessArgs(&writer, &arg, 1, &processedArg) == 0);

			// Validate the processing

			// We're using an inline name string
			REQUIRE(processedArg.nameSizeInWords == argumentNamePaddedLen / 8);
			// Name str ref
			REQUIRE(processedArg.nameStringRef == expectedNameStrRef);
			// UInt32 arg doesn't take up any extra space
			REQUIRE(processedArg.valueSizeInWords == 0);

			// Write the argument
			unsigned int wordsWritten;
			REQUIRE(fxt::WriteArg(&writer, &arg, &processedArg, &wordsWritten) == 0);
			REQUIRE(wordsWritten == expectedArgSizeInWords);

			// Validate the written bytes
			REQUIRE(buffer.size() == expectedArgSizeInWords * 8);

			// Validate the header fields
			uint64_t header = ReadUInt64(&buffer[0]);
			INFO("Header: " << std::showbase << std::hex << header);

			// Arg type
			REQUIRE(GetFieldFromValue(0, 3, header) == (uint64_t)fxt::internal::ArgumentType::UInt32);
			// Arg size in multiples of uint64_t
			REQUIRE(GetFieldFromValue(4, 15, header) == expectedArgSizeInWords);
			// Arg name string ref
			REQUIRE((uint16_t)GetFieldFromValue(16, 31, header) == expectedNameStrRef);
			// Value
			REQUIRE((uint32_t)GetFieldFromValue(32, 63, header) == expectedValue);

			// Check the name string
			std::string actualStringValue(buffer.begin() + 8, buffer.begin() + (8 + argumentNameLen));
			REQUIRE_THAT(actualStringValue, Catch::Matchers::Equals(argumentName));

			// Check that the remaining bytes are all zero
			std::vector<uint8_t> expectedZeros(argumentNamePaddedLen - argumentNameLen, 0);
			std::vector<uint8_t> actualBytes(buffer.begin() + (8 + argumentNameLen), buffer.end());
			REQUIRE_THAT(actualBytes, Catch::Matchers::Equals(expectedZeros));
		}
		SECTION("Int64") {
			// The argument should be the header (8 bytes), the padded name (16 bytes), and the value (8 bytes)
			uint64_t expectedArgSizeInWords = 4;
			int64_t expectedValue = 849418487798494;

			fxt::RecordArgument arg = fxt::RecordArgument(argumentName, fxt::RecordArgumentValue(expectedValue));

			fxt::ProcessedRecordArgument processedArg;
			REQUIRE(fxt::ProcessArgs(&writer, &arg, 1, &processedArg) == 0);

			// Validate the processing

			// We're using an inline name string
			REQUIRE(processedArg.nameSizeInWords == argumentNamePaddedLen / 8);
			// Name str ref
			REQUIRE(processedArg.nameStringRef == expectedNameStrRef);
			// Int64 arg takes 1 word
			REQUIRE(processedArg.valueSizeInWords == 1);

			// Write the argument
			unsigned int wordsWritten;
			REQUIRE(fxt::WriteArg(&writer, &arg, &processedArg, &wordsWritten) == 0);
			REQUIRE(wordsWritten == expectedArgSizeInWords);

			// Validate the written bytes
			REQUIRE(buffer.size() == expectedArgSizeInWords * 8);

			// Validate the header fields
			uint64_t header = ReadUInt64(&buffer[0]);
			INFO("Header: " << std::showbase << std::hex << header);

			// Arg type
			REQUIRE(GetFieldFromValue(0, 3, header) == (uint64_t)fxt::internal::ArgumentType::Int64);
			// Arg size in multiples of uint64_t
			REQUIRE(GetFieldFromValue(4, 15, header) == expectedArgSizeInWords);
			// Arg name string ref
			REQUIRE((uint16_t)GetFieldFromValue(16, 31, header) == expectedNameStrRef);
			// The rest should be zero
			REQUIRE(GetFieldFromValue(32, 63, header) == 0);

			// Check the name string
			std::string actualStringValue(buffer.begin() + 8, buffer.begin() + (8 + argumentNameLen));
			REQUIRE_THAT(actualStringValue, Catch::Matchers::Equals(argumentName));

			// Check that the remaining padding bytes are all zero
			size_t argNameEnd = 8 /* header */ + argumentNamePaddedLen;

			std::vector<uint8_t> expectedZeros(argumentNamePaddedLen - argumentNameLen, 0);
			std::vector<uint8_t> actualBytes(buffer.begin() + 8 + argumentNameLen, buffer.begin() + argNameEnd);
			REQUIRE_THAT(actualBytes, Catch::Matchers::Equals(expectedZeros));

			// Read the value
			int64_t value = (int64_t)ReadUInt64(&buffer[argNameEnd]);
			REQUIRE(value == expectedValue);
		}
		SECTION("UInt64") {
			// The argument should be the header (8 bytes), the padded name (16 bytes), and the value (8 bytes)
			uint64_t expectedArgSizeInWords = 4;
			uint64_t expectedValue = 2098732459873498723;

			fxt::RecordArgument arg = fxt::RecordArgument(argumentName, fxt::RecordArgumentValue(expectedValue));

			fxt::ProcessedRecordArgument processedArg;
			REQUIRE(fxt::ProcessArgs(&writer, &arg, 1, &processedArg) == 0);

			// Validate the processing

			// We're using an inline name string
			REQUIRE(processedArg.nameSizeInWords == argumentNamePaddedLen / 8);
			// Name str ref
			REQUIRE(processedArg.nameStringRef == expectedNameStrRef);
			// Int64 arg takes 1 word
			REQUIRE(processedArg.valueSizeInWords == 1);

			// Write the argument
			unsigned int wordsWritten;
			REQUIRE(fxt::WriteArg(&writer, &arg, &processedArg, &wordsWritten) == 0);
			REQUIRE(wordsWritten == expectedArgSizeInWords);

			// Validate the written bytes
			REQUIRE(buffer.size() == expectedArgSizeInWords * 8);

			// Validate the header fields
			uint64_t header = ReadUInt64(&buffer[0]);
			INFO("Header: " << std::showbase << std::hex << header);

			// Arg type
			REQUIRE(GetFieldFromValue(0, 3, header) == (uint64_t)fxt::internal::ArgumentType::UInt64);
			// Arg size in multiples of uint64_t
			REQUIRE(GetFieldFromValue(4, 15, header) == expectedArgSizeInWords);
			// Arg name string ref
			REQUIRE((uint16_t)GetFieldFromValue(16, 31, header) == expectedNameStrRef);
			// The rest should be zero
			REQUIRE(GetFieldFromValue(32, 63, header) == 0);

			// Check the name string
			std::string actualStringValue(buffer.begin() + 8, buffer.begin() + (8 + argumentNameLen));
			REQUIRE_THAT(actualStringValue, Catch::Matchers::Equals(argumentName));

			// Check that the remaining padding bytes are all zero
			size_t argNameEnd = 8 /* header */ + argumentNamePaddedLen;

			std::vector<uint8_t> expectedZeros(argumentNamePaddedLen - argumentNameLen, 0);
			std::vector<uint8_t> actualBytes(buffer.begin() + 8 + argumentNameLen, buffer.begin() + argNameEnd);
			REQUIRE_THAT(actualBytes, Catch::Matchers::Equals(expectedZeros));

			// Read the value
			uint64_t value = ReadUInt64(&buffer[argNameEnd]);
			REQUIRE(value == expectedValue);
		}
		SECTION("Double") {
			// The argument should be the header (8 bytes), the padded name (16 bytes), and the value (8 bytes)
			uint64_t expectedArgSizeInWords = 4;
			double expectedValue = 37728.564373821234;

			fxt::RecordArgument arg = fxt::RecordArgument(argumentName, fxt::RecordArgumentValue(expectedValue));

			fxt::ProcessedRecordArgument processedArg;
			REQUIRE(fxt::ProcessArgs(&writer, &arg, 1, &processedArg) == 0);

			// Validate the processing

			// We're using an inline name string
			REQUIRE(processedArg.nameSizeInWords == argumentNamePaddedLen / 8);
			// Name str ref
			REQUIRE(processedArg.nameStringRef == expectedNameStrRef);
			// Int64 arg takes 1 word
			REQUIRE(processedArg.valueSizeInWords == 1);

			// Write the argument
			unsigned int wordsWritten;
			REQUIRE(fxt::WriteArg(&writer, &arg, &processedArg, &wordsWritten) == 0);
			REQUIRE(wordsWritten == expectedArgSizeInWords);

			// Validate the written bytes
			REQUIRE(buffer.size() == expectedArgSizeInWords * 8);

			// Validate the header fields
			uint64_t header = ReadUInt64(&buffer[0]);
			INFO("Header: " << std::showbase << std::hex << header);

			// Arg type
			REQUIRE(GetFieldFromValue(0, 3, header) == (uint64_t)fxt::internal::ArgumentType::Double);
			// Arg size in multiples of uint64_t
			REQUIRE(GetFieldFromValue(4, 15, header) == expectedArgSizeInWords);
			// Arg name string ref
			REQUIRE((uint16_t)GetFieldFromValue(16, 31, header) == expectedNameStrRef);
			// The rest should be zero
			REQUIRE(GetFieldFromValue(32, 63, header) == 0);

			// Check the name string
			std::string actualStringValue(buffer.begin() + 8, buffer.begin() + (8 + argumentNameLen));
			REQUIRE_THAT(actualStringValue, Catch::Matchers::Equals(argumentName));

			// Check that the remaining padding bytes are all zero
			size_t argNameEnd = 8 /* header */ + argumentNamePaddedLen;

			std::vector<uint8_t> expectedZeros(argumentNamePaddedLen - argumentNameLen, 0);
			std::vector<uint8_t> actualBytes(buffer.begin() + 8 + argumentNameLen, buffer.begin() + argNameEnd);
			REQUIRE_THAT(actualBytes, Catch::Matchers::Equals(expectedZeros));

			// Read the value
			uint64_t rawValue = ReadUInt64(&buffer[argNameEnd]);
			double value;
			memcpy(&value, &rawValue, sizeof(value));
			REQUIRE(value == expectedValue);
		}
		SECTION("String") {
			const char *argumentValue = "myArgValue";
			const uint64_t argumentValueLen = 10;
			const uint64_t argumentValuePaddedLen = 16;

			SECTION("Inline value") {
				// The string ref has the high bit set to signal it's an inline string
				const uint16_t expectedValueStrRef = 0x8000 | argumentValueLen;
				const bool useStrTableForValue = false;

				// The argument should be the header (8 bytes), the padded name (16 bytes), and the padded value (16)
				uint64_t expectedArgSizeInWords = 5;

				fxt::RecordArgument arg = fxt::RecordArgument(argumentName, fxt::RecordArgumentValue(argumentValue, useStrTableForValue));

				fxt::ProcessedRecordArgument processedArg;
				REQUIRE(fxt::ProcessArgs(&writer, &arg, 1, &processedArg) == 0);

				// Validate the processing

				// We're using an inline name string
				REQUIRE(processedArg.nameSizeInWords == argumentNamePaddedLen / 8);
				// Name str ref
				REQUIRE(processedArg.nameStringRef == expectedNameStrRef);
				// We're using an inline value string
				REQUIRE(processedArg.valueSizeInWords == argumentValuePaddedLen / 8);
				// Value str ref
				REQUIRE(processedArg.valueStringRef == expectedValueStrRef);

				// Write the argument
				unsigned int wordsWritten;
				REQUIRE(fxt::WriteArg(&writer, &arg, &processedArg, &wordsWritten) == 0);
				REQUIRE(wordsWritten == expectedArgSizeInWords);

				// Validate the written bytes
				REQUIRE(buffer.size() == expectedArgSizeInWords * 8);
				uint64_t bufferOffset = 0;

				// Validate the header fields
				uint64_t header = ReadUInt64(&buffer[bufferOffset]);
				INFO("Header: " << std::showbase << std::hex << header);
				bufferOffset += 8;

				// Arg type
				REQUIRE(GetFieldFromValue(0, 3, header) == (uint64_t)fxt::internal::ArgumentType::String);
				// Arg size in multiples of uint64_t
				REQUIRE(GetFieldFromValue(4, 15, header) == expectedArgSizeInWords);
				// Arg name string ref
				REQUIRE((uint16_t)GetFieldFromValue(16, 31, header) == expectedNameStrRef);
				// Arg value string ref
				REQUIRE((uint16_t)GetFieldFromValue(32, 47, header) == expectedValueStrRef);
				// The rest should be zero
				REQUIRE(GetFieldFromValue(48, 63, header) == 0);

				// Check the name string
				std::string actualNameStringValue(buffer.begin() + bufferOffset, buffer.begin() + bufferOffset + argumentNameLen);
				REQUIRE_THAT(actualNameStringValue, Catch::Matchers::Equals(argumentName));

				// Check that the remaining padding bytes are all zero
				{
					std::vector<uint8_t> expectedZeros(argumentNamePaddedLen - argumentNameLen, 0);
					std::vector<uint8_t> actualBytes(buffer.begin() + bufferOffset + argumentNameLen, buffer.begin() + bufferOffset + argumentNamePaddedLen);
					REQUIRE_THAT(actualBytes, Catch::Matchers::Equals(expectedZeros));
				}
				bufferOffset += argumentNamePaddedLen;

				// Check the value string
				std::string actualValueStringValue(buffer.begin() + bufferOffset, buffer.begin() + bufferOffset + argumentValueLen);
				REQUIRE_THAT(actualValueStringValue, Catch::Matchers::Equals(argumentValue));

				// Check that the remaining padding bytes are all zero
				{
					std::vector<uint8_t> expectedZeros(argumentValuePaddedLen - argumentValueLen, 0);
					std::vector<uint8_t> actualBytes(buffer.begin() + bufferOffset + argumentValueLen, buffer.begin() + bufferOffset + argumentValuePaddedLen);
					REQUIRE_THAT(actualBytes, Catch::Matchers::Equals(expectedZeros));
				}
				bufferOffset += argumentValuePaddedLen;

				// Check that we read everything
				REQUIRE(bufferOffset == buffer.size());
			}
			SECTION("String table value") {
				// Index 1 in the table (we start from 1. Zero is a special value)
				const uint16_t expectedValueStrRef = 1;
				const bool useStrTableForValue = true;

				// The argument should be the header (8 bytes) and the padded name (16 bytes)
				uint64_t expectedArgSizeInWords = 3;

				fxt::RecordArgument arg = fxt::RecordArgument(argumentName, fxt::RecordArgumentValue(argumentValue, useStrTableForValue));

				fxt::ProcessedRecordArgument processedArg;
				REQUIRE(fxt::ProcessArgs(&writer, &arg, 1, &processedArg) == 0);

				// Validate the processing

				// We're using an inline name string
				REQUIRE(processedArg.nameSizeInWords == argumentNamePaddedLen / 8);
				// Name str ref
				REQUIRE(processedArg.nameStringRef == expectedNameStrRef);
				// We're using string table value
				REQUIRE(processedArg.valueSizeInWords == 0);
				// Value str ref
				REQUIRE(processedArg.valueStringRef == expectedValueStrRef);

				// Write the argument
				unsigned int wordsWritten;
				REQUIRE(fxt::WriteArg(&writer, &arg, &processedArg, &wordsWritten) == 0);
				REQUIRE(wordsWritten == expectedArgSizeInWords);

				// Validate the written bytes
				size_t bufferOffset = 0;

				// We write a string table record before the argument data,
				// which is the header (8 bytes) and the padded value (16 bytes)
				uint64_t expectedStringRecordSizeInWords = 3;
				REQUIRE(buffer.size() == (expectedStringRecordSizeInWords + expectedArgSizeInWords) * 8);

				// Validate the string table record
				bufferOffset += ValidateStringTableRecord(&buffer[0], argumentValue, expectedValueStrRef);

				// Validate the argument record
				{
					// Validate the header fields
					uint64_t header = ReadUInt64(&buffer[bufferOffset]);
					INFO("Header: " << std::showbase << std::hex << header);
					bufferOffset += 8;

					// Arg type
					REQUIRE(GetFieldFromValue(0, 3, header) == (uint64_t)fxt::internal::ArgumentType::String);
					// Arg size in multiples of uint64_t
					REQUIRE(GetFieldFromValue(4, 15, header) == expectedArgSizeInWords);
					// Arg name string ref
					REQUIRE((uint16_t)GetFieldFromValue(16, 31, header) == expectedNameStrRef);
					// Arg value string ref
					REQUIRE((uint16_t)GetFieldFromValue(32, 47, header) == expectedValueStrRef);
					// The rest should be zero
					REQUIRE(GetFieldFromValue(48, 63, header) == 0);

					// Check the name string
					std::string actualNameStringValue(buffer.begin() + bufferOffset, buffer.begin() + bufferOffset + argumentNameLen);
					REQUIRE_THAT(actualNameStringValue, Catch::Matchers::Equals(argumentName));

					std::vector<uint8_t> expectedZeros(argumentNamePaddedLen - argumentNameLen, 0);
					std::vector<uint8_t> actualBytes(buffer.begin() + bufferOffset + argumentNameLen, buffer.begin() + bufferOffset + argumentNamePaddedLen);
					REQUIRE_THAT(actualBytes, Catch::Matchers::Equals(expectedZeros));

					bufferOffset += argumentNamePaddedLen;
				}

				// Check that we read everything
				REQUIRE(bufferOffset == buffer.size());
			}
		}
		SECTION("Pointer") {
			// The argument should be the header (8 bytes), the padded name (16 bytes), and the value (8 bytes)
			uint64_t expectedArgSizeInWords = 4;
			int *expectedValue = (int *)0x5673738495;

			fxt::RecordArgument arg = fxt::RecordArgument(argumentName, fxt::RecordArgumentValue(expectedValue));

			fxt::ProcessedRecordArgument processedArg;
			REQUIRE(fxt::ProcessArgs(&writer, &arg, 1, &processedArg) == 0);

			// Validate the processing

			// We're using an inline name string
			REQUIRE(processedArg.nameSizeInWords == argumentNamePaddedLen / 8);
			// Name str ref
			REQUIRE(processedArg.nameStringRef == expectedNameStrRef);
			// Int64 arg takes 1 word
			REQUIRE(processedArg.valueSizeInWords == 1);

			// Write the argument
			unsigned int wordsWritten;
			REQUIRE(fxt::WriteArg(&writer, &arg, &processedArg, &wordsWritten) == 0);
			REQUIRE(wordsWritten == expectedArgSizeInWords);

			// Validate the written bytes
			REQUIRE(buffer.size() == expectedArgSizeInWords * 8);

			// Validate the header fields
			uint64_t header = ReadUInt64(&buffer[0]);
			INFO("Header: " << std::showbase << std::hex << header);

			// Arg type
			REQUIRE(GetFieldFromValue(0, 3, header) == (uint64_t)fxt::internal::ArgumentType::Pointer);
			// Arg size in multiples of uint64_t
			REQUIRE(GetFieldFromValue(4, 15, header) == expectedArgSizeInWords);
			// Arg name string ref
			REQUIRE((uint16_t)GetFieldFromValue(16, 31, header) == expectedNameStrRef);
			// The rest should be zero
			REQUIRE(GetFieldFromValue(32, 63, header) == 0);

			// Check the name string
			std::string actualStringValue(buffer.begin() + 8, buffer.begin() + (8 + argumentNameLen));
			REQUIRE_THAT(actualStringValue, Catch::Matchers::Equals(argumentName));

			// Check that the remaining padding bytes are all zero
			size_t argNameEnd = 8 /* header */ + argumentNamePaddedLen;

			std::vector<uint8_t> expectedZeros(argumentNamePaddedLen - argumentNameLen, 0);
			std::vector<uint8_t> actualBytes(buffer.begin() + 8 + argumentNameLen, buffer.begin() + argNameEnd);
			REQUIRE_THAT(actualBytes, Catch::Matchers::Equals(expectedZeros));

			// Read the value
			int *value = (int *)ReadUInt64(&buffer[argNameEnd]);
			REQUIRE(value == expectedValue);
		}
		SECTION("KOID") {
			// The argument should be the header (8 bytes), the padded name (16 bytes), and the value (8 bytes)
			uint64_t expectedArgSizeInWords = 4;
			fxt::KernelObjectID expectedValue = 8498156988;

			fxt::RecordArgument arg = fxt::RecordArgument(argumentName, fxt::RecordArgumentValue::KOID(expectedValue));

			fxt::ProcessedRecordArgument processedArg;
			REQUIRE(fxt::ProcessArgs(&writer, &arg, 1, &processedArg) == 0);

			// Validate the processing

			// We're using an inline name string
			REQUIRE(processedArg.nameSizeInWords == argumentNamePaddedLen / 8);
			// Name str ref
			REQUIRE(processedArg.nameStringRef == expectedNameStrRef);
			// Int64 arg takes 1 word
			REQUIRE(processedArg.valueSizeInWords == 1);

			// Write the argument
			unsigned int wordsWritten;
			REQUIRE(fxt::WriteArg(&writer, &arg, &processedArg, &wordsWritten) == 0);
			REQUIRE(wordsWritten == expectedArgSizeInWords);

			// Validate the written bytes
			REQUIRE(buffer.size() == expectedArgSizeInWords * 8);

			// Validate the header fields
			uint64_t header = ReadUInt64(&buffer[0]);
			INFO("Header: " << std::showbase << std::hex << header);

			// Arg type
			REQUIRE(GetFieldFromValue(0, 3, header) == (uint64_t)fxt::internal::ArgumentType::KOID);
			// Arg size in multiples of uint64_t
			REQUIRE(GetFieldFromValue(4, 15, header) == expectedArgSizeInWords);
			// Arg name string ref
			REQUIRE((uint16_t)GetFieldFromValue(16, 31, header) == expectedNameStrRef);
			// The rest should be zero
			REQUIRE(GetFieldFromValue(32, 63, header) == 0);

			// Check the name string
			std::string actualStringValue(buffer.begin() + 8, buffer.begin() + (8 + argumentNameLen));
			REQUIRE_THAT(actualStringValue, Catch::Matchers::Equals(argumentName));

			// Check that the remaining padding bytes are all zero
			size_t argNameEnd = 8 /* header */ + argumentNamePaddedLen;

			std::vector<uint8_t> expectedZeros(argumentNamePaddedLen - argumentNameLen, 0);
			std::vector<uint8_t> actualBytes(buffer.begin() + 8 + argumentNameLen, buffer.begin() + argNameEnd);
			REQUIRE_THAT(actualBytes, Catch::Matchers::Equals(expectedZeros));

			// Read the value
			fxt::KernelObjectID value = (fxt::KernelObjectID)ReadUInt64(&buffer[argNameEnd]);
			REQUIRE(value == expectedValue);
		}
		SECTION("Bool") {
			// The argument should just be the header (8 bytes) and the padded name (16 bytes)
			uint64_t expectedArgSizeInWords = 3;
			bool expectedValue = true;

			fxt::RecordArgument arg = fxt::RecordArgument(argumentName, fxt::RecordArgumentValue(expectedValue));

			fxt::ProcessedRecordArgument processedArg;
			REQUIRE(fxt::ProcessArgs(&writer, &arg, 1, &processedArg) == 0);

			// Validate the processing

			// We're using an inline name string
			REQUIRE(processedArg.nameSizeInWords == argumentNamePaddedLen / 8);
			// Name str ref
			REQUIRE(processedArg.nameStringRef == expectedNameStrRef);
			// Bool arg doesn't take up any extra space
			REQUIRE(processedArg.valueSizeInWords == 0);

			// Write the argument
			unsigned int wordsWritten;
			REQUIRE(fxt::WriteArg(&writer, &arg, &processedArg, &wordsWritten) == 0);
			REQUIRE(wordsWritten == expectedArgSizeInWords);

			// Validate the written bytes
			REQUIRE(buffer.size() == expectedArgSizeInWords * 8);

			// Validate the header fields
			uint64_t header = ReadUInt64(&buffer[0]);
			INFO("Header: " << std::showbase << std::hex << header);

			// Arg type
			REQUIRE(GetFieldFromValue(0, 3, header) == (uint64_t)fxt::internal::ArgumentType::Bool);
			// Arg size in multiples of uint64_t
			REQUIRE(GetFieldFromValue(4, 15, header) == expectedArgSizeInWords);
			// Arg name string ref
			REQUIRE((uint16_t)GetFieldFromValue(16, 31, header) == expectedNameStrRef);
			// Bool value is stored in a single bit
			REQUIRE((uint8_t)GetFieldFromValue(32, 32, header) == (expectedValue ? 1 : 0));
			// The rest should be zero
			REQUIRE(GetFieldFromValue(33, 63, header) == 0);

			// Check the name string
			std::string actualStringValue(buffer.begin() + 8, buffer.begin() + (8 + argumentNameLen));
			REQUIRE_THAT(actualStringValue, Catch::Matchers::Equals(argumentName));

			// Check that the remaining bytes are all zero
			std::vector<uint8_t> expectedZeros(argumentNamePaddedLen - argumentNameLen, 0);
			std::vector<uint8_t> actualBytes(buffer.begin() + (8 + argumentNameLen), buffer.end());
			REQUIRE_THAT(actualBytes, Catch::Matchers::Equals(expectedZeros));
		}
	}
	SECTION("String table name") {
		const char *argumentName = "myArgumentName";
		const uint64_t argumentNameLen = 14;
		const uint64_t argumentNamePaddedLen = 16;
		// This will be the first string record (index 0 is a special value)
		const uint16_t expectedNameStrRef = 1;
		const bool useStrTableForName = true;
		const fxt::RecordArgumentName argumentNameValue = fxt::RecordArgumentName(argumentName, useStrTableForName);

		SECTION("Null") {
			// The argument should just be the header (8 bytes)
			uint64_t expectedArgSizeInWords = 1;

			fxt::RecordArgument arg = fxt::RecordArgument(argumentNameValue, fxt::RecordArgumentValue(nullptr));

			fxt::ProcessedRecordArgument processedArg;
			REQUIRE(fxt::ProcessArgs(&writer, &arg, 1, &processedArg) == 0);

			// Validate the processing

			// We're using a string table name. So this is zero
			REQUIRE(processedArg.nameSizeInWords == 0);
			// Name str ref
			REQUIRE(processedArg.nameStringRef == expectedNameStrRef);
			// Null arg doesn't take up any extra space
			REQUIRE(processedArg.valueSizeInWords == 0);

			// Write the argument
			unsigned int wordsWritten;
			REQUIRE(fxt::WriteArg(&writer, &arg, &processedArg, &wordsWritten) == 0);
			REQUIRE(wordsWritten == expectedArgSizeInWords);

			// Validate the written bytes
			uint64_t bufferOffset = 0;

			// The name string record is first
			bufferOffset += ValidateStringTableRecord(&buffer[bufferOffset], argumentName, expectedNameStrRef);

			// Now we can validate the argument data itself
			{
				// Validate the header fields
				uint64_t header = ReadUInt64(&buffer[bufferOffset]);
				INFO("Header: " << std::showbase << std::hex << header);
				bufferOffset += 8;

				// Arg type
				REQUIRE(GetFieldFromValue(0, 3, header) == (uint64_t)fxt::internal::ArgumentType::Null);
				// Arg size in multiples of uint64_t
				REQUIRE(GetFieldFromValue(4, 15, header) == expectedArgSizeInWords);
				// Arg name string ref
				REQUIRE((uint16_t)GetFieldFromValue(16, 31, header) == expectedNameStrRef);
				// The rest should be zero
				REQUIRE(GetFieldFromValue(32, 63, header) == 0);
			}

			// Check that we read everything
			REQUIRE(bufferOffset == buffer.size());
		}
		SECTION("Int32") {
			// The argument should just be the header (8 bytes)
			uint64_t expectedArgSizeInWords = 1;
			int32_t expectedValue = 4373738;

			fxt::RecordArgument arg = fxt::RecordArgument(argumentNameValue, fxt::RecordArgumentValue(expectedValue));

			fxt::ProcessedRecordArgument processedArg;
			REQUIRE(fxt::ProcessArgs(&writer, &arg, 1, &processedArg) == 0);

			// Validate the processing

			// We're using a string table name. So this is zero
			REQUIRE(processedArg.nameSizeInWords == 0);
			// Name str ref
			REQUIRE(processedArg.nameStringRef == expectedNameStrRef);
			// Int32 arg doesn't take up any extra space
			REQUIRE(processedArg.valueSizeInWords == 0);

			// Write the argument
			unsigned int wordsWritten;
			REQUIRE(fxt::WriteArg(&writer, &arg, &processedArg, &wordsWritten) == 0);
			REQUIRE(wordsWritten == expectedArgSizeInWords);

			// Validate the written bytes
			uint64_t bufferOffset = 0;

			// The name string record is first
			bufferOffset += ValidateStringTableRecord(&buffer[bufferOffset], argumentName, expectedNameStrRef);

			// Now we can validate the argument data itself
			{
				// Validate the header fields
				uint64_t header = ReadUInt64(&buffer[bufferOffset]);
				INFO("Header: " << std::showbase << std::hex << header);
				bufferOffset += 8;

				// Arg type
				REQUIRE(GetFieldFromValue(0, 3, header) == (uint64_t)fxt::internal::ArgumentType::Int32);
				// Arg size in multiples of uint64_t
				REQUIRE(GetFieldFromValue(4, 15, header) == expectedArgSizeInWords);
				// Arg name string ref
				REQUIRE((uint16_t)GetFieldFromValue(16, 31, header) == expectedNameStrRef);
				// Check the value
				REQUIRE((int32_t)GetFieldFromValue(32, 63, header) == expectedValue);
			}

			// Check that we read everything
			REQUIRE(bufferOffset == buffer.size());
		}
		SECTION("UInt32") {
			// The argument should just be the header (8 bytes)
			uint64_t expectedArgSizeInWords = 1;
			uint32_t expectedValue = 97949649;

			fxt::RecordArgument arg = fxt::RecordArgument(argumentNameValue, fxt::RecordArgumentValue(expectedValue));

			fxt::ProcessedRecordArgument processedArg;
			REQUIRE(fxt::ProcessArgs(&writer, &arg, 1, &processedArg) == 0);

			// Validate the processing

			// We're using a string table name. So this is zero
			REQUIRE(processedArg.nameSizeInWords == 0);
			// Name str ref
			REQUIRE(processedArg.nameStringRef == expectedNameStrRef);
			// UInt32 arg doesn't take up any extra space
			REQUIRE(processedArg.valueSizeInWords == 0);

			// Write the argument
			unsigned int wordsWritten;
			REQUIRE(fxt::WriteArg(&writer, &arg, &processedArg, &wordsWritten) == 0);
			REQUIRE(wordsWritten == expectedArgSizeInWords);

			// Validate the written bytes
			uint64_t bufferOffset = 0;

			// The name string record is first
			bufferOffset += ValidateStringTableRecord(&buffer[bufferOffset], argumentName, expectedNameStrRef);

			// Now we can validate the argument data itself
			{
				// Validate the header fields
				uint64_t header = ReadUInt64(&buffer[bufferOffset]);
				INFO("Header: " << std::showbase << std::hex << header);
				bufferOffset += 8;

				// Arg type
				REQUIRE(GetFieldFromValue(0, 3, header) == (uint64_t)fxt::internal::ArgumentType::UInt32);
				// Arg size in multiples of uint64_t
				REQUIRE(GetFieldFromValue(4, 15, header) == expectedArgSizeInWords);
				// Arg name string ref
				REQUIRE((uint16_t)GetFieldFromValue(16, 31, header) == expectedNameStrRef);
				// Check the value
				REQUIRE((uint32_t)GetFieldFromValue(32, 63, header) == expectedValue);
			}

			// Check that we read everything
			REQUIRE(bufferOffset == buffer.size());
		}
		SECTION("Int64") {
			// The argument should just be the header (8 bytes) plus the value (8 bytes)
			uint64_t expectedArgSizeInWords = 2;
			int64_t expectedValue = 8978449955885;

			fxt::RecordArgument arg = fxt::RecordArgument(argumentNameValue, fxt::RecordArgumentValue(expectedValue));

			fxt::ProcessedRecordArgument processedArg;
			REQUIRE(fxt::ProcessArgs(&writer, &arg, 1, &processedArg) == 0);

			// Validate the processing

			// We're using a string table name. So this is zero
			REQUIRE(processedArg.nameSizeInWords == 0);
			// Name str ref
			REQUIRE(processedArg.nameStringRef == expectedNameStrRef);
			// Int64 arg takes 1 word
			REQUIRE(processedArg.valueSizeInWords == 1);

			// Write the argument
			unsigned int wordsWritten;
			REQUIRE(fxt::WriteArg(&writer, &arg, &processedArg, &wordsWritten) == 0);
			REQUIRE(wordsWritten == expectedArgSizeInWords);

			// Validate the written bytes
			uint64_t bufferOffset = 0;

			// The name string record is first
			bufferOffset += ValidateStringTableRecord(&buffer[bufferOffset], argumentName, expectedNameStrRef);

			// Now we can validate the argument data itself
			{
				// Validate the header fields
				uint64_t header = ReadUInt64(&buffer[bufferOffset]);
				INFO("Header: " << std::showbase << std::hex << header);
				bufferOffset += 8;

				// Arg type
				REQUIRE(GetFieldFromValue(0, 3, header) == (uint64_t)fxt::internal::ArgumentType::Int64);
				// Arg size in multiples of uint64_t
				REQUIRE(GetFieldFromValue(4, 15, header) == expectedArgSizeInWords);
				// Arg name string ref
				REQUIRE((uint16_t)GetFieldFromValue(16, 31, header) == expectedNameStrRef);
				// The rest should be zero
				REQUIRE(GetFieldFromValue(32, 63, header) == 0);

				// Read the value
				int64_t value = (int64_t)ReadUInt64(&buffer[bufferOffset]);
				bufferOffset += 8;
				REQUIRE(value == expectedValue);
			}

			// Check that we read everything
			REQUIRE(bufferOffset == buffer.size());
		}
		SECTION("UInt64") {
			// The argument should just be the header (8 bytes) plus the value (8 bytes)
			uint64_t expectedArgSizeInWords = 2;
			uint64_t expectedValue = 291849871298733;

			fxt::RecordArgument arg = fxt::RecordArgument(argumentNameValue, fxt::RecordArgumentValue(expectedValue));

			fxt::ProcessedRecordArgument processedArg;
			REQUIRE(fxt::ProcessArgs(&writer, &arg, 1, &processedArg) == 0);

			// Validate the processing

			// We're using a string table name. So this is zero
			REQUIRE(processedArg.nameSizeInWords == 0);
			// Name str ref
			REQUIRE(processedArg.nameStringRef == expectedNameStrRef);
			// UInt64 arg takes 1 word
			REQUIRE(processedArg.valueSizeInWords == 1);

			// Write the argument
			unsigned int wordsWritten;
			REQUIRE(fxt::WriteArg(&writer, &arg, &processedArg, &wordsWritten) == 0);
			REQUIRE(wordsWritten == expectedArgSizeInWords);

			// Validate the written bytes
			uint64_t bufferOffset = 0;

			// The name string record is first
			bufferOffset += ValidateStringTableRecord(&buffer[bufferOffset], argumentName, expectedNameStrRef);

			// Now we can validate the argument data itself
			{
				// Validate the header fields
				uint64_t header = ReadUInt64(&buffer[bufferOffset]);
				INFO("Header: " << std::showbase << std::hex << header);
				bufferOffset += 8;

				// Arg type
				REQUIRE(GetFieldFromValue(0, 3, header) == (uint64_t)fxt::internal::ArgumentType::UInt64);
				// Arg size in multiples of uint64_t
				REQUIRE(GetFieldFromValue(4, 15, header) == expectedArgSizeInWords);
				// Arg name string ref
				REQUIRE((uint16_t)GetFieldFromValue(16, 31, header) == expectedNameStrRef);
				// The rest should be zero
				REQUIRE(GetFieldFromValue(32, 63, header) == 0);

				// Read the value
				uint64_t value = (uint64_t)ReadUInt64(&buffer[bufferOffset]);
				bufferOffset += 8;
				REQUIRE(value == expectedValue);
			}

			// Check that we read everything
			REQUIRE(bufferOffset == buffer.size());
		}
		SECTION("Double") {
			// The argument should just be the header (8 bytes) plus the value (8 bytes)
			uint64_t expectedArgSizeInWords = 2;
			double expectedValue = 84152.5489512;

			fxt::RecordArgument arg = fxt::RecordArgument(argumentNameValue, fxt::RecordArgumentValue(expectedValue));

			fxt::ProcessedRecordArgument processedArg;
			REQUIRE(fxt::ProcessArgs(&writer, &arg, 1, &processedArg) == 0);

			// Validate the processing

			// We're using a string table name. So this is zero
			REQUIRE(processedArg.nameSizeInWords == 0);
			// Name str ref
			REQUIRE(processedArg.nameStringRef == expectedNameStrRef);
			// Double arg takes 1 word
			REQUIRE(processedArg.valueSizeInWords == 1);

			// Write the argument
			unsigned int wordsWritten;
			REQUIRE(fxt::WriteArg(&writer, &arg, &processedArg, &wordsWritten) == 0);
			REQUIRE(wordsWritten == expectedArgSizeInWords);

			// Validate the written bytes
			uint64_t bufferOffset = 0;

			// The name string record is first
			bufferOffset += ValidateStringTableRecord(&buffer[bufferOffset], argumentName, expectedNameStrRef);

			// Now we can validate the argument data itself
			{
				// Validate the header fields
				uint64_t header = ReadUInt64(&buffer[bufferOffset]);
				INFO("Header: " << std::showbase << std::hex << header);
				bufferOffset += 8;

				// Arg type
				REQUIRE(GetFieldFromValue(0, 3, header) == (uint64_t)fxt::internal::ArgumentType::Double);
				// Arg size in multiples of uint64_t
				REQUIRE(GetFieldFromValue(4, 15, header) == expectedArgSizeInWords);
				// Arg name string ref
				REQUIRE((uint16_t)GetFieldFromValue(16, 31, header) == expectedNameStrRef);
				// The rest should be zero
				REQUIRE(GetFieldFromValue(32, 63, header) == 0);

				// Read the value
				uint64_t rawValue = ReadUInt64(&buffer[bufferOffset]);
				bufferOffset += 8;
				double value;
				memcpy(&value, &rawValue, sizeof(value));
				REQUIRE(value == expectedValue);
			}

			// Check that we read everything
			REQUIRE(bufferOffset == buffer.size());
		}
		SECTION("String") {
			const char *argumentValue = "myArgValue";
			const uint64_t argumentValueLen = 10;
			const uint64_t argumentValuePaddedLen = 16;

			SECTION("Inline value") {
				// The string ref has the high bit set to signal it's an inline string
				const uint16_t expectedValueStrRef = 0x8000 | argumentValueLen;
				const bool useStrTableForValue = false;

				// The argument should just be the header (8 bytes) plus the padded value (16 bytes)
				uint64_t expectedArgSizeInWords = 3;

				fxt::RecordArgument arg = fxt::RecordArgument(argumentNameValue, fxt::RecordArgumentValue(argumentValue, useStrTableForValue));

				fxt::ProcessedRecordArgument processedArg;
				REQUIRE(fxt::ProcessArgs(&writer, &arg, 1, &processedArg) == 0);

				// Validate the processing

				// We're using a string table name. So this is zero
				REQUIRE(processedArg.nameSizeInWords == 0);
				// Name str ref
				REQUIRE(processedArg.nameStringRef == expectedNameStrRef);
				// We're using an inline value string
				REQUIRE(processedArg.valueSizeInWords == argumentValuePaddedLen / 8);
				// Value str ref
				REQUIRE(processedArg.valueStringRef == expectedValueStrRef);

				// Write the argument
				unsigned int wordsWritten;
				REQUIRE(fxt::WriteArg(&writer, &arg, &processedArg, &wordsWritten) == 0);
				REQUIRE(wordsWritten == expectedArgSizeInWords);

				// Validate the written bytes
				uint64_t bufferOffset = 0;

				// The name string record is first
				bufferOffset += ValidateStringTableRecord(&buffer[bufferOffset], argumentName, expectedNameStrRef);

				// Now we can validate the argument data itself
				{
					// Validate the header fields
					uint64_t header = ReadUInt64(&buffer[bufferOffset]);
					INFO("Header: " << std::showbase << std::hex << header);
					bufferOffset += 8;

					// Arg type
					REQUIRE(GetFieldFromValue(0, 3, header) == (uint64_t)fxt::internal::ArgumentType::String);
					// Arg size in multiples of uint64_t
					REQUIRE(GetFieldFromValue(4, 15, header) == expectedArgSizeInWords);
					// Arg name string ref
					REQUIRE((uint16_t)GetFieldFromValue(16, 31, header) == expectedNameStrRef);
					// Arg value string ref
					REQUIRE((uint16_t)GetFieldFromValue(32, 47, header) == expectedValueStrRef);
					// The rest should be zero
					REQUIRE(GetFieldFromValue(48, 63, header) == 0);

					// Check the value string
					std::string actualValueStringValue(buffer.begin() + bufferOffset, buffer.begin() + bufferOffset + argumentValueLen);
					REQUIRE_THAT(actualValueStringValue, Catch::Matchers::Equals(argumentValue));

					// Check that the remaining padding bytes are all zero
					{
						std::vector<uint8_t> expectedZeros(argumentValuePaddedLen - argumentValueLen, 0);
						std::vector<uint8_t> actualBytes(buffer.begin() + bufferOffset + argumentValueLen, buffer.begin() + bufferOffset + argumentValuePaddedLen);
						REQUIRE_THAT(actualBytes, Catch::Matchers::Equals(expectedZeros));
					}
					bufferOffset += argumentValuePaddedLen;
				}

				// Check that we read everything
				REQUIRE(bufferOffset == buffer.size());
			}
			SECTION("String table value") {
				// Index 2 in the table. The name string is index 1
				const uint16_t expectedValueStrRef = 2;
				const bool useStrTableForValue = true;

				// The argument should just be the header (8 bytes)
				uint64_t expectedArgSizeInWords = 1;

				fxt::RecordArgument arg = fxt::RecordArgument(argumentNameValue, fxt::RecordArgumentValue(argumentValue, useStrTableForValue));

				fxt::ProcessedRecordArgument processedArg;
				REQUIRE(fxt::ProcessArgs(&writer, &arg, 1, &processedArg) == 0);

				// Validate the processing

				// We're using a string table name. So this is zero
				REQUIRE(processedArg.nameSizeInWords == 0);
				// Name str ref
				REQUIRE(processedArg.nameStringRef == expectedNameStrRef);
				// We're using a string table value. So this is zero
				REQUIRE(processedArg.valueSizeInWords == 0);
				// Value str ref
				REQUIRE(processedArg.valueStringRef == expectedValueStrRef);

				// Write the argument
				unsigned int wordsWritten;
				REQUIRE(fxt::WriteArg(&writer, &arg, &processedArg, &wordsWritten) == 0);
				REQUIRE(wordsWritten == expectedArgSizeInWords);

				// Validate the written bytes
				uint64_t bufferOffset = 0;

				// The name string record is first
				bufferOffset += ValidateStringTableRecord(&buffer[bufferOffset], argumentName, expectedNameStrRef);

				// Then the value string record
				bufferOffset += ValidateStringTableRecord(&buffer[bufferOffset], argumentValue, expectedValueStrRef);

				// Now we can validate the argument data itself
				{
					// Validate the header fields
					uint64_t header = ReadUInt64(&buffer[bufferOffset]);
					INFO("Header: " << std::showbase << std::hex << header);
					bufferOffset += 8;

					// Arg type
					REQUIRE(GetFieldFromValue(0, 3, header) == (uint64_t)fxt::internal::ArgumentType::String);
					// Arg size in multiples of uint64_t
					REQUIRE(GetFieldFromValue(4, 15, header) == expectedArgSizeInWords);
					// Arg name string ref
					REQUIRE((uint16_t)GetFieldFromValue(16, 31, header) == expectedNameStrRef);
					// Arg value string ref
					REQUIRE((uint16_t)GetFieldFromValue(32, 47, header) == expectedValueStrRef);
					// The rest should be zero
					REQUIRE(GetFieldFromValue(48, 63, header) == 0);
				}

				// Check that we read everything
				REQUIRE(bufferOffset == buffer.size());
			}
		}
		SECTION("Pointer") {
			// The argument should just be the header (8 bytes) plus the value (8 bytes)
			uint64_t expectedArgSizeInWords = 2;
			int *expectedValue = (int *)0x35457373;

			fxt::RecordArgument arg = fxt::RecordArgument(argumentNameValue, fxt::RecordArgumentValue(expectedValue));

			fxt::ProcessedRecordArgument processedArg;
			REQUIRE(fxt::ProcessArgs(&writer, &arg, 1, &processedArg) == 0);

			// Validate the processing

			// We're using a string table name. So this is zero
			REQUIRE(processedArg.nameSizeInWords == 0);
			// Name str ref
			REQUIRE(processedArg.nameStringRef == expectedNameStrRef);
			// Pointer arg takes 1 word
			REQUIRE(processedArg.valueSizeInWords == 1);

			// Write the argument
			unsigned int wordsWritten;
			REQUIRE(fxt::WriteArg(&writer, &arg, &processedArg, &wordsWritten) == 0);
			REQUIRE(wordsWritten == expectedArgSizeInWords);

			// Validate the written bytes
			uint64_t bufferOffset = 0;

			// The name string record is first
			bufferOffset += ValidateStringTableRecord(&buffer[bufferOffset], argumentName, expectedNameStrRef);

			// Now we can validate the argument data itself
			{
				// Validate the header fields
				uint64_t header = ReadUInt64(&buffer[bufferOffset]);
				INFO("Header: " << std::showbase << std::hex << header);
				bufferOffset += 8;

				// Arg type
				REQUIRE(GetFieldFromValue(0, 3, header) == (uint64_t)fxt::internal::ArgumentType::Pointer);
				// Arg size in multiples of uint64_t
				REQUIRE(GetFieldFromValue(4, 15, header) == expectedArgSizeInWords);
				// Arg name string ref
				REQUIRE((uint16_t)GetFieldFromValue(16, 31, header) == expectedNameStrRef);
				// The rest should be zero
				REQUIRE(GetFieldFromValue(32, 63, header) == 0);

				// Read the value
				int *value = (int *)ReadUInt64(&buffer[bufferOffset]);
				bufferOffset += 8;
				REQUIRE(value == expectedValue);
			}

			// Check that we read everything
			REQUIRE(bufferOffset == buffer.size());
		}
		SECTION("KOID") {
			// The argument should just be the header (8 bytes) plus the value (8 bytes)
			uint64_t expectedArgSizeInWords = 2;
			fxt::KernelObjectID expectedValue = 8383929394;

			fxt::RecordArgument arg = fxt::RecordArgument(argumentNameValue, fxt::RecordArgumentValue::KOID(expectedValue));

			fxt::ProcessedRecordArgument processedArg;
			REQUIRE(fxt::ProcessArgs(&writer, &arg, 1, &processedArg) == 0);

			// Validate the processing

			// We're using a string table name. So this is zero
			REQUIRE(processedArg.nameSizeInWords == 0);
			// Name str ref
			REQUIRE(processedArg.nameStringRef == expectedNameStrRef);
			// KOID arg takes 1 word
			REQUIRE(processedArg.valueSizeInWords == 1);

			// Write the argument
			unsigned int wordsWritten;
			REQUIRE(fxt::WriteArg(&writer, &arg, &processedArg, &wordsWritten) == 0);
			REQUIRE(wordsWritten == expectedArgSizeInWords);

			// Validate the written bytes
			uint64_t bufferOffset = 0;

			// The name string record is first
			bufferOffset += ValidateStringTableRecord(&buffer[bufferOffset], argumentName, expectedNameStrRef);

			// Now we can validate the argument data itself
			{
				// Validate the header fields
				uint64_t header = ReadUInt64(&buffer[bufferOffset]);
				INFO("Header: " << std::showbase << std::hex << header);
				bufferOffset += 8;

				// Arg type
				REQUIRE(GetFieldFromValue(0, 3, header) == (uint64_t)fxt::internal::ArgumentType::KOID);
				// Arg size in multiples of uint64_t
				REQUIRE(GetFieldFromValue(4, 15, header) == expectedArgSizeInWords);
				// Arg name string ref
				REQUIRE((uint16_t)GetFieldFromValue(16, 31, header) == expectedNameStrRef);
				// The rest should be zero
				REQUIRE(GetFieldFromValue(32, 63, header) == 0);

				// Read the value
				fxt::KernelObjectID value = (fxt::KernelObjectID)ReadUInt64(&buffer[bufferOffset]);
				bufferOffset += 8;
				REQUIRE(value == expectedValue);
			}

			// Check that we read everything
			REQUIRE(bufferOffset == buffer.size());
		}
		SECTION("Bool") {
			// The argument should just be the header (8 bytes)
			uint64_t expectedArgSizeInWords = 1;
			bool expectedValue = true;

			fxt::RecordArgument arg = fxt::RecordArgument(argumentNameValue, fxt::RecordArgumentValue(expectedValue));

			fxt::ProcessedRecordArgument processedArg;
			REQUIRE(fxt::ProcessArgs(&writer, &arg, 1, &processedArg) == 0);

			// Validate the processing

			// We're using a string table name. So this is zero
			REQUIRE(processedArg.nameSizeInWords == 0);
			// Name str ref
			REQUIRE(processedArg.nameStringRef == expectedNameStrRef);
			// Bool arg doesn't take up any extra space
			REQUIRE(processedArg.valueSizeInWords == 0);

			// Write the argument
			unsigned int wordsWritten;
			REQUIRE(fxt::WriteArg(&writer, &arg, &processedArg, &wordsWritten) == 0);
			REQUIRE(wordsWritten == expectedArgSizeInWords);

			// Validate the written bytes
			uint64_t bufferOffset = 0;

			// The name string record is first
			bufferOffset += ValidateStringTableRecord(&buffer[bufferOffset], argumentName, expectedNameStrRef);

			// Now we can validate the argument data itself
			{
				// Validate the header fields
				uint64_t header = ReadUInt64(&buffer[bufferOffset]);
				INFO("Header: " << std::showbase << std::hex << header);
				bufferOffset += 8;

				// Arg type
				REQUIRE(GetFieldFromValue(0, 3, header) == (uint64_t)fxt::internal::ArgumentType::Bool);
				// Arg size in multiples of uint64_t
				REQUIRE(GetFieldFromValue(4, 15, header) == expectedArgSizeInWords);
				// Arg name string ref
				REQUIRE((uint16_t)GetFieldFromValue(16, 31, header) == expectedNameStrRef);
				// Check the value
				REQUIRE((uint8_t)GetFieldFromValue(32, 32, header) == (expectedValue ? 1 : 0));
				// The rest should be zero
				REQUIRE(GetFieldFromValue(33, 63, header) == 0);
			}

			// Check that we read everything
			REQUIRE(bufferOffset == buffer.size());
		}
	}
}

static uint64_t ValidateStringTableRecord(uint8_t *buffer, const char *expectedValue, uint16_t expectedValueStrRef) {
	uint64_t bufferOffset = 0;

	uint64_t expectedStrLen = strlen(expectedValue);
	uint64_t expectedPaddedStrLen = (expectedStrLen + 8 - 1) & (-8);
	uint64_t expectedStringRecordSizeInWords = 1 /* header */ + (expectedPaddedStrLen / 8);

	// Validate the header fields
	uint64_t header = ReadUInt64(&buffer[bufferOffset]);
	INFO("Header: " << std::showbase << std::hex << header);
	bufferOffset += 8;

	// Record type
	REQUIRE(GetFieldFromValue(0, 3, header) == (uint64_t)fxt::RecordType::String);
	// Record size in multiples of uint64_t
	REQUIRE(GetFieldFromValue(4, 15, header) == expectedStringRecordSizeInWords);
	// String index
	REQUIRE((uint16_t)GetFieldFromValue(16, 30, header) == expectedValueStrRef);
	// Check padding bit
	REQUIRE(GetFieldFromValue(31, 31, header) == 0);
	// String length
	uint64_t actualStringLen = GetFieldFromValue(32, 46, header);
	REQUIRE(actualStringLen == expectedStrLen);
	// The rest should be zero
	REQUIRE(GetFieldFromValue(47, 63, header) == 0);

	// Check the string
	std::string actualStringValue(buffer + bufferOffset, buffer + bufferOffset + expectedStrLen);
	REQUIRE_THAT(actualStringValue, Catch::Matchers::Equals(expectedValue));

	// Check that the remaining bytes are all zero
	std::vector<uint8_t> expectedZeros(expectedPaddedStrLen - expectedStrLen, 0);
	std::vector<uint8_t> actualBytes(buffer + bufferOffset + expectedStrLen, buffer + bufferOffset + expectedPaddedStrLen);
	REQUIRE_THAT(actualBytes, Catch::Matchers::Equals(expectedZeros));

	bufferOffset += expectedPaddedStrLen;

	return bufferOffset;
}
