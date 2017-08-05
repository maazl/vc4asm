/*
 * Eval.MSG.h
 *
 *  Created on: 28.05.2017
 *      Author: mueller
 */

#include <inttypes.h>

static const constexpr struct MSG
{
	#define E(name, major, minor, text, ...) const msgTemplate<__VA_ARGS__> name = { { MS_Evaluator, ERROR, major, minor }, text }
	#define F(name, major, minor, text, ...) const msgTemplate<__VA_ARGS__> name = { { MS_Evaluator, FATAL, major, minor }, text }
	// Entries must be ordered by major, minor.
	E(UNARY_OP_WRONG_TYPE   , 10, 1, "Cannot apply unary operator %s to operand of type %s.", const char*, const char*);
	E(BINARY_OP_WRONG_TYPES , 10, 2, "Cannot apply operator %s to operands of type %s and %s.", const char*, const char*, const char*);
	E(UNARY_OP_AFTER_VALUE  , 20, 1, "Unexpected unary operator %s after numeric expression.", const char*);
	E(BINARY_OP_WO_LHS_VALUE, 21, 1, "Binary operator %s needs left hand side expression.", const char*);
	E(MISSING_OP_BEFORE_VALUE,22, 2, "Missing operator before value '%s'.", const char*);
	E(MISSING_VALUE         , 23, 1, "Incomplete expression: expected value.");
	E(MISSING_CLOSING_BRACE , 23, 2, "Incomplete expression: missing ')'.");
	E(REGNO_OUT_OF_RANGE    , 50, 0, "Register number %" PRId64 " out of range.", int64_t);
	E(ROT_REGISTER_NOT_R5   , 51, 1, "Vector rotation are only allowed by constant or by r5");
	E(ROT_REGISTER_AND_VALUE, 51, 2, "Cannot apply additional offset to r5 vector rotation");
	E(CANNOT_NEGATE_PES3    , 60, 1, "Cannot negate per element value set containing a value of 3.");
	E(CANNOT_NEGATE_PESm2   , 60, 2, "Cannot negate per element value set containing a value of -2.");
	F(INTERNAL_ERROR        ,255, 0, "Internal parser error.");

	#undef E
	#undef F

	MAKE_MSGTEMPLATE_CONTAINER
} MSG;
