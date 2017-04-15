/*
 * WriteC.cpp
 *
 *  Created on: 19.03.2017
 *      Author: mueller
 */

#include "WriteC.h"
#include "DebugInfo.h"
#include "utils.h"

#include <vector>
#include <utility>
#include <algorithm>
#include <cctype>
#include <cstdio>


void WriteX::Write(const vector<uint64_t>& instructions, const DebugInfo& info)
{	// Invert label dictionary
	if (Options & O_WriteLabelComment)
		ScanLabels(info.Labels, info.GlobalsByName);
	auto lp = Labels.cbegin();
	for (auto cp = instructions.begin(); cp != instructions.end(); ++cp)
	{	unsigned offset = (cp - instructions.begin()) << 3;
		if (Options & O_WriteLabelComment)
			WriteLabelsAt(lp, offset);
		if (Options & O_WriteLocationComment)
			checkedfprintf(Target, "/* [0x%08x] */ ", offset);
		checkedfprintf(Target, "0x%08x, 0x%08x", (unsigned int)(*cp & 0xffffffffULL), (unsigned int)(*cp >> 32));
		if ((Options & O_WriteTrailingComma) || &*cp != &instructions.back())
			WriteChar(',');
		else if (Options & O_WriteSourceComment)
			WriteChar(' ');
		if (Options & O_WriteSourceComment)
		{	auto loc = info.LineNumbers[offset >> 3];
			WriteSourceLine(info.SourceFiles[loc.File].Name, loc.Line);
		} else
			WriteChar('\n');
	}
	if (Options & O_WriteLabelComment)
		WriteLabelsAt(lp, instructions.size() << 3);
}

void WriteX::ScanLabels(const DebugInfo::labels_t& labels, const DebugInfo::globals_t& globals)
{
	for (auto p = labels.begin(); p != labels.end(); ++p)
		if (!!p->Definition)
			Labels.emplace_back(p->Value | ((globals.find(p->Name) != globals.end()) << 31), p->Name);
	sort(Labels.begin(), Labels.end(),
		[](const labelref& a, const labelref& b) -> bool
		{	return (a.first & 0x7fffffff) < (b.first & 0x7fffffff);
		});
}

void WriteX::WriteLabelsAt(vector<labelref>::const_iterator& lp, unsigned offset)
{
	while (lp != Labels.cend())
	{	unsigned val = lp->first & 0x7fffffff;
		if (val > offset)
			break;
		if (val == offset)
		{	WriteString("// :");
			if (lp->first & 0x80000000)
				WriteChar(':');
			WriteString(lp->second);
			WriteChar('\n');
		}
		++lp;
	}
}

void WriteX::WriteSourceLine(const string& filename, unsigned line)
{	if (line)
	{	auto ret = FileData.emplace(filename, vector<string>());
		if (ret.second)
		{	// Cache miss => read the file
			FILEguard fh(fopen(filename.c_str(), "r"));
			if (fh)
			{	string s;
				while ((s = fgetstring(fh, 100)).length())
				{	if (s[s.length() - 1] != '\n')
						s += "...\n";
					ret.first->second.emplace_back(move(s));
				}
		}	}
		if (line <= ret.first->second.size())
		{	WriteString(" // ");
			WriteString(ret.first->second[line - 1]);
			return;
	}	}
	WriteChar('\n');
}


void WriteC::Write(const vector<uint64_t>& instructions, const DebugInfo& info, const char* filename, const string& headername)
{
	// Extract symbol name
	string symbolname = stripextension(strippath(filename));
	replacenonalphanum(symbolname);

	WriteWithTemplate((exepath + "../share/template/" + TemplateFile).c_str(), [=] (const string& placeholder) -> bool
		{	if (placeholder == "FILENAME")
			{	WriteString(filename);
			} else if (placeholder == "HEADERNAME")
			{	if (headername.length())
					WriteString(headername);
				else
					WriteString(stripextension(strippath(filename)) + ".h");
			} else if (placeholder == "SYMBOLNAME")
			{	WriteString(symbolname);
			} else if (placeholder == "INSTCOUNT2")
			{	checkedfprintf(Target, "%zu", instructions.size() << 1);
			} else if (placeholder == "HEXDATA")
			{	WriteX::Write(instructions, info);
			} else if (placeholder == "SYMBOLDEFS")
			{	LoadSymbols(info.GlobalsByName);
				WriteCDefines(symbolname);
			} else if (placeholder == "SYMBOLIMPORTS")
			{	LoadSymbols(info.GlobalsByName);
				if (WriteStdSymbol)
				{	WriteString("extern uint64_t ");
					WriteString(symbolname);
					WriteString("[];\n\n");
				}
				WriteCImports();
			} else if (placeholder == "SYMBOLPROXIES")
			{	WriteCConstProxies();
			} else
				return false;
			return true;
		} );
}

void WriteC::LoadSymbols(const DebugInfo::globals_t& globals)
{	// Order by type, value, name to keep output deterministic.
	OrderedSymbols.reserve(globals.size());
	for (const auto& sym : globals)
		OrderedSymbols.emplace_back(&sym);
	sort(OrderedSymbols.begin(), OrderedSymbols.end(),
		[](const pair<const string,exprValue>* a, const pair<const string,exprValue>* b) -> bool
		{	return a->second.Type != b->second.Type
				? a->second.Type < b->second.Type
				: a->second.iValue != b->second.iValue
					? a->second.iValue < b->second.iValue
					: a->first < b->first;
		});
}

void WriteC::WriteCDefines(const string& symbolname)
{
	for (auto symref : OrderedSymbols)
	{	string name = symref->first;
		replacenonalphanum(name);
		switch (symref->second.Type)
		{case V_INT:
			WriteString("#define ");
			WriteString(name);
			checkedfprintf(Target, " %" PRIi64, symref->second.iValue);
			if (symref->second.iValue < INT32_MIN || symref->second.iValue > UINT32_MAX)
				WriteString("LL");
			WriteChar('\n');
			break;
		 case V_FLOAT:
			WriteString("#define ");
			WriteString(name);
			checkedfprintf(Target, " %.16e\n", symref->second.fValue);
			break;
		 case V_LABEL:
			WriteString("#define ");
			WriteString(name);
			WriteString(" (");
			WriteString(symbolname);
			checkedfprintf(Target, " + %" PRIi64 ")\n", symref->second.iValue / sizeof(uint32_t));
		 default:;
		}
	}
}

void WriteC::WriteCImports()
{
	for (auto symref : OrderedSymbols)
	{	string name = symref->first;
		replacenonalphanum(name);
		switch (symref->second.Type)
		{case V_INT:
		 case V_FLOAT:
			WriteString("extern struct unspecified__ ");
			WriteString(name);
			WriteString(";\n");
			break;
		 case V_LABEL:
			WriteString("extern uint64_t ");
			WriteString(name);
			WriteString("[];\n");
		 default:;
		}
	}
}

void WriteC::WriteCConstProxies()
{
	for (auto symref : OrderedSymbols)
	{	string name = symref->first;
		replacenonalphanum(name);
		string type;
		switch (symref->second.Type)
		{default:
			continue;
		 case V_FLOAT:
			type = "float";
			break;
		 case V_INT:
			type = "uint32_t";
		}
		WriteString("static const ");
		WriteString(type);
		WriteChar(' ');
		WriteString(name);
		WriteString("__proxy__ = (");
		WriteString(type);
		WriteString(")&");
		WriteString(name);
		WriteString(";\n#define ");
		WriteString(name);
		WriteChar(' ');
		WriteString(name);
		WriteString("__proxy__\n");
	}
}
