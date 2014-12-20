/*
 * Parser.cpp
 *
 *  Created on: 30.08.2014
 *      Author: mueller
 */

#include "Parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <cctype>

#include "Parser.tables.cpp"


string Parser::location::toString() const
{	return stringf("%s (%u)", File.c_str(), Line);
}


Parser::saveContext::saveContext(Parser& parent, fileContext* ctx)
:	Parent(parent)
{	parent.Context.emplace_back(ctx);
}

Parser::saveContext::~saveContext()
{	Parent.Context.pop_back();
}

Parser::saveLineContext::saveLineContext(Parser& parent, fileContext* ctx)
:	saveContext(parent, ctx)
, LineBak(parent.Line)
, AtBak(parent.At)
{}

Parser::saveLineContext::~saveLineContext()
{	strncpy(Parent.Line, LineBak.c_str(), sizeof(Line));
	Parent.At = AtBak;
}


string Parser::enrichMsg(string msg)
{	// Show context
	contextType type = CTX_CURRENT;
	for (auto i = Context.rbegin(); i != Context.rend(); ++i)
	{	const fileContext& ctx = **i;
		if (ctx.Line)
			switch (type)
			{case CTX_CURRENT:
				msg = stringf("%s (%u,%u): ", ctx.File.c_str(), ctx.Line, At - Line - Token.size() + 1) + msg;
				break;
			 case CTX_INCLUDE:
				msg += stringf("\n  Included from %s (%u)", ctx.File.c_str(), ctx.Line);
				break;
			 case CTX_MACRO:
				msg += stringf("\n  At invocation of macro from %s (%u)", ctx.File.c_str(), ctx.Line);
				break;
			 case CTX_FUNCTION:
				msg += stringf("\n  At function invocation from %s (%u)", ctx.File.c_str(), ctx.Line);
				break;
			 case CTX_ROOT:;
			}
		type = ctx.Type;
	}
	return msg;
}

void Parser::Fail(const char* fmt, ...)
{	Success = false;
	va_list va;
	va_start(va, fmt);
	throw enrichMsg(vstringf(fmt, va));
	va_end(va);
}

static const char msgpfx[][10]
{	"ERROR: "
,	"Warning: "
,	"Info: "
};

void Parser::Error(const char* fmt, ...)
{	Success = false;
	va_list va;
	va_start(va, fmt);
	fputs(msgpfx[ERROR], stderr);
	fputs(enrichMsg(vstringf(fmt, va)).c_str(), stderr);
	va_end(va);
	fputc('\n', stderr);
}

void Parser::Msg(severity level, const char* fmt, ...)
{	if (!Pass2 || Verbose < level)
		return;
	va_list va;
	va_start(va, fmt);
	fputs(msgpfx[level], stderr);
	fputs(enrichMsg(vstringf(fmt, va)).c_str(), stderr);
	va_end(va);
	fputc('\n', stderr);
}

Parser::token_t Parser::NextToken()
{
	At += strspn(At, " \t\r\n");

	switch (*At)
	{case 0:
	 case '#':
		Token.clear();
		return END;
	 case '*':
	 case '>':
	 case '<':
	 case '&':
	 case '^':
	 case '|':
		if (At[1] == At[0])
		{	if (At[2] == At[1])
			{	Token.assign(At, 2);
				At += 2;
				return OP;
			}
		 op2:
			Token.assign(At, 2);
			At += 2;
			return OP;
		}
	 case '!':
	 case '=':
		if (At[1] == '=')
			goto op2;
	 case '+':
	 case '-':
	 case '/':
	 case '%':
	 case '~':
		Token.assign(At, 1);
		++At;
		return OP;
	 case '(':
	 case ')':
	 case '[':
	 case ']':
	 case '.':
	 case ',':
	 case ';':
	 case ':':
		Token.assign(At, 1);
		return (token_t)*At++;
	}

	size_t i = isdigit(*At)
		?	strspn(At, "0123456789abcdefABCDEFx.") // read until we don't find a hex digit, x (for hex) or .
		:	strcspn(At, ".,;:+-*/%()&|^~!=<> \t\r\n");
	Token.assign(At, i);
	At += i;

	return isdigit(Token[0]) ? NUM : WORD;
}

size_t Parser::parseUInt(const char* src, uint32_t& dst)
{	dst = 0;
	const char* cp = src;
	uint32_t basis = 10;
	uint32_t limit = UINT32_MAX / 10;
	for (char c; (c = *cp) != 0; ++cp)
	{	uint32_t digit = c - '0';
		if (digit >= 10)
		{	digit = toupper(c) - 'A';
			if (digit < 6)
				digit += 10;
			else
			{	if (dst == 0 && cp - src == 1)
					switch (c)
					{	case 'b': basis = 2; limit = UINT32_MAX/2; continue;
						case 'o': basis = 8; limit = UINT32_MAX/8; continue;
						case 'x': basis = 16; limit = UINT32_MAX/16; continue;
						case 'd': continue;
					}
				break;
			}
		}
		if (dst > limit)
			break;
		dst *= basis;
		if (dst > UINT32_MAX - digit)
			break;
		dst += digit;
	}
	return cp - src;
}

