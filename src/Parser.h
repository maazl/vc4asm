/*
 * Parser.h
 *
 *  Created on: 30.08.2014
 *      Author: mueller
 */

#ifndef PARSER_H_
#define PARSER_H_

#include "Eval.h"
#include "AssembleInst.h"
#include "DebugInfo.h"
#include "Message.h"
#include "utils.h"

#include <inttypes.h>
#include <climits>
#include <vector>
#include <string>
#include <memory>
#include <stdarg.h>


using namespace std;


// Work around because gcc can't define a constexpr struct inside a class, part 1.
#define MSG ParserMSG
#include "Parser.MSG.h"
#undef MSG

/// Core class that does the assembly process.
class Parser : protected AssembleInst, public DebugInfo
{public:
	/// Operation mode.
	enum mode : unsigned char
	{	PASS1ONLY    ///< Run pass 1 only. (Do not resolve branch labels.) @remarks Show all messages in pass 1.
	,	NORMAL       ///< Break after pass 1 in case of errors. @remarks Causes Warnings to be suppressed during pass 1.
	,	IGNOREERRORS ///< Always enter pass 2, even in case of errors. @remarks Show no messages in pass 1.
	};

	/// Type of parser call stack entry.
	enum contextType : unsigned char
	{	CTX_ROOT     ///< Root node
	,	CTX_FILE     ///< File supplied by command line
	,	CTX_BLOCK    ///< .local block
	,	CTX_INCLUDE  ///< .include directive
	,	CTX_MACRO    ///< macro invocation
	,	CTX_FUNCTION ///< function call
	,	CTX_CURRENT  ///< no context, current line
	};
	/// Invocation context
	struct context : location
	{	contextType  Type; ///< Type of this context.
	};

 public: // messages
	struct Message : AssembleInst::Message
	{	vector<context> Context;
		Parser& Parent() const noexcept { return (Parser&)AssembleInst::Message::Parent; }
	 private:
		void SetContext();
	 public:
		Message(AssembleInst::Message&& msg) : AssembleInst::Message(move(msg)) { SetContext(); }
		Message(Parser& parent, const ::Message& msg) : AssembleInst::Message(parent, msg) { SetContext(); }
		virtual string toString() const noexcept;
	};

	/// Message handler. Prints to \c stderr by default.
	/// @remarks Redefinition with more specialized type.
	function<void(const Message& msg)> OnMessage = Message::printHandler;

	// Work around because gcc can't define a constexpr struct inside a class, part 2.
	//#include "Parser.MSG.h"
	static constexpr const struct ParserMSG MSG = ParserMSG;

 public: // Input
	/// List of path prefixes to search for include files.
	vector<string> IncludePaths;
	/// The assembly process did not fail so far and can produce reasonable output.
	bool           Success = true;
	/// Output preprocessed code to this file stream if not NULL.
	/// This is for internal use only and does not guarantee a specific result.
	FILE*          Preprocessed = NULL;
	/// Display only messages with higher or same severity.
	severity       Verbose = WARN;
	/// See \see mode.
	mode           OperationMode = NORMAL;
 public: // Result
	/// Assembled result. The index is PC.
	/// This is only valid after EnsurePass2 has been called.
	vector<uint64_t> Instructions;

