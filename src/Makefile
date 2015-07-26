FLAGS    = -Wall -std=c++11 -g
CPPFLAGS = -c
LDFLAGS  =
LIBS     = -lm -lstdc++
CC  = g++-4.8
LD  = gcc-4.8
EXE =
O = .o

../obj/%$(O) : %.cpp ../obj
	$(CC) $(FLAGS) $(CPPFLAGS) -o $@ $<

BASEOBJECTS = ../obj/utils$(O) ../obj/expr$(O) ../obj/Inst$(O) ../obj/Eval$(O) ../obj/Validator$(O)
ASMOBJECTS  = $(BASEOBJECTS) ../obj/Parser$(O) ../obj/vc4asm$(O)
DISOBJECTS  = $(BASEOBJECTS) ../obj/Disassembler$(O) ../obj/vc4dis$(O)

all: ../bin/vc4asm$(EXE) ../bin/vc4dis$(EXE)

clean:
	-rm ../bin/* ../obj/*

../obj :
	mkdir ../obj

../bin/vc4asm$(EXE) : $(ASMOBJECTS)
	$(LD) $(FLAGS) $(LDFLAGS) -o $@ $(ASMOBJECTS) $(LIBS)

../bin/vc4dis$(EXE) : $(DISOBJECTS)
	$(LD) $(FLAGS) $(LDFLAGS) -o $@ $(DISOBJECTS) $(LIBS)

%.cpp : %.h
utils$(O) : utils.cpp utils.h
expr$(O) : expr.cpp expr.h utils.h
Inst$(O) : Inst.cpp Inst.h expr.h
Eval$(O) : Eval.cpp Eval.h Inst.h expr.h utils.h
Parser$(O) : Parser.cpp Parser.h Eval.h Inst.h expr.h utils.h Parser.tables.cpp
Validator$(O) : Validator.cpp Validator.h utils.h Inst.h expr.h
Disassembler$(O) : Disassembler.cpp Disassembler.h Inst.h utils.h Disassembler.tables.cpp
vc4asm$(O) : vc4asm.cpp Parser.h Eval.h Inst.h expr.h utils.h Validator.h
vc4dis$(O) : vc4dis.cpp Disassembler.h Inst.h expr.h Validator.h utils.h

Inst.h : expr.h
Eval.h : expr.h
Parser.h : Eval.h Inst.h utils.h
Validators.h : Inst.h utils.h
Disassembler.h : Inst.h