exprValue Parser::parseElemInt()
{	uint32_t value = 0;
	int pos = 0;
	signed char sign = 0;
	switch (Instruct.Sig)
	{case Inst::S_NONE:
		if (Instruct.Unpack == Inst::U_32
			&& Instruct.OpA == Inst::A_NOP && Instruct.OpM == Inst::M_NOP
			&& Instruct.RAddrA == Inst::R_NOP && Instruct.RAddrB == Inst::R_NOP )
			break;
	 default:
	 fail:
		Fail("Cannot combine load per QPU element with any other instruction.");
	 case Inst::S_LDI:
		if (Instruct.LdMode & Inst::L_SEMA)
			goto fail;
		if (Instruct.LdMode)
			sign = Instruct.LdMode == Inst::L_PEU ? 1 : -1;
	}
	while (true)
	{	auto val = ParseExpression();
		if (val.Type != V_INT)
			Fail("Only integers are allowed between [...].");
		if (val.iValue < -2 || val.iValue > 3)
			Fail("Load per QPU element can only deal with integer constants in the range of [-2..3].");
		if (val.iValue < 0)
		{	if (sign > 0)
				Fail("All integers in load per QPU element must be either in the range [-2..1] or in the range [0..3].");
			sign = -1;
		} else if (val.iValue > 1)
		{	if (sign < 0)
				Fail("All integers in load per QPU element must be either in the range [-2..1] or in the range [0..3].");
			sign = 1;
		}
		value |= (val.uValue * 0x8001 & 0x10001) << pos;

		switch (NextToken())
		{case END:
			Fail("Incomplete expression.");
		 default:
			Fail("Unexpected '%s' in per QPU element constant.", Token.c_str());
		 case COMMA:
			if (++pos >= 16)
				Fail("Too many initializers for per QPU element constant.");
			continue;
		 case SQBRC2:;
		}
		break;
	}
	// Make LDIPEx
	return exprValue(value, (valueType)(sign + V_LDPE));
}

exprValue Parser::ParseExpression()
{
	Eval eval;
 	try
	{next:
		switch (NextToken())
		{case WORD:
			{	// Expand constants
				for (auto i = Context.end(); i != Context.begin(); )
				{	auto c = (*--i)->Consts.find(Token);
					if (c != (*i)->Consts.end())
					{	eval.PushValue(c->second.Value);
						goto next;
					}
				}
			}
			{	// try function
				auto fp = Functions.find(Token);
				if (fp != Functions.end())
				{	// Hit!
					eval.PushValue(doFUNC(fp));
					break;
				}
			}
			{	// try register
				const regEntry* rp = binary_search(regMap, Token.c_str());
				if (rp)
				{	eval.PushValue(rp->Value);
					break;
				}
			}
			// Try label prefix
			{	string identifier = Token;
				if (NextToken() != COLON)
					Fail("Invalid expression. The identifier %s did not evaluate.", identifier.c_str());
				if (identifier != "r")
					Fail("'%s:' is no valid label prefix.", identifier.c_str());
			}
		 case COLON: // Label
			{	// Is forward reference?
				bool forward = false;
				switch (NextToken())
				{default:
					Fail("Expected Label after ':'.");
				 case WORD:
					break;
				 case NUM:
					// NextToken accepts some letters in numeric constants
//					char* at_bak = At;
//					if (NextToken() == WORD)
//					{	if (Token == "f")
//							// forward reference
//							forward = true;
//						else
//							Fail("Invalid label postfix '%s'.", Token.c_str());
//					} else
//						At = at_bak; // Undo
					if (Token.back() == 'f') // forward reference
					{	forward = true;
						Token.erase(Token.size()-1);
					}
				}
				const auto& l = LabelsByName.emplace(Token, LabelCount);
				if (l.second || (forward && Labels[l.first->second].Definition))
				{	// new label
					l.first->second = LabelCount;
					if (!Pass2)
						Labels.emplace_back(Token);
					else if (Labels.size() <= LabelCount || Labels[LabelCount].Name != Token)
						Fail("Inconsistent Label definition during Pass 2.");
					Labels[LabelCount].Reference = *Context.back();
					++LabelCount;
				}
				eval.PushValue(exprValue(Labels[l.first->second].Value, V_LABEL));
				break;
			}

		 default:
		 discard:
			At -= Token.size();
			return eval.Evaluate();

		 case OP:
		 case BRACE1:
		 case BRACE2:
			{	const opInfo* op = binary_search(operatorMap, Token.c_str());
				if (!op)
					Fail("Invalid operator: %s", Token.c_str());
				if (!eval.PushOperator(op->Op))
					goto discard;
				break;
			}
		 case SQBRC1: // per QPU element constant
			return parseElemInt();

		 case NUM:
			// parse number
			if (Token.find('.') == string::npos)
			{	// integer
				//size_t len;  sscanf of gcc4.8.2/Linux x32 can't read "0x80000000".
				//if (sscanf(Token.c_str(), "%i%n", &stack.front().iValue, &len) != 1 || len != Token.size())
				exprValue value;
				if (parseUInt(Token.c_str(), value.uValue) != Token.size())
					Fail("%s is no integral number.", Token.c_str());
				value.Type = V_INT;
				eval.PushValue(value);
			} else
			{	// float number
				float value;
				size_t len;
				if (sscanf(Token.c_str(), "%f%n", &value, &len) != 1 || len != Token.size())
					Fail("%s is no real number.", Token.c_str());
				eval.PushValue(value);
			}
			break;
		}
		goto next;
	} catch (const string& msg)
	{	throw enrichMsg(msg);
	}
}

Inst::mux Parser::muxReg(reg_t reg)
{	if (reg.Type & R_SEMA)
		Fail("Cannot use semaphore source in ALU instruction.");
	if (!(reg.Type & R_READ))
	{	// direct read access for r0..r5.
		if ((reg.Num ^ 32U) <= 5U)
			return (Inst::mux)(reg.Num ^ 32);
		Fail("The register is not readable.");
	}
	// try RA
	if (reg.Type & R_A)
	{	if ( Instruct.RAddrA == reg.Num
			|| ( (Instruct.OpA == Inst::A_NOP || (Instruct.MuxAA != Inst::X_RA && Instruct.MuxAB != Inst::X_RA))
				&& (Instruct.OpM == Inst::M_NOP || (Instruct.MuxMA != Inst::X_RA && Instruct.MuxMB != Inst::X_RA)) ))
		{	Instruct.RAddrA = reg.Num;
			return Inst::X_RA;
		}
	}
	// try RB
	if (reg.Type & R_B)
	{	if (Instruct.Sig >= Inst::S_SMI)
			Fail("Access to register file B conflicts with small immediate value.");
		if ( Instruct.RAddrB == reg.Num
			|| ( (Instruct.OpA == Inst::A_NOP || (Instruct.MuxAA != Inst::X_RB && Instruct.MuxAB != Inst::X_RB))
				&& (Instruct.OpM == Inst::M_NOP || (Instruct.MuxMA != Inst::X_RB && Instruct.MuxMB != Inst::X_RB)) ))
		{	Instruct.RAddrB = reg.Num;
			return Inst::X_RB;
		}
	}
	Fail("Read access to register conflicts with another access to the same register file.");
}