 private: // types...
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
	,	COMMENT= '#' ///< begin of comment not returned by NextToken()
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
		void (Parser::*Func)(int);///< Parser function to call, receives the argument below. Precondition: Token contains the directive name.
		int Arg;             ///< Arbitrary argument to the parser function.
	};
	///< OP code lookup table, ordered by Name.
	static const opEntry<8> opcodeMap[];
	/// Entry of the OP code extension lookup table, POD, compatible with binary_search().
	struct opExtEntry
	{	char           Name[16];///< Name of the extension
		instContext    Where;///< Flags, define the contexts where an extension may be used.
		void (Parser::*Func)(int);///< Parser function to call, receives an arbitrary argument.
		int            Arg;  ///< Arbitrary argument to the parser function. See documentation of the parser functions.
	};
	///< OP code extension lookup table, ordered by Name.
	static const opExtEntry extMap[];
	///< Assembler directive lookup table, ordered by Name.
	static const opEntry<8> directiveMap[];

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
	/// Base of function and macro.
	struct funcbase
	{	vector<string> Args;      ///< Identifier names of the function arguments in order of appearance.
		location       Definition;///< Where has this function been defined (for messages only)
	};
	/// @brief Function definition (.set)
	/// @details A function is any single line expression that evaluates to an exprValue from a set of exprValue arguments.
	struct funcMacro : funcbase
	{	string         DefLine;   ///< Copy of the entire Line where the function has been defined.
		unsigned       Start;     ///< Character in DefLine where the function body starts.
		/// Construct an empty function definition. The properties have to be assigned later.
		funcMacro(const location& definition) : Start(0) { Definition = definition; }
	};
	/// @brief Function lookup table.
	/// @details The key is the function name, the value is the function definition.
	typedef unordered_map<string,funcMacro> funcs_t;
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
	struct macro : funcbase
	{	macroFlags     Flags;     ///< Flags
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
	struct fileContext : public context
	{	consts_t       Consts;    ///< Constants (.set)
		/// Create a new invocation context.
		fileContext(contextType type, uint16_t file, uint16_t line) { Type = type; File = file; Line = line; }
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
		unsigned       AtBak;
	 public:
		/// Enter a deeper file context and save the current source line and the current parser position within this line.
		saveLineContext(Parser& parent, fileContext* ctx);
		/// Leave the entered file context and restore the current source line and parser position.
		~saveLineContext();
	};

 private: // parser working set
	/// Are we already in the second pass?
	bool             Pass2 = false;

	/// @brief Current source line to be parsed.
	string           Line;
 private: // items valid per parser token...
	/// @brief Current location within Line
	/// This Pointer always points to the next character to be parsed in Line.
	const char*      At = NULL;
	/// Last token captured by NextToken().
	string           Token;
 private: // items valid per expression...
	/// Current expression.
	exprValue        ExprValue;
 private: // items valid per QPU instruction word...
	/// Current program counter in GPU words. Relative to the start of the assembly.
	unsigned         PC;
	/// @brief Offset in bits in the current instruction word.
	/// @details Only valid for data directives since instruction words must always start on a 64 bit boundary.
	/// A non-zero value implies that the instruction slot at PC already has been allocated.
	unsigned         BitOffset;

 private: // context
	/// Points to an entry of \ref Macros if we are currently inside a macro definition block. NULL otherwise.
	macro*           AtMacro = NULL;
	/// Insert the next instruction # GPU instructions before the actual \ref PC.
	unsigned         Back = 0;
	/// @brief Current context of incomplete (nested) .if/.endif blocks.
	/// @details Empty if at the top level. The deepest level is the last entry in the list.
	ifs_t            AtIf;
	/// Number of currently entered .rep or .foreach contexts.
	unsigned         LoopDepth = 0;
	/// @brief Current call stack of .include and macro invocations.
	/// @details The entries are of different types. See contextType.
	/// The list will contain at least two elements, one for the current root and one for the current file.
	/// The deepest context is the last item in the list.
	contexts_t       Context;
	/// First unused entry in SourceFiles.
	/// @remark This may point to existing entries during pass 2.
	size_t           FilesCount;
	// definitions
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
	vector_safe<instFlags,IF_NONE> InstFlags;

 private:
	/// Override exception generation to subclass exception class.
	[[noreturn]] virtual void ThrowMessage(AssembleInst::Message&& msg) const;
	/// Replacement for the hidden OnMessage function.
	static void      EnrichMessage(AssembleInst::Message&& msg);
	/// Log thrown exception from Fail() as error message.
	/// @param msg Already enriched message.
	void             CaughtMsg(const Message& msg);

	/// Ensure minimum size of InstFlags array.
	void             FlagsSize(size_t min);
	/// Store instruction word and take care of .back block if any.
	/// @param value Instruction to store.
	void             StoreInstruction(uint64_t value);

	/// Move At to the next non whitespace character or the end of the line.
	/// @post At points to non whitespace character or 0 in case of line end.
	void             ToNextChar() { At += strspn(At, " \t\r\n"); }
	/// Parse the next token from the current line and store the result in \ref Token.
	/// @return Type of the token just parsed.
	token_t          NextToken();
	/// Work around for gcc on 32 bit Linux that can't read "0x80000000" with sscanf anymore.
	/// @return Number of characters parsed.
	static size_t    parseInt(const char* src, int64_t& dst);
	/// Read the next token and ensure that it is an identifier.
	/// @param Name of the related directive for error messages.
	/// @post Token contains the identifier name.
	void             parseIdentifier(const char* directive);
	/// @brief Get referenced label.
	/// @param name Label name.
	/// @param forward The reference is a forward reference, e.g. r:.1f.
	/// @return Reference to the label definition, either a new or an existing entry.
	/// The value is reliable only in pass 2.
	label&           labelRef(string name, bool forward = false);

	/// Parse immediate value per QPU element in the [x0,x1...x15] syntax.
	/// @pre \see At must be after the opening square brace.
	/// @post \see ExprValue is assigned to an integer compatible with \c ldi per element.
	/// Depending on the values between the brackets the expression has the signed or unsigned flag set.
	/// \see At is after the closing square brace.
	/// @exception std::string Syntax error.
	void             parseElemInt();
	/// @brief Parse internal register expression. Syntax: :[regnum,regtype,rotate,pack]
	/// @pre \see At must be after the opening square brace.
	/// @post \see ExprValue is assigned to a register expression.
	/// \see At is after the closing square brace.
	/// @exception std::string Syntax error.
	void             parseRegister();
	/// @brief Parse any expression and return its value.
	/// @details The parser will eat any expression and read until the next token cannot be part of the expression.
	/// It stops e.g. at a closing brace that is not opened within this function call.
	/// Therefore this function may be used recursively.
	/// @par The parser will always evaluate to exactly one value so every calculation must be compile time constant.
	/// Anything else is an error. But some runtime evaluations that are directly supported
	/// by the hardware, e.g. QPU element rotation, will work.
	/// @par The expansion of constants, functions and functional macros is also handled here.
	/// @post ExprValue is assigned the resulting expression value.
	/// @par NextToken does not return WORD, COLON, OP, BRACE1, SQBRC1 or NUM on the next invocation.
	/// I.e. only END, BRACE2, SQBRC2, COMMA and SEMI are left.
	/// @exception std::string Syntax error.
	void             ParseExpression();
	/// @brief Count the number of Arguments
	/// @param rem Remaining text in the line, usually At.
	/// @param max Scan for at most \a max arguments.
	/// @return Number of arguments found, at most max.
	/// @details The function in fact counts the number of commas at the current expression level.
	/// I.e. any comma in a nested level (enclosed by round or square braces) does not count.
	/// The number of arguments is the number of commas +1.
	/// Scanning stops if any of the following conditions is met:
	/// - end of line,
	/// - comment, i.e. '#',
	/// - semicolon,
	/// - an unmatched closing brace within this context or
	/// - \a max is reached.
	static int       ArgumentCount(const char* rem, int max = INT_MAX);

	// OP code extensions
	/// Handle \c .if opcode extension
	/// @param cond Requested condition code, must be of type Inst::conda.
	/// @exception std::string Failed, error message.
	void             addIf(int cond);
	/// Handle \c .pack or \c .unpack extensions
	/// @param mode Pack or unpack mode, see \see rPUp.
	/// @exception std::string Failed, error message.
	void             addPUp(int mode);
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

	/// @brief Assemble an expression as ALU target.
	/// @details The function will also try to read instruction extensions if any.
	/// @pre ExprValue contains the expression value to be used for the ALU target.
	/// @par The ALU selected by InstCtx must be available.
	/// @exception std::string Failed, error message.
	void             doALUTarget();
	/// Handle an ALU source expression.
	/// @exception std::string Failed, error message.
	void             doALUExpr();
	/// Try to get immediate value at mov by
	/// Handle nop instruction, ADD or MUL ALU.
	void             doNOP();

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
	/// @param mode One of
	/// - value for Inst::LdMode to create ldi or semaphore instruction. Bit 7 is the acquire flag in case of L_SEMA.
	/// - ~(IC_ADD|IC_MUL), ~IC_ADD or ~IC_MUL to choose load mode automatically and optionally restrict to only one of the ALUs.
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
	//void             assembleSEMA(int type);
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
	label&           defineLabel();
	/// Add a symbol to the global symbol table.
	/// @param name Global symbol name.
	/// @pre ExprValue contains global symbol value.
	/// @exception std::string Failed, error message.
	void             addGlobal(const string& name);
	/// @brief Handle label definition with trailing colon.
	/// @details The function is intended to be invoked after NextToken returned a colon at the start of a line.
	/// It parses the label name and leaves the rest of the line untouched for further parsing.
	/// @exception std::string Failed, error message.
	void             parseLabel();

	// directives
	/// @brief Handle .global
	void             parseGLOBAL(int);
	/// Handle raw data directives like \c .byte.
	/// @param bits Number of bits. Negative values are IEEE 754 floating point types.
	/// @exception std::string Failed, error message.
	void             parseDATA(int bits);
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

	/// Parse argument list of a macro or function.
	/// @param name Name of the macro (for error messages).
	/// @param count Expected number of arguments. Anything else is an error.
	/// @return List of expression values for macro invocation.
	vector<exprValue> parseArgumentList(const string& name, size_t count);
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
	/// @details It reads the optional macro arguments and invokes the function.
	/// @par Executing a function macro also creates a new invocation context.
	/// In contrast to non function contexts also the current Line and the current location in this line is preserved.
	/// @pre The function should be called immediately after the macro name.
	/// @post ExprValue receives the value to which the functional macro evaluated after passing arguments.
	/// @param m Iterator that points to the functional macro to invoke.
	/// This is an iterator to an entry in \ref MacroFuncs to get access to the macro name for error messages too.
	/// @exception std::string Failed, error message.
	void             doFUNCMACRO(macros_t::const_iterator m);
	/// @brief Invoke inline function, i.e. function defined by \c .set or similar.
	/// @details It reads the arguments and invokes the function.
	/// @par Executing a function also creates a new invocation context.
	/// In contrast to non function contexts also the current Line and the current location in this line is preserved.
	/// @param f Iterator that points to the function to invoke.
	/// This is an iterator to an entry in \ref Funcs to get access to the function name for error messages too.
	/// @pre The function should be called immediately after the function name.
	/// @post ExprValue receives the value to which the function evaluated after passing arguments.
	/// @exception std::string Failed, error message.
	void             doFUNC(funcs_t::const_iterator f);
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
	/// If instructions are generated by the current line, they are stored in the \ref Instructions array.
	/// @exception std::string Failed, error message.
	void             ParseLine();
	/// @brief Parse the content of a file.
	/// @pre The file name should be set up as invocation Context in \ref Context before.
	/// @details All lines of the file are read and passed to ParseLine one by one.
	/// If an exception is thrown the \ref Success flag is reset and the parser continues with the next line in the file.
	/// @exception std::string The file can't be read, error message.
	void             ParseFile();

	/// This function resets the parser before any pass.
	void             ResetPass();
 public:
	/// @brief Create a virgin parser
	/// @details You should call ParseFile to parse the source code.
	                 Parser();
	/// Reset the parser to it's initial state after construction.
  void             Reset();
	/// File file in IncludePaths
	/// @param file File Name to find.
	/// @return Path to the first matching file found. Empty on error.
	string           FindIncludePath(const string& file);
  /// @brief Parse a source file as root context.
  /// @details You may call this function repeatedly to parse multiple files as one block.
  /// This might be used for global function definition files that are not explicitly included by every source file.
  /// E.g. \c vc4.qinc.
	void             ParseFile(const string& file);
	/// This function switches to pass 2 after pass 1, i.e. ParseFile, has completed.
	/// @post This call ensures the validity of Instructions, GlobalSymbolsByName and DebugInfo.
	void             EnsurePass2();
};

#endif // PARSER_H_
