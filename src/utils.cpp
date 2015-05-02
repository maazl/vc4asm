/*
 * utils.cpp
 *
 *  Created on: 30.11.2014
 *      Author: mueller
 */

#include "utils.h"

#include <cstdio>


string vstringf(const char* format, va_list va)
{	int count = vsnprintf(NULL, 0, format, va);
	string ret;
	ret.resize(count);
	vsnprintf(&ret[0], count+1, format, va);
	return ret;
}
string stringf(const char* format, ...)
{	va_list va;
	va_start(va, format);
	string&& ret = vstringf(format, va);
	va_end(va);
	return ret;
}

string relpath(const string& context, const string& rel)
{	if (rel.size() && rel.front() == '/')
		return move(rel);
	return string(context, 0, context.rfind('/')+1) + rel;
}