uint8_t Parser::getSmallImmediate(uint32_t i)
{	if (i + 16 <= 0x1f)
		return (uint8_t)(i & 0x1f);
	if ((i & 0x807fffff) == 0 && i - 0x3b800000U <= 0x7800000U)
		return (uint8_t)(((i >> 23) - 0x77) ^ 0x28);
	return 0xff; // failed
}

const Parser::smiEntry* Parser::getSmallImmediateALU(uint32_t i)
{	size_t l = 0;
	size_t r = sizeof smiMap / sizeof *smiMap - 1;
	while (l < r)
	{	size_t m = (l+r) >> 1;
		if (i <= smiMap[m].Value)
			r = m;
		else
			l = m + 1;
	}
	return smiMap + r;
}

void Parser::doSMI(uint8_t si)
{	switch (Instruct.Sig)
	{default:
		Fail("Small immediate value or vector rotation cannot be used together with signals.");
	 case Inst::S_SMI:
		if (Instruct.SImmd != si)
			Fail("Only one distinct small immediate value supported per instruction. Requested value: %u, current Value: %u.", si, Instruct.SImmd);
		return; // value hit
	 case Inst::S_NONE:
		if (Instruct.RAddrB != Inst::R_NOP )
			Fail("Small immediate cannot be used together with register file B read access.");
	}
	Instruct.Sig   = Inst::S_SMI;
	Instruct.SImmd = si;
}

void Parser::addIf(int cond, bool mul)
{
	if (Instruct.Sig == Inst::S_BRANCH)
		return Error("Cannot apply conditional store (.ifxx) to branch instruction.");
	auto& target = mul ? Instruct.CondM : Instruct.CondA;
	if (target != Inst::C_AL)
		return Error("Store condition (.if) already specified.");
	target = (Inst::conda)cond;
}

void Parser::addUnpack(int mode, bool mul)
{
	if (Instruct.Sig >= Inst::S_LDI)
		return Error("Cannot apply .unpack to branch and load immediate instructions.");
	if (Instruct.Unpack != Inst::U_32)
		return Error("Only on .unpack per instruction, please.");
	if (Instruct.Sig != Inst::S_LDI && Instruct.Pack != Inst::P_32 && Instruct.PM != mul)
		return Error(".unpack conflicts with .pack instruction of ADD ALU.");
	if (mul)
	{	// TODO ensure source R4
		Instruct.PM = true;
	}
	Instruct.Pack = (Inst::pack)mode;
}

void Parser::addPack(int mode, bool mul)
{
	if (Instruct.Sig == Inst::S_BRANCH)
		return Error("Cannot apply .pack to branch instruction.");
	if (Instruct.Pack != Inst::P_32)
		return Error("Only on .pack per instruction, please.");
	if (Instruct.Sig != Inst::S_LDI && Instruct.Unpack != Inst::U_32 && Instruct.PM != mul)
		return Error(".pack conflicts with .unpack instruction of ADD ALU.");
	/* TODO: MUL ALU pack modes, cannot activate until we know the target
	if (mul)
	{	if (mode < Inst::P_8abcdS && mode != Inst::P_32)
			return Error("MUL ALU does not support pack mode.");
		mode &= 7;
		Instruct.PM = true;
	}*/
	Instruct.Pack = (Inst::pack)mode;
}

void Parser::addSetF(int, bool mul)
{
	if (Instruct.Sig == Inst::S_BRANCH)
		return Error("Cannot apply .setf to branch instruction.");
	if ( Instruct.Sig != Inst::S_LDI
		&& (Instruct.CondA == Inst::C_NEVER && Instruct.OpA == Inst::A_NOP) != mul )
		return Error("Cannot apply .setf because the flags of the other ALU will be used.");
	if (Instruct.SF)
		return Error("Don't use .setf twice.");
	Instruct.SF = true;
}

void Parser::addCond(int cond, bool mul)
{
	if (Instruct.Sig != Inst::S_BRANCH)
		return Error("Branch condition codes can only be applied to branch instructions.");
	if (Instruct.CondBr != Inst::B_AL)
		return Error("Only one branch condition per instruction, please.");
	Instruct.CondBr = (Inst::condb)cond;
}

void Parser::addRot(int, bool mul)
{
	if (!mul)
		Fail("QPU element rotation is only available with the MUL ALU.");
	auto count = ParseExpression();
	if (NextToken() != COMMA)
		Fail("Expected ',' after rotation count.");

	uint8_t si = 48;
	switch (count.Type)
	{case V_REG:
		if ((count.rValue.Type & R_READ) && count.rValue.Num == 37) // r5
			break;
	 default:
		Fail("QPU rotation needs an integer argument or r5 for the rotation count.");
	 case V_INT:
		si += count.uValue & 0xf;
		if (si == 48)
			return; // Rotation is a multiple of 16 => nothing to do
	}

	doSMI(si);
}

void Parser::doInstrExt(bool mul)
{
	while (NextToken() == DOT)
	{	if (NextToken() != WORD)
			Fail("Expected instruction extension after dot.");

		const opExtEntry* ep = binary_search(extMap, Token.c_str());
		if (ep == NULL)
			Fail("Unknown instruction extension '%s'.", Token.c_str());

		(this->*(ep->Func))(ep->Arg, mul);
	}
	At -= Token.size();
}

