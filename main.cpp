#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <string>
#include <sstream>
#include <strstream>

#ifndef CPPLOG_NO_SYSTEM_IDS
#include <boost/interprocess/detail/os_thread_functions.hpp>
using namespace boost::interprocess::detail;
#endif

#include "cpplog.hpp"

using namespace cpplog;
using namespace std;

// Helper function - gets the header string
void getLogHeader(string& outString, loglevel_t level, char* file, unsigned int line)
{
	ostringstream outStream;

#ifndef CPPLOG_NO_SYSTEM_IDS
	unsigned long processId	= get_current_process_id();
	unsigned long threadId	= get_current_thread_id();

	outStream << "[" 
			  << right << setfill('0') << setw(8) << hex 
			  << processId << "."
			  << setfill('0') << setw(8) << hex 
			  << threadId << "] ";
#endif

	outStream << setfill(' ') << setw(5) << left << dec 
			  << LogMessage::getLevelName(level) << " - " 
			  << cpplog::helpers::fileNameFromPath(file) << "(" << line << "): ";

	outString = outStream.str();
}


int TestLogLevels()
{
	int failed = 0;
	string expectedValue;
	StringLogger log;

	cout << "Testing logging macros... ";

#define TEST_EXPECTED(str, level)																		\
			if( level >= CPPLOG_FILTER_LEVEL )															\
			{																							\
				expectedValue.clear();																	\
				getLogHeader(expectedValue, level, __FILE__, __LINE__);									\
				expectedValue += str;																	\
				expectedValue += "\n";																	\
			} else																						\
			{																							\
				expectedValue = "";																		\
			}																							\
			if( expectedValue != log.getString() )														\
			{																							\
				cerr << "Mismatch detected at " << cpplog::helpers::fileNameFromPath(__FILE__)			\
					 << "(" << __LINE__ << "): \"" << log.getString() << "\" != \""						\
					 << expectedValue << "\"" << endl;													\
				failed++;																				\
			}																							\
			log.clear()																					
			

	// Note: All these are on the same line so that the __LINE__ macro will match.
	LOG_TRACE(log) << "Trace message"; TEST_EXPECTED("Trace message", LL_TRACE);
	LOG_DEBUG(log) << "Debug message"; TEST_EXPECTED("Debug message", LL_DEBUG);
	LOG_INFO(log) << "Info message"; TEST_EXPECTED("Info message", LL_INFO);
	LOG_WARN(log) << "Warning message"; TEST_EXPECTED("Warning message", LL_WARN);
	LOG_ERROR(log) << "Error message"; TEST_EXPECTED("Error message", LL_ERROR);
	LOG_FATAL(log) << "Fatal message"; TEST_EXPECTED("Fatal message", LL_FATAL);
	LOG_LEVEL(LL_WARN, log) << "Specified warning message"; TEST_EXPECTED("Specified warning message", LL_WARN);
	LOG(LL_DEBUG, log) << "Short specified debug message"; TEST_EXPECTED("Short specified debug message", LL_DEBUG);


#undef TEST_EXPECTED

	cout << "done!" << endl;
	return failed;
}

int TestDebugLogLevels()
{
	int failed = 0;
	string expectedValue;
	StringLogger log;

	cout << "Testing debug-logging macros... ";

#ifdef _DEBUG
#define TEST_EXPECTED(str, level)																		\
			if( level >= CPPLOG_FILTER_LEVEL )															\
			{																							\
				expectedValue.clear();																	\
				getLogHeader(expectedValue, level, __FILE__, __LINE__);									\
				expectedValue += str;																	\
				expectedValue += "\n";																	\
			} else																						\
			{																							\
				expectedValue = "";																		\
			}																							\
			if( expectedValue != log.getString() )														\
			{																							\
				cerr << "Mismatch detected at " << cpplog::helpers::fileNameFromPath(__FILE__)			\
					 << "(" << __LINE__ << "): \"" << log.getString() << "\" != \""						\
					 << expectedValue << "\"" << endl;													\
				failed++;																				\
			}																							\
			log.clear()																					
#else
#define TEST_EXPECTED(str, level)																		\
			expectedValue = "";																			\
			if( expectedValue != log.getString() )														\
			{																							\
				cerr << "Mismatch detected at " << cpplog::helpers::fileNameFromPath(__FILE__)			\
					 << "(" << __LINE__ << "): \"" << log.getString() << "\" != \""						\
					 << expectedValue << "\"" << endl;													\
				failed++;																				\
			}																							\
			log.clear()	
#endif
			

	// Note: All these are on the same line so that the __LINE__ macro will match.
	DLOG_TRACE(log) << "Trace message"; TEST_EXPECTED("Trace message", LL_TRACE);
	DLOG_DEBUG(log) << "Debug message"; TEST_EXPECTED("Debug message", LL_DEBUG);
	DLOG_INFO(log) << "Info message"; TEST_EXPECTED("Info message", LL_INFO);
	DLOG_WARN(log) << "Warning message"; TEST_EXPECTED("Warning message", LL_WARN);
	DLOG_ERROR(log) << "Error message"; TEST_EXPECTED("Error message", LL_ERROR);
	DLOG(LL_DEBUG, log) << "Short specified debug message"; TEST_EXPECTED("Short specified debug message", LL_DEBUG);

	// Fatal messages are always logged.  We have to handle this manually.
	DLOG_FATAL(log) << "Fatal message"; int line = __LINE__;
	expectedValue.clear();
	getLogHeader(expectedValue, LL_FATAL, __FILE__, line);
	expectedValue += "Fatal message\n";	
	if( expectedValue != log.getString() )
	{
		cerr << "Mismatch detected at " << cpplog::helpers::fileNameFromPath(__FILE__)
			 << "(" << line << "): \"" << log.getString() << "\" != \""
			 << expectedValue << "\"" << endl;
		failed++;
	}
	log.clear();


#undef TEST_EXPECTED

	cout << "done!" << endl;
	return failed;
}

