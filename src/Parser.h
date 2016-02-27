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
#include "DebugInfo.h"
#include "utils.h"

#include <inttypes.h>
#include <vector>
#include <string>
#include <memory>
#include <stdarg.h>

using namespace std;


/// Core class that does the assembly process.
class Parser : public DebugInfo
{public:
	/// Severity of assembler messages.
	enum severity : unsigned char
	{	ERROR
	,	WARNING
	,	INFO
	};

 public: // Input
	/// List of path prefixes to search for include files.
	vector<string> IncludePaths;
	/// The assembly process did not fail so far and can produce reasonable output.
	bool Success = true;
	/// Output preprocessed code to this file stream in not NULL.
	/// This is for internal use only and does not guarantee a specific result.
	FILE* Preprocessed = NULL;
	/// Display only messages with higher or same severity.
	severity Verbose = WARNING;
 public: // Result
	/// Assembled result. The index is PC.
	/// This is only valid after EnsurePass2 has been called.
	vector<uint64_t> Instructions;
 private:
	/// Type of a parser token.
	enum token_t : char
	{	END    =  0  ///< End of line
	,	WORD   = 'A' ///< Identifier or opcode
	,	NUM    = 'N' ///< numeric constant
	,	OP     = '@' ///< operator
	,	BRACE1 = '('
	,	BRACE2 = ')'
	,	SQBRC1 = '['
	,	SQBRC2 = ']'
	,	DOT    = '.'
	,	COMMA  = ','
	,	SEMI   = ';'
	,	COLON  = ':'
	};
	/// Entry of the operator lookup table, POD, compatible with binary_search().
	struct opInfo
	{	char        Name[7]; ///< Operator string
		Eval::mathOp Op;     ///< Operator type, see Eval::mathOp
	};
	///< Operator lookup table, ordered by Name. Symbol operators only.
	static const opInfo operatorMap[];
	///< Operator lookup table, ordered by Name. Alphanumeric operators only.
	static const opInfo operatorMap2[];
	/// Entry of the register lookup table, POD, compatible with binary_search().
	static const struct regEntry
	{	char        Name[16];///< Name of the register
		reg_t       Value;   ///< Register or register group, see \ref reg_t
	}             regMap[];///< Register lookup table, ordered by Name.
	/// Reference to a ADD \e or MUL ALU operator.
	class opAddMul
	{	const int8_t Op;     ///< ADD or MUL ALU opcode, bit 7 set: MUL ALU
	 public:
		/// Create from ADD ALU OP code.
		constexpr   opAddMul(Inst::opadd op) : Op(op) {}
		/// Create from MUL ALU OP code.
		constexpr   opAddMul(Inst::opmul op) : Op(op|0x80) {}
		/// true if the class instance represents a MUL ALU opcode
		/// false if it is an ADD ALU OP code.
		bool        isMul() const { return Op < 0; }
		/// Convert to Inst::opadd.
		/// @pre isMul() == false
		/// @return ADD ALU OP code.
		Inst::opadd asAdd() const { return (Inst::opadd)Op; }
		/// Convert to Inst::opmul.
		/// @pre isMul() == true
		/// @return MUL ALU OP code.
		Inst::opmul asMul() const { return (Inst::opmul)(Op & 0x7); }
	};
	/// Pack mode reference
	class packRaMul
	{	const int8_t Pack;   ///< Bit 0..3: desired pack mode, bit 7: PM.
	 public:
		/// Default constructor: do not require a pack mode.
		constexpr   packRaMul() : Pack(Inst::P_32) {}
		/// Require a pack mode.
		/// @param pack Pack mode, see Inst::pack. If you intend PM = 1 precede the pack mode with a minus sign.
		constexpr   packRaMul(int pack) : Pack((int8_t)(pack < 0 ? (-pack|0x80) : pack)) {}
		/// Check whether this instance requires any pack mode.
		bool        operator!() const { return !Pack; }
		/// Get the required pack mode for Inst::Pack.
		Inst::pack  pack() const { return (Inst::pack)(Pack & 0xf); }
		/// Get the required PM flag for Inst::PM.
		bool        mode() const { return Pack < 0; }
	};
	/// Entry of the immediate value lookup table, POD, compatible with binary_search().
	static const struct smiEntry
	{	uint32_t    Value;   ///< Desired immediate value (result)
		uint8_t     SImmd;   ///< Small immediate required to achieve this result.
		opAddMul    OpCode;  ///< ALU opcode to achieve this result.
		packRaMul   Pack;    ///< Pack mode to achieve the desired result.
	}             smiMap[];///< Immediate value lookup table used for <tt>mov rx, immd</tt>.
	/// Context of a current expression during parse, bit vector.
	enum InstContext : unsigned char
	{	IC_NONE     = 0      ///< No argument context, i.e. currently at the instruction itself.
	,	IC_SRC      = 1      ///< Bit test: currently at a source argument to an OP code; assignment: currently at the first source argument to an ADD ALU OP code.
	,	IC_DST      = 2      ///< Bit test: currently at the destination argument to an OP code; assignment: currently at the destination argument to an ADD ALU OP code.
	,	IC_B        = 4      ///< Bit test: currently at the second argument to an OP code.
	,	IC_SRCB     = 5      ///< Assignment: currently at the second source argument to an ADD ALU OP code.
	,	IC_ADD      = 8      ///< Bit test: currently at a ADD ALU instruction.
	,	IC_MUL      = 16     ///< Bit test: currently at a MUL ALU instruction.
	,	IC_BOTH     = 24     ///< Context of both ALUs, i.e branch or mov instruction with two targets.
	,	IC_CANSWAP  = 32     ///< Allow to swap ADD and MUL ALU, i.e. toggle bit 0
	};
	/// State of extensions to the current OP code, bit vector.
	enum ExtReq : unsigned char
	{	XR_NONE = 0          ///< No request by this ALU.
	,	XR_OP   = 1<<IC_NONE ///< Request at opcode level, mutual exclusive with UR_SRC*.
	,	XR_SRCA = 1<<IC_SRC  ///< Request for first source argument.
	,	XR_SRCB = 1<<IC_SRCB ///< Request for second source argument.
	,	XR_NEW  = 1<<IC_DST  ///< New request => check for interaction with other ALU.
	};

