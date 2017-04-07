/*
 * Parser.cpp
 *
 *  Created on: 30.08.2014
 *      Author: mueller
 */

#define __STDC_FORMAT_MACROS // Work around for older g++

#include "Parser.h"

#include <cstdio>
#include <cstdlib>
#include <cinttypes>
#include <cassert>
#include <cfloat>
#include <cmath>
#include <algorithm>
#include <cctype>
#include <sys/stat.h>
#include <inttypes.h>

#include "Parser.tables.cpp"

#ifndef UINT64_MAX
#define UINT64_MAX (~(uint64_t)0)
#endif

Parser::saveContext::saveContext(Parser& parent, fileContext* ctx)
:	Parent(parent)
,	Context(ctx)
{	parent.Context.emplace_back(ctx);
}

Parser::saveContext::~saveContext()
{	auto& ctx = *Parent.Context.back();
	if (Context != &ctx)
		throw stringf("Unterminated context block in the current file at line %u.", ctx.Line);
	Parent.Context.pop_back();
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
		{	auto fname = fName(ctx.File);
			switch (type)
			{case CTX_CURRENT:
				msg = stringf("%s (%u,%zi): ", fname, ctx.Line, At - Line - Token.size() + 1) + msg;
				break;
			 case CTX_INCLUDE:
				msg += stringf("\n  Included from %s (%u)", fname, ctx.Line);
				break;
			 case CTX_MACRO:
				msg += stringf("\n  At invocation of macro from %s (%u)", fname, ctx.Line);
				break;
			 case CTX_FUNCTION:
				msg += stringf("\n  At function invocation from %s (%u)", fname, ctx.Line);
				break;
			 default:;
			}
		}
		type = ctx.Type;
	}
	return msg;
}

void Parser::Fail(const char* fmt, ...)
{	va_list va;
	va_start(va, fmt);
	const string& msg = vstringf(fmt, va);
	va_end(va);
	throw enrichMsg(msg);
}

static const char msgpfx[][10]
{	"ERROR: "
,	"Warning: "
,	"Info: "
};

void Parser::CaughtMsg(const char* msg)
{	Success = false;
	if (OperationMode == IGNOREERRORS && !Pass2)
		return;
	fputs(msgpfx[ERROR], stderr);
	fputs(msg, stderr);
	fputc('\n', stderr);
}

void Parser::Msg(severity level, const char* fmt, ...)
{	if (Verbose < level)
		return;
	switch (OperationMode)
	{case NORMAL:
		if (level == ERROR)
			break;
	 case IGNOREERRORS:
		if (!Pass2)
			return;
	 default:;
	}
	Success &= level > ERROR;
	va_list va;
	va_start(va, fmt);
	fputs(msgpfx[level], stderr);
	fputs(enrichMsg(vstringf(fmt, va)).c_str(), stderr);
	va_end(va);
	fputc('\n', stderr);
}

void Parser::FlagsSize(size_t min)
{	if (InstFlags.size() < min)
		InstFlags.resize(min, IF_NONE);
}

void Parser::StoreInstruction(uint64_t inst)
{
	if (!Pass2)
		Instructions.emplace_back();
	LineNumbers.emplace_back();
	uint64_t* ptr = &Instructions[PC+Back];
	uint64_t* ip  = ptr - Back;
	instFlags* fp = &InstFlags[PC+Back];
	location* lp  = &LineNumbers[PC+Back];
	while (ptr != ip)
	{	*ptr = ptr[-1];
		*fp  = fp[-1];
		*lp  = lp[-1];
		--ptr;
		--fp;
		--lp;
	}
	*ptr = inst;
	*fp = Flags;
	*lp  = *Context.back();
}

Parser::token_t Parser::NextToken()
{	size_t i;
	token_t ret;
 restart:
	switch (*At)
	{case 0:
	 case '#':
		Token.clear();
		return END;
	 case ' ':
	 case '\t':
	 case '\r':
	 case '\n':
		ToNextChar();
		goto restart;
	 case '!':
		i = !memchr("^=", 2, At[1]) ? 0 : 1 + (At[2] == At[1]);
		goto op;
	 case '%':
	 case '+':
	 case '-':
	 case '/':
	 case '~':
		i = 0;
	 op:
		ret = OP;
		break;
	 case '&':
	 case '*':
	 case '^':
	 case '|':
		i = At[1] == *At;
		goto op;
	 case '<':
	 case '>':
		i = strspn(At+1, "<=>");
		goto op;
	 case '=':
		i = At[1] != '=' ? 0 : 1 + (At[2] == '=');
		goto op;
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
	 case '0':
	 case '1':
	 case '2':
	 case '3':
	 case '4':
	 case '5':
	 case '6':
	 case '7':
	 case '8':
	 case '9':
		i = strcspn(At+1, ",;:+-*/%()[]&|^~!=<># \t\r\n");
		ret = NUM;
		break;
	 default:
		i = strcspn(At+1, ".,;:+-*/%()[]&|^~!=<># \t\r\n");
		ret = WORD;
	}
	Token.assign(At, ++i);
	At += i;
	return ret;
}

size_t Parser::parseInt(const char* src, int64_t& dst)
{	dst = 0;
	const char* cp = src;
	uint64_t basis = 10;
	int64_t limit = UINT64_MAX / 10;
	for (char c; (c = *cp) != 0; ++cp)
	{	int digit = c - '0';
		if (digit >= 10)
		{	digit = toupper(c) - 'A';
			if (digit < 6)
				digit += 10;
			else
			{	if (dst == 0 && cp - src == 1)
					switch (c)
					{	case 'b': basis = 2; limit = UINT64_MAX/2; continue;
						case 'o': basis = 8; limit = UINT64_MAX/8; continue;
						case 'x': basis = 16; limit = UINT64_MAX/16; continue;
						case 'd': continue;
					}
				break;
			}
		}
		if (dst > limit)
			break;
		dst *= basis;
		if ((uint64_t)dst > UINT64_MAX - digit)
			break;
		dst += digit;
	}
	return cp - src;
}

Parser::label& Parser::labelRef(string name, bool forward)
{
	const auto& l = LabelsByName.emplace(name, LabelCount);
	if (!!l.second || (forward && !!Labels[l.first->second].Definition))
	{	// new label
		l.first->second = LabelCount;
		if (!Pass2)
			Labels.emplace_back(Token);
		else if (Labels.size() <= LabelCount || Labels[LabelCount].Name != Token)
			Fail("Inconsistent label definition during Pass 2.");
		Labels[LabelCount].Reference = *Context.back();
		++LabelCount;
	}
	return Labels[l.first->second];
}

