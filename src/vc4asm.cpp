#include "Parser.h"
#include "Validator.h"
#include "WriteC.h"
#ifdef __linux__
#include "WriteELF.h"
#endif
#include <cstdio>
#include <getopt.h>

using namespace std;


#if (defined(__BIG_ENDIAN__) && __BIG_ENDIAN__) || (defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN)
/// Byte swap
static inline uint64_t swap_uint64(uint64_t x)
{	x = x << 32 | x >> 32;
	x = (x & 0x0000FFFF0000FFFFULL) << 16 | (x & 0xFFFF0000FFFF0000ULL) >> 16;
	return (x & 0x00FF00FF00FF00FFULL) << 8  | (x & 0xFF00FF00FF00FF00ULL) >> 8;
}
#endif

int main(int argc, char **argv)
{
	const char* writeBIN = NULL;
	const char* writeC = NULL;
	const char* writeX = NULL;
	const char* writeH = NULL;
	const char* writeH2 = NULL;
	const char* writeELF = NULL;
	const char* writePRE = NULL;
	bool check = false;
	bool comments = false;
	bool nopredefsym = false;

	Parser parser;

	setexepath(argv[0]);

	// add default include path with low precedence
	parser.IncludePaths.emplace_back(exepath + "../share/include/");

	try
	{	int c;
		while ((c = getopt(argc, argv, "o:c:e:C:h:H:vI:i:VsQP:")) != -1)
		{	switch (c)
			{case 'o':
				writeBIN = optarg; break;
			 case 'c':
				writeC = optarg; break;
			 case 'C':
				writeX = optarg; break;
			 case 'h':
				writeH = optarg; break;
			 case 'H':
				writeH2 = optarg; break;
			#ifdef __linux__
			 case 'e':
				writeELF = optarg; break;
			#endif
			 case 'v':
				comments = true; break;
			 case 'I':
				parser.IncludePaths.push_back(optarg); break;
			 case 'V':
				check = true; break;
			 case 's':
				nopredefsym = true; break;
			 case 'Q':
				parser.OperationMode = Parser::IGNOREERRORS; break;
			 case 'P':
				writePRE = optarg; break;
			 case 'i':
				{	string file = parser.FindIncludePath(optarg);
					if (file.length() == 0)
						throw stringf("\"%s\" not found in include path.\n", optarg);
					parser.ParseFile(file);
					break;
				}
			}
		}

		if (!writeBIN && !writeC && !writeX && !writeH && !writeH2 && !writePRE && !writeELF && parser.OperationMode != Parser::PASS1ONLY)
		{	fputs("vc4asm V0.3\n"
				"Usage: vc4asm [options...] <qasm-file(s)>\n"
				" -o<file> Binary output file.\n"
				" -C<file> C output with hex data only.\n"
				" -c<file> C output with matching header, recommends -H.\n"
				" -h<file> C header output for use with -e or -c.\n"
				#ifdef __linux__
				" -H<file> C header output w/o inline symbol values for use with -e only.\n"
				" -e<file> Linux ELF output file.\n"
				#endif
				" -v       Write comments to C output\n"
				" -s       Do not generate predefined symbols with file name.\n"
				" -I<path> Add search path for .include <...>\n"
				" -i<file> Search include path for command line arguments as well.\n"
				" -V       Run instruction verifier and print warnings about suspicious code.\n"
				"Examples:\n"
				" vc4asm -V -C smitest.hex -i vc4.qinc smitest.qasm\n"
				" vc4asm -v -c rpi_shader.c -H rpi_shader.h -i vc4.qinc rpi_shader.qasm\n"
				, stderr);
			return 1;
		}

		if (writePRE)
			parser.Preprocessed = checkedopen(writePRE, "wt");

		// Pass 1
		while (optind < argc)
		{	parser.ParseFile(argv[optind]);
			++optind;
		}
		switch (parser.OperationMode)
		{case Parser::PASS1ONLY:
			return !parser.Success;
		 case Parser::NORMAL:
			if (!parser.Success)
				throw string("Aborted because of earlier errors.");
		 default:;
		}
		// Pass 2
		parser.EnsurePass2();
		// Validate
		if (check)
		{	Validator v;
			v.Instructions = &parser.Instructions;
			v.Info = &parser;
			v.Validate();
		}

		if (!parser.Success && parser.OperationMode != Parser::IGNOREERRORS)
			throw string("Aborted because of earlier errors.");

		// Write results
		if (writeH)
			WriteC(FILEguard(checkedopen(writeH, "wt")), "template.h", WriteX::O_NONE, true)
				.Write(parser.Instructions, parser, writeH);
		if (writeH2)
			WriteC(FILEguard(checkedopen(writeH2, "wt")), "template2.h", WriteX::O_NONE, !nopredefsym)
				.Write(parser.Instructions, parser, writeH2);
		if (writeC)
			WriteC(FILEguard(checkedopen(writeC, "wt")), "template.c", comments ? WriteX::O_WriteLabelComment|WriteX::O_WriteLocationComment|WriteX::O_WriteSourceComment : WriteX::O_NONE, true)
				.Write(parser.Instructions, parser, writeC, writeH ? strippath(writeH) : string());
		if (writeX)
			WriteX(FILEguard(checkedopen(writeX, "wt")), comments ? WriteX::O_WriteLabelComment|WriteX::O_WriteLocationComment|WriteX::O_WriteSourceComment : WriteX::O_NONE)
				.Write(parser.Instructions, parser);
		#ifdef __linux__
		if (writeELF)
			WriteELF(FILEguard(checkedopen(writeELF, "wb")), nopredefsym)
				.Write(parser.Instructions, parser, writeELF);
		#endif
		if (writeBIN)
		{
			#if (defined(__BIG_ENDIAN__) && __BIG_ENDIAN__) || (defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN)
			for (auto& i : parser.Instructions)
				i = swap_uint64(i);
			#endif
			FILEguard of(checkedopen(writeBIN, "wb"));
			fwrite(&*parser.Instructions.begin(), sizeof(uint64_t), parser.Instructions.size(), of);
		}
	} catch (const string& msg)
	{	fputs(msg.c_str(), stderr);
		fputc('\n', stderr);
		return 1;
	}

	return !parser.Success;
}