void Parser::doALUTarget(bool mul)
{	exprValue param = ParseExpression();
	if (param.Type != V_REG)
		Fail("The first argument to a ALU or branch instruction must be a register or '-', found %s.", Token.c_str());
	if (!(param.rValue.Type & R_WRITE))
		Fail("The register is not writable.");
	if ((param.rValue.Type & R_AB) != R_AB)
	{	bool wsfreeze = Instruct.Pack != Inst::P_32 || mul
			? Instruct.OpA != Inst::A_NOP && !Inst::isWRegAB(Instruct.WAddrA)
			: Instruct.OpM != Inst::M_NOP && !Inst::isWRegAB(Instruct.WAddrM);
		if ((param.rValue.Type & R_A) && (!wsfreeze || Instruct.WS == mul))
			Instruct.WS = mul;
		else if ((param.rValue.Type & R_B) && (!wsfreeze || Instruct.WS != mul))
			Instruct.WS = !mul;
		else
			Fail("ADD ALU and MUL ALU cannot write to the same register file.");
	}
	(mul ? Instruct.WAddrM : Instruct.WAddrA) = param.rValue.Num;
	// vector rotation
	if (param.rValue.Rotate)
	{	if (!mul)
			Fail("Vector rotation is only available to the MUL ALU.");
		if (param.rValue.Rotate == 16)
			Fail("Can only rotate ALU target left by r5.");
		doSMI(48 + (param.rValue.Rotate & 0xf));
	}
}

Inst::mux Parser::doALUExpr(bool mul)
{	if (NextToken() != COMMA)
		Fail("Expected ',' before next argument to ALU instruction, found %s.", Token.c_str());
	exprValue param = ParseExpression();
	switch (param.Type)
	{default:
		Fail("The second argument of a binary ALU instruction must be a register or a small immediate value.");
	 case V_REG:
		{	auto ret = muxReg(param.rValue);
			if (param.rValue.Rotate)
			{	if (!mul)
					Fail("Vector rotation is only available to the MUL ALU.");
				if (param.rValue.Rotate == -16)
					Fail("Can only rotate ALU source right by r5.");
				if (Instruct.Sig == Inst::S_SMI && Instruct.SImmd >= 48)
					Fail("Vector rotation is already applied to the instruction.");
				doSMI(48 + (-param.rValue.Rotate & 0xf));
			}
			return ret;
		}
	 case V_INT:
		{	uint8_t si = getSmallImmediate(param.uValue);
			if (si == 0xff)
				Fail("Value 0x%x does not fit into the small immediate field.", param.uValue);
			doSMI(si);
			return Inst::X_RB;
		}
	}
}

void Parser::doBRASource()
{
	auto param2 = ParseExpression();
	switch (param2.Type)
	{default:
		Fail("Data type is not allowed as branch target.");
	 case V_LABEL:
		if (Instruct.Rel)
			param2.uValue -= (Instructions.size() - Back + 4) * sizeof(uint64_t);
		else
			Msg(WARNING, "Using value of label as target of a absolute branch instruction.");
	 case V_INT:
		if (Instruct.Immd.uValue)
			Fail("Cannot specify two immediate values as branch target.");
		Instruct.Immd = param2;
		break;
	 case V_REG:
		if (!(param2.rValue.Type & R_A) || param2.rValue.Num >= 32)
			Fail("Branch target must be from register file A and no hardware register.");
		if (param2.rValue.Rotate)
			Fail("Cannot use vector rotation with branch instruction.");
		if (Instruct.Reg)
			Fail("Cannot specify two registers as branch target.");
		Instruct.Reg = true;
		Instruct.RAddrA = param2.rValue.Num;
		break;
	}
}

void Parser::assembleADD(int add_op)
{
	if (Instruct.Sig >= Inst::S_LDI)
		Fail("Cannot use ADD ALU in load immediate or branch instruction.");
	if (Instruct.WAddrA != Inst::R_NOP || Instruct.OpA != Inst::A_NOP)
	{	switch (add_op)
		{default:
			Fail("The ADD ALU has already been used in this instruction.");
		 case Inst::A_NOP:
			add_op = Inst::M_NOP; break;
		 case Inst::A_V8ADDS:
			add_op = Inst::M_V8ADDS; break;
		 case Inst::A_V8SUBS:
			add_op = Inst::M_V8SUBS; break;
		}
		// retry with MUL ALU
		return assembleMUL(add_op);
	}

	Instruct.OpA = (Inst::opadd)add_op;
	if (add_op == Inst::A_NOP)
	{	HaveNOP = true;
		return;
	}

	doInstrExt(false);

	doALUTarget(false);

	if (!Instruct.isUnary())
		Instruct.MuxAA = doALUExpr(false);

	Instruct.MuxAB = doALUExpr(false);
}

void Parser::assembleMUL(int mul_op)
{
	if (Instruct.Sig >= Inst::S_LDI)
		Fail("Cannot use MUL ALU in load immediate or branch instruction.");
	if (Instruct.WAddrM != Inst::R_NOP || Instruct.OpM != Inst::M_NOP)
		Fail("The MUL ALU has already been used by the current instruction.");

	Instruct.OpM = (Inst::opmul)mul_op;
	if (mul_op == Inst::M_NOP)
		return;

	doInstrExt(true);

	doALUTarget(true);

	Instruct.MuxMA = doALUExpr(true);
	Instruct.MuxMB = doALUExpr(true);
}

