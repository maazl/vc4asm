FLAGS    = -Wall -std=c++11 -g
CPPFLAGS = -c
LDFLAGS  =
LIBS     = -lm -lstdc++
CC  = g++
LD  = gcc
EXE =
OBJ = .o

obj/%$(OBJ) : src/%.cpp
	$(CC) $(FLAGS) $(CPPFLAGS) -o $@ $<

BASEOBJECTS = obj/expr$(OBJ) obj/Inst$(OBJ) obj/Eval$(OBJ)
ASMOBJECTS  = $(BASEOBJECTS) obj/Parser$(OBJ) obj/vc4asm$(OBJ)
DISOBJECTS  = $(BASEOBJECTS) obj/Disassembler$(OBJ) obj/vc4dis$(OBJ)

all: bin/vc4asm$(EXE) bin/vc4dis$(EXE)

clean:
	-rm $(ASMOBJECTS) $(DISOBJECTS) bin/*

bin/vc4asm$(EXE) : $(ASMOBJECTS)
	$(LD) $(FLAGS) $(LDFLAGS) -o $@ $(ASMOBJECTS) $(LIBS)

bin/vc4dis$(EXE) : $(DISOBJECTS)
	$(LD) $(FLAGS) $(LDFLAGS) -o $@ $(DISOBJECTS) $(LIBS)

src/%.cpp : src/%.h
src/Parser.cpp : src/Parser.h src/Parser.tables.cpp
src/Disassembler.cpp : src/Disassembler.h src/Eval.h src/Disassembler.tables.cpp
src/vc4asm.cpp : src/Parser.h
src/vc4dis.cpp : src/Disassembler.h

src/Inst.h : src/expr.h
src/Eval.h : src/expr.h
src/Parser.h : src/Eval.h src/Inst.h
src/Disassembler.h : src/Inst.h
