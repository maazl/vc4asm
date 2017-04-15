/*
 * WriteC.h
 *
 *  Created on: 19.03.2017
 *      Author: mueller
 */

#ifndef WRITEC_H_
#define WRITEC_H_

#include "WriterBase.h"
#include "DebugInfo.h"

#include <vector>
#include <utility>
#include <unordered_map>
#include <cstdint>
using namespace std;


/// Writer for C compatible hexadecimal output.
class WriteX : public WriterBase
{public:
	enum options : unsigned char    ///< Options for code generation.
	{	O_NONE                 = 0x00 ///< None of the options below.
	,	O_WriteLocationComment = 0x01 ///< Decorate the generated code with the hexadecimal offset at the beginning of each line.
	,	O_WriteLabelComment    = 0x02 ///< Decorate the generated code with label comments.
	,	O_WriteSourceComment   = 0x04 ///< Decorate the generated code with comments containing the source lines which generated the instructions.
	,	O_WriteTrailingComma   = 0x08 ///< Write a trailing comma to the
	};
	CLASSFLAGSENUM(options, unsigned char);
 private:
	const options Options;
	typedef pair<int,string> labelref;
	vector<labelref> Labels;
	/// Cache with files read.
	unordered_map<string,vector<string>> FileData;
 public:
	/// Setup hex code writer.
	/// @param target Target stream to write the result to.
	/// @param options Decorate the hex output with comments.
	WriteX(FILE* target, options opt)
		: WriterBase(target), Options(opt) {}
	/// Write hex code with binary GPU instructions to Target.
	/// @param instructions binary code.
	/// @param additional meta information for code decoration.
	void Write(const vector<uint64_t>& instructions, const DebugInfo& info);
 private:
	void ScanLabels(const DebugInfo::labels_t& labels, const DebugInfo::globals_t& globals);
	void WriteLabelsAt(vector<labelref>::const_iterator& lp, unsigned offset);
	void WriteSourceLine(const string& filename, unsigned line);
};

/// Writer for C source or header file from template file.
class WriteC : public WriteX
{	const char* const TemplateFile;
	const bool WriteStdSymbol;
	vector<const pair<const string,exprValue>*> OrderedSymbols;
 public:
	/// Setup C code writer.
	/// @param target Target stream to write the result to.
	/// @param tmpl Template file name (without path), e.g. "template.c".
	/// @param options Decorate the hex output with comments.
	/// @param writestdsym Refer to standard symbol with the mane of the entire file.
	/// Currently only used for C++ header output.
	WriteC(FILE* target, const char* tmpl, options options, bool writestdsym) : WriteX(target, options), TemplateFile(tmpl), WriteStdSymbol(writestdsym) {}
	/// Write C source code file with binary GPU instructions to Target.
	/// @param instructions binary code.
	/// @param additional meta information for code decoration.
	/// @param filename Name of the target file (for self references).
	/// @param filename Name of the matching header file (without path) - only required if template refers to it.
	void Write(const vector<uint64_t>& instructions, const DebugInfo& info, const char* filename, const string& headername = string());
 private:
	void LoadSymbols(const DebugInfo::globals_t& globals);
	void WriteCDefines(const string& symbolname);
	void WriteCImports();
	void WriteCConstProxies();
};

#endif // WRITEC_H_
