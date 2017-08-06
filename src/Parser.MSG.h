/*
 * Parser.MSG.h
 *
 *  Created on: 11.06.2017
 *      Author: mueller
 */

static const constexpr struct MSG
{
	#define F(name, major, minor, text, ...) const msgTemplate<__VA_ARGS__> name = { { MS_Parser, FATAL, major, minor }, text }
	#define E(name, major, minor, text, ...) const msgTemplate<__VA_ARGS__> name = { { MS_Parser, ERROR, major, minor }, text }
	#define W(name, major, minor, text, ...) const msgTemplate<__VA_ARGS__> name = { { MS_Parser, WARN, major, minor }, text }
	#define I(name, major, minor, text, ...) const msgTemplate<__VA_ARGS__> name = { { MS_Parser, INFO, major, minor }, text }
	// Entries must be ordered by major, minor.
	// General tokens
	E(EXPECTED_COMMA                  ,  1, 1, "Expected ',', found '%s'.", const char*);
	E(EXPECTED_COMMA_2                ,  1, 2, "Expected ', <%s>', found '%s'.", const char*, const char*);
	E(EXPECTED_COMMA_OR_BRC           ,  2,10, "Expected ',' or ')' after argument, found '%s'.", const char*);
	E(EXPECTED_COMMA_OR_SQBRC         ,  2,20, "Expected ',' or ']', found '%s'.", const char*);
	E(EXPECTED_BRC                    ,  3,10, "Expected ')', found '%s'.", const char*);
	E(EXPECTED_SQBRC                  ,  3,20, "Expected ']', found '%s'.", const char*);
	E(EXPECTED_EOL                    ,  4, 1, "Expected end of line.");
	E(EXPECTED_EOL_OR_COMMA           ,  4,10, "Expected end of line or ','. Found '%s'.", const char*);
	E(EXPECTED_EOL_OR_SEMICOLON       ,  4,20, "Expected end of line or ';' after instruction. Found '%s'.", const char*);
	E(EXPECTED_EOL_OR_BRO             ,  4,30, "Expected end of line or '(' (argument list). Found '%s'.", const char*);
	E(EXPECTED_EOL_OR_INSTRUCTION     ,  4,50, "Expected additional instruction or end of line after ';'. Found '%s'.", const char*);
	E(EXPECTED_ARG_NAME               ,  5, 0, "Argument name expected, found '%s'.", const char*);
	E(UNEXPECTED_TOKEN                ,  6, 0, "Unexpected '%s' at start of logical line.", const char*);
	E(UNKNOWN_OPERATOR                ,  7, 1, "Unknown operator: '%s'.", const char*);
	// Expressions
	E(NO_FLOAT_VALUE                  , 10,10, "'%s' is no floating point number.", const char*);
	E(NO_INT_VALUE                    , 10,20, "'%s' is no integer number.", const char*);
	E(SYNTAX_INCOMPLETE_EXPRESSION    , 11, 0, "Syntax: incomplete expression.");
	E(UNMATCHED_OPENING_BRACE         , 12, 1, "Closing brace is missing.");
	E(UNMATCHED_CLOSING_BRACE         , 12, 2, "Unexpected closing brace.");
	E(UNEXPECTED_END_IN_ARG_LIST      , 15, 0, "Unexpected end of argument list.");
	// pack/unpack expressions
	E(NO_2ND_PUP                      , 20, 0, "Already applied pack/unpack to the current expression.");
	E(PUP_REQUIRES_REGISTER           , 21, 0, "Pack/unpack options in expression context are only available for registers, not for %s.", const char*);
	E(NO_PPUP_VS_SEMA                 , 22, 0, "Cannot pack/unpack semaphore register.");
	// vector rotation expression
	E(ROT_REQUIRES_INT_OR_R5          , 25, 0, "QPU rotation needs an integer argument or r5 for the rotation count.");
	// identifiers
	E(IDENTIFIER_NO_START_DIGIT       , 30, 1, "Identifiers must not start with a digit.");
	E(UNDEFINED_IDENTIFIER            , 31,10, "The identifier '%s' is undefined.", const char*);
	E(UNDEFINED_OPCODE_OR_MACRO       , 31,20, "Invalid opcode or unknown macro: '%s'", const char*);
	// per QPU element constants
	E(LDPE_UNEXPECTED_TOKEN           , 40, 0, "Unexpected '%s' in per QPU element constant.", const char*);
	E(LDPE_REQUIRES_INT_VALUES        , 41, 1, "Only integers are allowed between [...].");
	E(LDPE_VALUE_OUT_OF_RANGE         , 42,10, "Load per QPU element can only deal with integer constants in the range of [-2..3]. Found: 0x" PRIx64 ".", int64_t);
	E(LDPE_SIGN_CONFLICT              , 42,20, "All integers in load per QPU element must be either in the range [-2..1] or in the range [0..3].");
	E(LDPE_TOO_MANY_VALUES            , 43, 1, "Too many initializers for per QPU element constant. Expected exactly 16.");
	E(LDPE_TOO_FEW_VALUES             , 43, 2, "Too few initializers for per QPU element constant. Expected exactly 16.");
	// raw register
	E(RAW_REG_REQUIRES_INT_VALUES     , 45, 0, "Expecting integer constant in raw register value, found %s.", const char*);
	// Labels
	E(LABEL_INVALID_PREFIX            , 60, 0, "'%s:' is no valid label prefix.", const char*);
	E(LABEL_EXPECTED_NAME             , 61, 0, "Expected label name after ':', found '%s'.", const char *);
	E(LABEL_EXPECTED_INTERNAL_NUMBER  , 62, 0, "Internal: expecting integer constant, found '%s'.", const char*);
	E(LABEL_REDEFINITION              , 63, 0, "Redefinition of non-local label %s, previously defined at %s (%u).", const char*, const char*, uint16_t);
	E(LABEL_UNDEFINED                 , 64, 0, "Label '%s' is undefined. Referenced from %s (%u).", const char*, const char*, uint16_t);
	I(LABEL_UNUSED                    , 65, 0, "Label '%s' defined at %s (%u) is not used.", const char*, const char*, uint16_t);
	F(INCONSISTENT_LABEL_DEFINITIONS  , 66, 0, "Internal: inconsistent label definition during Pass 2.");
	E(NO_LABEL_AT_BIT_BOUNDARY        , 67, 0, "Cannot set a label at a bit boundary. At least byte alignment is required.");
	// global symbols
	E(GLOBAL_EXPECTED_NAME            , 71, 0, "Expected global symbol name after .global. Found '%s'.", const char *);
	W(GLOBAL_SYMBOL_REDEFINED         , 73,10, "Another label or value has already been assigned to the global symbol '%s'.", const char*);
	I(LABEL_GLOBAL_TWICE              , 73,20, "Label '%s' has already been marked as global.", const char*);
	E(GLOBAL_REQUIRES_LABEL_OR_INT    , 75, 0, "The value of a global symbol definition must be either a label or an integer constant. Found %s.", const char *);
	E(GLOBAL_INT_OUT_OF_RANGE         , 76, 0, "Cannot export 64 bit constant 0x%" PRIx64 "as symbol.", int64_t);
	// instruction extensions
	E(EXPECTED_INSTRUCTION_EXTENSION  , 90, 0, "Expected instruction extension after '.', found '%s'.", const char*);
	E(UNKNOWN_INSTRUCTION_EXTENSION   , 91, 0, "Unknown instruction extension '%s'.", const char*);
	E(INSTRUCTION_EXTENSION_INVALID   , 92, 0, "Invalid instruction extension '%s' within this context (%x).", const char*, unsigned char);
	// general directives
	E(EXPECTED_DIRECTIVE              ,100, 0, "Expected assembler directive after '.'. Found '%s'.", const char*);
	E(UNKNOWN_DIRECTIVE               ,101, 0, "Unknown assembler directive '%s'.", const char*);
	E(END_DIRECTIVE_WO_START          ,105, 0, "Directive .%s without .%s within the current context.", const char*, const char*);
	E(DIRECTIVE_REQUIRES_IDENTIFIER   ,106, 0, "Directive .%s requires identifier, found '%s'.", const char*, const char*);
	E(UNTERMINATED_BLOCK              ,109, 0, "Unterminated context block in the current file at line %u.", unsigned);
	// .data
	E(DATA_REQUIRES_INT_OR_FLOAT      ,110, 0, "Immediate data instructions require integer or floating point constants. Found %s.", const char*);
	W(DATA_FLOAT32_OUT_OF_RANGE       ,111,11, "The value %g is outside the domain of single precision floating point.", double);
	W(DATA_FLOAT16_OUT_OF_RANGE       ,111,12, "The value %g is outside the domain of half precision floating point.", double);
	W(DATA_INT_OUT_OF_RANGE           ,111,21, "%i bits integer value out of range [%" PRIi32 ", %" PRIi64 "]: 0x%" PRIx64, int, int32_t, int64_t, int64_t);
	W(DATA_BIT_OUT_OF_RANGE           ,111,22, "Bit value out of range: 0x%" PRIx64 ". Expected 0 or 1.", int64_t);
	W(DATA_UNALIGNED                  ,115, 0, "Unaligned immediate data directive. %i bits missing for correct alignment.", int);
	// .align
	E(ALIGN_REQUIRES_INT              ,120, 0, "Expected integer constant after .align, found %s.", const char*);
	E(ALIGN_OUT_OF_RANGE              ,121,10, "Alignment value must be in the range [0, 64], found %" PRIi64 ".", int64_t);
	E(ALIGN_NO_POWER_OF_2             ,122,20, "Alignment value must be a power of 2, found %i.", int);
	E(ALIGN_EXPECTED_EOL_OR_COMMA     ,123, 0, "Expected end of line or ,<offset>. Found '%s'.", const char*);
	E(ALIGN_OFFSET_REQUIRES_INT_OR_LABEL,125,0,"Expected integer constant or label as alignment offset, found '%s'.", const char*);
	// .loop/.rep
	E(EXPECTED_LOOP_VARIABLE_NAME     ,130, 0, "Expected loop variable name, found '%s'.", const char*);
	E(REP_REQUIRES_INT                ,131, 0, "Second argument to .rep must be a non-negative integral number, found '%s'.", const char*);
	E(REP_COUNT_OUT_OF_RANGE          ,132, 0, "Repetition count out of range. Expected [0, 0x1000000], found '%" PRIi64 "'.", int64_t);
	// .if
	E(CONDITION_REQUIRES_INT          ,140, 0, "Conditional expression must be an integer, found '%s'.", const char*);
	W(IF_DIRECTIVE_OTHER_FILE         ,145,10, ".endif belongs to .if in another file: '%s'.", const char*);
	W(UNTERMINATED_IF_IN_FILE         ,145,20, ".if at line %u in current file unterminated.", uint16_t);
	// .back
	E(BACK_REQUIRES_INT               ,150, 0, "Expected integer constant after .back, found '%s'.", const char*);
	E(BACK_COUNT_OUT_OF_RANGE         ,151, 0, ".back count out of range. Expected [0, 10], found '%" PRIi64 "'.", int64_t);
	E(BACK_NO_NESTED                  ,152, 0, "Cannot nest .back directives.");
	E(BACK_BEFORE_START_OF_CODE       ,153, 0, "Cannot move instructions back before the start of the code.");
	W(BACK_CROSSES_BRANCH_TARGET      ,155, 0, ".back crosses branch target at address 0x%x. Code might not work.", unsigned);
	// .clone
	E(CLONE_REQUIRES_LABEL            ,160, 0, "The first argument to .clone must by a label, found '%s'.", const char*);
	E(CLONE_REQUIRES_INT              ,161, 0, "The second argument to .clone must by an integer constant, found '%s'.", const char*);
	E(CLONE_COUNT_OT_OUF_RANGE        ,162, 0, ".clone can only handle 0 to 3 instructions, found '%" PRIi64 "'.", int64_t);
	E(CLONE_BEHIND_END_OF_CODE        ,163, 0, "Cannot clone behind the end of the code.");
	W(CLONE_OF_BRANCH                 ,165, 0, "You should not clone branch instructions. (Instruction #%u)", unsigned);
	// .set & Co.
	E(SET_REQUIRES_VALUE_OR_ARG_LIST  ,170, 0, "Directive .set requires ', <value>' or '(<arguments>) <value>', found '%s'.", const char*);
	W(SET_FUNCTION_REDEFINED          ,171,10, "Redefinition of function '%s'. Previous definition at %s (%u).", const char*, const char*, uint16_t);
	E(SET_CONSTANT_REDEFINED          ,171,20, "Redefinition of constant '%s'. Previous definition at %s (%u).", const char*, const char*, uint16_t);
	W(UNSET_UNDEFINED_IDENTIFIER      ,172, 0, "Cannot unset '%s' because it has not yet been defined in the required context.", const char*);
	// .macro
	E(MACRO_ARG_EXPECTED              ,180, 0, "Expected macro argument identifier, found '%s'.", const char*);
	E(MACRO_NO_NESTED_DEFINITION      ,181, 0, "Cannot nest macro definitions. In definition of macro starting at %s (%u).", const char*, uint16_t);
	W(MACRO_REDEFINED                 ,182, 0, "Redefinition of macro '%s'. Previous definition at %s (%u).", const char*, const char*, uint16_t);
	E(MACRO_TOO_MANY_ARGS             ,185, 1, "Too many arguments for macro '%s', expected %u.", const char*, std::size_t);
	E(MACRO_TOO_FEW_ARGS              ,185, 2, "Too few arguments for macro '%s', expected %u, found %u.", const char*, std::size_t, std::size_t);
	E(MACRO_HAS_NO_ARGS               ,185, 3, "The macro '%s' does not take arguments.", const char*);
	F(INTERNAL_MACRO_ARG_EXPANSION    ,189, 0, "Internal error during macro argument expansion, found '%s'.", const char*);
	// .func
	E(FUNCTION_MISSING_RETURN         ,190, 1, "Failed to return a value in functional macro '%s'.", const char*);
	E(FUNCTION_2ND_EXPRESSION         ,190, 2, "Only one return expression allowed during expansion of functional macro '%s'.", const char*);
	E(FUNCTION_NO_LABEL_DEFINITION    ,191, 0, "Label definition not allowed in functional macro '%s'.", const char*);
	E(FUNCTION_REQUIRES_ARG_LIST      ,193, 0, "Expected '(' after function name.");
	E(FUNCTION_EXPECTED_COMMA_OR_BRC  ,194, 0, "Unexpected '%s' in argument list of function '%s'.", const char*, const char*);
	E(FUNCTION_TOO_MANY_ARGS          ,195, 1, "Too many arguments for function '%s', expected %u.", const char*, std::size_t);
	E(FUNCTION_TOO_FEW_ARGS           ,195, 2, "Too few arguments for function '%s', expected %u, found %u.", const char*, std::size_t, std::size_t);
	E(FUNCTION_HAS_NO_ARGS            ,195, 3, "Expected ')' because function '%s' has no arguments.", const char*);
	E(FUNCTION_INCLOMPLETE_EXPRESSION ,196, 1, "Function '%s' evaluated to an incomplete expression.", const char*);
	// .include
	E(INCLUDE_REQUIRES_FILENAME       ,200, 0, "Expected \"file-name\" or <file-name> after .include, found '%.30s'.", const char*);
	F(INTERNAL_INCLUDE_INCONSISTENCY  ,209, 1, "Inconsistent include invocation during pass 2, expected file '%s'.", const char*);
	F(INTERNAL_INCLUDE_INCONSISTENCY2 ,209, 2, "Inconsistent include invocation during pass 2.");
	// .assert
	E(ASSERTION_FAILED                ,210, 0, "Assertion failed.");
	// code generator
	W(INSTRUCTION_REQUIRES_ALIGNMENT  ,240, 0, "Used padding to enforce 64 bit alignment of GPU instruction.");
	// misc
	E(INPUT_FILE_NOT_FOUND            ,250, 0, "Cannot locate file '%s'.", const char*);
	F(INTERNAL_NO_ADD_FILE_IN_PASS2   ,255, 0, "Cannot add another file after pass 2 has been entered.");

	#undef F
	#undef E
	#undef W
	#undef I

	MAKE_MSGTEMPLATE_CONTAINER
} MSG;
