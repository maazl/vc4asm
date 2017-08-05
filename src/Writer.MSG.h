/*
 * Writer.MSG.h
 *
 *  Created on: 05.08.2017
 *      Author: mueller
 */

#include <inttypes.h>

static const constexpr struct MSG
{
	#define E(name, major, minor, text, ...) const msgTemplate<__VA_ARGS__> name = { { MS_Writer, ERROR, major, minor }, text }
	#define F(name, major, minor, text, ...) const msgTemplate<__VA_ARGS__> name = { { MS_Writer, FATAL, major, minor }, text }
	// Entries must be ordered by major, minor.
	E(UNTERMINATED_PLACEHOLDER        , 10, 1, "File '%s' contains unterminated placeholder at byte %zu: ___%.15s...", const char*, size_t, const char*);
	E(UNKNOWN_PLACEHOLDER             , 10, 2, "File '%s' contains unknown placeholder at byte %zu: ___%.15s", const char*, size_t, const char*);

	#undef E
	#undef F

	MAKE_MSGTEMPLATE_CONTAINER
} MSG;
