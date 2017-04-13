/*
 * utils.cpp
 *
 *  Created on: 30.11.2014
 *      Author: mueller
 */

#include "utils.h"

#include <cstdio>
#include <cstdlib>
#include <climits>
#include <unistd.h>
#include <errno.h>


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

string exepath;

void setexepath(const char* argv0)
{	{	char buf[PATH_MAX];
		// try /proc/self/exe (Linux)
		int rc = readlink("/proc/self/exe", buf, sizeof buf);
		if (rc > 0)
		{	buf[rc] = 0;
			argv0 = buf;
		} else
		{	// try absolute or relative path.
			char* path = realpath(argv0, buf);
			if (path != NULL)
				argv0 = buf;
	}	}
	// find last of [/\:]
	int p = strlen(argv0);
	while (p)
	{	char c = argv0[--p];
		if (c == '/' || c == '\\' || c == ':')
		{	++p;
			break;
	}	}
	exepath = string(argv0, p);
}
string fgetstring(FILE* fh, size_t maxlen)
{	char* buf = (char*)alloca(++maxlen);
	if (!fgets(buf, maxlen, fh))
	{	if (feof(fh))
			return string();
		throw stringf("Failed to read line from file: %s", strerror(errno));
	}
	string ret(buf);
	if (ret.length() && ret[ret.length()-1] != '\n' && !feof(fh))
	{	// incomplete, long line => skip remaining part
		(void)fscanf(fh, "%*[^\n]");
		fgetc(fh); // discard newline as well
	}
	return ret;
}

