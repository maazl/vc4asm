/*
 * WriteELF.cpp
 *
 *  Created on: 10.09.2015
 *      Author: mueller
 */

#ifdef __linux__
#include "WriteELF.h"
#include "expr.h"
#include "DebugInfo.h"

#include <string>
#include <cstring>
#include <cctype>


/* vc4asm ELF memory Layout:
 *
 * Elf32_Ehdr  ELF Header
 * Elf32_Shdr  [0] Section 0 entry
 * Elf32_Shdr  [1] Section header for section name string table
 * Elf32_Shdr  [2] Assembled QPU code
 * Elf32_Shdr  [3] External symbol table
 * Elf32_Shdr  [4] External symbol names
 * char[]      Section name string table
 * uint64_t[]  QPU code
 * Elf32_Sym[] Symbol table
 * char[]      Symbol names
 */
#define NUM_SECTS 5

const Elf32_Ehdr WriteELF::Hdr =
{	{	ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3,
		ELFCLASS32, ELFDATA2LSB, EV_CURRENT, ELFOSABI_NONE },
	ET_REL,
	40, //EM_ARM,
	EV_CURRENT,
	0,
	0,
	sizeof(Elf32_Ehdr),
	0x5000000, // flags
	sizeof(Elf32_Ehdr),
	0, 0,
	sizeof(Elf32_Shdr), NUM_SECTS,
	1
};

const Elf32_Shdr WriteELF::Sect0 = {0};

const char WriteELF::SNST[] =
	"\0.shstrtab" // [1]
	"\0.rodata"   // [11]
	"\0.symtab"   // [19]
	"\0.strtab";  // [27]

const Elf32_Shdr WriteELF::SectSNST =
{	1, // .shstrtab
	SHT_STRTAB,
	0, // flags
	0,
	sizeof(Elf32_Ehdr) + NUM_SECTS * sizeof(Elf32_Shdr),
	sizeof SNST,
	0, 0,
	1, 0
};

const Elf32_Shdr WriteELF::SectCode =
{	11, // .rodata
	SHT_PROGBITS,
	SHF_ALLOC,
	0,
	sizeof(Elf32_Ehdr) + NUM_SECTS * sizeof(Elf32_Shdr) + sizeof SNST,
	0, // size, to be patched!
	0, 0,
	sizeof(uint32_t), sizeof(uint32_t)
};

const Elf32_Shdr WriteELF::SectSym =
{	19, // .symtab
	SHT_SYMTAB,
	0, // flags
	0,
	sizeof(Elf32_Ehdr) + NUM_SECTS * sizeof(Elf32_Shdr) + sizeof SNST, // to be patched!
	0, // size, to be patched!
	4, // [.strtab]
	2, // only 2 local symbols: undef, file
	sizeof(uint32_t), sizeof(Elf32_Sym)
};

const Elf32_Shdr WriteELF::SectSymST =
{	27, // .strtab
	SHT_STRTAB,
	0, // flags
	0,
	sizeof(Elf32_Ehdr) + NUM_SECTS * sizeof(Elf32_Shdr) + sizeof SNST, // to be patched!
	0, // size, to be patched!
	0, 0,
	1, 0
};

const Elf32_Sym WriteELF::Sym0 = {0};


WriteELF::WriteELF()
:	Target(NULL)
,	NoStandardSymbols(false)
{	Symbols.emplace_back(Sym0);
}

void WriteELF::Write(const vector<uint64_t>& instructions, const DebugInfo& info, const char* filename)
{
	// File name symbol
	{	auto& sym = AddSymbol(filename);
		sym.st_info = (STB_LOCAL<<4) | STT_FILE;
		sym.st_shndx = SHN_ABS;
	}

	size_t code_size = instructions.size() * sizeof (uint64_t);

	// Code fragment symbol, name = file name
	if (!NoStandardSymbols)
	{	auto cp = strrchr(filename, '/');
		if (cp)
			filename = cp+1;
		cp = strrchr(filename, '.');
		string name;
		if (cp)
			name.assign(filename, cp);
		else
			name = filename;
		// Replace non-word chars
		for (auto& c : name)
		{	if (!isalnum(c))
				c = '_';
		}
		AddGlobalSymbol(name).st_size = code_size;
		// End Symbol
		name.append("_end");
		AddGlobalSymbol(name).st_value = code_size;
		// Size symbol
		name.erase(name.size() - 4, 4);
		name.append("_size");
		auto& sym = AddGlobalSymbol(name);
		sym.st_shndx = SHN_ABS;
		sym.st_value = code_size;
	}

	// Export global symbol table
	for (auto sym : info.GlobalsByName)
	{	auto& value = AddGlobalSymbol(sym.first);
		value.st_value = (Elf32_Addr)sym.second.iValue;
		if (sym.second.Type == V_INT)
			value.st_shndx = SHN_ABS;
	}

	// write header
	WriteRaw(Hdr);
	// write section table
	WriteRaw(Sect0);
	WriteRaw(SectSNST);
	Elf32_Shdr sect = SectCode;
	sect.sh_size = code_size;
	WriteRaw(sect);
	sect = SectSym;
	sect.sh_offset += code_size;
	size_t sym_size = Symbols.size() * sizeof(Elf32_Sym);
	sect.sh_size = sym_size;
	WriteRaw(sect);
	sect = SectSymST;
	sect.sh_offset += code_size + sym_size;
	sect.sh_size = SymbolNames.size() + 1;
	WriteRaw(sect);
	// write section names
	WriteRaw(SNST);
	// write code
	fwrite(&instructions.front(), sizeof(uint64_t), instructions.size(), Target);
	// write symbol table
	fwrite(&Symbols.front(), sizeof(Elf32_Sym), Symbols.size(), Target);
	// write symbol names
	fwrite(SymbolNames.c_str(), 1, SymbolNames.size() + 1, Target);
}

Elf32_Sym& WriteELF::AddSymbol(const string& name)
{
	Symbols.emplace_back(Sym0);
	auto& sym = Symbols.back();
	SymbolNames.push_back('\0');
	sym.st_name = SymbolNames.size();
	SymbolNames.append(name);
	return sym;
}

Elf32_Sym& WriteELF::AddGlobalSymbol(const string& name)
{	auto& sym = AddSymbol(name);
	sym.st_info = (STB_GLOBAL<<4) | STT_OBJECT;
	sym.st_shndx = 2; // [.rodata]
	return sym;
}

#endif // __linux__