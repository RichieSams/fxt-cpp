# fxt-cpp

[Fuschia Trace Format](https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/docs/reference/tracing/trace-format.md) (fxt) is a file format for storing trace / counter events in a compact binary format. These trace files can then be viewed with an interactive web-based UI: https://ui.perfetto.dev/

FXT was created by Google for use in their experimental operating system [Fuschia](https://fuchsia.dev/fuchsia-src). It's very well documented, simple to write, and can express lots of different types of events and data.

There are lots of other tracing file formats out there.

* [chrome://tracing](https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU/preview)
  * chrome://tracing is extremely simple and you just need a Chrome browser to view it.
  * But it is json-based. So the file can quickly explode in size for large traces.
* [Perfetto](https://perfetto.dev/)
  * Perfetto is the successor to chrome://tracing and has many improvements.
  * It's protobuf-based, so it's much smaller / compact.
  * However, protobuf can be a pain to use. You generally want to use the .proto file to generate a generator/parser, but that's a lot of extra steps / maintenance
* [Speedscope](https://github.com/jlfwong/speedscope/wiki/Importing-from-custom-sources#speedscopes-file-format)
  * Speedscope is a web-based trace visualizer like Perfetto UI
  * It can view quite a few different formats
  * In addition, it has its own json-based trace format
  * Which, unfortunately, has the same issues as chrome://tracing
* Many many more

This repo is a library for creating FXT files in C++
