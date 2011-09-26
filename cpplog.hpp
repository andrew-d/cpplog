#pragma once

#ifndef _CPPLOG_H
#define _CPPLOG_H

#include <iostream>
#include <iomanip>
#include <string>
#include <strstream>
#include <fstream>
#include <cstring>
#include <ctime>
#include <vector>

// The following #define's will change the behaviour of this library.
//		#define CPPLOG_FILTER_LEVEL		<level>
//			Prevents all log messages with level less than <level> from being emitted.
//
//		#define CPPLOG_NO_SYSTEM_IDS
//			Disables capturing of the Process and Thread ID.
//
//		#define CPPLOG_NO_THREADING
//			Disables threading (BackgroundLogger).  Note that this means that the library
//			is truly header-only, as it no longer depends on Boost.
//
//		#define CPPLOG_NO_HELPER_MACROS
//			Disables inclusion of the CHECK_* macros.
//
//		#define CPPLOG_FATAL_NOEXIT
//			Causes a fatal error to not exit() the process.
//
//		#define	CPPLOG_FATAL_NOEXIT_DEBUG
//			As above, but only in debug mode.
//			Note: Defined below by default.

// ------------------------------- DEFINITIONS -------------------------------

// Severity levels:
// Note: Giving a value for CPPLOG_FILTER_LEVEL will log all messages at
//		 or above that level.
//	0 - Trace
//	1 - Debug
//	2 - Info
//	3 - Warning
//	4 - Error
//	5 - Fatal (always logged)

#define LL_TRACE	0
#define LL_DEBUG	1
#define LL_INFO		2
#define LL_WARN		3
#define LL_ERROR	4
#define LL_FATAL	5


// ------------------------------ CONFIGURATION ------------------------------

//#define CPPLOG_FILTER_LEVEL				LL_WARN
//#define CPPLOG_NO_SYSTEM_IDS
//#define CPPLOG_NO_THREADING
//#define CPPLOG_NO_HELPER_MACROS
//#define CPPLOG_FATAL_NOEXIT

#define CPPLOG_FATAL_NOEXIT_DEBUG


// ---------------------------------- CODE -----------------------------------

#ifndef CPPLOG_NO_SYSTEM_IDS
#include <boost/interprocess/detail/os_thread_functions.hpp>
#endif

#ifndef CPPLOG_NO_THREADING
#include <boost/thread.hpp>
#include "concurrent_queue.hpp"
#endif

// If we don't have a level defined, set it to CPPLOG_LEVEL_DEBUG (log all except trace statements)
#ifndef CPPLOG_FILTER_LEVEL
#define CPPLOG_FILTER_LEVEL LL_DEBUG
#endif 


// The general concept for how logging works:
//	- Every call to LOG(LEVEL, logger) works as follows:
//		- Instantiates an object of type LogMessage.
//		- LogMessage's constructor captures __FILE__, __LINE__, severity and our output logger.
//		- LogMessage exposes a function getStream(), which is an ostringstream-style stream that
//		  client code can write debug information into.
//		- When the sendToLogger() method of a LogMessage is called, all the buffered data in the
//		  messages' stream is sent to the specified logger.
//		- When a LogMessage's destructor is called, it calls sendToLogger() to send all 
//		  remaining data.


namespace cpplog
{
	// Our log level type. 
	// NOTE: When C++11 becomes widely supported, convert this to "enum class LogLevel".
	typedef unsigned int loglevel_t;

	// Helper functions.  Stuck these in their own namespace for simplicity.
	namespace helpers
	{
		// Gets the filename from a path.
		inline static const char* fileNameFromPath(const char* filePath)
		{
			const char* fileName = strrchr(filePath, '/');
#ifdef _WIN32
			if( !fileName )
				fileName = strrchr(filePath, '\\');
#endif
			return fileName ? fileName + 1 : filePath;
		}

		// Simple class that allows us to evaluate a stream to void - prevents compiler errors.
		class VoidStreamClass
		{
		public:
			VoidStreamClass() { }
			void operator&(std::ostream&) { }
		};
	};

	// Logger data.  This is sent to a logger when a LogMessage is Flush()'ed, or
	// when the destructor is called.
	struct LogData
	{
		// Constant.
		static const size_t k_logBufferSize = 20000;

