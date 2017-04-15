/*
 * utils.cpp
 *
 *  Created on: 30.11.2014
 *      Author: mueller
 */

#include "utils.h"

#include <cstdlib>
#include <climits>
#include <cctype>
#include <cstdarg>
#include <alloca.h>
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

const char* strippath(const char* filepath)
{	// find last of / \ :
	size_t len = strlen(filepath);
	while (len--)
	{	char c = filepath[len];
		if (c == '/' || c == '\\' || c == ':')
			break;
	}
	return filepath + len + 1;
}

string stripextension(const char* filename)
{	const char* cp = strrchr(filename, '.');
	return cp ? string(filename, cp) : filename;
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

FILE* checkedopen(const char* file, const char* mode)
{	FILE* ret = fopen(file, mode);
	if (ret == NULL)
		throw stringf("Failed to open \"%s\" for %s: %s", file, strchr(mode, 'w') ? "writing" : "reading", strerror(errno));
	return ret;
}

void checkedwrite(FILE* fh, const void* data, size_t size)
{	switch (fwrite(data, size, 1, fh))
	{default:
		throw stringf("Failed to write to file: %s", strerror(errno));
	 case 0:
		throw string("Failed to write to file: Disk full");
	 case 1:;
	}
}

void checkedfprintf(FILE* fh, const char* fmt, ...)
{	va_list va;
	va_start(va, fmt);
	int ret = vfprintf(fh, fmt, va);
	va_end(va);
	if (ret < 0)
		throw stringf("Failed to write to file: %s", strerror(errno));
}

string readcomplete(const char* file)
{	FILEguard fh(checkedopen(file, "r"));
	if (fseek(fh, 0, SEEK_END))
		throw stringf("Failed to seek to end of \"%s\": %s", file, strerror(errno));
	long size = ftell(fh);
	if (size == -1)
		throw stringf("Failed to get size of file \"%s\": %s", file, strerror(errno));
	if (size > 1<<16)
		throw stringf("File \"%s\" is no reasonable template.", file);
	if (fseek(fh, 0, SEEK_SET))
		throw stringf("Failed to seek to start of \"%s\": %s", file, strerror(errno));
	string ret;
	ret.resize(size);
	if (fread(&ret.front(), size, 1, fh) != 1)
		throw stringf("Failed to read file content of \"%s\": %s", file, strerror(errno));
	return ret;
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

void replacenonalphanum(string& value, char repl)
{	for (auto& c : value)
		if (!isalnum(c))
			c = repl;
}
