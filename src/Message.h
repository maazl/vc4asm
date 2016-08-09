/*
 * Message.h
 *
 *  Created on: 04.06.2016
 *      Author: mueller
 */

#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <string>
using namespace std;


/// Severity of assembler messages.
enum severity : unsigned char
{	ERROR
,	WARNING
,	INFO
};

/// Exception thrown when evaluation Fails
/// @remarks This is only a typed string so far.
/// Future versions might add additional error information.
struct Message : string
{	Message(const char* fmt, ...);
};

#endif

