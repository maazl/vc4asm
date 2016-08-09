/*
 * Message.cpp
 *
 *  Created on: 04.06.2016
 *      Author: mueller
 */

#include "Message.h"
#include "utils.h"

#include <cstdarg>


Message::Message(const char* format, ...)
{	va_list va;
	va_start(va, format);
	const auto&& msg = vstringf(format, va);
	va_end(va);
	static_cast<string&>(*this) = msg;
}
