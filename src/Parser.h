/*
 * Parser.h
 *
 *  Created on: 30.08.2014
 *      Author: mueller
 */

#ifndef PARSER_H_
#define PARSER_H_

#include "Eval.h"
#include "Inst.h"
#include "utils.h"

#include <inttypes.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <string.h>
#include <stdarg.h>

using namespace std;

/// Find the first occurrence of key
template <typename T, size_t N>
inline T* binary_search(T (&arr)[N], const char* key)
{	size_t l = 0;
	size_t r = N;
	int nohit = -1;
	while (l < r)
	{	size_t m = (l+r) >> 1;
		int cmp = strcmp(key, (const char*)(arr + m));
		if (cmp > 0)
			l = m + 1;
		else
		{	r = m;
			nohit &= cmp;
		}
	}
	return nohit ? NULL : arr + r;
}

class Parser
{public:
	enum severity
	{	ERROR
	,	WARNING
	,	INFO
	};
 public:
	bool Success = true;
	bool Extensions = false;
	FILE* Preprocessed = NULL;
	severity Verbose = WARNING;
 private:
	enum token_t : char
	{	END    =  0 ///< End of line
	,	WORD   = 'A'///< Identifier or opcode
	,	NUM    = 'N'///< numeric constant
	,	OP     = '@'///< operator
	,	BRACE1 = '('
	,	BRACE2 = ')'
	,	SQBRC1 = '['
	,	SQBRC2 = ']'
	,	DOT    = '.'
	,	COMMA  = ','
	,	SEMI   = ';'
	,	COLON  = ':'
	};
	static const struct opInfo
	{	char        Name[4];
		Eval::mathOp Op;
	} operatorMap[];
	static const struct regEntry
	{	char        Name[16];
		reg_t       Value;
	}             regMap[];
	class opAddMul
	{	int8_t      Op;      ///< ADD or MUL ALU opcode, bit 7 set: MUL ALU, bit 6 set: undocumented opcode
	 public:
		constexpr   opAddMul(Inst::opadd op) : Op(op) {}
		constexpr   opAddMul(Inst::opmul op) : Op(op|0x80) {}
		constexpr   opAddMul(int op)         : Op(op|0x40) {}
		bool        isMul() const { return Op < 0; }
		bool        isExt() const { return (Op & 0x40) != 0; }
		Inst::opadd asAdd() const { return (Inst::opadd)(Op & 0x1f); }
		Inst::opmul asMul() const { return (Inst::opmul)(Op & 0x7); }
	};
	static const struct smiEntry
	{	uint32_t    Value;
		uint8_t     SImmd;   ///< small immediate to achieve this result
		opAddMul    OpCode;  ///< ALU opcode to achieve this result
	}             smiMap[];
	enum InstContext : uint8_t
	{	IC_NONE          = 0
	,	IC_MUL           = 1
	,	IC_SRC           = 2
	,	IC_MULSRC        = 3
	,	IC_DST           = 4
	,	IC_MULDST        = 5
	,	IC_SRCB          = 10
	,	IC_MULSRCB       = 11
	};

	enum contextType
	{	CTX_ROOT
	,	CTX_INCLUDE
	,	CTX_MACRO
	,	CTX_FUNCTION
	, CTX_CURRENT
	};
	enum preprocType
	{	PP_MACRO = 1
	,	PP_IF    = 2
	,	PP_ALL   = 3
	};
	template <size_t L>
	struct opEntry
	{	char Name[L];
		void (Parser::*Func)(int);
		int Arg;
	};
	static const opEntry<8> opcodeMap[];
	enum opExtFlags
	{	E_SRC   = 0x01
	,	E_DST   = 0x02
	,	E_OP    = 0x04
	,	E_SRCOP = 0x05
	,	E_DSTOP = 0x06
	};
	struct opExtEntry
	{	char           Name[16];
		void (Parser::*Func)(int,InstContext);
		int            Arg;
		opExtFlags     Flags;
	};
	static const opExtEntry extMap[];
	static const opEntry<8> directiveMap[];

	struct location
	{	string         File;
		unsigned       Line;
		location()     : Line(0) {}
		operator void*() const { return (void*)Line; }
		bool operator !() const { return !Line; }
		string         toString() const;
	};
	struct label
	{	const string   Name;
		unsigned       Value;
		location       Definition;
		location       Reference;
		label(const string& name) : Name(name), Value(0) {}
	};
	typedef vector<label> labels_t;
	typedef unordered_map<string,unsigned> lnames_t;
	enum defFlags : unsigned char
	{	C_NONE  = 0
	,	C_LOCAL = 1
	,	C_CONST = 2
	};
	struct constDef
	{	exprValue      Value;
		location       Definition;
		constDef(const exprValue& value, const location& loc) : Value(value), Definition(loc) {}
	};
	typedef unordered_map<string,constDef> consts_t;
	struct function
	{	location       Definition;
		vector<string> Args;
		string         DefLine;
		char*          Start;
		function(const location& definition) : Definition(definition), Start(NULL) {}
	};
	typedef unordered_map<string,function> funcs_t;
	enum macroFlags : unsigned char
	{	M_NONE = 0
	,	M_FUNC = 1
	};
	struct macro
	{	location       Definition;
		macroFlags     Flags;
		vector<string> Args;
		vector<string> Content;
	};
	typedef unordered_map<string,macro> macros_t;
	struct ifContext
	{	unsigned       Line;
		unsigned       State;       ///< 0 = .if false, 1 = .if true, 2 = .else, 4 = inherited .false
		ifContext(unsigned line, unsigned state) : Line(line), State(state) {}
	};
	typedef vector<ifContext> ifs_t;
	struct fileContext : public location
	{	const contextType Type;
		consts_t       Consts;      ///< Constants (.set)
		fileContext(contextType type, const string& file, unsigned line) : Type(type) { File = file; Line = line; }
	};
	typedef vector<unique_ptr<fileContext>> contexts_t;
	class saveContext
	{protected:
		Parser&        Parent;
	 public:
		saveContext(Parser& parent, fileContext* ctx);
		~saveContext();
	};
	class saveLineContext : public saveContext
	{	const string   LineBak;
		char* const    AtBak;
	 public:
		saveLineContext(Parser& parent, fileContext* ctx);
		~saveLineContext();
	};
	enum InstFlags : uint8_t
	{	IF_NONE          = 0
	,	IF_HAVE_NOP      = 1        ///< at least one NOP in the current instruction so far
	,	IF_CMB_ALLOWED   = 2        ///< Instruction of the following line could be merged
	,	IF_BRANCH_TARGET = 4        ///< This instruction is a branch target
	};

