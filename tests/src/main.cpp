/* FXT - A library for creating Fuschia Tracing System (FXT) files
 *
 * FXT is the legal property of Adrian Astley
 * Copyright Adrian Astley 2023
 */

#if defined(__ORBIS__) || defined(__PROSPERO__)
#	include <stdlib.h>
unsigned int sceLibcHeapExtendedAlloc = 1;
size_t sceLibcHeapSize = SCE_LIBC_HEAP_SIZE_EXTENDED_ALLOC_NO_LIMIT;

#endif

#if defined(_GAMING_XBOX_XBOXONE) || defined(_GAMING_XBOX_SCARLETT)

// Create output streams for logging output using DebugOutputA
#	define CATCH_CONFIG_NOSTDOUT
#	include "catch2/catch_session.hpp"

#	include <Windows.h>

#	define DEBUG_STREAM_STORAGE 1024
class debug_stream_buffer : public std::streambuf {
public:
	debug_stream_buffer() {
	}

	virtual ~debug_stream_buffer() {
		sync();
	}

	// Cannot read from this stream
	virtual std::streambuf::int_type underflow() {
		return traits_type::eof();
	}

	virtual std::streambuf::int_type overflow(std::streambuf::int_type value) {
		size_t write = pptr() - pbase();
		if (write) {
			// Copy buffer to string which can definitely hold a null terminator
			char output_data[DEBUG_STREAM_STORAGE + 1] = {0};
			indyfs_strlcpy(output_data, storage, write);

			// Write data to output
			OutputDebugStringA(output_data);
		}

		setp(storage, storage + DEBUG_STREAM_STORAGE);
		if (!traits_type::eq_int_type(value, traits_type::eof()))
			sputc((char)value);
		return traits_type::not_eof(value);
	};

	virtual int sync() {
		std::streambuf::int_type result = this->overflow(traits_type::eof());
		return traits_type::eq_int_type(result, traits_type::eof()) ? -1 : 0;
	}

private:
	char storage[DEBUG_STREAM_STORAGE];
};

class debug_ostream : public std::ostream {
public:
	debug_ostream()
	        : std::ostream(new debug_stream_buffer) {
	}

	virtual ~debug_ostream() {
		delete rdbuf();
	}
};

debug_ostream debug_cout;
debug_ostream debug_cerr;
debug_ostream debug_clog;

namespace Catch {
std::ostream &cout() {
	return debug_cout;
}
std::ostream &cerr() {
	return debug_cerr;
}
std::ostream &clog() {
	return debug_clog;
}
} // End of namespace Catch

#else

#	include "catch2/catch_session.hpp"

#endif

int main(int argc, char **argv) {
	Catch::Session session; // There must be exactly one instance

	// Build a new parser on top of Catch's
	using namespace Catch::Clara;

	auto cli = session.cli();

	// Now pass the new composite back to Catch so it uses that
	session.cli(cli);

	// Let Catch (using Clara) parse the command line
	int returnCode = session.applyCommandLine(argc, argv);
	if (returnCode != 0) // Indicates a command line error
		return returnCode;

	return session.run();
}
