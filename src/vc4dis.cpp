#include "Disassembler.h"
#include "WriteQasm.h"
#include "Validator.h"

#include <cinttypes>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <getopt.h>
#include <vector>
#include <sys/param.h>

using namespace std;


#include "vc4dis.MSG.h"

int hexinput = 0;


#if (defined(__BIG_ENDIAN__) && __BIG_ENDIAN__) || (defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN)
/// Byte swap
static inline uint64_t swap_uint64(uint64_t x)
{	x = x << 32 | x >> 32;
	x = (x & 0x0000FFFF0000FFFFULL) << 16 | (x & 0xFFFF0000FFFF0000ULL) >> 16;
	return (x & 0x00FF00FF00FF00FFULL) << 8  | (x & 0xFF00FF00FF00FF00ULL) >> 8;
}
#endif

static void file_load_bin(const char *filename, vector<uint64_t>& memory)
{	FILEguard f(checkedopen(filename, "rb"));
	if (fseek(f, 0, SEEK_END) == 0)
	{	long size = ftell(f);
		if (size % sizeof(uint64_t))
			MSG.BINARY_FILE_ODD_SIZED.toMsg(size, filename).print();
		size /= sizeof(uint64_t);
		size_t oldsize = memory.size();
		if (size > 0)
			memory.resize(oldsize + size);
		fseek(f, 0, SEEK_SET);
		memory.resize(oldsize + fread(&memory[oldsize], sizeof(uint64_t), size, f));
	} else
	{	size_t count = 8192;
		for (;;)
		{	size_t oldsize = memory.size();
			memory.resize(oldsize + count);
			count = fread(&memory[oldsize], sizeof(uint64_t), 8192, f);
			if (count < 8192)
			{	memory.resize(oldsize + count);
				break;
			}
		}
	}
	#if (defined(__BIG_ENDIAN__) && __BIG_ENDIAN__) || (defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN)
	for (auto& i : memory)
		i = swap_uint64(i);
	#endif
}

static void file_load_x32(const char *filename, vector<uint64_t>& memory)
{	FILEguard f(checkedopen(filename, "r"));
	uint32_t value1, value2;
	while (((void)fscanf(f, "%*[ \t]"), (void)fscanf(f, "//%*[^\n]"), fscanf(f, "%" SCNx32 ",", &value1)) == 1)
	{	if (fscanf(f, "%" SCNx32 ",", &value2) != 1)
		{	if (feof(f))
				throw MSG.HEX_FILE_ODD_SIZED.toMsg(filename);
			break;
		}
		memory.push_back(value1 | (uint64_t)value2 << 32);
	}
	if (!feof(f))
	{	char buf[10];
		*buf = 0;
		(void)fscanf(f, "%9s", buf);
		buf[9] = 0;
		throw MSG.HEX_FILE_NOT_PARSABLE.toMsg(filename, buf);
	}
}

static void file_load_x64(const char *filename, vector<uint64_t>& memory)
{	FILEguard f(checkedopen(filename, "r"));
	uint64_t value;
	while (((void)fscanf(f, "%*[ \t]"), (void)fscanf(f, "//%*[^\n]"), fscanf(f, "%" SCNx64 ",", &value)) == 1)
		memory.push_back(value);
	if (!feof(f))
	{	char buf[20];
		*buf = 0;
		(void)fscanf(f, "%18s", buf);
		buf[19] = 0;
		throw MSG.HEX_FILE_NOT_PARSABLE.toMsg(filename, buf);
	}
}

int main(int argc, char * argv[])
{	// parse options
	bool check = false;
	Disassembler dis;
	const char* writeD = NULL;
	WriteQasm::comments opt = WriteQasm::C_NONE;
	unsigned base = 0;

	try
	{
		int c;
		while ((c = getopt(argc, argv, "x::MFv::b:o:V")) != -1)
		{	switch (c)
			{case 'x':
				if (!optarg || (hexinput = atoi(optarg)) == 0)
					hexinput = 32;
				break;
			 case 'M':
				dis.Options &= ~Disassembler::O_UseMOV; break;
			 case 'F':
				dis.Options &= ~Disassembler::O_UseFloat; break;
			 case 'v':
				opt = optarg && atoi(optarg) >= 2 ? WriteQasm::C_Fields|WriteQasm::C_Hex : WriteQasm::C_Hex; break;
			 case 'b':
				base = atol(optarg); break;
			 case 'o':
				writeD = optarg; break;
			 case 'V':
				check = true; break;
			}
		}
		argv += optind;

		if (!*argv) {
			fputs("vc4dis V0.3\n"
				"Usage: vc4dis [-x[32|64]] [-M] [-F] [-v] [-b <addr>] [-o <out_file>] [-V] <code_file(s)>\n"
				" -x    Hexadecimal input, comma separated (rather than binary).\n"
				" -x64  64 bit formatted hexadecimal input.\n"
				" -M    Do not print simple ALU instructions and load immediate as mov.\n"
				" -F    Print floating point constants as hexadecimal.\n"
				" -v    Binary code and offset as comment behind each line.\n"
				" -v2   Write internal instruction field as comment behind every line also.\n"
				" -b<addr> base address (only for comments and labels).\n"
				" -o<file> Write output to this file rather than stdout.\n"
				" -V    Run instruction verifier and print warnings about suspicious code.\n"
				, stderr);
			return 1;
		}

		// read file(s)
		vector<uint64_t> instructions;
		do
		{	MSG.DISASSEMBLING.toMsg(*argv).print();
			switch (hexinput)
			{default:
				throw MSG.INVALID_OPTION_ARGUMENT.toMsg(hexinput, 'x');
			 case 32:
				file_load_x32(*argv, instructions);
				break;
			 case 64:
				file_load_x64(*argv, instructions);
				break;
			 case 0:
				file_load_bin(*argv, instructions);
				break;
			}
			if (instructions.size() == 0)
			{	MSG.FILE_EMPTY.toMsg(*argv).print();
				continue;
			}
		} while (*++argv);

		// disassemble
		{	FILEguard outf(writeD ? checkedopen(writeD, "w") : NULL);
			WriteQasm(outf ? (FILE*)outf : stdout, dis, opt)
				.Write(instructions, base);
		}

		// run validator
		if (check)
		{	Validator v;
			v.Instructions = &instructions;
			v.Validate();
		}

	} catch (const Message& msg)
	{	msg.print();
		return msg.ID.Value;
	}
}