void Parser::assembleMOV(int mode)
{
	if (Instruct.Sig == Inst::S_BRANCH)
		Fail("Cannot use MOV together with branch instruction.");
	bool isLDI = Instruct.Sig == Inst::S_LDI;
	bool useMUL = HaveNOP || Instruct.WAddrA != Inst::R_NOP || (!isLDI && Instruct.OpA != Inst::A_NOP);
	if (useMUL && (Instruct.WAddrM != Inst::R_NOP || (!isLDI && Instruct.OpM != Inst::M_NOP)))
		Fail("Both ALUs are already used by the current instruction.");

	doInstrExt(useMUL);

	doALUTarget(useMUL);

	if (NextToken() != COMMA)
		Fail("Expected ', <source1>' after first argument to ALU instruction, found %s.", Token.c_str());
	exprValue param = ParseExpression();
	switch (param.Type)
	{default:
		Fail("The last parameter of a MOV instruction must be a register or a immediate value. Found %s", type2string(param.Type));
	 case V_REG:
		if (param.rValue.Type & R_SEMA)
		{	// semaphore access by LDI like instruction
			mode = Inst::L_SEMA;
			param.uValue = param.rValue.Num | (param.rValue.Type & R_SACQ);
			break;
		}
		if (isLDI)
			Fail("mov instruction with register source cannot be combined with load immediate.");
		if (param.rValue.Rotate)
		{	if (!useMUL)
				Fail("Vector rotation is only available to the MUL ALU.");
			if (param.rValue.Rotate == 16)
				Fail("Can only rotate ALU source left by r5.");
			if (Instruct.Sig == Inst::S_SMI && Instruct.SImmd >= 48)
				Fail("Vector rotation is already active on this instruction.");
			doSMI(48 + (-param.rValue.Rotate & 0xf));
		}
		if (useMUL)
		{	Instruct.MuxMA = Instruct.MuxMB = muxReg(param.rValue);
			Instruct.OpM = Inst::M_V8MIN;
		} else
		{	Instruct.MuxAA = Instruct.MuxAB = muxReg(param.rValue);
			Instruct.OpA = Inst::A_OR;
		}
		return;
	 case V_LDPES:
		if (mode != Inst::L_LDI && mode != Inst::L_PES)
			Fail("Load immediate mode conflicts with per QPU element constant.");
		mode = Inst::L_PES;
		break;
	 case V_LDPEU:
		if (mode != Inst::L_LDI && mode != Inst::L_PEU)
			Fail("Load immediate mode conflicts with per QPU element constant.");
		mode = Inst::L_PEU;
		break;
	 case V_LDPE:
		if (mode >= Inst::L_SEMA)
			Fail("Load immediate mode conflicts with per QPU element constant.");
		if (mode < Inst::L_PES)
			mode = Inst::L_PES;
		break;
	 case V_INT:
	 case V_FLOAT:
		// try small immediate first
		if (!isLDI && mode < 0)
		{	for (const smiEntry* si = getSmallImmediateALU(param.uValue); si->Value == param.uValue; ++si)
			{	if ( (!si->OpCode.isMul() ^ useMUL)
					&& ( param.uValue == 0 || Instruct.Sig == Inst::S_NONE
						|| (Instruct.Sig == Inst::S_SMI && Instruct.SImmd == si->SImmd) ))
				{	if (param.uValue != 0) // zero value does not require SMI
					{	Instruct.Sig   = Inst::S_SMI;
						Instruct.SImmd = si->SImmd;
					}
					if (si->OpCode.isMul())
					{	Instruct.MuxMA = Instruct.MuxMB = Inst::X_RB;
						Instruct.OpM   = si->OpCode.asMul();
					} else
					{	Instruct.MuxAA = Instruct.MuxAB = Inst::X_RB;
						Instruct.OpA   = si->OpCode.asAdd();
					}
					return;
				}
			}
		}
		// LDI
		if (mode < 0)
			mode = Inst::L_LDI;
	}
	switch (Instruct.Sig)
	{default:
		Fail("Load immediate cannot be used with signals.");
	 case Inst::S_LDI:
		if (Instruct.Immd.uValue != param.uValue || Instruct.LdMode != mode)
			Fail("Tried to load two different immediate values in one instruction. (0x%x vs. 0x%x)", Instruct.Immd.uValue, param.uValue);
	 case Inst::S_NONE:;
	}
	// LDI or semaphore
	if (Instruct.OpA != Inst::A_NOP || Instruct.OpM != Inst::M_NOP)
		Fail("Cannot combine load immediate with value %s with ALU instructions.", param.toString().c_str());
	Instruct.Sig = Inst::S_LDI;
	Instruct.LdMode = (Inst::ldmode)mode;
	Instruct.Immd = param;
}

void Parser::assembleBRANCH(int relative)
{
	if (!Instruct.isVirgin())
		Fail("A branch instruction must be the only one in a line.");

	Instruct.Sig = Inst::S_BRANCH;
	Instruct.CondBr = Inst::B_AL;
	Instruct.Rel = !!relative;
	Instruct.RAddrA = 0;
	Instruct.Reg = false;
	Instruct.Immd.uValue = 0;

	doInstrExt(false);

	doALUTarget(false);

	if (NextToken() != COMMA)
		Fail("Expected ', <branch target>' after first argument to branch instruction, found %s.", Token.c_str());
	doBRASource();
	switch (NextToken())
	{default:
		Fail("Expected ',' or end of line, found '%s'.", Token.c_str());
	 case COMMA: // second branch target
		doBRASource();
		if (NextToken() != END)
			Fail("Expected end of line after branch instruction.");
	 case END:;
	}
}

void Parser::assembleSEMA(int type)
{
	doALUTarget(false);
	if (NextToken() != COMMA)
		Fail("Expected ', <number>' after first argument to semaphore instruction, found %s.", Token.c_str());

	auto param = ParseExpression();
	if (param.Type != V_INT || (param.uValue & ~(type << 4)) >= 16)
		Fail("Semaphore instructions require a single integer argument less than 16 with the semaphore number.");
	param.uValue |= type << 4;

	switch (Instruct.Sig)
	{case Inst::S_LDI:
		if (Instruct.LdMode == Inst::L_LDI)
		{	if ((Instruct.Immd.uValue & 0x1f) != param.uValue)
				Fail("Combining a semaphore instruction with load immediate requires the low order 5 bits to match the semaphore number and the direction bit.");
			break;
		}
	 default:
		Fail("Semaphore Instructions normally cannot be combined with any other instruction.");
	 case Inst::S_NONE:
		Instruct.Sig = Inst::S_LDI;
		Instruct.Immd = param;
	}
	Instruct.LdMode = Inst::L_SEMA;
}

void Parser::assembleSIG(int bits)
{
	switch (Instruct.Sig)
	{default:
		Fail("You must not use more than one signaling flag per line.");
	 case Inst::S_BRANCH:
	 case Inst::S_LDI:
	 case Inst::S_SMI:
		Fail("Signaling bits cannot be combined with branch instruction or immediate values.");
	 case Inst::S_NONE:
		Instruct.Sig = (Inst::sig)bits;
	}
}

