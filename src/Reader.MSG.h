/*
 * Reader.MSG.h
 *
 *  Created on: 06.08.2017
 *      Author: mueller
 */

static const constexpr struct MSG
{
	#define F(name, major, minor, text, ...) const msgTemplate<__VA_ARGS__> name = { { MS_vc4dis, FATAL, major, minor }, text }
	#define E(name, major, minor, text, ...) const msgTemplate<__VA_ARGS__> name = { { MS_vc4dis, ERROR, major, minor }, text }
	#define W(name, major, minor, text, ...) const msgTemplate<__VA_ARGS__> name = { { MS_vc4dis, WARN, major, minor }, text }
	#define I(name, major, minor, text, ...) const msgTemplate<__VA_ARGS__> name = { { MS_vc4dis, INFO, major, minor }, text }
	// Entries must be ordered by major, minor.

	// binary input
	W(BINARY_FILE_ODD_SIZED           ,110, 1, "File size of source '%s' is not a multiple of 64 bit.", const char*);
	// hex input
	W(HEX_FILE_ODD_SIZED              ,120, 0, "File '%s' must contain an even number of 32 bit words.", const char*);
	W(HEX_FILE_NOT_PARSABLE           ,121, 0, "File '%s' contains not parsable input '%s' at byte %li.", const char*, const char*, long);

	#undef F
	#undef E
	#undef W
	#undef I

	MAKE_MSGTEMPLATE_CONTAINER
} MSG;
