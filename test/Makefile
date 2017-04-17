vc4asmbinpath = ../build-Linux-armv6l-Debug

all : asm cout parser validator

asm : test_256 test_512 test_1k test_2k test_4k test_8k test_16k test_32k test_64k test_128k test_256k test_512k test_1024k test_2048k test_trans test_256_new

cout : testc_rpi_shader testh_rpi_shader

parser : testp.rot.hex testp.pup.hex

validator : testv.VPM.hex

clean :
	rm gpu_fft_*.hex *.strip

.SECONDARY :

%.strip : %.hex
	sed 's/\s*\/\/.*//' $< >$@

test_% : gpu_fft_%.hex shader_%.strip
	diff $^ >$@

%.strip.c : %.c
	perl -pe 's"//  *(.*?)(?:\s*#.*|$$)"// \1"' $< >$@

testc_% : %.strip.c %.ref.c
	diff $^ >$@

testh_% : %.h %.ref.h
	diff $^ >$@

%.c : %.qasm $(vc4asmbinpath)/vc4asm
	$(vc4asmbinpath)/vc4asm -v -c $@ -h $(subst .c,.h,$@) -i vc4.qinc $<

gpu_fft_%.hex : gpu_fft_%.qasm gpu_fft.qinc gpu_fft_ex.qinc $(vc4asmbinpath)/vc4asm
	$(vc4asmbinpath)/vc4asm -V -C $@ -i vc4.qinc $<

%.dis : %.hex $(vc4asmbinpath)/vc4dis
	$(vc4asmbinpath)/vc4dis -v -x $< >$@

test.strip : test.hex

test : test.hex test.strip
	diff $^

test.hex : test.qasm $(vc4asmbinpath)/vc4asm
	$(vc4asmbinpath)/vc4asm -V -c $@ -i vc4.qinc $<

testp.%.hex : parser.%.qasm $(vc4asmbinpath)/vc4asm
	-$(vc4asmbinpath)/vc4asm -V -Q -c $@ -i vc4.qinc $< >parser.out 2>&1
	./checkOutput.pl $< <parser.out

testv.%.hex : validator.%.qasm $(vc4asmbinpath)/vc4asm
	-$(vc4asmbinpath)/vc4asm -V -c $@ -i vc4.qinc $< >parser.out 2>&1
	./checkOutput.pl $< <parser.out