	/// Type of parser call stack entry.
	enum contextType : unsigned char
	{	CTX_ROOT             ///< Root node
	,	CTX_FILE             ///< File supplied by command line
	,	CTX_BLOCK            ///< .local block
	,	CTX_INCLUDE          ///< .include directive
	,	CTX_MACRO            ///< macro invocation
	,	CTX_FUNCTION         ///< function call
	,	CTX_CURRENT          ///< no context, current line
	};
	/// Type of preprocessor action, bit vector
	enum preprocType : unsigned char
	{	PP_MACRO = 1         ///< Macro expansion
	,	PP_IF    = 2         ///< .if/.else check
	,	PP_ALL   = 3         ///< all above actions
	};
	/// Entry of the OP code lookup table, POD, compatible with binary_search().
	/// @tparam L maximum length of Name.
	template <size_t L>
	struct opEntry
	{	char Name[L];        ///< OP code name
		void (Parser::*Func)(int);///< Parser function to call, receives the agrument below.
		int Arg;             ///< Arbitrary argument to the parser function.
	};
	///< OP code lookup table, ordered by Name.
	static const opEntry<8> opcodeMap[];
	///< Flags for OP code extensions, bit vector
	enum opExtFlags : unsigned char
	{	E_SRC   = 0x01       ///< Extension is valid for source arguments.
	,	E_DST   = 0x02       ///< Extension is valid for destination arguments.
	,	E_OP    = 0x04       ///< Extension can be applied directly to the OP code.
	,	E_SRCOP = 0x05       ///< Extension is valid for a source argument or the OP code directly.
	,	E_DSTOP = 0x06       ///< Extension is valid for the destination argument or the OP code directly.
	};
	/// Entry of the OP code extension lookup table, POD, compatible with binary_search().
	struct opExtEntry
	{	char           Name[16];///< Name of the extension
		void (Parser::*Func)(int);///< Parser function to call, receives an arbitrary argument.
		int            Arg;  ///< Arbitrary argument to the parser function. See documentation of the parser functions.
		opExtFlags     Flags;///< Flags, define the contexts where an extension may be used.
	};
	///< OP code extension lookup table, ordered by Name.
	static const opExtEntry extMap[];
	///< Assembler directive lookup table, ordered by Name.
	static const opEntry<8> directiveMap[];

