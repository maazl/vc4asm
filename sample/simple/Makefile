BINPATH=../../build-Linux-armv6l-Debug

all : smitest

%.o : %.c
	gcc -Wall -c -o $@ $<

smitest : smitest.o mailbox.o shader.o
	gcc -o $@ -Wall $^

smitest.o : smitest.c mailbox.h numbers.hex shader.h

mailbox.c : mailbox.h

shader.o shader.h : smitest.qasm
	$(BINPATH)/vc4asm -V -e $(subst .h,.o,$@) -h $(subst .o,.h,$@) -i vc4.qinc $<

smitest.dis : smitest.hex
	$(BINPATH)/vc4dis -v2 -M -x $< -o $@

clean :
	rm smitest.hex *.o

shader : smitest.hex
