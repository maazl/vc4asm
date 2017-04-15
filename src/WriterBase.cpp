/*
 * WriterBase.cpp
 *
 *  Created on: 02.04.2017
 *      Author: mueller
 */

#include "WriterBase.h"
#include "utils.h"

#include <cstring>
#include <cstdarg>


void WriterBase::WriteWithTemplate(const char* templname, function<bool(const string&)> atplaceholder)
{	// read template
	string tpl = readcomplete(templname);
	// write with substitutions
	string::size_type start = 0;
	while (true)
	{	auto p = tpl.find("___", start);
		if (p == string::npos)
		{	checkedwrite(Target, tpl.data() + start, tpl.size() - start);
			return;
		}
		// Found ___, write raw content
		checkedwrite(Target, tpl.data() + start, p - start);
		// find next delimiter
		start = p + 3;
		p = tpl.find("___", start);
		if (p == string::npos)
			throw stringf("\"%s\" contains unterminated placeholder at byte %zu: ___%.15s...", templname, start, tpl.c_str() + start);
		string placeholder(tpl, start, p - start);
		if (!atplaceholder(placeholder))
			throw stringf("\"%s\" contains unknown placeholder at byte %zu: ___%.15s", templname, start, placeholder.c_str());
		// skip placeholder
		start = p + 3;
	}
}