		// Our stream to log data to.
		std::ostrstream	stream;
		
		// Captured data.
		unsigned int level;
		unsigned long line;
		const char* fullPath;
		const char* fileName;
		time_t messageTime;
		::tm utcTime;

#ifndef CPPLOG_NO_SYSTEM_IDS
		// Process/thread ID.
		unsigned long processId;
		unsigned long threadId;
#endif //CPPLOG_NO_SYSTEM_IDS

		// Buffer for our text.
		char buffer[k_logBufferSize];

		// Constructor that initializes our stream.
		LogData(loglevel_t logLevel)
			: stream(buffer, k_logBufferSize), level(logLevel)
#ifndef CPPLOG_NO_SYSTEM_IDS
			  , processId(0), threadId(0)
#endif
		{
		}

		virtual ~LogData()
		{ }
	};

	// Base interface for a logger.
	class BaseLogger
	{
	public:
		// All loggers must provide an interface to log a message to.
		// The return value of this function indicates whether to delete
		// the log message.
		virtual bool sendLogMessage(LogData* logData) = 0;

		virtual ~BaseLogger() { }
	};

	// Log message - this is instantiated upon every call to LOG(logger)
	class LogMessage
	{
	private:
		BaseLogger*		m_logger;
		LogData*		m_logData;
		bool			m_flushed;
		bool			m_deleteMessage;

		// Flag for if a fatal message has been logged already.
		// This prevents us from calling exit(), which calls something,
		// which then logs a fatal message, which cause an infinite loop.
		// TODO: this should probably be thread-safe...
		static bool getSetFatal(bool get, bool val)
		{
			static bool m_fatalFlag = false;

			if( !get )
				m_fatalFlag = val;
			
			return m_fatalFlag;
		}

	public:
		LogMessage(const char* file, unsigned int line, loglevel_t logLevel, BaseLogger* outputLogger)
			: m_logger(outputLogger)
		{
			Init(file, line, logLevel);
		}

		LogMessage(const char* file, unsigned int line, loglevel_t logLevel, BaseLogger& outputLogger)
			: m_logger(&outputLogger)
		{
			Init(file, line, logLevel);
		}

		virtual ~LogMessage()
		{
			Flush();

			if( m_deleteMessage )
			{
				delete m_logData;
			}
		}

		inline std::ostream& getStream()
		{
			return m_logData->stream;
		}

	private:
		void Init(const char* file, unsigned int line, loglevel_t logLevel)
		{
			m_logData = new LogData(logLevel);
			m_flushed = false;
			m_deleteMessage = false;

			// Capture data.
			m_logData->fullPath		= file;
			m_logData->fileName		= cpplog::helpers::fileNameFromPath(file);
			m_logData->line			= line;
			m_logData->messageTime	= ::time(NULL);

			// Get current time.
			::tm* gmt = ::gmtime(&m_logData->messageTime);
			memcpy(&m_logData->utcTime, gmt, sizeof(tm));

#ifndef CPPLOG_NO_SYSTEM_IDS
			// Get process/thread ID.
			m_logData->processId	= boost::interprocess::detail::get_current_process_id();
			m_logData->threadId		= boost::interprocess::detail::get_current_thread_id();
#endif
		}

		void Flush()
		{
			if( !m_flushed )
			{
				// Check if we have a newline.
				char lastChar = m_logData->buffer[m_logData->stream.pcount() - 1];
				if( lastChar != '\n' )
					m_logData->stream << std::endl;

				// Null-terminate.
				m_logData->stream << '\0';

				// Send the message, set flushed=true.
				m_deleteMessage = m_logger->sendLogMessage(m_logData);
				m_flushed = true;

				// If this is a fatal message...
				if( m_logData->level == LL_FATAL && !getSetFatal(true, true) )
				{
					// Set our fatal flag.
					getSetFatal(false, true);

#ifdef _DEBUG
// Only exit in debug mode if CPPLOG_FATAL_NOEXIT_DEBUG is not set.
#if !defined(CPPLOG_FATAL_NOEXIT_DEBUG) && !defined(CPPLOG_FATAL_NOEXIT)
					::exit(1);
#endif
#else //!_DEBUG
#if !defined(CPPLOG_FATAL_NOEXIT_DEBUG)
					::exit(1)
#endif
#endif
				}
			}
		}

