/* FXT - A library for creating Fuschia Tracing System (FXT) files
 *
 * FXT is the legal property of Adrian Astley
 * Copyright Adrian Astley 2023
 */

#include "fxt/writer.h"

#include "catch2/catch_test_macros.hpp"

#include <stdio.h>
#include <vector>

TEST_CASE("TestGeneralWrite", "[write]") {
	std::vector<uint8_t> buffer;

	fxt::Writer writer((void *)&buffer, [](void *userContext, uint8_t *data, size_t len) -> int {
		std::vector<uint8_t> *buffer = (std::vector<uint8_t> *)userContext;

		buffer->insert(buffer->end(), data, data + len);
		return 0;
	});

	// Add the magic number record
	REQUIRE(writer.WriteMagicNumberRecord() == 0);

	// Set up the provider info
	REQUIRE(writer.AddProviderInfoRecord(1234, "Test Provider") == 0);
	REQUIRE(writer.AddProviderSectionRecord(1234) == 0);
	REQUIRE(writer.AddInitializationRecord(1000) == 0);

	// Name the processes / threads
	REQUIRE(writer.SetProcessName(3, "Test.exe") == 0);
	REQUIRE(writer.SetThreadName(3, 45, "Main") == 0);
	REQUIRE(writer.SetThreadName(3, 87, "Worker0") == 0);
	REQUIRE(writer.SetThreadName(3, 26, "Worker1") == 0);
	REQUIRE(writer.SetProcessName(4, "Server.exe") == 0);
	REQUIRE(writer.SetThreadName(4, 50, "ServerThread") == 0);

	// Do a basic set of spans
	// And throw in some async
	REQUIRE(writer.AddDurationBeginEvent("Foo", "Root", 3, 45, 200) == 0);
	REQUIRE(writer.AddInstantEvent("OtherThing", "EventHappened", 3, 45, 300) == 0);
	REQUIRE(writer.AddDurationBeginEvent("Foo", "Inner", 3, 45, 400) == 0);
	REQUIRE(writer.AddAsyncBeginEvent("Asdf", "AsyncThing", 3, 45, 450, 111) == 0);
	REQUIRE(writer.AddDurationCompleteEvent("OtherService", "DoStuff", 3, 45, 500, 800) == 0);
	REQUIRE(writer.AddAsyncInstantEvent("Asdf", "AsyncInstant", 3, 87, 825, 111) == 0);
	REQUIRE(writer.AddAsyncEndEvent("Asdf", "AsyncThing", 3, 87, 850, 111) == 0);
	REQUIRE(writer.AddDurationEndEvent("Foo", "Inner", 3, 45, 900) == 0);
	REQUIRE(writer.AddDurationEndEvent("Foo", "Root", 3, 45, 900) == 0);

	// Test out flows
	REQUIRE(writer.AddDurationBeginEvent("CategoryA", "REST Request to server", 3, 45, 950) == 0);
	REQUIRE(writer.AddFlowBeginEvent("CategoryA", "AwesomeFlow", 3, 45, 955, 123) == 0);
	REQUIRE(writer.AddDurationEndEvent("CategoryA", "REST Request to server", 3, 45, 1000) == 0);
	REQUIRE(writer.AddDurationBeginEvent("CategoryA", "Server process request", 4, 50, 1000) == 0);
	REQUIRE(writer.AddFlowStepEvent("CategoryA", "Server request handler", 4, 50, 1005, 123) == 0);
	REQUIRE(writer.AddDurationEndEvent("CategoryA", "Server process request", 4, 50, 1100) == 0);
	REQUIRE(writer.AddDurationBeginEvent("CategoryA", "Process server response", 3, 45, 1150) == 0);
	REQUIRE(writer.AddFlowEndEvent("CategoryA", "AwesomeFlow", 3, 45, 1155, 123) == 0);
	REQUIRE(writer.AddDurationEndEvent("CategoryA", "Process server response", 3, 45, 1200) == 0);

	// Add some counter events
	{
		fxt::Int32EventArgument intArg("int_arg", 111);
		fxt::UInt32EventArgument uintArg("uint_arg", 984);
		fxt::DoubleEventArgument doubleArg("double_arg", 1.0);
		fxt::Int64EventArgument int64Arg("int64_arg", 851);
		fxt::UInt64EventArgument uint64Arg("uint64_arg", 35);

		REQUIRE(writer.AddCounterEvent("Bar", "CounterA", 3, 45, 250, 555, &intArg, &uintArg, &doubleArg, &int64Arg, &uint64Arg) == 0);
	}

	{
		fxt::Int32EventArgument intArg("int_arg", 784);
		fxt::UInt32EventArgument uintArg("uint_arg", 561);
		fxt::DoubleEventArgument doubleArg("double_arg", 4.0);
		fxt::Int64EventArgument int64Arg("int64_arg", 445);
		fxt::UInt64EventArgument uint64Arg("uint64_arg", 95);

		REQUIRE(writer.AddCounterEvent("Bar", "CounterA", 3, 45, 500, 555, &intArg, &uintArg, &doubleArg, &int64Arg, &uint64Arg) == 0);
	}

	{
		fxt::Int32EventArgument intArg("int_arg", 333);
		fxt::UInt32EventArgument uintArg("uint_arg", 845);
		fxt::DoubleEventArgument doubleArg("double_arg", 9.0);
		fxt::Int64EventArgument int64Arg("int64_arg", 521);
		fxt::UInt64EventArgument uint64Arg("uint64_arg", 24);

		REQUIRE(writer.AddCounterEvent("Bar", "CounterA", 3, 45, 500, 555, &intArg, &uintArg, &doubleArg, &int64Arg, &uint64Arg) == 0);
	}

	// Add a blob record
	REQUIRE(writer.AddBlobRecord("TestBlob", (void *)"testing123", 10, fxt::BlobType::Data) == 0);

	// Add events with argument data
	{
		fxt::NullEventArgument nullArg("null_arg");
		REQUIRE(writer.AddDurationBeginEvent("Foo", "Root", 3, 87, 1200, &nullArg) == 0);
	}
	{
		fxt::Int32EventArgument intArg("int_arg", 4565);
		REQUIRE(writer.AddInstantEvent("OtherThing", "EventHappened", 3, 87, 1300, &intArg) == 0);
	}
	{
		fxt::UInt32EventArgument uintArg("uint_arg", 333);
		REQUIRE(writer.AddDurationBeginEvent("Foo", "Inner", 3, 87, 1400, &uintArg) == 0);
	}
	{
		fxt::Int64EventArgument int64Arg("int64_arg", 784);
		REQUIRE(writer.AddAsyncBeginEvent("Asdf", "AsyncThing2", 3, 87, 1450, 222, &int64Arg) == 0);
	}
	{
		fxt::UInt64EventArgument uint64Arg("uint64_arg", 454);
		REQUIRE(writer.AddDurationCompleteEvent("OtherService", "DoStuff", 3, 87, 1500, 800, &uint64Arg) == 0);
	}
	{
		fxt::DoubleEventArgument doubleArg("double_arg", 333.3424);
		REQUIRE(writer.AddAsyncInstantEvent("Asdf", "AsyncInstant2", 3, 26, 1825, 222, &doubleArg) == 0);
	}
	{
		fxt::StringEventArgument strArg("string_arg", "str_value");
		REQUIRE(writer.AddAsyncEndEvent("Asdf", "AsyncThing2", 3, 26, 1850, 222, &strArg) == 0);
	}
	{
		fxt::BoolEventArgument boolArg("bool_arg", true);
		REQUIRE(writer.AddUserspaceObjectRecord("MyAwesomeObject", 3, 26, (uintptr_t)(67890), &boolArg) == 0);
	}
	{
		fxt::PointerEventArgument ptrArg("pointer_arg", (uintptr_t)67890);
		REQUIRE(writer.AddDurationEndEvent("Foo", "Inner", 3, 87, 1900, &ptrArg) == 0);
	}
	{
		fxt::KOIDEventArgument koidArg("koid_arg", fxt::KernelObjectID(3));
		REQUIRE(writer.AddDurationEndEvent("Foo", "Root", 3, 87, 1900, &koidArg) == 0);
	}

	// Add some scheduling events
	{
		fxt::Int32EventArgument incomingWeight("incoming_weight", 2);
		fxt::Int32EventArgument outgoingWeight("outgoing_weight", 4);
		REQUIRE(writer.AddContextSwitchRecord(3, 1, 45, 87, 250, &incomingWeight, &outgoingWeight) == 0);
	}
	{
		fxt::Int32EventArgument incomingWeight("incoming_weight", 2);
		fxt::Int32EventArgument outgoingWeight("outgoing_weight", 4);
		REQUIRE(writer.AddContextSwitchRecord(3, 1, 87, 45, 255, &incomingWeight, &outgoingWeight) == 0);
	}
	REQUIRE(writer.AddThreadWakeupRecord(3, 45, 925) == 0);

	// Flush
	REQUIRE(writer.Flush() == 0);

	// Test the output
	FILE *file = fopen("test.fxt", "wb");
	REQUIRE(file != nullptr);

	fwrite(&buffer[0], 1, buffer.size(), file);

	fclose(file);
}