	/// Label instance
	struct label
	{	const string   Name; ///< Name of the label, not unique!
		unsigned       Value;///< Value of the label if already defined.
		location       Definition;///< Where has the label been defined if already defined.
		location       Reference; ///< Where has the label been referenced if already referenced. (for messages only, only one reference is kept)
		label(const string& name) : Name(name), Value(0) {} ///< create new label entry for name
	};
	/// List of label definitions in order of appearance
	/// The index in the vector is the unique label ID.
	typedef vector<label> labels_t;
	/// @brief Label name lookup table.
	/// @details The key is the label name the value the label ID, i.e. the index into labels_t.
	typedef unordered_map<string,unsigned> lnames_t;
	/// Definition directive type, bit vector
	enum defFlags : unsigned char
	{	C_NONE  = 0    ///< ordinary .set
	,	C_LOCAL = 1    ///< local definition, e.g. .lset
	,	C_CONST = 2    ///< constant definition, i.e. not redefinable, e.g. .const
	};
	/// @brief Constant definition value
	/// @details A constant is any ExprValue that is associated with an identifier name.
	/// The name is not part of this structure.
	struct constDef
	{	exprValue      Value;     ///< Constant value
		location       Definition;///< Where has this constant been defined (for messages only)
		///< Construct from property values.
		constDef(const exprValue& value, const location& loc) : Value(value), Definition(loc) {}
	};
	/// @brief Constant lookup table.
	/// @details The key is the identifier name, the value, well, the value.
	typedef unordered_map<string,constDef> consts_t;
	/// @brief Function definition (.set)
	/// @details A function is any single line expression that evaluates to an exprValue from a set of exprValue arguments.
	struct function
	{	location       Definition;///< Where has this function been defined (for messages only)
		vector<string> Args;      ///< Identifier names of the function arguments in order of appearance.
		string         DefLine;   ///< Copy of the entire Line where the function has been defined.
		char*          Start;     ///< Pointer to the location in DefLine where the function body starts.
		/// Construct an empty function definition. The properties have to be assigned later.
		function(const location& definition) : Definition(definition), Start(NULL) {}
	};
	/// @brief Function lookup table.
	/// @details The key is the function name, the value is the function definition.
	typedef unordered_map<string,function> funcs_t;
	/// Macro flags, bit vector
	enum macroFlags : unsigned char
	{	M_NONE = 0     ///< normal macro, i.e. without a return value
	,	M_FUNC = 1     ///< functional macro, i.e. with a return value and no instructions
	};
	/// @brief macro definition
	/// @details There are two type of macros:
	/// - normal macros, defined by .marco and
	/// - functional macros, defined by .func.
	/// The have no much common, but they happen to have the same data structures and to share some code,
	/// so the use a common type.
	struct macro
	{	location       Definition;///< Where has the macro been defined.
		macroFlags     Flags;     ///< Flags
		vector<string> Args;      ///< List of identifier names of the macro arguments in order of appearance if any.
		vector<string> Content;   ///< Macro body. Line by line the macro source code, unevaluated. To get the matching source file line add the location from Definition.
	};
	/// @brief Macro definition lookup table
	/// @details The key is the macro name, the value is the macro definition.
	typedef unordered_map<string,macro> macros_t;
	/// @brief State of conditional code block context, bit vector
	/// @details Only the state IF_TRUE is an enabled state. All others indicate disabled code.
	/// @par A integer conversion from bool will implicitly give the right stage for the initial .if.
	enum ifState : unsigned char
	{	IS_FALSE       ///< .if block with condition evaluated to false
	,	IS_TRUE        ///< .if block with condition evaluated to true
	,	IS_ELSE        ///< .else block, condition (IS_TRUE) is inverted
	,	IS_INHERIT = 4 ///< .if or .else block but the condition does not matter because an outer conditional block evaluated to false.
	};
	/// Context of conditional block.
	struct ifContext : public location
	{	unsigned char  State;     ///< State of conditional block, see ifState.
		/// Construct ifContext from property values
		ifContext(const location& loc, unsigned char state) : location(loc), State(state) {}
	};
	/// Call stack if .if nested contexts. The deepest context is always the last entry.
	typedef vector<ifContext> ifs_t;
	/// @brief Context of source file invocation, e.g. .include but also macro invocation etc.
	/// @details This context consists of a invocation location (inherited from \ref location)
	/// and a set of local constant redefinitions.
	/// There is currently no option to redefine macros locally.
	struct fileContext : public location
	{	const contextType Type;   ///< Type of this context.
		consts_t       Consts;    ///< Constants (.set)
		/// Create a new invocation context.
		fileContext(contextType type, uint16_t file, uint16_t line) : Type(type) { File = file; Line = line; }
	};
	/// Call stack of file invocations. The innermost context is the last entry.
	/// The containers owns the context instances exclusively.
	typedef vector<unique_ptr<fileContext>> contexts_t;
	/// RAII class to enter a deeper file context.
	class saveContext
	{protected:
		Parser&        Parent;
		fileContext*   Context;
	 public:
		/// Enter a file context.
		/// @param parent back reference to the current parser instance.
		/// @param ctx new fileContext. The ownership of this object is taken over.
		saveContext(Parser& parent, fileContext* ctx);
		/// Leave the entered file context.
		~saveContext();
	};
	/// RAII class to enter a deeper file context while preserving the current position in the current source line.
	class saveLineContext : public saveContext
	{	const string   LineBak;
		char* const    AtBak;
	 public:
		/// Enter a deeper file context and save the current source line and the current parser position within this line.
		saveLineContext(Parser& parent, fileContext* ctx);
		/// Leave the entered file context and restore the current source line and parser position.
		~saveLineContext();
	};
	/// Instruction optimization flags, bit vector
	enum InstFlags : unsigned char
	{	IF_NONE          = 0      ///< no flags, none of the conditions below is met
	,	IF_HAVE_NOP      = 1      ///< at least one NOP in the current instruction so far
	,	IF_CMB_ALLOWED   = 2      ///< Instruction of the following line could be merged
	,	IF_BRANCH_TARGET = 4      ///< This instruction is a branch target and should not be merged
	,	IF_DATA          = 8      ///< Result of .data directive, do not optimize
	,	IF_NOSWAP        = 16     ///< Do not swap regfile A and regfile B peripheral register
	};