	public:
		static const char* getLevelName(loglevel_t level)
		{
			switch( level )
			{
				case LL_TRACE:
					return "TRACE";
				case LL_DEBUG:
					return "DEBUG";
				case LL_INFO:
					return "WARN";
				case LL_WARN:
					return "WARN";
				case LL_ERROR:
					return "ERROR";
				case LL_FATAL:
					return "FATAL";
				default:
					return "OTHER";
			};
		};

	};

	// Generic class - logs to a given std::ostream.
	class OstreamLogger : public BaseLogger
	{
	private:
		std::ostream&	m_logStream;

	public:
		OstreamLogger(std::ostream& outStream)
			: m_logStream(outStream)
		{ }

		virtual bool sendLogMessage(LogData* logData)
		{
			// Log process ID and thread ID.
#ifndef CPPLOG_NO_SYSTEM_IDS
			m_logStream << "[" 
						<< std::right << std::setfill('0') << std::setw(8) << std::hex 
						<< logData->processId << "."
						<< std::setfill('0') << std::setw(8) << std::hex 
						<< logData->threadId << "] ";
#endif

			m_logStream << std::setfill(' ') << std::setw(5) << std::left << std::dec 
						<< LogMessage::getLevelName(logData->level) << " - " 
						<< logData->fileName << "(" << logData->line << "): " 
						<< logData->buffer;

			m_logStream	<< std::flush;

			return true;
		}

		virtual ~OstreamLogger() { }
	};
	
	// Simple implementation - logs to stderr.
	class StdErrLogger : public OstreamLogger
	{
	public:
		StdErrLogger()
			: OstreamLogger(std::cerr)
		{ }
	};

	// Simple implementation - logs to a string, provides the ability to get that string.
	class StringLogger : public OstreamLogger
	{
	private:
		std::ostringstream	m_stream;
	public:
		StringLogger()
			: OstreamLogger(m_stream)
		{ }

		std::string getString()
		{
			return m_stream.str();
		}

		void clear()
		{
			m_stream.str("");
			m_stream.clear();
		}
	};

	// Log to file.
	class FileLogger : public OstreamLogger
	{
	private:
		std::string		m_path;
		std::ofstream	m_outStream;

	public:
		FileLogger(std::string logFilePath)
			: m_path(logFilePath), m_outStream(logFilePath.c_str(), std::ios_base::out),
			  OstreamLogger(m_outStream)
		{
		}

		FileLogger(std::string logFilePath, bool append)
			: m_path(logFilePath), m_outStream(logFilePath.c_str(), append ? std::ios_base::app : std::ios_base::out),
			  OstreamLogger(m_outStream)
		{
		}
	};

	// Tee logger - given two loggers, will forward a message to both.
	class TeeLogger : public BaseLogger
	{
	private:
		BaseLogger* m_logger1;
		BaseLogger* m_logger2;

		bool		m_logger1Owned;
		bool		m_logger2Owned;

	public:
		TeeLogger(BaseLogger* one, BaseLogger* two)
			: m_logger1(one), m_logger2(two),
			  m_logger1Owned(false), m_logger2Owned(false)
		{ }

		TeeLogger(BaseLogger* one, bool ownOne, BaseLogger* two, bool ownTwo)
			: m_logger1(one), m_logger2(two),
			  m_logger1Owned(ownOne), m_logger2Owned(ownTwo)
		{ }

		TeeLogger(BaseLogger& one, BaseLogger& two)
			: m_logger1(&one), m_logger2(&two),
			  m_logger1Owned(false), m_logger2Owned(false)
		{ }

		TeeLogger(BaseLogger& one, bool ownOne, BaseLogger& two, bool ownTwo)
			: m_logger1(&one), m_logger2(&two),
			  m_logger1Owned(ownOne), m_logger2Owned(ownTwo)
		{ }
		
		~TeeLogger()
		{
			if( m_logger1Owned )
				delete m_logger1;
			if( m_logger2Owned )
				delete m_logger2;
		}