int TestConditionMacros()
{
	int failed = 0;
	string expectedValue;
	StringLogger log;

	cout << "Testing conditional macros... ";

#define TEST_EXPECTED(str, level, logged)																\
			if( level >= CPPLOG_FILTER_LEVEL && logged )												\
			{																							\
				expectedValue.clear();																	\
				getLogHeader(expectedValue, level, __FILE__, __LINE__);									\
				expectedValue += str;																	\
				expectedValue += "\n";																	\
			} else																						\
			{																							\
				expectedValue = "";																		\
			}																							\
			if( expectedValue != log.getString() )														\
			{																							\
				cerr << "Mismatch detected at " << cpplog::helpers::fileNameFromPath(__FILE__)			\
					 << "(" << __LINE__ << "): \"" << log.getString() << "\" != \""						\
					 << expectedValue << "\"" << endl;													\
				failed++;																				\
			}																							\
			log.clear()		


	LOG_IF(LL_WARN, log, 1 == 2) << "This should not be logged"; TEST_EXPECTED("This should not be logged", LL_WARN, false);
	LOG_IF(LL_WARN, log, 'a' == 'a') << "This should be logged"; TEST_EXPECTED("This should be logged", LL_WARN, true);

	LOG_IF_NOT(LL_WARN, log, 1 == 2) << "This should be logged"; TEST_EXPECTED("This should be logged", LL_WARN, true);
	LOG_IF_NOT(LL_WARN, log, 1 == 1) << "This should not be logged"; TEST_EXPECTED("This should not be logged", LL_WARN, false);

#undef TEST_EXPECTED
	cout << "done!" << endl;
	return failed;
}

int TestCheckMacros()
{
	int failed = 0;
	StringLogger log;

	cout << "Testing conditional macros... ";

#define TEST_EXPECTED(logged)																			\
			if( logged )																				\
			{																							\
				if( log.getString().find("Check failed: ") == string::npos )							\
				{																						\
					cerr << "Mismatch detected at " << cpplog::helpers::fileNameFromPath(__FILE__)		\
						 << "(" << __LINE__ << ")" << endl;												\
					failed++;																			\
				}																						\
				log.clear();																			\
			} else																						\
			{																							\
				if( log.getString().length() > 0 )														\
				{																						\
					cerr << "Mismatch detected at " << cpplog::helpers::fileNameFromPath(__FILE__)		\
						 << "(" << __LINE__ << ")" << endl;												\
					failed++;																			\
				}																						\
			}


	CHECK(log, 1 == 1) << "Should not log"; TEST_EXPECTED(false);
	CHECK(log, 1 == 2) << "Should log"; TEST_EXPECTED(true);

	CHECK_EQUAL(log, 1, 1) << "Should not log"; TEST_EXPECTED(false);
	CHECK_EQUAL(log, 1, 2) << "Should log"; TEST_EXPECTED(true);

	CHECK_LT(log, 1, 2) << "Should not log"; TEST_EXPECTED(false);
	CHECK_LT(log, 2, 1) << "Should log"; TEST_EXPECTED(true);
	CHECK_LT(log, 3, 3) << "Should log"; TEST_EXPECTED(true);

	CHECK_GT(log, 4, 2) << "Should not log"; TEST_EXPECTED(false);
	CHECK_GT(log, 2, 4) << "Should log"; TEST_EXPECTED(true);
	CHECK_GT(log, 4, 4) << "Should log"; TEST_EXPECTED(true);

	CHECK_LE(log, 1, 2) << "Should not log"; TEST_EXPECTED(false);
	CHECK_LE(log, 1, 1) << "Should not log"; TEST_EXPECTED(false);
	CHECK_LE(log, 2, 1) << "Should log"; TEST_EXPECTED(true);
	
	CHECK_GE(log, 3, 1) << "Should not log"; TEST_EXPECTED(false);
	CHECK_GE(log, 3, 3) << "Should not log"; TEST_EXPECTED(false);
	CHECK_GE(log, 1, 3) << "Should log"; TEST_EXPECTED(true);
	
	CHECK_NE(log, 1, 2) << "Should not log"; TEST_EXPECTED(false);
	CHECK_NE(log, 1, 1) << "Should log"; TEST_EXPECTED(true);
	
	CHECK_NOT_EQUAL(log, 1, 2) << "Should not log"; TEST_EXPECTED(false);
	CHECK_NOT_EQUAL(log, 1, 1) << "Should log"; TEST_EXPECTED(true);
	
	CHECK_STREQ(log, "ab", "ab") << "Should not log"; TEST_EXPECTED(false);
	CHECK_STREQ(log, "ab", "cd") << "Should log"; TEST_EXPECTED(true);

	CHECK_STRNE(log, "ab", "cd") << "Should not log"; TEST_EXPECTED(false);
	CHECK_STRNE(log, "qq", "qq") << "Should log"; TEST_EXPECTED(true);

	CHECK_NULL(log, NULL) << "Should not log"; TEST_EXPECTED(false);
	CHECK_NULL(log, (void*)(1)) << "Should log"; TEST_EXPECTED(true);

	CHECK_NOT_NULL(log, (void*)(1)) << "Should not log"; TEST_EXPECTED(false);
	CHECK_NOT_NULL(log, NULL) << "Should log"; TEST_EXPECTED(true);

#undef TEST_EXPECTED
	cout << "done!" << endl;
	return failed;
}

