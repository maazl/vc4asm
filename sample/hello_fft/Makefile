CC = arm-linux-gnueabihf-gcc

H = hex/shader_256.hex \
    hex/shader_512.hex \
    hex/shader_1k.hex \
    hex/shader_2k.hex \
    hex/shader_4k.hex \
    hex/shader_8k.hex \
    hex/shader_16k.hex \
    hex/shader_32k.hex \
    hex/shader_64k.hex \
    hex/shader_128k.hex

C = hello_fft.c mailbox.c gpu_fft.c gpu_fft_twiddles.c gpu_fft_shaders.c

B = hello_fft.bin

F = -lrt -lm -o $(B)

hex/shader_%.hex : qasm/gpu_fft_%.qasm ../../share/vc4.qinc qasm/gpu_fft.qinc
	../../bin/vc4asm -V -c $@ ../../share/vc4.qinc $<

$(B):	$(H) $(C)
	$(CC) $(C) $(F)

clean:
	rm -f $(B) $(H)
