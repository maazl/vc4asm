#include "Disassembler.h"
#include "ReadX.h"
#include "ReadBin.h"
#include "WriteQasm.h"
#include "Validator.h"

#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <vector>
#include <sys/param.h>
#include "ReadX.h"

using namespace std;


#include "vc4dis.MSG.h"


int main(int argc, char * argv[])
{	// parse options
	bool check = false;
	Disassembler dis;
	const char* writeD = NULL;
	WriteQasm::comments opt = WriteQasm::C_NONE;
	unsigned base = 0;
	int hexinput = 0;

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
			size_t before = instructions.size();
			switch (hexinput)
			{default:
				throw MSG.INVALID_OPTION_ARGUMENT.toMsg(hexinput, 'x');
			 case 32:
				ReadX32(*argv).Read(instructions);
				break;
			 case 64:
				ReadX64(*argv).Read(instructions);
				break;
			 case 0:
				ReadBin(*argv).Read(instructions);
				break;
			}
			if (instructions.size() == before)
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
