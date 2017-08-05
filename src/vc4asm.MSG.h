/*
 * vc4asm.MSG.h
 *
 *  Created on: 04.08.2017
 *      Author: mueller
 */

static const constexpr struct MSG
{
	#define F(name, major, minor, text, ...) const msgTemplate<__VA_ARGS__> name = { { MS_vc4asm, FATAL, major, minor }, text }
	#define E(name, major, minor, text, ...) const msgTemplate<__VA_ARGS__> name = { { MS_vc4asm, ERROR, major, minor }, text }
	#define W(name, major, minor, text, ...) const msgTemplate<__VA_ARGS__> name = { { MS_vc4asm, WARN, major, minor }, text }
	#define I(name, major, minor, text, ...) const msgTemplate<__VA_ARGS__> name = { { MS_vc4asm, INFO, major, minor }, text }
	// Entries must be ordered by major, minor.

	E(INCLUDE_NOT_FOUND                 , 10, 0, "'%s' not found in include path.", const char*);
	F(ABORT_BECAUSE_OF_ERROR            ,250, 0, "Aborted because of earlier errors.");

	#undef F
	#undef E
	#undef W
	#undef I

	MAKE_MSGTEMPLATE_CONTAINER
} MSG;
