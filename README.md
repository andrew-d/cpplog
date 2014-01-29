cpplog
======

A simple, header-only, MIT-licensed C++ logging library.

The layout of this library is based on Google's logging library (http://code.google.com/p/google-glog/), but does not use any code copied from that project.

## Installation

```sh
git clone https://github.com/andrew-d/cpplog.git
cd cpplog
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=/usr
make && make install
```

Note: you can change `-DCMAKE_INSTALL_PREFIX=/usr` to any install prefix.

## Usage

Basic usage example:

```cpp
    StdErrLogger log;
    LOG_WARN(log) << "Log message here" << std::endl;
    CHECK_EQUAL(log, 1 == 2) << "Some other message" << std::endl;
    CHECK_STREQ(log, "a", "a") << "Strings should be equal" << std::endl;
```

## Thanks

Thanks to GitHub's fakechris, there is experimental support for logging to a Scribe node (see: https://github.com/facebook/scribe for more information).  It requires Apache Thrift (http://thrift.apache.org/). To use it, `#define CPPLOG_WITH_SCRIBE_LOGGER`.

NOTE: Tests are relatively complete, but not exhaustive.  Please use at your own risk, and feel free to submit bug reports.

Thanks to (in alphabetical order):
  - fakechris [GitHub]
  - Kranar [Reddit]
  - olajep [GitHub]
  - vvavrychuk [GitHub]
  - z00m1n [GitHub]
