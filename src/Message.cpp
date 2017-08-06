/*
 * Message.cpp
 *
 *  Created on: 04.06.2016
 *      Author: mueller
 */

#include "Message.h"
#include "utils.h"

#include <cstdarg>
#include <cstdio>


static const char* Severities[] =
{	""
,	"Debug "
,	"Info "
,	"Warning "
,	"ERROR "
,	"FATAL "
};

string msgID::toString() const
{	return stringf(Minor() ? "%s%c%u.%u" : "%s%c%u", Severities[Severity()], Source(), Major(), Minor());
}


const msgTemplateBase* msgTemplateBase::FindTemplateByID(const msgTemplateBase* begin, const msgTemplateBase* end, unsigned idval)
{	idval &= ~0xff;
	while (begin != end)
	{	const msgTemplateBase* m = begin + ((end - begin) >> 1);
		unsigned mval = m->ID.Value & ~0xff;
		if (mval > idval)
			begin = m + 1;
		else if (mval == idval)
			return m;
		else
			end = m;
	}
	return NULL;
}


string Message::toString() const noexcept
{	return ID.toString() + ": " + Text;
}

void Message::print() const
{	if (ID.Severity() == NONE)
		return;
	const string& text = toString();
	fwrite(text.data(), 1, text.length(), stderr);
	fputc('\n', stderr);
}

void Message::printHandler(const Message& msg)
{	msg.print();
}

const char* Message::what() const noexcept
{	return (Buffer = toString()).c_str();
}