void Parser::parseElemInt()
{	uint32_t value = 0;
	int pos = 0;
	signed char sign = 0;
	auto savectx = InstCtx;
	InstCtx = IC_NONE;

	while (true)
	{	ParseExpression();
		if (ExprValue.Type != V_INT)
			Fail("Only integers are allowed between [...].");
		if (ExprValue.iValue < -2 || ExprValue.iValue > 3)
			Fail("Load per QPU element can only deal with integer constants in the range of [-2..3].");
		if (ExprValue.iValue < 0)
		{	if (sign > 0)
				Fail("All integers in load per QPU element must be either in the range [-2..1] or in the range [0..3].");
			sign = -1;
			ExprValue.iValue &= 3;
		} else if (ExprValue.iValue > 1)
		{	if (sign < 0)
				Fail("All integers in load per QPU element must be either in the range [-2..1] or in the range [0..3].");
			sign = 1;
		}
		value |= (ExprValue.iValue * 0x8001 & 0x10001) << pos;

		switch (NextToken())
		{case END:
			Fail("Incomplete expression.");
		 default:
			Fail("Unexpected '%s' in per QPU element constant.", Token.c_str());
		 case COMMA:
			if (++pos >= 16)
				Fail("Too many initializers for per QPU element constant.");
			continue;
		 case SQBRC2:
			if (pos != 15)
				Fail("Too few initializers for per QPU element constant.");
		}
		break;
	}
	InstCtx = savectx;
	// Make LDIPEx
	ExprValue = exprValue(value, (valueType)(sign + V_LDPE));
}

void Parser::parseRegister()
{	auto savectx = InstCtx;
	InstCtx = IC_NONE;
	reg_t reg;
	ParseExpression();
	if (ExprValue.Type != V_INT)
		Fail("Expecting integer constant, found %s.", type2string(ExprValue.Type));
	reg.Num = (uint8_t)ExprValue.iValue;
	if (NextToken() != COMMA)
		Fail("Expected ',', found '%s'.", Token.c_str());
	ParseExpression();
	if (ExprValue.Type != V_INT)
		Fail("Expecting integer constant, found %s.", type2string(ExprValue.Type));
	reg.Type = (regType)ExprValue.iValue;
	switch (NextToken())
	{default:
		Fail("Expected ',' or ']', found '%s'.", Token.c_str());
	 case COMMA:
		ParseExpression();
		if (ExprValue.Type != V_INT)
			Fail("Expecting integer constant, found %s.", type2string(ExprValue.Type));
		reg.Rotate = (uint8_t)ExprValue.iValue;
		switch (NextToken())
		{default:
			Fail("Expected ',' or ']', found '%s'.", Token.c_str());
		 case COMMA:
			ParseExpression();
			if (ExprValue.Type != V_INT)
				Fail("Expecting integer constant, found %s.", type2string(ExprValue.Type));
			reg.Pack.Mode = (int8_t)ExprValue.iValue;
			if (NextToken() != SQBRC2)
				Fail("Expected ']', found '%s'.", Token.c_str());
			break;
		 case SQBRC2:
			reg.Pack.Mode = 0;
		}
		if (NextToken() != SQBRC2)
			Fail("Expected ']', found '%s'.", Token.c_str());
		break;
	 case SQBRC2:
		reg.Rotate = 0;
		reg.Pack.Mode = 0;
	}
	ExprValue = reg;
	InstCtx = savectx;
}

void Parser::ParseExpression()
{
	Eval eval;
	try
	{next:
		switch (NextToken())
		{default:
		 discard:
			At -= Token.size();
			ExprValue = eval.Evaluate();
			return;

		 case WORD:
			{	// Expand constants
				for (auto i = Context.end(); i != Context.begin(); )
				{	auto c = (*--i)->Consts.find(Token);
					if (c != (*i)->Consts.end())
					{	ExprValue = c->second.Value;
						goto have_value;
					}
				}
			}
			{	// try function
				auto fp = Functions.find(Token);
				if (fp != Functions.end())
				{	// Hit!
					doFUNC(fp);
					break;
				}
			}
			{	// try functional macro
				auto mp = MacroFuncs.find(Token);
				if (mp != MacroFuncs.end())
				{	doFUNCMACRO(mp);
					break;
				}
			}
			{	// try register
				auto rp = binary_search(regMap, Token.c_str());
				if (rp)
				{	ExprValue = rp->Value;
					break;
				}
			}
			{	// try alphanumeric operator
				auto op = binary_search(operatorMap2, Token.c_str());
				if (op)
				{	if (!eval.PushOperator(op->Op))
						goto discard;
					goto next;
				}
			}
			// Try label prefix
			{	string identifier = Token;
				if (NextToken() != COLON)
					Fail("Invalid expression. The identifier %s is undefined.", identifier.c_str());
				if (identifier != "r")
					Fail("'%s:' is no valid label prefix.", identifier.c_str());
			}
		 case COLON: // Label
			{	// Is forward reference?
				bool forward = false;
				switch (NextToken())
				{default:
					Fail("Expected Label after ':'.");

				 case BRACE1: // Internal label constant
					if ( NextToken() != NUM
						|| parseInt(Token.c_str(), ExprValue.iValue) != Token.length() )
						Fail("Expecting integer constant, found %s.", Token.c_str());
					if (NextToken() != BRACE2)
						Fail("Expected ')', found '%s'.", Token.c_str());
					ExprValue.Type = V_LABEL;
					goto have_value;

				 case SQBRC1: // Internal register constant
					parseRegister();
					goto have_value;

				 case NUM:
					// NextToken accepts some letters in numeric constants
					if (Token.back() == 'f') // forward reference
					{	forward = true;
						Token.erase(Token.size()-1);
					}
				 case WORD:;
				}
				ExprValue = exprValue(labelRef(Token, forward).Value, V_LABEL);
			}
			break;

		 case OP:
		 case BRACE1:
		 case BRACE2:
			{	const opInfo* op = binary_search(operatorMap, Token.c_str());
				if (!op)
					Fail("Invalid operator: %s", Token.c_str());
				if (!eval.PushOperator(op->Op))
					goto discard;
				ToNextChar();
				if (*At == '.')
				{	ExprValue = eval.PeekExpression();
					doInstrExt();
					eval.PeekExpression() = ExprValue;
				}
				goto next;
			}
		 case SQBRC1: // per QPU element constant
			parseElemInt();
			break;

		 case NUM:
			// parse number
			if (Token.find('.') != string::npos || (Token.find_first_of("eE") != string::npos && (Token[1] & 0xdf) != 'X'))
			{	// float number
				size_t len;
				if (sscanf(Token.c_str(), "%lf%zn", &ExprValue.fValue, &len) != 1 || len != Token.size())
					Fail("%s is no real number.", Token.c_str());
				ExprValue.Type = V_FLOAT;
			} else
			{	// integer
				//size_t len;  sscanf of gcc4.8.2/Linux x32 can't read "0x80000000".
				//if (sscanf(Token.c_str(), "%i%n", &stack.front().iValue, &len) != 1 || len != Token.size())
				if (parseInt(Token.c_str(), ExprValue.iValue) != Token.size())
					Fail("%s is no integral number.", Token.c_str());
				ExprValue.Type = V_INT;
			}
			break;
		}
	 have_value:
		ToNextChar();
		if (*At == '.')
			doInstrExt();

		eval.PushValue(ExprValue);
		goto next;
	} catch (const Message& msg) // Messages from Eval are not yet enriched.
	{	throw enrichMsg(msg);
	}
}

int Parser::ArgumentCount(const char* rem, int max)
{
	for (int count = 1; true; ++rem)
	{	rem += strcspn(rem, ",;()[]#");
		switch (*rem)
		{	int brclevel;
		 default:
			return count;
		 case '(':
			brclevel = 1;
			do
			{	rem += strcspn(++rem, "()");
				if (!*rem)
					return count;
				brclevel += *rem == '(' ? 1 : -1;
			} while (brclevel);
			break;
		 case '[':
			brclevel = 1;
			do
			{	rem += strcspn(++rem, "[]");
				if (!*rem)
					return count;
				brclevel += *rem == '[' ? 1 : -1;
			} while (brclevel);
			break;
		 case ',':
			if (++count >= max)
				return count;
			break;
		}
	}
}