void Parser::ParseInstruction()
{
	while (true)
	{	const opEntry<8>* op = binary_search(opcodeMap, Token.c_str());
		if (!op)
			return Error("Invalid opcode or unknown macro: %s", Token.c_str());

		if (Preprocessed)
			fputs(Token.c_str(), Preprocessed);
		(this->*op->Func)(op->Arg);

		switch (NextToken())
		{default:
			Fail("Expected end of line after instruction.");
		 case END:
			return;
		 case SEMI:
			switch (NextToken())
			{default:
				Fail("Expected additional instruction after ';'.");
			 case END:
				return;
			 case WORD:;
			}
		}
	}
}

void Parser::defineLabel()
{
	// Lookup symbol
	const auto& lname = LabelsByName.emplace(Token, LabelCount);
	label* lp;
	if (lname.second)
	{	// new label, not yet referenced
	 new_label:
		if (!Pass2)
			Labels.emplace_back(Token);
		lp = &Labels[LabelCount];
		++LabelCount;
	} else
	{	// Label already in the lookup table.
		lp = &Labels[lname.first->second];
		if (lp->Definition)
		{	// redefinition
			if (!isdigit(lp->Name[0]))
				return Error("Redefinition of non-local label %s, previously defined at %s.",
					Token.c_str(), lp->Definition.toString().c_str());
			// redefinition allowed, but this is always a new label
			lname.first->second = LabelCount;
			goto new_label;
		}
	}
	if (!Pass2)
		lp->Value = (Instructions.size() - Back) * sizeof(uint64_t);
	else if (lp->Name != Token || lp->Value != (Instructions.size() - Back) * sizeof(uint64_t))
		return Error("Inconsistent Label definition during Pass 2.");
	lp->Definition = *Context.back();

	if (Preprocessed)
	{	fputs(Token.c_str(), Preprocessed);
		fputs(": ", Preprocessed);
	}
}

void Parser::parseLabel()
{
	switch (NextToken())
	{default:
		return Error("Expected label name after ':'.");
	 case WORD:
	 case NUM:;
	}

	defineLabel();
}

void Parser::beginREP(int)
{
	if (doPreprocessor())
		return;

	AtMacro = &Macros[".rep"];
	AtMacro->Definition = *Context.back();

	if (NextToken() != WORD)
		return Error("Expected loop variable name after .rep.");

	AtMacro->Args.push_back(Token);
	if (NextToken() != COMMA)
		return Error("Expected ', <count>' at .rep.");
	const auto& expr = ParseExpression();
	if (expr.Type != V_INT)
		return Error("Second argument to .rep must be an integral number. Found %s", expr.toString().c_str());
	if (NextToken() != END)
		return Error("Expected end of line.");
	AtMacro->Args.push_back(expr.toString());
}

void Parser::endREP(int)
{
	auto iter = Macros.find(".rep");
	if (AtMacro != &iter->second)
	{	if (doPreprocessor())
			return;
		return Error(".endr without .rep");
	}

	const macro m = *AtMacro;
	AtMacro = NULL;
	Macros.erase(iter);

	if (m.Args.size() != 2)
		return; // no loop count => 0

	// Setup invocation context
	saveContext ctx(*this, new fileContext(CTX_MACRO, m.Definition.File, m.Definition.Line));

	// loop
	uint32_t count;
	sscanf(m.Args[1].c_str(), "%i", &count);
	auto& current = *Context.back();
	for (uint32_t& i = current.Consts.emplace(m.Args.front(), constDef(exprValue(0), current)).first->second.Value.uValue;
		i < count; ++i)
	{	// Invoke rep
		for (const string& line : m.Content)
		{	++Context.back()->Line;
			strncpy(Line, line.c_str(), sizeof(Line));
			ParseLine();
		}
	}
}

void Parser::beginBACK(int)
{
	if (doPreprocessor())
		return;

	if (Back)
		return Error("Cannot nest .back directives.");
	exprValue param = ParseExpression();
	if (param.Type != V_INT)
		return Error("Expected integer constant after .back.");
	if (param.uValue > 3)
		return Error("Cannot move instructions more than 3 slots back.");
	if (param.uValue > Instructions.size())
		return Error("Cannot move instructions back before the start of the code.");
	if (NextToken() != END)
		return Error("Expected end of line, found '%s'.", Token.c_str());
	Back = param.uValue;
}

void Parser::endBACK(int)
{
	if (doPreprocessor())
		return;

	if (!Back)
		return Error(".endb without .back.");
	if (NextToken() != END)
		return Error("Expected end of line, found '%s'.", Token.c_str());
	Back = 0;
}

void Parser::parseSET(int flags)
{
	if (doPreprocessor())
		return;

	if (NextToken() != WORD)
		return Error("Directive .set requires identifier.");
	string Name = Token;
	if (NextToken() != COMMA)
		return Error("Directive .set requires ', <value>'.");

	exprValue expr = ParseExpression();
	if (NextToken() != END)
		return Error("Syntax error: unexpected %s.", Token.c_str());

	auto& current = flags & 1 ? *Context.back() : *Context.front();
	auto r = current.Consts.emplace(Name, constDef(expr, current));
	if (!r.second)
	{	if (flags & 2)
			// redefinition not allowed
			return Error("Identifier %s has already been defined at %s.",
				Name.c_str(), r.first->second.Definition.toString().c_str());
		r.first->second.Value = expr;
	}
}

void Parser::parseUNSET(int flags)
{
	if (doPreprocessor())
		return;

	if (NextToken() != WORD)
		return Error("Directive .unset requires identifier.");
	string Name = Token;
	if (NextToken() != END)
		return Error("Syntax error: unexpected %s.", Token.c_str());

	auto& consts = (flags & 1 ? Context.back() : Context.front())->Consts;
	auto r = consts.find(Name);
	if (r == consts.end())
		return Msg(WARNING, "Cannot unset %s because it has not yet been definied in the required context.", Name.c_str());
	consts.erase(r);
}

