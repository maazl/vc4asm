/*
 * Message.h
 *
 *  Created on: 04.06.2016
 *      Author: mueller
 */

#include "utils.h"

#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <cinttypes>
#include <string>
#include <functional>
#include <utility>
using namespace std;


/// Severity of message.
enum severity : uint8_t
{	NONE  ///< Never show this message (internal).
,	DEBUG ///< Diagnostic message.
,	INFO  ///< Informational message.
,	WARN  ///< Warning.
,	ERROR ///< Error.
,	FATAL ///< Fatal error.
};

/// Message source context.
enum msgsrc : uint8_t
{	MS_unknown      = ' '
	// backends
,	MS_Assembler    = 'A'
,	MS_Disassembler = 'D'
,	MS_Evaluator    = 'E'
,	MS_Parser       = 'P'
,	MS_Validator    = 'V'
,	MS_Writer       = 'W'
	// frontends and tools
,	MS_vc4asm       = 'a'
,	MS_vc4dis       = 'd'
,	MS_utils        = 'u'
};

/// Type safe wrapper around integer to represent a message ID
/// build of the components Source(). Major(), Minor() and Severity()
/// in that order.
struct msgID
{	unsigned           Value;
	// manual packing to ensure ordering
	constexpr msgsrc   Source() const noexcept   { return (msgsrc)(Value>>24); }
	constexpr uint8_t  Major() const noexcept    { return (uint8_t)(Value>>16); }
	constexpr uint8_t  Minor() const noexcept    { return (uint8_t)(Value>>8); }
	constexpr severity Severity() const noexcept { return (severity)Value; }
	/// As human readable string
	string toString() const;
	/// construction from components
	constexpr msgID(msgsrc source, severity severity, uint8_t major, uint8_t minor) noexcept
	:	Value((source<<24)|(major<<16)|(minor<<8)|severity) {}
	/// construction from integer - be careful
	constexpr explicit msgID(int value) noexcept : Value(value) {}
	// comparison
	constexpr friend bool operator==(msgID l, msgID r) noexcept { return l.Value == r.Value; }
	constexpr friend bool operator!=(msgID l, msgID r) noexcept { return l.Value != r.Value; }
	constexpr friend bool operator< (msgID l, msgID r) noexcept { return l.Value <  r.Value; }
	constexpr friend bool operator<=(msgID l, msgID r) noexcept { return l.Value <= r.Value; }
	constexpr friend bool operator> (msgID l, msgID r) noexcept { return l.Value >  r.Value; }
	constexpr friend bool operator>=(msgID l, msgID r) noexcept { return l.Value >= r.Value; }
};


/// Generic message class. In fact a text string with a message ID.
/// You may inherit from this class to provide further context information.
struct Message : public exception
{	msgID       ID;   ///< Message ID
	string      Text; ///< Message text
	/// Create message
	Message(msgID id, string&& msg) noexcept : ID(id), Text(move(msg)) {}

	/// Convert message to human readable string.
	virtual string toString() const noexcept;

	/// Print the message to \c stderr.
	void print() const;
	/// Default message handler => print the message to \c stderr.
	/// @remarks static version to be bound to std::function.
	static void printHandler(const Message& msg);

 private: // Keep the old style C++ API happy.
	mutable string Buffer;///< Buffer to keep last result of what.
	virtual const char* what() const noexcept;
};


/// Untyped template for message.
struct msgTemplateBase
{	msgID       ID;   ///< Message ID
	const char* Text; ///< Message text template
	/// Internal helper function for getByID.
	static const msgTemplateBase* FindTemplateByID(const msgTemplateBase* begin, const msgTemplateBase* end, unsigned idval);
};

/// Strongly typed template for message.
/// @tparam A type of message arguments.
template <typename ...A>
struct msgTemplate : msgTemplateBase
{	constexpr msgTemplate(msgID id, const char* text) noexcept : msgTemplateBase{ id, text } {}
	/// Format message text with parameters.
	/// @param args Parameter pack for text template.
	string format(A... args) const                                    { return stringf(Text, args...); }
	/// Convenient way to create a message.
	/// @remarks This is also a work around for not working implicit conversions with template functions with parameter packs.
	Message toMsg(A... args) const                                    { return Message(ID, format(args...)); }
};
/// Specialization for simple messages without parameters.
template <>
struct msgTemplate<> : msgTemplateBase
{	constexpr msgTemplate(msgID id, const char* text) noexcept : msgTemplateBase{ id, text } {}
	string format() const                                             { return Text; }
	/// Convenient way to create a message.
	/// @remarks This is also a work around for not working implicit conversions with template functions with parameter packs.
	Message toMsg() const                                             { return Message(ID, Text); }
};

/// Turns a structure containing only msgTemplate fields into a container.
/// The structure must not contain anything else but message templates,
/// and the templates must be ordered by msgID.
#define MAKE_MSGTEMPLATE_CONTAINER \
	/*** Return pointer to the first message template in this container. */\
	const           msgTemplateBase* begin() const noexcept           { return (const msgTemplateBase*)this; } \
	/*** Return number of message templates in this container. */\
	constexpr       size_t size() const noexcept                      { return sizeof(*this) / sizeof(msgTemplateBase); } \
	/*** Return pointer past the last message template in this container. */\
	const           msgTemplateBase* end() const noexcept             { return begin() + size(); } \
	/*** Access the i-th message template in this container. */\
	const           msgTemplateBase& operator[](unsigned index) const { return begin()[index]; } \
	/*** Search for a message template with a given ID in this container. The severity is ignored. */\
	const           msgTemplateBase* getByID(msgID id) const          { return msgTemplateBase::FindTemplateByID(begin(), end(), id.Value); }


#endif