void Parser::addIf(int cond)
{	applyIf((::Inst::conda)cond);
}

void Parser::addPUp(int mode)
{	rPUp pup { (uint8_t)mode };
	// Operator context => apply immediately
	if (InstCtx & IC_OP)
		return applyPackUnpack(pup);
	// else apply to expression
	if (ExprValue.Type != V_REG)
		Fail("Pack/unpack options are only available for registers.");
	if (ExprValue.rValue.Pack)
		Fail("Already applied pack/unpack to the current expression.");
	ExprValue.rValue.Pack = pup;
}

void Parser::addSetF(int)
{	applySetF();
}

void Parser::addCond(int cond)
{	applyCond((::Inst::condb)cond);
}

void Parser::addRot(int)
{
	auto savectx = InstCtx;
	InstCtx = IC_NONE;
	ParseExpression();
	InstCtx = savectx;
	if (NextToken() != COMMA)
		Fail("Expected ',' after rotation count.");

	int si;
	switch (ExprValue.Type)
	{case V_REG:
		si = 0;
		if ((ExprValue.rValue.Type & R_READ) && ExprValue.rValue.Num == 37 && !ExprValue.rValue.Rotate && !ExprValue.rValue.Pack) // r5
			break;
	 default:
		Fail("QPU rotation needs an integer argument or r5 for the rotation count.");
	 case V_INT:
		si = (int)ExprValue.iValue & 0xf;
		if (si == 0)
			return; // Rotation is a multiple of 16 => nothing to do
	}

	applyRot(si);
}

void Parser::doInstrExt()
{
	while (NextToken() == DOT)
	{	switch (NextToken())
		{default:
			Fail("Expected instruction extension after dot.");
		 case WORD:
		 case NUM:;
		}
		const opExtEntry* ep = binary_search(extMap, Token.c_str());
		if (!ep)
			Fail("Unknown instruction extension '%s'.", Token.c_str());
		if ((ep->Where & InstCtx) == 0)
			Fail("Invalid instruction extension '%s' within this context (%x).", Token.c_str(), InstCtx);
		(this->*(ep->Func))(ep->Arg);
	}
	At -= Token.size();
}

void Parser::doALUTarget()
{	InstCtx = (InstCtx & ~IC_OP) | IC_DST;
	ParseExpression();
	if (ExprValue.Type != V_REG)
		Fail("The target argument to a ALU or branch instruction must be a register or '-', found %s.", ExprValue.toString().c_str());
	if (!(ExprValue.rValue.Type & R_WRITE))
		Fail("The register is not writable.");

	applyTarget(ExprValue.rValue);
}

void Parser::doALUExpr()
{	if (NextToken() != COMMA)
		Fail("Expected ',' before next argument to ALU instruction, found '%s'.", Token.c_str());
	ParseExpression();

	applyALUSource(ExprValue);
}

void Parser::doNOP()
{	Flags |= IF_HAVE_NOP;

	ToNextChar();
	if (strchr(";#", *At))
		return;

	InstCtx = IC_DST|IC_MUL;
	doALUTarget();

	ToNextChar();
	if (!strchr(";#", *At))
		Fail("Expected end of instruction.");
}

void Parser::assembleADD(int add_op)
{
	int args = applyADD((::Inst::opadd)add_op);
	if (args == 0)
		return doNOP();
	ExprValue.Type = V_NONE;
	doInstrExt();

	doALUTarget();

	InstCtx ^= IC_DST|IC_SRCA;
	doALUExpr();
	if (args == 2)
	{	InstCtx ^= IC_SRC;
		doALUExpr();
	}
}

void Parser::assembleMUL(int mul_op)
{
	if (applyMUL((::Inst::opmul)mul_op) == 0)
		return doNOP();
	ExprValue.Type = V_NONE;
	doInstrExt();

	doALUTarget();

	InstCtx ^= IC_DST|IC_SRCA;
	doALUExpr();
	InstCtx ^= IC_SRC;
	doALUExpr();
}

void Parser::assembleMOV(int mode)
{
	bool target2 = ArgumentCount(At, 3) == 3;
	prepareMOV(target2);
	ExprValue.Type = V_NONE;
	doInstrExt();

	doALUTarget();

	if (NextToken() != COMMA)
		Fail("Expected comma after first argument to mov, ldi or semaphore instruction, found '%s'.", Token.c_str());

	if (target2)
	{	// second target
		InstCtx ^= IC_ADD|IC_MUL; // switch ALU
		doALUTarget();
		// From here we are double ALU
		InstCtx |= IC_ADD|IC_MUL; // now we are at both ALUs

		if (NextToken() != COMMA)
			Fail("Expected comma and source argument, found '%s'.", Token.c_str());
	}

	InstCtx ^= IC_DST|IC_SRC; // Swap to source context
	ParseExpression();
	ToNextChar();
	if (!strchr(";#", *At))
		Fail("Expected end of instruction.");

	// Try ALU expression first
	if (mode < 0)
	{	if (applyMOVsrc(ExprValue))
			return;
		mode = ::Inst::L_LDI;
	}
	// Try LDI
	applyLDIsrc(ExprValue, (::Inst::ldmode)mode);
}

/*void Parser::assembleSEMA(int type)
{	InstCtx = IC_ADD|IC_OP;

	bool combine;
 	switch (Sig)
	{case S_LDI:
		combine = true;
		if (LdMode == L_LDI)
			break;
	 default:
	 fail:
		Fail("Semaphore Instructions normally cannot be combined with any other instruction.");
	 case S_NONE:
		if ( OpA != A_NOP || OpM != M_NOP
			|| RAddrA != R_NOP || RAddrB != R_NOP )
			goto fail;
		combine = false;
		Sig = S_LDI;
	}
	LdMode = L_SEMA;

	doInitOP();

	doInstrExt();

	doALUTarget();
	if (NextToken() != COMMA)
		Fail("Expected ', <number>' after first argument to semaphore instruction, found %s.", Token.c_str());

	InstCtx = IC_ADD|IC_DST;
	ParseExpression();
	if (ExprValue.Type != V_INT || ((uint64_t)ExprValue.iValue & ~(type << 4)) >= 16)
		Fail("Semaphore instructions require a single integer argument less than 16 with the semaphore number.");
	uint32_t value = ExprValue.iValue | (type << 4);

	if (combine && (Immd.uValue & 0x1f) != (value & 0x1f))
		Fail("Combining a semaphore instruction with load immediate requires the low order 5 bits to match the semaphore number and the direction bit.");
	else
		Immd.uValue = value;
}*/

void Parser::assembleREAD(int)
{
	prepareREAD();
	ExprValue.Type = V_NONE;
	doInstrExt();

	ParseExpression();

	applyREADsrc(ExprValue);
}

