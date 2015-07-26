all : smitest

%.o : %.c
	gcc -Wall -c -o $@ $<

smitest : smitest.o mailbox.o
	gcc -o $@ -Wall $^

smitest.o : smitest.c mailbox.h smitest.hex numbers.hex

mailbox.c : mailbox.h

smitest.hex : smitest.qasm
	../../bin/vc4asm -V -C $@ ../../share/vc4.qinc $<

smitest.dis : smitest.hex
	../../bin/vc4dis -v2 -M -x $< -o $@

clean :
	rm smitest.hex *.o

shader : smitest.hex
