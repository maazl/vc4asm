/*
 * Eval.cpp
 *
 *  Created on: 16.11.2014
 *      Author: mueller
 */

#include "Eval.h"
#include "Inst.h"

#include <cmath>
#include <stdarg.h>


Eval::Fail::Fail(const char* format, ...)
{	va_list va;
	va_start(va, format);
	static_cast<string&>(*this) = vstringf(format, va);
	va_end(va);
}


bool Eval::partialEvaluate(bool unary)
{next_eval:
	exprEntry& rhs = Stack.back();
	if (Stack.size() <= 1)
	{	if (rhs.Op == BRC) // closing brace w/o opening brace, might not belong to our expression
		{	if (rhs.Type == V_NONE)
				throw Fail("Incomplete expression: expected value.");
			return false;
		}
		return true;
	}
	exprEntry& lhs = Stack[Stack.size()-2];
	if ( (lhs.Op & PRECEDENCE) < (rhs.Op & PRECEDENCE)
		|| (unary && !(lhs.Op & UNARY_OP)) )
		return true;
	// Can operator be applied to type?
	if (lhs.Type == V_LABEL || rhs.Type == V_LABEL)
		throw Fail("Operators on labels are currently unsupported.");
	if ((lhs.Op & NO_LFLOAT) && lhs.Type == V_FLOAT)
		throw Fail("Cannot apply operator %s to floating point value %g.", op2string(lhs.Op), lhs.fValue);
	if ((lhs.Op & NO_RFLOAT) && rhs.Type == V_FLOAT)
		throw Fail("Cannot apply operator %s to floating point value %g.", op2string(lhs.Op), rhs.fValue);
	if ((lhs.Op & NO_LREG) && lhs.Type == V_REG)
		throw Fail("Cannot apply operator %s to register.", op2string(lhs.Op));
	if ((lhs.Op & NO_RREG) && rhs.Type == V_REG)
		throw Fail("Cannot apply operator %s to register.", op2string(lhs.Op));
	if ((lhs.Type == V_REG && rhs.Type == V_FLOAT) || (lhs.Type == V_FLOAT && rhs.Type == V_REG))
		throw Fail("Cannot apply operator %s to register and float.", op2string(lhs.Op));
	if ((lhs.Op & 0xe00) == 6 && ((lhs.Type == V_REG) ^ (rhs.Type == V_REG)))
		throw Fail("Cannot apply operator %s to register and non register.", op2string(lhs.Op));
	// Propagate one operand to float?
	if ((lhs.Op & PROP_FLOAT) && (lhs.Type != rhs.Type))
	{	if (rhs.Type == V_INT && lhs.Type == V_FLOAT)
		{	rhs.fValue = rhs.iValue;
			rhs.Type = V_FLOAT;
		} else if (lhs.Type == V_INT && rhs.Type == V_FLOAT)
		{	lhs.fValue = lhs.iValue;
			lhs.Type = V_FLOAT;
		}
	}
	// apply operator
	switch (lhs.Op)
	{default:
		throw Fail("internal parser error");
	 case BRO:
		switch (rhs.Op)
		{case EVAL:
			throw Fail("Incomplete expression: missing ')'.");
		 case BRC:
			lhs.Op = NOP; // closing brace cancels opening brace
			lhs.iValue = rhs.iValue;
			lhs.Type = rhs.Type;
			Stack.pop_back();
			goto next_eval;
		 default:
			return true; // can't evaluate over opening brace
		}
	 case NOP:
		lhs.uValue = rhs.uValue; // works for float also
		break;
	 case NEG:
		if (rhs.Type == V_FLOAT)
			lhs.fValue = -rhs.fValue;
		else
			lhs.iValue = -rhs.iValue;
		break;
	 case NOT:
		lhs.uValue = ~rhs.uValue;
		break;
	 case lNOT:
		lhs.uValue = !rhs.uValue;
		break;
	 case POW:
		if (rhs.Type == V_FLOAT)
			lhs.fValue = pow(lhs.fValue, rhs.fValue);
		else
			lhs.iValue = (int32_t)floor(pow(lhs.iValue, rhs.iValue)+.5);
		break;
	 case MUL:
		if (rhs.Type == V_FLOAT)
			lhs.fValue *= rhs.fValue;
		else
			lhs.iValue *= rhs.iValue;
		break;
	 case DIV:
		if (rhs.Type == V_FLOAT)
			lhs.fValue /= rhs.fValue;
		else
			lhs.iValue /= rhs.iValue;
		break;
	 case MOD:
		if (rhs.Type == V_FLOAT)
			lhs.fValue = fmod(lhs.fValue, rhs.fValue);
		else
			lhs.iValue %= rhs.iValue;
		break;
	 case ADD:
		if (lhs.Type == V_REG)
		{	if (rhs.Type == V_FLOAT)
				throw Fail("Cannot add float value %g to a register number.", rhs.fValue);
			unsigned r = lhs.rValue.Num + rhs.iValue;
			if (r > 63 || ((lhs.rValue.Type & R_SEMA) && r > 15))
				throw Fail("Register number out of range.");
			lhs.rValue.Num = r;
		} else if (rhs.Type == V_FLOAT)
			lhs.fValue += rhs.fValue;
		else
			lhs.iValue += rhs.iValue;
		break;
	 case SUB:
		if (lhs.Type == V_REG)
		{	if (rhs.Type == V_FLOAT)
				throw Fail("Cannot add float value %g to a register number.", rhs.fValue);
			unsigned r = lhs.rValue.Num - rhs.iValue;
			if (r > 63 || ((lhs.rValue.Type & R_SEMA) && r > 15))
				throw Fail("Register number out of range.");
			lhs.rValue.Num = r;
		} else if (rhs.Type == V_FLOAT)
			lhs.fValue -= rhs.fValue;
		else
			lhs.iValue -= rhs.iValue;
		break;
	 case ASL:
		if (lhs.Type == V_REG)
		{	// curious syntax ...
			lhs.rValue.Rotate = (lhs.rValue.Rotate + rhs.iValue) & 0xf;
			break;
		} else if (rhs.iValue < 0)
		{	lhs.iValue >>= -rhs.iValue;
			break;
		}
	 case SHL:
		lhs.uValue <<= rhs.uValue;
		break;
	 case ASR:
		if (lhs.Type == V_REG)
			// curious syntax ...
			lhs.rValue.Rotate = (lhs.rValue.Rotate - rhs.iValue) & 0xf;
		else if (rhs.iValue < 0)
			lhs.iValue <<= -rhs.iValue;
		else
			lhs.iValue >>= rhs.iValue;
		break;
	 case SHR:
		lhs.uValue >>= rhs.uValue;
		break;
	 case GT:
		if (lhs.Type == V_REG)
			lhs.iValue = lhs.rValue.Num > rhs.rValue.Num;
		else if (rhs.Type == V_FLOAT)
			lhs.iValue = lhs.fValue > rhs.fValue;
		else
			lhs.iValue = lhs.iValue > rhs.iValue;
		rhs.Type = V_INT;
		break;
	 case GE:
		if (lhs.Type == V_REG)
			lhs.iValue = lhs.rValue.Num >= rhs.rValue.Num;
		else if (rhs.Type == V_FLOAT)
			lhs.iValue = lhs.fValue >= rhs.fValue;
		else
			lhs.iValue = lhs.iValue >= rhs.iValue;
		lhs.Type = V_INT;
		break;
	 case LT:
		if (lhs.Type == V_REG)
			lhs.iValue = lhs.rValue.Num < rhs.rValue.Num;
		else if (rhs.Type == V_FLOAT)
			lhs.iValue = lhs.fValue < rhs.fValue;
		else
			lhs.iValue = lhs.iValue < rhs.iValue;
		lhs.Type = V_INT;
		break;
	 case LE:
		if (lhs.Type == V_REG)
			lhs.iValue = lhs.rValue.Num <= rhs.rValue.Num;
		else if (rhs.Type == V_FLOAT)
			lhs.iValue = lhs.fValue <= rhs.fValue;
		else
			lhs.iValue = lhs.iValue <= rhs.iValue;
		rhs.Type = V_INT;
		break;
	 case EQ:
		if (lhs.Type == V_REG)
			lhs.iValue = lhs.rValue.Num == rhs.rValue.Num && lhs.rValue.Type == rhs.rValue.Type;
		else if (rhs.Type == V_FLOAT)
			lhs.iValue = lhs.fValue == rhs.fValue;
		else
			lhs.iValue = lhs.iValue == rhs.iValue;
		lhs.Type = V_INT;
		break;
	 case NE:
		if (lhs.Type == V_REG)
			lhs.iValue = lhs.rValue.Num != rhs.rValue.Num || lhs.rValue.Type != rhs.rValue.Type;
		else if (rhs.Type == V_FLOAT)
			lhs.iValue = lhs.fValue != rhs.fValue;
		else
			lhs.iValue = lhs.iValue != rhs.iValue;
		lhs.Type = V_INT;
		break;
	 case AND:
		lhs.uValue &= rhs.uValue;
		break;
	 case XOR:
		lhs.uValue ^= rhs.uValue;
		break;
	 case OR:
		lhs.uValue |= rhs.uValue;
		break;
	 case lAND:
		lhs.iValue = lhs.iValue && rhs.iValue;
		break;
	 case lXOR:
		lhs.iValue = !lhs.iValue ^ !rhs.iValue;
	 case lOR:
		lhs.iValue = lhs.iValue || rhs.iValue;
		break;
	}
	if (lhs.Op & UNARY_OP)
		lhs.Type = rhs.Type;
	lhs.Op = rhs.Op;
	Stack.pop_back();
	goto next_eval;
}