	// parser working set
	/// Are we already in the second pass?
	bool             Pass2 = false;

	/// @brief Current source line to be parsed.
	/// @remarks Well, static size ... todo
	char             Line[1024];
	/// @brief Current location within Line
	/// This Pointer always points to the next character to be parsed in Line.
	char*            At = NULL;
	/// Last token captured by NextToken().
	string           Token;
	/// Buffer to build up the current instruction word for the GPU.
	Inst             Instruct;
	/// Current expression context. Type InstContext.
	unsigned char    InstCtx;
	/// Unpack mode used by the current instruction. See toExtReq.
	unsigned char    UseUnpack;
	/// Vector rotation used by the current instruction. See toExtReq.
	unsigned char    UseRot;
	/// Current program counter in GPU words. Relative to the start of the assembly.
	unsigned         PC;
	/// @brief Offset in bits in the current instruction word.
	/// @details Only valid for data directives since instruction words must always start on a 64 bit boundary.
	/// A non-zero value implies that the instruction slot at PC already has been allocated.
	unsigned         BitOffset;

	// context
	/// Points to an entry of \ref Macros if we are currently inside a macro definition block. NULL otherwise.
	macro*           AtMacro = NULL;
	/// Insert the next instruction # GPU instructions before the actual \ref PC.
	unsigned         Back = 0;
	/// @brief Current context of incomplete (nested) .if/.endif blocks.
	/// @details Empty if at the top level. The deepest level is the last entry in the list.
	ifs_t            AtIf;
	/// @brief Current call stack of .include and macro invocations.
	/// @details The entries are of different types. See contextType.
	/// The list will contain at least one element for the current file.
	/// The deepest context is the last item in the list.
	contexts_t       Context;
	/// First unused entry in SourceFiles.
	/// @remark This may point to existing entries during pass 2.
	size_t           FilesCount;
	// definitions
	/// List of label definitions by label ID (= vector index).
	labels_t         Labels;
	/// @brief Next free label ID.
	/// @details The value can be different from Labels.size() because in pass 2 all labels are already defined
	/// and the Labels array is not cleared between passes.
	unsigned         LabelCount = 0;
	/// @brief Label names in the current context.
	/// @details This dictionary associates the label name with the label ID.
	/// The association may change because of local labels, e.g. with leading '.'.
	lnames_t         LabelsByName;
	/// Single line function definitions, i.e. .set with parameters.
	/// @details This definitions are not context sensitive.
	funcs_t          Functions;
	/// Function type macros = function type macros.
	/// @details This definitions are not context sensitive.
	macros_t         MacroFuncs;
	/// Macro definitions.
	/// @details This definitions are not context sensitive.
	macros_t         Macros;

	/// Flags for the above instructions.
	/// @remarks The flags are kept in a separate array to keep the Instructions array a binary blob that can directly be written to disk.
	vector_safe<uint8_t,IF_NONE> InstFlags;

 private:
	/// Get name of file ID.
	const char*      fName(uint16_t file) { return SourceFiles[file].Name.c_str(); }
	/// Enrich an assembler message with file and line context including the context stack.
	string           enrichMsg(string msg);
	/// Enrich the formatted message and throws the result as std::string.
	void             Fail(const char* fmt, ...) PRINTFATTR(2) NORETURNATTR;
	/// Enrich the formatted message and write the result to stderr.
	void             Msg(severity level, const char* fmt, ...) PRINTFATTR(3);

	/// Ensure minimum size of InstFlags array.
	void             FlagsSize(size_t min);
	/// Return a reference to the instruction flags of the current instruction.
	uint8_t&         Flags() { return InstFlags[PC]; }
	/// Store instruction word and take care of .back block if any.
	/// @param value Instruction to store.
	void             StoreInstruction(uint64_t value);

	/// Parse the next token from the current line and store the result in \ref Token.
	/// @return Type of the token just parsed.
	token_t          NextToken();
	/// Work around for gcc on 32 bit Linux that can't read "0x80000000" with sscanf anymore.
	/// @return Number of characters parsed.
	static size_t    parseInt(const char* src, int64_t& dst);
	/// @brief Get referenced label.
	/// @param name Label name.
	/// @param forward The reference is a forward reference, e.g. r:.1f.
	/// @return Reference to the label definition, either a new or an existing entry.
	/// The value is reliable only in pass 2.
	label&           labelRef(string name, bool forward = false);

