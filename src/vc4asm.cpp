#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <map>
#include <vector>
#include <assert.h>
#include <errno.h>
#include <sstream>
#include <algorithm>
#include <getopt.h>

#include "Parser.h"

using namespace std;

enum token_t {
    END=-1,
    WORD,
    DOT,
    COMMA,
    SEMI,
    COLON,
};

struct QPUreg {
    enum { A, B, ACCUM, SMALL } file;
    int num;
};

struct relocation {
    string label;
    int pc;
};

struct context {
    const char *stream;
    map<string, int> labels;
    int pc;
    vector<relocation> relocations;
};


string printRegister(const QPUreg& reg)
{
    char buffer[32];
    if (reg.file == QPUreg::A || reg.file == QPUreg::B) {
        snprintf(buffer, 32, "r%c%d", (reg.file == QPUreg::A) ? 'a' : 'b',
                                      reg.num);
    }
    else if (reg.file == QPUreg::ACCUM) {
        snprintf(buffer, 32, "r%d", reg.num);
    }
    else {
        snprintf(buffer, 32, ".0x%x.", reg.num);
    }

    return buffer;
}

uint64_t assembleSEMA(context& ctx, string word)
{

    uint64_t ins = (uint64_t)0x74 << 57;

    /*string token_str;
    token_t tok = nextToken(ctx.stream, token_str, &ctx.stream);
    if (tok != WORD) {
        cerr << "semaphore instruction expecting down/up or acquire/release" << endl;
        return -1;
    }

    uint8_t sa = 0;             // up
    if (token_str == "down" || token_str == "acquire")
        sa = 1;

    tok = nextToken(ctx.stream, token_str, &ctx.stream);
    if (tok != COMMA)   return -1;
    tok = nextToken(ctx.stream, token_str, &ctx.stream);
    uint32_t imm = parseSmallImmediate(token_str);
    if (imm < 0) {
        cerr << "semaphore out of range" << endl;
        return -1;
    }
    // cond_add, cond_mul = NEVER, ws, sf = false
    ins |= (uint64_t)39 << 38;          // waddr_add
    ins |= (uint64_t)39 << 32;          // waddr_mul
    ins |= sa << 4;
    ins |= (uint8_t)imm;

    cout << "Assembling SEMAPHORE instruction (" << imm << "), " << (int)sa << endl;*/

    return ins;
}


static const char CPPTemplate[] = ",\n0x%08lx, 0x%08lx";


int main(int argc, char **argv)
{
	const char* outfname = NULL;
	const char* writeCPP = NULL;
	const char* writePRE = NULL;

	int c;
	while ((c = getopt(argc, argv, "o:c:E:")) != -1) {
		switch (c) {
			case 'o':
				outfname = optarg;
				break;
			case 'c':
				writeCPP = optarg;
				break;
			case 'E':
				writePRE = optarg;
				break;
		}
	}

	if (!outfname && !writeCPP && !writePRE) {
		cerr << "Usage: " << argv[0] << " [-o <bin-output>] [-c <c-output>] [-E <preprocessed>] <qasm-file>" << endl;
		return -1;
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
	} catch (const string& msg)
	{	cerr << msg << endl;
		return 1;
	}

	if (outfname)
	{	FILE* of = fopen(outfname, "wb");
		if (of == NULL)
		{	fprintf(stderr, "Failed to open %s for writing.", outfname);
			return -1;
		}
		fwrite(&*parser.GetInstructions().begin(), sizeof(uint64_t), parser.GetInstructions().size(), of);
		fclose(of);
	}

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

/*
    // Process relocations
    ctx.labels["ZERO"] = 0x0;
    for (int i=0; i < ctx.relocations.size(); i++)
    {
        relocation& r = ctx.relocations[i];
        if (ctx.labels.count(r.label) < 1)
        {
            cerr << "undefined label: " << r.label << endl;
            return -1;
        }
        int offset = ctx.labels[r.label] - (r.pc + 4*8);
        if (r.label == "ZERO")
            offset = 0x0;
        cout << "Processing relocation at " << r.pc << " : " << r.label
                                            << " : " << offset << endl;
        uint64_t ins = instructions[r.pc / 8];
        ins &= (uint64_t)0xFFFFFFFF << 32;   // zero bottom 32-bits for new value
        ins |= (uint32_t)offset;
        instructions[r.pc / 8] = ins;
    }
*/
	return 0;
}