void Parser::assembleBRANCH(int relative)
{
	prepareBRANCH(!!relative);
	ExprValue.Type = V_NONE;
	doInstrExt();

	doALUTarget();
	if (NextToken() != COMMA)
		Fail("Expected ', <branch target>' after first argument to branch instruction, found %s.", Token.c_str());

	switch (ArgumentCount(At, 3))
	{default: // 2nd target
		InstCtx = IC_MUL|IC_DST;
		doALUTarget();
		if (NextToken() != COMMA)
			Fail("Expected ', <branch target>', found %s.", Token.c_str());
		InstCtx = IC_BOTH|IC_DST;
	 case 2:
		InstCtx ^= IC_DST|IC_SRCA;
		ParseExpression();
		applyBranchSource(ExprValue, PC);
		if (NextToken() != COMMA)
			Fail("Expected ', <branch target>', found %s.", Token.c_str());
		InstCtx ^= IC_SRC;
		break;
	 case 1:
		InstCtx = IC_ADD|IC_SRCA;
	}

	ParseExpression();
	if (applyBranchSource(ExprValue, PC))
	{	// add branch target flag for the branch point
		size_t pos = PC + 4;
		FlagsSize(pos + 1);
		InstFlags[pos] |= IF_BRANCH_TARGET;
	}
	if (NextToken() != END)
		Fail("Expected end of line after branch instruction.");
}

void Parser::assembleSIG(int bits)
{
	applySignal((::Inst::sig)bits);
}

void Parser::ParseInstruction()
{
	while (true)
	{	Flags &= ~IF_CMB_ALLOWED;
		const opEntry<8>* op = binary_search(opcodeMap, Token.c_str());
		if (!op)
			Fail("Invalid opcode or unknown macro: %s", Token.c_str());

		if (Preprocessed)
			fputs(Token.c_str(), Preprocessed);
		(this->*op->Func)(op->Arg);

		switch (NextToken())
		{default:
			Fail("Expected end of line or ';' after instruction. Found '%s'.", Token.c_str());
		 case END:
			return;
		 case SEMI:
			Flags |= IF_CMB_ALLOWED;
			switch (NextToken())
			{default:
				Fail("Expected additional instruction or end of line after ';'. Found '%s'.", Token.c_str());
			 case END:
				return;
			 case WORD:;
			}
		}
	}
}

Parser::label& Parser::defineLabel()
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
		if (!!lp->Definition)
		{	// redefinition
			if (!isdigit(lp->Name[0]))
				Fail("Redefinition of non-local label %s, previously defined at %s (%u).",
					Token.c_str(), fName(lp->Definition.File), lp->Definition.Line);
			// redefinition allowed, but this is always a new label
			lname.first->second = LabelCount;
			goto new_label;
		}
	}
	if (BitOffset & 7)
		Fail("Cannot set a label at a bit boundary. At least byte alignment is required.");
	lp->Value = PC * sizeof(uint64_t) + (BitOffset >> 3);
	lp->Definition = *Context.back();

	if (Preprocessed)
	{	fputs(Token.c_str(), Preprocessed);
		fputs(": ", Preprocessed);
	}
	return *lp;
}

void Parser::addGlobal(const string& name, exprValue value)
{
	auto p = GlobalsByName.emplace(name, value);
	if (!p.second && Pass2)
	{	// Doubly defined
		if (p.first->second == value)
			Msg(INFO, "Label '%s' has already been marked as global.", name.c_str());
		else
			Msg(ERROR, "Another label or value has already been assigned to the global symbol '%s'.", name.c_str());
	}
}

void Parser::parseLabel()
{
	if (!isspace(*At))
	{	bool global = false;
	 rep:
		switch (NextToken())
		{case COLON:
			if (!global)
			{	global = true;
				goto rep;
			}
		 default:
			Fail("Expected label name after ':', found '%s'.", Token.c_str());
		 case WORD:
		 case NUM:
			{	label& l = defineLabel();
				if (global)
					addGlobal(l.Name, exprValue(l.Value, V_LABEL));
			}
		 case END:;
		}
	}
	Flags |= IF_BRANCH_TARGET;
}

void Parser::parseGLOBAL(int)
{
	if (NextToken() != WORD)
		Fail("Expected global symbol name after .global. Found '%s'.", Token.c_str());
	string name = Token;
	//exprValue value;
	switch (NextToken())
	{default:
		Fail("Expected ',' or end of line after symbol name.");
	 case COMMA: // name, value
		ParseExpression();
		if (NextToken() != END)
			Fail("Expected end of line.");
		switch (ExprValue.Type)
		{default:
			Fail("The value of a global symbol definition must be either a label or an integer constant. Found %s.", type2string(ExprValue.Type));
		 case V_INT:
			if (ExprValue.iValue < -0x80000000LL || ExprValue.iValue > 0xffffffffLL)
				Fail("Cannot export 64 bit constant 0x%" PRIx64 "as symbol.", ExprValue.iValue);
		 case V_LABEL:;
		}
		break;
	 case COLON:
		if (NextToken() != WORD)
			Fail("Label name expected. Found '%s'.", Token.c_str());
		name = Token;
		if (NextToken() != END)
			Fail("Expected end of line.");
	 case END: // only label name
		ExprValue = exprValue(labelRef(name).Value, V_LABEL);
	}
	addGlobal(name, ExprValue);
}

void Parser::parseDATA(int bits)
{	if (doPreprocessor())
		return;

	int alignment = 0;
 next:
	ParseExpression();
	if (ExprValue.Type != V_INT && ExprValue.Type != V_FLOAT)
		Fail("Immediate data instructions require integer or floating point constants. Found %s.", type2string(ExprValue.Type));
	if (bits < 0 && ExprValue.Type == V_INT)
		ExprValue.fValue = ExprValue.iValue;
	switch (bits)
	{case -32: // float
		if (fabs(ExprValue.fValue) > FLT_MAX && !std::isinf(ExprValue.fValue))
			Msg(WARNING, "The value %g is outside the domain of single precision floating point.", ExprValue.fValue);
		{	float f = (float)ExprValue.fValue;
			ExprValue.iValue = *(uint32_t*)&f;
		}
	 case -64: // double
	 flt:
		bits = -bits;
		break;
	 case -16: // half precision float
		if (fabs(ExprValue.fValue) > 65504. && !std::isinf(ExprValue.fValue))
			Msg(WARNING, "The value %g is outside the domain of half precision floating point.", ExprValue.fValue);
		ExprValue.iValue = ::Inst::toFloat16(ExprValue.fValue);
		goto flt;
	 case 1: // bit
		if (ExprValue.iValue & ~1)
			Msg(WARNING, "Bit value out of range: 0x%" PRIx64, ExprValue.iValue);
		break;
	 default: // 2 to 32 bit integer
		{	int32_t lower = -1 << (bits - 1);
			int64_t upper = (1ULL << bits) -1;
			if (ExprValue.iValue > upper || ExprValue.iValue < lower)
				Msg(WARNING, "%u bits integer value out of range [%" PRIi32 ", %" PRIi64 "]: 0x%" PRIx64, bits, lower, upper, ExprValue.iValue);
			ExprValue.iValue &= upper;
			break;
		}
	 case 64:;
	}
	// Ensure slot
	if (!BitOffset)
	{	Flags |= IF_BRANCH_TARGET|IF_DATA;
		StoreInstruction(0);
		Flags = IF_NONE;
	}
	// Check alignment
	else if (!alignment && (alignment = BitOffset & (bits-1)) != 0)
		Msg(WARNING, "Unaligned immediate data directive. %i bits missing for correct alignment.", bits - alignment);
	// store value
	uint64_t& target = Instructions[PC];
	target |= ExprValue.iValue << BitOffset;
	if ((BitOffset += bits) >= 64)
	{	++PC;
		// If value crosses instruction boundary => store remaining part
		if ((BitOffset -= 64) != 0)
		{	Flags |= IF_BRANCH_TARGET|IF_DATA;
			StoreInstruction((uint64_t)ExprValue.iValue >> (bits - BitOffset));
			Flags = IF_NONE;
	}	}
	switch (NextToken())
	{default:
		Fail("Syntax error. Expected ',' or end of line.");
	 case COMMA:
		goto next;
	 case END:;
	}
	reset();
}