	/// @brief Parse immediate value per QPU element in the [a,b,{...14}] syntax.
	/// @details The function has to be called after the opening bracket.
	/// When it returns the matching closing bracket has been read from the input.
	/// @return Integer expression compatible with ldi per element.
	/// Depending on the values between the brackets the expression has the signed or unsigned flag set.
	/// @exception std::string Syntax error.
	exprValue        parseElemInt();
	/// @brief Parse any expression and return its value.
	/// @details The parser will eat any expression and read until the next token cannot be part of the expression.
	/// It stops e.g. at a closing brace that is not opened within this function call.
	/// Therefore this function may be used recursively.
	/// @par The parser will always evaluate to exactly one value so every calculation must be compile time constant.
	/// Anything else is an error. But some runtime evaluations that are directly supported
	/// by the hardware, e.g. QPU element rotation, will work.
	/// @par The expansion of constants, functions and functional macros is also handled here.
	/// @return The expression value
	/// @exception std::string Syntax error.
	/// @post NextToken does not return WORD, COLON, OP, BRACE1, SQBRC1 or NUM on the next invocation.
	/// I.e. only END, BRACE2, SQBRC2 and SEMI are left.
	exprValue        ParseExpression();
	/// Check the current location whether there are further arguments,
	/// i.e. the next ',' comes before any termination of the instruction or expression block.
	/// @param rem Remaining text in the line, usually At.
	/// @return true: there are more arguments in the current block.
	/// @pre The function should only be called after ParseExpression.
	/// It cannot deal with expressions and braces.
	static bool      HaveMoreOperands(const char* rem) { return rem[strcspn(rem, ",;)]")] == ','; }

	/// @brief Find the first potential match for an ALU instruction that can assign an immediate value using small immediates.
	/// @details The function does not only seek for an exactly match small immediate value but also for small immediate values
	/// that	can be transformed to the wanted value by applying an ADD or MUL ALU instruction to it.
	/// The ALU instruction always takes the small immediate value for all of its arguments.
	/// @param i Requested immediate value.
	/// @return First potential match in the small immediate table.
	/// The table may have further potential matches in consecutive entries. So you need to seek
	/// until the Value property of the smiEntry changes. It is guaranteed that this happens before the end of the list.
	/// If no match is found the returned entry is still valid but it's Value property does not match.
	/// @remarks This function can significantly increase code density,
	/// because <tt>mov xx, immediate</tt> instructions often can be packed with other instruction this way.
	static const smiEntry* getSmallImmediateALU(uint32_t i);
	/// @brief Get input mux for a register read access.
	/// @details Besides choosing the right multiplexer value this function also patches the register number
	/// into the current instruction and ensures that the value does not conflict with a previous usage of the same register file.
	/// While doing this job the function takes care of choosing regfile A or B respectively if the IO register
	/// is available in both register files. In doubt regfile A is preferred to keep the regfile B field free
	/// for small immediate values.
	/// @param reg Register to read.
	/// @return Matching multiplexer value.
	/// @exception std::string Error, i.e. no valid register or the current instruction cannot read this register because of conflicts
	/// with other parts of the same instruction word.
	Inst::mux        muxReg(reg_t reg);
	/// Set small immediate value. Fail if impossible.
	/// @param si desired value.
	/// @exception std::string Failed because of conflicts with other components of the current instruction word.
	void             doSMI(uint8_t si);
	/// Checks whether the given input mux may be the source of a unpack instruction, i.e. r4 or regfile A.
	/// @param input mux to check. If the mux points to regfile A The read address is also check not to address an IO register.
	/// @return PM flag for the instruction, i.e. 1 for r4 and 0 for regfile A or -1 if not possible.
	int              isUnpackable(Inst::mux mux) { if (mux == Inst::X_R4) return true; if (mux == Inst::X_RA && Instruct.RAddrA < 32) return false; return -1; }
	/// Convert instruction context to bit vector for UseUnpack or UseRot.
	/// @param ctx Context, type InstContext.
	/// @remarks Bit 0: operator level, bit 1: source arg 1, bit 5: source arg 2
	static constexpr unsigned char toExtReq(unsigned char ctx) { return 1 << (ctx&IC_SRCB); }
	/// Swap ADD and MUL ALU of current instruction if allowed by InstCtx, adjust InstCtx.
	/// @pre Instruct.Sig < Inst::S_BRANCH
	/// @return true: swap succeeded, false: swap failed
	/// @post InstCtx & IC_MUL is inverted on success.
	bool             trySwap();
	/// Apply vector rotation to the current context.
	/// @param count Desired rotation: 0 = none, 1 = one element right ... 15 = one element left, -16 = rotate by r5
	void             applyRot(int count);

