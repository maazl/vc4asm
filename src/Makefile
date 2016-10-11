FLAGS    = -Wall -std=c++11 -g#-O3
CPPFLAGS = -c
LDFLAGS  =
LIBS     = -lm -lstdc++
CC  = g++
LD  = gcc
EXE =
O   = .o

../obj/%$(O) : %.cpp | ../obj
	$(CC) $(FLAGS) $(CPPFLAGS) -o $@ $<

../obj/%.s : %.cpp | ../obj
	$(CC) $(FLAGS) $(CPPFLAGS) -S -o $@ $<

BASEOBJECTS = ../obj/utils$(O) ../obj/Message$(O) ../obj/expr$(O) ../obj/Inst$(O) ../obj/Eval$(O) ../obj/Validator$(O)
ASMOBJECTS  = $(BASEOBJECTS) ../obj/AssembleInst$(O) ../obj/Parser$(O) ../obj/vc4asm$(O) ../obj/WriteELF$(O) ../obj/Disassembler$(O)
DISOBJECTS  = $(BASEOBJECTS) ../obj/Disassembler$(O) ../obj/vc4dis$(O)

all: ../bin/vc4asm$(EXE) ../bin/vc4dis$(EXE)

clean:
	-rm ../bin/* ../obj/*

../bin/vc4asm$(EXE) : $(ASMOBJECTS) | ../bin
	$(LD) $(FLAGS) $(LDFLAGS) -o $@ $(ASMOBJECTS) $(LIBS)

../bin/vc4dis$(EXE) : $(DISOBJECTS) | ../bin
	$(LD) $(FLAGS) $(LDFLAGS) -o $@ $(DISOBJECTS) $(LIBS)

../obj:
	mkdir $@

../bin:
	mkdir $@

%.cpp : %.h

../obj/utils$(O) : utils.cpp utils.h
../obj/Message$(O) : Message.cpp Message.h utils.h
../obj/expr$(O) : expr.cpp expr.h utils.h
../obj/Inst$(O) : Inst.cpp Inst.h Eval.h expr.h
../obj/Eval$(O) : Eval.cpp Eval.h Inst.h expr.h Message.h utils.h
../obj/AssembleInst$(O) : AssembleInst.cpp AssembleInst.h Inst.h expr.h Message.h utils.h AssembleInst.tables.cpp
../obj/Parser$(O) : Parser.cpp Parser.h AssembleInst.h DebugInfo.h Eval.h Inst.h expr.h Message.h utils.h Parser.tables.cpp
../obj/Validator$(O) : Validator.cpp Validator.h DebugInfo.h utils.h Inst.h expr.h
../obj/WriteELF$(O) : WriteELF.cpp WriteELF.h DebugInfo.h expr.h
../obj/Disassembler$(O) : Disassembler.cpp Disassembler.h Inst.h utils.h Disassembler.tables.cpp
../obj/vc4asm$(O) : vc4asm.cpp Parser.h AssembleInst.h Eval.h Inst.h expr.h Message.h utils.h Validator.h
../obj/vc4dis$(O) : vc4dis.cpp Disassembler.h Inst.h expr.h Validator.h utils.h

Inst.h : expr.h
Eval.h : expr.h
Parser.h : Eval.h Inst.h utils.h
Validators.h : Inst.h utils.h
Disassembler.h : Inst.h