void Parser::parseALIGN(int bytes)
{	if (doPreprocessor())
		return;

	if (bytes < 0)
	{	ParseExpression();
		if (ExprValue.Type != V_INT)
			Fail("Expected integer constant after .align.");
		if (ExprValue.iValue > 64 || ExprValue.iValue < 0)
			Fail("Alignment value must be in the range [0, 64].");
		bytes = (int)ExprValue.iValue;
		if (bytes & (bytes-1))
			Fail("Alignment value must be a power of 2.");
	}
	int offset = 0;
	switch (NextToken())
	{default:
		Fail("Expected end of line or ,<offset>.");
	 case COMMA:
		{	ParseExpression();
			switch (ExprValue.Type)
			{default:
				Fail("Expected integer constant or label after .align.");
			 case V_LABEL:
			 case V_INT:
				offset = -ExprValue.iValue & 63;
			}
			if (NextToken() != END)
				Fail("Expected end of line after alignment directive.");
		}
	 case END:;
	}

	doALIGN(bytes, offset);
}

bool Parser::doALIGN(int bytes, int offset)
{	if (!bytes)
		return false;

	// bit alignment
	int align = (BitOffset+(offset<<3)) & 63 & ((bytes<<3)-1);
	if (align)
	{	BitOffset += 8*bytes - align;
		if (BitOffset >= 64) // cannot overflow
		{	++PC;
			Flags = IF_NONE;
			BitOffset = 0;
		}
	}

	// Instruction level alignment
	bytes >>= 3;
	offset >>= 3;
	if (!bytes || !((PC+offset) & --bytes))
		return align != 0;
	// BitOffset is necessarily zero at this point.
	do
	{	StoreInstruction(0x100009e7009e7000ULL); // nop
		++PC;
	} while ((PC+offset) & bytes);
	Flags = IF_NONE;
	return true;
}

void Parser::beginLOCAL(int)
{
	if (doPreprocessor())
		return;

	if (NextToken() != END)
		Fail("Expected end of line.");

	Context.emplace_back(new fileContext(CTX_BLOCK, Context.back()->File, Context.back()->Line));
}

void Parser::endLOCAL(int)
{
	if (doPreprocessor())
		return;

	if (Context.back()->Type != CTX_BLOCK)
		Fail(".endloc without .local in the current file.");
	// keep line
	unsigned line = Context.back()->Line;
	Context.pop_back();
	Context.back()->Line = line;
}

void Parser::beginREP(int mode)
{	if (doPreprocessor())
		return;

	auto name = mode ? ".foreach" : ".rep";
	AtMacro = &Macros[name];
	AtMacro->Definition = *Context.back();

	if (NextToken() != WORD)
		Fail("Expected loop variable name after %s.", name);

	AtMacro->Args.push_back(Token);
	if (NextToken() != COMMA)
		Fail(mode ? "Expected ', <parameters>' at .foreach." : "Expected ', <count>' at .rep.");

	{nextpar:
		ParseExpression();
		if (!mode && (ExprValue.Type != V_INT || (uint64_t)ExprValue.iValue > 0x1000000))
			Fail("Second argument to .rep must be a non-negative integral number. Found %s", ExprValue.toString().c_str());
		AtMacro->Args.push_back(ExprValue.toString());

		switch (NextToken())
		{default:
			Fail("Expected ',' or end of line.");
		 case COMMA:
			if (!mode)
				Fail("Expected end of line.");
			goto nextpar;
		 case END:;
		}
	}
}