	// OP code extensions
	/// Handle \c .if opcode extension
	/// @param cond Requested condition code, must be of type Inst::conda.
	/// @exception std::string Failed, error message.
	void             addIf(int cond);
	/// @brief Handle \c .unpack extensions
	/// @details Besides .unpack* this method can also handle the short hand form like .8a applied to source arguments.
	/// @param mode Unpack mode, must be of type Inst::unpack.
	/// @exception std::string Failed, error message.
	void             addUnpack(int mode);
	/// Handle \c .pack extensions
	/// @param mode Unpack mode, must be of type Inst::pack.
	/// @exception std::string Failed, error message.
	void             addPack(int mode);
	/// Handle \c .setf opcode extension
	/// @exception std::string Failed, error message.
	void             addSetF(int);
	/// Handle \c branch condition code
	/// @param cond Condition code, must be of type Inst::condb.
	/// @exception std::string Failed, error message.
	void             addCond(int cond);
	/// @brief Handle \c .rot extension
	/// @details This extension reads an expression immediately after the \c .rot to get the rotation count.
	/// Of course the count must be a constant less than 16 or \c r5.
	/// @exception std::string Failed, error message.
	void             addRot(int);
	/// @brief Handle and dispatch instruction extensions.
	/// @details The function will check the next token for a dot '.' and if true read an extension identifier after the dot.
	/// After the identifier is read the matching \c add... function is invoked,
	/// but only if the extension is allowed within the current context.
	/// See extMap table for valid extensions and their matching handlers.
	/// @exception std::string Failed, error message.
	void             doInstrExt();
	/// Apply value in UseUnpack to current instruction after source expression.
	/// @param mux Multiplexer value of current source instruction.
	/// Could be extracted from Instruct by InstCtx, but this is easier.
	void             check4Unpack(Inst::mux mux);

	/// @brief Assemble an expression as ALU target.
	/// @details The function will also try to read instruction extensions if any.
	/// @param param Expression value
	/// @exception std::string Failed, error message.
	/// @pre The ALU selected by InstCtx must be available.
	void             doALUTarget(exprValue param);
	/// Handle an ALU source expression.
	/// @exception std::string Failed, error message.
	void             doALUExpr();
	/// Handle source argument of branch instruction.
	/// @param param Expression value
	/// @exception std::string Failed, error message.
	void             doBRASource(exprValue param);
	/// Try to get immediate value at mov by a small immediate value and an availabe ALU.
	/// @param value requested immediate value.
	/// @return true: succeeded.
	/// @exception std::string Failed, error message.
	bool             trySmallImmd(uint32_t value);

	// OP codes
	/// @brief Assemble \c add instruction.
	/// @details Some instructions are available by both ALUs. assembleADD tries to call assembleMUL
	/// if such an instruction is encountered and the ADD ALU is already busy.
	/// @param op Operator for ADD ALU, must be of type Inst::opadd.
	/// If some bit higher than bit 7 is set in op the feature to try the MUL ALU is disabled,
	/// no more no less.
	/// @exception std::string Failed, error message.
	void             assembleADD(int op);
	/// @brief Assemble \c mul instruction.
	/// @details Some instructions are available by both ALUs. assembleMUL tries to call assembleADD
	/// if such an instruction is encountered and the MUL ALU is already busy.
	/// @param op Operator for MUL ALU, must be of type Inst::opmul.
	/// If some bit higher than bit 7 is set in op the feature to try the ADD ALU is disabled,
	/// no more no less.
	/// @exception std::string Failed, error message.
	void             assembleMUL(int op);
	/// @brief Assemble \c mov or \c ldi instruction.
	/// @details assembleMOV can create almost any \c ldi or ALU instruction.
	/// if auto mode is allowed, i.e. \a mode < 0, and the source argument is an immediate value
	/// then it first tries to load the desired value from any small immediate value
	/// using any ALU operator of any available ALU. If this fails a load immediate instruction is created.
	/// @param mode Value for Inst::LdMode or < 0 to choose automatically.
	/// @exception std::string Failed, error message.
	void             assembleMOV(int mode);
	/// @brief Assemble \c read pseudo instruction.
	/// @exception std::string Failed, error message.
	void             assembleREAD(int);
	/// Assemble branch instruction.
	/// @param relative zero => \c bra, non-zero => \c brr
	/// @exception std::string Failed, error message.
	void             assembleBRANCH(int relative);
	/// Assemble semaphore instruction.
	/// @param type zero => \c srel, non-zero => \c sacq
	/// @exception std::string Failed, error message.
	void             assembleSEMA(int type);
	/// Assemble signalling instruction like \c thrend.
	/// @param bits Signalling bits, must be of type Inst::sig.
	/// @exception std::string Failed, error message.
	void             assembleSIG(int bits);
	/// Assemble an entire instruction, i.e. everything with an opcode and between ';'.
	/// @exception std::string Failed, error message.
	void             ParseInstruction();