bool Parser::doCondition()
{
	const exprValue& param = ParseExpression();
	if (param.Type != V_INT)
		Fail("Conditional expression must be a integer constant, found '%s'.", param.toString().c_str());
	if (NextToken() != END)
		Fail("Expected end of line, found '%s'.", Token.c_str());
	return param.uValue != 0;
}

void Parser::parseIF(int)
{
	if (doPreprocessor(PP_MACRO))
		return;

	AtIf.emplace_back(Context.back()->Line, isDisabled() ? 4 : doCondition());
}

void Parser::parseELSEIF(int)
{
	if (doPreprocessor(PP_MACRO))
		return;

	if (!AtIf.size())
		return Error(".elseif without .if");

	if (AtIf.back().State == 0)
		AtIf.back().State = doCondition();
	else
		AtIf.back().State |= 2;
}

void Parser::parseELSE(int)
{
	if (doPreprocessor(PP_MACRO))
		return;

	if (!AtIf.size())
		return Error(".else without .if");
	if (NextToken() != END)
		Error("Expected end of line. .else has no arguments.");

	++AtIf.back().State;
}

void Parser::parseENDIF(int)
{
	if (doPreprocessor(PP_MACRO))
		return;

	if (!AtIf.size())
		return Error(".endif without .if");
	if (NextToken() != END)
		Error("Expected end of line. .endif has no arguments.");

	AtIf.pop_back();
}

void Parser::parseASSERT(int)
{
	if (doPreprocessor())
		return;

	if (!doCondition())
		Error("Assertion failed.");
}

void Parser::beginMACRO(int)
{
	if (doPreprocessor(PP_IF))
		return;

	if (AtMacro)
		return Error("Cannot nest macro definitions.\n"
		     "  In definition of macro starting at %s.",
		  AtMacro->Definition.toString().c_str());
	if (NextToken() != WORD)
		return Error("Expected macro name.");
	AtMacro = &Macros[Token];
	if (AtMacro->Definition.File.size())
	{	Msg(INFO, "Redefinition of macro %s.\n"
		          "  Previous definition at %s.",
		  Token.c_str(), AtMacro->Definition.toString().c_str());
		// redefine
		AtMacro->Args.clear();
		AtMacro->Content.clear();
	}
	AtMacro->Definition = *Context.back();

	while (true)
		switch (NextToken())
		{default:
			return Error("Expected ',' before (next) macro argument.");
		 case NUM:
			return Error("Macro arguments must not be numbers.");
		 case COMMA:
			// Macro argument
			if (NextToken() != WORD)
				return Error("Macro argument name expected. Found '%s'.", Token.c_str());
			AtMacro->Args.push_back(Token);
			break;
		 case END:
			return;
		}
}

void Parser::endMACRO(int)
{
	if (doPreprocessor(PP_IF))
		return;

	if (!AtMacro)
		return Error(".endm outside a macro definition.");
	AtMacro = NULL;
	if (NextToken() != END)
		Error("Expected end of line. .endm has no arguments.");
}

void Parser::doMACRO(macros_t::const_iterator m)
{
	// Fetch macro arguments
	const auto& argnames = m->second.Args;
	vector<exprValue> args;
	if (argnames.size())
	{	args.reserve(argnames.size());
		while (true)
		{	args.emplace_back(ParseExpression());
			switch (NextToken())
			{default:
				Fail("internal error");
			 case COMMA:
				if (args.size() == argnames.size())
					return Error("Too much arguments for macro %s.", m->first.c_str());
				continue;
			 case END:
				if (args.size() != argnames.size())
					return Error("Too few arguments for macro %s.", m->first.c_str());
			}
			break;
		}
	}

	// Setup invocation context
	saveContext ctx(*this, new fileContext(CTX_MACRO, m->second.Definition.File, m->second.Definition.Line));

	// setup args inside new context to avoid interaction with argument values that are also functions.
	auto& current = *Context.back();
	current.Consts.reserve(current.Consts.size() + argnames.size());
	size_t n = 0;
	for (auto arg : argnames)
		current.Consts.emplace(arg, constDef(args[n++], current));

	// Invoke macro
	for (const string& line : m->second.Content)
	{	++Context.back()->Line;
		strncpy(Line, line.c_str(), sizeof(Line));
		ParseLine();
	}
}

void Parser::defineFUNC(int)
{
	if (doPreprocessor())
		return;

	if (NextToken() != WORD)
		return Error("Expected function name");
	string name = Token;

	function func(*Context.back());

	if (NextToken() != BRACE1)
		return Error("Expected '(' after function name.");
 next:
	if (NextToken() != WORD)
		return Error("Function argument name expected. Found '%s'.", Token.c_str());
	func.Args.push_back(Token);
	switch (NextToken())
	{default:
		return Error("Expected ',' or ')' after function argument.");
	 case NUM:
		return Error("Function arguments must not start with a digit.");
	 case COMMA:
		goto next;
	 case END:
		return Error("Unexpected end of function argument definition");
	 case BRACE2:
		break;
	}

	// Anything after ')' is function body and evaluated delayed
	At += strspn(At, " \t\r\n");
	func.DefLine = Line;
	func.Start = At;

	const auto& ret = Functions.emplace(name, func);
	if (!ret.second)
	{	Msg(INFO, "Redefinition of function %s.\n"
		      "Previous definition at %s.",
		  Token.c_str(), ret.first->second.Definition.toString().c_str());
		ret.first->second = func;
	}
}

