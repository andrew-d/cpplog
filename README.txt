A simple, header-only, MIT-licensed C++ logging library.  

Basic usage example:

	StdErrLogger log;
	LOG_WARN(log) << "Log message here" << std::endl;
	CHECK_EQUAL(log, 1 == 2) << "Some other message" << std::endl;
	CHECK_STREQ(log, "a", "a") << "Strings should be equal" << std::endl;

The layout of this library is based on Google's logging library (http://code.google.com/p/google-glog/), but does not use any code copied from that project.

NOTE: Tests are relatively complete, but not exhaustive.  Please use at your own risk, and feel free to submit bug reports.