	/// @brief Handle label definition.
	/// @details The function will not forward the parser location.
	/// The label name is expected to be the last parsed \ref Token value.
	/// @exception std::string Failed, error message.
	void             defineLabel();
	/// @brief Handle label definition with trailing colon.
	/// @details The function is intended to be invoked after NextToken returned a colon at the start of a line.
	/// It parses the label name and leaves the rest of the line untouched for further persing.
	/// @exception std::string Failed, error message.
	void             parseLabel();

	// directives
	/// @brief Handle .global
	void             parseGLOBAL(int);
	/// Handle raw data directives like \c .byte.
	/// @param type Data type:
	/// - 1 => 8 bit int data
	/// - 2 => 16 bit int data
	/// - 4 => 32 bit int data
	/// - -4 => 32 bit float data
	/// @exception std::string Failed, error message.
	void             parseDATA(int type);
	/// Handle .align.
	/// @param bytes Bytes to align. 1 = byte alignment, 8 = 64 bit alignment etc.
	/// Must be a power of 2 or -1 for parsing an alignment argument.
	/// @exception std::string Failed, error message.
	void             parseALIGN(int bytes);
	/// Enforce alignment of PC.
	/// @param bytes Bytes to align. 1 = byte alignment, 8 = 64 bit alignment etc.
	/// @param offset Do not align based on 0 but based on offset instead.
	/// @pre \a bytes must be a power of 2.
	/// @return true: alignment caused padding.
	bool             doALIGN(int bytes, int offset);
	/// Enter local block.
	void             beginLOCAL(int);
	/// Leave local block.
	void             endLOCAL(int);
	/// @brief Handle \c .rep or .foreach directive.
	/// @param mode 0 = .rep, 1 = .foreach
	/// @details This opens a local macro with the reserved name ".rep" or ".foreach" to record the code block
	/// and pushes the arguments as macro arguments. The first argument is always the loop identifier.
	/// @exception std::string Failed, error message.
	void             beginREP(int mode);
	/// @brief Handle \c .endr or .endfor directive.
	/// @param mode 0 = .rep, 1 = .foreach
	/// @details This completes the .rep/.foreach macro and invokes it immediately the requested number of times.
	/// @exception std::string Failed, error message.
	void             endREP(int node);
	/// @brief Handle \c .back directive.
	/// @details This function basically sets the \ref Back member to a specific value.
	/// @exception std::string Failed, error message.
	void             beginBACK(int);
	/// @brief Handle \c .endback directive.
	/// @details This function basically resets the \ref Back member to zero.
	/// @exception std::string Failed, error message.
	void             endBACK(int);
	/// @brief Handle .clone directive.
	/// @details This function copies a few bytes of binary code at the current PC.
	/// If this is a forward reference the result is only reliable after pass 2.
	/// @exception std::string Failed, error message.
	void             parseCLONE(int);
	/// @brief Handle directives to set constants or inline functions, e.g. \c .set.
	/// @param flags Directive type, must be of type defFlags.
	/// @exception std::string Failed, error message.
	void             parseSET(int flags);
	/// @brief Handle directives to remove constants or inline functions, e.g. \c .unset.
	/// @param flags Directive type, must be of type defFlags.
	/// @exception std::string Failed, error message.
	void             parseUNSET(int flags);
	/// Read boolean value from the assembler source.
	/// @return boolean value.
	/// @exception std::string The next expression in the pipeline did not evaluate to a compile time constant.
	/// @remarks the function is intended to be used by \c .if or similar.
	bool             doCondition();
	/// Handle \c .if directive.
	/// @exception std::string Failed, error message.
	void             parseIF(int);
	/// Handle \c .ifset directive.
	/// @exception std::string Failed, error message.
	void             parseIFSET(int flags);
	/// Handle \c .else directive.
	/// @exception std::string Failed, error message.
	void             parseELSE(int);
	/// Handle \c .elseif directive.
	/// @exception std::string Failed, error message.
	void             parseELSEIF(int);
	/// Handle \c .endif directive.
	/// @exception std::string Failed, error message.
	void             parseENDIF(int);
	/// Check whether the current code line should be active.
	/// @return The function returns true if and only if the condition of all nested \c .if directives
	/// where the current line belongs to are either true or we are in the else block
	/// or we are not in a \c .if block at all.
	bool             isDisabled() { return AtIf.size() != 0 && AtIf.back().State != IS_TRUE; }
	/// @brief Handle \c .assert directive.
	/// @details throws an exception if the following condition is not met.
	/// @exception std::string Failed, error message or condition not met.
	void             parseASSERT(int);
	/// @brief Handle \c .macro or \c .func directive.
	/// @details The function creates a macro in \ref Macros and stores a pointer to the newly created entry in AtMacro.
	/// It also stores the identifiers of the macro arguments.
	/// @param flags Macro type, see macroFlags.
	/// @exception std::string Failed, error message.
	void             beginMACRO(int flags);
	/// @brief Handle \c .endm or \c .endf directive.
	/// @param flags Macro type, see macroFlags.
	/// @exception std::string Failed, error message.
	void             endMACRO(int flags);
	/// @brief Invoke a macro, defined by \c .macro.
	/// @details The function is intended to be called immediately after the macro name.
	/// It reads the optional macro arguments and invokes the macro.
	/// @par Executing a macro creates a new invocation context.
	/// @param m Iterator that points to the macro to invoke.
	/// This is an iterator to an entry in \ref Macros to get access to the macro name for error messages too.
	/// @exception std::string Failed, error message.
	void             doMACRO(macros_t::const_iterator m);
	/// @brief Invoke a functional macro defined by \c .func.
	/// @details The function is intended to be called immediately after the macro name.
	/// It reads the optional macro arguments and invokes the function.
	/// @par Executing a function macro also creates a new invocation context.
	/// In contrast to non function contexts also the current Line and the current location in this line is preserved.
	/// @param m Iterator that points to the functional macro to invoke.
	/// This is an iterator to an entry in \ref MacroFuncs to get access to the macro name for error messages too.
	/// @return Expression to which the functional macro evaluated after passing arguments.
	/// @exception std::string Failed, error message.
	exprValue        doFUNCMACRO(macros_t::const_iterator m);
	/// @brief Invoke inline function, i.e. function defined by \c .set or similar.
	/// @details The function is intended to be called immediately after the function name.
	/// It reads the arguments and invokes the function.
	/// @par Executing a function also creates a new invocation context.
	/// In contrast to non function contexts also the current Line and the current location in this line is preserved.
	/// @param f Iterator that points to the function to invoke.
	/// This is an iterator to an entry in \ref Funcs to get access to the function name for error messages too.
	/// @return Expression to which the functional macro evaluated after passing arguments.
	/// @exception std::string Failed, error message.
	exprValue        doFUNC(funcs_t::const_iterator f);
	/// Handle code segment directive
	/// @param flags see \ref SegFlags.
	void             doSEGMENT(int flags);
	/// Handle \c .include directive.
	/// @details The Function reads the file name and immediately invokes the parser for this file.
	/// @par This creates a new invocation context.
	/// @exception std::string Failed, error message.
	void             doINCLUDE(int);
	/// @brief Handle the current line in context of preprocessor instructions.
	/// @param type What kind of preprocessor actions to perform.
	/// Default \ref PP_ALL, i.e. check for macros and for .if context.
	/// @return The function returns true if the content of the current line should not be handled further
	/// by the parser. I.e. if the line is only recorded as macro for later execution or if it is disabled by \c .if.
	/// @details If \ref PP_MACRO is passed the function first checks whether we are currently recording a macro (including .rep or similar).
	/// If true the current line is appended to the recorded macro an the function returns true.
	/// @par If \ref PP_IF is passed and the current line is in an .if/else block that is currently inactive
	/// the function returns true.
	/// @par Otherwise the function returns false.
	/// @exception std::string Failed, error message.
	bool             doPreprocessor(preprocType type = PP_ALL);
	/// @brief Parse assembler directive.
	/// @details This function is intended to be called after the leading dot '.' that indicates a assembler directive.
	/// It reads the directive name and dispatches the execution to the matching handler function.
	/// See directiveMap for details.
	/// @exception std::string Failed, error message.
	void             ParseDirective();

