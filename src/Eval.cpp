/*
 * Eval.cpp
 *
 *  Created on: 16.11.2014
 *      Author: mueller
 */

#include "Eval.h"
#include "Inst.h"
#include "utils.h"

#include <cmath>
#include <cstdarg>


Eval::Fail::Fail(const char* format, ...)
{	va_list va;
	va_start(va, format);
	static_cast<string&>(*this) = vstringf(format, va);
	va_end(va);
}


Eval::operate::operate(vector<exprEntry>& stack)
:	rhs(stack.back())
,	lhs(stack[stack.size()-2])
,	types((1 << rhs.Type) | ((lhs.Op & UNARY_OP) == 0) * (1 << lhs.Type))
{}

void Eval::operate::TypesFail()
{	if (lhs.Op & UNARY_OP)
		throw Fail("Cannot apply unary operator %s to operand of type %s.", op2string(lhs.Op), type2string(rhs.Type));
	else
		throw Fail("Cannot apply operator %s to operands of type %s and %s.", op2string(lhs.Op), type2string(lhs.Type), type2string(rhs.Type));
}

void Eval::operate::CheckInt()
{	if (types != 1<<V_INT)
		TypesFail();
}

void Eval::operate::CheckBool()
{	if (types & ~(1<<V_INT | 1<<V_FLOAT | 1<<V_LDPE | 1<<V_LDPES | 1<<V_LDPEU))
		TypesFail();
	lhs.Type = V_INT;
}

void Eval::operate::PropFloat()
{	if (lhs.Type == V_INT)
	{	lhs.fValue = lhs.iValue;
		lhs.Type = V_FLOAT;
		types = 1<<V_FLOAT;
	}
	if (rhs.Type == V_INT)
	{	rhs.fValue = rhs.iValue;
		rhs.Type = V_FLOAT;
		types = 1<<V_FLOAT;
	}
}

void Eval::operate::CheckNumericPropFloat()
{	if (types & ~(1<<V_INT | 1<<V_FLOAT))
		TypesFail();
	if (types == (1<<V_INT | 1<<V_FLOAT))
		PropFloat();
}

void Eval::operate::CheckRelational()
{	switch (types)
	{default:
		TypesFail();
	 case 1<<V_INT | 1<<V_FLOAT:
		PropFloat();
	 case 1<<V_INT:
	 case 1<<V_FLOAT:
	 case 1<<V_REG:
	 case 1<<V_LABEL:;
	}
	lhs.Type = V_INT;
}

