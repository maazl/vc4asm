#ifndef DEBUGINFO_H_
#define DEBUGINFO_H_

#include "expr.h"

#include <string>
#include <unordered_map>
#include <cinttypes>

using namespace std;


struct DebugInfo
{
	/// Reference to a line of assembler code (for messages).
	struct location
	{	uint16_t       File; ///< Source file containing the line. This is only an index into SourceFiles.
		uint16_t       Line; ///< Line number in the source file
		location()     : File(0), Line(0) {}///< create uninitialized instance, assign File and Line by yourself
		bool operator !() const { return !Line; }///< Is this instance not initialized, i.e. Line == 0
	};

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

	/// @brief List of export symbol definitions. They are not necessarily labels.
	/// @details name => (value, type)
	typedef unordered_map<string,exprValue> globals_t;

	/// Source file entry
	struct file
	{	const string   Name; ///< Name of the source file
		const location Parent;///< Parent location from which the file has been included, optionally initial if command line argument.
		file(const string& name) : Name(name) {}
		file(const string& name, const location& parent) : Name(name), Parent(parent) {}
	};

	/// Segment flags (bit vector)
	enum SegFlags : uint8_t
	{	SF_None        = 0,   ///< No flags
		SF_Code        = 1,   ///< Ordinary code segment (default)
		SF_Data        = 2    ///< Data segment, i.e. created by the a .data directive.
	};
	/// @brief Segment entry
	/// @details A segment is a consecutive run of elements in the instructions array
	/// that have common properties.
	struct segment
	{	unsigned       Start; ///< Start index of the segment properties as index into Instructions.
		uint8_t        Flags; ///< Segment flags
		segment() {}
		segment(int start, uint8_t flags) : Start(start), Flags(flags) {}
	};

	/// List of label definitions by label ID (= vector index).
	labels_t         Labels;
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
	/// Segment properties ordered by Start address.
	vector<segment>  Segments;
};

#endif
