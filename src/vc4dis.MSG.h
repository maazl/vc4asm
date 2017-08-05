/*
 * vc4dis.MSG.h
 *
 *  Created on: 05.08.2017
 *      Author: mueller
 */

static const constexpr struct MSG
{
	#define F(name, major, minor, text, ...) const msgTemplate<__VA_ARGS__> name = { { MS_vc4dis, FATAL, major, minor }, text }
	#define E(name, major, minor, text, ...) const msgTemplate<__VA_ARGS__> name = { { MS_vc4dis, ERROR, major, minor }, text }
	#define W(name, major, minor, text, ...) const msgTemplate<__VA_ARGS__> name = { { MS_vc4dis, WARN, major, minor }, text }
	#define I(name, major, minor, text, ...) const msgTemplate<__VA_ARGS__> name = { { MS_vc4dis, INFO, major, minor }, text }
	// Entries must be ordered by major, minor.

	// Options
	E(INVALID_OPTION_ARGUMENT         , 11, 0, "Invalid argument %i to option -%c.", int, char);
	// program flow
	I(DISASSEMBLING                   , 50, 0, "Disassembling '%s' ...", const char*);
	W(FILE_EMPTY                      , 60, 0, "Couldn't read any data from file '%s', skipping.", const char*);
	// binary input
	W(BINARY_FILE_ODD_SIZED           ,110, 0, "File size %li of source '%s' is not a multiple of 64 bit.", long, const char*);
	// hex input
	W(HEX_FILE_ODD_SIZED              ,120, 0, "File '%s' must contain an even number of 32 bit words.", const char*);
	W(HEX_FILE_NOT_PARSABLE           ,121, 0, "File '%s' contains not parsable input %s.", const char*, const char*);

	#undef F
	#undef E
	#undef W
	#undef I

	MAKE_MSGTEMPLATE_CONTAINER
} MSG;