exprValue Parser::doFUNC(funcs_t::const_iterator f)
{
	if (NextToken() != BRACE1)
		Fail("Expected '(' after function name.");

	// Fetch macro arguments
	const auto& argnames = f->second.Args;
	vector<exprValue> args;
	args.reserve(argnames.size());
	if (argnames.size() == 0)
	{	// no arguments
		if (NextToken() != BRACE2)
			Fail("Expected ')' because function %s has no arguments.", f->first.c_str());
	} else
	{next:
		args.push_back(ParseExpression());
		switch (NextToken())
		{case BRACE2:
			// End of argument list. Are we complete?
			if (args.size() != argnames.size())
				Fail("Too few arguments for function %s. Expected %u, found %u.", f->first.c_str(), argnames.size(), args.size());
			break;
		 default:
			Fail("Unexpected '%s' in argument list of function %s.", Token.c_str(), f->first.c_str());
		 case COMMA:
			// next argument
			if (args.size() == argnames.size())
				Fail("Too much arguments for function %s. Expected %u.", f->first.c_str(), argnames.size());
			goto next;
		}
	}

	// Setup invocation context
	saveLineContext ctx(*this, new fileContext(CTX_FUNCTION, f->second.Definition.File, f->second.Definition.Line));
	strncpy(Line, f->second.DefLine.c_str(), sizeof Line);
	At = f->second.Start;
	// setup args inside new context to avoid interaction with argument values that are also functions.
	auto& current = *Context.back();
	current.Consts.reserve(current.Consts.size() + argnames.size());
	unsigned n = 0;
	for (auto arg : argnames)
		current.Consts.emplace(arg, constDef(args[n++], current));

	const exprValue&& val = ParseExpression();
	if (NextToken() != END)
		Fail("Function %s evaluated to an incomplete expression.", f->first.c_str());
	return val;
}

void Parser::doINCLUDE(int)
{
	if (doPreprocessor())
		return;

	At += strspn(At, " \t\r\n");
	{	// remove trailing blanks
		char* cp = At + strlen(At);
		while (strchr(" \t\r\n", *--cp))
			*cp = 0;
	}
	size_t len = strlen(At);
	if (*At != '"' || At[len-1] != '"')
		return Error("Syntax error. Expected \"<file-name>\" after .include, found '%s'.", At);
	Token.assign(At+1, len-2);

	Token = relpath(Context.back()->File, Token);

	saveContext ctx(*this, new fileContext(CTX_INCLUDE, Token, 0));

	ParseFile();
}

void Parser::ParseDirective()
{
	if (NextToken() != WORD)
		return Error("Expected assembler directive after '.'. Found '%s'.", Token.c_str());

	const opEntry<8>* op = binary_search(directiveMap, Token.c_str());
	if (!op)
		return Error("Invalid assembler directive: %s", Token.c_str());

	(this->*op->Func)(op->Arg);
}

bool Parser::doPreprocessor(preprocType type)
{
	if (AtMacro && (type & PP_MACRO))
	{	AtMacro->Content.push_back(Line);
		return true;
	}
	return (type & PP_IF) && isDisabled();
}

void Parser::ParseLine()
{
	At = Line;

 next:
	switch (NextToken())
	{default:
		return Error("Syntax error");

	 case DOT:
		// directives
		ParseDirective();
	 case END:
		return;

	 case COLON:
		if (doPreprocessor())
			return;

		parseLabel();
		goto next;

	 case WORD:
		if (doPreprocessor())
			return;

		// read-ahead to see if the next token is a colon in which case
		// this is a label.
		if (*At == ':')
		{	defineLabel();
			++At;
			goto next;
		}

		// Try macro
		macros_t::const_iterator m = Macros.find(Token);
		if (m != Macros.end())
		{	doMACRO(m);
			return;
		}

		Instruct.reset();
		HaveNOP = false;
		ParseInstruction();
		Instruct.optimize();
		Instructions.insert(Instructions.end() - Back, Instruct.encode());
		return;
	}
}

void Parser::ParseFile()
{
	FILE* f = fopen(Context.back()->File.c_str(), "r");
	if (!f)
		return Error("Failed to open file %s.", Context.back()->File.c_str());
	try
	{	while (fgets(Line, sizeof(Line), f))
		{	++Context.back()->Line;
			try
			{	ParseLine();
			} catch (const string& msg)
			{	// recover from errors
				fputs(msgpfx[ERROR], stderr);
				fputs(msg.c_str(), stderr);
				fputc('\n', stderr);
			}

			if (AtMacro && AtMacro->Definition.Line == 0)
				Fail("!!!");
		}
		fclose(f);
	} catch (...)
	{	fclose(f);
		throw;
	}
}
void Parser::ParseFile(const string& file)
{	if (Pass2)
		throw string("Cannot add another file after pass 2 has been entered.");
	saveContext ctx(*this, new fileContext(CTX_INCLUDE, file, 0));
	ParseFile();
	Filenames.emplace_back(file);
}

void Parser::ResetPass()
{	AtMacro = NULL;
	AtIf.clear();
	Context.clear();
	Context.emplace_back(new fileContext(CTX_ROOT, string(), 0));
	Functions.clear();
	Macros.clear();
	LabelsByName.clear();
	LabelCount = 0;
	Instructions.clear();
}

void Parser::EnsurePass2()
{
	if (Pass2 || !Success)
		return;

	// enter pass 2
	Pass2 = true;
	ResetPass();

	// Check all labels
	for (auto& label : Labels)
	{	if (!label.Definition)
			return Error("Label '%s' is undefined. Referenced from %s.\n",
				label.Name.c_str(), label.Reference.toString().c_str());
		label.Definition.Line = 0;
	}

	for (auto file : Filenames)
	{	saveContext ctx(*this, new fileContext(CTX_INCLUDE, file, 0));
		ParseFile();
	}
}

Parser::Parser()
{	Reset();
}

void Parser::Reset()
{ ResetPass();
	Labels.clear();
	Pass2 = false;
	Filenames.clear();
}

const vector<uint64_t>& Parser::GetInstructions()
{
	/*for (fixup fix : Fixups)
	{	const label& l = Labels[fix.Label];
		if (!l.Definition)
		{	Success = false;
			fprintf(stderr, "Label '%s' is undefined. Referenced from %s.\n",
				l.Name.c_str(), l.Reference.toString().c_str());
		} else
			Instructions[fix.Instr] += l.Value - (uint64_t)(Instructions[fix.Instr] & 0x80000000) * 2;
	}
	Fixups.clear();*/

	EnsurePass2();

	return Instructions;
}
