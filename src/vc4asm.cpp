#include "Parser.h"
#include "Validator.h"
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

static const char CPPTemplate[] = ",\n0x%08lx, 0x%08lx";

int main(int argc, char **argv)
{
	const char* writeBIN = NULL;
	const char* writeCPP = NULL;
	const char* writeCPP2 = NULL;
	const char* writeELF = NULL;
	const char* writeELF2 = NULL;
	const char* writePRE = NULL;
	bool check = false;

	Parser parser;

	int c;
	while ((c = getopt(argc, argv, "o:c:e:C:E:I:V")) != -1)
	{	switch (c)
		{case 'o':
			writeBIN = optarg; break;
		 case 'c':
			writeCPP = optarg; break;
		 case 'C':
			writeCPP2 = optarg; break;
#ifdef __linux__
		 case 'e':
			writeELF = optarg; break;
		 case 'E':
			writeELF2 = optarg; break;
#endif
		 case 'I':
			parser.IncludePaths.push_back(optarg); break;
		 case 'V':
			check = true; break;
		 case 'P':
			writePRE = optarg; break;
		}
	}

	if (!writeBIN && !writeCPP && !writeCPP2 && !writePRE && !writeELF) {
		fputs("vc4asm V0.2.2\n"
			"Usage: vc4asm [-o <bin-output>] [-{c|C} <c-output>] [-V] <qasm-file(s)>\n"
			" -o<file> Binary output file.\n"
			" -c<file> C output file with trailing ','.\n"
			" -C<file> C output file withOUT trailing ','.\n"
#ifdef __linux__
			" -e<file> Linux ELF output file.\n"
			" -E<file> Linux ELF output file without predefined symbols.\n"
#endif
			" -I<path> Add search path for .include <...>\n"
			" -V       Run instruction verifier and print warnings about suspicious code.\n"
			, stderr);
		return 1;
	}

	if (writePRE)
	{	parser.Preprocessed = fopen(writePRE, "wt");
		if (parser.Preprocessed == NULL)
		{	fprintf(stderr, "Failed to open %s for writing.", writePRE);
			return -1;
		}
	}

	try
	{	while (optind < argc)
		{	parser.ParseFile(argv[optind]);
			++optind;
		}
		parser.EnsurePass2();
		if (!parser.Success)
			throw string("Aborted because of earlier errors.");

		if (check)
		{	Validator v;
			v.Instructions = &parser.Instructions;
			v.Info = &parser;
			v.Validate();
		}

		if (writeCPP)
		{	FILE* of = fopen(writeCPP, "wt");
			if (of == NULL)
			{	fprintf(stderr, "Failed to open %s for writing.", writeCPP);
				return -1;
			}

			const char* tpl = CPPTemplate + 2; // no ,\n in the first line
			for (auto code : parser.Instructions)
			{	fprintf(of, tpl, (unsigned long)(code & 0xffffffffULL), (unsigned long)(code >> 32) );
				tpl = CPPTemplate;
			}
			fputs(",\n", of);
			fclose(of);
		}
		if (writeCPP2)
		{	FILE* of = fopen(writeCPP2, "wt");
			if (of == NULL)
			{	fprintf(stderr, "Failed to open %s for writing.", writeCPP2);
				return -1;
			}

			const char* tpl = CPPTemplate + 2; // no ,\n in the first line
			for (auto code : parser.Instructions)
			{	fprintf(of, tpl, (unsigned long)(code & 0xffffffffULL), (unsigned long)(code >> 32) );
				tpl = CPPTemplate;
			}
			fputc('\n', of);
			fclose(of);
		}

		if (writeBIN)
		{
			/*#if (defined(__BIG_ENDIAN__) && __BIG_ENDIAN__) || (defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN)
			for (auto& i : memory)
				i = swap_uint64(i);
			#endif*/
			FILE* of = fopen(writeBIN, "wb");
			if (of == NULL)
			{	fprintf(stderr, "Failed to open %s for writing.", writeBIN);
				return -1;
			}
			fwrite(&*parser.Instructions.begin(), sizeof(uint64_t), parser.Instructions.size(), of);
			fclose(of);
		}

#ifdef __linux__
		if (writeELF)
		{	WriteELF we;
			we.Target = fopen(writeELF, "wb");
			if (we.Target == NULL)
			{	fprintf(stderr, "Failed to open %s for writing.", writeELF);
				return -1;
			}
			we.Write(parser.Instructions, parser, writeELF);
			fclose(we.Target);
		}
		if (writeELF2)
		{	WriteELF we;
			we.Target = fopen(writeELF2, "wb");
			if (we.Target == NULL)
			{	fprintf(stderr, "Failed to open %s for writing.", writeELF2);
				return -1;
			}
			we.NoStandardSymbols = true;
			we.Write(parser.Instructions, parser, writeELF);
			fclose(we.Target);
		}
#endif
	} catch (const string& msg)
	{	fputs(msg.c_str(), stderr);
	  fputc('\n', stderr);
		return 1;
	}

	return !parser.Success;
}
