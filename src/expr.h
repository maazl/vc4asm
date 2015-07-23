/*
 * expr.h
 *
 *  Created on: 16.11.2014
 *      Author: mueller
 */

#ifndef EXPR_H_
#define EXPR_H_

#include <string>
#include <limits>
#include <type_traits>

using namespace std;

static_assert(numeric_limits<float>::is_iec559 && numeric_limits<float>::digits == 24, "error cannot cross compile on platform that does not support 32 bit IEEE float.");

/// Kind of register, bit vector
enum regType : uint8_t
{	R_A    =  1 ///< register file A
,	R_B    =  2 ///< register file B
,	R_AB   =  3 ///< A /or/ B, whatever is available
,	R_READ =  4 ///< read access
,	R_WRITE=  8 ///< write access
,	R_RDWR = 12 ///< read and write access
,	R_RDA  =  5 ///< read from register file A
,	R_RDB  =  6 ///< read from register file B
,	R_RDAB =  7 ///< read from register file A or B
,	R_WRA  =  9 ///< read from register file A
,	R_WRB  = 10 ///< read from register file B
,	R_WRAB = 11 ///< read from register file A or B
,	R_RWA  = 13 ///< read or write from register file A
,	R_RWB  = 14 ///< read or write from register file B
,	R_RWAB = 15 ///< read or write from register file A or B
,	R_SACQ = 16 ///< semaphore acquire @remarks Value must not change!!!
,	R_SREL = 32 ///< semaphore release
,	R_SEMA = 48 ///< any semaphore type
};
/// Structure for register type expressions
struct reg_t
{	uint8_t     Num;   ///< register number
	regType     Type;  ///< register type
	int8_t      Rotate;///< QPU element rotation [0..15], 16: >> r5, -16: << r5
};
/// Type of the expression value
enum valueType : char
{	V_NONE      ///< no value
,	V_INT       ///< integer literal or the result of an integer expression
,	V_FLOAT     ///< float literal or the result of an float expression
,	V_LDPES     ///< load per element signed, otherwise like V_INT
,	V_LDPE      ///< load per element signed or unsigned, otherwise like V_INT
,	V_LDPEU     ///< load per element unsigned, otherwise like V_INT
,	V_REG       ///< register
,	V_LABEL     ///< Label reference, iValue = Label value (only valid in Pass 2)
};
/// Convert expression type into a human readable format.
extern const char* type2string(valueType type);
/// Value of an expression. In most cases you should prefer exprValue.
/// @remarks This is basically a union rather than a struct.
/// But the struct wrapper is required to derive from this type.
struct value_t
{	union
	{	uint32_t  uValue;   ///< Value as unsigned integer
		int32_t   iValue;   ///< Value as signed integer
		float     fValue;   ///< Value as 32 bit float
		uint8_t   cValue[4];///< Value as 4 separate bytes
		reg_t     rValue;   ///< Register value
	};
};
/// @brief Expression value.
/// @details This is a polymorphic type. It can either be
/// - an integer constant,
/// - a floating point constant,
/// - a set of 2 bit integer constants for each QPU,
/// - a register reference or
/// - a label reference.
struct exprValue : value_t
{	valueType   Type;     /// Current type of the expression value.
	/// Construct invalid expression value.
	exprValue()           : Type(V_NONE)  { uValue = 0; }
	/// Construct constant expression with explicit type.
	/// @param i 32 bit value
	/// @param type One of V_INT, V_LDPE*, V_REG or V_LABEL.
	/// Everything else makes no sense here.
	exprValue(uint32_t i, valueType type) : Type(type) { uValue = i; }
	/// Construct signed integer constant.
	exprValue(int32_t i)  : Type(V_INT)   { iValue = i; }
	/// Construct floating point constant.
	exprValue(float f)    : Type(V_FLOAT) { fValue = f; }
	/// Construct register reference.
	exprValue(reg_t r)    : Type(V_REG)   { rValue = r; }
	/// Write the expression in human readable and turn around safe format.
	string      toString() const;
 private:
	static string toPE(unsigned value, bool sign);
};

#endif
