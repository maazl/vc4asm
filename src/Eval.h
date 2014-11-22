/*
 * Eval.h
 *
 *  Created on: 16.11.2014
 *      Author: mueller
 */

#ifndef EVAL_H_
#define EVAL_H_

#include "expr.h"
#include <vector>


class Eval
{public:
	/// Mathematical operators, the high byte is the precedence,
	enum mathOp
	{	BRO  = 0xe04 ///< opening brace
	,	NOP  = 0xd04 ///< no operation
	,	NEG  = 0xdc5 ///< negative value
	,	NOT  = 0xdf6 ///< 2s complement
	,	lNOT = 0xdc7 ///< logical not
	,	POW  = 0xcc8 ///< power
	,	MUL  = 0xbc8 ///< multiply
	,	DIV  = 0xbc9 ///< divide
	,	MOD  = 0xbca ///< modulus
	,	ADD  = 0xa88 ///< add
	,	SUB  = 0xa89 ///< subtract
	,	ASL  = 0x930 ///< arithmetic shift left
	,	ASR  = 0x931 ///< arithmetic shift right
	,	SHL  = 0x9f2 ///< logical shift left
	,	SHR  = 0x9f3 ///< logical shift right
	,	GT   = 0x808 ///< greater than
	,	GE   = 0x809 ///< greater or equal
	,	LT   = 0x80a ///< less than
	,	LE   = 0x80b ///< less or equal
	,	EQ   = 0x708 ///< equal
	,	NE   = 0x709 ///< not equal
	,	AND  = 0x6f0 ///< bitwise and
	,	XOR  = 0x5f1 ///< bitwise exclusive or
	,	OR   = 0x4f2 ///< bitwise or
	,	lAND = 0x3c8 ///< logical and
	,	lXOR = 0x2c9 ///< logical exclusive or
	,	lOR  = 0x1ca ///< logical or
	,	BRC  = 0x000 ///< closing brace
	,	EVAL = 0x001 ///< done => evaluate all
	};
	/// Bits encoded in mathOp enum values.
	enum
	{	UNARY_OP  = 0x004
	,	PROP_FLOAT= 0x008
	,	NO_LFLOAT = 0x010
	,	NO_RFLOAT = 0x020
	,	NO_LREG   = 0x040
	,	NO_RREG   = 0x080
	,	OP_TYPE   = 0x0fc
	,	PRECEDENCE= 0xf00
	};
	/// Exception thrown when evaluation Fails
	struct Fail : string
	{	Fail(const char* fmt, ...);
	};
 private:
	struct exprEntry : public exprValue
	{	mathOp    Op;
		exprEntry() : Op(NOP) {}
		//constexpr exprEntry(const exprValue& r) : exprValue(r), Op(&nopOperator) {}
	};
	vector<exprEntry> Stack;
 private:
	/// Evaluates everything in the call stack with higher priority than the most recently added operator.
	/// @param unary true: evaluate unary operators only, i.e. the last object pushed is a value.
	/// @return - true: evaluation done
	/// - false: Operator was a closing brace but it does not belong to the current expression because of no matching opening brace.
	/// This happens when parsing the last argument to a function.
	bool        partialEvaluate(bool unary);
 public:
	            Eval() { Stack.emplace_back(); }
	bool        PushOperator(mathOp op);
	void        PushValue(const exprValue& value);
	exprValue   Evaluate();
 public:
	static const char* op2string(mathOp op);
};

#endif // EVAL_H_