		virtual bool sendLogMessage(LogData* logData)
		{
			bool deleteMessage = true;

			deleteMessage = deleteMessage && m_logger1->sendLogMessage(logData);
			deleteMessage = deleteMessage && m_logger2->sendLogMessage(logData);

			return deleteMessage;
		}
	};

	// Multiplex logger - will forward a log message to all loggers.
	class MultiplexLogger : public BaseLogger
	{
		struct LoggerInfo
		{
			BaseLogger* logger;
			bool		owned;

			LoggerInfo(BaseLogger* l, bool o)
				: logger(l), owned(o)
			{ }
		};
		std::vector<LoggerInfo> m_loggers;

	public:
		MultiplexLogger()
		{ }

		MultiplexLogger(BaseLogger* one)
		{
			m_loggers.push_back(LoggerInfo(one, false));
		}

		MultiplexLogger(BaseLogger& one)
		{
			m_loggers.push_back(LoggerInfo(&one, false));
		}

		MultiplexLogger(BaseLogger* one, bool owned)
		{
			m_loggers.push_back(LoggerInfo(one, owned));
		}

		MultiplexLogger(BaseLogger& one, bool owned)
		{
			m_loggers.push_back(LoggerInfo(&one, owned));
		}

		MultiplexLogger(BaseLogger* one, BaseLogger* two)
		{
			m_loggers.push_back(LoggerInfo(one, false));
			m_loggers.push_back(LoggerInfo(two, false));
		}

		MultiplexLogger(BaseLogger* one, bool ownOne, BaseLogger* two, bool ownTwo)
		{
			m_loggers.push_back(LoggerInfo(one, ownOne));
			m_loggers.push_back(LoggerInfo(two, ownTwo));
		}

		MultiplexLogger(BaseLogger& one, bool ownOne, BaseLogger& two, bool ownTwo)
		{
			m_loggers.push_back(LoggerInfo(&one, ownOne));
			m_loggers.push_back(LoggerInfo(&two, ownTwo));
		}

		~MultiplexLogger()
		{
			for( std::vector<LoggerInfo>::iterator It = m_loggers.begin();
				 It != m_loggers.end();
				 It++ )
			{
				if( (*It).owned )
					delete (*It).logger;
			}
		}
		
		void addLogger(BaseLogger* logger)		{ m_loggers.push_back(LoggerInfo(logger, false)); }
		void addLogger(BaseLogger& logger)		{ m_loggers.push_back(LoggerInfo(&logger, false)); }

		void addLogger(BaseLogger* logger, bool owned)		{ m_loggers.push_back(LoggerInfo(logger, owned)); }
		void addLogger(BaseLogger& logger, bool owned)		{ m_loggers.push_back(LoggerInfo(&logger, owned)); }

		virtual bool sendLogMessage(LogData* logData)
		{
			bool deleteMessage = true;

			for( std::vector<LoggerInfo>::iterator It = m_loggers.begin();
				 It != m_loggers.end();
				 It++ )
			{
				deleteMessage = deleteMessage && (*It).logger->sendLogMessage(logData);
			}

			return deleteMessage;
		}
	};

	// Filtering logger.  Will not forward all messages less than a given level.
	class FilteringLogger : public BaseLogger
	{
	private:
		loglevel_t		m_lowestLevelAllowed;
		BaseLogger*		m_forwardTo;
		bool			m_owned;

	public:
		FilteringLogger(loglevel_t level, BaseLogger* forwardTo)
			: m_lowestLevelAllowed(level), m_forwardTo(forwardTo), m_owned(false)
		{ }

		FilteringLogger(loglevel_t level, BaseLogger& forwardTo)
			: m_lowestLevelAllowed(level), m_forwardTo(&forwardTo), m_owned(false)
		{ }

		FilteringLogger(loglevel_t level, BaseLogger* forwardTo, bool owned)
			: m_lowestLevelAllowed(level), m_forwardTo(forwardTo), m_owned(owned)
		{ }

		FilteringLogger(loglevel_t level, BaseLogger& forwardTo, bool owned)
			: m_lowestLevelAllowed(level), m_forwardTo(&forwardTo), m_owned(owned)
		{ }

		~FilteringLogger()
		{
			if( m_owned )
				delete m_forwardTo;
		}

