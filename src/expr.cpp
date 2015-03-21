/*
 * expr.cpp
 *
 *  Created on: 16.11.2014
 *      Author: mueller
 */

#include "expr.h"
#include "utils.h"

string exprValue::toPE(unsigned val, bool sign)
{	string r;
	for (int i = 32; i; --i)
	{	r += stringf(",%i", sign && (val & 0x10000) ? (val & 1) - 2 : (val & 0x10001) / 0x8001);
		val >>= 1;
	}
	r[0] = '[';
	r += ']';
	return r;
}

string exprValue::toString() const
{switch (Type)
	{default:
		return string();
	 case V_INT:
		return stringf("0x%x", uValue);
	 case V_LDPES:
		return toPE(uValue, true);
	 case V_LDPE:
	 case V_LDPEU:
		return toPE(uValue, false);
	 case V_LABEL:
		return stringf(":(0x%x)", uValue);
	 case V_FLOAT:
		return stringf("%g", fValue);
	 case V_REG:
		return stringf(rValue.Rotate ? ":[%i,%i,%i]" : ":[%i,%i]", rValue.Num, rValue.Type, rValue.Rotate);
	}
}

const char* type2string(valueType type)
{	switch (type)
	{	default:      return "unknown";
		case V_INT:   return "integer";
		case V_LDPES: return "signed integer array";
		case V_LDPE:  return "integer array";
		case V_LDPEU: return "unsigned integer array";
		case V_FLOAT: return "floating point";
		case V_REG:   return "register";
		case V_LABEL: return "label";
	}
}

bool operator ==(exprValue l, exprValue r)
{	if (l.Type != r.Type)
		return false;
	if (l.Type != V_REG)
		return l.uValue == r.uValue;
	if ( l.rValue.Num != r.rValue.Num
		|| l.rValue.Rotate != r.rValue.Rotate
		|| (((l.rValue.Type|r.rValue.Type)&R_AB  ) && !(l.rValue.Type&r.rValue.Type&R_AB  ))
		|| (((l.rValue.Type|r.rValue.Type)&R_RDWR) && !(l.rValue.Type&r.rValue.Type&R_RDWR))
		|| (((l.rValue.Type|r.rValue.Type)&R_SEMA) && !(l.rValue.Type&r.rValue.Type&R_SEMA)) )
		return false;
	return true;
}
