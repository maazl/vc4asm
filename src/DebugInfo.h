#ifndef DEBUGINFO_H_
#define DEBUGINFO_H_

#include <string>
#include <unordered_map>

using namespace std;


struct DebugInfo
{
	/// @brief List of export symbol definitions.
	/// @details name => (value, type)
	typedef unordered_map<string,exprValue> globals_t;

	/// Reference to a line of assembler code (for messages).
	struct location
	{	uint16_t       File; ///< Source file containing the line. This is only an index into SourceFiles.
		uint16_t       Line; ///< Line number in the source file
		location()     : File(0), Line(0) {}///< create uninitialized instance, assign File and Line by yourself
		bool operator !() const { return !Line; }///< Is this instance not initialized, i.e. Line == 0
	};

	/// Source file entry
	struct file
	{	const string   Name; ///< Name of the source file
		const location Parent;///< Parent location from which the file has been included, optionally initial if command line argument.
		file(const string& name) : Name(name) {}
		file(const string& name, const location& parent) : Name(name), Parent(parent) {}
	};

	/// @brief Global label definitions.
	/// @details List of labels that should be exported in ELF output.
	/// This is only valid after EnsurePass2 has been called.
	globals_t        GlobalsByName;
	/// @brief Files to be parsed in order of appearance.
	/// @details They are effectively filled by calls to ParseFile in pass 1.
	vector<file>     SourceFiles;
	/// @brief Debug Info: locations per instruction in Instructions.
	/// @details The index of each entry corresponds to the indices in Instruction.
	/// This is only valid after EnsurePass2 has been called.
	vector<location> LineNumbers;
};

#endif
