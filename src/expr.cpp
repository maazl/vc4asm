/*
 * expr.cpp
 *
 *  Created on: 16.11.2014
 *      Author: mueller
 */

#include "expr.h"
#include "utils.h"


string exprValue::toString() const
{switch (Type)
	{default:
		return string();
	 case V_INT:
	 case V_LDPES:
	 case V_LDPE:
	 case V_LDPEU:
	 case V_LABEL:
		return stringf("0x%x", uValue);
	 case V_FLOAT:
		return stringf("%g", fValue);
	 case V_REG:
		// TODO
		return string();
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
