#pragma once
#ifndef _OUTPUT_DEBUG_STREAM_H
#define _OUTPUT_DEBUG_STREAM_H

#include <windows.h>

template <class Elem>
class outputdebug_buf: public std::basic_stringbuf<Elem>
{
public:
	virtual int sync ( )
	{
		output_debug_string(str().c_str());
		return 0;
	}
private:
	void output_debug_string(const Elem* e);
};

template<>
void outputdebug_buf<char>::output_debug_string(const char* e)
{
	::OutputDebugStringA(e);
}

template<>
void outputdebug_buf<wchar_t>::output_debug_string(const wchar_t* e)
{
	::OutputDebugStringW(e);
}

template <class _Elem>
class outputdebug_stream: public std::basic_ostream<_Elem>
{
public:
	outputdebug_stream() : std::basic_ostream<_Elem>(new outputdebug_buf<_Elem>())
	{}

	~outputdebug_stream(){ delete rdbuf(); }
};

typedef outputdebug_stream<char> dbgwin_stream;
typedef outputdebug_stream<wchar_t> wdbgwin_stream;

#endif