bool Eval::operate::Apply(bool unary)
{	if ( (lhs.Op & PRECEDENCE) < (rhs.Op & PRECEDENCE)
		|| (unary && !(lhs.Op & UNARY_OP)) )
		return true;

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
			return false;
		 default:
			return true; // can't evaluate over opening brace
		}
	 case NOP:
		lhs.uValue = rhs.uValue; // works for float also
		lhs.Type = rhs.Type;
		break;
	 case NEG:
		lhs.Type = rhs.Type;
		switch (rhs.Type)
		{	uint32_t sh;
		 default:
			TypesFail();
		 case V_FLOAT:
			lhs.fValue = -rhs.fValue; break;
		 case V_INT:
			lhs.iValue = -rhs.iValue; break;
		 case V_LDPEU:
		 case V_LDPE:
			sh = (rhs.uValue & 0x55555555U) << 1;
			if (rhs.uValue & sh)
				throw Fail("Cannot negate per element value set containing a value of 3.");
			lhs.uValue = rhs.uValue | sh;
			lhs.Type = V_LDPES;
			break;
		 case V_LDPES:
			sh = (rhs.uValue & 0x55555555U) << 1;
			lhs.uValue = rhs.uValue | sh;
			if (lhs.uValue & sh)
				throw Fail("Cannot negate per element value set containing a value of -2.");
			break;
		}
		break;
	 case NOT:
		switch (rhs.Type)
		{default:
			TypesFail();
		 case V_INT:
		 case V_LDPES:
		 case V_LDPE:
		 case V_LDPEU:
			lhs.uValue = ~rhs.uValue; break;
		}
		lhs.Type = rhs.Type;
		break;
	 case lNOT:
		CheckBool();
		lhs.uValue = !rhs.uValue;
		break;
	 case POW:
		CheckNumericPropFloat();
		if (rhs.Type == V_FLOAT)
			lhs.fValue = pow(lhs.fValue, rhs.fValue);
		else
			lhs.iValue = (int32_t)floor(pow(lhs.iValue, rhs.iValue)+.5);
		break;
	 case MUL:
		CheckNumericPropFloat();
		if (rhs.Type == V_FLOAT)
			lhs.fValue *= rhs.fValue;
		else
			lhs.iValue *= rhs.iValue;
		break;
	 case DIV:
		CheckNumericPropFloat();
		if (rhs.Type == V_FLOAT)
			lhs.fValue /= rhs.fValue;
		else
			lhs.iValue /= rhs.iValue;
		break;
	 case MOD:
		CheckNumericPropFloat();
		if (rhs.Type == V_FLOAT)
			lhs.fValue = fmod(lhs.fValue, rhs.fValue);
		else
			lhs.iValue %= rhs.iValue;
		break;
	 case ADD:
		switch (types)
		{default:
			TypesFail();
		 case 1<<V_INT | 1<<V_FLOAT:
			PropFloat();
			lhs.Type = V_FLOAT;
		 case 1<<V_FLOAT:
			lhs.fValue += rhs.fValue; break;
		 case 1<<V_LABEL | 1<<V_INT:
			lhs.Type = V_LABEL;
		 case 1<<V_INT:
			lhs.uValue += rhs.uValue; break;
		 case 1<<V_REG | 1<<V_INT:
			{	unsigned r = lhs.Type == V_REG ? lhs.rValue.Num + rhs.iValue : rhs.rValue.Num + lhs.iValue;
								if (lhs.Type == V_REG)
				if (r > 63 || ((lhs.rValue.Type & R_SEMA) && r > 15))
					throw Fail("Register number out of range.");
				lhs.rValue.Num = r;
				lhs.Type = V_REG;
			} break;
		} break;
	 case SUB:
		switch (types)
		{default:
			TypesFail();
		 case 1<<V_INT | 1<<V_FLOAT:
			PropFloat();
			lhs.Type = V_FLOAT;
		 case 1<<V_FLOAT:
			lhs.fValue -= rhs.fValue; break;
		 case 1<<V_LABEL:
			lhs.Type = V_INT;
			lhs.uValue -= rhs.uValue; break;
		 case 1<<V_LABEL | 1<<V_INT:
			if (rhs.Type == V_LABEL)
				TypesFail();
		 case 1<<V_INT:
			lhs.uValue -= rhs.uValue; break;
		 case 1<<V_REG | 1<<V_INT:
			if (rhs.Type == V_REG)
				TypesFail();
			{	unsigned r = lhs.rValue.Num - rhs.iValue;
				if (r > 63 || ((lhs.rValue.Type & R_SEMA) && r > 15))
					throw Fail("Register number out of range.");
				lhs.rValue.Num = r;
			} break;
		} break;
	 case ASL:
		switch (types)
		{case 1<<V_INT:
			if (rhs.iValue < 0)
				lhs.iValue >>= -rhs.iValue;
			else
				lhs.iValue <<= rhs.iValue;
			break;
		 case 1 << V_REG:
			if (lhs.rValue.Rotate || rhs.rValue.Num != 32+5 || rhs.rValue.Type != R_AB || rhs.rValue.Rotate)
				Fail("Vector rotation are only allowed by constant or by r5");
			lhs.rValue.Rotate = 16;
			break;
		 case 1<<V_REG | 1<<V_INT:
			if (lhs.Type == V_REG)
			{	// curious syntax ...
				if (lhs.rValue.Rotate & ~0xf)
					Fail("Cannot apply additional offset to r5 vector rotation");
				lhs.rValue.Rotate = (lhs.rValue.Rotate + rhs.iValue) & 0xf;
			} break;
		 default:
			TypesFail();
		} break;
	 case ASR:
		switch (types)
		{case 1<<V_INT:
			if (rhs.iValue < 0)
				lhs.iValue <<= -rhs.iValue;
			else
				lhs.iValue >>= rhs.iValue;
			break;
		 case 1<<V_REG | 1<<V_INT:
			if (lhs.Type == V_REG)
			{	// curious syntax ...
				if (lhs.rValue.Rotate & ~0xf)
					Fail("Cannot apply additional offset to r5 vector rotation");
				lhs.rValue.Rotate = (lhs.rValue.Rotate - rhs.iValue) & 0xf;
			} break;
		 default:
			TypesFail();
		} break;
	 case SHL:
		CheckInt();
		lhs.uValue <<= rhs.uValue;
		break;
	 case SHR:
		CheckInt();
		lhs.uValue >>= rhs.uValue;
		break;
	 case GT:
		CheckRelational();
		if (rhs.Type == V_REG)
			lhs.iValue = lhs.rValue.Num > rhs.rValue.Num;
		else if (rhs.Type == V_FLOAT)
			lhs.iValue = lhs.fValue > rhs.fValue;
		else
			lhs.iValue = lhs.iValue > rhs.iValue;
		break;
	 case GE:
		CheckRelational();
		if (rhs.Type == V_REG)
			lhs.iValue = lhs.rValue.Num >= rhs.rValue.Num;
		else if (rhs.Type == V_FLOAT)
			lhs.iValue = lhs.fValue >= rhs.fValue;
		else
			lhs.iValue = lhs.iValue >= rhs.iValue;
		break;
	 case LT:
		CheckRelational();
		if (rhs.Type == V_REG)
			lhs.iValue = lhs.rValue.Num < rhs.rValue.Num;
		else if (rhs.Type == V_FLOAT)
			lhs.iValue = lhs.fValue < rhs.fValue;
		else
			lhs.iValue = lhs.iValue < rhs.iValue;
		break;
	 case LE:
		CheckRelational();
		if (rhs.Type == V_REG)
			lhs.iValue = lhs.rValue.Num <= rhs.rValue.Num;
		else if (rhs.Type == V_FLOAT)
			lhs.iValue = lhs.fValue <= rhs.fValue;
		else
			lhs.iValue = lhs.iValue <= rhs.iValue;
		break;
	 case EQ:
		if (types == (1<<V_INT | 1<<V_FLOAT))
			PropFloat();
		lhs.iValue = lhs == rhs;
		lhs.Type = V_INT;
		break;
	 case NE:
		if (types == (1<<V_INT | 1<<V_FLOAT))
			PropFloat();
		lhs.iValue = !(lhs == rhs);
		lhs.Type = V_INT;
		break;
	 case AND:
		CheckInt();
		lhs.uValue &= rhs.uValue;
		break;
	 case XOR:
		CheckInt();
		lhs.uValue ^= rhs.uValue;
		break;
	 case OR:
		CheckInt();
		lhs.uValue |= rhs.uValue;
		break;
	 case lAND:
		CheckBool();
		lhs.iValue = lhs.uValue && rhs.uValue;
		break;
	 case lXOR:
		CheckBool();
		lhs.iValue = !lhs.uValue ^ !rhs.uValue;
	 case lOR:
		CheckBool();
		lhs.iValue = lhs.uValue || rhs.uValue;
		break;
	}
	lhs.Op = rhs.Op;
	return false;
}


bool Eval::partialEvaluate(bool unary)
{	for (;;)
	{	if (Stack.size() <= 1)
		{	if (Stack.back().Op == BRC) // closing brace w/o opening brace, might not belong to our expression
			{	if (Stack.back().Type == V_NONE)
					throw Fail("Incomplete expression: expected value.");
				return false;
			}
			return true;
		}
		operate op(Stack);
		if (op.Apply(unary))
			return true;
		Stack.pop_back();
	}
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