bool Eval::PushOperator(mathOp op)
{	exprEntry& current = Stack.back();
	current.Op = op;
	if (op & UNARY_OP)
	{	if (current.Type != V_NONE)
			throw Fail("Unexpected unary operator %s after numeric expression.", op2string(op));
	 unary:
		Stack.emplace_back();
		return true;
	} else
	{	if (current.Type == V_NONE)
			switch (op)
			{default:
				throw Fail("Binary operator %s needs left hand side expression.", op2string(op));
			 case ADD:
				current.Op = NOP;
				goto unary;
			 case SUB:
				current.Op = NEG;
				goto unary;
			 case BRC:;
			}
	}
	if (!partialEvaluate(false))
		return false;
	if (op != BRC)
		Stack.emplace_back();
	return true;
}

void Eval::PushValue(const exprValue& value)
{	exprEntry& current = Stack.back();
	if (current.Type != V_NONE)
		throw Fail("Missing operator before value '%s'.", value.toString().c_str());
	(exprValue&)current = value; // assign slice
	partialEvaluate(true);
}

exprValue Eval::Evaluate()
{	exprEntry& current = Stack.back();
	if (current.Type == V_NONE)
	{	if ( Stack.size() == 2 && Stack.front().Op == NEG
			&& current.Op == NOP && current.Type == V_NONE ) // special handling for '-' as r39
		{	current.Type = V_REG;
			current.rValue.Num = Inst::R_NOP;
			current.rValue.Rotate = 0;
			current.rValue.Type = R_WRAB;
			return current;
		}
		throw Fail("Incomplete expression: expected value.");
	}
	current.Op = EVAL;
	partialEvaluate(false);
	return Stack.front();
}

const char* Eval::op2string(mathOp op)
{	switch (op)
	{case BRO: return "(";
	 case BRC: return ")";
	 case NOP:
	 case ADD: return "+";
	 case NEG:
	 case SUB: return "-";
	 case NOT: return "~";
	 case lNOT:return "!";
	 case POW: return "**";
	 case MUL: return "*";
	 case DIV: return "/";
	 case MOD: return "%";
	 case ASL: return "<<";
	 case ASR: return ">>";
	 case SHL: return "<<<";
	 case SHR: return ">>>";
	 case GT:  return ">";
	 case GE:  return ">=";
	 case LT:  return "<";
	 case LE:  return "<=";
	 case EQ:  return "==";
	 case NE:  return "!=";
	 case AND: return "&";
	 case XOR: return "^";
	 case OR:  return "|";
	 case lAND:return "&&";
	 case lXOR:return "^^";
	 case lOR: return "||";
	 default:  return NULL;
	}
}