int TestTeeLogger()
{
	int failed = 0;
	StringLogger logger1;
	StringLogger logger2;
	TeeLogger tlog(logger1, logger2);
	string expectedValue;

	cout << "Testing TeeLogger... ";
	
	LOG_WARN(tlog) << "Some message here" << endl;	int line = __LINE__;

	getLogHeader(expectedValue, LL_WARN, __FILE__, line);
	expectedValue += "Some message here\n";

	// Compare.
	if( expectedValue != logger1.getString() )
	{
		cerr << "Mismatch (1) detected at " << cpplog::helpers::fileNameFromPath(__FILE__)
			 << "(" << line << ")" << endl;
		failed++;	
	}
	if( expectedValue != logger2.getString() )
	{
		cerr << "Mismatch (2) detected at " << cpplog::helpers::fileNameFromPath(__FILE__)
			 << "(" << line << ")" << endl;
		failed++;	
	}
	
	cout << "done!" << endl;
	return failed;
}

#ifndef CPPLOG_NO_THREADING
int TestBackgroundLogger()
{
	int failed = 0, line;
	StringLogger slogger;
	string expectedValue;

	cout << "Testing BackgroundLogger... ";
	
	// Scoped, such that when the object is destructed we wait for the log 
	// messages to be flushed to the StringLogger.
	{
		BackgroundLogger blog(slogger);
		LOG_WARN(blog) << "Background message here." << endl;	line = __LINE__;
	}

	getLogHeader(expectedValue, LL_WARN, __FILE__, line);
	expectedValue += "Background message here.\n";
	if( expectedValue != slogger.getString() )
	{
		cerr << "Mismatch detected at " << cpplog::helpers::fileNameFromPath(__FILE__)
			 << "(" << line << ")" << endl;
		failed++;	
	}

	cout << "done!" << endl;
	return failed;
}

class CountingLogger : public BaseLogger
{
private:
	int			m_logMessageCount;

public:
	CountingLogger()
		: m_logMessageCount(0)
	{ }

	virtual bool sendLogMessage(LogData* /* logData */)
	{
		m_logMessageCount++;
		return true;
	}

	int getCount()
	{
		return m_logMessageCount;
	}
};

int TestBackgroundLoggerConcurrency()
{
	int failed = 0;
	CountingLogger clog;
	const int numMessages = 100000;

	cout << "Testing BackgroundLogger for consistency... " << flush;

	// Scoped!
	{
		BackgroundLogger blog(clog);

		for( int i = 0; i < numMessages; i++ )
		{
			LOG_INFO(blog) << "Message " << i << std::endl;
		}
	}

	// Validate the number of messages sent.
	if( clog.getCount() != numMessages )
	{
		cerr << "Mismatch detected!  Sent: " << numMessages << ", Received: " << clog.getCount() << endl;
		failed++;
	}

	cout << "done!" << endl;

	return failed;
}
#endif // CPPLOG_NO_THREADING

int TestLogging()
{
	int totalFailures = 0;

	totalFailures += TestLogLevels();
	totalFailures += TestDebugLogLevels();
	totalFailures += TestConditionMacros();
	totalFailures += TestCheckMacros();
	totalFailures += TestTeeLogger();

#ifndef CPPLOG_NO_THREADING
	totalFailures += TestBackgroundLogger();
	totalFailures += TestBackgroundLoggerConcurrency();
#endif

	return totalFailures;
}

int main(void)
{
	int totalFailures;

	totalFailures = TestLogging();

	cout << "\n------------------------------" << endl;
	if( 0 == totalFailures )
		cout << "All tests passed!" << endl;
	else 
		cerr << totalFailures << " tests failed :-(" << endl;

	return totalFailures;
}