	template <typename T, T def>
	class vector_safe : public vector<T>
	{public:
		typename vector<T>::reference operator[](typename vector<T>::size_type n)
		{	if (n >= vector<T>::size())
				vector<T>::resize(n+1, def);
			return vector<T>::operator[](n);
		}
	};

	// parser working set
	bool             Pass2 = false;
	vector<string>   Filenames;

	char             Line[1024];  ///< Buffer for line input. Well, static size...
	char*            At = NULL;   ///< Current location within Line
	string           Token;       ///< Current token
	Inst             Instruct;    ///< Current instruction
	unsigned         PC;          ///< Current program counter
	// context
	macro*           AtMacro = NULL;///< Currently at a macro definition
	unsigned         Back = 0;    ///< Insert # instructions in the past
	ifs_t            AtIf;        ///< List of (nested) if statements.
	contexts_t       Context;     ///< Include and macro call stack
	// definitions
	labels_t         Labels;      ///< Label values
	unsigned         LabelCount = 0;///< Next free label index
	lnames_t         LabelsByName;///< Label names
	funcs_t          Functions;   ///< Single line function definitions
	macros_t         MacroFuncs;  ///< Multi line function definitions
	macros_t         Macros;      ///< Macros
	// instruction
	vector_safe<uint64_t,0> Instructions;
	vector_safe<uint8_t,IF_NONE> InstFlags;
 private:
	string           enrichMsg(string msg);
	void             Fail(const char* fmt, ...) PRINTFATTR(2) NORETURNATTR;
	void             Msg(severity level, const char* fmt, ...) PRINTFATTR(3);

	/// Ensure minimum size of InstFlags array.
	void             FlagsSize(size_t min);
	uint8_t&         Flags() { return InstFlags[PC]; }

	void             StoreInstruction(uint64_t value);

	token_t          NextToken();
	/// Work around for gcc on 32 bit Linux that can't read "0x80000000" with sscanf anymore.
	/// @return Number of characters parsed.
	static size_t    parseUInt(const char* src, uint32_t& dst);
	exprValue        parseElemInt();
	exprValue        ParseExpression();

	static uint8_t   getSmallImmediate(uint32_t i);
	static const smiEntry* getSmallImmediateALU(uint32_t i);
	Inst::mux        muxReg(reg_t reg);
	void             doSMI(uint8_t si);

	// OP code extensions
	void             addIf(int cond, InstContext ctx);
	void             addUnpack(int mode, InstContext ctx);
	void             addPack(int mode, InstContext ctx);
	void             addSetF(int, InstContext ctx);
	void             addCond(int cond, InstContext ctx);
	void             addRot(int, InstContext ctx);
	void             doInstrExt(InstContext ctx);

	void             doALUTarget(exprValue param, bool mul);
	Inst::mux        doALUExpr(InstContext ctx);
	void             doBRASource(exprValue param);

	// OP codes
	void             assembleADD(int op);
	void             assembleMUL(int op);
	void             assembleMOV(int mode);
	void             assembleBRANCH(int relative);
	void             assembleSEMA(int type);
	void             assembleSIG(int bits);

	void             ParseInstruction();

	void             defineLabel();
	void             parseLabel();

	void             parseDATA(int type);
	void             beginREP(int);
	void             endREP(int);
	void             beginBACK(int);
	void             endBACK(int);
	void             parseCLONE(int);
	void             parseSET(int flags);
	void             parseUNSET(int flags);
	bool             doCondition();
	void             parseIF(int);
	void             parseELSE(int);
	void             parseELSEIF(int);
	void             parseENDIF(int);
	bool             isDisabled() { return AtIf.size() != 0 && AtIf.back().State != 1; }
	void             parseASSERT(int);
	void             beginMACRO(int flags);
	void             endMACRO(int flags);
	void             doMACRO(macros_t::const_iterator m);
	exprValue        doFUNCMACRO(macros_t::const_iterator m);
	exprValue        doFUNC(funcs_t::const_iterator f);
	void             doINCLUDE(int);
	bool             doPreprocessor(preprocType type = PP_ALL);
	void             ParseDirective();

	void             ParseLine();
	void             ParseFile();

	void             ResetPass();
	void             EnsurePass2();
 public:
	                 Parser();
  void             Reset();
	void             ParseFile(const string& file);
	const vector<uint64_t>& GetInstructions();
};

#endif // PARSER_H_