		virtual bool sendLogMessage(LogData* logData)
		{
			if( logData->level >= m_lowestLevelAllowed )
				return m_forwardTo->sendLogMessage(logData);
			else
				return true;
		}
	};

	// Logger that moves all processing of log messages to a background thread. 
	// Only include if we have support for threading.
#ifndef CPPLOG_NO_THREADING
	class BackgroundLogger : public BaseLogger
	{
	private:
		BaseLogger*					m_forwardTo;
		concurrent_queue<LogData*>	m_queue;

		boost::thread				m_backgroundThread;
		LogData*					m_dummyItem;

		void backgroundFunction()
		{
			LogData* nextLogEntry;
			bool deleteMessage = true;

			do
			{
				m_queue.wait_and_pop(nextLogEntry);

				if( nextLogEntry != m_dummyItem )
					deleteMessage = m_forwardTo->sendLogMessage(nextLogEntry);

				if( deleteMessage )
					delete nextLogEntry;
			} while( nextLogEntry != m_dummyItem );
		}

		void Init()
		{
			// Create dummy item.
			m_dummyItem = new LogData(LL_TRACE);

			// And create background thread.
			m_backgroundThread = boost::thread(&BackgroundLogger::backgroundFunction, this);
		}

	public:
		BackgroundLogger(BaseLogger* forwardTo)
			: m_forwardTo(forwardTo) 
		{ 
			Init();
		}

		BackgroundLogger(BaseLogger& forwardTo)
			: m_forwardTo(&forwardTo) 
		{
			Init();
		}

		~BackgroundLogger()
		{
			// Push our "dummy" item on the queue ...
			m_queue.push(m_dummyItem);

			// ... and wait for thread to terminate.
			m_backgroundThread.join();

			// NOTE: The loop will free the dummy item for us, we can ignore it.
		}

		virtual bool sendLogMessage(LogData* logData)
		{
			m_queue.push(logData);

			// Don't delete - the background thread should handle this.
			return false;
		}

	};

#endif //CPPLOG_NO_THREADING

	// Seperate namespace for loggers that use templates.
	namespace templated
	{
		// Filtering logger that accepts the level as a template parameter.
		// This will be slightly faster at runtime, as the if statement will
		// be performed on a constant value, as opposed to needing a memory
		// lookup (as with FilteringLogger)
		template <loglevel_t lowestLevel = LL_TRACE>
		class TFilteringLogger : public BaseLogger
		{
			BaseLogger* m_forwardTo;

		public:
			TFilteringLogger(BaseLogger* forwardTo)
				: m_forwardTo(forwardTo)
			{ }

			virtual bool sendLogMessage(LogData* logData)
			{
				if( logData->level >= lowestLevel )
					return m_forwardTo->sendLogMessage(logData);
				else
					return true;
			}
		};

		// TODO: Implement others?
	};
};

// Our logging macros.

// Default macros - log, and don't log something.
#define LOG_LEVEL(level, logger)	cpplog::LogMessage(__FILE__, __LINE__, (level), logger).getStream()
#define LOG_NOTHING(level, logger)	true ? (void)0 : cpplog::helpers::VoidStreamClass() & LOG_LEVEL(level, logger)

// Series of debug macros, depending on what we log.
#if CPPLOG_FILTER_LEVEL <= LL_TRACE
#define LOG_TRACE(logger)	LOG_LEVEL(LL_TRACE, logger)
#else
#define LOG_TRACE(logger)	LOG_NOTHING(LL_TRACE, logger)
#endif 

#if CPPLOG_FILTER_LEVEL <= LL_DEBUG
#define LOG_DEBUG(logger)	LOG_LEVEL(LL_DEBUG, logger)
#else
#define LOG_DEBUG(logger)	LOG_NOTHING(LL_DEBUG, logger)
#endif 

#if CPPLOG_FILTER_LEVEL <= LL_INFO
#define LOG_INFO(logger)	LOG_LEVEL(LL_INFO, logger)
#else
#define LOG_INFO(logger)	LOG_NOTHING(LL_INFO, logger)
#endif

#if CPPLOG_FILTER_LEVEL <= LL_WARN
#define LOG_WARN(logger)	LOG_LEVEL(LL_WARN, logger)
#else
#define LOG_WARN(logger)	LOG_NOTHING(LL_WARN, logger)
#endif

