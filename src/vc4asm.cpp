#include "Parser.h"
#include "Validator.h"

#include <cstdio>
#include <getopt.h>

using namespace std;


/// Byte swap
static inline uint64_t swap_uint64(uint64_t x)
{	x = x << 32 | x >> 32;
	x = (x & 0x0000FFFF0000FFFFULL) << 16 | (x & 0xFFFF0000FFFF0000ULL) >> 16;
	return (x & 0x00FF00FF00FF00FFULL) << 8  | (x & 0xFF00FF00FF00FF00ULL) >> 8;
}

static const char CPPTemplate[] = ",\n0x%08lx, 0x%08lx";

int main(int argc, char **argv)
{
	const char* outfname = NULL;
	const char* writeCPP = NULL;
	const char* writePRE = NULL;
	bool check = false;

	int c;
	while ((c = getopt(argc, argv, "o:c:E:C")) != -1)
	{	switch (c)
		{case 'o':
			outfname = optarg; break;
		 case 'c':
			writeCPP = optarg; break;
		 case 'C':
			check = true; break;
		 case 'E':
			writePRE = optarg; break;
		}
	}

	if (!outfname && !writeCPP && !writePRE) {
		fputs("Usage: vc4asm [-o <bin-output>] [-c <c-output>] [-E <preprocessed>] <qasm-file>\n", stderr);
		return 1;
	}

	Parser parser;

	if (writePRE)
	{
		parser.Preprocessed = fopen(writePRE, "wt");
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
		if (!parser.Success)
			throw string("Aborted because of earlier errors.");
	} catch (const string& msg)
	{	fputs(msg.c_str(), stderr);
	  fputc('\n', stderr);
		return 1;
	}

	if (check)
		Validator().Validate(parser.GetInstructions());

	if (writeCPP)
	{	FILE* of = fopen(writeCPP, "wt");
		if (of == NULL)
		{	fprintf(stderr, "Failed to open %s for writing.", writeCPP);
			return -1;
		}

		const char* tpl = CPPTemplate + 2; // no ,\n in the first line
		for (auto code : parser.GetInstructions())
		{	fprintf(of, tpl, (unsigned long)(code & 0xffffffffULL), (unsigned long)(code >> 32) );
			tpl = CPPTemplate;
		}
		fputc('\n', of);
		fclose(of);
	}

	if (outfname)
	{
		#if (defined(__BIG_ENDIAN__) && __BIG_ENDIAN__) || (defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN)
		for (auto& i : memory)
			i = swap_uint64(i);
		#endif
		FILE* of = fopen(outfname, "wb");
		if (of == NULL)
		{	fprintf(stderr, "Failed to open %s for writing.", outfname);
			return -1;
		}
		fwrite(&*parser.GetInstructions().begin(), sizeof(uint64_t), parser.GetInstructions().size(), of);
		fclose(of);
	}

	return 0;
}
