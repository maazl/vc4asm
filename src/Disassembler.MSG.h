/*
 * Disassembler.MSGS.h
 *
 *  Created on: 21.05.2017
 *      Author: mueller
 */

static const constexpr struct MSG
{
	#define W(name, major, minor, text, ...) const msgTemplate<__VA_ARGS__> name = msgTemplate<__VA_ARGS__>(msgID(MS_Disassembler, WARN, major, minor), text)
	// Entries must be ordered by major, minor.
	W(INVALID_ADDOP        , 10, 0, "Invalid opcode %u for ADD ALU.", uint8_t);
	W(INVALID_LDI_MODE     , 20, 0, "Invalid load immediate mode.");
	W(INVALID_BRANCH_CC    , 30, 0, "Invalid branch condition.");
	W(BRANCH_NO_UNPACK     , 31, 0, "Branch does not support unpack modes.");
	W(INVALID_MUL_PACK     , 40, 0, "Invalid MUL ALU pack mode.");
	W(SRC_ANOP             , 50, 1, "Input operand to ADD ALU nop opcode.");
	W(SRC_MNOP             , 50, 2, "Input operand to MUL ALU nop opcode.");
	W(SECOND_SRC_UNARY_OP  , 50, 3, "Second operand to unary ADD ALU opcode.");
	W(WADDRANOP_NOT_CCNEVER, 60, 1, "ADD ALU writes to nop register with condition != never.");
	W(WADDRMNOP_NOT_CCNEVER, 60, 2, "MUL ALU writes to nop register with condition != never.");
	#undef W

	MAKE_MSGTEMPLATE_CONTAINER
} MSG;
