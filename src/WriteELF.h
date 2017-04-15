/*
 * WriteELF.h
 *
 *  Created on: 10.09.2015
 *      Author: mueller
 */

#ifndef WRITEELF_H_
#define WRITEELF_H_
#ifdef __linux__

#include "WriterBase.h"

#include <linux/elf.h>
#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <string>
using namespace std;


class DebugInfo;

/// Writer for Linux ELF object output.
class WriteELF : public WriterBase
{	bool NoStandardSymbols;
 public:
	WriteELF(FILE* target, bool nostdsymbols = false);
	void Write(const vector<uint64_t>& instructions, const DebugInfo& info, const char* filename);

 private: // Templates for ELF creation
	static const Elf32_Ehdr Hdr;      ///< ELF header
	static const Elf32_Shdr Sect0;    ///< Section 0 entry
	static const Elf32_Shdr SectSNST; ///< Section header string table
	static const Elf32_Shdr SectCode; ///< Section header for QPU code
	static const Elf32_Shdr SectSym;  ///< Section header for symbol table
	static const Elf32_Shdr SectSymST;///< Section header for symbol table names
	static const char SNST[];         ///< Section name string table
	static const Elf32_Sym  Sym0;     ///< Undefined symbol entry
 private:
	vector<Elf32_Sym> Symbols;
	string            SymbolNames;
 private:
	Elf32_Sym& AddSymbol(const string& name);
	Elf32_Sym& AddGlobalSymbol(const string& name);
};

#endif // __linux__
#endif // WRITEELF_H_
