all : smitest

%.o : %.c
	gcc -Wall -c -o $@ $<

smitest : smitest.o mailbox.o
	gcc -o $@ -Wall $^

smitest.o : smitest.c mailbox.h smitest.hex

mailbox.c : mailbox.h

smitest.hex : smitest.qasm
	../../bin/vc4asm -V -C $@ ../../share/vc4.qinc $<

clean :
	rm smitest.hex *.o