void Parser::endREP(int mode)
{
	auto name = mode ? ".foreach" : ".rep";
	auto iter = Macros.find(name);
	if (AtMacro != &iter->second)
	{	if (doPreprocessor())
			return;
		Fail("%s without %s", Token.c_str(), name);
	}
	const macro m = *AtMacro;
	AtMacro = NULL;
	Macros.erase(iter);

	if (NextToken() != END)
		Fail("Expected end of line.");

	if (m.Args.size() < 2)
		return; // no loop count => 0

	// Setup invocation context
	saveContext ctx(*this, new fileContext(CTX_MACRO, m.Definition.File, m.Definition.Line));

	// loop
	size_t count;
	if (mode)
		count = m.Args.size()-1;
	else
		sscanf(m.Args[1].c_str(), "%zi", &count);
	auto& current = *Context.back();
	auto& value = current.Consts.emplace(m.Args.front(), constDef(exprValue((int64_t)0), current)).first->second.Value;
	for (size_t i = 0; i < count; ++i)
	{	// set argument
		if (mode)
		{	strncpy(Line, m.Args[i+1].c_str(), sizeof(Line));
			At = Line;
			ParseExpression();
			value = ExprValue;
		} else
			value.iValue = i;
		// Invoke body
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
		Fail("Cannot nest .back directives.");
	ParseExpression();
	if (ExprValue.Type != V_INT)
		Fail("Expected integer constant after .back.");
	if (ExprValue.iValue < 0)
		Fail(".back expects positive integer argument.");
	if (ExprValue.iValue > 10)
		Fail("Cannot move instructions more than 10 slots back.");
	if ((unsigned)ExprValue.iValue > PC)
		Fail("Cannot move instructions back before the start of the code.");
	if (NextToken() != END)
		Fail("Expected end of line, found '%s'.", Token.c_str());
	Back = (unsigned)ExprValue.iValue;
	size_t pos = PC -= Back;
	// Load last instruction before .back to provide combine support
	if (pos)
	{	decode(Instructions[pos-1]);
		Flags = InstFlags[pos-1];
	}
	if (Pass2)
		while (++pos < PC + Back)
			if (InstFlags[pos] & IF_BRANCH_TARGET)
				Msg(WARNING, ".back crosses branch target at address 0x%zx. Code might not work.", pos*8);
}

void Parser::endBACK(int)
{
	if (doPreprocessor())
		return;

	/* avoids .back 0  if (!Back)
		Fail(".endb without .back.");*/
	if (NextToken() != END)
		Fail("Expected end of line, found '%s'.", Token.c_str());
	PC += Back;
	Back = 0;
	// Restore last instruction to provide combine support
	if (PC)
	{	decode(Instructions[PC-1]);
		Flags = InstFlags[PC-1];
	}
}

void Parser::parseCLONE(int)
{
	if (doPreprocessor())
		return;

	ParseExpression();
	if (ExprValue.Type != V_LABEL)
		Fail("The first argument to .clone must by a label. Found %s.", type2string(ExprValue.Type));
	unsigned param1 = (unsigned)ExprValue.iValue >> 3; // offset in instructions rather than bytes
	if (NextToken() != COMMA)
		Fail("Expected ', <count>' at .clone.");
	ParseExpression();
	if (ExprValue.Type != V_INT || (uint64_t)ExprValue.iValue > 3)
		Fail("Expected integer constant in the range [0,3].");
	// Fast exit without any further checks.
	if (ExprValue.iValue == 0)
		return;

	unsigned param2 = (unsigned)ExprValue.iValue;
	FlagsSize(PC + param2);
	param2 += param1; // end offset rather than count
	if (Pass2 && param2 >= Instructions.size())
		Fail("Cannot clone behind the end of the code.");

	if (doALIGN(8, 0))
		Msg(WARNING, "Used padding to enforce 64 bit alignment of GPU instruction.");

	auto src = param1;
	do
	{	InstFlags[PC] |= InstFlags[src] & ~IF_BRANCH_TARGET;
		if ((Instructions[src] & 0xF000000000000000ULL) == 0xF000000000000000ULL)
			Msg(WARNING, "You should not clone branch instructions. (#%u)", src - param1);
		StoreInstruction(Instructions[src]);
		++PC;
		++src;
	} while (src < param2);
	// Restore last instruction to provide combine support
	decode(Instructions[PC-1]);
	Flags = InstFlags[PC-1];
}

void Parser::parseSET(int flags)
{
	if (doPreprocessor())
		return;

	if (NextToken() != WORD)
		Fail("Directive .set requires identifier.");
	string name = Token;
	switch (NextToken())
	{default:
		Fail("Directive .set requires ', <value>' or '(<arguments>) <value>'. Found %s.", Token.c_str());
	 case BRACE1:
		{	function func(*Context.back());
		 next:
			if (NextToken() != WORD)
				Fail("Function argument name expected. Found '%s'.", Token.c_str());
			func.Args.push_back(Token);
			switch (NextToken())
			{default:
				Fail("Expected ',' or ')' after function argument.");
			 case NUM:
				Fail("Function arguments must not start with a digit.");
			 case COMMA:
				goto next;
			 case END:
				Fail("Unexpected end of function argument definition");
			 case BRACE2:
				break;
			}

			// Anything after ')' is function body and evaluated delayed
			At += strspn(At, " \t\r\n,");
			func.DefLine = Line;
			func.Start = At;

			const auto& ret = Functions.emplace(name, func);
			if (!ret.second)
			{	Msg(INFO, "Redefinition of function %s.\n"
				      "Previous definition at %s (%u).",
				  Token.c_str(), fName(ret.first->second.Definition.File), ret.first->second.Definition.Line);
				ret.first->second = func;
			}
			break;
		}
	 case COMMA:
		{	InstCtx = IC_XP;
			ParseExpression();
			if (NextToken() != END)
				Fail("Syntax error: unexpected %s.", Token.c_str());

			auto& consts = (flags & C_LOCAL ? Context.back() : Context.front())->Consts;
			auto r = consts.emplace(name, constDef(ExprValue, *Context.back()));
			if (!r.second)
			{	if (flags & C_CONST)
					// redefinition not allowed
					Fail("Identifier %s has already been defined at %s (%u).",
						name.c_str(), fName(r.first->second.Definition.File), r.first->second.Definition.Line);
				r.first->second.Value = ExprValue;
			}
		}
	}
}

void Parser::parseUNSET(int flags)
{
	if (doPreprocessor())
		return;

	if (NextToken() != WORD)
		Fail("Directive .unset requires identifier.");
	string Name = Token;
	if (NextToken() != END)
		Fail("Syntax error: unexpected %s.", Token.c_str());

	auto& consts = (flags & C_LOCAL ? Context.back() : Context.front())->Consts;
	auto r = consts.find(Name);
	if (r == consts.end())
		return Msg(WARNING, "Cannot unset %s because it has not yet been definied in the required context.", Name.c_str());
	consts.erase(r);
}

bool Parser::doCondition()
{
	ParseExpression();
	if (ExprValue.Type != V_INT)
		Fail("Conditional expression must be a integer constant, found '%s'.", ExprValue.toString().c_str());
	if (NextToken() != END)
		Fail("Expected end of line, found '%s'.", Token.c_str());
	return ExprValue.iValue != 0;
}

void Parser::parseIF(int)
{
	if (doPreprocessor(PP_MACRO))
		return;

	AtIf.emplace_back(*Context.back(), isDisabled() ? 4 : doCondition());
}

void Parser::parseIFSET(int)
{
	if (doPreprocessor(PP_MACRO))
		return;

	if (NextToken() != WORD)
		Fail("Expected identifier after .ifset, found '%s'.", Token.c_str());

	int state = 4;
	if (!isDisabled())
	{	state = 0;
		for (auto i = Context.end(); i != Context.begin(); )
		{	auto c = (*--i)->Consts.find(Token);
			if (c != (*i)->Consts.end())
			{	state = 1;
				break;
			}
		}
	}

	if (NextToken() != END)
		Fail("Expected end of line, found '%s'.", Token.c_str());

	AtIf.emplace_back(*Context.back(), state);
}

void Parser::parseELSEIF(int)
{
	if (doPreprocessor(PP_MACRO))
		return;

	if (!AtIf.size())
		Fail(".elseif without .if");

	if (AtIf.back().State == IS_FALSE)
		AtIf.back().State = doCondition();
	else
		AtIf.back().State |= IS_ELSE;
}

void Parser::parseELSE(int)
{
	if (doPreprocessor(PP_MACRO))
		return;

	if (!AtIf.size())
		Fail(".else without .if");
	if (NextToken() != END)
		Msg(ERROR, "Expected end of line. .else has no arguments.");

	// Hack for .else
	// State transition from IS_FALSE to IF_TRUE and from IF_TRUE to IS_FALSE|IS_ELSE
	++AtIf.back().State;
}

void Parser::parseENDIF(int)
{
	if (doPreprocessor(PP_MACRO))
		return;

	if (NextToken() != END)
		Msg(ERROR, "Expected end of line. .endif has no arguments.");
	if (!AtIf.size())
		Fail(".endif without .if.");
	if (AtIf.back().File != Context.back()->File)
		Fail(".endif without .if in the same file.");

	AtIf.pop_back();
}

void Parser::parseASSERT(int)
{
	if (doPreprocessor())
		return;

	if (!doCondition())
		Msg(ERROR, "Assertion failed.");
}

void Parser::beginMACRO(int flags)
{
	if (doPreprocessor(PP_IF))
		return;

	if (AtMacro)
		Fail("Cannot nest macro definitions.\n"
		     "  In definition of macro starting at %s (%u).",
		     fName(AtMacro->Definition.File), AtMacro->Definition.Line);
	if (NextToken() != WORD)
		Fail("Expected macro name.");
	AtMacro = &(flags & M_FUNC ? MacroFuncs : Macros)[Token];
	if (!!AtMacro->Definition)
	{	Msg(INFO, "Redefinition of macro %s.\n"
		          "  Previous definition at %s (%u).",
		  Token.c_str(), fName(AtMacro->Definition.File), AtMacro->Definition.Line);
		// redefine
		AtMacro->Args.clear();
		AtMacro->Content.clear();
	}
	AtMacro->Definition = *Context.back();
	AtMacro->Flags = (macroFlags)flags;

	int brace = 0;
	while (true)
		switch (NextToken())
		{case BRACE1:
			if (!AtMacro->Args.size())
			{	brace = 1;
				goto comma;
			}
		 default:
			Fail("Expected ',' before (next) macro argument.");
		 case NUM:
			Fail("Macro arguments must not be numbers.");
		 case COMMA:
			if (brace == 2)
				Fail("Expected end of line after closing brace.");
		 comma:
			// Macro argument
			if (NextToken() != WORD)
				Fail("Macro argument name expected. Found '%s'.", Token.c_str());
			AtMacro->Args.push_back(Token);
			break;
		 case BRACE2:
			if (brace != 1)
				Fail("Unexpected closing brace.");
			brace = 2;
			break;
		 case END:
			if (brace == 1)
				Fail("Closing brace is missing");
			return;
		}
}

void Parser::endMACRO(int flags)
{
	if (doPreprocessor(PP_IF))
		return;

	if (!AtMacro)
		Fail(".%s outside a macro definition.", Token.c_str());
	if (AtMacro->Flags != flags)
		Fail("Cannot close this macro with .%s. Expected .end%c.", Token.c_str(), flags & M_FUNC ? 'f' : 'm');
	AtMacro = NULL;
	if (NextToken() != END)
		Msg(ERROR, "Expected end of line.");
}

void Parser::doMACRO(macros_t::const_iterator m)
{
	InstCtx = IC_XP;
	// Fetch macro arguments
	const auto& argnames = m->second.Args;
	vector<exprValue> args;
	if (argnames.size())
	{	args.reserve(argnames.size());
		while (true)
		{	ParseExpression();
			args.emplace_back(ExprValue);
			switch (NextToken())
			{default:
				Fail("internal error");
			 case COMMA:
				if (args.size() == argnames.size())
					Fail("Too much arguments for macro %s.", m->first.c_str());
				continue;
			 case END:
				if (args.size() != argnames.size())
					Fail("Too few arguments for macro %s.", m->first.c_str());
			}
			break;
		}
	} else if (NextToken() != END)
		Fail("The macro %s does not take arguments.", m->first.c_str());

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

void Parser::doFUNCMACRO(macros_t::const_iterator m)
{
	if (NextToken() != BRACE1)
		Fail("Expected '(' after function name.");

	auto oldctx = InstCtx;
	InstCtx = IC_XP;
	// Fetch macro arguments
	const auto& argnames = m->second.Args;
	vector<exprValue> args;
	args.reserve(argnames.size());
	if (argnames.size() == 0)
	{	// no arguments
		if (NextToken() != BRACE2)
			Fail("Expected ')' because function %s has no arguments.", m->first.c_str());
	} else
	{next:
		ParseExpression();
		args.push_back(ExprValue);
		switch (NextToken())
		{case BRACE2:
			// End of argument list. Are we complete?
			if (args.size() != argnames.size())
				Fail("Too few arguments for function %s. Expected %zu, found %zu.", m->first.c_str(), argnames.size(), args.size());
			break;
		 default:
			Fail("Unexpected '%s' in argument list of function %s.", Token.c_str(), m->first.c_str());
		 case COMMA:
			// next argument
			if (args.size() == argnames.size())
				Fail("Too much arguments for function %s. Expected %zu.", m->first.c_str(), argnames.size());
			goto next;
		}
	}
	InstCtx = oldctx; // restore context

	// Setup invocation context
	saveLineContext ctx(*this, new fileContext(CTX_MACRO, m->second.Definition.File, m->second.Definition.Line));

	// setup args inside new context to avoid interaction with argument values that are also functions.
	auto& current = *Context.back();
	current.Consts.reserve(current.Consts.size() + argnames.size());
	size_t n = 0;
	for (auto arg : argnames)
		current.Consts.emplace(arg, constDef(args[n++], current));

	// Invoke macro
	exprValue ret;
	for (const string& line : m->second.Content)
	{	++Context.back()->Line;
		strncpy(Line, line.c_str(), sizeof(Line));
		At = Line;
		switch (NextToken())
		{case DOT:
			// directives
			ParseDirective();
		 case END:
			break;

		 case COLON:
		 label:
			Msg(ERROR, "Label definition not allowed in functional macro %s.", m->first.c_str());
			break;

		 default:
			if (doPreprocessor())
				break;
			// read-ahead to see if the next token is a colon in which case
			// this is a label.
			if (*At == ':')
				goto label;
			if (ret.Type != V_NONE)
			{	Msg(ERROR, "Only one expression allowed per functional macro.");
				break;
			}
			At -= Token.size();
			ParseExpression();
			ret = ExprValue;
		}
	}
	if (ret.Type == V_NONE)
		Fail("Failed to return a value in functional macro %s.", m->first.c_str());
	ExprValue = ret;
}

void Parser::doFUNC(funcs_t::const_iterator f)
{
	if (NextToken() != BRACE1)
		Fail("Expected '(' after function name.");

	auto oldctx = InstCtx;
	InstCtx = IC_XP;
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
		ParseExpression();
		args.push_back(ExprValue);
		switch (NextToken())
		{case BRACE2:
			// End of argument list. Are we complete?
			if (args.size() != argnames.size())
				Fail("Too few arguments for function %s. Expected %zu, found %zu.", f->first.c_str(), argnames.size(), args.size());
			break;
		 default:
			Fail("Unexpected '%s' in argument list of function %s.", Token.c_str(), f->first.c_str());
		 case COMMA:
			// next argument
			if (args.size() == argnames.size())
				Fail("Too much arguments for function %s. Expected %zu.", f->first.c_str(), argnames.size());
			goto next;
		}
	}
	InstCtx = oldctx;

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

	ParseExpression();
	if (NextToken() != END)
		Fail("Function %s evaluated to an incomplete expression.", f->first.c_str());
}

void Parser::doSEGMENT(int flags)
{
	if (doPreprocessor())
		return;

	if (NextToken() != END)
		Msg(ERROR, "End of line expected.");

	// Search segment entry at PC
	auto ptr = upper_bound(Segments.begin()+1, Segments.end(), PC,
		[](decltype(PC) pc, const segment& seg) { return pc < seg.Start; } );
	auto cur = ptr - 1;

	// Segment attributes happen to match => no action
	if (cur->Flags == flags)
		return;
	// otherwise ensure distinct entry at PC
	if (cur->Start != PC)
		Segments.insert(ptr, segment(PC, (uint8_t)flags));
	else
		cur->Flags = flags;
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
	if ((*At != '"' || At[len-1] != '"') && (*At != '<' || At[len-1] != '>'))
		Fail("Syntax error. Expected \"file-name\" or <file-name> after .include, found '%s'.", At);
	Token.assign(At+1, len-2);

	// find file
	string file;
	if (*At != '<' || (file = FindIncludePath(Token)).length() == 0) // check include paths first
	{	file = relpath(SourceFiles[Context.back()->File].Name, Token);
		struct stat buffer;
		if (stat(file.c_str(), &buffer) != 0)
			Fail("Cannot locate included file '%s'.", Token.c_str());
	}

	if (Pass2)
	{	const auto& p1file = SourceFiles[FilesCount];
		if (p1file.Name != file)
			Fail("Inconsistent include invocation during pass 2.");
		++FilesCount;
	} else
	{	SourceFiles.emplace_back(file, *Context.back());
		FilesCount = SourceFiles.size();
	}
	saveContext ctx(*this, new fileContext(CTX_INCLUDE, FilesCount-1, 0));
	ParseFile();
}

void Parser::ParseDirective()
{
	if (NextToken() != WORD)
		Fail("Expected assembler directive after '.'. Found '%s'.", Token.c_str());

	const opEntry<8>* op = binary_search(directiveMap, Token.c_str());
	if (!op)
		Fail("Invalid assembler directive: %s", Token.c_str());

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
	bool trycombine = false;
	bool isinst = false;
	size_t pos = PC;
	FlagsSize(pos + 1);

 next:
	switch (NextToken())
	{case DOT:
		if (isinst)
			goto def;
		// directives
		InstCtx = IC_NONE;
		ParseDirective();
		return;

	 case END:
		*Line = 0;
		doPreprocessor(PP_MACRO);
		return;

	 case COLON:
		if (doPreprocessor())
			return;
		if (isinst)
			goto def;

		parseLabel();
		goto next;

	 case SEMI:
		if (doPreprocessor())
			return;
		if (pos && (InstFlags[pos-1] & IF_CMB_ALLOWED) && (InstFlags[pos] & IF_BRANCH_TARGET) == 0)
			trycombine = true;
		isinst = true;
		goto next;

	 def:
	 default:
		if (!AtMacro || (AtMacro->Flags & M_FUNC) == 0)
			Fail("Syntax error. Unexpected '%s'.", Token.c_str());
	 case WORD:
		if (doPreprocessor())
			return;

		// read-ahead to see if the next token is a colon in which case
		// this is a label.
		if (*At == ':')
		{	defineLabel();
			InstFlags[pos] |= IF_BRANCH_TARGET;
			++At;
			goto next;
		}

		// Try macro
		macros_t::const_iterator m = Macros.find(Token);
		if (m != Macros.end())
		{	doMACRO(m);
			return;
		}

		if (doALIGN(8, 0))
			Msg(WARNING, "Used padding to enforce 64 bit alignment of GPU instruction.");

		if (trycombine)
		{	char* atbak = At;
			bool succbak = Success;
			string tokenbak = Token;
			try
			{	// Try to parse into existing instruction.
				ParseInstruction();
				// Do not combine TMU instructions
				if (isTMUconflict())
					throw string();
				// Combine succeeded
				Instructions[pos-1] = encode();
				InstFlags[pos-1] = Flags;
				return;
			} catch (const string& msg)
			{	// Combine failed => try new instruction.
				At = atbak;
				Success = succbak;
				Token = tokenbak;
			}
		}
		// new instruction
		reset();

		ParseInstruction();
		StoreInstruction(encode());
		++PC;
		Flags = IF_NONE;
		return;
	}
}

string Parser::FindIncludePath(const string& file)
{	struct stat buffer;
	for (string path : IncludePaths)
	{	string test = path + file;
		if (stat(test.c_str(), &buffer) == 0 && S_ISREG(buffer.st_mode))
			return test;
	}
	return string();
}

void Parser::ParseFile()
{
	FILE* f = fopen(fName(Context.back()->File), "r");
	if (!f)
		Fail("Failed to open file %s.", fName(Context.back()->File));
	auto ifs = AtIf.size();
	try
	{	while (fgets(Line, sizeof(Line), f))
		{	++Context.back()->Line;
			try
			{	ParseLine();
			} catch (const string& msg)
			{	// recover from errors
				CaughtMsg(msg.c_str());
			}

			if (AtMacro && AtMacro->Definition.Line == 0)
				Fail("!!!");
		}
		fclose(f);
	} catch (...)
	{	fclose(f);
		throw;
	}
	if (ifs < AtIf.size())
		Fail("Unterminated .if at line %u.", AtIf[ifs].Line);
}
void Parser::ParseFile(const string& file)
{	if (Pass2)
		throw string("Cannot add another file after pass 2 has been entered.");
	struct stat buffer;
	string target = file;
	if ( !(stat(file.c_str(), &buffer) == 0 && S_ISREG(buffer.st_mode))
		&& !(SearchIncludePaths && (target = FindIncludePath(file)).length() != 0) )
		throw stringf("\"%s\" not found or no regular file.", file.c_str());

	SourceFiles.emplace_back(target);
	FilesCount = SourceFiles.size();
	saveContext ctx(*this, new fileContext(CTX_FILE, FilesCount-1, 0));
	try
	{	ParseFile();
	} catch (const string& msg)
	{	// recover from errors
		Success = false;
		CaughtMsg(msg.c_str());
	}
}

void Parser::ResetPass()
{	AtMacro = NULL;
	AtIf.clear();
	Context.clear();
	Context.emplace_back(new fileContext(CTX_ROOT, 0, 0));
	FilesCount = 0;
	Functions.clear();
	Macros.clear();
	LabelsByName.clear();
	GlobalsByName.clear();
	LabelCount = 0;
	InstFlags.clear();
	PC = 0;
	reset();
	BitOffset = 0;
	Segments.resize(1);
	Segments[0].Start = 0;
	Segments[0].Flags = SF_None;
}

void Parser::EnsurePass2()
{
	if (Pass2 || (!Success && OperationMode != IGNOREERRORS))
		return;

	// enter pass 2
	Pass2 = true;
	ResetPass();

	// Check all labels
	for (auto& label : Labels)
	{	if (!label.Definition)
			Msg(ERROR, "Label '%s' is undefined. Referenced from %s (%u).\n",
				label.Name.c_str(), fName(label.Reference.File), label.Reference.Line);
		if (!label.Reference)
			Msg(INFO, "Label '%s' defined at %s (%u) is not used.\n",
				label.Name.c_str(), fName(label.Definition.File), label.Definition.Line);
		// prepare for next pass
		label.Definition.Line = 0;
	}

	//for (auto& file : SourceFiles)
	while (FilesCount < SourceFiles.size())
	{	const auto& file = SourceFiles[FilesCount];
		if (!!file.Parent)
			Fail("Inconsistent include files during pass 2.");
		saveContext ctx(*this, new fileContext(CTX_FILE, FilesCount, 0));
		++FilesCount;
		ParseFile();
	}

	// Optimize instructions identify code segments automatically
	unsigned pc = 0;
	auto sp = Segments.begin();
	auto np = sp + 1;
	bool autocode = false;
	for (auto& inst : Instructions)
	{	// next segment?
		if (np != Segments.end() && np->Start <= pc)
		{	sp = np++;
			autocode = false;
		}
		if ((InstFlags[pc++] & IF_DATA) == 0)
		{	// instruction
			decode(inst);
			optimize();
			inst = encode();
			// auto code?
			if (sp->Flags == SF_None)
			{	if (sp->Start != pc)
				{	// not current pc => insert new segment
					sp = Segments.emplace(np, pc, SF_Code);
					np = sp + 1;
				} else
					// exact match => just change the flags
					sp->Flags = SF_Code;
				autocode = true;
			}
		} else if (autocode)
		{	// autocode implies that sp->Start != pc && last Flags = SF_None
			// => insert an entry at pc
			sp = Segments.insert(np, segment(pc, SF_None));
			np = sp + 1;
			autocode = false;
		}
	}
}

void Parser::Reset()
{ ResetPass();
	Labels.clear();
	Pass2 = false;
	SourceFiles.clear();
}

Parser::Parser()
{	Reset();
}
