CFLAGS = -O3

S = hex/shader_256.hex \
    hex/shader_512.hex \
    hex/shader_1k.hex \
    hex/shader_2k.hex \
    hex/shader_4k.hex \
    hex/shader_8k.hex \
    hex/shader_16k.hex \
    hex/shader_32k.hex \
    hex/shader_64k.hex \
    hex/shader_128k.hex \
    hex/shader_256k.hex \
    hex/shader_512k.hex \
    hex/shader_1024k.hex \
    hex/shader_2048k.hex \
    hex/shader_4096k.hex \
    hex/shader_trans.hex

O = mailbox.o gpu_fft.o gpu_fft_base.o gpu_fft_twiddles.o gpu_fft_shaders.o

O1D = $(O) hello_fft.o
O2D = $(O) hello_fft_2d.o gpu_fft_trans.o

F = -lrt -lm -ldl

hex/shader_%.hex:	qasm/gpu_fft_%.qasm ../../share/vc4.qinc qasm/gpu_fft.qinc qasm/gpu_fft_ex.qinc
	../../bin/vc4asm -V -c $@ ../../share/vc4.qinc $<

hex/shader_%.dis:       hex/shader_%.hex
	../../bin/vc4dis -x -o $@ $<

%.o : %.c
	gcc $(CFLAGS) -o $@ -c $<

all: hello_fft.bin hello_fft_2d.bin

shader: $(S)

test: hello_fft.bin
	./test.pl

profile: hello_fft.bin
	./profile.pl

hex/shader_2048k.hex:	qasm/gpu_fft_2048k.qasm ../../share/vc4.qinc qasm/gpu_fft.qinc qasm/gpu_fft_ex.qinc qasm/gpu_fft_2048k.qinc
hex/shader_4096k.hex:	qasm/gpu_fft_4096k.qasm ../../share/vc4.qinc qasm/gpu_fft.qinc qasm/gpu_fft_ex.qinc qasm/gpu_fft_2048k.qinc

hello_fft.bin:	$(O1D)
	gcc $(CFLAGS) -o hello_fft.bin $(F) $(O1D)

hello_fft_2d.bin:	$(O2D)
	gcc $(CFLAGS) -o hello_fft_2d.bin $(F) $(O2D)

gpu_fft_trans.o:	gpu_fft_trans.c gpu_fft_trans.h gpu_fft.h
gpu_fft.o:	gpu_fft.c gpu_fft.h
gpu_fft_base.o:	gpu_fft_base.c gpu_fft.h mailbox.h
gpu_fft_twiddles.o:	gpu_fft_twiddles.c gpu_fft.h
hello_fft.o:	hello_fft.c gpu_fft.h mailbox.h
hello_fft_2d.o:	hello_fft_2d.c gpu_fft_trans.h gpu_fft.h mailbox.h hex/shader_trans.hex
mailbox.o:	mailbox.c mailbox.h
gpu_fft_shaders.o:	gpu_fft_shaders.c $(S)

clean:
	rm -f *.bin *.o hex/*.hex