	/// @brief Parse the current line.
	/// @pre The data is expected to be placed in \ref Line before.
	/// @details The function handles all kind of instructions, directives or whatever.
	/// If instructions are generated by the current line, they are stored in the \ref Instruc array.
	/// @exception std::string Failed, error message.
	void             ParseLine();
	/// @brief Parse the content of a file.
	/// @pre The file name should be set up as incocation Context in \ref Context before.
	/// @details All lines of the file are read and passed to ParseLine one by one.
	/// If an exception is thrown the \ref Success flag is reset and the parser continues with the next line in the file.
	/// @exception std::string The file can't be read, error message.
	void             ParseFile();

	/// This function resets the parser before any pass.
	void             ResetPass();
 public:
	/// @brief Create a virgin parser
	/// @details You should call ParseFile to parse the sorce code.
	                 Parser();
	/// Reset the parser to it's initial state after construction.
  void             Reset();
  /// @brief Parse a source file as root context.
  /// @details You may call this function repeatedly to parse multiple files as one block.
  /// This might be used for global function definition files that are not explicitely included by every source file.
  /// E.g. \c vc4.qinc.
	void             ParseFile(const string& file);
	/// This function switches to pass 2 after pass 1, i.e. ParseFile, has completed.
	/// @post This call ensures the validity of Instructions, GlobalSymbolsByName and DebugInfo.
	void             EnsurePass2();
};

#endif // PARSER_H_
