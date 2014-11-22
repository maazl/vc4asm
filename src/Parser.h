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
#include <inttypes.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <string.h>
#include <stdarg.h>

using namespace std;

template <typename T, size_t N>
inline T* binary_search(T (&arr)[N], const char* key)
{	return (T*)bsearch(key, &arr, N, sizeof(T), (int (*)(const void*, const void*))&strcmp);
}

class Parser
{public:
	bool Success = true;
	FILE* Preprocessed = NULL;
 private:
	enum token_t : char
	{	END    =  0 ///< End of line
	,	WORD   = 'A'///< Identifier or opcode
	,	NUM    = 'N'///< numeric constant
	,	OP     = '@'///< operator
	,	BRACE1 = '('
	,	BRACE2 = ')'
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
	static const reg_t regNOP;
	/// Bit vector with register numbers where register file A and B are interchangeable,
	/// i.e. Instr.WS is not fixed.
	static const uint64_t regAB = 0xfff9f9df00000000ULL;
	class opAddMul
	{	int8_t      Op;      ///< ADD or MUL ALU opcode, bit 7 set: MUL ALU
	 public:
		constexpr   opAddMul(Inst::opadd op) : Op(op) {}
		constexpr   opAddMul(Inst::opmul op) : Op(op|0x80) {}
		bool        isMul() const { return Op < 0; }
		Inst::opadd asAdd() const { return (Inst::opadd)Op; }
		Inst::opmul asMul() const { return (Inst::opmul)(Op ^ 0x80); }
	};
	static const struct smiEntry
	{	uint32_t    Value;
		uint8_t     SImmd;   ///< small immediate to achieve this result
		opAddMul    OpCode;  ///< ALU opcode to achieve this result
	}             smiMap[];

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
	/*template <typename... P>
	class dispatchHelper
	{	void (Parser::*const Func(P));
		P Args;
	 public:
		dispatchHelper(void (Parser::*func)(P), P args) : Func(func), Args(args) {}
		void operator()(Parser& that) { that.*Func(Args); }
	};*/
	template <size_t L>
	struct opEntry
	{	char Name[L];
		void (Parser::*Func)(int);
		int Arg;
	};
	static const opEntry<8> opcodeMap[];
	enum opExtFlags
	{	E_SRC   = 0x01,
		E_DST   = 0x02,
		E_OP    = 0x04,
		E_SRCOP = 0x05,
		E_DSTOP = 0x06,
	};
	struct opExtEntry
	{	char           Name[16];
		void (Parser::*Func)(int,bool);
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
	struct fixup
	{	unsigned       Label;
		unsigned       Instr;
		fixup(unsigned label, unsigned instr) : Label(label), Instr(instr) {}
	};
	typedef vector<fixup> fixups_t;
	struct constDef
	{	exprValue      Value;
		location       Definition;
		constDef(const exprValue& value, const location& loc) : Value(value), Definition(loc) {}
	};
	typedef unordered_map<string,constDef> consts_t;
	struct macro
	{	location       Definition;
		vector<string> Args;
		vector<string> Content;
	};
	typedef unordered_map<string,macro> macros_t;
	struct function
	{	location       Definition;
		vector<string> Args;
		string         DefLine;
		char*          Start;
		function(const location& definition) : Definition(definition), Start(NULL) {}
	};
	typedef unordered_map<string,function> funcs_t;
	struct ifContext
	{	unsigned       Line;
		bool           Disabled;
		ifContext(unsigned line, bool disabled) : Line(line), Disabled(disabled) {}
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

	// parser working set
	char             Line[1024];  ///< Buffer for line input. Well, static size...
	char*            At = NULL;   ///< Current location within Line
	string           Token;       ///< Current token
	// context
	macro*           AtMacro = NULL;///< Currently at a macro definition
	ifs_t            AtIf;        ///< List of (nested) if statements.
	contexts_t       Context;     ///< Include and macro call stack
	// definitions
	labels_t         Labels;      ///< Label values
	lnames_t         LabelsByName;///< Label names
	fixups_t         Fixups;      ///< delayed label fixups
	funcs_t          Functions;   ///< Function definitions
	macros_t         Macros;      ///< Macros
	// instruction
	vector<uint64_t> Instructions;
	Inst             Instruct;    ///< current instruction
 private:
	string           enrichMsg(string msg);
	void             Fail(const char* fmt, ...) PRINTFATTR(2) NORETURNATTR;
	void             Error(const char* fmt, ...) PRINTFATTR(2);
	void             Warn(const char* fmt, ...) PRINTFATTR(2);

	token_t          NextToken();
	/// Work around for gcc on 32 bit Linux that can't read "0x80000000" with sscanf anymore.
	/// @return Number of characters parsed.
	static size_t    parseUInt(const char* src, uint32_t& dst);
	exprValue        ParseExpression();

	static uint8_t   getSmallImmediate(uint32_t i);
	static const smiEntry* getSmallImmediateALU(uint32_t i);
	Inst::mux        muxReg(reg_t reg);
	void             doSMI(uint8_t si);

	// OP code extensions
	void             addIf(int cond, bool mul);
	void             addUnpack(int mode, bool mul);
	void             addPack(int mode, bool mul);
	void             addSetF(int, bool mul);
	void             addCond(int cond, bool mul);
	void             addRot(int, bool mul);
	void             doInstrExt(bool mul);

	void             doALUTarget(bool mul);
	Inst::mux        doALUExpr(bool mul);

	// OP codes
	void             assembleADD(int op);
	void             assembleMUL(int op);
	void             assembleMOV(int mode);
	void             assembleBRANCH(int relative);
	void             assembleSEMA(int type);
	void             assembleSIG(int bits);

	void             ParseInstruction();

	void             defineLabel();
	void             ParseLabel();

	void             beginREP(int);
	void             endREP(int);
	void             parseSET(int flags);
	void             parseUNSET(int flags);
	void             parseIF(int);
	void             parseELSE(int);
	void             parseENDIF(int);
	bool             isDisabled();
	void             beginMACRO(int);
	void             endMACRO(int);
	void             doMACRO(macros_t::const_iterator m);
	void             defineFUNC(int);
	exprValue        doFUNC(funcs_t::const_iterator f);
	void             doINCLUDE(int);
	bool             doPreprocessor(preprocType type = PP_ALL);
	void             ParseDirective();

	void             ParseLine();
	void             ParseFile();
 public:
	                 Parser();
	void             ParseFile(const string& file);
	const vector<uint64_t>& GetInstructions();
};

#endif // PARSER_H_