#if CPPLOG_FILTER_LEVEL <= LL_ERROR
#define LOG_ERROR(logger)	LOG_LEVEL(LL_ERROR, logger)
#else
#define LOG_ERROR(logger)	LOG_NOTHING(LL_ERROR, logger)
#endif

// Note: Always logged.
#define LOG_FATAL(logger)	LOG_LEVEL(LL_FATAL, logger)



// Debug macros - only logged in debug mode.
#ifdef _DEBUG
#define DLOG_TRACE(logger)	LOG_TRACE(logger)
#define DLOG_DEBUG(logger)	LOG_DEBUG(logger)
#define DLOG_INFO(logger)	LOG_INFO(logger)
#define DLOG_WARN(logger)	LOG_WARN(logger)
#define DLOG_ERROR(logger)	LOG_ERROR(logger)
#else
#define DLOG_TRACE(logger)	LOG_NOTHING(LL_TRACE, logger)
#define DLOG_DEBUG(logger)	LOG_NOTHING(LL_DEBUG, logger)
#define DLOG_INFO(logger)	LOG_NOTHING(LL_INFO,  logger)
#define DLOG_WARN(logger)	LOG_NOTHING(LL_WARN,  logger)
#define DLOG_ERROR(logger)	LOG_NOTHING(LL_ERROR, logger)
#endif

// Note: Always logged.
#define DLOG_FATAL(logger)	LOG_FATAL(logger)

// Aliases - so we can do:
//		LOG(LL_FATAL, logger)
#define LOG_LL_TRACE	LOG_TRACE
#define LOG_LL_DEBUG	LOG_DEBUG
#define LOG_LL_INFO		LOG_INFO
#define LOG_LL_WARN		LOG_WARN
#define LOG_LL_ERROR	LOG_ERROR
#define LOG_LL_FATAL	LOG_FATAL

#define DLOG_LL_TRACE	DLOG_TRACE
#define DLOG_LL_DEBUG	DLOG_DEBUG
#define DLOG_LL_INFO	DLOG_INFO
#define DLOG_LL_WARN	DLOG_WARN
#define DLOG_LL_ERROR	DLOG_ERROR
#define DLOG_LL_FATAL	DLOG_FATAL


// Helper - if you want to do:
//		LOG(FATAL, logger)
#define LOG(level, logger)	LOG_##level(logger)
#define DLOG(level, logger)	DLOG_##level(logger)


// Log conditions.
#define LOG_IF(level, logger, condition)		!(condition) ? (void)0 : cpplog::helpers::VoidStreamClass() & LOG(##level, logger)
#define LOG_IF_NOT(level, logger, condition)	LOG_IF(##level, logger, !(condition))

// Debug conditions.
#ifdef _DEBUG
#define DLOG_IF(level, logger, condition)		!(condition) ? (void)0 : cpplog::helpers::VoidStreamClass() & LOG(level, logger)
#else
#define DLOG_IF(level, logger, condition)		(true || !(condition)) ? (void)0 : \
													cpplog::VoidStreamClass() & LOG(level, logger)
#endif
#define DLOG_IF_NOT(level, logger, condition)	DLOG_IF(level, logger, !(condition))

// Assertion helpers.
#define LOG_ASSERT(logger, condition)			LOG_IF(LL_FATAL, logger, !(condition)) << "Assertion failed: " #condition
#define DLOG_ASSERT(logger, condition)			DLOG_IF(LL_FATAL, logger, !(condition)) << "Assertion failed: " #condition


// Only include further helper macros if we are supposed to.
#ifndef CPPLOG_NO_HELPER_MACROS

// The following CHECK_* functions act similar to a LOG_ASSERT, but with a bit more
// readability.

#define __CHECK(logger, condition, print)	LOG_IF(LL_FATAL, logger, !(condition)) << "Check failed: " ## print ## ": "

#define CHECK(logger, condition)			__CHECK(logger, condition, #condition)

