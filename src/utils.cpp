/*
 * utils.cpp
 *
 *  Created on: 30.11.2014
 *      Author: mueller
 */

#include "utils.h"

#include <cstdio>
#ifndef _LIBCPP_VERSION // libstdc++
  #include <tr1/cstdint>
#else                   // libc++
  #include <cstdint>
#endif

string vstringf(const char* format, va_list va)
{	char buf[256];
	// save va for the 2nd try
	va_list va2;
	va_copy(va2, va);
	// first try with small buffer
	auto count = vsnprintf(buf, sizeof buf, format, va);
	if (count < 0)
		return string(); // TODO: throw an exception
	if ((size_t)count < sizeof buf)
	{	va_end(va2);
		return string(buf, count);
	}
	// failed because of buffer overflow => retry with matching buffer
	string ret;
	ret.resize(count+1);
	vsnprintf(&ret[0], count+1, format, va2);
	ret.resize(count);
	va_end(va2);
	return ret;
}
string stringf(const char* format, ...)
{	va_list va;
	va_start(va, format);
	string ret = vstringf(format, va);
	va_end(va);
	return ret;
}

string relpath(const string& context, const string& rel)
{	if (rel.size() && rel.front() == '/')
		return rel;
	return string(context, 0, context.rfind('/')+1) + rel;
}
