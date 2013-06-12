#include <iostream>

#ifdef WIN32
#define __func__ __FUNCTION__
#endif

// This define must be before including "cpplog.hpp"
#define LOG_LEVEL(level, logger) CustomLogMessage(__FILE__, __func__, __LINE__, (level), logger).getStream()
#include "../cpplog.hpp"

class CustomLogMessage : public cpplog::LogMessage
{
public:
    CustomLogMessage(const char* file, const char* function,
        unsigned int line, cpplog::loglevel_t logLevel,
        cpplog::BaseLogger &outputLogger)
    : cpplog::LogMessage(file, line, logLevel, outputLogger, false),
      m_function(function)
    {
        InitLogMessage();
    }

    static const char* shortLogLevelName(cpplog::loglevel_t logLevel)
    {
        switch( logLevel )
        {
            case LL_TRACE: return "T";
            case LL_DEBUG: return "D";
            case LL_INFO:  return "I";
            case LL_WARN:  return "W";
            case LL_ERROR: return "E";
            case LL_FATAL: return "F";
            default:       return "O";
        };
    }

protected:
    virtual void InitLogMessage()
    {
        m_logData->stream
            << "["
            << m_logData->fileName << ":"
            << m_function          << ":"
            << m_logData->line
            << "]["
            << shortLogLevelName(m_logData->level)
            << "] ";
    }
private:
    const char *m_function;
};

int main()
{
    cpplog::StdErrLogger slog;
    LOG_WARN(slog) << "Custom log format.";

    return 0;
}

