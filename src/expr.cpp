/*
 * expr.cpp
 *
 *  Created on: 16.11.2014
 *      Author: mueller
 */

#include "expr.h"

#include <stdarg.h>


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
	const string& ret = vstringf(format, va);
	va_end(va);
	return ret;
}

string exprValue::toString() const
{switch (Type)
	{default:
		return string();
	 case V_INT:
		return stringf("0x%x", uValue);
	 case V_FLOAT:
		return stringf("%g", fValue);
	 case V_REG:
		// TODO
		return string();
	}
}

