/* FXT - A library for creating Fuschia Tracing System (FXT) files
 *
 * FXT is the legal property of Adrian Astley
 * Copyright Adrian Astley 2023
 */

#include "fxt/record_args.h"

#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_string.hpp"

#include <initializer_list>
#include <string>
#include <vector>

TEST_CASE("TestArgumentMacroExpansion", "[write]") {
	auto countArgs = [](std::initializer_list<fxt::RecordArgument> args) {
		return args.size();
	};

	REQUIRE(countArgs(FXT_ARG_LIST("arg1", 10)) == 1);
	REQUIRE(countArgs(FXT_ARG_LIST("arg1", 10, "arg2", 20)) == 2);
	REQUIRE(countArgs(FXT_ARG_LIST("arg1", 10, "arg2", 20, "arg3", 30, "arg4", 40, "arg5", 50)) == 5);
	REQUIRE(countArgs(FXT_ARG_LIST("arg1", 10, "arg2", 20, "arg3", 30, "arg4", 40, "arg5", 50, "arg6", 60, "arg7", 70, "arg8", 80, "arg9", 90, "arg10", 100)) == 10);
}

TEST_CASE("TestArgumentMacroExpansionContent", "[write]") {
	auto toVector = [](std::initializer_list<fxt::RecordArgument> args) {
		return std::vector<fxt::RecordArgument>(args);
	};

	const auto args = toVector(FXT_ARG_LIST("count", 7, "enabled", true, "label", "alpha", "big", (uint64_t)42, "value", 3.5));

	REQUIRE(args.size() == 5);

	REQUIRE(args[0].name.nameLen == 5);
	REQUIRE_THAT(std::string(args[0].name.name, args[0].name.nameLen), Catch::Matchers::Equals("count"));
	REQUIRE(args[0].value.type == fxt::internal::ArgumentType::Int32);
	REQUIRE(args[0].value.int32Value == 7);

	REQUIRE(args[1].name.nameLen == 7);
	REQUIRE_THAT(std::string(args[1].name.name, args[1].name.nameLen), Catch::Matchers::Equals("enabled"));
	REQUIRE(args[1].value.type == fxt::internal::ArgumentType::Bool);
	REQUIRE(args[1].value.boolValue == true);

	REQUIRE(args[2].name.nameLen == 5);
	REQUIRE_THAT(std::string(args[2].name.name, args[2].name.nameLen), Catch::Matchers::Equals("label"));
	REQUIRE(args[2].value.type == fxt::internal::ArgumentType::String);
	REQUIRE(args[2].value.stringLen == 5);
	REQUIRE_THAT(std::string(args[2].value.stringValue, args[2].value.stringLen), Catch::Matchers::Equals("alpha"));

	REQUIRE(args[3].name.nameLen == 3);
	REQUIRE_THAT(std::string(args[3].name.name, args[3].name.nameLen), Catch::Matchers::Equals("big"));
	REQUIRE(args[3].value.type == fxt::internal::ArgumentType::UInt64);
	REQUIRE(args[3].value.uint64Value == 42);

	REQUIRE(args[4].name.nameLen == 5);
	REQUIRE_THAT(std::string(args[4].name.name, args[4].name.nameLen), Catch::Matchers::Equals("value"));
	REQUIRE(args[4].value.type == fxt::internal::ArgumentType::Double);
	REQUIRE(args[4].value.doubleValue == 3.5);
}
