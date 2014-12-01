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
		return stringf("0x%x", uValue);
	 case V_FLOAT:
		return stringf("%g", fValue);
	 case V_REG:
		// TODO
		return string();
	}
}