#define CHECK_EQUAL(logger, ex1, ex2)		__CHECK(logger, (ex1) == (ex2), #ex1 " == " #ex2)
#define CHECK_LT(logger, ex1, ex2)			__CHECK(logger, (ex1) < (ex2), #ex1 " < " #ex2)
#define CHECK_GT(logger, ex1, ex2)			__CHECK(logger, (ex1) > (ex2), #ex1 " > " #ex2)
#define CHECK_LE(logger, ex1, ex2)			__CHECK(logger, (ex1) <= (ex2), #ex1 " <= " #ex2)
#define CHECK_GE(logger, ex1, ex2)			__CHECK(logger, (ex1) >= (ex2), #ex1 " >= " #ex2)

#define CHECK_NE(logger, ex1, ex2)			__CHECK(logger, (ex1) != (ex2), #ex1 " != " #ex2)
#define CHECK_NOT_EQUAL(logger, ex1, ex2)	__CHECK(logger, (ex1) != (ex2), #ex1 " != " #ex2)


// String helpers.
#define CHECK_STREQ(logger, s1, s2)			__CHECK(logger, strcmp((s1), (s2)) == 0, "") << s1 << " == " << s2
#define CHECK_STRNE(logger, s1, s2)			__CHECK(logger, strcmp((s1), (s2)) != 0, "") << s1 << " != " << s2

// NULL helpers.
#define CHECK_NULL(logger, exp)				__CHECK(logger, (exp) == NULL, #exp " == NULL")
#define CHECK_NOT_NULL(logger, exp)			__CHECK(logger, (exp) != NULL, #exp " != NULL")



// Debug versions of above.
#ifdef _DEBUG
#define DCHECK(logger, condition)			CHECK(logger, condition)
#define DCHECK_EQUAL(logger, ex1, ex2)		CHECK_EQUAL(logger, ex1, ex2)		
#define DCHECK_LT(logger, ex1, ex2)			CHECK_LT(logger, ex1, ex2)			
#define DCHECK_GT(logger, ex1, ex2)			CHECK_GT(logger, ex1, ex2)			
#define DCHECK_LE(logger, ex1, ex2)			CHECK_LE(logger, ex1, ex2)			
#define DCHECK_GE(logger, ex1, ex2)			CHECK_GE(logger, ex1, ex2)			
#define DCHECK_NE(logger, ex1, ex2)			CHECK_NE(logger, ex1, ex2)			
#define DCHECK_NOT_EQUAL(logger, ex1, ex2)	CHECK_NOT_EQUAL(logger, ex1, ex2)	
#define DCHECK_STREQ(logger, s1, s2)		CHECK_STREQ(logger, s1, s2)		
#define DCHECK_STRNE(logger, s1, s2)		CHECK_STRNE(logger, s1, s2)		
#define DCHECK_NULL(logger, exp)			CHECK_NULL(logger, exp)			
#define DCHECK_NOT_NULL(logger, exp)		CHECK_NOT_NULL(logger, exp)		
#else
#define DCHECK(logger, condition)			while(false) CHECK(logger, condition)
#define DCHECK_EQUAL(logger, ex1, ex2)		while(false) CHECK_EQUAL(logger, ex1, ex2)		
#define DCHECK_LT(logger, ex1, ex2)			while(false) CHECK_LT(logger, ex1, ex2)			
#define DCHECK_GT(logger, ex1, ex2)			while(false) CHECK_GT(logger, ex1, ex2)			
#define DCHECK_LE(logger, ex1, ex2)			while(false) CHECK_LE(logger, ex1, ex2)			
#define DCHECK_GE(logger, ex1, ex2)			while(false) CHECK_GE(logger, ex1, ex2)			
#define DCHECK_NE(logger, ex1, ex2)			while(false) CHECK_NE(logger, ex1, ex2)			
#define DCHECK_NOT_EQUAL(logger, ex1, ex2)	while(false) CHECK_NOT_EQUAL(logger, ex1, ex2)	
#define DCHECK_STREQ(logger, s1, s2)		while(false) CHECK_STREQ(logger, s1, s2)		
#define DCHECK_STRNE(logger, s1, s2)		while(false) CHECK_STRNE(logger, s1, s2)		
#define DCHECK_NULL(logger, exp)			while(false) CHECK_NULL(logger, exp)			
#define DCHECK_NOT_NULL(logger, exp)		while(false) CHECK_NOT_NULL(logger, exp)		
#endif


#endif //CPPLOG_NO_HELPER_MACROS

#endif //_CPPLOG_H
