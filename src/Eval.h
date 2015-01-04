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
	/// Mathematical operators, the high nibble is the precedence
	enum mathOp : unsigned char
	{	BRO  = 0xe4 ///< opening brace
	,	NOP  = 0xd4 ///< no operation
	,	NEG  = 0xd5 ///< negative value
	,	NOT  = 0xd6 ///< 2s complement
	,	lNOT = 0xd7 ///< logical not
	,	POW  = 0xc0 ///< power
	,	MUL  = 0xb0 ///< multiply
	,	DIV  = 0xb1 ///< divide
	,	MOD  = 0xb2 ///< modulus
	,	ADD  = 0xa0 ///< add
	,	SUB  = 0xa1 ///< subtract
	,	ASL  = 0x90 ///< arithmetic shift left
	,	ASR  = 0x91 ///< arithmetic shift right
	,	SHL  = 0x92 ///< logical shift left
	,	SHR  = 0x93 ///< logical shift right
	,	GT   = 0x80 ///< greater than
	,	GE   = 0x81 ///< greater or equal
	,	LT   = 0x82 ///< less than
	,	LE   = 0x83 ///< less or equal
	,	EQ   = 0x70 ///< equal
	,	NE   = 0x71 ///< not equal
	,	AND  = 0x60 ///< bitwise and
	,	XOR  = 0x51 ///< bitwise exclusive or
	,	OR   = 0x42 ///< bitwise or
	,	lAND = 0x30 ///< logical and
	,	lXOR = 0x21 ///< logical exclusive or
	,	lOR  = 0x1a ///< logical or
	,	BRC  = 0x00 ///< closing brace
	,	EVAL = 0x01 ///< done => evaluate all
	};
	/// Exception thrown when evaluation Fails
	struct Fail : string
	{	Fail(const char* fmt, ...);
	};
 private:
	/// Bits encoded in mathOp enum values.
	enum
	{	UNARY_OP  = 0x04
	,	PRECEDENCE= 0xf0
	};
	struct exprEntry : public exprValue
	{	mathOp    Op;
		exprEntry() : Op(NOP) {}
		//constexpr exprEntry(const exprValue& r) : exprValue(r), Op(&nopOperator) {}
	};
	vector<exprEntry> Stack;
	class operate
	{	exprEntry& rhs;
		exprEntry& lhs;
		unsigned  types;
		void      TypesFail();
		void      CheckInt();
		void      CheckBool();
		void      PropFloat();
		void      CheckNumericPropFloat();
		int       Compare();
	 public:
		operate(vector<exprEntry>& stack);
		bool      Apply(bool unary);
	};
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
