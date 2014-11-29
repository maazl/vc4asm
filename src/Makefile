FLAGS    = -Wall -std=c++11 -g
CPPFLAGS = -c
LDFLAGS  =
LIBS     = -lm -lstdc++
CC  = g++
LD  = gcc
EXE =
OBJ = .o

../obj/%$(OBJ) : %.cpp
	$(CC) $(FLAGS) $(CPPFLAGS) -o $@ $<

BASEOBJECTS = ../obj/utils$(OBJ) ../obj/expr$(OBJ) ../obj/Inst$(OBJ) ../obj/Eval$(OBJ) ../obj/Validator$(OBJ)
ASMOBJECTS  = $(BASEOBJECTS) ../obj/Parser$(OBJ) ../obj/vc4asm$(OBJ)
DISOBJECTS  = $(BASEOBJECTS) ../obj/Disassembler$(OBJ) ../obj/vc4dis$(OBJ)

all: ../bin/vc4asm$(EXE) ../bin/vc4dis$(EXE)

clean:
	-rm ../bin/* ../obj/*

../bin/vc4asm$(EXE) : $(ASMOBJECTS)
	$(LD) $(FLAGS) $(LDFLAGS) -o $@ $(ASMOBJECTS) $(LIBS)

../bin/vc4dis$(EXE) : $(DISOBJECTS)
	$(LD) $(FLAGS) $(LDFLAGS) -o $@ $(DISOBJECTS) $(LIBS)

%.cpp : %.h
expr.cpp : expr.h utils.h
Eval.cpp : Eval.h utils.h
Parser.cpp : Parser.h Parser.tables.cpp
Disassembler.cpp : Disassembler.h utils.h Disassembler.tables.cpp
vc4asm.cpp : Parser.h Validator.h
vc4dis.cpp : Disassembler.h Validator.h

Inst.h : expr.h
Eval.h : expr.h
Parser.h : Eval.h Inst.h
Disassembler.h : Inst.h

